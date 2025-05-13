import subprocess
from flask import Flask, render_template, jsonify, request
import json
import sqlite3
from dataclasses import dataclass
from openai import OpenAI
from dotenv import load_dotenv
import os

load_dotenv()
api_key = os.getenv("API_KEY")
client = OpenAI(api_key=api_key)

app = Flask(__name__)

@dataclass
class Contact:
    name: str
    email: str
    phone: str
    location: str
    current_company: str
    previous_companies: str
    industry: str
    job_title: str
    relationship_type: str
    closeness: str
    reliability: str
    interests: str
    college: str
    high_school: str
    career_goals: str
    skills: str
    talent_rating: str

@app.route("/")
def index():
    return render_template("index.html")

@app.route('/edges/', defaults={'feature': ''})

@app.route('/edges/<feature>')
def get_edges(feature):
    result = subprocess.run(['./generate_edges', feature], capture_output=True, text=True)
    print("[CPP STDOUT]", result.stdout.strip())
    print("[CPP STDERR]", result.stderr.strip())

    
    if result.returncode != 0:
        print("[CPP ERROR]", result.stderr)
        return jsonify({"error": "C++ backend failed", "details": result.stderr}), 500

    try:
        print("[CPP STDOUT]", result.stdout)
        output_json = json.loads(result.stdout)
        return jsonify(output_json)
    except json.JSONDecodeError as e:
        print("[JSON ERROR]", e)
        print("Raw output:", result.stdout)
        return jsonify({"error": "Invalid JSON from C++"}), 500


def load_all_contacts(db_path="my_database.db"):
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()

    query = """
    SELECT 
        contacts.name,
        contacts.email,
        contacts.phone,
        contacts.location,
        employment.current_company,
        employment.previous_companies,
        employment.industry,
        employment.job_title,
        relationships.relationship_type,
        relationships.closeness,
        relationships.reliability,
        background.interests,
        background.college,
        background.high_school,
        profile.career_goals,
        profile.skills,
        profile.talent_rating
    FROM contacts
    LEFT JOIN employment ON contacts.id = employment.contact_id
    LEFT JOIN relationships ON contacts.id = relationships.contact_id
    LEFT JOIN background ON contacts.id = background.contact_id
    LEFT JOIN profile ON contacts.id = profile.contact_id
    """

    cur.execute(query)
    rows = cur.fetchall()

    contacts = []
    for row in rows:
        contacts.append(Contact(
            name=row[0] or "Unnamed",
            email=row[1] or "",
            phone=row[2] or "",
            location=row[3] or "",
            current_company=row[4] or "",
            previous_companies=row[5] or "",
            industry=row[6] or "",
            job_title=row[7] or "",
            relationship_type=row[8] or "",
            closeness=row[9] or "",
            reliability=row[10] or "",
            interests=row[11] or "",
            college=row[12] or "",
            high_school=row[13] or "",
            career_goals=row[14] or "",
            skills=row[15] or "",
            talent_rating=row[16] or ""
        ))

    conn.close()
    return contacts


@app.route("/ask", methods=["POST"])
def ask():
    question = request.json.get("question", "")
    
    contacts = load_all_contacts()

    prompt = "You are analyzing a contact network. Here are the contacts:\n\n"
    for c in contacts:
        # prompt += f"name={c.name}: interests={c.interests}, location={c.location}, skills={c.skills}, goals={c.career_goals}\n"
        # prompt += f"phone={c.phone}, email={c.email}, current company={c.current_company}, previous companies={c.previous_companies}\n"
        # prompt += f"industry={c.industry}, job title={c.job_title}, relationship type={c.relationship_type}, closeness={c.closeness}, reliability={c.reliability}\n"
        # prompt += f"college={c.college}, high school={c.high_school}, talent rating={c.talent_rating}"
        prompt += f"""
            Contact: {c.name}
            - Interests: {c.interests}
            - Location: {c.location}
            - Skills: {c.skills}
            - Career Goals: {c.career_goals}
            - Phone: {c.phone}
            - Email: {c.email}
            - Current Company: {c.current_company}
            - Previous Companies: {c.previous_companies}
            - Industry: {c.industry}
            - Job Title: {c.job_title}
            - Relationship Type: {c.relationship_type}
            - Closeness: {c.closeness}
            - Reliability: {c.reliability}
            - College: {c.college}
            - High School: {c.high_school}
            - Talent Rating: {c.talent_rating}
            """

    prompt += f"\nAnswer the following question:\n{question} based on the information of the people in the contact network you received above, make an informed decision after reading all of the data and give a detailed response"

    try:
        response = client.chat.completions.create(
            model="gpt-4",
            messages=[{"role": "user", "content": prompt}],
            temperature=0.5
        )
        answer = response.choices[0].message.content
        return jsonify({"answer": answer})
    except Exception as e:
        return jsonify({"answer": f"Error: {str(e)}"}), 500

@app.route("/data")
def data():
    conn = sqlite3.connect("my_database.db")
    cursor = conn.cursor()
    cursor.execute("SELECT name FROM contacts")
    names = cursor.fetchall()
    conn.close()

    nodes = [{"id": i, "name": name[0]} for i, name in enumerate(names)]

    return jsonify({"nodes": nodes})

@app.route("/add_contact", methods=["POST"])
def add_contact():
    data = request.get_json()
    print("[DEBUG] Received:", data)

    try:
        conn = sqlite3.connect("my_database.db")
        cur = conn.cursor()

        # Check if contact with the same name already exists
        cur.execute("SELECT id FROM contacts WHERE name = ?", (data["name"],))
        result = cur.fetchone()

        if result:
            contact_id = result[0]
            print("[INFO] Contact exists. Updating:", contact_id)

            # Update contacts (overwrite phone)
            cur.execute("""
                UPDATE contacts
                SET email = ?, phone = ?, location = ?
                WHERE id = ?
            """, (data["email"], data["phone"], data["location"], contact_id))

            # Update employment: append previous_companies and industry
            cur.execute("SELECT previous_companies, industry FROM employment WHERE contact_id = ?", (contact_id,))
            old_emp = cur.fetchone()
            old_prev = old_emp[0] if old_emp and old_emp[0] else ""
            old_industry = old_emp[1] if old_emp and old_emp[1] else ""

            def merge_field(existing, new_val):
                old_set = set(i.strip() for i in existing.split(',')) if existing else set()
                new_set = set(i.strip() for i in new_val.split(',')) if new_val else set()
                return ', '.join(sorted(old_set.union(new_set)))

            merged_prev = merge_field(old_prev, data["previous_companies"])
            merged_industry = merge_field(old_industry, data["industry"])

            cur.execute("""
                UPDATE employment
                SET current_company = ?, previous_companies = ?, industry = ?, job_title = ?
                WHERE contact_id = ?
            """, (data["current_company"], merged_prev, merged_industry, data["job_title"], contact_id))

            # Update relationships
            cur.execute("""
                UPDATE relationships
                SET relationship_type = ?, closeness = ?, reliability = ?
                WHERE contact_id = ?
            """, (data["relationship_type"], data["closeness"], data["reliability"], contact_id))

            # Update profile: append skills and career goals
            cur.execute("SELECT skills, career_goals FROM profile WHERE contact_id = ?", (contact_id,))
            old_profile = cur.fetchone()
            old_skills = old_profile[0] if old_profile and old_profile[0] else ""
            old_goals = old_profile[1] if old_profile and old_profile[1] else ""

            merged_skills = merge_field(old_skills, data["skills"])
            merged_goals = merge_field(old_goals, data["career_goals"])

            cur.execute("""
                UPDATE profile
                SET skills = ?, talent_rating = ?, career_goals = ?
                WHERE contact_id = ?
            """, (merged_skills, data["talent_rating"], merged_goals, contact_id))

            # Update background: append interests
            cur.execute("SELECT interests FROM background WHERE contact_id = ?", (contact_id,))
            existing_interests = cur.fetchone()
            existing = existing_interests[0] if existing_interests and existing_interests[0] else ""
            merged_interests = merge_field(existing, data["interests"])

            cur.execute("""
                UPDATE background
                SET interests = ?, college = ?, high_school = ?
                WHERE contact_id = ?
            """, (merged_interests, data["college"], data["high_school"], contact_id))

            conn.commit()
            conn.close()
            return jsonify({"success": True, "message": "Contact updated."})

        else:
            # Insert into contacts
            cur.execute("""
                INSERT INTO contacts (name, email, phone, location)
                VALUES (?, ?, ?, ?)
            """, (data["name"], data["email"], data["phone"], data["location"]))
            contact_id = cur.lastrowid

            # Insert into other tables
            cur.execute("""
                INSERT INTO employment (contact_id, current_company, previous_companies, industry, job_title)
                VALUES (?, ?, ?, ?, ?)
            """, (contact_id, data["current_company"], data["previous_companies"], data["industry"], data["job_title"]))

            cur.execute("""
                INSERT INTO relationships (contact_id, relationship_type, closeness, reliability)
                VALUES (?, ?, ?, ?)
            """, (contact_id, data["relationship_type"], data["closeness"], data["reliability"]))

            cur.execute("""
                INSERT INTO profile (contact_id, career_goals, skills, talent_rating)
                VALUES (?, ?, ?, ?)
            """, (contact_id, data["career_goals"], data["skills"], data["talent_rating"]))

            cur.execute("""
                INSERT INTO background (contact_id, interests, college, high_school)
                VALUES (?, ?, ?, ?)
            """, (contact_id, data["interests"], data["college"], data["high_school"]))

            conn.commit()
            conn.close()
            return jsonify({"success": True, "message": "Contact added."})
    except Exception as e:
        print("[ERROR]", e)
        return jsonify({"error": str(e)}), 500

if __name__ == "__main__":
    app.run(debug=True)


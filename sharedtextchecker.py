import sqlite3
from dataclasses import dataclass
from itertools import combinations
import numpy as np
from collections import defaultdict
from openai import OpenAI

from dotenv import load_dotenv
import os

load_dotenv()
api_key = os.getenv("API_KEY")
client = OpenAI(api_key=api_key)

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
    relationship: str
    closeness: str
    reliability: str
    interests: str
    college: str
    high_school: str
    career_goals: str
    skills: str
    talent_rating: str

def load_all_contacts(db_path="my_database.db"):
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()

    query = """
        SELECT 
            contacts.name, contacts.email, contacts.phone, contacts.location,
            employment.current_company, employment.previous_companies, employment.industry, employment.job_title,
            relationships.relationship_type, relationships.closeness, relationships.reliability,
            background.interests, background.college, background.high_school,
            profile.career_goals, profile.skills, profile.talent_rating
        FROM contacts
        LEFT JOIN employment ON contacts.id = employment.contact_id
        LEFT JOIN background ON contacts.id = background.contact_id
        LEFT JOIN profile ON contacts.id = profile.contact_id
        LEFT JOIN relationships ON contacts.id = relationships.contact_id;
    """

    cur.execute(query)
    rows = cur.fetchall()
    conn.close()

    return [Contact(*row) for row in rows]

def openai_get_interests_in_one_req(comparisons):
    if not comparisons:
        return "array is empty"

    formatted_lines = [
        f'["{p1}", "{i1}", "{p2}", "{i2}"]'
        for (p1, i1, p2, i2) in comparisons
    ]
    formatted_array = "\n".join(formatted_lines)

    prompt = (
        "Compare the interests between people based on this input (they dont have to be exactly the same, as long as they are similar such as math vs competitive math). Each entry is:\n"
        "[person1 name, person1 interests, person2 name, person2 interests]\n\n"
        f"{formatted_array}\n\n"
        "Now output ONLY the results as raw arrays (no explanation, no code block), like:\n"
        "[\"Alice\", \"Bob\", \"math, chess\"]\n"
        "[\"Ben\", \"John\", \"None\"]\n"
        "...\n"
        "Don't wrap anything in code blocks or markdown."
    )


    try:
        response = client.chat.completions.create(
            model="gpt-4o",
            messages=[
                {"role": "system", "content": "You are a helpful assistant."},
                {"role": "user", "content": prompt}
            ],
            temperature=0
        )
        return response.choices[0].message.content.strip()
    except Exception as e:
        print(f"Error during comparison: {e}")
        return None

def openai_find_common_interests(name1, interests1, name2, interests2):
    if not interests1.strip() or not interests2.strip():
        return None

    prompt = (
        f"Person A ({name1}) has these interests: {interests1}\n"
        f"Person B ({name2}) has these interests: {interests2}\n"
        "List all shared or overlapping interests in a comma-separated list. "
        "If there are none, say 'None'."
    )

    try:
        response = client.chat.completions.create(
            model="gpt-4o",
            messages=[
                {"role": "system", "content": "You are a helpful assistant."},
                {"role": "user", "content": prompt}
            ],
            temperature=0
        )
        answer = response.choices[0].message.content.strip()
        if answer.lower() == "none":
            return None
        return [interest.strip() for interest in answer.split(",") if interest.strip()]
    except Exception as e:
        print(f"Error comparing {name1} and {name2}: {e}")
        return None

def compare_all_contacts_and_store(contacts, db_path="interests_edges.db"):
    # Prepare DB
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()
    cur.execute("""
        CREATE TABLE IF NOT EXISTS interest_edges (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            person1 TEXT,
            person2 TEXT,
            shared_interests TEXT
        );
    """)

    insert_batch = []
    combos = []

    for c1, c2 in combinations(contacts, 2):
        if c1.interests and c2.interests:
            combos.append([c1.name, c1.interests, c2.name, c2.interests])

    result = openai_get_interests_in_one_req(combos)

    print(result + " = result")

    if result:
        for line in result.strip().split("\n"):
            if not line.startswith("[") or not line.endswith("]"):
                continue
            try:
                parts = eval(line)  # safe for trusted input only
                if len(parts) == 3 and parts[2].strip().lower() != "none":
                    insert_batch.append((parts[0], parts[1], parts[2]))
            except Exception as e:
                print(f"Skipping line due to error: {line} — {e}")

    if insert_batch:
        cur.executemany("""
        INSERT OR IGNORE INTO interest_edges (person1, person2, shared_interests)
        VALUES (?, ?, ?);
    """, insert_batch)

        conn.commit()

    conn.close()
    print(f"Inserted {len(insert_batch)} rows into interest_edges.db")


def openai_get_goals_in_one_req(comparisons):
    if not comparisons:
        return "array is empty"

    formatted_lines = [
        f'["{p1}", "{i1}", "{p2}", "{i2}"]'
        for (p1, i1, p2, i2) in comparisons
    ]
    formatted_array = "\n".join(formatted_lines)

    prompt = (
        "You will receive a list of entries. Each entry is an array in this format:\n"
        "[person1 name, person1 career goals, person2 name, person2 career goals]\n\n"
        f"{formatted_array}\n\n"
        "For each line, determine if person1 and person2 share a similar career goal. "
        "Even if the wording is different (e.g. 'become muscular' vs. 'bench 215'), treat them as similar if they point to the same general idea. "
        "If they share a goal, return a new array: [person1 name, person2 name, shared generalized goal]. "
        "If there is no meaningful shared goal, do not return anything for that pair.\n\n"
        "Only return the resulting arrays, one per line, no explanations or markdown. Example:\n"
        "[\"Alice\", \"Bob\", \"get an internship\"]\n"
        "[\"Ben\", \"John\", \"start a startup\"]"
    )


    try:
        response = client.chat.completions.create(
            model="gpt-4o",
            messages=[
                {"role": "system", "content": "You are a helpful assistant."},
                {"role": "user", "content": prompt}
            ],
            temperature=0
        )
        return response.choices[0].message.content.strip()
    except Exception as e:
        print(f"Error during comparison: {e}")
        return None


def compare_all_goals_and_store(contacts, db_path="goals_edges.db"):
    # Prepare DB
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()
    cur.execute("""
        CREATE TABLE IF NOT EXISTS share_goals (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            person1 TEXT,
            person2 TEXT,
            shared_goal TEXT,
            UNIQUE(person1, person2)
        );
    """)

    insert_batch = []
    combos = []

    for c1, c2 in combinations(contacts, 2):
        if c1.career_goals and c2.career_goals:
            combos.append([c1.name, c1.career_goals, c2.name, c2.career_goals])

    result = openai_get_goals_in_one_req(combos)

    print(result + " = result")

    if result:
        for line in result.strip().split("\n"):
            if not line.startswith("[") or not line.endswith("]"):
                continue
            try:
                parts = eval(line)  # safe for trusted input only
                if len(parts) == 3 and parts[2].strip().lower() != "none":
                    insert_batch.append((parts[0], parts[1], parts[2]))
            except Exception as e:
                print(f"skipping line due to error: {line} — {e}")

    if insert_batch:
        cur.executemany("""
        INSERT OR IGNORE INTO share_goals (person1, person2, shared_goal)
        VALUES (?, ?, ?);
    """, insert_batch)

        conn.commit()

    conn.close()
    print(f"Inserted {len(insert_batch)} rows into goals_edges.db")



def group_by_current_company(contacts, db_path = "company_groups.db"):
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()

    groups = defaultdict(list)

    for contact in contacts:
        if contact.current_company:
            companies = [c.strip().lower() for c in contact.current_company.split(",") if c.strip()]
        else:
            companies = ["unknown"]

        for company in companies:
            groups[company].append(contact.name)

    insert_batch = [
        (company, ", ".join(people))
        for company, people in groups.items()
    ]

    cur.executemany("""
        INSERT OR IGNORE INTO company_groups (company, people)
        VALUES (?, ?);
    """, insert_batch)


    conn.commit()
    conn.close()

    print(f"Inserted {len(insert_batch)} company groups into {db_path}")

    return 0


def group_by_previous_company(contacts, db_path = "previous_companies.db"):
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()

    groups = defaultdict(list)

    for contact in contacts:
        if contact.previous_companies:
            companies = [c.strip().lower() for c in contact.previous_companies.split(",") if c.strip()]
        else:
            companies = ["unknown"]

        for company in companies:
            groups[company].append(contact.name)

    insert_batch = [
        (company, ", ".join(people))
        for company, people in groups.items()
    ]

    cur.executemany("""
        INSERT OR IGNORE INTO previous_companies (company, people)
        VALUES (?, ?);
    """, insert_batch)


    conn.commit()
    conn.close()

    print(f"Inserted {len(insert_batch)} company groups into {db_path}")

    return 0


def openai_get_skills_in_one_req(comparisons):
    if not comparisons:
        return "array is empty"

    formatted_lines = [
        f'["{p1}", "{i1}", "{p2}", "{i2}"]'
        for (p1, i1, p2, i2) in comparisons
    ]
    formatted_array = "\n".join(formatted_lines)

    prompt = (
        "You will receive a list of entries. Each entry is an array in this format:\n"
        "[person1 name, person1 skills, person2 name, person2 skills]\n\n"
        f"{formatted_array}\n\n"
        "For each line, determine if person1 and person2 share a similar skill. "
        "Even if the wording is different (e.g. 'coding in java' vs. 'building a backend (since java is a backend language)'), treat them as similar if they point to the same general idea. "
        "If they share a skill, return a new array: [person1 name, person2 name, shared generalized skill]. "
        "If there is no meaningful shared skill, do not return anything for that pair.\n\n"
        "Only return the resulting arrays, one per line, no explanations or markdown. Example:\n"
        "[\"Alice\", \"Bob\", \"problem solving\"]\n"
        "[\"Ben\", \"John\", \"leading a team of 20 people\"]"
    )


    try:
        response = client.chat.completions.create(
            model="gpt-4o",
            messages=[
                {"role": "system", "content": "You are a helpful assistant."},
                {"role": "user", "content": prompt}
            ],
            temperature=0
        )
        return response.choices[0].message.content.strip()
    except Exception as e:
        print(f"Error during comparison: {e}")
        return None


def compare_all_skills_and_store(contacts, db_path="skill_edges.db"):
    # Prepare DB
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()
    cur.execute("""
        CREATE TABLE IF NOT EXISTS shared_skills (
            id INTEGER PRIMARY KEY AUTOINCREMENT,
            person1 TEXT,
            person2 TEXT,
            shared_skill TEXT,
            UNIQUE(person1, person2)
        );
    """)

    insert_batch = []
    combos = []

    for c1, c2 in combinations(contacts, 2):
        if c1.skills and c2.skills:
            combos.append([c1.name, c1.skills, c2.name, c2.skills])

    result = openai_get_goals_in_one_req(combos)

    print(result + " = result")

    if result:
        for line in result.strip().split("\n"):
            if not line.startswith("[") or not line.endswith("]"):
                continue
            try:
                parts = eval(line)  # safe for trusted input only
                if len(parts) == 3 and parts[2].strip().lower() != "none":
                    insert_batch.append((parts[0], parts[1], parts[2]))
            except Exception as e:
                print(f"Skipping line due to error: {line} — {e}")

    if insert_batch:
        cur.executemany("""
        INSERT OR IGNORE INTO shared_skills (person1, person2, shared_skills)
        VALUES (?, ?, ?);
    """, insert_batch)

        conn.commit()

    conn.close()
    print(f"Inserted {len(insert_batch)} rows into skill_edges.db")




#have industry, college, and high school left, can do all three of these the same way we did current company and previous company


def group_by_college(contacts, db_path = "college_groups.db"):
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()

    groups = defaultdict(list)

    for contact in contacts:
        if contact.college:
            companies = [c.strip().lower() for c in contact.college.split(",") if c.strip()]
        else:
            companies = ["unknown"]

        for company in companies:
            groups[company].append(contact.name)

    insert_batch = [
        (company, ", ".join(people))
        for company, people in groups.items()
    ]

    cur.executemany("""
        INSERT OR IGNORE INTO college_groups (college, people)
        VALUES (?, ?);
    """, insert_batch)


    conn.commit()
    conn.close()

    print(f"Inserted {len(insert_batch)} college groups into {db_path}")

    return 0




def group_by_highschool(contacts, db_path = "highschool_groups.db"):
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()

    groups = defaultdict(list)

    for contact in contacts:
        if contact.high_school:
            companies = [c.strip().lower() for c in contact.high_school.split(",") if c.strip()]
        else:
            companies = ["unknown"]

        for company in companies:
            groups[company].append(contact.name)

    insert_batch = [
        (company, ", ".join(people))
        for company, people in groups.items()
    ]

    cur.executemany("""
        INSERT OR IGNORE INTO highschool_groups (highschool, people)
        VALUES (?, ?);
    """, insert_batch)


    conn.commit()
    conn.close()

    print(f"Inserted {len(insert_batch)} highschool groups into {db_path}")

    return 0






def group_by_industry(contacts, db_path = "industry_groups.db"):
    conn = sqlite3.connect(db_path)
    cur = conn.cursor()

    groups = defaultdict(list)

    for contact in contacts:
        if contact.industry:
            companies = [c.strip().lower() for c in contact.industry.split(",") if c.strip()]
        else:
            companies = ["unknown"]

        for company in companies:
            groups[company].append(contact.name)

    insert_batch = [
        (company, ", ".join(people))
        for company, people in groups.items()
    ]

    cur.executemany("""
        INSERT OR IGNORE INTO industry_groups (industry, people)
        VALUES (?, ?);
    """, insert_batch)


    conn.commit()
    conn.close()

    print(f"Inserted {len(insert_batch)} industry groups into {db_path}")

    return 0



if __name__ == "__main__":
    contacts = load_all_contacts()
    print("📦 Loaded", len(contacts), "contacts.")
    #compare_all_contacts_and_store(contacts)
    compare_all_goals_and_store(contacts)
    #group_by_current_company(contacts)
    #group_by_previous_company(contacts)
    #compare_all_skills_and_store(contacts)
    # group_by_college(contacts)
    # group_by_highschool(contacts)
    # group_by_industry(contacts)

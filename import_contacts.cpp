#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>
#include <string>
#include <sqlite3.h>

struct Contact {
    std::string name, email, phone, location;
    std::string current_company, previous_companies, industry, job_title;
    std::string relationship_type;
    std::string closeness, reliability;
    std::string interests, college, high_school;
    std::string career_goals, skills;
    std::string talent_rating;
};

std::vector<Contact> read_contacts(const std::string& filename) {
    std::ifstream file(filename);
    std::string line;
    std::vector<Contact> contacts;

    while (std::getline(file, line)) {
        std::stringstream ss(line);
        Contact c;
        std::string segment;
        std::vector<std::string> fields;

        while (std::getline(ss, segment, '|')) {
            // Trim leading/trailing whitespace
            segment.erase(0, segment.find_first_not_of(" \t\n\r"));
            segment.erase(segment.find_last_not_of(" \t\n\r") + 1);
            fields.push_back(segment);
        }

        if (fields.size() < 17) continue; // not enough data

        c.name = fields[0];
        c.email = fields[1];
        c.phone = fields[2];
        c.location = fields[3];

        c.current_company = fields[4];
        c.previous_companies = fields[5];
        c.industry = fields[6];
        c.job_title = fields[7];

        c.relationship_type = fields[8];
        c.closeness = fields[9];
        c.reliability = fields[10];

        c.interests = fields[11];
        c.college = fields[12];
        c.high_school = fields[13];

        c.career_goals = fields[14];
        c.skills = fields[15];
        c.talent_rating = fields[16];

        contacts.push_back(c);
    }

    return contacts;
}

void insert_contacts_to_db(const std::vector<Contact>& contacts, const std::string& db_path) {
    sqlite3* db;
    if (sqlite3_open(db_path.c_str(), &db)) {
        std::cerr << "Cannot open database: " << sqlite3_errmsg(db) << std::endl;
        return;
    }

    for (const auto& c : contacts) {
        sqlite3_stmt* stmt;

        // Insert into contacts
        std::string sql_contacts = "INSERT INTO contacts (name, email, phone, location) VALUES (?, ?, ?, ?);";
        sqlite3_prepare_v2(db, sql_contacts.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_text(stmt, 1, c.name.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 2, c.email.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, c.phone.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, c.location.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        int contact_id = (int)sqlite3_last_insert_rowid(db);

        // Insert into employment
        std::string sql_emp = "INSERT INTO employment (contact_id, current_company, previous_companies, industry, job_title) VALUES (?, ?, ?, ?, ?);";
        sqlite3_prepare_v2(db, sql_emp.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, contact_id);
        sqlite3_bind_text(stmt, 2, c.current_company.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, c.previous_companies.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, c.industry.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 5, c.job_title.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // Insert into relationships
        std::string sql_rel = "INSERT INTO relationships (contact_id, relationship_type, closeness, reliability) VALUES (?, ?, ?, ?);";
        sqlite3_prepare_v2(db, sql_rel.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, contact_id);
        sqlite3_bind_text(stmt, 2, c.relationship_type.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, c.closeness.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, c.reliability.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // Insert into background
        std::string sql_back = "INSERT INTO background (contact_id, interests, college, high_school) VALUES (?, ?, ?, ?);";
        sqlite3_prepare_v2(db, sql_back.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, contact_id);
        sqlite3_bind_text(stmt, 2, c.interests.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, c.college.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, c.high_school.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);

        // Insert into profile
        std::string sql_prof = "INSERT INTO profile (contact_id, career_goals, skills, talent_rating) VALUES (?, ?, ?, ?);";
        sqlite3_prepare_v2(db, sql_prof.c_str(), -1, &stmt, nullptr);
        sqlite3_bind_int(stmt, 1, contact_id);
        sqlite3_bind_text(stmt, 2, c.career_goals.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 3, c.skills.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_bind_text(stmt, 4, c.talent_rating.c_str(), -1, SQLITE_TRANSIENT);
        sqlite3_step(stmt);
        sqlite3_finalize(stmt);
    }

    sqlite3_close(db);
}

std::ostream& operator<<(std::ostream& os, const Contact& c) {
    os << "Name: " << c.name << "\n"
       << "Email: " << c.email << "\n"
       << "Phone: " << c.phone << "\n"
       << "Location: " << c.location << "\n"
       << "Current Company: " << c.current_company << "\n"
       << "Previous Companies: " << c.previous_companies << "\n"
       << "Industry: " << c.industry << "\n"
       << "Job Title: " << c.job_title << "\n"
       << "Relationship Type: " << c.relationship_type << "\n"
       << "Closeness: " << c.closeness << "\n"
       << "Reliability: " << c.reliability << "\n"
       << "Interests: " << c.interests << "\n"
       << "College: " << c.college << "\n"
       << "High School: " << c.high_school << "\n"
       << "Career Goals: " << c.career_goals << "\n"
       << "Skills: " << c.skills << "\n"
       << "Talent Rating: " << c.talent_rating << "\n"
       << "------------------------------------------";
    return os;
}


int main() {
    auto contacts = read_contacts("test.txt");
    
    /* testing */

    for (Contact& item : contacts) {
        std::cout << item << std::endl;
    }

    printf("%d\n", contacts.size());

    insert_contacts_to_db(contacts, "my_database.db");
    std::cout << "✅ Contacts imported into database." << std::endl;
    return 0;
}

/* gcc -c sqlite3.c -o sqlite3.o   
g++ import_contacts.cpp sqlite3.o -o import_contacts -I. 

use this command to compile */

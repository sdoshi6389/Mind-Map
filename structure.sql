-- Main contact table
CREATE TABLE IF NOT EXISTS contacts (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT,
    email TEXT,
    phone TEXT,
    location TEXT
);

-- Employment info
CREATE TABLE IF NOT EXISTS employment (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    contact_id INTEGER,
    current_company TEXT,
    previous_companies TEXT,
    industry TEXT,
    job_title TEXT,
    FOREIGN KEY(contact_id) REFERENCES contacts(id)
);

-- Relationship info
CREATE TABLE IF NOT EXISTS relationships (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    contact_id INTEGER,
    relationship_type TEXT,
    closeness TEXT,
    reliability TEXT,
    FOREIGN KEY(contact_id) REFERENCES contacts(id)
);

-- Personal background
CREATE TABLE IF NOT EXISTS background (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    contact_id INTEGER,
    interests TEXT,
    college TEXT,
    high_school TEXT,
    FOREIGN KEY(contact_id) REFERENCES contacts(id)
);

-- Professional traits
CREATE TABLE IF NOT EXISTS profile (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    contact_id INTEGER,
    career_goals TEXT,
    skills TEXT,
    talent_rating TEXT,
    FOREIGN KEY(contact_id) REFERENCES contacts(id)
);

CREATE TABLE IF NOT EXISTS shared_skills (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    person1 TEXT,
    person2 TEXT,
    shared_skills TEXT,
    UNIQUE(person1, person2)
);

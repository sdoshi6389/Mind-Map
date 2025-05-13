CREATE TABLE previous_companies (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    company TEXT NOT NULL,
    people TEXT NOT NULL,
    UNIQUE(company, people)
);
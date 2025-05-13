CREATE TABLE company_groups (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    company TEXT NOT NULL,
    people TEXT NOT NULL,
    UNIQUE(company, people)
);
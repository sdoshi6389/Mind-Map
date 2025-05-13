CREATE TABLE industry_groups (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    industry TEXT NOT NULL,
    people TEXT NOT NULL,
    UNIQUE(industry, people)
);
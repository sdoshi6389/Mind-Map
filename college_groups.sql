CREATE TABLE college_groups (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    college TEXT NOT NULL,
    people TEXT NOT NULL,
    UNIQUE(college, people)
);
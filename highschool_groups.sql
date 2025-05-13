CREATE TABLE highschool_groups (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    highschool TEXT NOT NULL,
    people TEXT NOT NULL,
    UNIQUE(highschool, people)
);
CREATE TABLE IF NOT EXISTS share_goals (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    person1 TEXT,
    person2 TEXT,
    shared_goal TEXT,
    UNIQUE(person1, person2)
);
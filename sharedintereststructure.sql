CREATE TABLE IF NOT EXISTS interest_edges (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    person1 TEXT,
    person2 TEXT,
    shared_interests TEXT,
    UNIQUE(person1, person2)
);

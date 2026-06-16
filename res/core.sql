-- sqlite3 core.db < core.sql
-- sqlite3.exe core.db < core.sql
CREATE TABLE IF NOT EXISTS session (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    title TEXT NOT NULL,
    content TEXT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    vector_index TEXT,
    FOREIGN KEY (user_id) REFERENCES user(id)
);

CREATE TABLE IF NOT EXISTS user (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    username TEXT NOT NULL,
    encrypted_passwd TEXT NOT NULL
);

INSERT INTO user (username, encrypted_passwd) VALUES ('admin', 'admin');

INSERT INTO session (user_id, title, content, timestamp, vector_index) VALUES (
    1, 'x', 'HELLO WORLD', '2024-06-01 12:00:00', '');
INSERT INTO session (user_id, title, content, timestamp, vector_index) VALUES (
    1, 'xx', 'What is RAG?', '2024-06-01 12:00:01', '');
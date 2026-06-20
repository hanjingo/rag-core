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
    encrypted_passwd TEXT NOT NULL,
    privilege INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS skill (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    name TEXT NOT NULL,
    desc TEXT,
    publisher TEXT NOT NULL,
    version TEXT NOT NULL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    hash TEXT NOT NULL
);

CREATE TABLE IF NOT EXISTS file (
    hash TEXT PRIMARY KEY,
    addr TEXT NOT NULL,
    owner INTEGER NOT NULL,
    size_kb INTEGER DEFAULT 0,
    FOREIGN KEY (owner) REFERENCES user(id)
);

INSERT INTO user (username, encrypted_passwd, privilege) VALUES ('admin', 'admin', 1);

INSERT INTO session (user_id, title, content, timestamp, vector_index) VALUES (
    1, 'x', 'HELLO WORLD', '2024-06-01 12:00:00', '');
INSERT INTO session (user_id, title, content, timestamp, vector_index) VALUES (
    1, 'xx', 'What is RAG?', '2024-06-01 12:00:01', '');

INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'chatbox', 'A skill for Chat, using qwen model, free for use it etc...', 'admin', 'v0.0.1', '2026-06-19 19:00:00', 
    'sha256:1cef61acb5f0379c51bd723bfcac0152112a399f3bdbc8072bfae65b3986dc07');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill2', 'A skill for testing2', 'admin', '0.0.1', '2026-06-01 12:00:01', 'hash2');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill3','A skill for testing3', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash3');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill4','A skill for testing4', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash4');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill5', 'A skill for testing5', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash5');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill6', 'A skill for testing6', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash6');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill7', 'A skill for testing7', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash7');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill8', 'A skill for testing8', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash8');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill9', 'A skill for testing9', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash9');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill10', 'A skill for testing10', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash10');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill11', 'A skill for testing11', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash11');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill12', 'A skill for testing12', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash12');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill13', 'A skill for testing13', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash13');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill14', 'A skill for testing14', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash14');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill15', 'A skill for testing15', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash15');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill16', 'A skill for testing16', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash16');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill17', 'A skill for testing17', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash17');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill18', 'A skill for testing18', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash18');
INSERT INTO skill (name, desc, publisher, version, timestamp, hash) VALUES (
    'skill19', 'A skill for testing19', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash19');

INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'sha256:1cef61acb5f0379c51bd723bfcac0152112a399f3bdbc8072bfae65b3986dc07', 
    'https://github.com/hanjingo/rag-qt-chatbox/releases/download/v0.0.1/chatbox-windows-x64-v0.0.1.zip', 1, 35);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash2', 'http://example.com/file2.zip', 1, 200);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash3', 'http://example.com/file3.zip', 1, 300);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash4', 'http://example.com/file4.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash5', 'http://example.com/file5.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash6', 'http://example.com/file6.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash7', 'http://example.com/file7.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash8', 'http://example.com/file8.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash9', 'http://example.com/file9.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash10', 'http://example.com/file10.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash11', 'http://example.com/file11.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash12', 'http://example.com/file12.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash13', 'http://example.com/file13.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash14', 'http://example.com/file14.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash15', 'http://example.com/file15.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash16', 'http://example.com/file16.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash17', 'http://example.com/file17.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash18', 'http://example.com/file18.zip', 1, 100);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'hash19', 'http://example.com/file19.zip', 1, 100);
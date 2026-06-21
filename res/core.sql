-- sqlite3 core.db < core.sql
-- sqlite3.exe core.db < core.sql
CREATE TABLE IF NOT EXISTS session (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    user_id INTEGER NOT NULL,
    title TEXT NOT NULL,
    content TEXT,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
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
    platform INTEGER DEFAULT 0, -- 1: windows, 2: linux, 4: macos, 5: win+mac, ...
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

INSERT INTO session (user_id, title, content, timestamp) VALUES (
    1, 'x', 'HELLO WORLD', '2024-06-01 12:00:00');
INSERT INTO session (user_id, title, content, timestamp) VALUES (
    1, 'xx', 'What is RAG?', '2024-06-01 12:00:01');

INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'chatbox', 1, 'A skill for Chat, using qwen model, free for use it etc...', 'admin', '0.0.1', '2026-06-21 19:00:00', 
    'sha256:4b6acc7d6edb5c71620adc9cc3def5b4f3e5b0beca27f9476edd066190ac02f5');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'grammar', 1, 'A skill for english grammar checking', 'admin', '0.0.1', '2026-06-21 12:00:01', 
    'sha256:607396c3c0d48ddef72125d7a7a25d447961a0c19112ecf629e5eb1e25f89b52');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'ielts-writer', 1, 'A skill for IELTS Writer Guide', 'admin', '0.0.1', '2026-06-21 12:00:00', 
    'sha256:c82c0465c9c6d0e0cfdd51fd440d341ef60d892330fbcce880dc28fcf9f776a8');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill4', 1, 'A skill for testing4', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash4');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill5', 1, 'A skill for testing5', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash5');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill6', 1, 'A skill for testing6', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash6');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill7', 1, 'A skill for testing7', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash7');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill8', 1, 'A skill for testing8', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash8');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill9', 1, 'A skill for testing9', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash9');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill10', 1, 'A skill for testing10', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash10');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill11', 1, 'A skill for testing11', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash11');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill12', 1, 'A skill for testing12', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash12');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill13', 1, 'A skill for testing13', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash13');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill14', 1, 'A skill for testing14', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash14');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill15', 1, 'A skill for testing15', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash15');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill16', 1, 'A skill for testing16', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash16');
INSERT INTO skill (name, platform,  desc, publisher, version, timestamp, hash) VALUES (
    'skill17', 1, 'A skill for testing17', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash17');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill18', 1, 'A skill for testing18', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash18');
INSERT INTO skill (name, platform, desc, publisher, version, timestamp, hash) VALUES (
    'skill19', 1, 'A skill for testing19', 'admin', '0.0.1', '2026-06-01 12:00:00', 'hash19');

INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'sha256:4b6acc7d6edb5c71620adc9cc3def5b4f3e5b0beca27f9476edd066190ac02f5', 
    'https://github.com/hanjingo/rag-qt-chatbox/releases/download/v0.0.2%231/chatbox-windows-x64-v0.0.2.1.zip', 1, 35);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'sha256:607396c3c0d48ddef72125d7a7a25d447961a0c19112ecf629e5eb1e25f89b52', 
    'https://github.com/hanjingo/rag-qt-grammar/releases/download/v0.0.1/grammar-windows-x64-v0.0.1.zip', 1, 60);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'sha256:c82c0465c9c6d0e0cfdd51fd440d341ef60d892330fbcce880dc28fcf9f776a8', 
    'https://github.com/hanjingo/rag-qt-ielts-writer/releases/download/v0.0.1%231/ielts-writer-windows-x64-v0.0.1.1.zip', 1, 49);
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
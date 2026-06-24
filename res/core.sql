-- sqlite3 core.db < core.sql
-- sqlite3.exe core.db < core.sql
CREATE TABLE IF NOT EXISTS session (
    id BIGINT PRIMARY KEY,
    user_id BIGINT NOT NULL,
    title TEXT NOT NULL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (user_id) REFERENCES user(id)
);

CREATE TABLE IF NOT EXISTS message (
    id BIGINT PRIMARY KEY,
    session_id BIGINT NOT NULL,
    role TEXT NOT NULL CHECK(role IN ('system', 'user', 'assistant')),
    content TEXT NOT NULL,
    prev_message_id BIGINT DEFAULT NULL, -- for threading message, NULL means no parent
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,

    FOREIGN KEY (session_id) 
        REFERENCES session(id) 
        ON DELETE CASCADE,

    FOREIGN KEY (prev_message_id) 
        REFERENCES message(id) 
        ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS user (
    id BIGINT PRIMARY KEY,
    username TEXT NOT NULL,
    encrypted_passwd TEXT NOT NULL,
    privilege INTEGER DEFAULT 0 -- 0: normal user, 1: admin, ...
);

CREATE TABLE IF NOT EXISTS model (
    hash TEXT PRIMARY KEY,
    name TEXT NOT NULL,
    publisher TEXT DEFAULT 'unknown',
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP,
    addr TEXT,
    capabilities TEXT,
    context_size INTEGER DEFAULT 0,
    cost INTEGER DEFAULT 0
);

CREATE TABLE IF NOT EXISTS skill (
    hash TEXT PRIMARY KEY,
    platform INTEGER DEFAULT 0, -- 1: windows, 2: linux, 4: macos, 5: win+mac, ...
    name TEXT NOT NULL,
    desc TEXT,
    publisher TEXT NOT NULL,
    version TEXT NOT NULL,
    timestamp DATETIME DEFAULT CURRENT_TIMESTAMP
);

CREATE TABLE IF NOT EXISTS file (
    hash TEXT PRIMARY KEY,
    addr TEXT NOT NULL,
    owner BIGINT NOT NULL,
    size_kb INTEGER DEFAULT 0,
    FOREIGN KEY (owner) REFERENCES user(id)
);

INSERT INTO user (id, username, encrypted_passwd, privilege) VALUES (1, 'admin', 'admin', 1);

INSERT INTO model (hash, name, publisher, timestamp, addr, capabilities, context_size, cost) VALUES (
    'hash1', 'TinyStories-656K-Q3_K_M', 'unknown', '2024-06-01 12:00:01', './models/TinyStories-656K-Q3_K_M.gguf', 'chat', 4000, 0.003);

INSERT INTO session (id, user_id, title, timestamp) VALUES (
    1, 1, 'x', '2024-06-01 12:00:00');
INSERT INTO session (id, user_id, title, timestamp) VALUES (
    2, 1, 'xx', '2024-06-01 12:00:01');

INSERT INTO message (id, session_id, role, content, prev_message_id, timestamp) VALUES (
    1, 1, 'user', 'Question Test1', NULL, '2026-06-01 12:00:00');
INSERT INTO message (id, session_id, role, content, prev_message_id, timestamp) VALUES (
    2, 1, 'assistant', 'Answer Test1', 1, '2026-06-01 12:00:01');
INSERT INTO message (id, session_id, role, content, prev_message_id, timestamp) VALUES (
    3, 1, 'user', 'Question Test2', NULL, '2026-06-01 12:00:02');
INSERT INTO message (id, session_id, role, content, prev_message_id, timestamp) VALUES (
    4, 1, 'assistant', 'Answer Test2', 3, '2026-06-01 12:00:03');

INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'sha256:4b6acc7d6edb5c71620adc9cc3def5b4f3e5b0beca27f9476edd066190ac02f5', 'chatbox', 1, 
    'A skill for Chat, using llm model, free for use it etc...', 'admin', '0.0.1', '2026-06-21 19:00:00');
INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'sha256:607396c3c0d48ddef72125d7a7a25d447961a0c19112ecf629e5eb1e25f89b52', 'grammar', 1, 
    'A skill for english grammar checking', 'admin', '0.0.1', '2026-06-21 12:00:01');
INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'sha256:c82c0465c9c6d0e0cfdd51fd440d341ef60d892330fbcce880dc28fcf9f776a8', 'ielts-writer', 1, 
    'A skill for IELTS Writer Guide', 'admin', '0.0.1', '2026-06-21 12:00:00');
INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'sha256:8637d9bf081dd37aaec97cc46cb7426585ab2cd056b291c0b72ed2102661a9c3', 'stock', 1, 
    'A skill for Stock trade', 'admin', '0.0.1', '2026-06-24 12:00:00');
INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'hash9', 'skill9', 1, 
    'A skill for testing9', 'admin', '0.0.1', '2026-06-01 12:00:00');
INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'hash10', 'skill10', 1, 
    'A skill for testing10', 'admin', '0.0.1', '2026-06-01 12:00:00');
INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'hash11', 'skill11', 1, 
    'A skill for testing11', 'admin', '0.0.1', '2026-06-01 12:00:00');
INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'hash12', 'skill12', 1, 
    'A skill for testing12', 'admin', '0.0.1', '2026-06-01 12:00:00');
INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'hash13', 'skill13', 1, 
    'A skill for testing13', 'admin', '0.0.1', '2026-06-01 12:00:00');
INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'hash14', 'skill14', 1, 
    'A skill for testing14', 'admin', '0.0.1', '2026-06-01 12:00:00');
INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'hash15', 'skill15', 1, 
    'A skill for testing15', 'admin', '0.0.1', '2026-06-01 12:00:00');
INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'hash16', 'skill16', 1, 
    'A skill for testing16', 'admin', '0.0.1', '2026-06-01 12:00:00');
INSERT INTO skill (hash, name, platform,  desc, publisher, version, timestamp) VALUES (
    'hash17', 'skill17', 1, 
    'A skill for testing17', 'admin', '0.0.1', '2026-06-01 12:00:00');
INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'hash18', 'skill18', 1, 
    'A skill for testing18', 'admin', '0.0.1', '2026-06-01 12:00:00');
INSERT INTO skill (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'hash19', 'skill19', 1, 
    'A skill for testing19', 'admin', '0.0.1', '2026-06-01 12:00:00');

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
    'sha256:8637d9bf081dd37aaec97cc46cb7426585ab2cd056b291c0b72ed2102661a9c3', 
    'https://github.com/hanjingo/rag-qt-stock/releases/download/v0.0.1/stock-windows-x64-v0.0.1.zip', 1, 45);
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
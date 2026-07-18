-- sqlite3 core.db < core.sql
-- sqlite3.exe core.db < core.sql
CREATE TABLE IF NOT EXISTS session (
    id BIGINT PRIMARY KEY,
    user_id BIGINT NOT NULL,
    title TEXT NOT NULL,
    timestamp BIGINT, -- use integer is a better choice

    FOREIGN KEY (user_id) REFERENCES user(id)
);

CREATE TABLE IF NOT EXISTS message (
    id BIGINT PRIMARY KEY,
    session_id BIGINT NOT NULL,
    role TEXT NOT NULL CHECK(role IN ('system', 'user', 'assistant')),
    content TEXT NOT NULL,
    prev_message_id BIGINT DEFAULT NULL, -- for threading message, NULL means no parent
    timestamp BIGINT,

    FOREIGN KEY (session_id) 
        REFERENCES session(id) 
        ON DELETE CASCADE,

    FOREIGN KEY (prev_message_id) 
        REFERENCES message(id) 
        ON DELETE SET NULL
);

CREATE TABLE IF NOT EXISTS user (
    id BIGINT PRIMARY KEY,
    username TEXT NOT NULL UNIQUE,
    encrypted_passwd TEXT NOT NULL,
    privilege INTEGER DEFAULT 0 -- 0: normal user, 1: developer, 2: admin, ...
);

CREATE TABLE IF NOT EXISTS plugin (
    hash TEXT PRIMARY KEY,
    platform INTEGER DEFAULT 0, -- 1: windows, 2: linux, 4: macos, 5: win+mac, ...
    name TEXT NOT NULL,
    desc TEXT,
    publisher TEXT NOT NULL,
    version TEXT NOT NULL,
    timestamp BIGINT
);

CREATE TABLE IF NOT EXISTS file (
    hash TEXT PRIMARY KEY,
    addr TEXT NOT NULL,
    owner BIGINT NOT NULL,
    size_kb INTEGER DEFAULT 0,
    FOREIGN KEY (owner) REFERENCES user(id)
);

INSERT INTO user (id, username, encrypted_passwd, privilege) VALUES (1, 'admin', 'admin', 2);

INSERT INTO session (id, user_id, title, timestamp) VALUES (
    1, 1, 'x', 0);
INSERT INTO session (id, user_id, title, timestamp) VALUES (
    2, 1, 'xx', 1);

INSERT INTO message (id, session_id, role, content, prev_message_id, timestamp) VALUES (
    1, 1, 'user', 'Question Test1', NULL, 0);
INSERT INTO message (id, session_id, role, content, prev_message_id, timestamp) VALUES (
    2, 1, 'assistant', 'Answer Test1', 1, 1);
INSERT INTO message (id, session_id, role, content, prev_message_id, timestamp) VALUES (
    3, 1, 'user', 'Question Test2', NULL, 2);
INSERT INTO message (id, session_id, role, content, prev_message_id, timestamp) VALUES (
    4, 1, 'assistant', 'Answer Test2', 3, 3);

INSERT INTO plugin (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'sha256:a05c13fcd95e7305526d8a70820247046335f461356271b4eaee0919ee340f2a', 'chatbox', 1, 
    'A plugin for Chat, using llm model, free for use it etc...', 'admin', '0.0.3', 0);
INSERT INTO plugin (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'sha256:4135b32bf4257a54aabeabd416b53dbd1bc411c50257688f0073eeb9aa785d16', 'grammar', 1, 
    'A plugin for english grammar checking', 'admin', '0.0.2', 1);
INSERT INTO plugin (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'sha256:cdfcc494cf944a74a06985688063197513295e6c3c876521ea184da11b6f0076', 'ielts-writer', 1, 
    'A plugin for IELTS Writer Guide', 'admin', '0.0.2', 2);
INSERT INTO plugin (hash, name, platform, desc, publisher, version, timestamp) VALUES (
    'sha256:cac176b447f4dd995677d1ee998b9bc37d49feed0fe5125020110ad7f52edfb5', 'stock', 1, 
    'A plugin for Stock trade', 'admin', '0.0.2', 3);

INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'sha256:a05c13fcd95e7305526d8a70820247046335f461356271b4eaee0919ee340f2a', 
    'https://github.com/hanjingo/rag-qt-chatbox/releases/download/v0.0.3/chatbox-windows-x64-v0.0.3.zip', 1, 152);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'sha256:4135b32bf4257a54aabeabd416b53dbd1bc411c50257688f0073eeb9aa785d16', 
    'https://github.com/hanjingo/rag-qt-grammar/releases/download/v0.0.2/grammar-windows-x64-v0.0.2.zip', 1, 87);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'sha256:cdfcc494cf944a74a06985688063197513295e6c3c876521ea184da11b6f0076', 
    'https://github.com/hanjingo/rag-qt-ielts-writer/releases/download/v0.0.2/ielts-writer-windows-x64-v0.0.2.zip', 1, 49);
INSERT INTO file (hash, addr, owner, size_kb) VALUES (
    'sha256:cac176b447f4dd995677d1ee998b9bc37d49feed0fe5125020110ad7f52edfb5', 
    'https://github.com/hanjingo/rag-qt-stock/releases/download/v0.0.2/stock-windows-x64-v0.0.2.zip', 1, 45);
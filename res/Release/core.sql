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
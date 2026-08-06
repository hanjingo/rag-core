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
    encrypted_passwd TEXT NOT NULL
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

INSERT INTO user (id, username, encrypted_passwd) VALUES (1, 'admin', 'x61Ey612Kl2gpFL56FT9weDnpSo4AV8j8+qx2AuTHdRyY036xxzTTrw10Wq3+4qQyB+XURPWx1ONxp3Y3pB37A==');

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
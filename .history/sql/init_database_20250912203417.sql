-- MPIM 数据库初始化脚本
-- 创建数据库
CREATE DATABASE IF NOT EXISTS chat CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
USE mpchat;

-- 用户表 (结合原有结构的优势)
CREATE TABLE IF NOT EXISTS user (
    id INT(11) NOT NULL AUTO_INCREMENT,
    name VARCHAR(50) NOT NULL,
    password VARCHAR(50) NOT NULL,
    state ENUM('online', 'offline') DEFAULT 'offline',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY name (name)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 好友关系表 (保持原有简洁性)
CREATE TABLE IF NOT EXISTS friend (
    userid INT(11) NOT NULL,
    friendid INT(11) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    KEY userid (userid, friendid),
    FOREIGN KEY (userid) REFERENCES user(id) ON DELETE CASCADE,
    FOREIGN KEY (friendid) REFERENCES user(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 群组表 (结合原有结构)
CREATE TABLE IF NOT EXISTS allgroup (
    id INT(11) NOT NULL AUTO_INCREMENT,
    groupname VARCHAR(50) NOT NULL,
    groupdesc VARCHAR(200) DEFAULT '',
    creator_id INT(11) NOT NULL,
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    UNIQUE KEY groupname (groupname),
    FOREIGN KEY (creator_id) REFERENCES user(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 群组成员表 (保持原有角色管理)
CREATE TABLE IF NOT EXISTS groupuser (
    groupid INT(11) NOT NULL,
    userid INT(11) NOT NULL,
    grouprole ENUM('creator', 'normal') DEFAULT 'normal',
    joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    KEY groupid (groupid, userid),
    FOREIGN KEY (groupid) REFERENCES allgroup(id) ON DELETE CASCADE,
    FOREIGN KEY (userid) REFERENCES user(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 离线消息表 (保持原有JSON格式的灵活性)
CREATE TABLE IF NOT EXISTS offlinemessage (
    id INT(11) NOT NULL AUTO_INCREMENT,
    userid INT(11) NOT NULL,
    message TEXT NOT NULL,
    msg_type ENUM('c2c', 'group') DEFAULT 'c2c',
    created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP,
    PRIMARY KEY (id),
    KEY userid (userid),
    FOREIGN KEY (userid) REFERENCES user(id) ON DELETE CASCADE
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4;

-- 创建额外索引
CREATE INDEX idx_friend_userid ON friend(userid);
CREATE INDEX idx_friend_friendid ON friend(friendid);
CREATE INDEX idx_groupuser_groupid ON groupuser(groupid);
CREATE INDEX idx_groupuser_userid ON groupuser(userid);
CREATE INDEX idx_offlinemessage_userid ON offlinemessage(userid);
CREATE INDEX idx_offlinemessage_type ON offlinemessage(msg_type);

-- 插入测试数据
INSERT INTO user (name, password, state) VALUES 
('zhang san', '123456', 'offline'),
('li si', '666666', 'offline'),
('gao yang', '123456', 'offline'),
('pi pi', '123456', 'offline');

-- 插入测试好友关系
INSERT INTO friend (userid, friendid) VALUES 
(1, 2), (2, 1),  -- zhang san <-> li si
(1, 3), (3, 1),  -- zhang san <-> gao yang
(2, 4), (4, 2);  -- li si <-> pi pi

-- 插入测试群组
INSERT INTO allgroup (groupname, groupdesc, creator_id) VALUES 
('C++ chat project', 'start develop a chat project', 1),
('测试群组', '这是一个测试群组', 2);

-- 插入测试群组成员
INSERT INTO groupuser (groupid, userid, grouprole) VALUES 
(1, 1, 'creator'), (1, 2, 'normal'), (1, 3, 'normal'),  -- 群组1: zhang san(creator), li si, gao yang
(2, 2, 'creator'), (2, 4, 'normal');                    -- 群组2: li si(creator), pi pi

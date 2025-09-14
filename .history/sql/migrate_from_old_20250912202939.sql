-- 数据库迁移脚本：从原有结构迁移到优化后的结构
-- 注意：请在迁移前备份原有数据！

-- 1. 备份原有数据到临时表
CREATE TABLE user_backup AS SELECT * FROM user;
CREATE TABLE friend_backup AS SELECT * FROM friend;
CREATE TABLE allgroup_backup AS SELECT * FROM allgroup;
CREATE TABLE groupuser_backup AS SELECT * FROM groupuser;
CREATE TABLE offlinemessage_backup AS SELECT * FROM offlinemessage;

-- 2. 添加新字段到现有表
ALTER TABLE user ADD COLUMN created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP;
ALTER TABLE user ADD COLUMN updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP;

ALTER TABLE friend ADD COLUMN created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP;

ALTER TABLE allgroup ADD COLUMN creator_id INT(11) DEFAULT 0;
ALTER TABLE allgroup ADD COLUMN created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP;
ALTER TABLE allgroup ADD COLUMN updated_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP;

ALTER TABLE groupuser ADD COLUMN joined_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP;

ALTER TABLE offlinemessage ADD COLUMN id INT(11) NOT NULL AUTO_INCREMENT PRIMARY KEY FIRST;
ALTER TABLE offlinemessage ADD COLUMN msg_type ENUM('c2c', 'group') DEFAULT 'c2c';
ALTER TABLE offlinemessage ADD COLUMN created_at TIMESTAMP DEFAULT CURRENT_TIMESTAMP;

-- 3. 更新群组创建者信息（需要手动设置）
-- UPDATE allgroup SET creator_id = (SELECT userid FROM groupuser WHERE groupid = allgroup.id AND grouprole = 'creator' LIMIT 1);

-- 4. 创建索引
CREATE INDEX idx_friend_userid ON friend(userid);
CREATE INDEX idx_friend_friendid ON friend(friendid);
CREATE INDEX idx_groupuser_groupid ON groupuser(groupid);
CREATE INDEX idx_groupuser_userid ON groupuser(userid);
CREATE INDEX idx_offlinemessage_userid ON offlinemessage(userid);
CREATE INDEX idx_offlinemessage_type ON offlinemessage(msg_type);

-- 5. 添加外键约束
ALTER TABLE friend ADD CONSTRAINT fk_friend_userid FOREIGN KEY (userid) REFERENCES user(id) ON DELETE CASCADE;
ALTER TABLE friend ADD CONSTRAINT fk_friend_friendid FOREIGN KEY (friendid) REFERENCES user(id) ON DELETE CASCADE;
ALTER TABLE allgroup ADD CONSTRAINT fk_allgroup_creator FOREIGN KEY (creator_id) REFERENCES user(id) ON DELETE CASCADE;
ALTER TABLE groupuser ADD CONSTRAINT fk_groupuser_groupid FOREIGN KEY (groupid) REFERENCES allgroup(id) ON DELETE CASCADE;
ALTER TABLE groupuser ADD CONSTRAINT fk_groupuser_userid FOREIGN KEY (userid) REFERENCES user(id) ON DELETE CASCADE;
ALTER TABLE offlinemessage ADD CONSTRAINT fk_offlinemessage_userid FOREIGN KEY (userid) REFERENCES user(id) ON DELETE CASCADE;

-- 6. 更新字符集为utf8mb4
ALTER TABLE user CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
ALTER TABLE friend CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
ALTER TABLE allgroup CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
ALTER TABLE groupuser CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
ALTER TABLE offlinemessage CONVERT TO CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;

-- 迁移完成提示
SELECT 'Migration completed! Please verify data integrity and remove backup tables if everything looks good.' as status;

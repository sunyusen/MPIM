#include "offline_model.h"
#include <mysql.h>
#include <sstream>
#include <string>
#include <cstring>
#include <iostream>
#include "logger/logger.h"
#include "logger/log_init.h"

// Base64编码函数
std::string base64_encode(const std::string& input) {
    const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string result;
    int val = 0, valb = -6;
    for (unsigned char c : input) {
        val = (val << 8) + c;
        valb += 8;
        while (valb >= 0) {
            result.push_back(chars[(val >> valb) & 0x3F]);
            valb -= 6;
        }
    }
    if (valb > -6) result.push_back(chars[((val << 8) >> (valb + 8)) & 0x3F]);
    while (result.size() % 4) result.push_back('=');
    return result;
}

// Base64解码函数
std::string base64_decode(const std::string& input) {
	const char* chars = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	std::string result;
	int val = 0, valb = -8;
	for (char c : input) {
		if (c == '=') break;
		const char* p = strchr(chars, c);
		if (!p) continue;
		val = (val << 6) + (p - chars);
		valb += 6;
		if (valb >= 0) {
			result.push_back(char((val >> valb) & 0xFF));
			valb -= 8;
		}
	}
    return result;
}

OfflineModel::OfflineModel()
{
	db_.reset(new MySQL());
	if (!db_->connect()) {
			LOG_ERROR << "OfflineModel: MySQL connection failed! Offline message storage will be disabled.";
		db_.reset(); // 设置为null，后续操作会检查
	} else {
			LOG_INFO << "OfflineModel: MySQL connected successfully";
	}
}
bool OfflineModel::insert(int64_t uid, const std::string &payload)
{
	if (!db_) {
			LOG_WARN << "OfflineModel::insert: Database not available, skipping offline storage";
		return false;
	}
	
	// 将protobuf序列化的二进制数据转换为Base64编码
	std::string base64_payload = base64_encode(payload);
	
	std::string esc;
	db_->escape(base64_payload, esc);

	std::ostringstream os;
	os << "INSERT INTO offlinemessage(userid,message,msg_type) VALUES(" << uid << ",'" << esc << "','c2c')";
	
	LOG_DEBUG << "OfflineModel::insert: uid=" << uid << " payload_size=" << payload.size() << " base64_size=" << base64_payload.size();
	
	bool result = db_->update(os.str());
	LOG_DEBUG << "OfflineModel::insert: result=" << (result ? "success" : "failed");
	return result;
}
std::vector<std::string> OfflineModel::query(int64_t uid)
{
	if (!db_) {
			LOG_WARN << "OfflineModel::query: Database not available, returning empty result";
		return {};
	}
	
	char sql[256];
	snprintf(sql, sizeof(sql), "SELECT message FROM offlinemessage WHERE userid=%d AND msg_type='c2c'", (int)uid);

	MYSQL_RES *res = db_->query(sql);
	if (!res)
		return {};

	std::vector<std::string> out;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)) != nullptr)
	{
		// 将Base64编码的消息解码回原始protobuf数据
		std::string base64_msg = row[0] ? row[0] : "";
		std::string decoded_msg = base64_decode(base64_msg);
		out.push_back(decoded_msg);
	}
	mysql_free_result(res);
	return out;
}
bool OfflineModel::remove(int64_t uid)
{
	if (!db_) {
			LOG_WARN << "OfflineModel::remove: Database not available, skipping remove operation";
		return false;
	}
	
	char sql[256];
	snprintf(sql, sizeof(sql), "DELETE FROM offlinemessage WHERE userid=%d AND msg_type='c2c'", (int)uid);
	return db_->update(sql);
}

bool OfflineModel::insertGroupMessage(int64_t group_id, const std::string &payload)
{
	std::string esc;
	db_->escape(payload, esc);

	// 群组消息需要存储到所有群成员的离线消息中
	// 这里简化处理，只存储到群组表中，实际应该查询群成员并分别存储
	std::ostringstream os;
	os << "INSERT INTO offlinemessage(userid,message,msg_type) VALUES(0,'" << esc << "','group')";
	return db_->update(os.str());
}

std::vector<std::string> OfflineModel::queryGroupMessages(int64_t group_id)
{
	char sql[256];
	snprintf(sql, sizeof(sql), "SELECT message FROM offlinemessage WHERE userid=0 AND msg_type='group'");

	MYSQL_RES *res = db_->query(sql);
	if (!res)
		return {};

	std::vector<std::string> out;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)) != nullptr)
	{
		out.emplace_back(row[0] ? row[0] : "");
	}
	mysql_free_result(res);
	return out;
}

bool OfflineModel::removeGroupMessages(int64_t group_id)
{
	char sql[256];
	snprintf(sql, sizeof(sql), "DELETE FROM offlinemessage WHERE userid=0 AND msg_type='group'");
	return db_->update(sql);
}

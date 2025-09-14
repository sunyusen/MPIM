#include "offline_model.h"
#include <mysql.h>
#include <sstream>
#include <string>

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
#include <iostream>

OfflineModel::OfflineModel()
{
	db_.reset(new MySQL());
	if (!db_->connect()) {
		std::cout << "MySQL connection failed in OfflineModel!" << std::endl;
		exit(EXIT_FAILURE);
	}
	std::cout << "OfflineModel: MySQL connected successfully" << std::endl;
}
bool OfflineModel::insert(int64_t uid, const std::string &payload)
{
	// 将protobuf序列化的二进制数据转换为Base64编码
	std::string base64_payload = base64_encode(payload);
	
	std::string esc;
	db_->escape(base64_payload, esc);

	std::ostringstream os;
	os << "INSERT INTO offlinemessage(userid,message,msg_type) VALUES(" << uid << ",'" << esc << "','c2c')";
	
	std::cout << "OfflineModel::insert: uid=" << uid << " payload_size=" << payload.size() 
			  << " base64_size=" << base64_payload.size() << std::endl;
	
	bool result = db_->update(os.str());
	std::cout << "OfflineModel::insert: result=" << (result ? "success" : "failed") << std::endl;
	return result;
}
std::vector<std::string> OfflineModel::query(int64_t uid)
{
	char sql[256];
	snprintf(sql, sizeof(sql), "SELECT message FROM offlinemessage WHERE userid=%d AND msg_type='c2c'", (int)uid);

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
bool OfflineModel::remove(int64_t uid)
{
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

#include "offline_model.h"
#include <mysql.h>
#include <sstream>

OfflineModel::OfflineModel()
{
	db_.reset(new MySQL());
	db_->connect();
}
bool OfflineModel::insert(int64_t uid, const std::string &payload)
{
	std::string esc;
	db_->escape(payload, esc);

	std::ostringstream os;
	os << "INSERT INTO offlinemessage(userid,message,msg_type) VALUES(" << uid << ",'" << esc << "','c2c')";
	return db_->update(os.str());
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
	snprintf(sql, sizeof(sql), "DELETE FROM offline_message WHERE user_id=%lld", (long long)uid);
	return db_->update(sql);
}

bool OfflineModel::insertGroupMessage(int64_t group_id, const std::string &payload)
{
	std::string esc;
	db_->escape(payload, esc);

	std::ostringstream os;
	os << "INSERT INTO group_offline_message(group_id,payload) VALUES(" << group_id << ",'" << esc << "')";
	return db_->update(os.str());
}

std::vector<std::string> OfflineModel::queryGroupMessages(int64_t group_id)
{
	char sql[256];
	snprintf(sql, sizeof(sql), "SELECT payload FROM group_offline_message WHERE group_id=%lld", (long long)group_id);

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
	snprintf(sql, sizeof(sql), "DELETE FROM group_offline_message WHERE group_id=%lld", (long long)group_id);
	return db_->update(sql);
}

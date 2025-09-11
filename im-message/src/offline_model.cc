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
	os << "INSERT INTO offline_message(user_id,payload) VALUES(" << uid << ",'" << esc << "')";
	return db_->update(os.str());
}
std::vector<std::string> OfflineModel::query(int64_t uid)
{
	char sql[256];
	snprintf(sql, sizeof(sql), "SELECT payload FROM offline_message WHERE user_id=%lld", (long long)uid);

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

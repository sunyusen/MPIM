#include "user_service.h"
#include <mysql/mysql.h>
#include "logger/logger.h"
#include "logger/log_init.h"
#include "cache/business_cache.h"

UserServiceImpl::UserServiceImpl()
{
	LOG_INFO << "UserServiceImpl: Initializing user service";
	db_.reset(new MySQL());
	if (db_->connect()) {
		LOG_INFO << "UserServiceImpl: Database connected successfully";
	} else {
		LOG_ERROR << "UserServiceImpl: Database connection failed! User operations will be disabled";
	}
}

void UserServiceImpl::Login(google::protobuf::RpcController *,
							const mpim::LoginReq *req,
							mpim::LoginResp *resp,
							google::protobuf::Closure *done)
{
	LOG_INFO << "UserServiceImpl::Login: User '" << req->username() << "' attempting to login";
	
	mpim::Result *r = resp->mutable_result();
	std::string u(req->username().begin(), req->username().end());
	std::string p(req->password().begin(), req->password().end());
	char sql[512];
	snprintf(sql, sizeof(sql),
			 "SELECT id FROM user WHERE name='%s' AND password='%s' LIMIT 1",
			 u.c_str(), p.c_str()); // demo：先不做防注入

	LOG_DEBUG << "UserServiceImpl::Login: Executing SQL: " << sql;
	
	auto *res = db_->query(sql);
	if (!res)
	{
		LOG_ERROR << "UserServiceImpl::Login: Database query failed for user '" << req->username() << "'";
		r->set_code(mpim::Code::INTERNAL);
		r->set_msg("db fail");
		if (done)
			done->Run();
		return;
	}
	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		r->set_code(mpim::Code::INVALID);
		r->set_msg("bad cred");
		mysql_free_result(res);
		if (done)
			done->Run();
		return;
	}

	int64_t uid = atoll(row[0]);
	mysql_free_result(res);
	
	// 检查用户是否已经在线
	char check_online_sql[512];
	snprintf(check_online_sql, sizeof(check_online_sql),
			 "SELECT state FROM user WHERE id=%lld", uid);
	auto *check_res = db_->query(check_online_sql);
	if (check_res) {
		MYSQL_ROW check_row = mysql_fetch_row(check_res);
		if (check_row && strcmp(check_row[0], "online") == 0) {
			// 用户已经在线，拒绝重复登录
			mysql_free_result(check_res);
			r->set_code(mpim::Code::INVALID);
			r->set_msg("user already online");
			if (done) done->Run();
			return;
		}
		mysql_free_result(check_res);
	}
	
	// 更新用户状态为在线
	char update_sql[512];
	snprintf(update_sql, sizeof(update_sql),
			 "UPDATE user SET state='online' WHERE id=%lld", uid);
	if (!db_->update(update_sql)) {
			LOG_ERROR << "Failed to update user state to online for uid=" << uid;
	}
	
	resp->set_user_id(uid);
	resp->set_token("tok_" + std::to_string(uid)); // 简历版：占位 token
	r->set_code(mpim::Code::Ok);
	r->set_msg("ok");
	if (done)
		done->Run();
}

bool UserServiceImpl::userExists(const std::string& username, long long* id_out)
{
	char sql[512];
	snprintf(sql, sizeof(sql), "SELECT id FROM user WHERE name='%s' LIMIT 1", username.c_str());
	MYSQL_RES *res = db_->query(sql);
	if (!res) return false; // 失败时上层根据 result 处理
	MYSQL_ROW row = mysql_fetch_row(res);
	bool exists = (row != nullptr);
	if (exists && id_out) *id_out = atoll(row[0]);
	mysql_free_result(res);
	return exists;
}

void UserServiceImpl::Register(google::protobuf::RpcController *,
								const mpim::RegisterReq *req,
								mpim::RegisterResp *resp,
								google::protobuf::Closure *done)
{
	mpim::Result *r = resp->mutable_result();
	std::string u(req->username().begin(), req->username().end());
	std::string p(req->password().begin(), req->password().end());

	// 先查重
	long long exist_id = 0;
	if (userExists(u, &exist_id))
	{
		r->set_code(mpim::Code::INVALID);
		r->set_msg("exists");
		if (done) done->Run();
		return;
	}

	// 插入
	{
		char sql[512];
		snprintf(sql, sizeof(sql), "INSERT INTO user(name,password) VALUES('%s','%s')", u.c_str(), p.c_str());
		if (!db_->update(sql))
		{
			r->set_code(mpim::Code::INTERNAL);
			r->set_msg("insert fail");
			if (done) done->Run();
			return;
		}
		// 获取自增 id
		MYSQL *c = db_->getConnection();
		long long uid = (long long)mysql_insert_id(c);
		resp->set_user_id(uid);
		r->set_code(mpim::Code::Ok);
		r->set_msg("ok");
	}
	if (done) done->Run();
}

void UserServiceImpl::Logout(google::protobuf::RpcController *,
							const mpim::LogoutReq *req,
							mpim::LogoutResp *resp,
							google::protobuf::Closure *done)
{
	mpim::Result *r = resp->mutable_result();
	
	// 更新用户状态为离线
	char update_sql[512];
	snprintf(update_sql, sizeof(update_sql),
			 "UPDATE user SET state='offline' WHERE id=%lld", req->user_id());
	if (!db_->update(update_sql)) {
			LOG_ERROR << "Failed to update user state to offline for uid=" << req->user_id();
	}
	
	r->set_code(mpim::Code::Ok);
	r->set_msg("logout success");
	if (done) done->Run();
}

void UserServiceImpl::AddFriend(google::protobuf::RpcController *,
							   const mpim::AddFriendReq *req,
							   mpim::AddFriendResp *resp,
							   google::protobuf::Closure *done)
{
	mpim::Result *r = resp->mutable_result();
	
	// 检查是否已经是好友
	if (isFriend(req->user_id(), req->friend_id()))
	{
		r->set_code(mpim::Code::INVALID);
		r->set_msg("already friends");
		if (done) done->Run();
		return;
	}
	
	// 检查目标用户是否存在
	long long friend_id_check = 0;
	char sql[512];
	snprintf(sql, sizeof(sql), "SELECT id FROM user WHERE id=%lld LIMIT 1", req->friend_id());
	MYSQL_RES *res = db_->query(sql);
	if (!res)
	{
		r->set_code(mpim::Code::INTERNAL);
		r->set_msg("db query failed");
		if (done) done->Run();
		return;
	}
	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		mysql_free_result(res);
		r->set_code(mpim::Code::NOT_FOUND);
		r->set_msg("friend not found");
		if (done) done->Run();
		return;
	}
	mysql_free_result(res);
	
	// 添加好友关系（双向）
	char sql1[512], sql2[512];
	snprintf(sql1, sizeof(sql1), 
		"INSERT INTO friend(userid, friendid) VALUES(%d, %d)", 
		(int)req->user_id(), (int)req->friend_id());
	snprintf(sql2, sizeof(sql2), 
		"INSERT INTO friend(userid, friendid) VALUES(%d, %d)", 
		(int)req->friend_id(), (int)req->user_id());
	
	if (!db_->update(sql1) || !db_->update(sql2))
	{
		r->set_code(mpim::Code::INTERNAL);
		r->set_msg("add friend failed");
		if (done) done->Run();
		return;
	}
	
	r->set_code(mpim::Code::Ok);
	r->set_msg("friend added");
	if (done) done->Run();
}

void UserServiceImpl::GetFriends(google::protobuf::RpcController *,
								const mpim::GetFriendsReq *req,
								mpim::GetFriendsResp *resp,
								google::protobuf::Closure *done)
{
	mpim::Result *r = resp->mutable_result();
	
	char sql[512];
	snprintf(sql, sizeof(sql), 
		"SELECT friendid FROM friend WHERE userid=%d", (int)req->user_id());
	
	MYSQL_RES *res = db_->query(sql);
	if (!res)
	{
		r->set_code(mpim::Code::INTERNAL);
		r->set_msg("db query failed");
		if (done) done->Run();
		return;
	}
	
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)) != nullptr)
	{
		resp->add_friend_ids(atoll(row[0]));
	}
	mysql_free_result(res);
	
	r->set_code(mpim::Code::Ok);
	r->set_msg("ok");
	if (done) done->Run();
}

void UserServiceImpl::RemoveFriend(google::protobuf::RpcController *,
								  const mpim::RemoveFriendReq *req,
								  mpim::RemoveFriendResp *resp,
								  google::protobuf::Closure *done)
{
	mpim::Result *r = resp->mutable_result();
	
	// 删除双向好友关系
	char sql1[512], sql2[512];
	snprintf(sql1, sizeof(sql1), 
		"DELETE FROM friend WHERE userid=%d AND friendid=%d", 
		(int)req->user_id(), (int)req->friend_id());
	snprintf(sql2, sizeof(sql2), 
		"DELETE FROM friend WHERE userid=%d AND friendid=%d", 
		(int)req->friend_id(), (int)req->user_id());
	
	if (!db_->update(sql1) || !db_->update(sql2))
	{
		r->set_code(mpim::Code::INTERNAL);
		r->set_msg("remove friend failed");
		if (done) done->Run();
		return;
	}
	
	r->set_code(mpim::Code::Ok);
	r->set_msg("friend removed");
	if (done) done->Run();
}

bool UserServiceImpl::isFriend(int64_t user_id, int64_t friend_id)
{
	char sql[512];
	snprintf(sql, sizeof(sql), 
		"SELECT 1 FROM friend WHERE userid=%d AND friendid=%d LIMIT 1", 
		(int)user_id, (int)friend_id);
	
	MYSQL_RES *res = db_->query(sql);
	if (!res) return false;
	
	MYSQL_ROW row = mysql_fetch_row(res);
	bool exists = (row != nullptr);
	mysql_free_result(res);
	return exists;
}

void UserServiceImpl::resetAllUsersToOffline()
{
	char sql[512];
	snprintf(sql, sizeof(sql), "UPDATE user SET state='offline' WHERE state='online'");
	if (db_->update(sql)) {
			LOG_INFO << "All users reset to offline state on service startup";
	} else {
			LOG_ERROR << "Failed to reset users to offline state";
	}
}
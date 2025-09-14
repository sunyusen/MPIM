#include "user_service.h"
#include <mysql/mysql.h>

UserServiceImpl::UserServiceImpl()
{
	db_.reset(new MySQL());
	db_->connect(); // TODO: 改成 conf
}

void UserServiceImpl::Login(google::protobuf::RpcController *,
							const mpim::LoginReq *req,
							mpim::LoginResp *resp,
							google::protobuf::Closure *done)
{
	mpim::Result *r = resp->mutable_result();
	std::string u(req->username().begin(), req->username().end());
	std::string p(req->password().begin(), req->password().end());
	char sql[512];
	snprintf(sql, sizeof(sql),
			 "SELECT id FROM user WHERE name='%s' AND password='%s' LIMIT 1",
			 u.c_str(), p.c_str()); // demo：先不做防注入

	auto *res = db_->query(sql);
	if (!res)
	{
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
	// 简单的登出实现，实际项目中可能需要清理会话状态
	// 这里可以添加 token 验证逻辑
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
		"DELETE FROM friends WHERE user_id=%lld AND friend_id=%lld", 
		req->user_id(), req->friend_id());
	snprintf(sql2, sizeof(sql2), 
		"DELETE FROM friends WHERE user_id=%lld AND friend_id=%lld", 
		req->friend_id(), req->user_id());
	
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
		"SELECT 1 FROM friends WHERE user_id=%lld AND friend_id=%lld LIMIT 1", 
		user_id, friend_id);
	
	MYSQL_RES *res = db_->query(sql);
	if (!res) return false;
	
	MYSQL_ROW row = mysql_fetch_row(res);
	bool exists = (row != nullptr);
	mysql_free_result(res);
	return exists;
}
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
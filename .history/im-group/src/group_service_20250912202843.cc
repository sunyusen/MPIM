#include "group_service.h"
#include <mysql/mysql.h>
#include <chrono>

GroupServiceImpl::GroupServiceImpl()
{
	db_.reset(new MySQL());
	db_->connect();
}

void GroupServiceImpl::CreateGroup(google::protobuf::RpcController *,
								  const mpim::CreateGroupReq *req,
								  mpim::CreateGroupResp *resp,
								  google::protobuf::Closure *done)
{
	mpim::Result *r = resp->mutable_result();
	
	// 创建群组
	char sql[512];
	auto now = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
	
	snprintf(sql, sizeof(sql), 
		"INSERT INTO allgroup(groupname, groupdesc, creator_id, created_at) VALUES('%s', '%s', %d, %lld)",
		req->group_name().c_str(), req->description().c_str(), (int)req->creator_id(), now);
	
	if (!db_->update(sql))
	{
		r->set_code(mpim::Code::INTERNAL);
		r->set_msg("create group failed");
		if (done) done->Run();
		return;
	}
	
	// 获取群组ID
	MYSQL *c = db_->getConnection();
	long long group_id = (long long)mysql_insert_id(c);
	resp->set_group_id(group_id);
	
	// 将创建者加入群组
	char sql2[512];
	snprintf(sql2, sizeof(sql2), 
		"INSERT INTO groupuser(groupid, userid, grouprole, joined_at) VALUES(%d, %d, 'creator', %lld)",
		(int)group_id, (int)req->creator_id(), now);
	
	if (!db_->update(sql2))
	{
		r->set_code(mpim::Code::INTERNAL);
		r->set_msg("add creator to group failed");
		if (done) done->Run();
		return;
	}
	
	r->set_code(mpim::Code::Ok);
	r->set_msg("group created");
	if (done) done->Run();
}

void GroupServiceImpl::JoinGroup(google::protobuf::RpcController *,
								const mpim::JoinGroupReq *req,
								mpim::JoinGroupResp *resp,
								google::protobuf::Closure *done)
{
	mpim::Result *r = resp->mutable_result();
	
	// 检查群组是否存在
	if (!groupExists(req->group_id()))
	{
		r->set_code(mpim::Code::NOT_FOUND);
		r->set_msg("group not found");
		if (done) done->Run();
		return;
	}
	
	// 检查是否已经是群成员
	if (isGroupMember(req->user_id(), req->group_id()))
	{
		r->set_code(mpim::Code::INVALID);
		r->set_msg("already in group");
		if (done) done->Run();
		return;
	}
	
	// 加入群组
	auto now = std::chrono::duration_cast<std::chrono::seconds>(
		std::chrono::system_clock::now().time_since_epoch()).count();
	
	char sql[512];
	snprintf(sql, sizeof(sql), 
		"INSERT INTO groupuser(groupid, userid, grouprole, joined_at) VALUES(%d, %d, 'normal', %lld)",
		(int)req->group_id(), (int)req->user_id(), now);
	
	if (!db_->update(sql))
	{
		r->set_code(mpim::Code::INTERNAL);
		r->set_msg("join group failed");
		if (done) done->Run();
		return;
	}
	
	r->set_code(mpim::Code::Ok);
	r->set_msg("joined group");
	if (done) done->Run();
}

void GroupServiceImpl::LeaveGroup(google::protobuf::RpcController *,
								 const mpim::LeaveGroupReq *req,
								 mpim::LeaveGroupResp *resp,
								 google::protobuf::Closure *done)
{
	mpim::Result *r = resp->mutable_result();
	
	// 检查是否是群成员
	if (!isGroupMember(req->user_id(), req->group_id()))
	{
		r->set_code(mpim::Code::NOT_FOUND);
		r->set_msg("not in group");
		if (done) done->Run();
		return;
	}
	
	// 离开群组
	char sql[512];
	snprintf(sql, sizeof(sql), 
		"DELETE FROM groupuser WHERE groupid=%d AND userid=%d",
		(int)req->group_id(), (int)req->user_id());
	
	if (!db_->update(sql))
	{
		r->set_code(mpim::Code::INTERNAL);
		r->set_msg("leave group failed");
		if (done) done->Run();
		return;
	}
	
	r->set_code(mpim::Code::Ok);
	r->set_msg("left group");
	if (done) done->Run();
}

void GroupServiceImpl::GetGroupMembers(google::protobuf::RpcController *,
									  const mpim::GetGroupMembersReq *req,
									  mpim::GetGroupMembersResp *resp,
									  google::protobuf::Closure *done)
{
	mpim::Result *r = resp->mutable_result();
	
	char sql[512];
	snprintf(sql, sizeof(sql), 
		"SELECT userid FROM groupuser WHERE groupid=%d", (int)req->group_id());
	
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
		resp->add_member_ids(atoll(row[0]));
	}
	mysql_free_result(res);
	
	r->set_code(mpim::Code::Ok);
	r->set_msg("ok");
	if (done) done->Run();
}

void GroupServiceImpl::GetUserGroups(google::protobuf::RpcController *,
									const mpim::GetUserGroupsReq *req,
									mpim::GetUserGroupsResp *resp,
									google::protobuf::Closure *done)
{
	mpim::Result *r = resp->mutable_result();
	
	char sql[512];
	snprintf(sql, sizeof(sql), 
		"SELECT groupid FROM groupuser WHERE userid=%d", (int)req->user_id());
	
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
		resp->add_group_ids(atoll(row[0]));
	}
	mysql_free_result(res);
	
	r->set_code(mpim::Code::Ok);
	r->set_msg("ok");
	if (done) done->Run();
}

void GroupServiceImpl::GetGroupInfo(google::protobuf::RpcController *,
								   const mpim::GetGroupInfoReq *req,
								   mpim::GetGroupInfoResp *resp,
								   google::protobuf::Closure *done)
{
	mpim::Result *r = resp->mutable_result();
	
	char sql[512];
	snprintf(sql, sizeof(sql), 
		"SELECT group_id, group_name, description, creator_id, created_at FROM groups WHERE group_id=%lld LIMIT 1",
		req->group_id());
	
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
		r->set_msg("group not found");
		if (done) done->Run();
		return;
	}
	
	mpim::GroupInfo *info = resp->mutable_group_info();
	info->set_group_id(atoll(row[0]));
	info->set_group_name(row[1]);
	info->set_description(row[2]);
	info->set_creator_id(atoll(row[3]));
	info->set_created_at(atoll(row[4]));
	
	mysql_free_result(res);
	
	r->set_code(mpim::Code::Ok);
	r->set_msg("ok");
	if (done) done->Run();
}

bool GroupServiceImpl::isGroupMember(int64_t user_id, int64_t group_id)
{
	char sql[512];
	snprintf(sql, sizeof(sql), 
		"SELECT 1 FROM group_members WHERE group_id=%lld AND user_id=%lld LIMIT 1",
		group_id, user_id);
	
	MYSQL_RES *res = db_->query(sql);
	if (!res) return false;
	
	MYSQL_ROW row = mysql_fetch_row(res);
	bool exists = (row != nullptr);
	mysql_free_result(res);
	return exists;
}

bool GroupServiceImpl::groupExists(int64_t group_id)
{
	char sql[512];
	snprintf(sql, sizeof(sql), 
		"SELECT 1 FROM allgroup WHERE id=%d LIMIT 1", (int)group_id);
	
	MYSQL_RES *res = db_->query(sql);
	if (!res) return false;
	
	MYSQL_ROW row = mysql_fetch_row(res);
	bool exists = (row != nullptr);
	mysql_free_result(res);
	return exists;
}

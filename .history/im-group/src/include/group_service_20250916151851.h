#pragma once
#include "group.pb.h"
#include "rpcprovider.h"
#include "db.h"
#include "group_cache.h"
#include <memory>

class GroupServiceImpl : public mpim::GroupService {
public:
  GroupServiceImpl();
  void CreateGroup(::google::protobuf::RpcController*,
                   const ::mpim::CreateGroupReq*,
                   ::mpim::CreateGroupResp*,
                   ::google::protobuf::Closure*) override;
  void JoinGroup(::google::protobuf::RpcController*,
                 const ::mpim::JoinGroupReq*,
                 ::mpim::JoinGroupResp*,
                 ::google::protobuf::Closure*) override;
  void LeaveGroup(::google::protobuf::RpcController*,
                  const ::mpim::LeaveGroupReq*,
                  ::mpim::LeaveGroupResp*,
                  ::google::protobuf::Closure*) override;
  void GetGroupMembers(::google::protobuf::RpcController*,
                       const ::mpim::GetGroupMembersReq*,
                       ::mpim::GetGroupMembersResp*,
                       ::google::protobuf::Closure*) override;
  void GetUserGroups(::google::protobuf::RpcController*,
                     const ::mpim::GetUserGroupsReq*,
                     ::mpim::GetUserGroupsResp*,
                     ::google::protobuf::Closure*) override;
  void GetGroupInfo(::google::protobuf::RpcController*,
                    const ::mpim::GetGroupInfoReq*,
                    ::mpim::GetGroupInfoResp*,
                    ::google::protobuf::Closure*) override;
private:
  std::unique_ptr<MySQL> db_;
  std::unique_ptr<mpim::group::GroupCache> group_cache_;
  bool isGroupMember(int64_t user_id, int64_t group_id);
  bool groupExists(int64_t group_id);
  
  // 缓存相关方法
  std::string serializeGroupInfo(int64_t group_id, const std::string& group_name, const std::string& description);
  std::string serializeGroupMembers(const std::vector<int64_t>& members);
  std::string serializeUserGroups(const std::vector<int64_t>& groups);
};

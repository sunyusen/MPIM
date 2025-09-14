#pragma once
#include "group.pb.h"
#include "rpcprovider.h"
#include "db.h"
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
  bool isGroupMember(int64_t user_id, int64_t group_id);
  bool groupExists(int64_t group_id);
};

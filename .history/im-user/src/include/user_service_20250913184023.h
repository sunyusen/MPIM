#pragma once
#include "user.pb.h"
#include "rpcprovider.h"
#include "db.h"
#include <memory>

class UserServiceImpl : public mpim::UserService {
public:
  UserServiceImpl();
  void Login(::google::protobuf::RpcController*,
             const ::mpim::LoginReq*,
             ::mpim::LoginResp*,
             ::google::protobuf::Closure*) override;
  void Register(::google::protobuf::RpcController*,
                const ::mpim::RegisterReq*,
                ::mpim::RegisterResp*,
                ::google::protobuf::Closure*) override;
  void Logout(::google::protobuf::RpcController*,
              const ::mpim::LogoutReq*,
              ::mpim::LogoutResp*,
              ::google::protobuf::Closure*) override;
  void AddFriend(::google::protobuf::RpcController*,
                 const ::mpim::AddFriendReq*,
                 ::mpim::AddFriendResp*,
                 ::google::protobuf::Closure*) override;
  void GetFriends(::google::protobuf::RpcController*,
                  const ::mpim::GetFriendsReq*,
                  ::mpim::GetFriendsResp*,
                  ::google::protobuf::Closure*) override;
  void RemoveFriend(::google::protobuf::RpcController*,
                    const ::mpim::RemoveFriendReq*,
                    ::mpim::RemoveFriendResp*,
                    ::google::protobuf::Closure*) override;
  
  // 重置所有用户状态为离线（服务启动时调用）
  void resetAllUsersToOffline();
private:
  std::unique_ptr<MySQL> db_;
  bool userExists(const std::string& username, long long* id_out);
  bool isFriend(int64_t user_id, int64_t friend_id);
};

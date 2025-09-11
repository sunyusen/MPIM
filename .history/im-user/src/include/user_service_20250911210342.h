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
private:
  std::unique_ptr<MySQL> db_;
};

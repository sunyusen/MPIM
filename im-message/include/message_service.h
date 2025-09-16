#pragma once
#include "message.pb.h"
#include "presence.pb.h"
#include "mprpcchannel.h"
#include "mprpccontroller.h"
#include "offline_model.h"
#include <memory>

/*
实现了消息服务的核心功能
1，发送消息(Send方法)
2，拉取离线消息(PullOffline方法)
3，消息确认(Ack方法)
*/
class MessageServiceImpl : public mpim::MessageService {
public:
  MessageServiceImpl();
  void Send(::google::protobuf::RpcController*,
            const ::mpim::SendReq*,
            ::mpim::SendResp*,
            ::google::protobuf::Closure*) override;

  void PullOffline(::google::protobuf::RpcController*,
                   const ::mpim::PullOfflineReq*,
                   ::mpim::PullOfflineResp*,
                   ::google::protobuf::Closure*) override;

  void Ack(::google::protobuf::RpcController*,
           const ::mpim::AckReq*,
           ::mpim::AckResp*,
           ::google::protobuf::Closure*) override;

  void SendGroup(::google::protobuf::RpcController*,
                 const ::mpim::SendGroupReq*,
                 ::mpim::SendGroupResp*,
                 ::google::protobuf::Closure*) override;
private:
  OfflineModel offline_;
  std::unique_ptr<MprpcChannel> ch_presence_;
  std::unique_ptr<mpim::PresenceService_Stub> presence_;
};

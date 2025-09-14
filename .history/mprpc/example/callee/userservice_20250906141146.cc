#include <iostream>
#include <string>
#include "user.pb.h"
#include "mprpcapplication.h"
#include "rpcprovider.h"
#include "logger.h"
#include <muduo/base/Logging.h>

/*
UserService原来是一个本地服务，提供了两个进程内的本地方法，Login和GetFriendLists
*/
class UserService : public fixbug::UserServiceRpc	//使用在rpc服务发布端
{
public:
	bool Login(std::string name, std::string pwd)
	{
		std::cout << "dong local service: Login " << std::endl;
		std::cout << "name:" << name << " pwd:" << pwd << std::endl;
		// 改为日志
		// LOG_INFO("doing local service: Login");
		// LOG_INFO("name:%s pwd:%s", name.c_str(), pwd.c_str());
		return true;
	}

	bool Register(uint32_t id, std::string name, std::string pwd)
	{
		// std::cout << "dong local service: Register " << std::endl;
		// std::cout << "id:" << id << " name:" << name << " pwd:" << pwd << std::endl;
		// LOG_INFO("doing local service: Register");
		// LOG_INFO("id:%d name:%s pwd:%s", id, name.c_str(), pwd.c_str());
		return true;
	}

	/*
	重写基类UserServiceRpc的虚函数 下面这些方法都是框架直接调用的
	1. caller ====> Login(LoginRequest) => muduo => callee
	2. callee ====> Login(LoginRequest) => 交到下面重写的Login方法上
	*/
	void Login(::google::protobuf::RpcController* controller,
                       const ::fixbug::LoginRequest* request,
                       ::fixbug::LoginResponse* response,
                       ::google::protobuf::Closure* done)
	{
		//框架给业务上报了请求参数LoginRequest, 业务获取相应数据做本地业务
		std::string name = request->name();
		std::string pwd = request->pwd();

		bool login_result = Login(name, pwd);	//做本地业务

		// 把响应写入response	包括错误码， 错误消息， 返回值
		fixbug::ResultCode *code = response->mutable_result();
		code->set_errcode(0);	//设置错误码
		code->set_errmsg("");	//设置错误信息
		response->set_success(login_result);

		// 执行回调操作	执行响应对象数据的序列化和网络发送(都是用框架来完成的)
		done->Run();	//框架会在Login方法执行完后调用done->Run()，通知框架本次请求处理完毕
	}

	void Register(::google::protobuf::RpcController* controller,
                       const ::fixbug::RegisterRequest* request,
                       ::fixbug::RegisterResponse* response,
                       ::google::protobuf::Closure* done)
	{
		uint32_t id = request->id();
		std::string name = request->name();
		std::string pwd = request->pwd();

		bool register_result = Register(id, name, pwd);	//做本地业务
		response->mutable_result()->set_errcode(0);	//设置错误码
		response->mutable_result()->set_errmsg("");	//设置错误信息
		response->set_success(register_result);	//设置返回值

		done->Run();	//框架会在Register方法执行完后调用done->Run()，通知框架本次请求处理完毕
	}

};

int main(int argc, char **argv)
{
	// 调用框架的初始化操作
	MprpcApplication::Init(argc, argv);

	muduo::Logger::setLogLevel(muduo::Logger::WARN);

	// provider是一个rpc网络服务对象。把UserService对象发布到rpc节点上
	RpcProvider provider;	//创建rpc服务发布者对象
	provider.NotifyService(new UserService());	//注册UserService服务到rpc框架中
	
	// 启动rpc服务发布节点Run以后 进程进入阻塞状态 等待远程的rpc调用请求
	provider.Run();

	// std::cout << "rpc service end!" << std::endl;

	return 0;
}
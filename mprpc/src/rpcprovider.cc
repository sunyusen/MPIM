#include "rpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include "logger.h"
#include "zookeeperutil.h"

/*
service_name => service描述
				=》service* 记录服务对象
				method_name => method方法对象
json(文本)	protobuf(二进制，效率高，有用信息多，rpc方法的描述)
*/
// 这里是框架提供给外部使用的，可以发布rpc方法的函数接口
void RpcProvider::NotifyService(google::protobuf::Service *service)
{
	ServiceInfo service_info;

	// 获取了服务对象的描述信息
	const google::protobuf::ServiceDescriptor *pserviceDesc = service->GetDescriptor();
	// 获取服务的名字
	std::string service_name = pserviceDesc->name();
	// 获取服务对象service的方法的数量
	int methodCnt = pserviceDesc->method_count();

	// std::cout << "service_name:" << service_name << std::endl;
	// LOG_INFO("service_name: %s", service_name.c_str());

	for (int i = 0; i < methodCnt; ++i)
	{
		// 获取了服务对象指定下标的服务方法的描述（抽象描述）
		const google::protobuf::MethodDescriptor *pmethodDesc = pserviceDesc->method(i);
		std::string method_name = pmethodDesc->name();
		service_info.m_methodMap.insert({method_name, pmethodDesc});

		// std::cout << "method_name:" << method_name << std::endl;
		// LOG_INFO("method_name: %s", method_name.c_str());
	}
	service_info.m_service = service;
	std::cout << "service_name: " << service_name << " method_count: " << methodCnt << std::endl;
	m_serviceMap.insert({service_name, service_info});
}

// 启动rpc服务节点，开始提供rpc远程网络调用服务
void RpcProvider::Run()
{
	// 组合了TcpServer
	std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
	uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
	muduo::net::InetAddress address(ip, port);

	std::cout << "ip: " << ip << " port: " << port << std::endl;

	// 创建TcpServer对象
	muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");
	// 绑定连接回调和消息读写回调方法	分离了网络代码和业务代码
	server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
	server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1,
										std::placeholders::_2, std::placeholders::_3));

	// 设置muduo库的线程数量
	server.setThreadNum(4);

	// 把当前rpc节点上要发布的服务全部注册到zk上面， 让rpc client可以从zk上发现服务
	ZkClient zkCli;
	zkCli.Start();
	// service_name为永久性节点  method_name为临时性节点
	// session timeout  30s   zkclient 网络io线程	1/3 * timeout 时间发送ping消息
	for (auto &sp : m_serviceMap)
	{
		// /service_name
		std::string service_path = "/" + sp.first;
		zkCli.Create(service_path.c_str(), nullptr, 0); // 创建永久性节点

		for (auto &mp : sp.second.m_methodMap)
		{
			// /service_name/method_name
			std::string method_path = service_path + "/" + mp.first;
			char method_path_data[128] = {0};
			sprintf(method_path_data, "%s:%d", ip.c_str(), port); // ip:port
			// std::cout << "method_data: " << method_data << std::endl;
			// ZOO_EPHEMERAL表示是一个临时性的结点
			zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL); // 创建临时性节点
		}
	}

	std::cout << "RpcProvider start service at ip:" << ip << " port:" << port << std::endl;

	// 启动服务
	server.start();
	m_eventLoop.loop();
}

// 新的socket连接回调
void RpcProvider::OnConnection(const muduo::net::TcpConnectionPtr &conn)
{
	if (!conn->connected())
	{
		// 和rpc client的连接断开了
		conn->shutdown();
		conn->setTcpNoDelay(true);	//关闭Nagle
	}
}

/*
在框架内部，RpcProvider和RpcConsumer协商好之间通信用的protobuf数据类型
service_name method_name args	定义proto的message类型，进行数据头的序列化和反序列化
								service_name method_name args_size
16UserServiceLoginzhang	san123456

header_size(4个字节) + header_str + args_str
*/
// 已建立连接用户的读写事件回调	如果远程有一个rpc服务的调用请求，那么OnMessage方法就会响应
void RpcProvider::OnMessage(const muduo::net::TcpConnectionPtr &conn,
							muduo::net::Buffer *buffer, muduo::Timestamp)
{
// =========================================从网络接收到的字符流中解析出RPC请求的详细信息===============================================================
	// 网络上接收的远程rpc调用请求的字符流		Login args
	std::string recv_buf = buffer->retrieveAllAsString();

	// 从字符流中读取前4个字节的内容
	uint32_t header_size = 0;
	recv_buf.copy((char *)&header_size, 4, 0);

	// 根据header_size读取数据头的原始字符流, 反序列化数据，得到rpc请求的详细信息
	std::string rpc_header_str = recv_buf.substr(4, header_size);
	mprpc::RpcHeader rpcHeader;
	std::string service_name;
	std::string method_name;
	uint32_t args_size;
	if (rpcHeader.ParseFromString(rpc_header_str))
	{
		// 数据头反序列化成功
		service_name = rpcHeader.service_name();
		method_name = rpcHeader.method_name();
		args_size = rpcHeader.args_size();
	}
	else
	{
		// 数据头反序列化失败
		std::cout << "rpc_header_str: " << rpc_header_str << " ParseFromString failed!" << std::endl;
		return;
	}

	// 获取rpc方法参数的字符流数据
	std::string args_str = recv_buf.substr(4 + header_size, args_size);

	// 打印调试信息
	// std::cout << "=======================================" << std::endl;
	// std::cout << "header_size: " << header_size << std::endl;
	// std::cout << "service_name: " << service_name << std::endl;
	// std::cout << "method_name: " << method_name << std::endl;
	// std::cout << "rpc_header_str: " << rpc_header_str << std::endl;
	// std::cout << "args_size: " << args_size << std::endl;
	// std::cout << "=======================================" << std::endl;
	

// ========================================根据请求的服务名和方法名找到对应的服务对象和方法===============================================================
	// 获取service对象和method对象
	auto it = m_serviceMap.find(service_name);
	if (it == m_serviceMap.end())
	{
		std::cout << "service_name: " << service_name << " is not exist!" << std::endl;
		return;
	}

	auto mit = it->second.m_methodMap.find(method_name);
	if (mit == it->second.m_methodMap.end())
	{
		std::cout << "method_name: " << method_name << " is not exist!" << std::endl;
		return;
	}

	google::protobuf::Service *service = it->second.m_service;		// 获取service对象		new UserService
	const google::protobuf::MethodDescriptor *method = mit->second; // 获取method对象	Login

// ================================================反序列化请求参数，调用服务方法，并将响应结果发送回客户端。====================================================================
	// 生成rpc方法调用的请求request和响应response参数

	// GetRequestPrototype(method)返回一个请求对象的原型
	// .New()方法基于原型创建一个新的请求对象
	// 这个请求对象的类型是由method(方法描述符)决定的,确保与服务方法的参数类型匹配
	// 利用protobuf的反射机制,通过MethodDescriptor获取方法的响应消息类型,并通过MessageFactory返回该类型的原型对象.
	google::protobuf::Message *request = service->GetRequestPrototype(method).New();
	if (!request->ParseFromString(args_str))	// ParseFromString用于将二进制数据解析为protobuf对象
	{
		std::cout << "ParseFromString failed for args_str: " << args_str << std::endl;
		return;
	}
	// GetResponsePrototype(method) 返回一个响应对象的原型。
	// .New() 方法基于原型创建一个新的响应对象。
	// 这个响应对象将用于存储服务方法的执行结果。
	google::protobuf::Message *response = service->GetResponsePrototype(method).New();

	// 给下面的method方法的调用,绑定一个Closure的回调函数
	google::protobuf::Closure* done = 
		google::protobuf::NewCallback<RpcProvider,
								const muduo::net::TcpConnectionPtr&,
								google::protobuf::Message*>
								(this, &RpcProvider::SendRpcResponse, conn, response);

	// 在框架上根据远程rpc请求,调用当前rpc节点上发布的方法
	// new UserService().Login(controller, request, response, done);
	service->CallMethod(method, nullptr, request, response, done);
}

// Closure的回调操作,用于序列化rpc的响应和网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
{
	std::string response_str;
	if(response->SerializeToString(&response_str)){	// 序列化响应对象
		// 序列化成功后,通过网络把rpc方法执行的结果发送回rpc的调用方
		conn->send(response_str);	// 发送响应数据
		conn->shutdown();	//模拟http的短链接服务,由rpcprovider主动断开连接
	}
	else
	{
		std::cout << "serialize reponse failed!" << std::endl;
	}

	conn->shutdown();	//模拟http的短链接服务,由rpcprovider主动断开连接
}
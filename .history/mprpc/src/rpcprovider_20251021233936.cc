#include "rpcprovider.h"
#include "mprpcapplication.h"
#include "rpcheader.pb.h"
#include "logger/logger.h"
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

			LOG_DEBUG << "method_name: " << method_name;
		// LOG_INFO("method_name: %s", method_name.c_str());
	}
	service_info.m_service = service;
	service_info.m_service_owner.reset(); // 不拥有所有权
	LOG_INFO << "service_name: " << service_name << " method_count: " << methodCnt;
	m_serviceMap.emplace(service_name, std::move(service_info));
}


// 启动rpc服务节点，开始提供rpc远程网络调用服务
void RpcProvider::Run()
{
	// 组合了TcpServer, 获取当前的ip和port
	std::string ip = MprpcApplication::GetInstance().GetConfig().Load("rpcserverip");
	uint16_t port = atoi(MprpcApplication::GetInstance().GetConfig().Load("rpcserverport").c_str());
	muduo::net::InetAddress address(ip, port);

	LOG_INFO << "ip: " << ip << " port: " << port;

	// 创建TcpServer对象
	muduo::net::TcpServer server(&m_eventLoop, address, "RpcProvider");
	// 绑定连接回调和消息读写回调方法	分离了网络代码和业务代码
	server.setConnectionCallback(std::bind(&RpcProvider::OnConnection, this, std::placeholders::_1));
	server.setMessageCallback(std::bind(&RpcProvider::OnMessage, this, std::placeholders::_1,
										std::placeholders::_2, std::placeholders::_3));

	// 设置muduo库的线程数量
	server.setThreadNum(4);

	// 把当前rpc节点上要发布的服务全部注册到zk上面， 让rpc client可以从zk上发现服务
	LOG_INFO << "Starting ZooKeeper client...";
	ZkClient zkCli;
	zkCli.Start();
	LOG_INFO << "ZooKeeper client started successfully";
	
	// service_name为永久性节点  method_name为临时性节点
	// session timeout  30s   zkclient 网络io线程	1/3 * timeout 时间发送ping消息
	for (auto &sp : m_serviceMap)
	{
		// /service_name
		std::string service_path = "/" + sp.first;
				LOG_DEBUG << "Creating service path: " << service_path;
		zkCli.Create(service_path.c_str(), nullptr, 0); // 创建永久性节点

		for (auto &mp : sp.second.m_methodMap)
		{
			// /service_name/method_name
			std::string method_path = service_path + "/" + mp.first;
			char method_path_data[128] = {0};
			sprintf(method_path_data, "%s:%d", ip.c_str(), port); // ip:port
						LOG_DEBUG << "Creating method path: " << method_path << " with data: " << method_path_data;
			// ZOO_EPHEMERAL表示是一个临时性的结点
			zkCli.Create(method_path.c_str(), method_path_data, strlen(method_path_data), ZOO_EPHEMERAL); // 创建临时性节点
		}
	}
	
	LOG_INFO << "All services registered to ZooKeeper successfully";

	LOG_INFO << "RpcProvider start service at ip:" << ip << " port:" << port;

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
	}
	else
	{
		// 连接建立后关闭Nagle，降低往返延迟
		conn->setTcpNoDelay(true);
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
	// 基于长度字段的帧解析：当数据不足时返回，等待下一次回调
	while (true)
	{
		if (buffer->readableBytes() < 4) break;
		// peek 4-byte header_size
		uint32_t header_size = 0;
		::memcpy(&header_size, buffer->peek(), 4);
		if (buffer->readableBytes() < (size_t)(4 + header_size)) break;
		std::string rpc_header_str(buffer->peek() + 4, buffer->peek() + 4 + header_size);
		mprpc::RpcHeader rpcHeader;
		if (!rpcHeader.ParseFromString(rpc_header_str))
		{
			LOG_ERROR << "rpc_header_str ParseFromString failed!";
			// 丢弃此帧，以免粘连阻塞
			buffer->retrieve(4 + header_size);
			continue;
		}
		uint32_t args_size = rpcHeader.args_size();
		size_t total = 4 + header_size + args_size;
		if (buffer->readableBytes() < total) break;

		std::string service_name = rpcHeader.service_name();
		std::string method_name = rpcHeader.method_name();
		std::string args_str(buffer->peek() + 4 + header_size, buffer->peek() + total);
		buffer->retrieve(total);

		// 获取service和method
			LOG_DEBUG << "Processing RPC call: " << service_name << "." << method_name;
		
		auto it = m_serviceMap.find(service_name);
		if (it == m_serviceMap.end())
		{
					LOG_ERROR << "service_name: " << service_name << " is not exist!";
			continue;
		}
		auto mit = it->second.m_methodMap.find(method_name);
		if (mit == it->second.m_methodMap.end())
		{
					LOG_ERROR << "method_name: " << method_name << " is not exist!";
			continue;
		}

		google::protobuf::Service *service = it->second.m_service;
		const google::protobuf::MethodDescriptor *method = mit->second;

		google::protobuf::Message *request = service->GetRequestPrototype(method).New();
		if (!request)
		{
					LOG_ERROR << "Failed to create request message";
			continue;
		}
		
		if (!request->ParseFromString(args_str))
		{
					LOG_ERROR << "ParseFromString failed for args_str";
			delete request;
			continue;
		}
		
		google::protobuf::Message *response = service->GetResponsePrototype(method).New();
		if (!response)
		{
					LOG_ERROR << "Failed to create response message";
			delete request;
			continue;
		}

		google::protobuf::Closure* done = 
			google::protobuf::NewCallback<RpcProvider,
							const muduo::net::TcpConnectionPtr&,
							google::protobuf::Message*>
							(this, &RpcProvider::SendRpcResponse, conn, response);

			LOG_DEBUG << "Calling RPC method: " << service_name << "." << method_name;
		service->CallMethod(method, nullptr, request, response, done);
	}
}

// Closure的回调操作,用于序列化rpc的响应和网络发送
void RpcProvider::SendRpcResponse(const muduo::net::TcpConnectionPtr &conn, google::protobuf::Message *response)
{
	LOG_DEBUG << "SendRpcResponse: Sending response to " << conn->peerAddress().toIpPort();
	
	std::string response_str;
	if(response && response->SerializeToString(&response_str)){
		// 发送长度前缀 + 内容（保持长连接，不主动关闭）
		uint32_t len = (uint32_t)response_str.size();
		char lenbuf[4];
		::memcpy(lenbuf, &len, 4);
		conn->send(std::string(lenbuf, 4));
		conn->send(response_str);
		
			LOG_DEBUG << "SendRpcResponse: Response sent successfully, size=" << len;
	}
	else
	{
			LOG_ERROR << "serialize response failed!";
	}
	
	// Clean up the response message to prevent memory leaks
	if (response) {
		delete response;
	}

	int* arr = new int[10];
}
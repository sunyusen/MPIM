#include "mprpcchannel.h"
#include "rpcheader.pb.h"
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <unistd.h>
#include "mprpcapplication.h"
#include "mprpccontroller.h"
#include "zookeeperutil.h"
// 可选：TCP_NODELAY（仅在支持的平台使用）
#ifdef TCP_NODELAY
#include <netinet/tcp.h>
#endif
#include <mutex>
#include <unordered_map>
#include <chrono>

/*
header_size + service_name method_name args_size + args
*/
// 所有通过stub代理对象调用的rpc方法，都走到了这里了，统一做rpc方法调用的数据序列化和网络发送
void MprpcChannel::CallMethod(const google::protobuf::MethodDescriptor *method,
							  google::protobuf::RpcController *controller,
							  const google::protobuf::Message *request,
							  google::protobuf::Message *response,
							  google::protobuf::Closure *done)
{
	using namespace std::chrono;
	const google::protobuf::ServiceDescriptor *sd = method->service();
	std::string service_name = sd->name();	  // 获取服务名
	std::string method_name = method->name(); // 获取方法名

	// 获取参数的序列化字符串长度	args_size
	uint32_t args_size = 0;
	std::string args_str;
	if (request->SerializeToString(&args_str))
	{
		args_size = args_str.size();
	}
	else
	{
		controller->SetFailed("serialize request error!");
		return;
	}

	// 定义rpc的请求header
	mprpc::RpcHeader rpcHeader;
	rpcHeader.set_service_name(service_name); // 设置服务名
	rpcHeader.set_method_name(method_name);	  // 设置方法名
	rpcHeader.set_args_size(args_size);		  // 设置参数的序列化字符串长度

	uint32_t header_size = 0;
	std::string rpc_header_str;
	if (rpcHeader.SerializeToString(&rpc_header_str))
	{
		header_size = rpc_header_str.size();
	}
	else
	{
		controller->SetFailed("serialize rpc header error!");
		return;
	}

	// 组织待发送的rpc请求的字符串
	std::string send_rpc_str;
	send_rpc_str.insert(0, std::string((char *)&header_size, 4)); // header_size
	send_rpc_str += rpc_header_str;								  // rpcheader
	send_rpc_str += args_str;								  // args

	// 使用tcp编程，完成rpc方法的远程调用
	int clientfd = socket(AF_INET, SOCK_STREAM, 0);
	if (-1 == clientfd)
	{
		char errtext[512] = {0};
		sprintf(errtext, "create socket error! errno: %d", errno);
		controller->SetFailed(errtext);
		return;
	}

	// 单例 ZK + 地址缓存，减少每次调用的服务发现开销
	struct CacheEntry { sockaddr_in addr; steady_clock::time_point expire; };
	static std::once_flag s_zk_once;
	static ZkClient s_zk;
	static std::mutex s_cache_mu;
	static std::unordered_map<std::string, CacheEntry> s_addr_cache;

	std::call_once(s_zk_once, [](){ s_zk.Start(); });

	std::string method_path = "/" + service_name + "/" + method_name;
	sockaddr_in server_addr{};
	bool need_resolve = true;
	const auto now_tp = steady_clock::now();
	const auto ttl = milliseconds(1000); // 1s TTL
	{
		std::lock_guard<std::mutex> lk(s_cache_mu);
		auto it = s_addr_cache.find(method_path);
		if (it != s_addr_cache.end() && now_tp < it->second.expire)
		{
			server_addr = it->second.addr;
			need_resolve = false;
		}
	}
	if (need_resolve)
	{
		// 127.0.0.1:8000
		std::string host_data = s_zk.GetData(method_path.c_str());
		if (host_data == "")
		{
			controller->SetFailed(method_path + " is not exist!");
			close(clientfd);
			return;
		}
		int idx = host_data.find(":");
		if (idx == -1)
		{
			controller->SetFailed(method_path + " address is invalid!");
			close(clientfd);
			return;
		}
		std::string ip = host_data.substr(0, idx);
		uint16_t port = atoi(host_data.substr(idx + 1, host_data.size()-idx).c_str());

		server_addr.sin_family = AF_INET;
		server_addr.sin_port = htons(port);
		server_addr.sin_addr.s_addr = inet_addr(ip.c_str());

		std::lock_guard<std::mutex> lk(s_cache_mu);
		s_addr_cache[method_path] = CacheEntry{server_addr, now_tp + ttl};
	}

	// 连接rpc服务节点
	if (-1 == connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr)))
	{
		char errtext[512] = {0};
		sprintf(errtext, "connect error! errno:  %d", errno);
		controller->SetFailed(errtext);
		close(clientfd);
		return;
	}

	// 发送rpc请求（尽量降低RTT）
#ifdef TCP_NODELAY
	int one = 1;
	setsockopt(clientfd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
#endif
	if (-1 == send(clientfd, send_rpc_str.c_str(), send_rpc_str.size(), 0))
	{
		char errtext[512] = {0};
		sprintf(errtext, "send error! errno:   %d", errno);
		controller->SetFailed(errtext);
		close(clientfd);
		return;
	}

	// 读取4字节长度前缀
	uint32_t resp_len = 0;
	int recvd = 0;
	char lenbuf[4];
	while (recvd < 4)
	{
		int n = recv(clientfd, lenbuf + recvd, 4 - recvd, 0);
		if (n > 0) { recvd += n; continue; }
		if (n == 0) { controller->SetFailed("connection closed before length"); close(clientfd); return; }
		char errtext[512] = {0};
		sprintf(errtext, "recv error! errno:   %d", errno);
		controller->SetFailed(errtext);
		close(clientfd);
		return;
	}
	::memcpy(&resp_len, lenbuf, 4);

	// 读取指定长度的响应体
	std::string resp_buf;
	resp_buf.resize(resp_len);
	size_t got = 0;
	while (got < resp_buf.size())
	{
		int n = recv(clientfd, &resp_buf[got], (int)(resp_buf.size() - got), 0);
		if (n > 0) { got += n; continue; }
		if (n == 0) { controller->SetFailed("connection closed mid-body"); close(clientfd); return; }
		char errtext[512] = {0};
		sprintf(errtext, "recv error! errno:   %d", errno);
		controller->SetFailed(errtext);
		close(clientfd);
		return;
	}

	// 反序列化rpc调用的响应数据
	if (!response->ParseFromArray(resp_buf.data(), (int)resp_buf.size()))
	{
		controller->SetFailed("parse response error!");
		close(clientfd);
		return;
	}

	close(clientfd);
}
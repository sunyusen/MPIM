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
#include <vector>

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
    const google::protobuf::ServiceDescriptor *sd = method->service();
    const std::string service_name = sd->name();
    const std::string method_name = method->name();

    // 1) 构造请求帧（长度前缀 + header + args）
    std::string frame;
    if (!buildRequestFrame(method, request, frame, controller))
        return;

    // 2) 解析服务地址（带本地 TTL 缓存）
    sockaddr_in server_addr{};
    if (!resolveEndpoint(service_name, method_name, server_addr, controller))
        return;

    // 3) 连接池获取连接
    const std::string key = "/" + service_name + "/" + method_name;
    int clientfd = getConnection(key, server_addr);
    if (clientfd == -1)
    {
        char errtext[256] = {0};
        sprintf(errtext, "connect error! errno:  %d", errno);
        controller->SetFailed(errtext);
        return;
    }

    // 4) 发送与接收
    if (!sendAll(clientfd, frame.data(), frame.size(), controller))
    {
        close(clientfd);
        return;
    }
    if (!recvResponse(clientfd, response, controller))
    {
        close(clientfd);
        return;
    }

    // 5) 归还连接
    returnConnection(key, clientfd);
}

// ---- helpers ----
// 构建一个RPC请求的序列化数据帧
bool MprpcChannel::buildRequestFrame(const google::protobuf::MethodDescriptor* method,
                                     const google::protobuf::Message* request,
                                     std::string& out,
                                     google::protobuf::RpcController* controller)
{
	// 从MethodDescriptor中提取服务名和方法名
    const google::protobuf::ServiceDescriptor *sd = method->service();
    std::string service_name = sd->name();
    std::string method_name = method->name();

	// 序列化请求参数
    std::string args_str;
    if (!request->SerializeToString(&args_str))
    {
        controller->SetFailed("serialize request error!");
        return false;
    }

	// 构造RPC请求头
    mprpc::RpcHeader header;
    header.set_service_name(service_name);
    header.set_method_name(method_name);
    header.set_args_size((uint32_t)args_str.size());

    std::string header_str;
    if (!header.SerializeToString(&header_str))
    {
        controller->SetFailed("serialize rpc header error!");
        return false;
    }

	//构造完整的请求帧
    uint32_t header_size = (uint32_t)header_str.size();
    out.clear();
    out.reserve(4 + header_str.size() + args_str.size());
    out.append(std::string((char*)&header_size, 4));
    out.append(header_str);
    out.append(args_str);
    return true;
}

// 解析RPC服务的网络地址，并将结果存储到sockaddr_in结构中。他通过检查本地缓存或从zookeeper获取服务地址，确保高效且准确地服务端点
bool MprpcChannel::resolveEndpoint(const std::string& service,
                                   const std::string& method,
                                   struct sockaddr_in& out,
                                   google::protobuf::RpcController* controller)
{
    using namespace std::chrono;
    struct CacheEntry { sockaddr_in addr; steady_clock::time_point expire; };
    static std::once_flag s_zk_once;	//确保zookeeper客户端只初始化一次
    static ZkClient s_zk;			// zookeeper客户端，用于从zookeeper获取服务地址
    static std::mutex s_cache_mu;	//互斥锁，保护地址缓存的线程安全
    static std::unordered_map<std::string, CacheEntry> s_addr_cache;//地址缓存，存储服务方法对应的网络地址及其过期时间

// 这什么用法
// 函数定义与mutex, call_once保证可调用对象只被执行一次,防止多次初始化zk客户端
// s_zk_once:标志对象
    std::call_once(s_zk_once, [](){ s_zk.Start(); });

	//构造方法路径
    const std::string method_path = "/" + service + "/" + method;
    const auto now_tp = steady_clock::now();
    const auto ttl = milliseconds(1000);

	// 检查缓存中是否存在对应的服务地址
	// 如果缓存未过期，直接返回缓存的地址
    {
        std::lock_guard<std::mutex> lk(s_cache_mu);
        auto it = s_addr_cache.find(method_path);
        if (it != s_addr_cache.end() && now_tp < it->second.expire)
        {
            out = it->second.addr;
            return true;
        }
    }

    std::string host_data = s_zk.GetData(method_path.c_str());
    if (host_data.empty())
    {
        controller->SetFailed(method_path + " is not exist!");
        return false;
    }
    int idx = host_data.find(":");
    if (idx == -1)
    {
        controller->SetFailed(method_path + " address is invalid!");
        return false;
    }
    std::string ip = host_data.substr(0, (size_t)idx);
    uint16_t port = (uint16_t)atoi(host_data.substr((size_t)idx + 1).c_str());

    out.sin_family = AF_INET;
    out.sin_port = htons(port);
    out.sin_addr.s_addr = inet_addr(ip.c_str());

    {
        std::lock_guard<std::mutex> lk(s_cache_mu);
        s_addr_cache[method_path] = CacheEntry{out, now_tp + ttl};
    }
    return true;
}

// 获取一个TCP连接的文件描述符，实现了一个简单的连接池机制，用于复用已有的连接，避免频繁创建和销毁连接带来的性能开销
int MprpcChannel::getConnection(const std::string& key, const struct sockaddr_in& addr)
{
    static std::mutex s_pool_mu;
    static std::unordered_map<std::string, std::vector<int>> s_conn_pool;
    static const size_t kMaxPoolPerKey = 64;

    {
        std::lock_guard<std::mutex> lk(s_pool_mu);
        auto &vec = s_conn_pool[key];
        if (!vec.empty()) { int fd = vec.back(); vec.pop_back(); return fd; }
    }

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) return -1;
#ifdef TCP_NODELAY
    int one = 1; setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
#endif
    if (-1 == connect(fd, (const struct sockaddr *)&addr, sizeof(addr)))
    {
        close(fd);
        return -1;
    }
    return fd;
}

void MprpcChannel::returnConnection(const std::string& key, int fd)
{
    static std::mutex s_pool_mu;
    static std::unordered_map<std::string, std::vector<int>> s_conn_pool;
    static const size_t kMaxPoolPerKey = 64;

    std::lock_guard<std::mutex> lk(s_pool_mu);
    auto &vec = s_conn_pool[key];
    if (vec.size() < kMaxPoolPerKey) vec.push_back(fd); else close(fd);
}

bool MprpcChannel::sendAll(int fd, const char* data, size_t len, google::protobuf::RpcController* controller)
{
    const char* p = data;
    size_t left = len;
    while (left > 0)
    {
        ssize_t n = ::send(fd, p, left, 0);
        if (n > 0) { p += n; left -= (size_t)n; continue; }
        char errtext[512] = {0};
        sprintf(errtext, "send error! errno:   %d", errno);
        controller->SetFailed(errtext);
        return false;
    }
    return true;
}

bool MprpcChannel::recvResponse(int fd, google::protobuf::Message* response, google::protobuf::RpcController* controller)
{
    uint32_t resp_len = 0;
    int recvd = 0;
    char lenbuf[4];
    while (recvd < 4)
    {
        int n = ::recv(fd, lenbuf + recvd, 4 - recvd, 0);
        if (n > 0) { recvd += n; continue; }
        if (n == 0) { controller->SetFailed("connection closed before length"); return false; }
        char errtext[512] = {0};
        sprintf(errtext, "recv error! errno:   %d", errno);
        controller->SetFailed(errtext);
        return false;
    }
    ::memcpy(&resp_len, lenbuf, 4);

    std::string resp_buf;
    resp_buf.resize(resp_len);
    size_t got = 0;
    while (got < resp_buf.size())
    {
        int n = ::recv(fd, &resp_buf[got], (int)(resp_buf.size() - got), 0);
        if (n > 0) { got += n; continue; }
        if (n == 0) { controller->SetFailed("connection closed mid-body"); return false; }
        char errtext[512] = {0};
        sprintf(errtext, "recv error! errno:   %d", errno);
        controller->SetFailed(errtext);
        return false;
    }

    if (!response->ParseFromArray(resp_buf.data(), (int)resp_buf.size()))
    {
        controller->SetFailed("parse response error!");
        return false;
    }
    return true;
}
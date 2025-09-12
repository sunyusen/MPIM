#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <unordered_map>
#include <condition_variable>

using namespace std::chrono;

struct Cmd {
    std::string host = "127.0.0.1";
    int port = 6000;
    int sender_count = 4;    // 发送方数量
    int receiver_count = 4;  // 接收方数量
    int seconds = 15;
    int payload = 64;
    std::string user_base = "user";
    std::string password = "123456";
    int messages_per_sender = 100; // 每个发送方发送的消息数
};

static bool send_all(int fd, const char* data, size_t len) {
    size_t left = len; const char* p = data;
    while (left > 0) {
        ssize_t n = ::send(fd, p, left, 0);
        if (n > 0) { p += n; left -= (size_t)n; continue; }
        if (n == -1 && (errno == EINTR || errno == EAGAIN)) continue;
        return false;
    }
    return true;
}

static bool send_line(int fd, const std::string& s) {
    std::string line = s; line.push_back('\n');
    return send_all(fd, line.data(), line.size());
}

static bool recv_line(int fd, std::string& out) {
    out.clear();
    char buf[1024];
    while (true) {
        ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
        if (n > 0) {
            for (ssize_t i = 0; i < n; ++i) {
                char c = buf[i];
                if (c == '\n') return true;
                if (c != '\r') out.push_back(c);
            }
            if (out.size() > 65536) return false;
            continue;
        }
        if (n == 0) return false;
        if (errno == EINTR) continue;
        if (errno == EAGAIN || errno == EWOULDBLOCK) { std::this_thread::yield(); continue; }
        return false;
    }
}

static int dial(const std::string& host, int port) {
    int fd = ::socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) return -1;
    int one = 1; setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
    sockaddr_in addr{}; addr.sin_family = AF_INET; addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(host.c_str());
    if (-1 == ::connect(fd, (sockaddr*)&addr, sizeof(addr))) { ::close(fd); return -1; }
    std::string wl; recv_line(fd, wl);
    return fd;
}

struct E2EStats {
    std::atomic<uint64_t> sent{0}, received{0}, failed{0};
    std::vector<double> latencies_ms;
    std::mutex mu;
    std::condition_variable cv;
    std::unordered_map<std::string, steady_clock::time_point> pending_messages;
};

static double pct(std::vector<double>& v, double p) {
    if (v.empty()) return 0; 
    size_t k = (size_t)std::llround((p/100.0)*(v.size()-1));
    std::nth_element(v.begin(), v.begin()+k, v.end()); 
    return v[k];
}

// 接收方线程：监听消息并记录端到端延迟
void receiver_thread(int fd, int64_t uid, E2EStats& stats, std::atomic<bool>& stop) {
    std::string line;
    while (!stop.load()) {
        if (!recv_line(fd, line)) break;
        
        // 解析 MSG 行：MSG id=123 from=456 ts=789 text=hello
        if (line.rfind("MSG ", 0) == 0) {
            // 简单解析：提取 id 和 from
            size_t id_pos = line.find("id=");
            size_t from_pos = line.find("from=");
            if (id_pos != std::string::npos && from_pos != std::string::npos) {
                size_t id_end = line.find(' ', id_pos + 3);
                size_t from_end = line.find(' ', from_pos + 5);
                if (id_end != std::string::npos && from_end != std::string::npos) {
                    std::string msg_id = line.substr(id_pos + 3, id_end - id_pos - 3);
                    std::string from_uid = line.substr(from_pos + 5, from_end - from_pos - 5);
                    
                    // 查找对应的发送时间
                    std::string key = from_uid + ":" + msg_id;
                    std::lock_guard<std::mutex> lk(stats.mu);
                    auto it = stats.pending_messages.find(key);
                    if (it != stats.pending_messages.end()) {
                        auto now = steady_clock::now();
                        double latency = duration<double, std::milli>(now - it->second).count();
                        stats.latencies_ms.push_back(latency);
                        stats.pending_messages.erase(it);
                        stats.received.fetch_add(1);
                    }
                }
            }
        }
    }
}

// 发送方线程：发送消息并记录发送时间
void sender_thread(int sender_fd, int64_t sender_uid, int64_t receiver_uid, 
                  E2EStats& stats, const Cmd& cmd, std::atomic<bool>& stop) {
    std::string payload(cmd.payload, 'x');
    int sent_count = 0;
    
    while (!stop.load() && sent_count < cmd.messages_per_sender) {
        auto t0 = steady_clock::now();
        std::string msg_id = std::to_string(sender_uid * 10000 + sent_count);
        
        // 记录发送时间
        {
            std::lock_guard<std::mutex> lk(stats.mu);
            std::string key = std::to_string(sender_uid) + ":" + msg_id;
            stats.pending_messages[key] = t0;
        }
        
        // 发送消息
        std::string cmd_str = "SEND " + std::to_string(receiver_uid) + " " + payload;
        if (!send_line(sender_fd, cmd_str)) {
            stats.failed.fetch_add(1);
            break;
        }
        
        // 读取响应
        std::string resp;
        if (!recv_line(sender_fd, resp)) {
            stats.failed.fetch_add(1);
            break;
        }
        
        if (resp.rfind("+OK", 0) == 0) {
            stats.sent.fetch_add(1);
        } else {
            stats.failed.fetch_add(1);
        }
        
        sent_count++;
        std::this_thread::sleep_for(milliseconds(10)); // 避免过于频繁
    }
}

int main(int argc, char** argv) {
    Cmd cmd;
    for (int i=1;i<argc;i++){
        std::string a=argv[i];
        auto eat=[&](const char* k, auto& dst){ 
            if(a.rfind(k,0)==0){ 
                dst=static_cast<decltype(dst)>(std::stoi(a.substr(strlen(k)))); 
                return true;
            } 
            return false;
        };
        if (a.rfind("--host=",0)==0) cmd.host = a.substr(7);
        else if (a.rfind("--port=",0)==0) eat("--port=", cmd.port);
        else if (a.rfind("--senders=",0)==0) eat("--senders=", cmd.sender_count);
        else if (a.rfind("--receivers=",0)==0) eat("--receivers=", cmd.receiver_count);
        else if (a.rfind("--seconds=",0)==0) eat("--seconds=", cmd.seconds);
        else if (a.rfind("--payload=",0)==0) eat("--payload=", cmd.payload);
        else if (a.rfind("--user_base=",0)==0) cmd.user_base = a.substr(12);
        else if (a.rfind("--password=",0)==0) cmd.password = a.substr(11);
        else if (a.rfind("--msgs_per_sender=",0)==0) eat("--msgs_per_sender=", cmd.messages_per_sender);
    }

    std::cout << "[e2e-bench] host="<<cmd.host<<" port="<<cmd.port
              << " senders="<<cmd.sender_count<<" receivers="<<cmd.receiver_count
              << " seconds="<<cmd.seconds<<" payload="<<cmd.payload<<"\n";

    // 建立连接并登录
    std::vector<int> sender_fds, receiver_fds;
    std::vector<int64_t> sender_uids, receiver_uids;
    
    // 创建发送方连接
    for (int i = 0; i < cmd.sender_count; i++) {
        int fd = dial(cmd.host, cmd.port);
        if (fd == -1) { std::cerr << "sender dial fail\n"; return 1; }
        std::string user = cmd.user_base + std::to_string(i + 1);
        if (!send_line(fd, "LOGIN " + user + " " + cmd.password)) return 1;
        std::string resp; if (!recv_line(fd, resp) || resp.rfind("+OK", 0) != 0) {
            std::cerr << "sender login fail: " << resp << "\n"; return 1;
        }
        // 提取 uid
        size_t uid_pos = resp.find("uid=");
        if (uid_pos != std::string::npos) {
            int64_t uid = std::stoll(resp.substr(uid_pos + 4));
            sender_fds.push_back(fd);
            sender_uids.push_back(uid);
        }
    }
    
    // 创建接收方连接
    for (int i = 0; i < cmd.receiver_count; i++) {
        int fd = dial(cmd.host, cmd.port);
        if (fd == -1) { std::cerr << "receiver dial fail\n"; return 1; }
        std::string user = cmd.user_base + std::to_string(cmd.sender_count + i + 1);
        if (!send_line(fd, "LOGIN " + user + " " + cmd.password)) return 1;
        std::string resp; if (!recv_line(fd, resp) || resp.rfind("+OK", 0) != 0) {
            std::cerr << "receiver login fail: " << resp << "\n"; return 1;
        }
        // 提取 uid
        size_t uid_pos = resp.find("uid=");
        if (uid_pos != std::string::npos) {
            int64_t uid = std::stoll(resp.substr(uid_pos + 4));
            receiver_fds.push_back(fd);
            receiver_uids.push_back(uid);
        }
    }

    E2EStats stats;
    std::atomic<bool> stop{false};
    std::vector<std::thread> threads;

    // 启动接收方线程
    for (size_t i = 0; i < receiver_fds.size(); i++) {
        threads.emplace_back(receiver_thread, receiver_fds[i], receiver_uids[i], 
                           std::ref(stats), std::ref(stop));
    }

    // 启动发送方线程（每个发送方对应一个接收方）
    for (size_t i = 0; i < sender_fds.size() && i < receiver_uids.size(); i++) {
        threads.emplace_back(sender_thread, sender_fds[i], sender_uids[i], receiver_uids[i],
                           std::ref(stats), std::ref(cmd), std::ref(stop));
    }

    // 运行指定时间
    std::this_thread::sleep_for(std::chrono::seconds(cmd.seconds));
    stop.store(true);

    // 等待所有线程结束
    for (auto& t : threads) {
        if (t.joinable()) t.join();
    }

    // 关闭连接
    for (int fd : sender_fds) if (fd != -1) ::close(fd);
    for (int fd : receiver_fds) if (fd != -1) ::close(fd);

    // 输出统计结果
    auto& v = stats.latencies_ms;
    std::cout << "[e2e-bench] sent="<<stats.sent.load()<<" received="<<stats.received.load()
              <<" failed="<<stats.failed.load()<<"\n";
    if (!v.empty()) {
        double e2e_qps = stats.received.load() / (double)cmd.seconds;
        std::cout << "E2E QPS="<<e2e_qps
                  << " P50="<<pct(v,50)
                  << " P95="<<pct(v,95)
                  << " P99="<<pct(v,99) << " ms\n";
    } else {
        std::cout << "No end-to-end messages received\n";
    }
    return 0;
}

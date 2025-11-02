#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>
#include <poll.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include <algorithm>
#include <cmath>
#include <cerrno>

using namespace std::chrono;

struct Cmd
{
	std::string host = "127.0.0.1";
	int port = 6000; // im-gateway 监听端口（请按实际网关端口传参）
	int pairs = 4;	 // 会话对数量（每对2个连接：sender/receiver）
	int threads = 4; // 工作线程数
	int seconds = 15;
	int payload = 64;
	std::string user_base = "sunyusen"; // 账号前缀：user1,user2,...
	std::string password = "123456";
};

static bool send_all(int fd, const char *data, size_t len)
{
	size_t left = len;
	const char *p = data;
	while (left > 0)
	{
		ssize_t n = ::send(fd, p, left, 0);
		if (n > 0)
		{
			p += n;
			left -= (size_t)n;
			continue;
		}
		if (n == -1 && (errno == EINTR || errno == EAGAIN))
			continue;
		return false;
	}
	return true;
}

static bool send_line(int fd, const std::string &s)
{
	std::string line = s;
	line.push_back('\n');
	// std::cout << line << std::endl;
	return send_all(fd, line.data(), line.size());
}

static bool recv_line(int fd, std::string &out)
{
	out.clear();
	char buf[1024];
	while (true)
	{
		ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
		if (n > 0)
		{
			for (ssize_t i = 0; i < n; ++i)
			{
				char c = buf[i];
				if (c == '\n')
					return true;
				if (c != '\r')
					out.push_back(c);
			}
			if (out.size() > 65536)
				return false; // 防御
			continue;
		}
		if (n == 0)
			return false; // 断开
		if (errno == EINTR)
			continue;
		if (errno == EAGAIN || errno == EWOULDBLOCK)
		{
			std::this_thread::yield();
			continue;
		}
		return false;
	}
}

// 带超时的按行读取，避免永久阻塞
static bool recv_line_timeout(int fd, std::string &out, int timeout_ms)
{
    out.clear();
    char buf[1024];
    auto deadline = steady_clock::now() + std::chrono::milliseconds(timeout_ms);
    for (;;)
    {
        auto now = steady_clock::now();
        if (now >= deadline)
            return false; // 超时
        int remain_ms = (int)std::chrono::duration_cast<std::chrono::milliseconds>(deadline - now).count();
        struct pollfd pfd { fd, POLLIN, 0 };
        int pr = ::poll(&pfd, 1, remain_ms);
        if (pr > 0)
        {
            if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL))
                return false;
            ssize_t n = ::recv(fd, buf, sizeof(buf), 0);
            if (n > 0)
            {
                for (ssize_t i = 0; i < n; ++i)
                {
                    char c = buf[i];
                    if (c == '\n')
                        return true;
                    if (c != '\r')
                        out.push_back(c);
                }
                if (out.size() > 65536)
                    return false; // 防御
                continue;
            }
            if (n == 0)
                return false; // 对端关闭
            if (errno == EINTR)
                continue;
            if (errno == EAGAIN || errno == EWOULDBLOCK)
                continue;
            return false;
        }
        else if (pr == 0)
        {
            return false; // 超时
        }
        else
        {
            if (errno == EINTR)
                continue;
            return false;
        }
    }
}

static int dial(const std::string &host, int port)
{
	int fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
		return -1;
	int one = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(host.c_str());
	if (-1 == ::connect(fd, (sockaddr *)&addr, sizeof(addr)))
	{
		::close(fd);
		return -1;
	}
    // 读欢迎行（限时）
    std::string line;
    if (!recv_line_timeout(fd, line, 3000))
    {
        ::close(fd);
        return -1;
    }
	return fd;
}

struct Stats
{
	std::atomic<uint64_t> ok{0}, fail{0};
	std::vector<double> lat_ms;
	std::mutex mu;
};

static double pct(std::vector<double> &v, double p)
{
	if (v.empty())
		return 0;

	// 目标下标：四舍五入到最近的秩位置
	const double pos = (p / 100.0) * (v.size() - 1);
	const size_t k = static_cast<size_t>(std::llround(pos)); // 需要 <cmath>

	std::nth_element(v.begin(), v.begin() + k, v.end());
	return v[k];
}

int main(int argc, char **argv)
{
	Cmd cmd;
	for (int i = 1; i < argc; i++)
	{
		std::string a = argv[i];
		auto eat = [&](const char *key, auto &dst)
		{
			if (a.rfind(key, 0) == 0)
			{
				// 这里 dst 是 int&，直接赋值即可，不要强转成引用类型
				dst = std::stoi(a.substr(strlen(key)));
				return true;
			}
			return false;
		};
		if (a.rfind("--host=", 0) == 0)
			cmd.host = a.substr(7);
		else if (a.rfind("--port=", 0) == 0)
			eat("--port=", cmd.port);
		else if (a.rfind("--pairs=", 0) == 0)
			eat("--pairs=", cmd.pairs);
		else if (a.rfind("--threads=", 0) == 0)
			eat("--threads=", cmd.threads);
		else if (a.rfind("--seconds=", 0) == 0)
			eat("--seconds=", cmd.seconds);
		else if (a.rfind("--payload=", 0) == 0)
			eat("--payload=", cmd.payload);
		else if (a.rfind("--user_base=", 0) == 0)
			cmd.user_base = a.substr(12);
		else if (a.rfind("--password=", 0) == 0)
			cmd.password = a.substr(11);
	}

	std::cout << "[gw-bench] host=" << cmd.host << " port=" << cmd.port
			  << " pairs=" << cmd.pairs << " threads=" << cmd.threads
			  << " seconds=" << cmd.seconds << " payload=" << cmd.payload << "\n";

	// 预先建立每对的 sender/receiver 连接并登录、绑定
	struct ConnPair
	{
		int a{-1}, b{-1};
	};
	std::vector<ConnPair> cps(cmd.pairs);
	for (int i = 0; i < cmd.pairs; i++)
	{
		int a = dial(cmd.host, cmd.port);
		if (a == -1)
		{
			std::cerr << "dial A fail\n";
			return 1;
		}
		int b = dial(cmd.host, cmd.port);
		if (b == -1)
		{
			std::cerr << "dial B fail\n";
			return 1;
		}
		// user ids: base+(2*i+1) and base+(2*i+2)
		auto userA = cmd.user_base + std::to_string(2 * i + 1);
		auto userB = cmd.user_base + std::to_string(2 * i + 2);
		std::cout << "userA=" << userA << " userB=" << userB << "\n";
		if (!send_line(a, "LOGIN " + userA + " " + cmd.password))
			return 1;
		std::string ln;
		if (!recv_line(a, ln) || ln.rfind("+OK", 0) != 0)
		{
			std::cerr << "login A fail: " << ln << "\n";
			return 1;
		}
		if (!send_line(b, "LOGIN " + userB + " " + cmd.password))
			return 1;
		if (!recv_line(b, ln) || ln.rfind("+OK", 0) != 0)
		{
			std::cerr << "login B fail: " << ln << "\n";
			return 1;
		}
		cps[i] = {a, b};
	}

	std::atomic<bool> stop{false};
	Stats st_send, st_pull;
	std::string payload(cmd.payload, 'x');

	auto worker = [&](int tid)
	{
		int per = (cmd.pairs + cmd.threads - 1) / cmd.threads;
		int beg = tid * per, end = std::min(cmd.pairs, (tid + 1) * per);
		for (;;)
		{
			if (stop.load())
				break;
			for (int i = beg; i < end; i++)
			{
				// A -> B 发送
				auto t0 = steady_clock::now();
				// 目标 uid 这里假设为 (2*i+2)，与登录策略一致
				std::string to = std::to_string(2 * i + 2);
				if (!send_line(cps[i].a, std::string("SEND ") + to + " " + payload))
					continue;
				std::string ln;
				if (!recv_line(cps[i].a, ln))
					continue;
				bool ok = ln.rfind("+OK", 0) == 0;
				auto t1 = steady_clock::now();
				double ms = duration<double, std::milli>(t1 - t0).count();
				{
					std::lock_guard<std::mutex> g(st_send.mu);
					st_send.lat_ms.push_back(ms);
					(ok ? st_send.ok : st_send.fail).fetch_add(1, std::memory_order_relaxed);
				}
				// 可选：B 接收一条 MSG 行（非阻塞尝试，避免阻塞）
			}
		}
	};

	// 使用多线程模拟并发送消息，每个线程负责一部分连接对，循环发送消息
	std::vector<std::thread> th;
	for (int t = 0; t < cmd.threads; t++)
		th.emplace_back(worker, t);
	std::this_thread::sleep_for(std::chrono::seconds(cmd.seconds));
	stop.store(true);
	for (auto &t : th)
		t.join();

	// 关闭连接
	for (auto &cp : cps)
	{
		if (cp.a != -1)
			::close(cp.a);
		if (cp.b != -1)
			::close(cp.b);
	}

	auto &v = st_send.lat_ms;
	std::cout << "[gw-bench] SEND ok=" << st_send.ok.load() << " fail=" << st_send.fail.load() << "\n";
	if (!v.empty())
	{
		double qps = (st_send.ok.load() + st_send.fail.load()) / (double)cmd.seconds;
		std::cout << "QPS=" << qps
				  << " P50=" << pct(v, 50)
				  << " P95=" << pct(v, 95)
				  << " P99=" << pct(v, 99) << " ms\n";
	}
	return 0;
}

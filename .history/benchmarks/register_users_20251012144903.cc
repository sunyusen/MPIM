#include <arpa/inet.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <unistd.h>

#include <atomic>
#include <chrono>
#include <cstring>
#include <iostream>
#include <string>
#include <thread>

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
				return false;
			continue;
		}
		if (n == 0)
			return false;
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

int main(int argc, char **argv)
{
	std::string host = "127.0.0.1";
	int port = 6000;
	std::string base = "user";
	std::string password = "123456";
	int from = 1, to = 1000; // [from, to]

	for (int i = 1; i < argc; i++)
	{
		std::string a = argv[i];
		auto eat=[&](const char* k, auto& dst){
            if (a.rfind(k,0)==0) {
                // 直接赋值，别把临时值强转成引用
                dst = std::stoi(a.substr(strlen(k)));
                return true;
            }
            return false;
        };
		if (a.rfind("--host=", 0) == 0)
			host = a.substr(7);
		else if (a.rfind("--port=", 0) == 0)
			eat("--port=", port);
		else if (a.rfind("--base=", 0) == 0)
			base = a.substr(7);
		else if (a.rfind("--password=", 0) == 0)
			password = a.substr(11);
		else if (a.rfind("--from=", 0) == 0)
			eat("--from=", from);
		else if (a.rfind("--to=", 0) == 0)
			eat("--to=", to);
	}

	int fd = ::socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		std::cerr << "socket fail\n";
		return 1;
	}
	int one = 1;
	setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
	sockaddr_in addr{};
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	addr.sin_addr.s_addr = inet_addr(host.c_str());
	if (-1 == ::connect(fd, (sockaddr *)&addr, sizeof(addr)))
	{
		std::cerr << "connect fail\n";
		return 1;
	}
	std::string wl;
	recv_line(fd, wl);

	for (int i = from; i <= to; i++)
	{
		std::string u = base + std::to_string(i);
		if (!send_line(fd, std::string("REGISTER ") + u + " " + password))
		{
			std::cerr << "send fail\n";
			break;
		}
		std::string resp;
		if (!recv_line(fd, resp))
		{
			std::cerr << "recv fail\n";
			break;
		}
		std::cout << u << ": " << resp << "\n";
	}
	::close(fd);
	return 0;
}

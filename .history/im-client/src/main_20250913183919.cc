// main.cpp
#include "client.hpp"
#include "commandHandler.hpp"
#include <iostream>
#include <thread>
#include <atomic>
#include <cctype> // ★ 新增：用于输入清洗
#include <signal.h>
#include <csignal>

using namespace std;

// 全局客户端指针，用于信号处理
Client* g_client = nullptr;

// 信号处理函数
void signalHandler(int signal) {
    if (g_client) {
        std::cout << "\nReceived signal " << signal << ", logging out and exiting..." << std::endl;
        g_client->gracefulExit(0);
    }
}

int main(int argc, char **argv)
{
	if (argc < 3)
	{
		cerr << "Usage: " << argv[0] << " <host> <port>" << endl;
		return 1;
	}
	string host = argv[1];
	uint16_t port = atoi(argv[2]);

	cout << host << " " << port << endl;

	Client client(host, port);
	client.start();

	cout << "MPIM 客户端已启动：连接 " << host << ":" << port << "\n"
		 << "命令：login:<user>:<pwd> | chat:<uid>:<text> | pull | help | quit\n"
		 << "例如：login:alice:123456  或  chat:@24:hello\n";

	CommandHandler cmdHandler(&client);
	string command;

	auto sanitize = [](std::string &s)
	{
		std::string out;
		out.reserve(s.size());
		for (unsigned char ch : s)
		{
			if (ch == 0x08 || ch == 0x7F)
			{
				if (!out.empty())
					out.pop_back();
				continue;
			} // 退格/DEL
			if (std::iscntrl(ch) && ch != '\t')
				continue; // 过滤其他控制符
			out.push_back((char)ch);
		}
		s.swap(out);
	};

	while (true)
	{
		if (client.isLoginSuccess())
			std::cout << "[uid=" << client.getUid() << "] > ";
		else
			std::cout << "[未登录] > ";
		std::cout.flush();

		if (!std::getline(cin, command))
		{
			// EOF / 管道断开：优雅退出
			client.gracefulExit(0);
			break;
		}

		sanitize(command);
		if (command == "quit" || command == "exit")
		{
			client.gracefulExit(0);
			break;
		}

		cmdHandler.handleCommand(command);
	}

	return 0; // 正常到不了；gracefulExit 会 exit(0/1)
}

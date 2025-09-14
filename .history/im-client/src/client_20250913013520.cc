// client.cpp
#include "client.hpp"
#include <iostream>
#include <thread>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

Client::Client(const std::string &host, uint16_t port)
	: host(host), port(port), clientfd(-1)
{
}

Client::~Client()
{
	stop();
}

// 用于客户端与服务器（im-gateway模块）建立连接（实际上中间经过nginx）
// 两个线程分别处理：1，读取服务器消息（readTaskHandler） 2，发送消息（sendLoop）
void Client::connectToServer()
{
	// ===================== 封装套接字 =====================
	sockaddr_in serverAddr{};
	serverAddr.sin_family = AF_INET;
	serverAddr.sin_port = htons(port);
	serverAddr.sin_addr.s_addr = inet_addr(host.c_str());

	clientfd = socket(AF_INET, SOCK_STREAM, 0);
	if (clientfd < 0)
	{
		std::cerr << "Failed to create socket!" << std::endl;
		return;
	}

	// 忽略SIGPIPE信号，防止写已关闭的socket时程序崩溃
	signal(SIGPIPE, SIG_IGN); 

	// 进行连接
	if (connect(clientfd, (sockaddr *)&serverAddr, sizeof(serverAddr)) < 0)
	{
		std::cerr << "Failed to connect to server!" << std::endl;
		close(clientfd);
		return;
	}

	// 使用两个线程分别处理接收和发送任务，避免了单线程阻塞的情况，提高程序响应性
	// ==================== 启动接收线程 =====================
	recvThread_ = std::thread(&Client::readTaskHandler, this);

	running_.store(true);
	// =================== 启动发送线程 =====================
	sendThread_ = std::thread(&Client::sendLoop, this);

	connected_.store(true);
	closing_.store(false);
}

void Client::start()
{
	connectToServer();
}

/*
+ 先关闭队列， 发送线程不会再取新任务
+ 再shutdown 接收线程不会卡着
+ join recv, 避免在关闭fd之后还读
+ join send, 此时若还在 sendAll, 会因shutdown失败立即返回并退出
+ 最后close fd
*/
void Client::stop()
{
	closing_.store(true); // 标记为客户端主动关闭
	running_.store(false);	//线程关闭标志
	outbox_.Close(); // 关闭发送队列，通知发送线程退出
	if (clientfd != -1)
	{
		// 强制使阻塞在recv调用上的线程被唤醒并返回错误，从而让接收线程有机会检查循环条件
		shutdown(clientfd, SHUT_RDWR);	
	}

	// 避免read线程里自我join死锁
	const auto self = std::this_thread::get_id();
	if (recvThread_.joinable() && recvThread_.get_id() != self)
	{
		recvThread_.join(); // 等待读取线程退出，里面会检查running_
	}
	if (sendThread_.joinable() && sendThread_.get_id() != self)
	{
		sendThread_.join(); // 等待发送线程退出，里面会检查running_
	}
	if (clientfd != -1)
	{
		::close(clientfd);
		clientfd = -1;
	}
	if (connected_.exchange(false))
	{
		if (!exiting_.load()){
			std::lock_guard<std::mutex> lk(io_mtx_);
			std::cout << "\r\33[2K 已断开连接。\n";
			refreshPrompt();
		}
	}
}

void Client::login(const std::string &user, const std::string &pwd)
{
	enqueueRaw("LOGIN " + user + " " + pwd); // 发送登录命令,异步操作
}

bool Client::isLoginSuccess() const
{
	return loginSuccess.load();
}

void Client::sendMessage(const std::string &toUid, const std::string &msg)
{
	enqueueRaw("SEND " + toUid + " " + msg);
}

void Client::pullOfflineMessages()
{
	enqueueRaw("PULL");
}

void Client::logout()
{
	enqueueRaw("LOGOUT");
}

void Client::addFriend(const std::string &friendId)
{
	enqueueRaw("ADDFRIEND " + friendId);
}

void Client::getFriends()
{
	enqueueRaw("GETFRIENDS");
}

void Client::createGroup(const std::string &groupName, const std::string &description)
{
	enqueueRaw("CREATEGROUP " + groupName + " " + description);
}

void Client::joinGroup(const std::string &groupId)
{
	enqueueRaw("JOINGROUP " + groupId);
}

void Client::sendGroupMessage(const std::string &groupId, const std::string &message)
{
	enqueueRaw("SENDGROUP " + groupId + " " + message);
}

// 循环读取服务器消息
void Client::readTaskHandler()
{
	char buf[4096];
	while (running_)
	{
		int n = ::recv(clientfd, buf, sizeof(buf), 0);
		if (n > 0)
		{
			// 将接收到的数据添加到缓冲区
			inbuf_.append(buf, n);
			for (;;)
			{
				// 尝试从缓冲区提取完整行（拆包）
				auto pos = inbuf_.find('\n');
				if (pos == std::string::npos)
					break;
				std::string line = inbuf_.substr(0, pos);
				inbuf_.erase(0, pos + 1);
				if (!line.empty() && line.back() == '\r')
					line.pop_back();

				// === 根据网关回包分派 ===
				prettyPrintLine(line); // 美化输出
			}
		}
		else if (n == 0)
		{ // 如果recv返回0，表示服务器关闭连接
			running_.store(false);
			outbox_.Close();
			connected_.store(false);
			gracefulExit(1);	//统一走清理后退出
			break;
		}
		else if (errno == EINTR)
		{ // recv被信号中断(eintr),继续读取
			continue;
		}
		else
		{ // 其他错误，打印错误信息并退出循环
			running_.store(false);
			outbox_.Close();
			connected_.store(false);
			gracefulExit(1);
			break;
		}
	}
}

void Client::handleLoginResponse()
{
	loginSuccess.store(true);
}

void Client::handleChatResponse(const std::string &response)
{
	// 解析MSG消息格式: MSG id=1 from=5 ts=1234567890 text=hello
	if (response.find("MSG ") == 0) {
		std::istringstream iss(response.substr(4)); // 跳过"MSG "
		std::string token;
		int64_t msg_id = 0, from = 0, ts = 0;
		std::string text;
		
		while (iss >> token) {
			if (token.find("id=") == 0) {
				msg_id = std::stoll(token.substr(3));
			} else if (token.find("from=") == 0) {
				from = std::stoll(token.substr(5));
			} else if (token.find("ts=") == 0) {
				ts = std::stoll(token.substr(3));
			} else if (token.find("text=") == 0) {
				text = token.substr(5);
				// 读取剩余文本（可能包含空格）
				std::string remaining;
				while (iss >> remaining) {
					text += " " + remaining;
				}
			}
		}
		
		std::cout << "\r💬 收到来自用户" << from << "的消息: " << text << "\n";
		std::cout << "[uid=" << (loginSuccess.load() ? std::to_string(current_uid) : "?") << "] > ";
		std::cout.flush();
	} else {
		std::cout << "Received message: " << response << std::endl;
	}
}

void Client::handlePullResponse(const std::string &response)
{
	if (response == "+OK pull_done") {
		std::cout << "\r📦 离线消息拉取完毕。\n";
		std::cout << "[uid=" << (loginSuccess.load() ? std::to_string(current_uid) : "?") << "] > ";
		std::cout.flush();
	} else {
		// 处理离线消息（格式与在线消息相同）
		handleChatResponse(response);
	}
}

void Client::enqueueRaw(const std::string &line)
{
	std::string s = line;
	// 确保命令以换行结尾
	if (s.empty() || s.back() != '\n')
		s.push_back('\n');
	// 放入发送队列
	outbox_.Push(std::move(s));
}

// 该方法通常用于需要可靠发送数据的场景，确保数据不会因为网络波动或系统中断而丢失。
// 常见于客户端与服务器之间的通信模块。
bool Client::sendAll(const char *data, size_t len)
{
	size_t off = 0; // off记录已经成功发送的数据字节数
	while (off < len)
	{															// 当前发送的字节数小于总长度时，继续发送
		ssize_t n = ::send(clientfd, data + off, len - off, 0); // 第一次发送的时候尝试全部发送
		if (n > 0)
			off += (size_t)n; // n>0 表示成功发送，更新已发送字节数
		else if (n < 0 && errno == EINTR)
			continue; // n<0 且错误码EINTR，表示系统调用被信号中断，直接继续循环尝试发送
		else if (n < 0 && (errno == EAGAIN || errno == EWOULDBLOCK))
			continue; // 表示发送缓冲区暂时不可用（非阻塞模式下可能出现）
		else
			return false;
	}
	return true;
}

// 发送线程主循环，从队列中取出消息并发送
void Client::sendLoop()
{
	std::string frame;
	while (running_)
	{
		if (!outbox_.Pop(frame)){	 // 取消息失败，可能是队列关闭了
			// std::cout << "outbox_.Pop failed, exit sendLoop\n";
			break;
		}
		if (!sendAll(frame.data(), frame.size())) // 发送失败 -> 退出
		{
			// std::cerr << "sendAll failed, exit sendLoop\n";
			break;
		}
	}
}

void Client::prettyPrintLine(const std::string &line)
{
	std::lock_guard<std::mutex> lk(io_mtx_); // 串行化输出
	std::cout << "\r\33[2K"; // 清除当前行

	if (line.rfind("+OK welcome", 0) == 0)
	{
		std::cout << "\r已连接网关。可用命令：login:<user>:<pwd> | chat:<uid>:<text> | register:<user>:<pwd> | pull | help | quit\n";
	}
	else if (line.rfind("+OK uid=", 0) == 0)
	{
		try
		{
			uint32_t new_uid = static_cast<uint32_t>(std::stoul(line.substr(8)));
			uid_.store(new_uid);
			last_uid_.store(new_uid);
		}
		catch (...)
		{
			uid_.store(0);
		}
		handleLoginResponse();
		std::cout << "\r✅ login success，uid=" << uid_ << "。现在可以 chat:@<uid>:<text> 或 pull。\n";
	}
	else if (line.rfind("+OK msg_id=", 0) == 0)
	{
		std::cout << "\r📨 已发送（" << line.substr(4) << "），等待送达…\n";
	}
	else if (line.rfind("MSG ", 0) == 0 || line.rfind("MSG", 0) == 0)
	{
		handleChatResponse(line);
	}
	else if (line == "+OK pull_done")
	{
		handlePullResponse(line);
		std::cout << "\r📦 离线消息拉取完毕。\n";
	}
	else if (line.rfind("-ERR", 0) == 0)
	{
		std::cout << "\r\033[31m" << line << "\033[0m\n"; // 红色错误
	}
	else
	{
		std::cout << "\r" << line << "\n";
	}
	// 打印完消息后，重绘提示符
	refreshPrompt();
}

uint32_t Client::getUid() const
{
	return uid_.load();
}

std::string Client::prompt() const {
    if (isLoginSuccess()) {
        return "[uid=" + std::to_string(getUid()) + "] > ";
    }
	// 未登录但已连接
    if (connected_.load()) {
        return "[已连接] > ";
    }
	// 已断开：若曾经登录过，则显示上次登录的uid
	uint32_t lu = last_uid_.load();
	if (lu != 0) {
		return "[断开, 上次登录 uid=" + std::to_string(lu) + "] > ";
	}
    return "[未连接] > ";
}

void Client::refreshPrompt() {
    // 注意：调用处通常已持有 io_mtx_
    std::cout << prompt() << std::flush;
}

void Client::onDisconnected(const std::string& reason) {
    // 重置登录态
    loginSuccess.store(false);
    uid_.store(0);
	connected_.store(false);

    // 串行化输出：清行 -> 打印原因 -> 重绘提示符
    std::lock_guard<std::mutex> lk(io_mtx_);
    std::cerr << "\r\33[2K" << reason << "\n" << std::flush;

    refreshPrompt();
}

void Client::gracefulExit(int code) {
    bool expected = false;
    if (!exiting_.compare_exchange_strong(expected, true)) {
        return;                           // 已在退出流程中
    }

    // 给出断开提示
    onDisconnected("❗ 连接已断开，正在退出…");

    // ★ 关键：先按既定顺序清理
    stop();                               // stop 内部会跳过自我 join，安全

    // ★ 再正常退出（会执行静态析构/atexit；资源已手工释放，所以不会“泄露”）
    std::exit(code);
}

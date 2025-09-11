// Client.hpp
#pragma once
#include <string>
#include <thread>
#include <atomic>
#include <semaphore.h>
#include "lockqueue.hpp"
#include <mutex>

class Client {
public:
    Client(const std::string& host, uint16_t port);
    ~Client();

    void start();
    void stop();

    bool isLoginSuccess() const;
    void login(const std::string& user, const std::string& pwd);
    void sendMessage(const std::string& toUid, const std::string& msg);
    void pullOfflineMessages();

	// 把待发送的命令放入发送队列
	void enqueueRaw(const std::string& line);

	// 获取当前用户ID(未登录返回0)
	uint32_t getUid() const;

	void gracefulExit(int code); 	//优雅退出程序
    
private:
    int clientfd;
    std::string host;
    uint16_t port;
    std::atomic_bool loginSuccess{false};

	// 发送线程相关
	std::thread sendThread_;
	LockQueue<std::string> outbox_;	 //存储将要发送的消息
	std::atomic_bool running_{false};

	// 接收线程
	std::thread recvThread_;

	// 行缓冲与登录uid
	std::string inbuf_;
	// 为了避免资源竞争而导致的未知问题
	std::atomic<uint32_t> uid_{0};
	// 上次登录成功的uid
	std::atomic<uint32_t> last_uid_{0};

	// 维护连接与退出状态
	std::atomic_bool connected_{false};	//是否已建立tcp连接
	std::atomic_bool closing_{false};	//是否客户端主动在关闭
	std::atomic_bool exiting_{false};	//是否进程正在退出

	// 串行化控制台输出 + 提示工具符
	std::mutex io_mtx_;
	std::string prompt() const;	//根据登录态生成提示符
	void refreshPrompt();	//重绘提示符
	void onDisconnected(const std::string& reason);	//统一断开提示

	// 发送完整缓冲区，处理部分写/EINTR
	bool sendAll(const char* data, size_t len);
	void sendLoop();

	void prettyPrintLine(const std::string& line);

    void connectToServer();
    void readTaskHandler();
    void handleLoginResponse();
    void handleChatResponse(const std::string& response);
    void handlePullResponse(const std::string& response);
};

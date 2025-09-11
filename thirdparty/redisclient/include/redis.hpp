#pragma once

#include <hiredis/hiredis.h>
#include <thread>
#include <functional>
#include <mutex>
#include <string>
using namespace std;

/*
两个bug博客地址
https://blog.csdn.net/QIANGWEIYUAN/article/details/97895611
*/
class Redis
{
public:
	Redis();
	~Redis();

	// 连接redis服务器
	bool connect();

	// 向redis指定的通道channel发布消息
	bool publish(int channel, string message);
	
	// 向redis指定的通道subscribe订阅消息
	bool subscribe(int channel);
	
	// 向redis指定的通道unsubscribe取消订阅消息
	bool unsubscribe(int channel);

	// 在独立线程中接收订阅通道中的消息
	void observer_channel_message();

	// 初始化向业务层上报通道消息的回调函数
	void init_notify_handler(function<void(int, string)> fn);

	// === KV ===
	// SETEX key ttl value => true/false
	bool setex(const std::string& key, int ttl_sec, const std::string& value);
	// GET key => 若不存在返回空串；可用found判断是否真的存在
	std::string get(const std::string& key, bool* found = nullptr);
private:
	// hiredis 同步上下文对象，负责publish消息
	redisContext *_publish_context;
	// hiredis 同步上下文对象，负责subscribe消息
	redisContext *_subscribe_context;
	// 回调操作，收到订阅的消息，给service层上报
	function<void(int, string)> _notify_message_handler;

	// 保护 _publish_context, 避免并发命令交叉
	std::mutex _cmd_mu;
};
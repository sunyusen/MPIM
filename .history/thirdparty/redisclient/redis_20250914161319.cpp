#include "redis.hpp"
#include <iostream>
#include <cstring>
using namespace std;

Redis::Redis()
	: _publish_context(nullptr), _subscribe_context(nullptr)
{
}

Redis::~Redis()
{
	if (_publish_context != nullptr)
	{
		redisFree(_publish_context);
	}
	if (_subscribe_context != nullptr)
	{
		redisFree(_subscribe_context);
	}
}

bool Redis::connect()
{
	// 负责publich发布消息的上下文连接
	_publish_context = redisConnect("127.0.0.1", 6379);
	if (nullptr == _publish_context)
	{
		cerr << "connect redis failed!" << endl;
		return false;
	}

	// 负责subscribe订阅消息的上下文连接
	_subscribe_context = redisConnect("127.0.0.1", 6379);
	if (nullptr == _subscribe_context)
	{
		cerr << "connect redis failed!" << endl;
		return false;
	}

	//在单独的线程中，监听通道上的事件，有消息给业务层进行上报
	thread t([&](){
		observer_channel_message();
	});
	t.detach(); // 分离线程，独立运行

	cout << "connect redis-server success!" << endl;

	return true;
}

// 向redis指定的通道channel发布消息
bool Redis::publish(int channel, string message)
{
	std::lock_guard<std::mutex> lock(_cmd_mu);
	redisReply *reply = (redisReply *)redisCommand(
		_publish_context, "PUBLISH %d %s", channel,
		message.data(), (size_t)message.size());
	if (nullptr == reply)
	{
		cerr << "publish message error!" << endl;
		return false;
	}
	freeReplyObject(reply);
	return true;
}

//向redis指定的通道subscribe订阅消息
bool Redis::subscribe(int channel)
{
	cout << "subscribe channel:" << channel << endl;
	// subscribe命令本身会造成线程阻塞等待通道里面发生消息，这里只做订阅通道，不接收通道消息
	// 通道消息的接收专门在observer_channel_message函数中的独立线程中进行
	// 只负责发送命令，不阻塞接收redis server响应消息，否则和notifMsg线程抢占响应资源
	if (REDIS_ERR == redisAppendCommand(this->_subscribe_context, "SUBSCRIBE %d", channel))
	{
		cerr << "subscribe channel error!" << endl;
		return false;
	}
	// redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕
	int done = 0;
	while (!done)
	{
		if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done))
		{
			cerr << "subscribe channel error!" << endl;
			return false;
		}
	}
	//redisGetReply

	return true;
}

// 向redis指定的通道unsubscribe取消订阅消息
bool Redis::unsubscribe(int channel){
	if(REDIS_ERR == redisAppendCommand(this->_subscribe_context, "UNSUBSCRIBE %d", channel)){
		cerr << "unsubscribe command failed!" << endl;
		return false;
	}

	//redisBufferWrite可以循环发送缓冲区，直到缓冲区数据发送完毕(done被置为1)
	int done = 0;
	while(!done){
		if (REDIS_ERR == redisBufferWrite(this->_subscribe_context, &done)){
			cerr << "unsubscribe channel error!" << endl;
			return false;
		}
	}

	return true;
}

// 在独立线程中接收订阅通道中的消息
void Redis::observer_channel_message()
{
	redisReply *reply = nullptr;

	// 打印线程 ID 和 Redis 对象地址
    cerr << "[observer_channel_message] thread start. this = " << this 
         << ", _subscribe_context = " << _subscribe_context << endl;

	while (REDIS_OK == redisGetReply(this->_subscribe_context, (void **)&reply))
	{
		cerr << "[observer_channel_message] redisGetReply returned. reply = " << reply << endl;

		// 检查 reply 是否有效
        if (reply == nullptr)
        {
            cerr << "[observer_channel_message] Received null reply." << endl;
            continue;
        }

        // 打印 reply 的类型
        cerr << "[observer_channel_message] Reply type: " << reply->type << endl;

        // 如果是数组类型，打印元素数量
        if (reply->type == REDIS_REPLY_ARRAY)
        {
            cerr << "[observer_channel_message] Reply elements: " << reply->elements << endl;

            // 遍历数组元素并打印
            for (size_t i = 0; i < reply->elements; ++i)
            {
                if (reply->element[i] && reply->element[i]->str)
                {
                    cerr << "[observer_channel_message] Element " << i << ": " << reply->element[i]->str << endl;
                }
                else
                {
                    cerr << "[observer_channel_message] Element " << i << ": (null or non-string)" << endl;
                }
            }
        }
        else
        {
            cerr << "[observer_channel_message] Non-array reply received. Skipping." << endl;
        }

		if(reply && reply->type == REDIS_REPLY_ARRAY && reply->elements >= 3 &&
        reply->element[0] && strcmp(reply->element[0]->str, "message") == 0 &&
        reply->element[1] && reply->element[2])
		{
			_notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
		}
		else{
			cerr << "[observer_channel_message] Control reply or malformed: skip." << endl;
		}

// 为什么会收到非三元数的数字呢
		// // 订阅收到的消息是一个带三元素的数组
		// if (reply != nullptr &&  reply->element[2] != nullptr && reply->element[2]->str != nullptr)
		// {
		// 	//给业务层上报通道上发生的消息
		// 	_notify_message_handler(atoi(reply->element[1]->str), reply->element[2]->str);
		// }

		if(reply) freeReplyObject(reply);
	}

	cerr << "observer_channel_message quit!" << endl;
}

void Redis::init_notify_handler(function<void(int, string)> fn)
{
	// this->_notify_message_handler = fn;
	_notify_message_handler = std::move(fn);
}

// === KV ===
bool Redis::setex(const std::string& key, int ttl_sec, const std::string& value) {
  std::lock_guard<std::mutex> lk(_cmd_mu);
  redisReply* reply = (redisReply*)redisCommand(
      _publish_context, "SETEX %s %d %s",
      key.c_str(),
      ttl_sec,
      value.c_str());
  if (!reply) { 
    std::cerr << "SETEX failed - reply is NULL\n"; 
    return false; 
  }
  
  // 对于hiredis 0.12.1，STATUS类型的回复可能没有str字段
  // 我们通过检查命令是否成功执行来判断
  bool ok = (reply->type == REDIS_REPLY_STATUS);
  
  if (!ok) {
    std::cerr << "SETEX failed - unexpected reply type: " << reply->type << std::endl;
  }
  
  freeReplyObject(reply);
  return ok;
}

std::string Redis::get(const std::string& key, bool* found) {
  std::lock_guard<std::mutex> lk(_cmd_mu);
  if (found) *found = false;
  redisReply* reply = (redisReply*)redisCommand(
      _publish_context, "GET %s", key.c_str());
  if (!reply) { 
    std::cerr << "GET failed - reply is NULL\n"; 
    return {}; 
  }

  std::string out;
  if (reply->type == REDIS_REPLY_STRING) {
    out.assign(reply->str, (size_t)reply->len);
    if (found) *found = true;
  } else if (reply->type == REDIS_REPLY_NIL) {
    // key 不存在
  } else {
    std::cerr << "GET unexpected reply type: " << reply->type << "\n";
  }
  freeReplyObject(reply);
  return out;
}
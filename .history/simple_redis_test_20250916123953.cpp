#include <iostream>
#include <memory>
#include "thirdparty/redisclient/include/redis_client.h"
#include "thirdparty/redisclient/include/cache_manager.h"
#include "thirdparty/redisclient/include/message_queue.h"

using namespace mpim::redis;

int main() {
    std::cout << "=== Redis 功能测试 ===" << std::endl;
    
    // 测试 1: RedisClient 连接
    std::cout << "\n--- 测试 RedisClient 连接 ---" << std::endl;
    RedisClient& client = RedisClient::GetInstance();
    if (client.Connect("127.0.0.1", 6379)) {
        std::cout << "✓ RedisClient 连接成功" << std::endl;
        
        // 测试基本命令
        auto reply = client.ExecuteCommand("SET test_key test_value");
        if (reply) {
            std::cout << "✓ SET 命令执行成功" << std::endl;
            freeReplyObject(reply);
        }
        
        reply = client.ExecuteCommand("GET test_key");
        if (reply && reply->type == REDIS_REPLY_STRING) {
            std::cout << "✓ GET 命令执行成功: " << reply->str << std::endl;
            freeReplyObject(reply);
        }
        
        // 清理测试数据
        client.ExecuteCommand("DEL test_key");
    } else {
        std::cout << "✗ RedisClient 连接失败" << std::endl;
        return 1;
    }
    
    // 测试 2: CacheManager 功能
    std::cout << "\n--- 测试 CacheManager 功能 ---" << std::endl;
    CacheManager cache_manager;
    if (cache_manager.Connect("127.0.0.1", 6379)) {
        std::cout << "✓ CacheManager 连接成功" << std::endl;
        
        // 测试字符串操作
        if (cache_manager.Setex("cache_test", 60, "cache_value")) {
            std::cout << "✓ SETEX 操作成功" << std::endl;
        }
        
        bool found = false;
        std::string value = cache_manager.Get("cache_test", &found);
        if (found) {
            std::cout << "✓ GET 操作成功: " << value << std::endl;
        }
        
        // 测试哈希操作
        if (cache_manager.Hset("user:123", "name", "test_user")) {
            std::cout << "✓ HSET 操作成功" << std::endl;
        }
        
        std::string name = cache_manager.Hget("user:123", "name", &found);
        if (found) {
            std::cout << "✓ HGET 操作成功: " << name << std::endl;
        }
        
        // 测试集合操作
        if (cache_manager.Sadd("friends:123", "456")) {
            std::cout << "✓ SADD 操作成功" << std::endl;
        }
        
        if (cache_manager.Sismember("friends:123", "456")) {
            std::cout << "✓ SISMEMBER 操作成功" << std::endl;
        }
        
        // 清理测试数据
        cache_manager.Del("cache_test");
        cache_manager.Del("user:123");
        cache_manager.Del("friends:123");
    } else {
        std::cout << "✗ CacheManager 连接失败" << std::endl;
    }
    
    // 测试 3: MessageQueue 功能
    std::cout << "\n--- 测试 MessageQueue 功能 ---" << std::endl;
    MessageQueue message_queue;
    if (message_queue.Connect("127.0.0.1", 6379)) {
        std::cout << "✓ MessageQueue 连接成功" << std::endl;
        
        // 测试发布消息
        if (message_queue.Publish(1, "test_message")) {
            std::cout << "✓ PUBLISH 操作成功" << std::endl;
        }
        
        // 测试订阅（这里只是测试订阅命令，不实际接收消息）
        if (message_queue.Subscribe(1)) {
            std::cout << "✓ SUBSCRIBE 操作成功" << std::endl;
        }
        
        message_queue.Disconnect();
    } else {
        std::cout << "✗ MessageQueue 连接失败" << std::endl;
    }
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    
    return 0;
}

#include <iostream>
#include <memory>
#include "thirdparty/redisclient/include/redis_client.h"
#include "thirdparty/redisclient/include/cache_manager.h"
#include "thirdparty/redisclient/include/message_queue.h"

using namespace mpim::redis;

int main() {
    std::cout << "=== Redis 功能直接测试 ===" << std::endl;
    
    // 测试 RedisClient
    std::cout << "\n1. 测试 RedisClient 连接..." << std::endl;
    RedisClient& client = RedisClient::GetInstance();
    if (client.Connect()) {
        std::cout << "✓ RedisClient 连接成功" << std::endl;
        
        // 测试基本命令
        redisReply* reply = client.ExecuteCommand("SET test_key test_value");
        if (reply) {
            std::cout << "✓ SET 命令执行成功" << std::endl;
            freeReplyObject(reply);
        }
        
        reply = client.ExecuteCommand("GET test_key");
        if (reply && reply->type == REDIS_REPLY_STRING) {
            std::cout << "✓ GET 命令执行成功: " << reply->str << std::endl;
            freeReplyObject(reply);
        }
        
        // 测试删除
        reply = client.ExecuteCommand("DEL test_key");
        if (reply) {
            std::cout << "✓ DEL 命令执行成功" << std::endl;
            freeReplyObject(reply);
        }
    } else {
        std::cout << "✗ RedisClient 连接失败" << std::endl;
        return 1;
    }
    
    // 测试 CacheManager
    std::cout << "\n2. 测试 CacheManager..." << std::endl;
    CacheManager cache_manager;
    if (cache_manager.Connect()) {
        std::cout << "✓ CacheManager 连接成功" << std::endl;
        
        // 测试字符串操作
        if (cache_manager.Setex("cache_test", 60, "cache_value")) {
            std::cout << "✓ SETEX 命令执行成功" << std::endl;
            
            bool found = false;
            std::string value = cache_manager.Get("cache_test", &found);
            if (found && value == "cache_value") {
                std::cout << "✓ GET 命令执行成功: " << value << std::endl;
            } else {
                std::cout << "✗ GET 命令执行失败" << std::endl;
            }
        } else {
            std::cout << "✗ SETEX 命令执行失败" << std::endl;
        }
        
        // 测试哈希操作
        if (cache_manager.Hset("user:123", "name", "test_user")) {
            std::cout << "✓ HSET 命令执行成功" << std::endl;
            
            std::string name = cache_manager.Hget("user:123", "name");
            if (!name.empty() && name == "test_user") {
                std::cout << "✓ HGET 命令执行成功: " << name << std::endl;
            } else {
                std::cout << "✗ HGET 命令执行失败" << std::endl;
            }
        } else {
            std::cout << "✗ HSET 命令执行失败" << std::endl;
        }
        
        // 测试集合操作
        if (cache_manager.Sadd("friends:123", "456")) {
            std::cout << "✓ SADD 命令执行成功" << std::endl;
            
            if (cache_manager.Sismember("friends:123", "456")) {
                std::cout << "✓ SISMEMBER 命令执行成功" << std::endl;
            } else {
                std::cout << "✗ SISMEMBER 命令执行失败" << std::endl;
            }
        } else {
            std::cout << "✗ SADD 命令执行失败" << std::endl;
        }
    } else {
        std::cout << "✗ CacheManager 连接失败" << std::endl;
    }
    
    // 测试 MessageQueue
    std::cout << "\n3. 测试 MessageQueue..." << std::endl;
    MessageQueue message_queue;
    if (message_queue.Connect()) {
        std::cout << "✓ MessageQueue 连接成功" << std::endl;
        
        // 测试发布消息
        if (message_queue.Publish(1, "test_message")) {
            std::cout << "✓ PUBLISH 命令执行成功" << std::endl;
        } else {
            std::cout << "✗ PUBLISH 命令执行失败" << std::endl;
        }
        
        // 测试订阅
        if (message_queue.Subscribe(1)) {
            std::cout << "✓ SUBSCRIBE 命令执行成功" << std::endl;
        } else {
            std::cout << "✗ SUBSCRIBE 命令执行失败" << std::endl;
        }
    } else {
        std::cout << "✗ MessageQueue 连接失败" << std::endl;
    }
    
    std::cout << "\n=== Redis 功能测试完成 ===" << std::endl;
    return 0;
}

#include <iostream>
#include <memory>
#include "thirdparty/redisclient/include/redis_client.h"
#include "thirdparty/redisclient/include/cache_manager.h"
#include "thirdparty/redisclient/include/message_queue.h"
#include "im-common/include/cache/user_cache.h"

using namespace mpim::redis;
using namespace mpim::cache;

int main() {
    std::cout << "=== 测试我们编写的 Redis 代码 ===" << std::endl;
    
    // 测试 RedisClient
    std::cout << "\n1. 测试 RedisClient 类..." << std::endl;
    RedisClient& client = RedisClient::GetInstance();
    if (client.Connect()) {
        std::cout << "✓ RedisClient 连接成功" << std::endl;
        
        // 测试基本命令
        redisReply* reply = client.ExecuteCommand("SET test_key test_value");
        if (reply) {
            std::cout << "✓ RedisClient SET 命令执行成功" << std::endl;
            freeReplyObject(reply);
        }
        
        reply = client.ExecuteCommand("GET test_key");
        if (reply && reply->type == REDIS_REPLY_STRING) {
            std::cout << "✓ RedisClient GET 命令执行成功: " << reply->str << std::endl;
            freeReplyObject(reply);
        }
        
        // 测试删除
        reply = client.ExecuteCommand("DEL test_key");
        if (reply) {
            std::cout << "✓ RedisClient DEL 命令执行成功" << std::endl;
            freeReplyObject(reply);
        }
    } else {
        std::cout << "✗ RedisClient 连接失败" << std::endl;
        return 1;
    }
    
    // 测试 CacheManager
    std::cout << "\n2. 测试 CacheManager 类..." << std::endl;
    CacheManager cache_manager;
    if (cache_manager.Connect()) {
        std::cout << "✓ CacheManager 连接成功" << std::endl;
        
        // 测试字符串操作
        if (cache_manager.Setex("cache_test", 60, "cache_value")) {
            std::cout << "✓ CacheManager SETEX 命令执行成功" << std::endl;
            
            std::string value = cache_manager.Get("cache_test");
            if (!value.empty() && value == "cache_value") {
                std::cout << "✓ CacheManager GET 命令执行成功: " << value << std::endl;
            } else {
                std::cout << "✗ CacheManager GET 命令执行失败" << std::endl;
            }
        } else {
            std::cout << "✗ CacheManager SETEX 命令执行失败" << std::endl;
        }
        
        // 测试哈希操作
        if (cache_manager.Hset("user:123", "name", "test_user")) {
            std::cout << "✓ CacheManager HSET 命令执行成功" << std::endl;
            
            std::string name = cache_manager.Hget("user:123", "name");
            if (!name.empty() && name == "test_user") {
                std::cout << "✓ CacheManager HGET 命令执行成功: " << name << std::endl;
            } else {
                std::cout << "✗ CacheManager HGET 命令执行失败" << std::endl;
            }
        } else {
            std::cout << "✗ CacheManager HSET 命令执行失败" << std::endl;
        }
        
        // 测试集合操作
        if (cache_manager.Sadd("friends:123", "456")) {
            std::cout << "✓ CacheManager SADD 命令执行成功" << std::endl;
            
            if (cache_manager.Sismember("friends:123", "456")) {
                std::cout << "✓ CacheManager SISMEMBER 命令执行成功" << std::endl;
            } else {
                std::cout << "✗ CacheManager SISMEMBER 命令执行失败" << std::endl;
            }
        } else {
            std::cout << "✗ CacheManager SADD 命令执行失败" << std::endl;
        }
    } else {
        std::cout << "✗ CacheManager 连接失败" << std::endl;
    }
    
    // 测试 MessageQueue
    std::cout << "\n3. 测试 MessageQueue 类..." << std::endl;
    MessageQueue message_queue;
    if (message_queue.Connect()) {
        std::cout << "✓ MessageQueue 连接成功" << std::endl;
        
        // 测试发布消息
        if (message_queue.Publish(1, "test_message")) {
            std::cout << "✓ MessageQueue PUBLISH 命令执行成功" << std::endl;
        } else {
            std::cout << "✗ MessageQueue PUBLISH 命令执行失败" << std::endl;
        }
        
        // 测试订阅
        if (message_queue.Subscribe(1)) {
            std::cout << "✓ MessageQueue SUBSCRIBE 命令执行成功" << std::endl;
        } else {
            std::cout << "✗ MessageQueue SUBSCRIBE 命令执行失败" << std::endl;
        }
    } else {
        std::cout << "✗ MessageQueue 连接失败" << std::endl;
    }
    
    // 测试 UserCache
    std::cout << "\n4. 测试 UserCache 类..." << std::endl;
    UserCache user_cache;
    if (user_cache.Connect()) {
        std::cout << "✓ UserCache 连接成功" << std::endl;
        
        // 测试登录信息缓存
        mpim::LoginResp login_resp;
        login_resp.set_user_id(12345);
        login_resp.set_token("test_token_12345");
        login_resp.mutable_result()->set_code(mpim::Code::Ok);
        login_resp.mutable_result()->set_msg("Login successful");
        
        if (user_cache.SetLoginInfo(12345, login_resp, 60)) {
            std::cout << "✓ UserCache 登录信息缓存设置成功" << std::endl;
            
            auto cached_login = user_cache.GetLoginInfo(12345);
            if (cached_login && cached_login->user_id() == 12345) {
                std::cout << "✓ UserCache 登录信息缓存读取成功: user_id=" << cached_login->user_id() 
                          << ", token=" << cached_login->token() << std::endl;
            } else {
                std::cout << "✗ UserCache 登录信息缓存读取失败" << std::endl;
            }
        } else {
            std::cout << "✗ UserCache 登录信息缓存设置失败" << std::endl;
        }
        
        // 测试好友关系缓存
        mpim::GetFriendsResp friends_resp;
        friends_resp.mutable_result()->set_code(mpim::Code::Ok);
        friends_resp.add_friend_ids(1001);
        friends_resp.add_friend_ids(1002);
        friends_resp.add_friend_ids(1003);
        
        if (user_cache.SetFriends(12345, friends_resp, 60)) {
            std::cout << "✓ UserCache 好友关系缓存设置成功" << std::endl;
            
            auto cached_friends = user_cache.GetFriends(12345);
            if (cached_friends && cached_friends->friend_ids_size() == 3) {
                std::cout << "✓ UserCache 好友关系缓存读取成功: 好友数量=" << cached_friends->friend_ids_size() << std::endl;
            } else {
                std::cout << "✗ UserCache 好友关系缓存读取失败" << std::endl;
            }
        } else {
            std::cout << "✗ UserCache 好友关系缓存设置失败" << std::endl;
        }
        
        // 测试群组信息缓存
        mpim::GroupInfo group_info;
        group_info.set_group_id(2001);
        group_info.set_group_name("测试群组");
        group_info.set_description("这是一个测试群组");
        group_info.set_creator_id(12345);
        group_info.set_created_at(1640995200);
        
        if (user_cache.SetGroupInfo(2001, group_info, 60)) {
            std::cout << "✓ UserCache 群组信息缓存设置成功" << std::endl;
            
            auto cached_group = user_cache.GetGroupInfo(2001);
            if (cached_group && cached_group->group_id() == 2001) {
                std::cout << "✓ UserCache 群组信息缓存读取成功: group_id=" << cached_group->group_id() 
                          << ", name=" << cached_group->group_name() << std::endl;
            } else {
                std::cout << "✗ UserCache 群组信息缓存读取失败" << std::endl;
            }
        } else {
            std::cout << "✗ UserCache 群组信息缓存设置失败" << std::endl;
        }
    } else {
        std::cout << "✗ UserCache 连接失败" << std::endl;
    }
    
    std::cout << "\n=== Redis 代码测试完成 ===" << std::endl;
    return 0;
}

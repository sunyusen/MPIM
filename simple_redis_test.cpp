#include <iostream>
#include <string>
#include <chrono>
#include <thread>

// 包含我们的缓存类
#include "im-user/include/user_cache.h"
#include "thirdparty/redisclient/include/cache_manager.h"
#include "thirdparty/redisclient/include/message_queue.h"

using namespace mpim::user;
using namespace mpim::redis;

int main() {
    std::cout << "=== Simple Redis Cache Test ===" << std::endl;
    
    // 测试UserCache
    std::cout << "\n--- Testing UserCache ---" << std::endl;
    UserCache userCache;
    
    // 测试连接
    std::cout << "1. Testing connection..." << std::endl;
    bool connected = userCache.Connect();
    std::cout << "   Connection: " << (connected ? "SUCCESS" : "FAILED") << std::endl;
    
    if (!connected) {
        std::cout << "   Redis not available, testing basic functionality..." << std::endl;
        
        // 测试基本方法调用（即使Redis不可用）
        std::string username = "testuser";
        std::string userData = userCache.GetUserInfo(username);
        std::cout << "   GetUserInfo (no Redis): " << (userData.empty() ? "EXPECTED" : "UNEXPECTED") << std::endl;
        
        bool setResult = userCache.SetUserInfo(username, "123:testuser", 60);
        std::cout << "   SetUserInfo (no Redis): " << (setResult ? "UNEXPECTED" : "EXPECTED") << std::endl;
        
        std::cout << "   UserCache basic functionality: OK" << std::endl;
    } else {
        // Redis可用，进行完整测试
        std::cout << "2. Testing user info cache..." << std::endl;
        std::string username = "testuser";
        std::string userData = "123:testuser";
        
        // 设置用户信息
        bool setResult = userCache.SetUserInfo(username, userData, 60);
        std::cout << "   Set user info: " << (setResult ? "SUCCESS" : "FAILED") << std::endl;
        
        // 获取用户信息
        std::string retrieved = userCache.GetUserInfo(username);
        std::cout << "   Get user info: " << (retrieved == userData ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "   Retrieved data: " << retrieved << std::endl;
        
        // 测试好友关系缓存
        std::cout << "3. Testing friends cache..." << std::endl;
        int64_t userId = 123;
        std::string friendsData = "456,789,101";
        
        // 设置好友列表
        bool setFriends = userCache.SetFriends(userId, friendsData, 60);
        std::cout << "   Set friends: " << (setFriends ? "SUCCESS" : "FAILED") << std::endl;
        
        // 获取好友列表
        std::string retrievedFriends = userCache.GetFriends(userId);
        std::cout << "   Get friends: " << (retrievedFriends == friendsData ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "   Retrieved friends: " << retrievedFriends << std::endl;
        
        // 测试添加好友（先清空之前的好友列表，使用Set操作）
        std::cout << "4. Testing add friend..." << std::endl;
        userCache.DelFriends(userId); // 清空之前的好友列表
        bool addFriend1 = userCache.AddFriend(userId, 999);
        bool addFriend2 = userCache.AddFriend(userId, 888);
        std::cout << "   Add friend 999: " << (addFriend1 ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "   Add friend 888: " << (addFriend2 ? "SUCCESS" : "FAILED") << std::endl;
        
        // 测试移除好友
        std::cout << "5. Testing remove friend..." << std::endl;
        bool removeFriend = userCache.RemoveFriend(userId, 999);
        std::cout << "   Remove friend 999: " << (removeFriend ? "SUCCESS" : "FAILED") << std::endl;
        
        // 测试用户状态缓存
        std::cout << "6. Testing user status cache..." << std::endl;
        bool setStatus = userCache.SetUserStatus(userId, "online", 60);
        std::cout << "   Set user status: " << (setStatus ? "SUCCESS" : "FAILED") << std::endl;
        
        std::string status = userCache.GetUserStatus(userId);
        std::cout << "   Get user status: " << (status == "online" ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "   Status: " << status << std::endl;
        
        // 测试用户名存在性缓存
        std::cout << "7. Testing username exists cache..." << std::endl;
        bool setExists = userCache.SetUsernameExists(username, true, 60);
        std::cout << "   Set username exists: " << (setExists ? "SUCCESS" : "FAILED") << std::endl;
        
        bool exists = userCache.IsUsernameExists(username);
        std::cout << "   Check username exists: " << (exists ? "SUCCESS" : "FAILED") << std::endl;
        
        // 清理测试数据
        userCache.DelUserInfo(username);
        userCache.DelFriends(userId);
        userCache.DelUserStatus(userId);
        
        std::cout << "   UserCache tests completed" << std::endl;
    }
    
    // 测试CacheManager
    std::cout << "\n--- Testing CacheManager ---" << std::endl;
    CacheManager cacheManager;
    
    std::cout << "1. Testing connection..." << std::endl;
    bool cacheConnected = cacheManager.Connect();
    std::cout << "   Connection: " << (cacheConnected ? "SUCCESS" : "FAILED") << std::endl;
    
    if (cacheConnected) {
        // 测试字符串操作
        std::cout << "2. Testing string operations..." << std::endl;
        std::string key = "test:string";
        std::string value = "test_value_123";
        
        bool setResult = cacheManager.Setex(key, 60, value);
        std::cout << "   SETEX: " << (setResult ? "SUCCESS" : "FAILED") << std::endl;
        
        std::string getValue = cacheManager.Get(key);
        std::cout << "   GET: " << (getValue == value ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "   Retrieved value: " << getValue << std::endl;
        
        // 测试哈希操作
        std::cout << "3. Testing hash operations..." << std::endl;
        std::string hashKey = "test:hash";
        std::string field = "field1";
        std::string hashValue = "hash_value_123";
        
        bool hsetResult = cacheManager.Hset(hashKey, field, hashValue);
        std::cout << "   HSET: " << (hsetResult ? "SUCCESS" : "FAILED") << std::endl;
        
        std::string hgetValue = cacheManager.Hget(hashKey, field);
        std::cout << "   HGET: " << (hgetValue == hashValue ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "   Retrieved hash value: " << hgetValue << std::endl;
        
        // 测试集合操作
        std::cout << "4. Testing set operations..." << std::endl;
        std::string setKey = "test:set";
        std::string member = "member1";
        
        bool saddResult = cacheManager.Sadd(setKey, member);
        std::cout << "   SADD: " << (saddResult ? "SUCCESS" : "FAILED") << std::endl;
        
        bool sismemberResult = cacheManager.Sismember(setKey, member);
        std::cout << "   SISMEMBER: " << (sismemberResult ? "SUCCESS" : "FAILED") << std::endl;
        
        bool sremResult = cacheManager.Srem(setKey, member);
        std::cout << "   SREM: " << (sremResult ? "SUCCESS" : "FAILED") << std::endl;
        
        // 清理测试数据
        cacheManager.Del(key);
        cacheManager.Del(hashKey);
        cacheManager.Del(setKey);
        
        std::cout << "   CacheManager tests completed" << std::endl;
    }
    
    // 测试MessageQueue
    std::cout << "\n--- Testing MessageQueue ---" << std::endl;
    MessageQueue messageQueue;
    
    std::cout << "1. Testing connection..." << std::endl;
    bool mqConnected = messageQueue.Connect();
    std::cout << "   Connection: " << (mqConnected ? "SUCCESS" : "FAILED") << std::endl;
    
    if (mqConnected) {
        // 测试发布消息
        std::cout << "2. Testing publish..." << std::endl;
        int channel = 999;
        std::string message = "Test message for channel 999";
        
        bool published = messageQueue.Publish(channel, message);
        std::cout << "   Publish: " << (published ? "SUCCESS" : "FAILED") << std::endl;
        
        // 测试订阅（这里只测试订阅命令，不测试实际接收消息）
        std::cout << "3. Testing subscribe..." << std::endl;
        bool subscribed = messageQueue.Subscribe(channel);
        std::cout << "   Subscribe: " << (subscribed ? "SUCCESS" : "FAILED") << std::endl;
        
        std::cout << "   MessageQueue tests completed" << std::endl;
    }
    
    std::cout << "\n=== Test Completed ===" << std::endl;
    return 0;
}

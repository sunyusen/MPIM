#include <iostream>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <cassert>

// 包含我们的缓存类
#include "im-user/include/user_cache.h"
#include "im-group/include/group_cache.h"
#include "im-presence/include/presence_service.h"
#include "thirdparty/redisclient/include/cache_manager.h"
#include "thirdparty/redisclient/include/message_queue.h"

using namespace mpim::user;
using namespace mpim::group;
using namespace mpim::redis;

class RedisCacheTester {
public:
    RedisCacheTester() {
        std::cout << "=== Redis Cache Test Suite ===" << std::endl;
    }
    
    void runAllTests() {
        testUserCache();
        testGroupCache();
        testPresenceCache();
        testCacheManager();
        testMessageQueue();
        
        std::cout << "\n=== All Tests Completed ===" << std::endl;
    }
    
private:
    void testUserCache() {
        std::cout << "\n--- Testing UserCache ---" << std::endl;
        
        UserCache userCache;
        
        // 测试连接
        std::cout << "1. Testing connection..." << std::endl;
        bool connected = userCache.Connect();
        std::cout << "   Connection: " << (connected ? "SUCCESS" : "FAILED") << std::endl;
        
        if (!connected) {
            std::cout << "   Skipping UserCache tests (Redis not available)" << std::endl;
            return;
        }
        
        // 测试用户信息缓存
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
        
        // 测试添加好友
        std::cout << "4. Testing add friend..." << std::endl;
        bool addFriend = userCache.AddFriend(userId, 999);
        std::cout << "   Add friend: " << (addFriend ? "SUCCESS" : "FAILED") << std::endl;
        
        // 测试移除好友
        std::cout << "5. Testing remove friend..." << std::endl;
        bool removeFriend = userCache.RemoveFriend(userId, 999);
        std::cout << "   Remove friend: " << (removeFriend ? "SUCCESS" : "FAILED") << std::endl;
        
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
    
    void testGroupCache() {
        std::cout << "\n--- Testing GroupCache ---" << std::endl;
        
        GroupCache groupCache;
        
        // 测试连接
        std::cout << "1. Testing connection..." << std::endl;
        bool connected = groupCache.Connect();
        std::cout << "   Connection: " << (connected ? "SUCCESS" : "FAILED") << std::endl;
        
        if (!connected) {
            std::cout << "   Skipping GroupCache tests (Redis not available)" << std::endl;
            return;
        }
        
        // 测试群组信息缓存
        std::cout << "2. Testing group info cache..." << std::endl;
        int64_t groupId = 1001;
        std::string groupData = "1001:TestGroup:This is a test group";
        
        bool setGroup = groupCache.SetGroupInfo(groupId, groupData, 60);
        std::cout << "   Set group info: " << (setGroup ? "SUCCESS" : "FAILED") << std::endl;
        
        std::string retrieved = groupCache.GetGroupInfo(groupId);
        std::cout << "   Get group info: " << (retrieved == groupData ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "   Retrieved data: " << retrieved << std::endl;
        
        // 测试群成员缓存
        std::cout << "3. Testing group members cache..." << std::endl;
        std::string membersData = "123,456,789";
        
        bool setMembers = groupCache.SetGroupMembers(groupId, membersData, 60);
        std::cout << "   Set group members: " << (setMembers ? "SUCCESS" : "FAILED") << std::endl;
        
        std::string retrievedMembers = groupCache.GetGroupMembers(groupId);
        std::cout << "   Get group members: " << (retrievedMembers == membersData ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "   Retrieved members: " << retrievedMembers << std::endl;
        
        // 测试添加群成员
        std::cout << "4. Testing add group member..." << std::endl;
        bool addMember = groupCache.AddGroupMember(groupId, 999);
        std::cout << "   Add group member: " << (addMember ? "SUCCESS" : "FAILED") << std::endl;
        
        // 测试移除群成员
        std::cout << "5. Testing remove group member..." << std::endl;
        bool removeMember = groupCache.RemoveGroupMember(groupId, 999);
        std::cout << "   Remove group member: " << (removeMember ? "SUCCESS" : "FAILED") << std::endl;
        
        // 测试用户群组缓存
        std::cout << "6. Testing user groups cache..." << std::endl;
        int64_t userId = 123;
        std::string userGroupsData = "1001,1002,1003";
        
        bool setUserGroups = groupCache.SetUserGroups(userId, userGroupsData, 60);
        std::cout << "   Set user groups: " << (setUserGroups ? "SUCCESS" : "FAILED") << std::endl;
        
        std::string retrievedUserGroups = groupCache.GetUserGroups(userId);
        std::cout << "   Get user groups: " << (retrievedUserGroups == userGroupsData ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "   Retrieved user groups: " << retrievedUserGroups << std::endl;
        
        // 清理测试数据
        groupCache.DelGroupInfo(groupId);
        groupCache.DelGroupMembers(groupId);
        groupCache.DelUserGroups(userId);
        
        std::cout << "   GroupCache tests completed" << std::endl;
    }
    
    void testPresenceCache() {
        std::cout << "\n--- Testing Presence Cache ---" << std::endl;
        
        CacheManager cacheManager;
        MessageQueue messageQueue;
        
        // 测试连接
        std::cout << "1. Testing CacheManager connection..." << std::endl;
        bool cacheConnected = cacheManager.Connect();
        std::cout << "   CacheManager connection: " << (cacheConnected ? "SUCCESS" : "FAILED") << std::endl;
        
        std::cout << "2. Testing MessageQueue connection..." << std::endl;
        bool mqConnected = messageQueue.Connect();
        std::cout << "   MessageQueue connection: " << (mqConnected ? "SUCCESS" : "FAILED") << std::endl;
        
        if (!cacheConnected || !mqConnected) {
            std::cout << "   Skipping Presence tests (Redis not available)" << std::endl;
            return;
        }
        
        // 测试路由缓存
        std::cout << "3. Testing route cache..." << std::endl;
        std::string routeKey = "route:123";
        std::string gatewayId = "gateway_001";
        
        bool setRoute = cacheManager.Setex(routeKey, 60, gatewayId);
        std::cout << "   Set route: " << (setRoute ? "SUCCESS" : "FAILED") << std::endl;
        
        std::string retrievedRoute = cacheManager.Get(routeKey);
        std::cout << "   Get route: " << (retrievedRoute == gatewayId ? "SUCCESS" : "FAILED") << std::endl;
        std::cout << "   Retrieved route: " << retrievedRoute << std::endl;
        
        // 测试消息队列
        std::cout << "4. Testing message queue..." << std::endl;
        int channel = 123;
        std::string message = "Hello, World!";
        
        bool published = messageQueue.Publish(channel, message);
        std::cout << "   Publish message: " << (published ? "SUCCESS" : "FAILED") << std::endl;
        
        // 清理测试数据
        cacheManager.Del(routeKey);
        
        std::cout << "   Presence cache tests completed" << std::endl;
    }
    
    void testCacheManager() {
        std::cout << "\n--- Testing CacheManager ---" << std::endl;
        
        CacheManager cacheManager;
        
        // 测试连接
        std::cout << "1. Testing connection..." << std::endl;
        bool connected = cacheManager.Connect();
        std::cout << "   Connection: " << (connected ? "SUCCESS" : "FAILED") << std::endl;
        
        if (!connected) {
            std::cout << "   Skipping CacheManager tests (Redis not available)" << std::endl;
            return;
        }
        
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
    
    void testMessageQueue() {
        std::cout << "\n--- Testing MessageQueue ---" << std::endl;
        
        MessageQueue messageQueue;
        
        // 测试连接
        std::cout << "1. Testing connection..." << std::endl;
        bool connected = messageQueue.Connect();
        std::cout << "   Connection: " << (connected ? "SUCCESS" : "FAILED") << std::endl;
        
        if (!connected) {
            std::cout << "   Skipping MessageQueue tests (Redis not available)" << std::endl;
            return;
        }
        
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
};

int main() {
    try {
        RedisCacheTester tester;
        tester.runAllTests();
        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Test failed with exception: " << e.what() << std::endl;
        return 1;
    }
}

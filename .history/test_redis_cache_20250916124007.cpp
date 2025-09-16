#include <iostream>
#include <memory>
#include "im-common/include/cache/user_cache.h"
#include "im-common/include/logger/log_init.h"
#include "im-common/proto/user.pb.h"
#include "im-common/proto/group.pb.h"

using namespace mpim;

int main() {
    // 初始化日志系统
    LogInit::InitDefault();
    
    std::cout << "=== Redis 缓存功能测试 ===" << std::endl;
    
    // 测试 UserCache
    cache::UserCache user_cache;
    
    // 连接 Redis
    if (!user_cache.Connect()) {
        std::cerr << "Failed to connect to Redis!" << std::endl;
        return 1;
    }
    
    std::cout << "✓ Redis 连接成功" << std::endl;
    
    // 测试 1: 登录信息缓存
    std::cout << "\n--- 测试登录信息缓存 ---" << std::endl;
    
    LoginResp login_resp;
    login_resp.mutable_result()->set_code(Code::Ok);
    login_resp.mutable_result()->set_msg("Login successful");
    login_resp.set_user_id(12345);
    login_resp.set_token("test_token_12345");
    
    // 设置登录信息
    if (user_cache.SetLoginInfo(12345, login_resp, 60)) {
        std::cout << "✓ 登录信息缓存设置成功" << std::endl;
    } else {
        std::cout << "✗ 登录信息缓存设置失败" << std::endl;
    }
    
    // 获取登录信息
    auto cached_login = user_cache.GetLoginInfo(12345);
    if (cached_login) {
        std::cout << "✓ 登录信息缓存获取成功: user_id=" << cached_login->user_id() 
                  << ", token=" << cached_login->token() << std::endl;
    } else {
        std::cout << "✗ 登录信息缓存获取失败" << std::endl;
    }
    
    // 测试 2: 好友关系缓存
    std::cout << "\n--- 测试好友关系缓存 ---" << std::endl;
    
    GetFriendsResp friends_resp;
    friends_resp.mutable_result()->set_code(Code::Ok);
    friends_resp.mutable_result()->set_msg("Success");
    friends_resp.add_friend_ids(1001);
    friends_resp.add_friend_ids(1002);
    friends_resp.add_friend_ids(1003);
    
    // 设置好友列表
    if (user_cache.SetFriends(12345, friends_resp, 60)) {
        std::cout << "✓ 好友列表缓存设置成功" << std::endl;
    } else {
        std::cout << "✗ 好友列表缓存设置失败" << std::endl;
    }
    
    // 获取好友列表
    auto cached_friends = user_cache.GetFriends(12345);
    if (cached_friends) {
        std::cout << "✓ 好友列表缓存获取成功: ";
        for (int i = 0; i < cached_friends->friend_ids_size(); ++i) {
            std::cout << cached_friends->friend_ids(i) << " ";
        }
        std::cout << std::endl;
    } else {
        std::cout << "✗ 好友列表缓存获取失败" << std::endl;
    }
    
    // 测试 3: 群组信息缓存
    std::cout << "\n--- 测试群组信息缓存 ---" << std::endl;
    
    GroupInfo group_info;
    group_info.set_group_id(2001);
    group_info.set_group_name("测试群组");
    group_info.set_description("这是一个测试群组");
    group_info.set_creator_id(12345);
    group_info.set_created_at(1640995200);
    
    // 设置群组信息
    if (user_cache.SetGroupInfo(2001, group_info, 60)) {
        std::cout << "✓ 群组信息缓存设置成功" << std::endl;
    } else {
        std::cout << "✗ 群组信息缓存设置失败" << std::endl;
    }
    
    // 获取群组信息
    auto cached_group = user_cache.GetGroupInfo(2001);
    if (cached_group) {
        std::cout << "✓ 群组信息缓存获取成功: group_id=" << cached_group->group_id()
                  << ", name=" << cached_group->group_name() << std::endl;
    } else {
        std::cout << "✗ 群组信息缓存获取失败" << std::endl;
    }
    
    // 测试 4: 群组成员缓存
    std::cout << "\n--- 测试群组成员缓存 ---" << std::endl;
    
    GetGroupMembersResp members_resp;
    members_resp.mutable_result()->set_code(Code::Ok);
    members_resp.mutable_result()->set_msg("Success");
    members_resp.add_member_ids(12345);
    members_resp.add_member_ids(1001);
    members_resp.add_member_ids(1002);
    
    // 设置群组成员
    if (user_cache.SetGroupMembers(2001, members_resp, 60)) {
        std::cout << "✓ 群组成员缓存设置成功" << std::endl;
    } else {
        std::cout << "✗ 群组成员缓存设置失败" << std::endl;
    }
    
    // 获取群组成员
    auto cached_members = user_cache.GetGroupMembers(2001);
    if (cached_members) {
        std::cout << "✓ 群组成员缓存获取成功: ";
        for (int i = 0; i < cached_members->member_ids_size(); ++i) {
            std::cout << cached_members->member_ids(i) << " ";
        }
        std::cout << std::endl;
    } else {
        std::cout << "✗ 群组成员缓存获取失败" << std::endl;
    }
    
    // 测试 5: 用户群组列表缓存
    std::cout << "\n--- 测试用户群组列表缓存 ---" << std::endl;
    
    GetUserGroupsResp groups_resp;
    groups_resp.mutable_result()->set_code(Code::Ok);
    groups_resp.mutable_result()->set_msg("Success");
    groups_resp.add_group_ids(2001);
    groups_resp.add_group_ids(2002);
    
    // 设置用户群组列表
    if (user_cache.SetUserGroups(12345, groups_resp, 60)) {
        std::cout << "✓ 用户群组列表缓存设置成功" << std::endl;
    } else {
        std::cout << "✗ 用户群组列表缓存设置失败" << std::endl;
    }
    
    // 获取用户群组列表
    auto cached_groups = user_cache.GetUserGroups(12345);
    if (cached_groups) {
        std::cout << "✓ 用户群组列表缓存获取成功: ";
        for (int i = 0; i < cached_groups->group_ids_size(); ++i) {
            std::cout << cached_groups->group_ids(i) << " ";
        }
        std::cout << std::endl;
    } else {
        std::cout << "✗ 用户群组列表缓存获取失败" << std::endl;
    }
    
    std::cout << "\n=== 测试完成 ===" << std::endl;
    
    return 0;
}

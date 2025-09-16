#include <iostream>
#include <memory>
#include "im-common/include/cache/user_cache.h"
#include "im-common/include/logger/log_init.h"
#include "im-common/include/logger/logger.h"

using namespace mpim::cache;

int main() {
    // 初始化日志系统
    mpim::LogInit::InitDefault();
    
    std::cout << "=== Redis Cache 功能测试 ===" << std::endl;
    
    // 创建 UserCache 实例
    UserCache user_cache;
    
    // 测试连接
    std::cout << "\n1. 测试 Redis 连接..." << std::endl;
    if (user_cache.Connect()) {
        std::cout << "✓ Redis 连接成功" << std::endl;
    } else {
        std::cout << "✗ Redis 连接失败" << std::endl;
        return 1;
    }
    
    // 测试登录信息缓存
    std::cout << "\n2. 测试登录信息缓存..." << std::endl;
    mpim::LoginResp login_resp;
    login_resp.set_user_id(12345);
    login_resp.set_token("test_token_12345");
    login_resp.mutable_result()->set_code(mpim::Code::Ok);
    login_resp.mutable_result()->set_msg("Login successful");
    
    if (user_cache.SetLoginInfo(12345, login_resp, 60)) {
        std::cout << "✓ 登录信息缓存设置成功" << std::endl;
        
        auto cached_login = user_cache.GetLoginInfo(12345);
        if (cached_login && cached_login->user_id() == 12345) {
            std::cout << "✓ 登录信息缓存读取成功: user_id=" << cached_login->user_id() 
                      << ", token=" << cached_login->token() << std::endl;
        } else {
            std::cout << "✗ 登录信息缓存读取失败" << std::endl;
        }
    } else {
        std::cout << "✗ 登录信息缓存设置失败" << std::endl;
    }
    
    // 测试好友关系缓存
    std::cout << "\n3. 测试好友关系缓存..." << std::endl;
    mpim::GetFriendsResp friends_resp;
    friends_resp.mutable_result()->set_code(mpim::Code::Ok);
    friends_resp.add_friend_ids(1001);
    friends_resp.add_friend_ids(1002);
    friends_resp.add_friend_ids(1003);
    
    if (user_cache.SetFriends(12345, friends_resp, 60)) {
        std::cout << "✓ 好友关系缓存设置成功" << std::endl;
        
        auto cached_friends = user_cache.GetFriends(12345);
        if (cached_friends && cached_friends->friend_ids_size() == 3) {
            std::cout << "✓ 好友关系缓存读取成功: 好友数量=" << cached_friends->friend_ids_size() << std::endl;
            for (int i = 0; i < cached_friends->friend_ids_size(); ++i) {
                std::cout << "  好友ID: " << cached_friends->friend_ids(i) << std::endl;
            }
        } else {
            std::cout << "✗ 好友关系缓存读取失败" << std::endl;
        }
    } else {
        std::cout << "✗ 好友关系缓存设置失败" << std::endl;
    }
    
    // 测试群组信息缓存
    std::cout << "\n4. 测试群组信息缓存..." << std::endl;
    mpim::GroupInfo group_info;
    group_info.set_group_id(2001);
    group_info.set_group_name("测试群组");
    group_info.set_description("这是一个测试群组");
    group_info.set_creator_id(12345);
    group_info.set_created_at(1640995200); // 2022-01-01 00:00:00
    
    if (user_cache.SetGroupInfo(2001, group_info, 60)) {
        std::cout << "✓ 群组信息缓存设置成功" << std::endl;
        
        auto cached_group = user_cache.GetGroupInfo(2001);
        if (cached_group && cached_group->group_id() == 2001) {
            std::cout << "✓ 群组信息缓存读取成功: group_id=" << cached_group->group_id() 
                      << ", name=" << cached_group->group_name() << std::endl;
        } else {
            std::cout << "✗ 群组信息缓存读取失败" << std::endl;
        }
    } else {
        std::cout << "✗ 群组信息缓存设置失败" << std::endl;
    }
    
    // 测试群组成员缓存
    std::cout << "\n5. 测试群组成员缓存..." << std::endl;
    mpim::GetGroupMembersResp members_resp;
    members_resp.mutable_result()->set_code(mpim::Code::Ok);
    members_resp.add_member_ids(12345);
    members_resp.add_member_ids(1001);
    members_resp.add_member_ids(1002);
    
    if (user_cache.SetGroupMembers(2001, members_resp, 60)) {
        std::cout << "✓ 群组成员缓存设置成功" << std::endl;
        
        auto cached_members = user_cache.GetGroupMembers(2001);
        if (cached_members && cached_members->member_ids_size() == 3) {
            std::cout << "✓ 群组成员缓存读取成功: 成员数量=" << cached_members->member_ids_size() << std::endl;
            for (int i = 0; i < cached_members->member_ids_size(); ++i) {
                std::cout << "  成员ID: " << cached_members->member_ids(i) << std::endl;
            }
        } else {
            std::cout << "✗ 群组成员缓存读取失败" << std::endl;
        }
    } else {
        std::cout << "✗ 群组成员缓存设置失败" << std::endl;
    }
    
    // 测试用户群组列表缓存
    std::cout << "\n6. 测试用户群组列表缓存..." << std::endl;
    mpim::GetUserGroupsResp groups_resp;
    groups_resp.mutable_result()->set_code(mpim::Code::Ok);
    groups_resp.add_group_ids(2001);
    groups_resp.add_group_ids(2002);
    
    if (user_cache.SetUserGroups(12345, groups_resp, 60)) {
        std::cout << "✓ 用户群组列表缓存设置成功" << std::endl;
        
        auto cached_groups = user_cache.GetUserGroups(12345);
        if (cached_groups && cached_groups->group_ids_size() == 2) {
            std::cout << "✓ 用户群组列表缓存读取成功: 群组数量=" << cached_groups->group_ids_size() << std::endl;
            for (int i = 0; i < cached_groups->group_ids_size(); ++i) {
                std::cout << "  群组ID: " << cached_groups->group_ids(i) << std::endl;
            }
        } else {
            std::cout << "✗ 用户群组列表缓存读取失败" << std::endl;
        }
    } else {
        std::cout << "✗ 用户群组列表缓存设置失败" << std::endl;
    }
    
    // 测试删除操作
    std::cout << "\n7. 测试删除操作..." << std::endl;
    if (user_cache.DelLoginInfo(12345)) {
        std::cout << "✓ 登录信息删除成功" << std::endl;
        
        auto deleted_login = user_cache.GetLoginInfo(12345);
        if (!deleted_login) {
            std::cout << "✓ 登录信息删除验证成功" << std::endl;
        } else {
            std::cout << "✗ 登录信息删除验证失败" << std::endl;
        }
    } else {
        std::cout << "✗ 登录信息删除失败" << std::endl;
    }
    
    std::cout << "\n=== Redis Cache 功能测试完成 ===" << std::endl;
    return 0;
}
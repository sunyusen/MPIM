#include <gtest/gtest.h>
#include <thread>
#include <chrono>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string>
#include <vector>

class MPIMTest : public ::testing::Test {
protected:
    void SetUp() override {
        // 等待服务启动
        std::this_thread::sleep_for(std::chrono::seconds(2));
    }
    
    void TearDown() override {
        // 清理资源
    }
    
    // 发送命令到服务器
    std::string sendCommand(const std::string& command) {
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            return "SOCKET_ERROR";
        }
        
        struct sockaddr_in server_addr;
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(6000);
        server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
        
        if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            close(sock);
            return "CONNECT_ERROR";
        }
        
        std::string full_command = command + "\n";
        send(sock, full_command.c_str(), full_command.length(), 0);
        
        char buffer[1024];
        int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
        close(sock);
        
        if (n > 0) {
            buffer[n] = '\0';
            return std::string(buffer);
        }
        return "RECV_ERROR";
    }
    
    // 解析响应
    bool isSuccess(const std::string& response) {
        return response.find("+OK") == 0;
    }
    
    bool isError(const std::string& response) {
        return response.find("-ERR") == 0;
    }
};

// 测试用户注册
TEST_F(MPIMTest, TestUserRegistration) {
    std::string response = sendCommand("register testuser1 123456");
    EXPECT_TRUE(isSuccess(response)) << "Registration failed: " << response;
    
    // 测试重复注册
    response = sendCommand("register testuser1 123456");
    EXPECT_TRUE(isError(response)) << "Duplicate registration should fail: " << response;
}

// 测试用户登录
TEST_F(MPIMTest, TestUserLogin) {
    // 先注册用户
    sendCommand("register testuser2 123456");
    
    // 测试正确登录
    std::string response = sendCommand("login testuser2 123456");
    EXPECT_TRUE(isSuccess(response)) << "Login failed: " << response;
    
    // 测试错误密码
    response = sendCommand("login testuser2 wrongpass");
    EXPECT_TRUE(isError(response)) << "Wrong password should fail: " << response;
    
    // 测试不存在的用户
    response = sendCommand("login nonexistent 123456");
    EXPECT_TRUE(isError(response)) << "Nonexistent user should fail: " << response;
}

// 测试用户登出
TEST_F(MPIMTest, TestUserLogout) {
    // 先注册并登录
    sendCommand("register testuser3 123456");
    sendCommand("login testuser3 123456");
    
    // 测试登出
    std::string response = sendCommand("logout");
    EXPECT_TRUE(isSuccess(response)) << "Logout failed: " << response;
}

// 测试好友功能
TEST_F(MPIMTest, TestFriendFunctions) {
    // 注册两个用户
    sendCommand("register testuser4 123456");
    sendCommand("register testuser5 123456");
    
    // 登录第一个用户
    std::string response = sendCommand("login testuser4 123456");
    EXPECT_TRUE(isSuccess(response)) << "Login failed: " << response;
    
    // 添加好友（假设用户ID为4和5）
    response = sendCommand("addfriend 5");
    EXPECT_TRUE(isSuccess(response)) << "Add friend failed: " << response;
    
    // 获取好友列表
    response = sendCommand("getfriends");
    EXPECT_TRUE(isSuccess(response)) << "Get friends failed: " << response;
    EXPECT_TRUE(response.find("5") != std::string::npos) << "Friend not in list: " << response;
}

// 测试群组功能
TEST_F(MPIMTest, TestGroupFunctions) {
    // 注册用户
    sendCommand("register testuser6 123456");
    sendCommand("register testuser7 123456");
    
    // 登录第一个用户
    std::string response = sendCommand("login testuser6 123456");
    EXPECT_TRUE(isSuccess(response)) << "Login failed: " << response;
    
    // 创建群组
    response = sendCommand("creategroup testgroup1 测试群组");
    EXPECT_TRUE(isSuccess(response)) << "Create group failed: " << response;
    
    // 登录第二个用户
    sendCommand("logout");
    response = sendCommand("login testuser7 123456");
    EXPECT_TRUE(isSuccess(response)) << "Login failed: " << response;
    
    // 加入群组（假设群组ID为1）
    response = sendCommand("joingroup 1");
    EXPECT_TRUE(isSuccess(response)) << "Join group failed: " << response;
}

// 测试消息发送
TEST_F(MPIMTest, TestMessageSending) {
    // 注册两个用户
    sendCommand("register testuser8 123456");
    sendCommand("register testuser9 123456");
    
    // 登录第一个用户
    std::string response = sendCommand("login testuser8 123456");
    EXPECT_TRUE(isSuccess(response)) << "Login failed: " << response;
    
    // 发送消息（假设用户ID为8和9）
    response = sendCommand("chat 9 Hello from user8");
    EXPECT_TRUE(isSuccess(response)) << "Send message failed: " << response;
    
    // 登录第二个用户
    sendCommand("logout");
    response = sendCommand("login testuser9 123456");
    EXPECT_TRUE(isSuccess(response)) << "Login failed: " << response;
    
    // 拉取离线消息
    response = sendCommand("pull");
    EXPECT_TRUE(isSuccess(response)) << "Pull messages failed: " << response;
}

// 测试群组消息
TEST_F(MPIMTest, TestGroupMessage) {
    // 注册用户
    sendCommand("register testuser10 123456");
    
    // 登录用户
    std::string response = sendCommand("login testuser10 123456");
    EXPECT_TRUE(isSuccess(response)) << "Login failed: " << response;
    
    // 创建群组
    response = sendCommand("creategroup testgroup2 测试群组2");
    EXPECT_TRUE(isSuccess(response)) << "Create group failed: " << response;
    
    // 发送群组消息（假设群组ID为2）
    response = sendCommand("sendgroup 2 Hello group message");
    EXPECT_TRUE(isSuccess(response)) << "Send group message failed: " << response;
}

// 测试错误处理
TEST_F(MPIMTest, TestErrorHandling) {
    // 测试未登录时发送消息
    std::string response = sendCommand("chat 1 Hello");
    EXPECT_TRUE(isError(response)) << "Should fail when not logged in: " << response;
    
    // 测试无效命令
    response = sendCommand("invalidcommand");
    EXPECT_TRUE(isError(response)) << "Invalid command should fail: " << response;
    
    // 测试参数不足
    response = sendCommand("login");
    EXPECT_TRUE(isError(response)) << "Insufficient parameters should fail: " << response;
}

// 测试并发操作
TEST_F(MPIMTest, TestConcurrentOperations) {
    // 注册多个用户
    for (int i = 1; i <= 5; ++i) {
        std::string cmd = "register concurrentuser" + std::to_string(i) + " 123456";
        std::string response = sendCommand(cmd);
        EXPECT_TRUE(isSuccess(response)) << "Concurrent registration failed: " << response;
    }
    
    // 并发登录测试
    std::vector<std::thread> threads;
    std::vector<bool> results(5, false);
    
    for (int i = 0; i < 5; ++i) {
        threads.emplace_back([i, &results]() {
            int sock = socket(AF_INET, SOCK_STREAM, 0);
            if (sock < 0) return;
            
            struct sockaddr_in server_addr;
            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(6000);
            server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
            
            if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
                close(sock);
                return;
            }
            
            std::string cmd = "login concurrentuser" + std::to_string(i + 1) + " 123456\n";
            send(sock, cmd.c_str(), cmd.length(), 0);
            
            char buffer[1024];
            int n = recv(sock, buffer, sizeof(buffer) - 1, 0);
            close(sock);
            
            if (n > 0) {
                buffer[n] = '\0';
                results[i] = (std::string(buffer).find("+OK") == 0);
            }
        });
    }
    
    for (auto& t : threads) {
        t.join();
    }
    
    for (int i = 0; i < 5; ++i) {
        EXPECT_TRUE(results[i]) << "Concurrent login " << i << " failed";
    }
}

// 性能测试
TEST_F(MPIMTest, TestPerformance) {
    // 注册用户
    sendCommand("register perftest 123456");
    
    // 登录
    std::string response = sendCommand("login perftest 123456");
    EXPECT_TRUE(isSuccess(response)) << "Login failed: " << response;
    
    // 测试大量消息发送
    auto start = std::chrono::high_resolution_clock::now();
    
    for (int i = 0; i < 100; ++i) {
        std::string cmd = "chat 1 Message " + std::to_string(i);
        response = sendCommand(cmd);
        EXPECT_TRUE(isSuccess(response)) << "Message " << i << " failed: " << response;
    }
    
    auto end = std::chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start);
    
    std::cout << "Sent 100 messages in " << duration.count() << "ms" << std::endl;
    EXPECT_LT(duration.count(), 5000) << "Performance test took too long: " << duration.count() << "ms";
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

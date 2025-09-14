#pragma once
#include <string>
#include <map>
#include <chrono>

namespace mpim {
namespace auth {

class JWTAuth {
public:
    static JWTAuth& GetInstance();
    
    // 生成JWT Token
    std::string GenerateToken(const std::string& user_id, 
                             const std::string& username,
                             int64_t expire_seconds = 3600);
    
    // 验证JWT Token
    bool VerifyToken(const std::string& token);
    
    // 解析Token获取用户信息
    std::map<std::string, std::string> ParseToken(const std::string& token);
    
    // 刷新Token
    std::string RefreshToken(const std::string& old_token);
    
    // 撤销Token
    bool RevokeToken(const std::string& token);

private:
    std::string secret_key_;
    std::map<std::string, std::chrono::system_clock::time_point> revoked_tokens_;
    std::mutex mutex_;
};

} // namespace auth
} // namespace mpim

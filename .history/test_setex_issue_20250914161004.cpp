#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <cstring>

int main() {
    std::cout << "Testing SETEX issue..." << std::endl;
    
    redisContext *c = redisConnect("127.0.0.1", 6379);
    if (c == NULL || c->err) {
        if (c) {
            std::cout << "Connection error: " << c->errstr << std::endl;
            redisFree(c);
        } else {
            std::cout << "Can't allocate redis context" << std::endl;
        }
        return 1;
    }
    
    std::cout << "Redis connection successful!" << std::endl;
    
    // 测试与你的代码完全相同的SETEX命令
    std::string key = "route:5";
    std::string value = "gateway-1";
    int ttl = 120;
    
    std::cout << "Testing SETEX with key='" << key << "', value='" << value << "', ttl=" << ttl << std::endl;
    
    redisReply *reply = (redisReply*)redisCommand(
        c, "SETEX %b %d %b",
        key.data(), (size_t)key.size(),
        ttl,
        value.data(), (size_t)value.size());
    
    if (reply == NULL) {
        std::cout << "SETEX command failed - reply is NULL" << std::endl;
        redisFree(c);
        return 1;
    }
    
    std::cout << "SETEX reply type: " << reply->type << std::endl;
    if (reply->str) {
        std::cout << "SETEX reply: " << reply->str << std::endl;
    }
    
    bool success = (reply->type == REDIS_REPLY_STATUS) &&
                   (reply->str && strcasecmp(reply->str, "OK") == 0);
    
    std::cout << "SETEX success: " << (success ? "true" : "false") << std::endl;
    
    freeReplyObject(reply);
    
    // 验证设置是否成功
    reply = (redisReply*)redisCommand(c, "GET %b", key.data(), (size_t)key.size());
    if (reply && reply->type == REDIS_REPLY_STRING) {
        std::cout << "GET result: " << reply->str << std::endl;
    } else {
        std::cout << "GET failed or key not found" << std::endl;
    }
    freeReplyObject(reply);
    
    // 清理
    reply = (redisReply*)redisCommand(c, "DEL %b", key.data(), (size_t)key.size());
    freeReplyObject(reply);
    
    redisFree(c);
    std::cout << "SETEX test completed!" << std::endl;
    return 0;
}

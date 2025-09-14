#include <hiredis/hiredis.h>
#include <iostream>
#include <string>
#include <cstring>

// 模拟修复后的Redis方法
bool setex_fixed(redisContext* context, const std::string& key, int ttl_sec, const std::string& value) {
  redisReply* reply = (redisReply*)redisCommand(
      context, "SETEX %s %d %s",
      key.c_str(),
      ttl_sec,
      value.c_str());
  if (!reply) { 
    std::cerr << "SETEX failed - reply is NULL\n"; 
    return false; 
  }
  
  // 检查回复类型和内容
  bool ok = false;
  if (reply->type == REDIS_REPLY_STATUS) {
    if (reply->str && strcasecmp(reply->str, "OK") == 0) {
      ok = true;
    } else {
      std::cerr << "SETEX failed - unexpected status: " << (reply->str ? reply->str : "NULL") << std::endl;
    }
  } else {
    std::cerr << "SETEX failed - unexpected reply type: " << reply->type << std::endl;
  }
  
  freeReplyObject(reply);
  return ok;
}

std::string get_fixed(redisContext* context, const std::string& key, bool* found) {
  if (found) *found = false;
  redisReply* reply = (redisReply*)redisCommand(
      context, "GET %s", key.c_str());
  if (!reply) { 
    std::cerr << "GET failed - reply is NULL\n"; 
    return {}; 
  }

  std::string out;
  if (reply->type == REDIS_REPLY_STRING) {
    out.assign(reply->str, (size_t)reply->len);
    if (found) *found = true;
  } else if (reply->type == REDIS_REPLY_NIL) {
    // key 不存在
  } else {
    std::cerr << "GET unexpected reply type: " << reply->type << "\n";
  }
  freeReplyObject(reply);
  return out;
}

int main() {
    std::cout << "Testing fixed Redis methods..." << std::endl;
    
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
    
    // 测试修复后的SETEX方法
    std::string key = "route:5";
    std::string value = "gateway-1";
    int ttl = 120;
    
    std::cout << "Testing fixed SETEX..." << std::endl;
    bool success = setex_fixed(c, key, ttl, value);
    std::cout << "SETEX success: " << (success ? "true" : "false") << std::endl;
    
    // 测试修复后的GET方法
    std::cout << "Testing fixed GET..." << std::endl;
    bool found = false;
    std::string result = get_fixed(c, key, &found);
    std::cout << "GET result: '" << result << "', found: " << (found ? "true" : "false") << std::endl;
    
    // 清理
    redisReply* reply = (redisReply*)redisCommand(c, "DEL %s", key.c_str());
    freeReplyObject(reply);
    
    redisFree(c);
    std::cout << "Fixed Redis test completed!" << std::endl;
    return 0;
}

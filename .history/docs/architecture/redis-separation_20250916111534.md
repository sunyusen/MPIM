# Redis功能分离设计

## 设计背景

随着系统功能增加，Redis承担了多种职责：
- 消息队列（pub/sub模式）
- 在线路由缓存
- 用户信息缓存
- 群组信息缓存

为了职责清晰、易于维护，将Redis功能分离为三个独立的类。

## 架构设计

### 1. RedisClient - 底层连接管理
```cpp
class RedisClient {
    // 单例模式，管理Redis连接
    static RedisClient& GetInstance();
    bool Connect(const std::string& host, int port);
    redisReply* ExecuteCommand(const char* format, ...);
};
```

**职责**:
- 管理Redis连接生命周期
- 提供基础Redis命令执行接口
- 单例模式，避免重复连接

### 2. MessageQueue - 消息队列功能
```cpp
class MessageQueue {
    bool Publish(int channel, const std::string& message);
    bool Subscribe(int channel);
    void SetNotifyHandler(std::function<void(int, std::string)> handler);
    void Start();
    void Stop();
};
```

**职责**:
- 消息发布/订阅
- 异步消息处理
- 频道管理

### 3. CacheManager - 缓存管理功能
```cpp
class CacheManager {
    // 字符串操作
    bool Setex(const std::string& key, int ttl, const std::string& value);
    std::string Get(const std::string& key, bool* found = nullptr);
    
    // 哈希操作
    bool Hset(const std::string& key, const std::string& field, const std::string& value);
    std::string Hget(const std::string& key, const std::string& field);
    bool Hgetall(const std::string& key, std::map<std::string, std::string>& result);
    
    // 集合操作
    bool Sadd(const std::string& key, const std::string& member);
    std::vector<std::string> Smembers(const std::string& key);
};
```

**职责**:
- 在线路由缓存
- 用户信息缓存
- 群组信息缓存
- 好友关系缓存

## 使用场景

### 1. 在线路由管理 (Presence服务)
```cpp
class PresenceServiceImpl {
private:
    mpim::redis::CacheManager cache_manager_;
    mpim::redis::MessageQueue message_queue_;
    
public:
    // 绑定用户路由
    void BindRoute(int64_t user_id, const std::string& gateway_id) {
        std::string key = "route:" + std::to_string(user_id);
        cache_manager_.Setex(key, 120, gateway_id); // 120秒TTL
    }
    
    // 查询用户路由
    std::string QueryRoute(int64_t user_id) {
        std::string key = "route:" + std::to_string(user_id);
        bool found = false;
        return cache_manager_.Get(key, &found);
    }
    
    // 投递消息
    void DeliverMessage(int64_t to_user, const std::string& message) {
        std::string gateway_id = QueryRoute(to_user);
        if (!gateway_id.empty()) {
            int channel = 10000 + hash(gateway_id) % 50000;
            message_queue_.Publish(channel, message);
        }
    }
};
```

### 2. 用户信息缓存 (User服务)
```cpp
class UserServiceImpl {
private:
    mpim::cache::UserCache user_cache_;
    
public:
    std::shared_ptr<User> GetUser(int64_t user_id) {
        // 先查缓存
        auto cached_user = user_cache_.GetUser(user_id);
        if (cached_user) {
            return cached_user;
        }
        
        // 查数据库并缓存
        auto user = mysql_.GetUser(user_id);
        if (user) {
            user_cache_.SetUser(user_id, *user, 3600); // 1小时缓存
        }
        
        return user;
    }
};
```

### 3. 消息投递 (Gateway服务)
```cpp
class GatewayServer {
private:
    mpim::redis::MessageQueue message_queue_;
    
public:
    void Start() {
        // 订阅网关频道
        int channel = gatewayChannel();
        message_queue_.Subscribe(channel);
        message_queue_.SetNotifyHandler([this](int ch, std::string msg) {
            // 处理接收到的消息
            ProcessMessage(msg);
        });
        message_queue_.Start();
    }
};
```

## 优势

### 1. 职责清晰
- 每个类只负责一个功能
- 代码更易理解和维护
- 符合单一职责原则

### 2. 易于测试
- 可以单独测试每个功能
- 模拟测试更容易
- 单元测试覆盖更全面

### 3. 易于扩展
- 新增Redis功能只需添加新类
- 不影响现有功能
- 支持不同Redis实例

### 4. 性能优化
- 连接复用，减少连接开销
- 专门的缓存策略
- 异步消息处理

## 配置管理

### Redis连接配置
```cpp
// 所有Redis操作共享同一个连接池
RedisClient& client = RedisClient::GetInstance();
client.Connect("127.0.0.1", 6379);
```

### 缓存策略配置
```cpp
// 用户信息缓存：1小时
user_cache_.SetUser(user_id, user, 3600);

// 好友关系缓存：30分钟
user_cache_.SetFriends(user_id, friends, 1800);

// 在线路由缓存：2分钟
cache_manager_.Setex(route_key, 120, gateway_id);
```

## 监控和运维

### 1. 连接监控
```cpp
bool RedisClient::IsConnected() const {
    return context_ != nullptr && !context_->err;
}
```

### 2. 缓存命中率监控
```cpp
class CacheManager {
    void RecordCacheHit(const std::string& key) {
        hit_count_[key]++;
    }
    
    void RecordCacheMiss(const std::string& key) {
        miss_count_[key]++;
    }
};
```

### 3. 消息队列监控
```cpp
class MessageQueue {
    void RecordMessageSent(int channel) {
        sent_count_[channel]++;
    }
    
    void RecordMessageReceived(int channel) {
        received_count_[channel]++;
    }
};
```

## 总结

Redis功能分离设计实现了：
- **职责清晰**: 每个类只负责一个功能
- **易于维护**: 代码结构更清晰
- **性能优化**: 连接复用和专门优化
- **易于扩展**: 支持新功能添加
- **监控友好**: 便于性能监控和问题排查

这种设计既保持了Redis的高性能特性，又提高了代码的可维护性和可扩展性。

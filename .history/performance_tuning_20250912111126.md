# MPIM 性能优化指南

## 1. 数据库优化

### MySQL 连接池
```cpp
// 在 im-user 中实现连接池
class MySQLPool {
private:
    std::queue<MYSQL*> available_connections_;
    std::mutex mutex_;
    std::condition_variable cv_;
    size_t max_connections_ = 10;
public:
    MYSQL* acquire();
    void release(MYSQL* conn);
};
```

### 索引优化
```sql
-- 为 user 表添加索引
ALTER TABLE user ADD UNIQUE INDEX idx_name (name);
ALTER TABLE user ADD INDEX idx_id (id);

-- 为消息表添加索引（如果存在）
CREATE INDEX idx_to_ts ON messages (to_user_id, timestamp);
```

## 2. Redis 优化

### 连接池实现
```cpp
class RedisPool {
private:
    std::queue<Redis*> available_connections_;
    std::mutex mutex_;
    std::condition_variable cv_;
public:
    Redis* acquire();
    void release(Redis* conn);
};
```

### Pipeline 批量操作
```cpp
// 批量执行 Redis 命令
std::vector<std::string> commands = {"SET key1 val1", "SET key2 val2", "GET key1"};
redis.pipeline(commands);
```

## 3. RPC 连接池

### 连接池实现
```cpp
class RpcConnectionPool {
private:
    std::unordered_map<std::string, std::queue<int>> pools_;
    std::mutex mutex_;
public:
    int borrow(const std::string& service_method);
    void return_connection(const std::string& service_method, int fd);
};
```

## 4. 系统调优

### TCP 参数调优
```bash
# 增加 TCP 缓冲区
echo 'net.core.rmem_max = 134217728' >> /etc/sysctl.conf
echo 'net.core.wmem_max = 134217728' >> /etc/sysctl.conf
echo 'net.ipv4.tcp_rmem = 4096 87380 134217728' >> /etc/sysctl.conf
echo 'net.ipv4.tcp_wmem = 4096 65536 134217728' >> /etc/sysctl.conf

# 优化连接参数
echo 'net.core.somaxconn = 65535' >> /etc/sysctl.conf
echo 'net.ipv4.tcp_max_syn_backlog = 65535' >> /etc/sysctl.conf
echo 'net.ipv4.tcp_tw_reuse = 1' >> /etc/sysctl.conf

# 应用设置
sysctl -p
```

### 文件描述符限制
```bash
# 增加文件描述符限制
echo '* soft nofile 65535' >> /etc/security/limits.conf
echo '* hard nofile 65535' >> /etc/security/limits.conf
```

## 5. 应用层优化

### 内存池
```cpp
class ObjectPool {
private:
    std::queue<T*> available_objects_;
    std::mutex mutex_;
public:
    T* acquire();
    void release(T* obj);
};
```

### 批量处理
```cpp
// 批量处理消息
class BatchProcessor {
public:
    void add_message(const Message& msg);
    void flush(); // 批量处理
private:
    std::vector<Message> batch_;
    std::mutex mutex_;
};
```

## 6. 监控和调优

### 性能指标监控
- QPS (每秒查询数)
- 延迟分布 (P50, P95, P99)
- 错误率
- 资源使用率 (CPU, 内存, 网络)

### 压测工具
```bash
# 使用新的端到端延迟测试
./e2e_latency_bench --senders=8 --receivers=8 --seconds=60 --msgs_per_sender=1000

# 使用原始 RPC 测试
./rpc_roundtrip_bench --threads=16 --qps=0 --seconds=30
```

## 7. 架构优化建议

### 微服务拆分
- 将 `im-user` 拆分为认证服务和用户信息服务
- 将 `im-message` 拆分为消息存储和消息路由服务
- 引入消息队列 (如 Kafka) 处理高并发消息

### 缓存策略
- 在网关层添加 Redis 缓存用户状态
- 使用本地缓存缓存热点数据
- 实现多级缓存策略

### 负载均衡
- 在网关前添加负载均衡器
- 实现基于用户 ID 的一致性哈希
- 支持动态扩缩容

## 8. 代码优化示例

### 减少内存分配
```cpp
// 使用对象池复用 protobuf 对象
class MessagePool {
private:
    std::queue<mpim::C2CMsg*> pool_;
public:
    mpim::C2CMsg* acquire() {
        std::lock_guard<std::mutex> lk(mutex_);
        if (pool_.empty()) {
            return new mpim::C2CMsg();
        }
        auto* msg = pool_.front();
        pool_.pop();
        msg->Clear();
        return msg;
    }
    
    void release(mpim::C2CMsg* msg) {
        std::lock_guard<std::mutex> lk(mutex_);
        pool_.push(msg);
    }
};
```

### 异步处理
```cpp
// 使用异步 RPC 调用
class AsyncRpcClient {
public:
    void send_async(const Request& req, std::function<void(Response&)> callback);
private:
    std::thread worker_thread_;
    std::queue<Task> task_queue_;
};
```

## 9. 部署优化

### Docker 优化
```dockerfile
# 多阶段构建减小镜像大小
FROM ubuntu:20.04 AS builder
# ... 构建步骤

FROM ubuntu:20.04
# 只复制必要的文件
COPY --from=builder /app/bin /usr/local/bin
```

### 容器资源限制
```yaml
# docker-compose.yml
services:
  im-gateway:
    deploy:
      resources:
        limits:
          cpus: '2.0'
          memory: 2G
        reservations:
          cpus: '1.0'
          memory: 1G
```

## 10. 监控和告警

### Prometheus 指标
```cpp
// 添加性能指标
class Metrics {
private:
    prometheus::Counter* rpc_requests_total_;
    prometheus::Histogram* rpc_duration_seconds_;
    prometheus::Gauge* active_connections_;
public:
    void record_rpc_request(const std::string& method);
    void record_rpc_duration(const std::string& method, double seconds);
    void set_active_connections(int count);
};
```

### Grafana 仪表板
- 创建实时监控面板
- 设置关键指标告警
- 实现自动化运维

这些优化建议可以根据实际需求和资源情况逐步实施。建议先从数据库连接池和 Redis 优化开始，这些通常能带来最明显的性能提升。

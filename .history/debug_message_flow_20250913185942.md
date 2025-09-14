# 消息流程调试指南

## 问题排查步骤

### 1. 检查服务启动状态
```bash
# 检查所有服务是否运行
ps aux | grep im-

# 检查端口占用
netstat -tlnp | grep -E "(6000|8010|8011|8012|8013)"
```

### 2. 检查ZooKeeper服务注册
```bash
# 连接ZooKeeper
zkCli.sh -server 127.0.0.1:2181

# 在ZooKeeper中执行：
ls /
ls /PresenceService
ls /MessageService
ls /UserService

# 检查具体的方法节点
ls /PresenceService/QueryRoute
ls /PresenceService/Deliver
ls /MessageService/Send
```

### 3. 检查Redis连接
```bash
# 连接Redis
redis-cli

# 检查Redis连接
ping

# 检查Redis中的路由信息
keys route:*
```

### 4. 检查数据库
```bash
# 连接MySQL
mysql -u root -p123456

# 检查用户状态
USE mpchat;
SELECT id, name, state FROM user;

# 检查离线消息
SELECT * FROM offlinemessage;
```

### 5. 测试消息发送流程

#### 步骤1: 启动两个客户端
```bash
# 终端1
./bin/client 127.0.0.1 6000
login:user1:123456

# 终端2  
./bin/client 127.0.0.1 6000
login:user2:123456
```

#### 步骤2: 发送消息
```bash
# 在终端1中
chat:2:hello from user1
```

#### 步骤3: 检查日志
```bash
# 查看网关日志
tail -f bin/logs/im-gateway.log

# 查看消息服务日志
tail -f bin/logs/im-messaged.log

# 查看Presence服务日志
tail -f bin/logs/im-presenced.log
```

### 6. 预期日志输出

#### 消息发送时应该看到：
```
MessageService::Send: from=1 to=2 text=hello from user1
MessageService::Send: Calling Presence QueryRoute for user 2
PresenceService::QueryRoute: user_id=2 route_key=route:2
Found route for user 2 gateway=gateway-001
MessageService::Send: User 2 is online, gateway=gateway-001
PresenceService::Deliver: to=2 route_key=route:2
Found gateway gateway-001 for user 2
Publishing to channel 1522041372 for gateway gateway-001
Message published successfully to channel 1522041372
Gateway: Received message from Redis, payload size=45
Gateway: Parsed message to=2 from=1 text=hello from user1
Gateway: Found connection for user 2, sending message
```

#### 离线消息存储时应该看到：
```
MessageService::Send: User 2 is offline or route not found
Storing message offline for user 2
OfflineModel::insert: uid=2 payload_size=45
OfflineModel::insert: SQL=INSERT INTO offlinemessage(userid,message,msg_type) VALUES(2,'...','c2c')
OfflineModel::insert: result=success
```

### 7. 常见问题

1. **RPC调用失败**: 检查ZooKeeper服务注册
2. **Redis连接失败**: 检查Redis服务是否运行
3. **数据库连接失败**: 检查MySQL服务和数据库配置
4. **消息投递失败**: 检查Redis发布/订阅机制
5. **离线消息存储失败**: 检查数据库连接和SQL语句

### 8. 调试命令

```bash
# 重新编译
cd build && make -j4

# 重启所有服务
cd ..
./bin/stop.sh
./bin/start.sh

# 查看实时日志
tail -f bin/logs/*.log
```

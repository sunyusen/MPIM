#!/bin/bash

echo "=== MPIM 消息流调试脚本 ==="
echo

# 1. 检查服务状态
echo "1. 检查服务进程状态:"
ps aux | grep -E "(im-userd|im-presenced|im-messaged|im-groupd|im-gateway)" | grep -v grep
echo

# 2. 检查端口监听
echo "2. 检查端口监听状态:"
netstat -tlnp | grep -E "(8010|8011|8012|8013|9000)"
echo

# 3. 检查Redis连接
echo "3. 检查Redis连接:"
redis-cli ping
echo

# 4. 检查ZooKeeper服务注册
echo "4. 检查ZooKeeper服务注册:"
echo "ls /" | zkCli.sh -server 127.0.0.1:2181 2>/dev/null | grep -E "(PresenceService|MessageService|UserService)"
echo

# 5. 检查数据库连接
echo "5. 检查数据库连接:"
mysql -u root -p123456 -e "SELECT COUNT(*) as user_count FROM mpchat.users;" 2>/dev/null
echo

# 6. 检查离线消息表
echo "6. 检查离线消息表:"
mysql -u root -p123456 -e "SELECT userid, msg_type, LENGTH(message) as msg_len FROM mpchat.offlinemessage ORDER BY id DESC LIMIT 5;" 2>/dev/null
echo

# 7. 检查Redis中的路由信息
echo "7. 检查Redis中的路由信息:"
redis-cli keys "route:*" | head -5
echo

# 8. 检查Redis频道订阅
echo "8. 检查Redis频道订阅:"
redis-cli pubsub channels | head -5
echo

echo "=== 调试完成 ==="
echo "如果发现问题，请查看相关服务的日志文件:"
echo "  tail -f logs/im-presenced.log"
echo "  tail -f logs/im-messaged.log"
echo "  tail -f logs/im-gateway.log"

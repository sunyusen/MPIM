#!/bin/bash

echo "=== ZooKeeper连接测试 ==="

# 1. 检查ZooKeeper服务状态
echo "1. 检查ZooKeeper服务状态:"
ps aux | grep zookeeper | grep -v grep
echo

# 2. 检查ZooKeeper端口
echo "2. 检查ZooKeeper端口:"
netstat -tlnp | grep 2181
echo

# 3. 测试ZooKeeper连接
echo "3. 测试ZooKeeper连接:"
echo "ls /" | zkCli.sh -server 127.0.0.1:2181
echo

# 4. 检查现有服务注册
echo "4. 检查现有服务注册:"
echo "ls /" | zkCli.sh -server 127.0.0.1:2181 | grep -E "(PresenceService|MessageService|UserService|GroupService)"
echo

# 5. 手动创建测试节点
echo "5. 手动创建测试节点:"
echo "create /test_node 'test_data'" | zkCli.sh -server 127.0.0.1:2181
echo "get /test_node" | zkCli.sh -server 127.0.0.1:2181
echo "delete /test_node" | zkCli.sh -server 127.0.0.1:2181
echo

echo "=== 测试完成 ==="

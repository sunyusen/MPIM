#!/bin/bash

echo "=== ZooKeeper服务注册测试脚本 ==="

# 1. 停止所有服务
echo "1. 停止所有服务..."
./bin/stop.sh

# 2. 清理ZooKeeper中的旧节点
echo "2. 清理ZooKeeper中的旧节点..."
zkCli.sh -server 127.0.0.1:2181 << 'EOF'
rmr /UserService
rmr /PresenceService  
rmr /MessageService
rmr /GroupService
rmr /HealthService
rmr /UserServiceRpc
rmr /FriendServiceRpc
quit
EOF

# 3. 重新编译项目
echo "3. 重新编译项目..."
cd build
make -j4
if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi
cd ..

# 4. 启动服务
echo "4. 启动服务..."
./bin/start.sh

# 5. 等待服务启动
echo "5. 等待服务启动..."
sleep 5

# 6. 检查ZooKeeper中的服务注册
echo "6. 检查ZooKeeper中的服务注册..."
zkCli.sh -server 127.0.0.1:2181 << 'EOF'
ls /
ls /UserService
ls /PresenceService
ls /MessageService
ls /GroupService
ls /HealthService
quit
EOF

echo "=== 测试完成 ==="

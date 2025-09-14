#!/bin/bash

# 手动测试脚本
echo "=== MPIM 手动测试脚本 ==="

# 检查服务是否运行
check_service() {
    local port=$1
    local service_name=$2
    if nc -z 127.0.0.1 $port 2>/dev/null; then
        echo "✅ $service_name 正在运行 (端口 $port)"
        return 0
    else
        echo "❌ $service_name 未运行 (端口 $port)"
        return 1
    fi
}

echo "1. 检查服务状态..."
check_service 6000 "Gateway" || exit 1
check_service 8010 "User Service" || exit 1
check_service 8012 "Presence Service" || exit 1
check_service 8011 "Message Service" || exit 1
check_service 8013 "Group Service" || exit 1

echo ""
echo "2. 测试客户端连接..."
echo "请手动运行以下命令进行测试："
echo ""
echo "# 启动客户端"
echo "./bin/client 127.0.0.1 6000"
echo ""
echo "# 测试命令序列："
echo "register testuser1 123456"
echo "login testuser1 123456"
echo "addfriend 2"
echo "getfriends"
echo "creategroup testgroup 测试群组"
echo "joingroup 1"
echo "chat 2 Hello friend"
echo "sendgroup 1 Hello group"
echo "pull"
echo "logout"
echo "quit"
echo ""
echo "3. 检查数据库..."
echo "请检查数据库中的表："
echo "mysql -u root -p -e 'USE chat; SHOW TABLES;'"
echo "mysql -u root -p -e 'USE chat; SELECT * FROM user;'"
echo "mysql -u root -p -e 'USE chat; SELECT * FROM friend;'"
echo "mysql -u root -p -e 'USE chat; SELECT * FROM allgroup;'"
echo "mysql -u root -p -e 'USE chat; SELECT * FROM groupuser;'"
echo "mysql -u root -p -e 'USE chat; SELECT * FROM offlinemessage;'"
echo ""
echo "4. 运行单元测试..."
echo "cd build && make test_mpim_functionality && ./test_mpim_functionality"

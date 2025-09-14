#!/bin/bash

echo "=== MPIM 修复验证脚本 ==="

# 1. 重新编译项目
echo "1. 重新编译项目..."
cd /home/glf/MPIM
mkdir -p build
cd build
cmake ..
make -j4

if [ $? -ne 0 ]; then
    echo "❌ 编译失败"
    exit 1
fi
echo "✅ 编译成功"

# 2. 检查服务状态
echo ""
echo "2. 检查服务状态..."
services=("im-gatewayd:6000" "im-userd:8010" "im-presenced:8012" "im-messaged:8011" "im-groupd:8013")

for service in "${services[@]}"; do
    IFS=':' read -r name port <<< "$service"
    if pgrep -f "$name" > /dev/null; then
        echo "✅ $name 正在运行"
    else
        echo "❌ $name 未运行"
    fi
    
    if nc -z 127.0.0.1 $port 2>/dev/null; then
        echo "✅ 端口 $port 可访问"
    else
        echo "❌ 端口 $port 不可访问"
    fi
done

# 3. 测试数据库连接
echo ""
echo "3. 测试数据库连接..."
mysql -u root -p123456 -e "USE mpchat; SELECT COUNT(*) as user_count FROM user;" 2>/dev/null
if [ $? -eq 0 ]; then
    echo "✅ 数据库连接正常"
else
    echo "❌ 数据库连接失败"
fi

# 4. 创建测试脚本
echo ""
echo "4. 创建自动化测试脚本..."
cat > /tmp/test_mpim_auto.sh << 'EOF'
#!/bin/bash

# 自动化测试脚本
test_client() {
    local test_name="$1"
    local commands="$2"
    
    echo "测试: $test_name"
    
    # 创建命名管道
    local fifo="/tmp/mpim_test_$$"
    mkfifo "$fifo"
    
    # 启动客户端并发送命令
    {
        echo "$commands"
        echo "quit"
    } | timeout 10s ./bin/client 127.0.0.1 6000 > "$fifo" &
    
    local client_pid=$!
    
    # 等待并收集输出
    local output=""
    local count=0
    while [ $count -lt 20 ]; do
        if read -t 1 line < "$fifo" 2>/dev/null; then
            output="$output$line"$'\n'
            echo "  $line"
        fi
        count=$((count + 1))
    done
    
    # 清理
    kill $client_pid 2>/dev/null
    rm -f "$fifo"
    
    # 检查结果
    if echo "$output" | grep -q "+OK"; then
        echo "  ✅ 成功"
    else
        echo "  ❌ 失败"
    fi
    echo ""
}

# 运行测试
cd /home/glf/MPIM

echo "=== 开始自动化测试 ==="
echo ""

test_client "用户注册" "register testuser1 123456"
test_client "用户登录" "login testuser1 123456"
test_client "发送消息" "chat 2 Hello from testuser1"
test_client "拉取消息" "pull"

echo "=== 测试完成 ==="
EOF

chmod +x /tmp/test_mpim_auto.sh

echo "✅ 测试脚本已创建: /tmp/test_mpim_auto.sh"
echo ""
echo "5. 运行测试..."
echo "请手动运行以下命令进行测试："
echo "  /tmp/test_mpim_auto.sh"
echo ""
echo "或者手动测试："
echo "  ./bin/client 127.0.0.1 6000"
echo "  然后输入: register testuser1 123456"
echo "  然后输入: login testuser1 123456"
echo "  然后输入: chat 2 Hello"
echo "  然后输入: pull"
echo "  然后输入: quit"

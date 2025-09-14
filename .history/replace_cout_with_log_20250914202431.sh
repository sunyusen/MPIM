#!/bin/bash

# 批量替换cout为日志的脚本

echo "开始替换cout为日志..."

# 需要处理的文件列表
files=(
    "im-message/src/message_service.cc"
    "im-message/src/offline_model.cc"
    "im-user/src/user_service.cc"
    "im-group/src/group_service.cc"
    "im-gateway/src/gatewayServer.cc"
    "im-client/src/client.cc"
    "im-client/src/commandHandler.cc"
    "mprpc/src/rpcprovider.cc"
    "mprpc/src/mprpcchannel.cc"
    "mprpc/src/zookeeperutil.cc"
)

# 为每个文件添加日志头文件
for file in "${files[@]}"; do
    if [ -f "$file" ]; then
        echo "处理文件: $file"
        
        # 检查是否已经包含logger头文件
        if ! grep -q "logger/logger.h" "$file"; then
            # 在第一个#include后添加logger头文件
            sed -i '/^#include/a #include "logger/logger.h"\n#include "logger/log_init.h"' "$file"
        fi
        
        # 替换cout为LOG_INFO
        sed -i 's/std::cout << /LOG_INFO(/g' "$file"
        sed -i 's/ << std::endl;/);/g' "$file"
        sed -i 's/ << / + /g' "$file"
        
        # 替换cerr为LOG_ERROR
        sed -i 's/std::cerr << /LOG_ERROR(/g' "$file"
        
        # 处理多行cout的情况
        sed -i ':a;N;$!ba;s/LOG_INFO(\([^)]*\)\n[[:space:]]*<< /LOG_INFO(\1 + /g' "$file"
        
        echo "完成: $file"
    else
        echo "文件不存在: $file"
    fi
done

echo "cout替换完成！"

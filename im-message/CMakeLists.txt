# 可执行文件（消息服务进程）
add_executable(im-messaged
  src/main.cc
  src/message_service.cc
  src/offline_model.cc
)

# 头文件搜索路径
target_include_directories(im-messaged PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/src/include          # 本服务头
  ${CMAKE_SOURCE_DIR}/mprpc/src/include            # mprpc 公开头
  ${CMAKE_BINARY_DIR}/mprpc/src                    # mprpc 生成头（如 rpcheader.pb.h）
  ${CMAKE_SOURCE_DIR}/im-common/include            # 公共 proto 头
  ${CMAKE_BINARY_DIR}/im-common                    # proto 生成头
  /usr/include/mysql                               # MySQL 头（如路径不同改这里）
)

# 链接依赖
target_link_libraries(im-messaged PRIVATE
  mprpc            # muduo/zookeeper/protobuf 会经由它传递进来
  im-common        # 公共 proto 的目标
  mysqldb          # 你的 MySQL 封装
  mysqlclient      # 系统 MySQL 客户端库
)

# 产物落到项目根 bin/
set_target_properties(im-messaged PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin"
)
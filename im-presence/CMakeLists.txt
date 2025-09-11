# 业务实现 -> 静态库
add_library(im-presence STATIC
  src/presence_service.cc         # 现在文件在本目录；若你改放到 src/，写成 src/presence_service.cc
)

# 对外暴露头文件目录（给本库的使用者）
target_include_directories(im-presence PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

# 业务库依赖：
# - mprpc     : RpcProvider/Rpc 服务基类
# - im-common : proto产物（PresenceService、Result、Code 等）
# - redisclient : 你的 Redis 薄封装（订阅/投递路由、在线状态）
target_link_libraries(im-presence PUBLIC
  mprpc
  im-common
  redisclient
)

# 进程入口（守护进程）
add_executable(im-presenced
  src/main.cc
)

# main 只需链接业务库即可（mprpc/redis 等通过 im-presence 闭包带过来）
target_link_libraries(im-presenced PRIVATE
  im-presence
)

# 二进制输出到项目根 bin/，好找
set_target_properties(im-presenced PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)

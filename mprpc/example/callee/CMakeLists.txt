# mprpc/example/callee/CMakeLists.txt
add_executable(mprpc_example_callee
  userservice.cc
#   friendsservice.cc
)

target_link_libraries(mprpc_example_callee
  PRIVATE
    mprpc                 # 你的 RPC 框架库
    mprpc_examples_proto  # 示例 proto 生成的库
)

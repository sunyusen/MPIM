add_subdirectory(src)

# 可执行程序（进程入口）
add_executable(im-userd
  src/main.cc
)

# main 需要能包含本模块头
target_include_directories(im-userd PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 进程链接：业务库 + mprpc（RpcProvider/MprpcApplication 在 main 里用）
target_link_libraries(im-userd PRIVATE
  im-user        # 本目录 src 里生成的库
  mprpc          # 你的 RPC 框架
  im-common      # （可选）main 
)

# 把二进制放到项目根的 bin/，方便找
set_target_properties(im-userd PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)
# mprpc/example/CMakeLists.txt
cmake_minimum_required(VERSION 3.16)

# 先生成示例用的 .pb 库，再编可执行
add_subdirectory(proto)
add_subdirectory(callee)
add_subdirectory(caller)

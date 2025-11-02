cmake_minimum_required(VERSION 3.10)
project(LRU_Algorithms)

# 设置C++标准
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 编译选项
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -O2")

# 查找线程库
find_package(Threads REQUIRED)

# 标准LRU
add_executable(standard_lru standard_lru.cc)

# 固定TTL LRU
add_executable(lru_fixed_ttl lru_with_fixed_ttl.cc)
target_link_libraries(lru_fixed_ttl Threads::Threads)

# 可变TTL LRU
add_executable(lru_variable_ttl lru_with_variable_ttl.cc)
target_link_libraries(lru_variable_ttl Threads::Threads)

# 性能测试
add_executable(benchmark benchmark.cc)
target_link_libraries(benchmark Threads::Threads)

# 使用示例
add_executable(usage_examples usage_examples.cc)
target_link_libraries(usage_examples Threads::Threads)

# 安装
install(TARGETS standard_lru lru_fixed_ttl lru_variable_ttl benchmark usage_examples
        RUNTIME DESTINATION bin)


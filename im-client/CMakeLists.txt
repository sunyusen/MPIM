# 头文件
include_directories(${CMAKE_CURRENT_SOURCE_DIR}/src/include)

# 可执行文件
add_executable(client
	src/client.cc
	src/commandHandler.cc
	src/main.cc
)
# 依赖
target_link_libraries(client
	muduo_net
	muduo_base
	pthread
)

# 输出到项目根目录 bin/, 和其他可执行一起
set_target_properties(client PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin
)
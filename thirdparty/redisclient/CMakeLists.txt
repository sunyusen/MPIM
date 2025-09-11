# 1)生成静态库 redisclient(基于hiredis的最小封装：connect/publish/subscrieb)
add_library(redisclient STATIC
	redis.cpp
)

# 2)导出本库的头文件
target_include_directories(redisclient PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/include
)

# 直接按库名链接 hiredis 和 pthread
target_link_libraries(redisclient PUBLIC
	hiredis
	pthread
)
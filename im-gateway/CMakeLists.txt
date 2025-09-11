add_executable(im-gateway
	src/main.cc
	src/gatewayServer.cc
)

# 包含依赖目录
target_include_directories(im-gateway PRIVATE
	${CMAKE_CURRENT_SOURCE_DIR}/src/include   #gatewayServer.h
	${CMAKE_SOURCE_DIR}/mprpc/src/include	  #mprpc公开头文件
	${CMAKE_BINARY_DIR}/mprpc/src    	  #mprpc生成的头文件
	${CMAKE_SOURCE_DIR}/thirdparty/redisclient/include #hireids公开头文件
)

# 链接相关依赖
target_link_libraries(im-gateway PRIVATE
	mprpc
	im-common
	redisclient
	hiredis
)

# 把二进制丢到项目根的 bin/
set_target_properties(im-gateway PROPERTIES
	RUNTIME_OUTPUT_DIRECTORY
	"${CMAKE_SOURCE_DIR}/bin"
)

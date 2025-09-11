# 以前：find_package(Protobuf REQUIRED)
# 现在（兼容你的前缀/可显示探测结果）
find_package(Protobuf REQUIRED)
message(STATUS "Protobuf includes: ${Protobuf_INCLUDE_DIRS}")
message(STATUS "Protobuf libs    : ${Protobuf_LIBRARIES}")
message(STATUS "Protoc           : ${Protobuf_PROTOC_EXECUTABLE}")

set(RPC_PROTO ${CMAKE_CURRENT_SOURCE_DIR}/rpcheader.proto)
protobuf_generate_cpp(RPC_PB_SRCS RPC_PB_HDRS ${RPC_PROTO})

add_library(mprpc STATIC
  logger.cc mprpcapplication.cc mprpcconfig.cc
  mprpccontroller.cc mprpcchannel.cc rpcprovider.cc
  zookeeperutil.cc
  ${RPC_PB_SRCS}
)

include(GNUInstallDirs)	 # 提供 ${CMAKE_INSTALL_INCLUDEDIR}

target_include_directories(mprpc
  PUBLIC
    # 构建时可见：源码头 & 生成的 rpcheader.pb.h
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>

    # 安装后可见：安装到 include/mprpc 和 include/mprpc/proto
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mprpc>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/mprpc/proto>
)

target_link_libraries(mprpc
  PUBLIC
    protobuf::libprotobuf            # ← 绑定到同一套 libprotobuf
    muduo_net
	muduo_base
	pthread
	zookeeper_mt
  PRIVATE
    ${MUDUO_NET_LIB} ${MUDUO_BASE_LIB} ${ZOOKEEPER_LIB} ${RT_LIB}
)

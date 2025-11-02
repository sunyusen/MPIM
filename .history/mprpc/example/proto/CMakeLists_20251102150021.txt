# mprpc/example/proto/CMakeLists.txt
find_package(Protobuf REQUIRED)

# 生成当前目录下所有 .proto
file(GLOB EX_PROTO_FILES ${CMAKE_CURRENT_SOURCE_DIR}/user.proto)
protobuf_generate_cpp(EX_PB_SRCS EX_PB_HDRS ${EX_PROTO_FILES})

# 打成一个示例专用的小库，供 caller/callee 复用
add_library(mprpc_examples_proto STATIC ${EX_PB_SRCS} ${EX_PB_HDRS})

# 暴露生成头所在目录，链接到这个库的目标可直接 #include "user.pb.h" 等
target_include_directories(mprpc_examples_proto
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}>
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}>
)

target_link_libraries(mprpc_examples_proto PUBLIC protobuf::libprotobuf)

# 不做 install()，避免混入发布产物

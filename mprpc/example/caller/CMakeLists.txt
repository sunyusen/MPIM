# mprpc/example/caller/CMakeLists.txt
add_executable(mprpc_example_caller
  calluserservice.cc
#   callfriendsservice.cc
)

target_link_libraries(mprpc_example_caller
  PRIVATE
    mprpc
    mprpc_examples_proto
)

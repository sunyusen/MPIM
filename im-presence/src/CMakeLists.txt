add_library(im-presence STATIC src/presence_service.cc)
target_link_libraries(im-presence PUBLIC mprpc im-common redisclient)

add_executable(im-presenced src/main.cc)
target_link_libraries(im-presenced PRIVATE im-presence)
set_target_properties(im-presenced PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin")

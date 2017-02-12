add_executable (simple simple.c)
target_link_libraries (simple squash${SQUASH_VERSION_API})
target_add_extra_warning_flags (simple)
target_include_directories (simple PRIVATE "${CMAKE_SOURCE_DIR}/squash")

add_executable (stream stream.c)
target_link_libraries (stream squash${SQUASH_VERSION_API})
target_add_extra_warning_flags (stream)
target_include_directories (stream PRIVATE "${CMAKE_SOURCE_DIR}/squash")

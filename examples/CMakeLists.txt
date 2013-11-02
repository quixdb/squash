add_executable (simple simple.c)
target_link_libraries (simple squash${SQUASH_VERSION_API})
add_extra_warning_flags (TARGET simple)

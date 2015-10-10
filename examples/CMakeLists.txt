add_executable (simple simple.c)
target_link_libraries (simple squash${SQUASH_VERSION_API})
target_add_extra_warning_flags (simple)

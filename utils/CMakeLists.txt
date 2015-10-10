add_executable (squash squash.c)
target_add_extra_warning_flags (squash)
target_link_libraries (squash squash${SQUASH_VERSION_API})

install (TARGETS squash
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

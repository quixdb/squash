configure_file (
    "${CMAKE_CURRENT_SOURCE_DIR}/squash.vapi"
    "${CMAKE_CURRENT_BINARY_DIR}/squash-${SQUASH_VERSION_API}.vapi"
    COPYONLY)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squash-${SQUASH_VERSION_API}.vapi
  DESTINATION ${CMAKE_INSTALL_PREFIX}/share/vala/vapi)

add_subdirectory (examples)

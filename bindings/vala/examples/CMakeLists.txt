find_package (Vala)
include (UseVala)
find_package(PkgConfig)

pkg_check_modules(GLIB glib-2.0 gobject-2.0)

set (compress_stream_SOURCES
  compress-stream.vala)

if (VALA_FOUND AND GLIB_FOUND)
  vala_precompile (compress_stream_C_SOURCES
    ${compress_stream_SOURCES}
    OPTIONS --vapidir ${CMAKE_CURRENT_BINARY_DIR}/..
    PACKAGES squash-${SQUASH_VERSION_API})

  add_executable("compress-stream" ${compress_stream_C_SOURCES})
  target_add_compiler_flags ("compress-stream" "-w")
  target_link_libraries ("compress-stream" squash${SQUASH_VERSION_API})
  target_link_libraries ("compress-stream" ${GLIB_LIBRARIES})
  target_include_directories ("compress-stream" PRIVATE ${GLIB_INCLUDE_DIRS})
  target_include_directories ("compress-stream" PRIVATE "${CMAKE_SOURCE_DIR}/squash")
endif ()

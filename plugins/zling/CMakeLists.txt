set (zling_sources
  squash-zling.cpp
  libzling/src/libzling.cpp
  libzling/src/libzling_huffman.cpp
  libzling/src/libzling_lz.cpp
  libzling/src/libzling_utils.cpp)

if ("${ENABLE_ZLING}" STREQUAL "yes")
  squash_plugin_add (zling zling_sources)
  squash_plugin_add_include_directories (zling libzling/src)
endif ()

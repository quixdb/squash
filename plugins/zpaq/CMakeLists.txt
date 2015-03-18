set (zpaq_sources
  squash-zpaq.cpp
  zpaq/libzpaq.cpp)

squash_plugin_add (zpaq zpaq_sources)
squash_plugin_add_include_directories (zpaq zpaq)
squash_source_file_add_cflag ("zpaq/libzpaq.cpp" "-include \"${CMAKE_CURRENT_SOURCE_DIR}/zpaq-config.h\"")

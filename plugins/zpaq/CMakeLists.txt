set (zpaq_sources
  squash-zpaq.cpp
  zpaq/libzpaq.cpp)

squash_plugin_add (zpaq zpaq_sources)
squash_plugin_add_include_directories (zpaq zpaq)
squash_source_file_add_compile_flag_ex ("zpaq/libzpaq.cpp" "-include \"${CMAKE_CURRENT_SOURCE_DIR}/zpaq-config.h\"")

foreach (file ${zpaq_sources})
  squash_source_file_add_compile_flag ("${file}" "-Wno-cast-align")
  squash_source_file_add_compile_flag ("${file}" "-Wno-shift-negative-value")
endforeach ()

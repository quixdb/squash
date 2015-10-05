set (zling_sources
  squash-zling.cpp
  libzling/src/libzling.cpp
  libzling/src/libzling_huffman.cpp
  libzling/src/libzling_lz.cpp
  libzling/src/libzling_utils.cpp)

squash_plugin_add (zling zling_sources)
squash_plugin_add_include_directories (zling libzling/src)

foreach (file ${zling_sources})
  squash_source_file_add_compile_flag ("${file}" "-Wno-undef")
endforeach ()

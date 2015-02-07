set (ms_compress_sources
  squash-ms-compress.c
  ms-compress/compression.cpp
  ms-compress/lznt1.cpp
  ms-compress/lzx.cpp
  ms-compress/xpress.cpp
  ms-compress/xpress_huff.cpp
  ms-compress/Threaded.cpp)

squash_plugin_add (ms-compress ms_compress_sources)
add_definitions (-DCOMPRESSION_API_EXPORT -DPTHREADS)

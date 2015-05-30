set (ms_compress_sources
  squash-ms-compress.c

  ms-compress/src/lznt1_compress.cpp
  ms-compress/src/lznt1_decompress.cpp
  ms-compress/src/mscomp.cpp
  ms-compress/src/xpress_compress.cpp
  ms-compress/src/xpress_decompress.cpp
  ms-compress/src/xpress_huff_compress.cpp
  ms-compress/src/xpress_huff_decompress.cpp)

add_definitions(-DMSCOMP_WITHOUT_UNALIGNED_ACCESS)

squash_plugin_add (ms-compress ms_compress_sources)

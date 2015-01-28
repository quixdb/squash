set (brotli_sources
  squash-brotli.cpp
  brotli/dec/bit_reader.c
  brotli/dec/decode.c
  brotli/dec/huffman.c
  brotli/dec/safe_malloc.c
  brotli/dec/streams.c
  brotli/enc/backward_references.cc
  brotli/enc/block_splitter.cc
  brotli/enc/brotli_bit_stream.cc
  brotli/enc/encode.cc
  brotli/enc/entropy_encode.cc
  brotli/enc/histogram.cc
  brotli/enc/literal_cost.cc
  brotli/tools/bro.cc)

if (NOT "${DISABLE_BROTLI}" STREQUAL "yes")
  squash_plugin_add (brotli brotli_sources)
  squash_plugin_add_include_directories (brotli brotli)

  list (APPEND CMAKE_CXX_FLAGS "-std=c++11")
endif ()

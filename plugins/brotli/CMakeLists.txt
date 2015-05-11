set (brotli_sources
  squash-brotli.cpp
  brotli/dec/bit_reader.c
  brotli/dec/decode.c
  brotli/dec/huffman.c
  brotli/dec/safe_malloc.c
  brotli/dec/state.c
  brotli/dec/streams.c
  brotli/enc/backward_references.cc
  brotli/enc/block_splitter.cc
  brotli/enc/brotli_bit_stream.cc
  brotli/enc/encode.cc
  brotli/enc/encode_parallel.cc
  brotli/enc/entropy_encode.cc
  brotli/enc/histogram.cc
  brotli/enc/literal_cost.cc
  brotli/enc/metablock.cc
  brotli/enc/streams.cc)

if (NOT "${DISABLE_BROTLI}" STREQUAL "yes")
  squash_plugin_add (brotli brotli_sources)
  squash_plugin_add_include_directories (brotli brotli)

  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
  # CMake 3.2
  # set_target_properties("squash${SQUASH_VERSION_API}-plugin-${plugin}" PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES)
endif ()

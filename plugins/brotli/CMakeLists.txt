include (SquashPlugin)

squash_plugin (
  NAME brotli
  SOURCES squash-brotli.cpp
  CXX_STANDARD c++11
  EMBED_SOURCES
    brotli/dec/bit_reader.c
    brotli/dec/decode.c
    brotli/dec/huffman.c
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
    brotli/enc/static_dict.cc
    brotli/enc/streams.cc
  COMPILER_FLAGS
    -Wno-cast-align
    -Wno-sign-compare
  INCLUDE_DIRS brotli)

# if (CMAKE_HOST_APPLE)
#   set_property (TARGET squash${SQUASH_VERSION_API}-plugin-brotli APPEND_STRING PROPERTY COMPILE_FLAGS "-stdlib=libc++")
# endif ()

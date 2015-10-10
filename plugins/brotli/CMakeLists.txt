include (SquashPlugin)

squash_plugin (
  NAME brotli
  SOURCES squash-brotli.cpp
  STANDARD c++11
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

# if (NOT "${DISABLE_BROTLI}" STREQUAL "yes")
#   squash_plugin_add (brotli brotli_sources)
#   squash_plugin_add_include_directories (brotli brotli)

#   set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
#   # CMake 3.2
#   # set_target_properties("squash${SQUASH_VERSION_API}-plugin-${plugin}" PROPERTIES CXX_STANDARD 11 CXX_STANDARD_REQUIRED YES)

#   foreach (file ${brotli_sources})
#     squash_source_file_add_compile_flag ("${file}" "-Wno-shadow")
#     squash_source_file_add_compile_flag ("${file}" "-Wno-sign-compare")
#     squash_source_file_add_compile_flag ("${file}" "-Wno-cast-align")
#   endforeach ()

#   squash_source_file_add_compile_flag ("brotli/enc/static_dict.cc" "-Wno-tautological-constant-out-of-range-compare")
# endif ()

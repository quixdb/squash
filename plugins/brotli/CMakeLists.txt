include (SquashPlugin)

set (BROTLI_PLATFORM_DEFINES)

if (${CMAKE_SYSTEM_NAME} MATCHES "Linux")
  list (APPEND BROTLI_PLATFORM_DEFINES "OS_LINUX")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "FreeBSD")
  list (APPEND BROTLI_PLATFORM_DEFINES "OS_FREEBSD")
elseif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  list (APPEND BROTLI_PLATFORM_DEFINES "OS_MACOSX")
endif ()

set_compiler_specific_flags(
  VARIABLE embed_compiler_flags
  MSVC
    /wd4244
    /wd4018
    /wd4334)

squash_plugin (
  NAME brotli
  SOURCES squash-brotli.cpp
  CXX_STANDARD c++11
  EMBED_SOURCES
    brotli/dec/bit_reader.c
    brotli/dec/decode.c
    brotli/dec/dictionary.c
    brotli/dec/huffman.c
    brotli/dec/state.c
    brotli/enc/backward_references.cc
    brotli/enc/block_splitter.cc
    brotli/enc/brotli_bit_stream.cc
    brotli/enc/compress_fragment.cc
    brotli/enc/compress_fragment_two_pass.cc
    # brotli/enc/dictionary.cc
    brotli/enc/encode.cc
    brotli/enc/encode_parallel.cc
    brotli/enc/entropy_encode.cc
    brotli/enc/histogram.cc
    brotli/enc/literal_cost.cc
    brotli/enc/metablock.cc
    brotli/enc/static_dict.cc
    brotli/enc/streams.cc
    brotli/enc/utf8_util.cc
  COMPILER_FLAGS
    -Wno-cast-align
    -Wno-sign-compare
  EMBED_COMPILER_FLAGS
    ${embed_compiler_flags}
  INCLUDE_DIRS brotli
  EMBED_DEFINES
    ${BROTLI_PLATFORM_DEFINES}
  NO_UNDEFINED_DEFINES
    BROTLI_BUILD_PORTABLE)

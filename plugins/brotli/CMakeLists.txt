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
  SOURCES squash-brotli.c
  EMBED_SOURCES
    brotli/enc/backward_references.c
    brotli/enc/bit_cost.c
    brotli/enc/block_splitter.c
    brotli/enc/brotli_bit_stream.c
    brotli/enc/cluster.c
    brotli/enc/compress_fragment.c
    brotli/enc/compress_fragment_two_pass.c
    brotli/enc/encode.c
    brotli/enc/entropy_encode.c
    brotli/enc/histogram.c
    brotli/enc/literal_cost.c
    brotli/enc/memory.c
    brotli/enc/metablock.c
    brotli/enc/static_dict.c
    brotli/enc/utf8_util.c
    brotli/enc/compressor.cc
    brotli/enc/encode_parallel.cc
    brotli/enc/streams.cc
    brotli/dec/bit_reader.c
    brotli/dec/decode.c
    brotli/dec/huffman.c
    brotli/dec/state.c
    brotli/common/dictionary.c
  # COMPILER_FLAGS
  #   -Wno-cast-align
  #   -Wno-sign-compare
  # EMBED_COMPILER_FLAGS
  #   ${embed_compiler_flags}
  INCLUDE_DIRS brotli
  EMBED_DEFINES
    ${BROTLI_PLATFORM_DEFINES}
  NO_UNDEFINED_DEFINES
    BROTLI_BUILD_PORTABLE)

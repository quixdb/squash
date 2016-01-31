include (SquashPlugin)

if (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")
  set_compiler_specific_flags(
    VARIABLE no_lto
    GCC -fno-lto)
endif (${CMAKE_SYSTEM_NAME} MATCHES "Darwin")

squash_plugin (
  NAME libdeflate
  SOURCES squash-libdeflate.c
  C_STANDARD c99
  EMBED_SOURCES
    libdeflate/src/adler32.c
    libdeflate/src/aligned_malloc.c
    libdeflate/src/crc32.c
    libdeflate/src/deflate_compress.c
    libdeflate/src/deflate_decompress.c
    libdeflate/src/gzip_compress.c
    libdeflate/src/gzip_decompress.c
    libdeflate/src/x86_cpu_features.c
    libdeflate/src/zlib_compress.c
    libdeflate/src/zlib_decompress.c
  DEFINES
    _ANSI_SOURCE
    SUPPORT_NEAR_OPTIMAL_PARSING=1
  COMPILER_FLAGS
    ${no_lto}
  INCLUDE_DIRS libdeflate)

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
    libdeflate/lib/adler32.c
    libdeflate/lib/aligned_malloc.c
    libdeflate/lib/crc32.c
    libdeflate/lib/deflate_compress.c
    libdeflate/lib/deflate_decompress.c
    libdeflate/lib/gzip_compress.c
    libdeflate/lib/gzip_decompress.c
    libdeflate/lib/x86_cpu_features.c
    libdeflate/lib/zlib_compress.c
    libdeflate/lib/zlib_decompress.c
  DEFINES
    _ANSI_SOURCE
    SUPPORT_NEAR_OPTIMAL_PARSING=1
  COMPILER_FLAGS
    ${no_lto}
  INCLUDE_DIRS
    libdeflate
    libdeflate/common
    libdeflate/lib)

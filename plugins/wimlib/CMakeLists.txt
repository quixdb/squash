include (SquashPlugin)

squash_plugin (
  NAME wimlib
  SOURCES squash-wimlib.c
  EMBED_SOURCES
    wimlib/src/compress.c
    wimlib/src/compress_common.c
    wimlib/src/compress_parallel.c
    wimlib/src/compress_serial.c
    wimlib/src/decompress.c
    wimlib/src/decompress_common.c
    wimlib/src/divsufsort.c
    wimlib/src/lcpit_matchfinder.c
    wimlib/src/lzms_common.c
    wimlib/src/lzms_compress.c
    wimlib/src/lzms_decompress.c
    wimlib/src/lzx_common.c
    wimlib/src/lzx_compress.c
    wimlib/src/lzx_decompress.c
    wimlib/src/util.c
    wimlib/src/xpress_compress.c
    wimlib/src/xpress_decompress.c
    wimlib/src/x86_cpu_features.c
  INCLUDE_DIRS wimlib/include)

include (SquashPlugin)

squash_plugin (
  NAME ms-compress
  SOURCES squash-ms-compress.c
  EMBED_SOURCES
    ms-compress/src/lznt1_compress.cpp
    ms-compress/src/lznt1_decompress.cpp
    ms-compress/src/mscomp.cpp
    ms-compress/src/xpress_compress.cpp
    ms-compress/src/xpress_decompress.cpp
    ms-compress/src/xpress_huff_compress.cpp
    ms-compress/src/xpress_huff_decompress.cpp
  EMBED_DEFINES
    MSCOMP_WITHOUT_UNALIGNED_ACCESS)

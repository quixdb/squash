include (SquashPlugin)

squash_plugin (
  NAME zstd
  SOURCES squash-zstd.c
  EXTERNAL_PKG libzstd
  EMBED_SOURCES
    zstd/lib/legacy/zstd_v01.c
    zstd/lib/legacy/zstd_v02.c
    zstd/lib/legacy/zstd_v03.c
    zstd/lib/legacy/zstd_v04.c
    zstd/lib/legacy/zstd_v05.c
    zstd/lib/legacy/zstd_v06.c
    zstd/lib/legacy/zstd_v07.c
    zstd/lib/common/entropy_common.c
    zstd/lib/common/fse_decompress.c
    zstd/lib/common/xxhash.c
    zstd/lib/common/zstd_common.c
    zstd/lib/compress/fse_compress.c
    zstd/lib/compress/huf_compress.c
    zstd/lib/compress/zbuff_compress.c
    zstd/lib/compress/zstd_compress.c
    zstd/lib/decompress/huf_decompress.c
    zstd/lib/decompress/zbuff_decompress.c
    zstd/lib/decompress/zstd_decompress.c
    zstd/lib/dictBuilder/divsufsort.c
    zstd/lib/dictBuilder/zdict.c
  EMBED_DEFINES
    ZSTD_LEGACY_SUPPORT=1
    SQUASH_ZSTD_EMBED
  EMBED_INCLUDE_DIRS
    zstd/lib
    zstd/lib/common
    zstd/lib/legacy)

include (SquashPlugin)

squash_plugin (
  NAME zstd
  SOURCES squash-zstd.c
  EMBED_SOURCES
    zstd/lib/legacy/zstd_v01.c
    zstd/lib/legacy/zstd_v02.c
    zstd/lib/legacy/zstd_v03.c
    zstd/lib/legacy/zstd_v04.c
    zstd/lib/fse.c
    zstd/lib/huff0.c
    zstd/lib/divsufsort.c
    zstd/lib/zbuff.c
    zstd/lib/zdict.c
    zstd/lib/zstd_compress.c
    zstd/lib/zstd_decompress.c
  EMBED_INCLUDE_DIRS
    zstd/lib
    zstd/lib/legacy)

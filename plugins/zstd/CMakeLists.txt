include (SquashPlugin)

squash_plugin (
  NAME zstd
  SOURCES squash-zstd.c
  EMBED_SOURCES
    zstd/lib/fse.c
    zstd/lib/huff0.c
    zstd/lib/zstd_buffered.c
    zstd/lib/zstd_compress.c
    zstd/lib/zstd_decompress.c
    zstd/lib/legacy/zstd_v01.c
    zstd/lib/legacy/zstd_v02.c
    zstd/lib/legacy/zstd_v03.c
  EMBED_INCLUDE_DIRS
    zstd/lib
    zstd/lib/legacy)

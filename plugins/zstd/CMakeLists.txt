set (zstd_sources
  squash-zstd.c
  zstd/lib/zstd.c)

squash_plugin_add (zstd zstd_sources)

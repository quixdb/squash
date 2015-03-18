set (lz4_sources
  squash-lz4.c
  squash-lz4f.c
  lz4/lib/lz4.c
  lz4/lib/lz4hc.c
  lz4/lib/lz4frame.c
  lz4/lib/xxhash.c)

squash_plugin_add (lz4 lz4_sources)
squash_plugin_add_include_directories (lz4 lz4/lib)

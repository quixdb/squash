set (snappy_sources
  squash-snappy.c
  crc32.c)

squash_plugin_add_for_package(snappy Snappy snappy_sources)

set (quicklz_sources
  squash-quicklz.c
  quicklz.c)

squash_plugin_add (quicklz quicklz_sources)
add_definitions (-DQLZ_MEMORY_SAFE)

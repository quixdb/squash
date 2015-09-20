set (brieflz_sources
  squash-brieflz.c
  brieflz/brieflz.c
  brieflz/depack.c)

squash_plugin_add (brieflz brieflz_sources)
squash_plugin_add_include_directories (brieflz brieflz)

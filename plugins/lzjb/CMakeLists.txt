set (lzjb_sources
  squash-lzjb.c
  lzjb/lzjb.c)

squash_plugin_add (lzjb lzjb_sources)
squash_plugin_add_include_directories (lzjb lzjb)

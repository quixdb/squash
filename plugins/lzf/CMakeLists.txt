set (lzf_sources
  squash-lzf.c
  liblzf/lzf_c.c
  liblzf/lzf_d.c)

squash_plugin_add (lzf lzf_sources)
squash_plugin_add_include_directories (lzf liblzf)

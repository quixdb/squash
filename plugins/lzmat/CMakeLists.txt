set (lzmat_sources
  squash-lzmat.c
  lzmat/lzmat_dec.c
  lzmat/lzmat_enc.c)

squash_plugin_add (lzmat lzmat_sources)
squash_plugin_add_include_directories (lzmat lzmat)

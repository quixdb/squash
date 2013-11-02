set (lzmat_sources
  squash-lzmat.c
  lzmat/lzmat_dec.c
  lzmat/lzmat_enc.c)

squash_plugin_add (lzmat lzmat_sources)
squash_plugin_add_include_directories (lzmat lzmat)
squash_target_add_cflag (squash${SQUASH_VERSION_API}-plugin-lzmat "-Wno-pointer-to-int-cast")

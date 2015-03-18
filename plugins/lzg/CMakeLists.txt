set (lzg_sources
  squash-lzg.c
  liblzg/src/lib/checksum.c
  liblzg/src/lib/decode.c
  liblzg/src/lib/encode.c
  liblzg/src/lib/version.c)

squash_plugin_add (lzg lzg_sources)

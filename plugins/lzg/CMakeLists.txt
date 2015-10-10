include (SquashPlugin)

squash_plugin (
  NAME lzg
  SOURCES squash-lzg.c
  EMBED_SOURCES
    liblzg/src/lib/checksum.c
    liblzg/src/lib/decode.c
    liblzg/src/lib/encode.c
    liblzg/src/lib/version.c)

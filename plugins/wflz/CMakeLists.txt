include (SquashPlugin)

squash_plugin (
  NAME wflz
  SOURCES squash-wflz.c
  EMBED_SOURCES wflz/wfLZ.c
  EMBED_DEFINES WF_LZ_NO_UNALIGNED_ACCESS
  COMPILER_FLAGS
    -Wno-strict-prototypes)

include (SquashPlugin)

squash_plugin (
  NAME wflz
  NO_BIG_ENDIAN
  SOURCES squash-wflz.c
  EMBED_SOURCES wflz/wfLZ.c
  NO_UNDEFINED_DEFINES
    WF_LZ_NO_UNALIGNED_ACCESS
  COMPILER_FLAGS
    -Wno-strict-prototypes
    -Wno-undef)

include (SquashPlugin)

squash_plugin (
  NAME lzf
  SOURCES squash-lzf.c
  EMBED_SOURCES
    liblzf/lzf_c.c
    liblzf/lzf_d.c
  INCLUDE_DIRS
    liblzf
  COMPILER_FLAGS
    -Wno-undef
    -Wno-missing-prototypes
    -Wno-shadow
    -Wno-missing-declarations
    -Wno-pointer-to-int-cast)

include (SquashPlugin)

set_compiler_specific_flags(
  VARIABLE compiler_flags
  GCCISH
    -Wno-undef
    -Wno-missing-prototypes
    -Wno-shadow
    -Wno-missing-declarations
    -Wno-pointer-to-int-cast
  ICC -wd3656
  MSVC
    /wd4090
    /wd4244)

squash_plugin (
  NAME lzf
  SOURCES squash-lzf.c
  EMBED_SOURCES
    liblzf/lzf_c.c
    liblzf/lzf_d.c
  INCLUDE_DIRS
    liblzf
  COMPILER_FLAGS
    ${compiler_flags})

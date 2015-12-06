include (SquashPlugin)

squash_plugin (
  NAME miniz
  SOURCES squash-miniz.c
  EXTERNAL_PKG_PREFIX ZLIB
  INCLUDE_DIRS
    miniz
  COMPILER_FLAGS
    -Wno-unused-function
    -Wno-cast-align
    -Wno-undef)

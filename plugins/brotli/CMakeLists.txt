include (SquashPlugin)

set(BROTLI_BUNDLED_MODE ON)

SQUASH_PLUGIN(
  SOURCES squash-brotli.c
  EXTERNAL_PKG "libbrotlicommon libbrotlidec libbrotlienc"
  NAME brotli
  C_STANDARD c99
  EMBED_SUBDIR brotli
  EMBED_TARGET_VAR BROTLI_LIBRARIES
  EMBED_INCLUDE_DIRS_VAR BROTLI_INCLUDE_DIRS)

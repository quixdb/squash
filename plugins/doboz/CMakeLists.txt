include (SquashPlugin)

squash_plugin(
  NAME doboz
  DEFAULT_DISABLED
  SOURCES squash-doboz.cpp
  EMBED_SOURCES
    doboz/Source/Doboz/Compressor.cpp
    doboz/Source/Doboz/Decompressor.cpp
    doboz/Source/Doboz/Dictionary.cpp
  DEFINES
    __STDC_LIMIT_MACROS)

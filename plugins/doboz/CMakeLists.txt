set (doboz_sources
  squash-doboz.cpp
  doboz/Source/Doboz/Compressor.cpp
  doboz/Source/Doboz/Decompressor.cpp
  doboz/Source/Doboz/Dictionary.cpp)

add_definitions(-D__STDC_LIMIT_MACROS)
squash_plugin_add (doboz doboz_sources)

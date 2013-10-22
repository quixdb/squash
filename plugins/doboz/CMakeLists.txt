set (doboz_sources
  squash-doboz.cpp
  doboz/Source/Doboz/Compressor.cpp
  doboz/Source/Doboz/Decompressor.cpp
  doboz/Source/Doboz/Dictionary.cpp)

squash_plugin_add (doboz doboz_sources)

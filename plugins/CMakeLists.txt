set (plugins_available
  blosc
  brotli
  bzip2
  crush
  doboz
  fari
  fastlz
  gipfeli
  ipp
  lz4
  lzf
  lzg
  lzham
  lzjb
  lzma
  lzmat
  lzo
  ms-compress
  quicklz
  sharc
  snappy
  wflz
  zlib
  zling
  zpaq
  zstd)

foreach (plugin ${plugins_available})
  string(TOUPPER ${plugin} plugin_uc_name)
  if (NOT "${ENABLE_${plugin_uc_name}}" STREQUAL "no")
    add_subdirectory (${plugin})
  endif()
endforeach(plugin)

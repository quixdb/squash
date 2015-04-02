set (plugins_available
  brotli
  bsc
  bzip2
  copy
  crush
  density
  doboz
  fari
  fastlz
  gipfeli
  lz4
  lzf
  lzg
  lzham
  lzjb
  lzma
  lzo
  ms-compress
  pithy
  quicklz
  snappy
  wflz
  yalz77
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

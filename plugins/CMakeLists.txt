set (plugins_available
  blosc
  bzip2
  doboz
  fari
  fastlz
  ipp
  lz4
  lzf
  lzg
  lzjb
  lzma
  lzmat
  lzo
  quicklz
  sharc
  snappy
  wflz
  zlib
  zpaq)

foreach (plugin ${plugins_available})
  string(TOUPPER ${plugin} plugin_uc_name)
  if (NOT "${ENABLE_${plugin_uc_name}}" STREQUAL "no")
    add_subdirectory (${plugin})
  endif()
endforeach(plugin)


set (zlibng_sources squash-zlib-ng.c)

set (libzlibng_sources
  zlib-ng/adler32.c
  zlib-ng/compress.c
  zlib-ng/crc32.c
  zlib-ng/deflate.c
  zlib-ng/inflate.c
  zlib-ng/infback.c
  zlib-ng/inftrees.c
  zlib-ng/inffast.c
  zlib-ng/match.c
  zlib-ng/trees.c
  zlib-ng/uncompr.c
  zlib-ng/zutil.c)
list (APPEND zlibng_sources ${libzlibng_sources})
squash_plugin_add (zlib-ng zlibng_sources)
squash_plugin_add_include_directories (zlib-ng zlib-ng)

foreach (file ${libzlib_sources})
  squash_source_file_add_compile_flag ("${file}" "-Wno-implicit-function-declaration")
endforeach ()

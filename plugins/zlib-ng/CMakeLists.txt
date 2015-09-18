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

# Begin copied (and slightly modified) from zlib-ng

check_include_file(unistd.h Z_HAVE_UNISTD_H)

macro(generate_cmakein input output)
    execute_process(COMMAND sed "/#define ZCONF_H/ a\\\n#cmakedefine Z_HAVE_UNISTD_H\n" -
                    INPUT_FILE ${input}
                    OUTPUT_FILE ${output}
)

endmacro(generate_cmakein)

generate_cmakein( ${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng/zconf.h.in ${CMAKE_CURRENT_BINARY_DIR}/zconf.h.cmakein )

configure_file(	${CMAKE_CURRENT_BINARY_DIR}/zconf.h.cmakein
		${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)
include_directories(${CMAKE_CURRENT_BINARY_DIR} ${CMAKE_SOURCE_DIR})

## End copied from zlib-ng

squash_plugin_add (zlib-ng zlibng_sources)
squash_plugin_add_include_directories (zlib-ng zlib-ng)

foreach (file ${libzlib_sources})
  squash_source_file_add_compile_flag ("${file}" "-Wno-implicit-function-declaration")
endforeach ()

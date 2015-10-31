# Begin copied (and slightly modified) from zlib-ng

check_include_file(unistd.h Z_HAVE_UNISTD_H)

find_program (SED sed DOC "sed executable")

macro(generate_cmakein input output)
    execute_process(COMMAND ${SED} "/#define ZCONF_H/ a\\\n#cmakedefine Z_HAVE_UNISTD_H\n" ${input}
                    OUTPUT_FILE ${output}
)

endmacro(generate_cmakein)

generate_cmakein( ${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng/zconf.h.in ${CMAKE_CURRENT_BINARY_DIR}/zconf.h.cmakein )

configure_file(	${CMAKE_CURRENT_BINARY_DIR}/zconf.h.cmakein
		${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)

## End copied from zlib-ng

include (SquashPlugin)

squash_plugin(
  NAME zlib-ng
  SOURCES squash-zlib-ng.c
  EMBED_SOURCES
    zlib-ng/adler32.c
    zlib-ng/compress.c
    zlib-ng/crc32.c
    zlib-ng/deflate.c
    zlib-ng/deflate_fast.c
    zlib-ng/deflate_medium.c
    zlib-ng/deflate_slow.c
    zlib-ng/gzclose.c
    zlib-ng/gzlib.c
    zlib-ng/gzread.c
    zlib-ng/gzwrite.c
    zlib-ng/infback.c
    zlib-ng/inffast.c
    zlib-ng/inflate.c
    zlib-ng/inftrees.c
    zlib-ng/match.c
    zlib-ng/trees.c
    zlib-ng/uncompr.c
    zlib-ng/zutil.c
  INCLUDE_DIRS
    ${CMAKE_CURRENT_BINARY_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng
  EMBED_COMPILER_FLAGS
    -Wno-shift-negative-value)
list (APPEND zlibng_sources ${libzlibng_sources})

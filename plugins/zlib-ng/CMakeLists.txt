# Begin copied (and slightly modified) from zlib-ng

check_include_file(unistd.h Z_HAVE_UNISTD_H)

function(generate_cmakein input output)
  file(READ "${input}" input_data)
  string(REPLACE "#define ZCONF_H" "#define ZCONF_H\n#cmakedefine Z_HAVE_UNISTD_H" output_data "${input_data}")
  file(WRITE "${output}" "${output_data}")
endfunction(generate_cmakein)

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

if (ENABLE_ZLIB_NG)
  generate_cmakein("${CMAKE_CURRENT_SOURCE_DIR}/zlib-ng/zconf.h.in" "${CMAKE_CURRENT_BINARY_DIR}/zconf.h.cmakein")

  configure_file(	${CMAKE_CURRENT_BINARY_DIR}/zconf.h.cmakein
		  ${CMAKE_CURRENT_BINARY_DIR}/zconf.h @ONLY)
endif ()

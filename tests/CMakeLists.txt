enable_testing ()

set (codec_tests
  buffer
  bounds
  file
  flush
  random-data
  splice
  stream-compress
  stream-decompress
  stream-single-byte
  single-byte
  threads)

find_package(PkgConfig)

pkg_check_modules(GLIB glib-2.0 gobject-2.0)

function (squash_target_add_glib target)
  foreach (glib_ldflag ${GLIB_LIBRARY_DIRS})
    target_link_libraries (${target} "-L${glib_ldflag}")
  endforeach (glib_ldflag)

  target_link_libraries (${target} ${GLIB_LDFLAGS})

  target_include_directories (${target} PRIVATE ${GLIB_INCLUDE_DIRS})
endfunction (squash_target_add_glib)

if (GLIB_FOUND)
  foreach (test ${codec_tests})
    add_executable(${test} ${test}.c "test-codecs.c")
    target_link_libraries (${test} squash${SQUASH_VERSION_API})
    squash_target_add_glib (${test})
    target_add_extra_warning_flags (${test})
    add_test (${test} ${test} --squash-plugins=${PROJECT_BINARY_DIR}/plugins)
  endforeach (test)
endif (GLIB_FOUND)

set (codec_tests
  buffer
  random-data
  stream-compress
  stream-decompress
  single-byte
  threads)

find_package(PkgConfig)

pkg_check_modules(GLIB glib-2.0 gobject-2.0)

if (GLIB_FOUND)
  foreach (test ${codec_tests})
    add_executable(${test} "test-codecs" ${test}.c)

    target_link_libraries (${test} squash${SQUASH_VERSION_API})
    target_link_libraries (${test} ${GLIB_LIBRARIES})
    target_include_directories (${test} PRIVATE ${GLIB_INCLUDE_DIRS})

    add_test (${test} ${test} --squash-plugins=${PROJECT_BINARY_DIR}/plugins)
  endforeach (test)
endif (GLIB_FOUND)

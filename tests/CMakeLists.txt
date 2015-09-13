set (codec_tests
  buffer
  bounds
  flush
  random-data
  stream-compress
  stream-decompress
  stream-single-byte
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
    add_extra_warning_cflags (TARGET ${test})

    add_test (${test} ${test} --squash-plugins=${PROJECT_BINARY_DIR}/plugins)

    squash_source_file_add_compile_flag ("${test}.c" "-Wno-unknown-attributes")
  endforeach (test)
  squash_source_file_add_compile_flag ("test-codecs.c" "-Wno-unknown-attributes")

  add_executable ("file" "file.c")
  target_link_libraries ("file" squash${SQUASH_VERSION_API})
  target_link_libraries ("file" ${GLIB_LIBRARIES})
  target_include_directories ("file" PRIVATE ${GLIB_INCLUDE_DIRS})
  add_extra_warning_cflags (TARGET "file")
  add_test ("file" "file" --squash-plugins=${PROJECT_BINARY_DIR}/plugins)
endif (GLIB_FOUND)

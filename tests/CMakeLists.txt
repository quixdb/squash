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

function (squash_target_add_glib target)
  # Fuck CMake.  Not only is it a pain in the ass to add the relevant
  # flags (you have to use a loop to iterate through each one and
  # append it individually), but you can't just append all the ldflags
  # at once becase the pkg_LDFLAGS variable changes the order it gets
  # from pkg-config.  It actually moves the -L... flag to before one
  # of the libraries.
  #
  # This code will first add all the library dirs, then it will add
  # all flags (including the library ones).  Hopefully this is safe.

  foreach (glib_ldflag ${GLIB_LIBRARY_DIRS})
    set_property (TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " -L${glib_ldflag}")
  endforeach (glib_ldflag)

  foreach (glib_ldflag ${GLIB_LDFLAGS})
    set_property (TARGET ${target} APPEND_STRING PROPERTY LINK_FLAGS " ${glib_ldflag}")
  endforeach (glib_ldflag)
endfunction (squash_target_add_glib)

if (GLIB_FOUND)
  foreach (test ${codec_tests})
    add_executable(${test} "test-codecs" ${test}.c)

    target_link_libraries (${test} squash${SQUASH_VERSION_API})
    squash_target_add_glib (${test})

    target_include_directories (${test} PRIVATE ${GLIB_INCLUDE_DIRS})
    add_extra_warning_cflags (TARGET ${test})

    add_test (${test} ${test} --squash-plugins=${PROJECT_BINARY_DIR}/plugins)

    squash_source_file_add_compile_flag ("${test}.c" "-Wno-unknown-attributes")
  endforeach (test)

  squash_source_file_add_compile_flag ("test-codecs.c" "-Wno-unknown-attributes")

  add_executable ("file" "file.c")
  target_link_libraries ("file" squash${SQUASH_VERSION_API})
  squash_target_add_glib ("file")
  target_include_directories ("file" PRIVATE ${GLIB_INCLUDE_DIRS})
  add_extra_warning_cflags (TARGET "file")
  add_test ("file" "file" --squash-plugins=${PROJECT_BINARY_DIR}/plugins)
endif (GLIB_FOUND)

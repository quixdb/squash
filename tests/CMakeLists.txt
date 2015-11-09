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

if (MSVC)
  set (GLIB_DIR "${PROJECT_SOURCE_DIR}/glib/build/native")
  set (GLIB_DLL_DIR "${PROJECT_SOURCE_DIR}/glib.redist/build/native")
  set (LIBINTL_DLL_DIR "${PROJECT_SOURCE_DIR}/libintl.redist/build/native")

  # Find Visual Studio version number of MSVC
  if (MSVC14)
    set (MSVC_VER "v140")
  elseif (MSVC12)
    set (MSVC_VER "v120")
  elseif (MSVC11)
    set (MSVC_VER "v110")
  elseif (MSVC10)
    set (MSVC_VER "v100")
  else ()
    message (WARNING "Unable to determine MSVC version, defaulting to v120.")
    set (MSVC_VER "v120")
  endif ()

  # If no glib for this MSVC, choose one that is available
  if (NOT EXISTS "${GLIB_DIR}/lib/${MSVC_VER}/")
    if (EXISTS "${GLIB_DIR}/lib/v140/")
      set (MSVC_VER "v140")
    elseif (EXISTS "${GLIB_DIR}/lib/v120/")
      set (MSVC_VER "v120")
    elseif (EXISTS "${GLIB_DIR}/lib/v110/")
      set (MSVC_VER "v110")
    elseif (EXISTS "${GLIB_DIR}/lib/v100/")
      set (MSVC_VER "v100")
    endif ()
  endif ()

  if (CMAKE_CL_64)
    set (MSVC_ARCH "x64")
  else ()
    set (MSVC_ARCH "Win32")
  endif ()

  if (CMAKE_BUILD_TYPE)
    set (MSVC_BUILD "${CMAKE_BUILD_TYPE}")
  else ()
    set (MSVC_BUILD "Debug")
  endif ()

  if (GLIB_STATIC)
    set (MSVC_LINKAGE "static")
  else ()
    set (MSVC_LINKAGE "dynamic")
  endif ()

  if (EXISTS "${GLIB_DIR}/lib/${MSVC_VER}/${MSVC_ARCH}/${MSVC_BUILD}/${MSVC_LINKAGE}/glib-2.0.lib")
    set (GLIB_INCLUDE_DIRS
      "${GLIB_DIR}/include"
      "${GLIB_DIR}/include/glib"
    )
    set (GLIB_LDFLAGS
      "${GLIB_DIR}/lib/${MSVC_VER}/${MSVC_ARCH}/${MSVC_BUILD}/${MSVC_LINKAGE}/glib-2.0.lib"
      "${GLIB_DIR}/lib/${MSVC_VER}/${MSVC_ARCH}/${MSVC_BUILD}/${MSVC_LINKAGE}/gobject-2.0.lib"
    )
    set (GLIB_DLLS
      "${GLIB_DLL_DIR}/bin/${MSVC_VER}/${MSVC_ARCH}/${MSVC_BUILD}/${MSVC_LINKAGE}/glib-2-vs10.dll"
      "${GLIB_DLL_DIR}/bin/${MSVC_VER}/${MSVC_ARCH}/${MSVC_BUILD}/${MSVC_LINKAGE}/gobject-2-vs10.dll"
      "${LIBINTL_DLL_DIR}/bin/${MSVC_ARCH}/${MSVC_VER}/${MSVC_LINKAGE}/libintl.dll"
    )
    set (GLIB_FOUND "YES")
  else ()
      message (STATUS "Could NOT find glib")
  endif ()

  unset (GLIB_DIR)
  unset (GLIB_DLL_DIR)
  unset (LIBINTL_DLL_DIR)
  unset (MSVC_VER)
  unset (MSVC_ARCH)
  unset (MSVC_BUILD)
  unset (MSVC_LINKAGE)
else ()
  find_package(PkgConfig)
  pkg_check_modules(GLIB glib-2.0 gobject-2.0)
endif ()

function (squash_target_add_glib target)
  foreach (glib_ldflag ${GLIB_LIBRARY_DIRS})
    target_link_libraries (${target} "-L${glib_ldflag}")
  endforeach (glib_ldflag)

  target_link_libraries (${target} ${GLIB_LDFLAGS})

  target_include_directories (${target} PRIVATE ${GLIB_INCLUDE_DIRS})
endfunction (squash_target_add_glib)

if (NOT DISABLE_UNIT_TESTS STREQUAL "yes")
  if (GLIB_FOUND)
    foreach (test ${codec_tests})
      add_executable(${test} ${test}.c "test-codecs.c")
      target_link_libraries (${test} squash${SQUASH_VERSION_API})
      squash_target_add_glib (${test})
      source_file_add_extra_warning_flags (${test}.c)
      if (MSVC)
        foreach (dll ${GLIB_DLLS})
          add_custom_command(TARGET ${test} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${dll}" "$<TARGET_FILE_DIR:${test}>")
        endforeach (dll)
        add_custom_command(TARGET ${test} POST_BUILD
          COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:squash${SQUASH_VERSION_API}>" "$<TARGET_FILE_DIR:${test}>")
      endif ()
      add_test (${test} ${test} --squash-plugins=${PROJECT_BINARY_DIR}/plugins)
    endforeach (test)
    source_file_add_extra_warning_flags ("test-codecs.c")
  endif (GLIB_FOUND)
endif ()

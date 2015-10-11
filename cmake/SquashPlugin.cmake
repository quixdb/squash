include (AddCompilerFlags)
include (RequireStandard)

# set (SQUASH_ENABLED_PLUGINS "" CACHE INTERNAL "enabled plugins")

function (SQUASH_PLUGIN)
  set (options EXTRA_WARNINGS DEFAULT_DISABLED)
  set (oneValueArgs NAME EXTERNAL_PKG EXTERNAL_PKG_PREFIX C_STANDARD CXX_STANDARD)
  set (multiValueArgs SOURCES EMBED_SOURCES LIBRARIES LDFLAGS COMPILER_FLAGS EMBED_COMPILER_FLAGS INCLUDE_DIRS EMBED_INCLUDE_DIRS DEFINES EMBED_DEFINES)
  cmake_parse_arguments(SQUASH_PLUGIN "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  unset (options)
  unset (oneValueArgs)
  unset (multiValueArgs)

  string (TOUPPER "${SQUASH_PLUGIN_NAME}" PLUGIN_NAME_UC)
  set (ENABLE_PLUGIN "")
  string (REGEX REPLACE "[\\-]" "_" ENABLE_PLUGIN "ENABLE_${PLUGIN_NAME_UC}")

  if (${SQUASH_PLUGIN_DEFAULT_DISABLED})
    if (NOT ${ENABLE_PLUGIN} STREQUAL "yes")
      return ()
    endif ()
  else ()
    if (${ENABLE_PLUGIN} STREQUAL "no")
      return ()
    endif ()
  endif ()

  set (PLUGIN_TARGET squash${SQUASH_VERSION_API}-plugin-${SQUASH_PLUGIN_NAME})
  set (EMBED TRUE)

  if (NOT "${FORCE_IN_TREE_DEPENDENCIES}" STREQUAL "yes")
    if (NOT "${SQUASH_PLUGIN_EXTERNAL_PKG_PREFIX}" STREQUAL "")
      if (${SQUASH_PLUGIN_EXTERNAL_PKG_PREFIX}_FOUND)
        set (EMBED FALSE)
      endif ()
    elseif (NOT "${SQUASH_PLUGIN_EXTERNAL_PKG}" STREQUAL "")
      include (FindPkgConfig)

      pkg_check_modules ("${PLUGIN_NAME_UC}" "${SQUASH_PLUGIN_EXTERNAL_PKG}")

      if (${PLUGIN_NAME_UC}_FOUND)
        set (EMBED FALSE)
        set (SQUASH_PLUGIN_EXTERNAL_PKG_PREFIX "${PLUGIN_NAME_UC}")
      endif ()
    endif ()
  endif ()

  set (sources ${SQUASH_PLUGIN_SOURCES})
  if (${EMBED})
    list (APPEND sources ${SQUASH_PLUGIN_EMBED_SOURCES})
  endif ()

  add_library (${PLUGIN_TARGET} SHARED ${sources})
  target_link_libraries (${PLUGIN_TARGET} squash${SQUASH_VERSION_API})
  target_include_directories (${PLUGIN_TARGET} PRIVATE ${SQUASH_PLUGIN_INCLUDE_DIRS})
  set_property (TARGET ${PLUGIN_TARGET} APPEND PROPERTY COMPILE_DEFINITIONS ${SQUASH_PLUGIN_DEFINES})

  foreach (source ${SQUASH_PLUGIN_SOURCES})
    source_file_add_extra_warning_flags (${source})
  endforeach ()

  target_add_compiler_flags (${PLUGIN_TARGET} ${SQUASH_PLUGIN_COMPILER_FLAGS})

  if (NOT "${SQUASH_PLUGIN_C_STANDARD}" STREQUAL "")
    target_require_c_standard (${PLUGIN_TARGET} ${SQUASH_PLUGIN_C_STANDARD})
  endif ()
  if (NOT "${SQUASH_PLUGIN_CXX_STANDARD}" STREQUAL "")
    target_require_cxx_standard (${PLUGIN_TARGET} ${SQUASH_PLUGIN_CXX_STANDARD})
  endif ()

  if (${EMBED})
    foreach (source ${SQUASH_PLUGIN_EMBED_SOURCES})
      set_property (SOURCE ${source} APPEND PROPERTY COMPILE_DEFINITIONS ${SQUASH_PLUGIN_EMBED_DEFINES})
      source_file_add_compiler_flags (${source} ${SQUASH_PLUGIN_EMBED_COMPILER_FLAGS})
    endforeach (source)
    target_include_directories (${PLUGIN_TARGET} PRIVATE ${SQUASH_PLUGIN_EMBED_INCLUDE_DIRS})
  else ()
    if (NOT "${${SQUASH_PLUGIN_EXTERNAL_PKG_PREFIX}_LDFLAGS}" STREQUAL "")
      foreach (ldflag ${${SQUASH_PLUGIN_EXTERNAL_PKG_PREFIX}_LDFLAGS})
        set_property (TARGET ${PLUGIN_TARGET} APPEND_STRING PROPERTY LINK_FLAGS " ${ldflag}")
      endforeach ()
    else ()
      set_property (TARGET ${PLUGIN_TARGET} APPEND PROPERTY LINK_LIBRARIES ${${SQUASH_PLUGIN_EXTERNAL_PKG_PREFIX}_LIBRARIES})
    endif ()

    if (NOT "${${SQUASH_PLUGIN_EXTERNAL_PKG_PREFIX}_CFLAGS}" STREQUAL "")
      target_add_compiler_flags (${PLUGIN_TARGET} ${${PLUGIN_NAME_UC}_CFLAGS})
    else ()
      set_property (TARGET ${PLUGIN_TARGET} APPEND PROPERTY INCLUDE_DIRECTORIES ${${SQUASH_PLUGIN_EXTERNAL_PKG_PREFIX}_INCLUDE_DIR})
    endif ()
  endif ()

  # Mostly so we can use the plugins uninstalled
  configure_file (squash.ini squash.ini)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squash.ini
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/squash/${SQUASH_VERSION_API}/plugins/${SQUASH_PLUGIN_NAME})

  install(TARGETS ${PLUGIN_TARGET}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/squash/${SQUASH_VERSION_API}/plugins/${SQUASH_PLUGIN_NAME}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/squash/${SQUASH_VERSION_API}/plugins/${SQUASH_PLUGIN_NAME})

  list (FIND SQUASH_ENABLED_PLUGINS "${SQUASH_PLUGIN_NAME}" PLUGIN_ALREADY_ENABLED)
  if (PLUGIN_ALREADY_ENABLED EQUAL -1)
    set (ENABLED_PLUGINS_TMP ${SQUASH_ENABLED_PLUGINS})
    list (APPEND ENABLED_PLUGINS_TMP "${SQUASH_PLUGIN_NAME}")
    set (SQUASH_ENABLED_PLUGINS "${ENABLED_PLUGINS_TMP}" CACHE INTERNAL "enabled plugins" FORCE)
    unset (ENABLED_PLUGINS_TMP)
  endif ()
  unset (PLUGIN_ALREADY_ENABLED)

  unset (EMBED)
  unset (PLUGIN_NAME_UC)
  unset (PLUGIN_TARGET)
  unset (sources)
endfunction ()
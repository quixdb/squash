cmake_minimum_required (VERSION 2.8.7)
set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

project (Squash)
enable_testing ()

include (GNUInstallDirs)
include (ExtraWarningFlags)

set (PACKAGE_NAME Squash)
set (SQUASH_TOP_SRCDIR "${PROJECT_SOURCE_DIR}")

set (SQUASH_VERSION_MAJOR 0)
set (SQUASH_VERSION_MINOR 0)
set (SQUASH_VERSION_REVISION 1)
set (SQUASH_VERSION_API "0.1")
set (PACKAGE_VERSION ${SQUASH_VERSION_MAJOR}.${SQUASH_VERSION_MINOR}.${SQUASH_VERSION_REVISION})

include_directories ("${PROJECT_BINARY_DIR}")
include_directories ("${PROJECT_SOURCE_DIR}")

set (SQUASH_PLUGIN_DIR ${CMAKE_INSTALL_FULL_LIBDIR}/squash/${SQUASH_VERSION_API}/plugins)

configure_file (
    "${PROJECT_SOURCE_DIR}/squash/config.h.in"
    "${PROJECT_BINARY_DIR}/squash/config.h")

configure_file (
    "${PROJECT_SOURCE_DIR}/squash/version.h.in"
    "${PROJECT_BINARY_DIR}/squash/version.h")

configure_file (
    "${PROJECT_SOURCE_DIR}/squash.pc.in"
    "${PROJECT_BINARY_DIR}/squash-${SQUASH_VERSION_API}.pc"
    @ONLY)

set (squash_enabled_plugins "" CACHE INTERNAL "enabled plugins" FORCE)

check_c_compiler_flag (-fvisibility=hidden CFLAG__fvisibility_hidden)

####
## Compatibility with older CMake
##
## This will probably be removed when Ubuntu bumps their LTS server
## release (and Travis upgrades).
####

if (${CMAKE_VERSION} VERSION_LESS 2.8.11)
  function (target_include_directories target scope items)
    foreach (argn RANGE 2 ${ARGC})
      if (${argn} LESS ${ARGC})
        include_directories (${ARGV${argn}})
      endif ()
    endforeach ()
  endfunction ()
endif ()

####
## Helper functions
####

function (squash_plugin_add name sources)
  add_library(squash${SQUASH_VERSION_API}-plugin-${name} SHARED ${${sources}})
  target_link_libraries (squash${SQUASH_VERSION_API}-plugin-${name} squash${SQUASH_VERSION_API})

  # Mostly so we can use the plugins uninstalled
  configure_file (${name}.codecs ${name}.codecs)

  install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${name}.codecs
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/squash/${SQUASH_VERSION_API}/plugins/${name})

  install(TARGETS squash${SQUASH_VERSION_API}-plugin-${name}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}/squash/${SQUASH_VERSION_API}/plugins/${name}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}/squash/${SQUASH_VERSION_API}/plugins/${name})

  set (squash_enabled_plugins "${squash_enabled_plugins};${name}" CACHE INTERNAL "enabled plugins" FORCE)

  foreach (file ${${sources}})
    squash_source_file_add_cflag (${file} "-O3")
  endforeach ()
endfunction (squash_plugin_add)

macro (squash_plugin_add_link_libraries plugin libs)
  target_link_libraries (squash${SQUASH_VERSION_API}-plugin-${plugin} ${libs})
endmacro (squash_plugin_add_link_libraries)

macro (squash_plugin_add_include_directories plugin dirs)
  target_include_directories (squash${SQUASH_VERSION_API}-plugin-${plugin} PRIVATE ${dirs})
endmacro (squash_plugin_add_include_directories)

macro (squash_plugin_add_for_package plugin package sources)
  find_package (${package})
  string (TOUPPER ${package} package_uc)

  if (${package_uc}_FOUND)
    squash_plugin_add (${plugin} ${sources})

    if (${package_uc}_LIBRARIES)
      squash_plugin_add_link_libraries (${plugin} ${${package_uc}_LIBRARIES})
    endif ()

    if (${package_uc}_INCLUDE_DIRS)
      squash_plugin_add_include_directories (${plugin} ${${package_uc}_INCLUDE_DIRS})
    endif ()
  endif (${package_uc}_FOUND)
endmacro (squash_plugin_add_for_package)

function (squash_target_add_cflag target flag)
  get_target_property (existing ${target} COMPILE_FLAGS)
  if (${existing} STREQUAL "existing-NOTFOUND")
    set_target_properties (${target}
      PROPERTIES COMPILE_FLAGS "${flag}")
  else ()
    set_target_properties (${target}
      PROPERTIES COMPILE_FLAGS "${existing} ${flag}")
  endif ()
endfunction ()

function (squash_target_add_ldflag target flag)
  get_target_property (existing ${target} LINK_FLAGS)
  if (${existing} STREQUAL "existing-NOTFOUND")
    set_target_properties (${target}
      PROPERTIES LINK_FLAGS "${flag}")
  else ()
    set_target_properties (${target}
      PROPERTIES LINK_FLAGS "${existing} ${flag}")
  endif ()
endfunction ()

function (squash_source_file_add_cflag file flag)
  get_source_file_property (existing ${file} COMPILE_FLAGS)
  if (${existing} STREQUAL "NOTFOUND")
    set_source_files_properties (${file}
      PROPERTIES COMPILE_FLAGS "${flag}")
  else ()
    set_source_files_properties (${file}
      PROPERTIES COMPILE_FLAGS "${existing} ${flag}")
  endif ()
endfunction ()

function (squash_source_file_set_c99 files)
  foreach (file ${${files}})
    squash_source_file_add_cflag (${file} "-std=c99")
  endforeach (file)
endfunction (squash_source_file_set_c99)

add_subdirectory (squash)
add_subdirectory (plugins)
add_subdirectory (utils)
add_subdirectory (docs)
add_subdirectory (examples)
add_subdirectory (bindings)
add_subdirectory (tests)
add_subdirectory (benchmark)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/squash-${SQUASH_VERSION_API}.pc
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/pkgconfig)

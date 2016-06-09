# GenPkgConfig.cmake
# (c) 2016 Evan Nemerson
#
# This CMake module is intended to be used to help CMake-based build
# systems generate good pkg-config files.
#
# License:
#
#   Copyright (c) 2016 Evan Nemerson <evan@nemerson.com>
#
#   Permission is hereby granted, free of charge, to any person
#   obtaining a copy of this software and associated documentation
#   files (the "Software"), to deal in the Software without
#   restriction, including without limitation the rights to use, copy,
#   modify, merge, publish, distribute, sublicense, and/or sell copies
#   of the Software, and to permit persons to whom the Software is
#   furnished to do so, subject to the following conditions:
#
#   The above copyright notice and this permission notice shall be
#   included in all copies or substantial portions of the Software.
#
#   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
#   EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
#   MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
#   NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
#   HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
#   WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
#   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
#   DEALINGS IN THE SOFTWARE.
#
# ====================================================================
#
# THIS IS A WORK IN PROGRESS.  It probably needs a rewrite.
#
# Why not just use configure_file?
# --------------------------------
#
# That's what I used to do, and as far as I could tell it was working,
# but there is a problem.  pkg-config files generally set the prefix
# variable, then use that to generate the libdir, includedir, etc.
# For example:
#
#   prefix=/usr
#   libdir=${prefix}/lib
#   includedir=${prefix}/include
#
#   ...
#   Name: Foo
#   Libs: -L${libdir} -lfoo
#   Cflags: -I${includedir}/foo
#
# With autotools, getting this exact output is trivial.  Everything
# after the ellipsis isn't going to change, so we'll skip it from now
# on, but the first part would look like this:
#
#   prefix=@prefix@
#   libdir=@libdir@
#   includedir=@includedir@
#
# Now, if you go the obvious route with CMake and use configure_file
# (skipping the second section, because it doesn't change):
#
#   prefix=@CMAKE_INSTALL_PREFIX@
#   libdir=@CMAKE_INSTALL_FULL_LIBDIR@
#   includedir=@CMAKE_INSTALL_FULL_INCLUDEDIR@
#
# Which will be replaced with something like:
#
#   prefix=/usr
#   libdir=/usr/lib
#   includedir=/usr/include
#
# This is obviously not very different, why should you care if the
# pkg-config file uses /usr instead of ${path}?  Well, pkg-config
# allows people to override the prefix on Windows when running
# pkg-config (see the --dont-define-prefix and --prefix-variable
# flags, which are only available on Windows).  This means that people
# using Windows can easily move an entire installation tree and still
# have pkg-config work.  If you don't use Windows that may sound like
# a dumb idea, but apparently on Windows it's a thing…
#
# Now, here is the fun part: you know how CMake wants you to write
# Find*.cmake modules instead of relying on pkg-config, and the reason
# they give is that they can't depend on pkg-config files being right?
# Well, this is apparently the big reason for that.  Basically, if it
# weren't for CMake's own stupidity when generating pkg-config files,
# we could just use pkg-config instead of those goddamn Find*.cmake
# modules.
#
# So, this module provides a function to generate a pkg-config file
# which will put "${prefix}" in the generated pkg-config file instead
# of the expanded value.

include(CMakeParseArguments)

###
#
# generate_pkg_config_path(
#   outvar
#   path
#   [var_name var_value]…
#   )
#
# The path is the full path you want to encode in the pkg-config file,
# then you can have any number of argument pairs with the name and
# value of different variables.  For example:
#
#   generate_pkg_config(foo "/usr/lib64"
#     libdir "/usr/lib64"
#     prefix "/usr")
#
# Would place '${libdir}' in the "foo" variable.  If you omitted the
# libdir argument, the value would instead be '${path}/lib64'.
# Prefixes are tested in the order passed to the function, so if you
# switched the libdir and prefix aruguments around the result would be
# '${path}/lib64'.
#
###
function(generate_pkg_config_path outvar path)
  string(LENGTH "${path}" path_length)

  set(path_args ${ARGV})
  list(REMOVE_AT path_args 0 1)
  list(LENGTH path_args path_args_remaining)

  set("${outvar}" "${path}")

  while(path_args_remaining GREATER 1)
    list(GET path_args 0 name)
    list(GET path_args 1 value)

    get_filename_component(value_full "${value}" ABSOLUTE)
    string(LENGTH "${value}" value_length)

    if(path_length EQUAL value_length AND path STREQUAL value)
      set("${outvar}" "\${${name}}")
      break()
    elseif(path_length GREATER value_length)
      # We might be in a subdirectory of the value, but we have to be
      # careful about a prefix matching but not being a subdirectory
      # (for example, /usr/lib64 is not a subdirectory of /usr/lib).
      # We'll do this by making sure the next character is a directory
      # separator.
      string(SUBSTRING "${path}" ${value_length} 1 sep)
      if(sep STREQUAL "/")
        string(SUBSTRING "${path}" 0 ${value_length} s)
        if(s STREQUAL value)
          string(SUBSTRING "${path}" "${value_length}" -1 suffix)
          set("${outvar}" "\${${name}}${suffix}")
          break()
        endif()
      endif()
    endif()

    list(REMOVE_AT path_args 0 1)
    list(LENGTH path_args path_args_remaining)
  endwhile()

  set("${outvar}" "${${outvar}}" PARENT_SCOPE)
endfunction(generate_pkg_config_path)

###
#
# generate_pkg_config(
#   output_file
#   [NAME name]
#   [DESCRIPTION description]
#   [PREFIX prefix]
#   [LIBDIR libdir]
#   [INCLUDEDIR includedir]
#   [DEPENDS …]
#   [CFLAGS …]
#   [LIBRARIES …])
#
# Arguments:
#
#   name: name of the package (default: ${CMAKE_PROJECT_NAME})
#   description: human-readable description
#   output_file: location to write the pkg-config file to.
#   prefix: prefix to use (default: ${CMAKE_INSTALL_PREFIX}
#   libdir: library dir (defualt: ${CMAKE_INSTALL_FULL_LIBDIR})
#   exec_prefix: exec prefix (default: '${prefix}'
#   depends: packages to depend on
#   libraries: library targets
###
function(generate_pkg_config output_file)
  set (options)
  set (oneValueArgs NAME DESCRIPTION PREFIX LIBDIR INCLUDEDIR VERSION)
  set (multiValueArgs DEPENDS CFLAGS LIBRARIES)
  cmake_parse_arguments(GEN_PKG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
  unset (options)
  unset (oneValueArgs)
  unset (multiValueArgs)

  if(NOT GEN_PKG_PREFIX)
    set(GEN_PKG_PREFIX "${CMAKE_INSTALL_PREFIX}")
  endif()

  if(NOT GEN_PKG_LIBDIR)
    set(GEN_PKG_LIBDIR "${CMAKE_INSTALL_FULL_LIBDIR}")
  endif()
  generate_pkg_config_path(GEN_PKG_LIBDIR "${GEN_PKG_LIBDIR}"
    prefix "${GEN_PKG_PREFIX}")

  if(NOT GEN_PKG_INCLUDEDIR)
    set(GEN_PKG_INCLUDEDIR "${CMAKE_INSTALL_FULL_INCLUDEDIR}")
  endif()
  generate_pkg_config_path(GEN_PKG_INCLUDEDIR "${GEN_PKG_INCLUDEDIR}"
    prefix "${GEN_PKG_PREFIX}")

  file(WRITE  "${output_file}" "prefix=${GEN_PKG_PREFIX}\n")
  file(APPEND "${output_file}" "libdir=${GEN_PKG_LIBDIR}\n")
  file(APPEND "${output_file}" "includedir=${GEN_PKG_INCLUDEDIR}\n")
  file(APPEND "${output_file}" "\n")

  if(GEN_PKG_NAME)
    file(APPEND "${output_file}" "Name: ${GEN_PKG_NAME}\n")
  else()
    file(APPEND "${output_file}" "Name: ${CMAKE_PROJECT_NAME}\n")
  endif()

  if(GEN_PKG_DESCRIPTION)
    file(APPEND "${output_file}" "Description: ${GEN_PKG_DESCRIPTION}\n")
  endif()

  if(GEN_PKG_VERSION)
    file(APPEND "${output_file}" "Version: ${GEN_PKG_VERSION}\n")
  endif()

  if(GEN_PKG_LIBRARIES)
    set(libs)

    file(APPEND "${output_file}" "Libs: -L\${libdir}")
    foreach(lib ${GEN_PKG_LIBRARIES})
      file(APPEND "${output_file}" " -l${lib}")
    endforeach()
    file(APPEND "${output_file}" "\n")
  endif()

  file(APPEND "${output_file}" "Cflags: -I\${includedir}")
  if(GEN_PKG_CFLAGS)
    foreach(cflag ${GEN_PKG_CFLAGS})
      file(APPEND "${output_file}" " ${cflag}")
    endforeach()
  endif()
  file(APPEND "${output_file}" "\n")
endfunction(generate_pkg_config)

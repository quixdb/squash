# - Try to find Lzma
# Once done this will define
#  LZMA_FOUND - System has Lzma
#  LZMA_INCLUDE_DIRS - The Lzma include directories
#  LZMA_LIBRARIES - The libraries needed to use liblzma
#  LZMA_DEFINITIONS - Compiler switches required for using liblzma

find_package(PkgConfig)
pkg_check_modules(PC_LIBXML QUIET liblzma)
set(LZMA_DEFINITIONS ${PC_LIBXML_CFLAGS_OTHER})

find_path(LZMA_INCLUDE_DIR "lzma.h"
          HINTS ${PC_LIBXML_INCLUDEDIR} ${PC_LIBXML_INCLUDE_DIRS})

find_library(LZMA_LIBRARY NAMES lzma
             HINTS ${PC_LIBXML_LIBDIR} ${PC_LIBXML_LIBRARY_DIRS})

set(LZMA_LIBRARIES ${LZMA_LIBRARY})
set(LZMA_INCLUDE_DIRS ${LZMA_INCLUDE_DIR})

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set LZMA_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(LZMA DEFAULT_MSG
  LZMA_LIBRARY LZMA_INCLUDE_DIR)

mark_as_advanced(LZMA_INCLUDE_DIR LZMA_LIBRARY)
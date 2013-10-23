find_path(LZO_INCLUDE_DIR lzo/lzoconf.h ${_LZO_PATHS} PATH_SUFFIXES include)

if (NOT LZO_LIBRARIES)
  find_library(LZO_LIBRARY NAMES lzo2 ${_LZO_PATHS} PATH_SUFFIXES lib)
endif ()

find_path (LZO_INCLUDE_DIR lzo/lzoconf.h
  PATH_SUFFIXES include)
find_library (LZO_LIBRARY
  NAMES lzo2)

if (EXISTS ${LZO_INCLUDE_DIR})
  if (EXISTS ${LZO_LIBRARY})
    set (LZO_LIBRARIES ${LZO_LIBRARY})
    set (LZO_FOUND "LZO_FOUND")
    break ()
  endif ()
endif ()

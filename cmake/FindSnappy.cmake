find_path(SNAPPY_INCLUDE_DIR snappy/snappy-c.h ${_SNAPPY_PATHS} PATH_SUFFIXES include)

if (NOT SNAPPY_LIBRARIES)
  find_library(SNAPPY_LIBRARY NAMES snappy ${_SNAPPY_PATHS} PATH_SUFFIXES lib)
endif ()

find_path (SNAPPY_INCLUDE_DIR snappy-c.h
  PATH_SUFFIXES include)
find_library (SNAPPY_LIBRARY
  NAMES snappy)

if (EXISTS ${SNAPPY_INCLUDE_DIR})
  if (EXISTS ${SNAPPY_LIBRARY})
    set (SNAPPY_INCLUDE_DIRS ${SNAPPY_INCLUDE_DIR})
    set (SNAPPY_LIBRARIES ${SNAPPY_LIBRARY})
    set (SNAPPY_FOUND "SNAPPY_FOUND")
    break ()
  endif ()
endif ()

set(_IPPROOT)

if (IPPDC_ROOT)
  set (_IPPDC_SEARCH_ROOT PATHS ${IPPDC_ROOT} NO_DEFAULT_PATH)
  list (APPEND _IPPROOT _IPPDC_SEARCH_ROOT)
endif (IPPDC_ROOT)

list (APPEND _IPPROOT $ENV{IPPROOT})

foreach (prefix ${_IPPROOT})
  find_path (IPPDC_INCLUDE_DIR ippdc.h
    PATHS ${prefix}
    PATH_SUFFIXES include)
  find_library (IPPDC_LIBRARY ippdc
    NAMES ippdc
    PATHS ${prefix}
    PATH_SUFFIXES lib64 lib lib/intel64 lib/ia32)

  if (EXISTS ${IPPDC_INCLUDE_DIR})
    if (EXISTS ${IPPDC_LIBRARY})
      set (IPPDC_FOUND "IPPDC_FOUND")
      break ()
    endif ()
  endif ()
endforeach ()

if (IPPDC_FOUND)
  set (IPPDC_INCLUDE_DIRS)
  list (APPEND IPPDC_INCLUDE_DIRS  ${IPPDC_INCLUDE_DIR})
  set (IPPDC_LIBRARIES ${IPPDC_LIBRARY})
endif ()

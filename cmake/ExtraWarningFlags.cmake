# This is basically supposed to be the CMake equivalent of
# https://git.gnome.org/browse/gnome-common/tree/macros2/gnome-compiler-flags.m4

include (CheckCCompilerFlag)

set (EXTRA_WARNING_FLAGS)

foreach (flag
    -Wall
    -Wstrict-prototypes
    -Wnested-externs
    -Werror=missing-prototypes
    -Werror=implicit-function-declaration
    -Werror=pointer-arith
    -Werror=init-self
    -Werror=format-security
    -Werror=format=2
    -Werror=missing-include-dirs)
  # Hackish, I know.  If anyone knows a better way please let me know.
  string (REGEX REPLACE "[-=]+" "_" test_name "CFLAG_${flag}")
  CHECK_C_COMPILER_FLAG (${flag} ${test_name})
  if (${test_name})
    set (EXTRA_WARNING_FLAGS "${EXTRA_WARNING_FLAGS} ${flag}")
  endif ()
endforeach ()

function (add_extra_warning_flags scope obj)
  get_property (existing ${scope} ${obj} PROPERTY COMPILE_FLAGS)
  if ("${existing}" STREQUAL "NOTFOUND")
    set_property (${scope} ${obj} PROPERTY COMPILE_FLAGS "${EXTRA_WARNING_FLAGS}")
  else ()
    set_property (${scope} ${obj} PROPERTY COMPILE_FLAGS "${existing} ${EXTRA_WARNING_FLAGS}")
  endif ()
endfunction ()

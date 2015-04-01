# This is basically supposed to be the CMake equivalent of
# https://git.gnome.org/browse/gnome-common/tree/macros2/gnome-compiler-flags.m4

include (CheckCCompilerFlag)
include (CheckCXXCompilerFlag)

set (EXTRA_WARNING_CFLAGS)
set (EXTRA_WARNING_CXXFLAGS)

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
    -Werror=missing-include-dirs
    -Wextra
    -Waggregate-return
    -Wcast-align
    -Wno-uninitialized
    -Wclobbered
    -Wempty-body
    -Wformat-nonliteral
    -Wformat-security
    -Wignored-qualifiers
    -Winit-self
    -Winline
    -Winvalid-pch
    -Wlogical-op
    -Wmissing-declarations
    -Wmissing-format-attribute
    -Wmissing-include-dirs
    -Wmissing-noreturn
    -Wmissing-parameter-type
    -Wmissing-prototypes
    -Wno-missing-field-initializers
    -Wno-strict-aliasing
    -Wno-unused-parameter
    -Wold-style-definition
    -Woverride-init
    -Wpacked
    -Wpointer-arith
    -Wredundant-decls
    -Wreturn-type
    -Wshadow
    -Wsign-compare
    -Wswitch-enum
    -Wsync-nand
    -Wtype-limits
    -Wundef
    -Wuninitialized
    -WUnsafe-loop-optimizations
    -Wwrite-strings
)
  # Hackish, I know.  If anyone knows a better way please let me know.
  string (REGEX REPLACE "[-=]+" "_" test_name "CFLAG_${flag}")
  CHECK_C_COMPILER_FLAG (${flag} ${test_name})
  if (${test_name})
    set (EXTRA_WARNING_CFLAGS "${EXTRA_WARNING_CFLAGS} ${flag}")
  endif ()
endforeach ()

foreach (flag
    -Wall
    -Werror=missing-prototypes
    -Werror=implicit-function-declaration
    -Werror=pointer-arith
    -Werror=init-self
    -Werror=format=2
    -Werror=missing-include-dirs
    -Wextra
    -Waggregate-return
    -Wcast-align
    -Wno-uninitialized
    -Wclobbered
    -Wempty-body
    -Wformat-nonliteral
    -Wformat-security
    -Wignored-qualifiers
    -Winit-self
    -Winline
    -Winvalid-pch
    -Wlogical-op
    -Wmissing-declarations
    -Wmissing-format-attribute
    -Wmissing-include-dirs
    -Wmissing-noreturn
    -Wmissing-parameter-type
    -Wno-missing-field-initializers
    -Wno-strict-aliasing
    -Wno-unused-parameter
    -Wpacked
    -Wpointer-arith
    -Wredundant-decls
    -Wreturn-type
    -Wshadow
    -Wsign-compare
    -Wswitch-enum
    -Wsync-nand
    -Wtype-limits
    -Wundef
    -Wuninitialized
    -Wwrite-strings
)
  string (REGEX REPLACE "[-=]+" "_" test_name "CXXFLAG_${flag}")
  CHECK_CXX_COMPILER_FLAG (${flag} ${test_name})
  if (${test_name})
    set (EXTRA_WARNING_CXXFLAGS "${EXTRA_WARNING_CXXFLAGS} ${flag}")
  endif ()
endforeach ()

function (add_extra_warning_cflags scope obj)
  get_property (existing ${scope} ${obj} PROPERTY COMPILE_FLAGS)
  if ("${existing}" STREQUAL "NOTFOUND")
    set_property (${scope} ${obj} PROPERTY COMPILE_FLAGS "${EXTRA_WARNING_CFLAGS}")
  else ()
    set_property (${scope} ${obj} PROPERTY COMPILE_FLAGS "${existing} ${EXTRA_WARNING_CFLAGS}")
  endif ()
endfunction ()

function (add_extra_warning_cxxflags scope obj)
  get_property (existing ${scope} ${obj} PROPERTY COMPILE_FLAGS)
  if ("${existing}" STREQUAL "NOTFOUND")
    set_property (${scope} ${obj} PROPERTY COMPILE_FLAGS "${EXTRA_WARNING_CXXFLAGS}")
  else ()
    set_property (${scope} ${obj} PROPERTY COMPILE_FLAGS "${existing} ${EXTRA_WARNING_CXXFLAGS}")
  endif ()
endfunction ()

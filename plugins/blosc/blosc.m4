AC_ARG_ENABLE([blosc],
              [AC_HELP_STRING([--enable-blosc=@<:@no/auto/internal/system@:>@], [Enable blosc plugin @<:@default=auto@:>@])],,
              [enable_blosc=auto])

AS_CASE([$enable_blosc],
    [no], [enable_blosc=no],
    [system], [
      AC_CHECK_HEADER([blosc.h],,[AC_MSG_ERROR([blosc.h is required for system blosc])])
      AC_CHECK_LIB([blosc],[blosc_init],,[AC_MSG_ERROR([libblosc is required for system blosc])])
      system_blosc=yes
    ], [auto], [
      AC_CHECK_HEADER([blosc.h],[
          AC_CHECK_LIB([blosc],[blosc_compress],[
              system_blosc=yes
            ])
        ])
      enable_blosc=yes
    ], [internal], [
      enable_blosc=yes
    ], [
      AC_MSG_ERROR([Invalid argument passed to --enable-blosc, should be one of @<:@no/auto/internal/system@:>@])
    ])

AM_CONDITIONAL([USE_SYSTEM_BLOSC], test x$system_blosc = "xyes")
AM_CONDITIONAL([ENABLE_BLOSC], test x$enable_blosc = "xyes")

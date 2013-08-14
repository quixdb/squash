AC_ARG_ENABLE([lzf],
              [AC_HELP_STRING([--enable-lzf=@<:@no/auto/internal/system@:>@], [Enable lzf plugin @<:@default=auto@:>@])],,
              [enable_lzf=auto])

AS_CASE([$enable_lzf],
    [no], [enable_lzf=no],
    [system], [
      AC_CHECK_HEADER([lzf.h],,[AC_MSG_ERROR([lzf.h is required for system lzf])])
      AC_CHECK_LIB([lzf],[lzf_compress],,[AC_MSG_ERROR([liblzf is required for system lzf])])
      system_lzf=yes
    ], [auto], [
      AC_CHECK_HEADER([lzf.h],[
          AC_CHECK_LIB([lzf],[lzf_compress],[
              system_lzf=yes
            ])
        ])
      enable_lzf=yes
    ], [internal], [
      enable_lzf=yes
    ], [
      AC_MSG_ERROR([Invalid argument passed to --enable-lzf, should be one of @<:@no/auto/internal/system@:>@])
    ])

AM_CONDITIONAL([USE_SYSTEM_LZF], test x$system_lzf = "xyes")
AM_CONDITIONAL([ENABLE_LZF], test x$enable_lzf = "xyes")

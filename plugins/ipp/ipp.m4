SQUASH_ENABLE_PLUGIN([ipp])

dnl $IPPROOT is set if you source $IPPROOT/bin/ippvars.sh
AC_ARG_VAR([IPPROOT], [root directory for Intel Performance Primitives])

AS_IF([test "x$IPPROOT" != "x"],[
    IPP_CFLAGS="-I$IPPROOT/include"
  ])
AC_SUBST([IPP_CFLAGS])

CFLAGS_ORIG="$CFLAGS"
CFLAGS="$CFLAGS $IPP_CFLAGS"
AS_CASE([$enable_ipp],
    [no], [enable_ipp=no],
    [yes], [
      AC_CHECK_HEADER([ippdc.h],,[AC_MSG_ERROR([ippdc.h is required for ipp])])
      AC_CHECK_LIB([ippdc],[ippsDeflateLZ77_8u],,[AC_MSG_ERROR([libippdc is required for ipp])])
    ], [auto], [
      AC_CHECK_HEADER([ippdc.h],[
        AC_CHECK_LIB([ippdc],[ippsDeflateLZ77_8u],[
            enable_ipp=yes
          ],[
            enable_ipp=no
          ])
      ], [enable_ipp=no])
    ], [
      AC_MSG_ERROR([Invalid argument passed to --enable-ipp, should be one of @<:@no/auto/yes@:>@])
    ])
CFLAGS="$CFLAGS_ORIG"

AM_CONDITIONAL([ENABLE_IPP], test "x$enable_ipp" = "xyes")

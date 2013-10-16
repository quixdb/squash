AC_ARG_ENABLE([lzjb],
              [AC_HELP_STRING([--enable-lzjb=@<:@yes/no@:>@], [Enable lzjb plugin @<:@default=yes@:>@])],,
              [enable_lzjb=yes])

AM_CONDITIONAL([ENABLE_LZJB], test x$enable_lzjb = "xyes")

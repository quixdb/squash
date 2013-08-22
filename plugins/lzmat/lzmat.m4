AC_ARG_ENABLE([lzmat],
              [AC_HELP_STRING([--enable-lzmat=@<:@yes/no@:>@], [Enable lzmat plugin @<:@default=yes@:>@])],,
              [enable_lzmat=yes])

AM_CONDITIONAL([ENABLE_LZMAT], test x$enable_lzmat = "xyes")

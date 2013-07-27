AC_ARG_ENABLE([lz4],
              [AC_HELP_STRING([--enable-lz4=@<:@yes/no@:>@], [Enable lz4 plugin @<:@default=yes@:>@])],,
              [enable_lz4=yes])

AM_CONDITIONAL([ENABLE_LZ4], test x$enable_lz4 = "xyes")

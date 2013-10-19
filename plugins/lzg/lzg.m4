AC_ARG_ENABLE([lzg],
              [AC_HELP_STRING([--enable-lzg=@<:@yes/no@:>@], [Enable lzg plugin @<:@default=no@:>@])],,
              [enable_lzg=no])

AM_CONDITIONAL([ENABLE_LZG], test x$enable_lzg = "xyes")

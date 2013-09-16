AC_ARG_ENABLE([wflz],
              [AC_HELP_STRING([--enable-wflz=@<:@yes/no@:>@], [Enable wflz plugin @<:@default=yes@:>@])],,
              [enable_wflz=yes])

AM_CONDITIONAL([ENABLE_WFLZ], test x$enable_wflz = "xyes")

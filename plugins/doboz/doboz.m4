AC_ARG_ENABLE([doboz],
              [AC_HELP_STRING([--enable-doboz=@<:@yes/no@:>@], [Enable doboz plugin @<:@default=yes@:>@])],,
              [enable_doboz=yes])

AM_CONDITIONAL([ENABLE_DOBOZ], test x$enable_doboz = "xyes")

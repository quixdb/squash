AC_ARG_ENABLE([sharc],
              [AC_HELP_STRING([--enable-sharc=@<:@yes/no@:>@], [Enable sharc plugin @<:@default=yes@:>@])],,
              [enable_sharc=yes])

AM_CONDITIONAL([ENABLE_SHARC], test x$enable_sharc = "xyes")

AC_ARG_ENABLE([fastlz],
              [AC_HELP_STRING([--enable-fastlz=@<:@yes/no@:>@], [Enable fastlz plugin @<:@default=yes@:>@])],,
              [enable_fastlz=yes])

AM_CONDITIONAL([ENABLE_FASTLZ], test x$enable_fastlz = "xyes")

AC_ARG_ENABLE([quicklz],
              [AC_HELP_STRING([--enable-quicklz=@<:@yes/no@:>@], [Enable fastlz plugin @<:@default=yes@:>@])],,
              [enable_quicklz=yes])

AM_CONDITIONAL([ENABLE_QUICKLZ], test x$enable_quicklz = "xyes")

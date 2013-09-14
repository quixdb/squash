AC_LANG_PUSH([C++])

AC_ARG_ENABLE([zpaq],
              [AC_HELP_STRING([--enable-zpaq=@<:@yes/no@:>@], [Enable zpaq plugin @<:@default=yes@:>@])],,
              [enable_zpaq=yes])

AM_CONDITIONAL([ENABLE_ZPAQ], test x$enable_zpaq = "xyes")

AC_PROG_CXX
AC_TYPE_INT64_T
AC_TYPE_UINT16_T
AC_TYPE_UINT32_T
AC_TYPE_UINT64_T

AC_FUNC_ERROR_AT_LINE

AC_CHECK_FUNCS([floor pow])

AC_OPENMP

AC_LANG_POP([C++])

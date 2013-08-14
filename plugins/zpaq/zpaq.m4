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

if test x"$ax_cv_have_sse2_ext" != x"yes"; then
  ZPAQ_SSE2_CPPFLAGS="-DNOJIT"
fi
AC_SUBST([ZPAQ_SSE2_CPPFLAGS])

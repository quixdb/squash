AC_DEFUN([SQUASH_CHECK_BUILTIN_FUNC], [
    AC_MSG_CHECKING([for $1])
    AH_TEMPLATE([HAVE_]translit($1, 'a-z', 'A-Z'), [Define to 1 if you have the `$1' function.])
    AC_LINK_IFELSE(
        [AC_LANG_SOURCE([
          int main(void) { $2 }
        ])], [
          AC_MSG_RESULT([yes])
          AC_DEFINE([HAVE_]translit($1, 'a-z', 'A-Z'),[1])
          have_$1=yes
        ], [
          AC_MSG_RESULT([no])
          have_$1=no
        ])
  ])

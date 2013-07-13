dnl
dnl Taken from glib (and modified slightly), so this is LGPLv2.1
dnl
AC_DEFUN([CHECK_HIDDEN_VISIBILITY], [
  HIDDEN_VISIBILITY_CFLAGS=""
  case "$host" in
    *-*-mingw*)
      dnl on mingw32 we do -fvisibility=hidden and __declspec(dllexport)
      AC_DEFINE([$1], [__attribute__((visibility("default"))) __declspec(dllexport) extern],
                [defines how to decorate public symbols while building])
      CFLAGS="${CFLAGS} -fvisibility=hidden"
      ;;
    *)
      dnl on other compilers, check if we can do -fvisibility=hidden
      SAVED_CFLAGS="${CFLAGS}"
      CFLAGS="-fvisibility=hidden"
      AC_MSG_CHECKING([for -fvisibility=hidden compiler flag])
      AC_TRY_COMPILE([], [int main (void) { return 0; }],
                     AC_MSG_RESULT(yes)
                     enable_fvisibility_hidden=yes,
                     AC_MSG_RESULT(no)
                     enable_fvisibility_hidden=no)
      CFLAGS="${SAVED_CFLAGS}"

      AS_IF([test "${enable_fvisibility_hidden}" = "yes"], [
        AC_DEFINE([$1], [__attribute__((visibility("default"))) extern],
                  [defines how to decorate public symbols while building])
        HIDDEN_VISIBILITY_CFLAGS="-fvisibility=hidden"
      ])
      ;;
  esac
  AC_SUBST(HIDDEN_VISIBILITY_CFLAGS)
])

#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.

m4_define([squash_major], [0])
m4_define([squash_minor], [0])
m4_define([squash_micro], [1])

m4_define([squash_version], [squash_major.squash_minor.squash_micro])
m4_define([squash_api_version], [0.1])
m4_define([squash_api_version_u], [0_1])

AC_PREREQ([2.69])
AC_INIT([Squash], [squash_version], [https://github.com/quixdb/squash/issues])
AC_CONFIG_HEADERS([squash/config.h])
AC_CONFIG_MACRO_DIR([m4])
LT_INIT

AM_INIT_AUTOMAKE([1.11 -Wno-portability no-define no-dist-gzip dist-xz tar-ustar])
AM_MAINTAINER_MODE([enable])

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

SQUASH_MAJOR_VERSION=squash_major
SQUASH_MINOR_VERSION=squash_minor
SQUASH_MICRO_VERSION=squash_micro
SQUASH_VERSION=squash_version
SQUASH_API_VERSION=squash_api_version
SQUASH_API_VERSION_U=squash_api_version_u
AC_SUBST(SQUASH_MAJOR_VERSION)
AC_SUBST(SQUASH_MINOR_VERSION)
AC_SUBST(SQUASH_MICRO_VERSION)
AC_SUBST(SQUASH_VERSION)
AC_SUBST(SQUASH_API_VERSION)
AC_SUBST(SQUASH_API_VERSION_U)

AC_DEFUN([SQUASH_ENABLE_PLUGIN],
[
  AC_ARG_ENABLE([$1],
                [AC_HELP_STRING([--enable-$1=@<:@yes/no/auto@:>@], [Enable $1 plugin @<:@default=auto@:>@])],,
                [enable_$1=auto])
])

AC_DEFUN([SQUASH_ENABLE_PLUGIN_PKG_CONFIG],
[
  SQUASH_ENABLE_PLUGIN([$1])

  AS_CASE([$enable_$1],
      [no], [enable_$1=no],
      [yes], [
        PKG_CHECK_MODULES([$2],[$3])
      ], [auto], [
        PKG_CHECK_MODULES([$2],[$3],[
            enable_$1=yes
          ], [
            enable_$1=no
          ])
      ], [
        AC_MSG_ERROR([Invalid argument passed to --enable-$1, should be one of @<:@no/auto/yes@:>@])
      ])

  AM_CONDITIONAL([ENABLE_$2], test x$enable_$1 = "xyes")
])

dnl SQUASH_ENABLE_PLUGIN_HEADER_AND_LIB([plugin-name],[uppercase-name],[header],[library],[function])
AC_DEFUN([SQUASH_ENABLE_PLUGIN_HEADER_AND_LIB], [
  SQUASH_ENABLE_PLUGIN([$1])

  AS_CASE([$enable_$1],
      [no], [enable_$1=no],
      [yes], [
        AC_CHECK_HEADER([$3],,[AC_MSG_ERROR([$3 is required for $1])])
        AC_CHECK_LIB([$4],[$5],,[AC_MSG_ERROR([lib$4 is required for $1])])
      ], [auto], [
        AC_CHECK_HEADER([$3],[
          AC_CHECK_LIB([$4],[$5],[
              enable_$1=yes
            ],[
              enable_$1=no
            ])
        ], [enable_$1=no])
      ], [
        AC_MSG_ERROR([Invalid argument passed to --enable-$1, should be one of @<:@no/auto/yes@:>@])
      ])

  AM_CONDITIONAL([ENABLE_$2], test x$enable_$1 = "xyes")
])

# Checks for programs.
AC_PROG_CC

SQUASH_DOXYGEN_PATHS=""
AC_ARG_ENABLE([docs],
              [AC_HELP_STRING([--enable-docs=@<:@yes/no/auto@:>@], [Enable documentation (using Doxygen) @<:@default=auto@:>@])],,
              [enable_docs=auto])

AS_CASE([$enable_docs],
  [yes], [
      AC_PATH_PROG([DOXYGEN], [doxygen])
      AS_IF([test "x" = "x$DOXYGEN"], [
          AC_MSG_ERROR([doxygen is required when enabling documentation.])
        ])
    ],
  [auto], [
      AC_PATH_PROG([DOXYGEN], [doxygen])
    ])
AM_CONDITIONAL([ENABLE_DOCS], test -x "$DOXYGEN")

AM_PROG_VALAC(,[have_valac="yes"],[have_valac="no"])
AM_CONDITIONAL([HAVE_VALAC], test x$have_valac = "xyes")

GNOME_COMPILE_WARNINGS
CHECK_HIDDEN_VISIBILITY([SQUASH_API])

# Checks for libraries.

AC_CHECK_LIB([ltdl],[lt_dlopenext],,[AC_MSG_ERROR([libltdl is required.])])

GLIB_TARGET_VERSION=2.26
AC_SUBST([GLIB_TARGET_VERSION])

PKG_CHECK_MODULES([GLIB], [glib-2.0 >= $GLIB_TARGET_VERSION], [
    have_glib=yes
    GLIB_CFLAGS="${GLIB_CFLAGS} -DGLIB_VERSION_MIN_REQUIRED=GLIB_VERSION_2_26 -DGLIB_VERSION_MAX_ALLOWED=GLIB_VERSION_2_26"
  ], [
    have_glib=no
  ])
AM_CONDITIONAL([HAVE_GLIB], test x$have_glib = "xyes")
AC_SUBST([GLIB_CFLAGS])
AC_SUBST([GLIB_LIBS])

PKG_CHECK_MODULES([GOBJECT], [gobject-2.0 >= $GLIB_TARGET_VERSION], [
    have_gobject=yes
  ], [
    have_gobject=no
  ])
AM_CONDITIONAL([HAVE_GOBJECT], test x$have_gobject = "xyes")
AC_SUBST([GOBJECT_CFLAGS])
AC_SUBST([GOBJECT_LIBS])

PKG_CHECK_MODULES([GIO], [gio-2.0 >= $GLIB_TARGET_VERSION], [
    have_gio=yes
  ], [
    have_gio=no
  ])
AM_CONDITIONAL([HAVE_GIO], test x$have_gio = "xyes")
AC_SUBST([GIO_CFLAGS])
AC_SUBST([GIO_LIBS])

AH_TEMPLATE([HAVE_PTHREAD],
            [Define if you have POSIX threads libraries and header files.])
AX_PTHREAD([
    have_pthread=yes
    AC_DEFINE([HAVE_PTHREAD],[1])
  ], [
    have_pthread=no
  ])
AM_CONDITIONAL([HAVE_PTHREAD], test x$have_pthread = "xyes")

AC_ARG_ENABLE([unit-testing], [AC_HELP_STRING([--enable-unit-testing=@<:@yes/no/auto@:>@], [Enable unit testing @<:@default=auto@:>@])], [
  AS_CASE([$enableval],
      [no], [enable_unit_testing=no],
      [auto], [enable_unit_testing=$have_glib],
      [yes], [
          AS_IF([test "x$have_glib" != "xyes"], [
              AC_MSG_ERROR([Unit testing requires glib >= $GLIB_TARGET_VERSION])
            ])
        ],
      [AC_MSG_ERROR([Invalid argument passed to --enable-unit-testing, should be one of @<:@no/auto/yes@:>@])])
  ], [
    enable_unit_testing=$have_glib
  ])

# For plug-ins
AX_EXT
PLUGIN_LIBTOOL_FLAGS="-module -avoid-version"
AC_SUBST(PLUGIN_LIBTOOL_FLAGS)

SQUASH_PLUGINS=""
SQUASH_PLUGINS_ENABLED=""

m4_foreach([plugin],[bzip2, doboz, fastlz, lz4, lzma, lzf, lzo, quicklz, sharc, snappy, zlib, zpaq], [
    m4_include(plugins/plugin/plugin[.m4])
    SQUASH_PLUGINS="${SQUASH_PLUGINS} plugin"
  ])

for plugin in $SQUASH_PLUGINS; do
  enable_plugin=enable_$plugin
  if test "${!enable_plugin}" = "yes"; then
    SQUASH_PLUGINS_ENABLED="$SQUASH_PLUGINS_ENABLED $plugin"
    SQUASH_DOXYGEN_PATHS="$SQUASH_DOXYGEN_PATHS ../plugins/${plugin}/${plugin}.md"
  fi
done
AC_SUBST(SQUASH_PLUGINS)
AC_SUBST(SQUASH_PLUGINS_ENABLED)

AC_SUBST(SQUASH_DOXYGEN_PATHS)

# Checks for header files.

AC_CHECK_HEADERS([stddef.h])
AC_CHECK_HEADER_STDBOOL

# Checks for typedefs, structures, and compiler characteristics.
AC_TYPE_SIZE_T
AC_TYPE_UINT8_T

SQUASH_CHECK_BUILTIN_FUNC([__sync_bool_compare_and_swap], [
     return __sync_bool_compare_and_swap ((int *)0, 0, 0);
   ])

SQUASH_CHECK_BUILTIN_FUNC([__sync_fetch_and_add], [
     return __sync_fetch_and_add ((int *)0, 0);
   ])

SQUASH_CHECK_BUILTIN_FUNC([__sync_fetch_and_sub], [
     return __sync_fetch_and_sub ((int *)0, 0);
   ])

have_atomic_builtins=no
AS_IF([test "x$have___sync_bool_compare_and_swap" = "xyes"], [
  AS_IF([test "x$have___sync_fetch_and_add" = "xyes"], [
    AS_IF([test "x$have___sync_fetch_and_sub" = "xyes"], [
      have_atomic_builtins=yes
    ])
  ])
])

# Checks for library functions.

AC_CHECK_FUNCS([memset pathconf strchr strdup strtol])
AC_FUNC_MALLOC
AC_FUNC_REALLOC

AC_CONFIG_FILES([Makefile
                 squash.pc
                 benchmark/Makefile
                 bindings/Makefile
                 bindings/vala/Makefile
                 bindings/vala/examples/Makefile
                 squash/Makefile
                 plugins/Makefile
                 plugins/bzip2/Makefile
                 plugins/doboz/Makefile
                 plugins/fastlz/Makefile
                 plugins/lz4/Makefile
                 plugins/lzma/Makefile
                 plugins/lzf/Makefile
                 plugins/lzo/Makefile
                 plugins/quicklz/Makefile
                 plugins/sharc/Makefile
                 plugins/snappy/Makefile
                 plugins/zlib/Makefile
                 plugins/zpaq/Makefile
                 utils/Makefile
                 docs/Doxyfile
                 docs/Makefile
                 examples/Makefile
                 tests/Makefile])

AC_OUTPUT

echo "
    Squash was configured with the following options:

    Prefix:           ${prefix}
    Unit testing:     ${enable_unit_testing}
    pthreads:         ${have_pthread}
    Atomic builtins:  ${have_atomic_builtins}

    Bindings
    ========

      Vala:           yes

    Plugins
    =======
"

for plugin in $SQUASH_PLUGINS; do
  enable_plugin=enable_$plugin
  printf "      %-16s%s\n" "${plugin}:" "${!enable_plugin}"
done
echo ""

dnl Process this file with autoconf to produce a configure script.

AC_INIT(ngraph, 6.08.05, ZXB01226@nifty.com, ngraph-gtk)

LIBNGRAPH_VERSION=0:0:0
AC_SUBST(LIBNGRAPH_VERSION)

AC_CANONICAL_HOST
AC_SYS_LARGEFILE

vsymldflags=
	AC_MSG_CHECKING([version script options])
	AC_ARG_WITH([versioned-symbol],AC_HELP_STRING([--with-versioned-symbol],[Use versioned symbols]),[dnl
	vsymldflags="-Wl,--version-script,ngraph_export.map -Wl,-O1"
	])
	AC_SUBST(vsymldflags)
	AC_MSG_RESULT([${vsymldflags}])

AC_MSG_CHECKING([for native Win32])
case "$host" in
  *-*-mingw*)
    os_win32=yes
    AC_CHECK_TOOL(WINDRES, windres)
    ;;
  *)
    os_win32=no
    ;;
esac
AC_MSG_RESULT([$os_win32])
AM_CONDITIONAL(OS_WIN32, test "$os_win32" = "yes")

AC_MSG_CHECKING([for native OSX])
case "$host" in
  *-apple-darwin*)
    os_osx=yes
    PKG_CHECK_MODULES(MAC_INTEGRATION, [gtk-mac-integration-gtk3])
    ;;
  *)
    os_osx=no
    ;;
esac
AC_MSG_RESULT([$os_osx])
AM_CONDITIONAL(OS_OSX, test "$os_osx" = "yes")

AC_CONFIG_SRCDIR(src/main.c)
AC_CONFIG_MACRO_DIR([m4])

AM_INIT_AUTOMAKE
LT_INIT

AC_LIBTOOL_WIN32_DLL
#AC_LIBTOOL_DLOPEN
AC_PROG_LIBTOOL

AC_SUBST(LIBTOOL_DEPS)

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

GSL_REQUIRED_VERSION=0.2.5
GTK3_REQUIRED_VERSION=3.12.0
GLIB_REQUIRED_VERSION=2.32.0

dnl Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL
AC_PROG_SED
AC_PROG_MKDIR_P

AC_CHECK_PROG(RM, rm, yes, no)

dnl Checks for gettext.
ALL_LINGUAS="ja"
AM_GNU_GETTEXT([external])

dnl Checks for libraries.
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([readline], [rl_completion_matches],
			  [
			  AC_DEFINE([HAVE_LIBREADLINE], 1, [have readline library])
			  READLINE_LIBS=-lreadline
			  AC_SUBST(READLINE_LIBS)
			  ])

dnl Checks for ruby
AC_CHECK_PROG(RUBY, ruby, yes, no)
if test $RUBY = yes; then
    [ruby_incrudedir=`ruby -e 'puts(RbConfig::CONFIG["rubyhdrdir"])'`]
    AC_CHECK_FILE($ruby_incrudedir/ruby.h, [have_ruby_dev=yes])
    [RUBY_DLEXT=`ruby -e 'puts(RbConfig::CONFIG["DLEXT"])'`]
    AC_SUBST(RUBY_DLEXT)
fi
AM_CONDITIONAL(HAVE_RUBY_DEV, test x$have_ruby_dev = xyes)

AM_PATH_GLIB_2_0($GLIB_REQUIRED_VERSION,,
  [AC_MSG_ERROR(Test for GLib failed.)],
  gobject gthread gmodule)

dnl GLIB_LIBS_WITH_GTHREAD="$GLIB_LIBS"
dnl AC_SUBST(GLIB_LIBS_WITH_GTHREAD)

dnl AM_PATH_GLIB_2_0($GLIB_REQUIRED_VERSION,,
dnl   [AC_MSG_ERROR(Test for GLib failed.)],
dnl   gobject)

AM_PATH_GTK_3_0($GTK3_REQUIRED_VERSION,,AC_MSG_ERROR(Test for GTK failed.),)

AM_PATH_GSL($GSL_REQUIRED_VERSION,
	AC_DEFINE([HAVE_LIBGSL], 1, [have GNU Scientific Library]),)

AC_PATH_PROG(GLIB_COMPILE_RESOURCES, glib-compile-resources)
AC_PATH_PROG(GLIB_MKENUMS, glib-mkenums)

AC_SUBST(PKG_CONFIG)

PKG_CHECK_MODULES([GTKSOURCEVIEW4], [gtksourceview-4], GTKSOURCEVIEW4="yes", GTKSOURCEVIEW4="no")
PKG_CHECK_MODULES([GTKSOURCEVIEW3], [gtksourceview-3.0], GTKSOURCEVIEW3="yes", GTKSOURCEVIEW3="no")
if test "$GTKSOURCEVIEW4" = yes; then
   AC_SUBST(GTKSOURCEVIEW_LIBS, $GTKSOURCEVIEW4_LIBS)
   AC_SUBST(GTKSOURCEVIEW_CFLAGS, $GTKSOURCEVIEW4_CFLAGS)
elif test "$GTKSOURCEVIEW3" = yes; then
   AC_SUBST(GTKSOURCEVIEW_LIBS, $GTKSOURCEVIEW3_LIBS)
   AC_SUBST(GTKSOURCEVIEW_CFLAGS, $GTKSOURCEVIEW3_CFLAGS)
else
   AC_MSG_ERROR((Test for GtkSourceview failed.),)
fi

# Checks for header files.
AC_PATH_X
AC_HEADER_DIRENT
AC_HEADER_STDC
AC_HEADER_SYS_WAIT
AC_CHECK_HEADERS([fcntl.h limits.h stdlib.h string.h strings.h sys/ioctl.h sys/time.h unistd.h utime.h readline/readline.h])

# Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_PID_T
AC_TYPE_SIZE_T
AC_STRUCT_TM

# Checks for library functions.
AC_FUNC_CLOSEDIR_VOID
AC_FUNC_ERROR_AT_LINE
AC_FUNC_FORK
AC_FUNC_MALLOC
AC_FUNC_ALLOCA
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
AC_FUNC_MMAP
AC_CHECK_FUNCS([roundl sigaction sigsuspend alarm dup2 getcwd mkdir pow powl log10l fabsl floorl sqrt strchr strdup strerror strncasecmp strstr strtol utime isfinite finite nanosleep sleep])

AC_CONFIG_FILES([Makefile \
		 pathconv.sh \
		 src/Makefile \
		 src/gtk/Makefile \
		 src/math/Makefile \
		 driver/Makefile \
		 addin/Makefile \
		 po/Makefile.in \
		 m4/Makefile \
		 demo/Makefile \
		 initfile/Makefile \
		 doc/Makefile \
		 doc/html/Makefile \
		 doc/html/img/Makefile \
		 doc/html/ja/Makefile \
		 doc/html/ja/tutorial/Makefile \
		 doc/html/ja/dialogs/Makefile \
		 zsh/Makefile \
		 test/Makefile \
		 plugins/Makefile \
		 plugins/ruby/Makefile \
		 gtksourceview/Makefile \
		 mingw/Makefile \
		 osx/Makefile \
		 icons/Makefile \
		 icons/scalable/Makefile \
		 icons/scalable/apps/Makefile \
		 icons/scalable/mimetypes/Makefile \
		 misc/Makefile])

AM_CONFIG_HEADER([config.h])
AC_OUTPUT

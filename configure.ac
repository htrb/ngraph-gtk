dnl Process this file with autoconf to produce a configure script.

AC_INIT(ngraph, 6.06.11, ZXB01226@nifty.com, ngraph-gtk)

AC_CONFIG_SRCDIR(src/gtk/main.c)
AM_INIT_AUTOMAKE

AC_PROG_LIBTOOL

m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

GSL_REQUIRED_VERSION=0.2.5
GTK2_REQUIRED_VERSION=2.16.0
GTK3_REQUIRED_VERSION=3.0.0
GLIB_REQUIRED_VERSION=2.16.0

dnl Checks for programs.
AC_PROG_CC
AM_PROG_CC_C_O
AC_PROG_INSTALL

AC_CHECK_PROG(RM, rm, yes, no)
AC_CHECK_PROG(MKDIR, mkdir, yes, no)
AC_CHECK_PROG(RUBY, ruby, yes, no)

# Checks for gettext.
ALL_LINGUAS="ja"
AM_GNU_GETTEXT([external])

# Checks for libraries.
AC_CHECK_LIB([m], [sqrt])
AC_CHECK_LIB([readline], [rl_completion_matches],
			  [
			  AC_DEFINE([HAVE_LIBREADLINE], 1, [have readline Library])
			  READLINE_LIBS=-lreadline
			  AC_SUBST(READLINE_LIBS)
			  ])

AM_PATH_GLIB_2_0($GLIB_REQUIRED_VERSION,,
  [AC_MSG_ERROR(Test for GLib failed.)],
  gobject gthread gmodule)

dnl GLIB_LIBS_WITH_GTHREAD="$GLIB_LIBS"
dnl AC_SUBST(GLIB_LIBS_WITH_GTHREAD)

dnl AM_PATH_GLIB_2_0($GLIB_REQUIRED_VERSION,,
dnl   [AC_MSG_ERROR(Test for GLib failed.)],
dnl   gobject)

AM_PATH_GTK_3_0($GTK3_REQUIRED_VERSION,,
	 [AM_PATH_GTK_2_0($GTK2_REQUIRED_VERSION,, [AC_MSG_ERROR(Test for GTK failed.)],)],)

AM_PATH_GSL($GSL_REQUIRED_VERSION,
	AC_DEFINE([HAVE_LIBGSL], 1, [have GNU Scientific Library]),)

AC_SUBST(PKG_CONFIG)

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
AC_FUNC_MKTIME
AC_FUNC_REALLOC
AC_TYPE_SIGNAL
AC_FUNC_STAT
AC_FUNC_STRTOD
AC_FUNC_UTIME_NULL
AC_FUNC_VPRINTF
AC_FUNC_MMAP
AC_CHECK_FUNCS([sigaction sigsuspend alarm dup2 getcwd mkdir pow powl log10l fabsl floorl sqrt strchr strdup strerror strncasecmp strstr strtol utime isfinite finite nanosleep sleep])

AC_CONFIG_FILES([Makefile \
		 src/Makefile \
		 src/gtk/Makefile \
		 src/math/Makefile \
		 driver/Makefile \
		 addin/Makefile \
		 po/Makefile.in \
		 m4/Makefile \
		 demo/Makefile
		 initfile/Makefile \
		 doc/Makefile \
		 doc/html/Makefile \
		 doc/html/img/Makefile \
		 doc/html/ja/Makefile \
		 doc/html/ja/tutorial/Makefile \
		 doc/html/ja/dialogs/Makefile \
		 zsh/Makefile
		 test/Makefile
		 mingw/Makefile \
		 pixmap/Makefile \
		 icons/Makefile \
		 icons/scalable/Makefile \
		 icons/scalable/apps/Makefile \
		 icons/scalable/mimetypes/Makefile \
		 misc/Makefile])

AM_CONFIG_HEADER([config.h])
AC_OUTPUT

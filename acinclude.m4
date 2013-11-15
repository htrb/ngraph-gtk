dnl check compiler/ld flags
AC_DEFUN([NGRAPH_CC_TRY_LINK_FLAG],
[dnl
	_save_cflags="$CFLAGS"
	CFLAGS="$CFLAGS -Werror -rdynamic"
	AC_LINK_IFELSE([AC_LANG_PROGRAM([],[])],
		    [$1="$2"],
		    [$1=""])
	CFLAGS="$_save_cflags"
	AC_SUBST($1)
])

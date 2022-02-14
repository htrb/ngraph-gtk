langspecdir = $(pkgdatadir)/gtksourceview
langspecs = ngraph-math.lang
nodist_langspec_DATA = $(langspecs)

EXTRA_DIST = create_lang.nsc $(langspecs).pre $(langspecs).post $(langspecs)

MATH_DEFINITION = $(top_builddir)/src/odata.c		\
	$(top_builddir)/src/math/math_constant.h.in	\
	$(top_builddir)/src/math/math_function.h.in

$(langspecs): create_lang.nsc $(MATH_DEFINITION) $(srcdir)/$(langspecs).pre $(srcdir)/$(langspecs).post
	$(top_builddir)/src/ngraph -i $(srcdir)/create_lang.nsc "$(SED)" "$(srcdir)/$(langspecs).pre" "$(srcdir)/$(langspecs).post" > $(langspecs)

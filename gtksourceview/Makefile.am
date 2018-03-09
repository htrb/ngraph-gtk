langspecdir = $(pkgdatadir)/gtksourceview
langspecs = ngraph-math.lang
nodist_langspec_DATA = $(langspecs)

EXTRA_DIST = create_lang.nsc $(langspecs).pre $(langspecs).post

$(langspecs): create_lang.nsc $(top_builddir)/src/libngraph.la $(srcdir)/$(langspecs).pre $(srcdir)/$(langspecs).post
	$(top_builddir)/src/ngraph -i $(srcdir)/create_lang.nsc "$(SED)" "$(srcdir)/$(langspecs).pre" "$(srcdir)/$(langspecs).post" > $(langspecs)

CLEANFILES = $(langspecs)

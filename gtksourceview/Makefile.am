langspecdir = $(pkgdatadir)/gtksourceview
langspecs = ngraph_math.lang
nodist_langspec_DATA = $(langspecs)

EXTRA_DIST = create_lang.nsc $(langspecs).pre $(langspecs).post

$(langspecs): create_lang.nsc $(top_builddir)/src/libngraph.la $(srcdir)/$(langspecs).pre $(srcdir)/$(langspecs).post
	$(top_builddir)/src/ngraph -i create_lang.nsc $(srcdir)/$(langspecs).pre $(srcdir)/$(langspecs).post > $(langspecs)

CLEANFILES = $(langspecs)

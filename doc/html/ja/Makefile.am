ngraphconfdir = $(sysconfdir)/$(PACKAGE)

dist_html_DATA = addin.html copyrigh.html data.html gra.html math.html	\
	ngraph.html operate.html refer.html text.html tutorial.html

nodist_html_DATA = object.html intro.html script.html

EXTRA_DIST = object.html.in intro.html.in script.html.in

htmldir = $(docdir)/html/ja

object.html: $(top_builddir)/src/libngraph.la object.html.in $(srcdir)/../objlist.nsc
	cat $(srcdir)/object.html.in > object.html
	$(top_builddir)/src/ngraph -i $(srcdir)/../objlist.nsc >> object.html

intro.html: intro.html.in Makefile
	$(SHELL) $(top_srcdir)/pathconv.sh "$(bindir)" "$(docdir)" "$(pkglibexecdir)" "$(pkglibdir)" "$(pkgdatadir)" "$(ngraphconfdir)" $(srcdir)/intro.html.in

script.html: script.html.in Makefile
	$(SHELL) $(top_srcdir)/pathconv.sh "$(bindir)" "$(docdir)" "$(pkglibexecdir)" "$(pkglibdir)" "$(pkgdatadir)" "$(ngraphconfdir)" $(srcdir)/script.html.in

CLEANFILES = intro.html script.html
DISTCLEANFILES = object.html

SUBDIRS = tutorial dialogs

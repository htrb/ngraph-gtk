ngraphconfdir = $(sysconfdir)/$(PACKAGE)

dist_html_DATA = addin.html copyrigh.html data.html gra.html math.html	\
	ngraph.html operate.html refer.html text.html tutorial.html

nodist_html_DATA = object.html intro.html script.html

EXTRA_DIST = object.html.in intro.html.in script.html.in

htmldir = $(docdir)/html/ja

object.html: ../../../src/libngraph.la object.html.in ../objlist.nsc
	cat object.html.in > object.html
	../../../src/ngraph -i ../objlist.nsc >> object.html

intro.html: intro.html.in Makefile
	sh ../../../pathconv.sh "$(bindir)" "$(docdir)" "$(pkglibexecdir)" "$(pkglibdir)" "$(pkgdatadir)" "$(ngraphconfdir)" "$(pixmapdir)" intro.html.in

script.html: script.html.in Makefile
	sh ../../../pathconv.sh "$(bindir)" "$(docdir)" "$(pkglibexecdir)" "$(pkglibdir)" "$(pkgdatadir)" "$(ngraphconfdir)" "$(pixmapdir)" script.html.in

CLEANFILES = intro.html script.html
DISTCLEANFILES = object.html

SUBDIRS = tutorial dialogs

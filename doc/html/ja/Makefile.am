ngraphconfdir = $(sysconfdir)/$(PACKAGE)

dist_html_DATA = addin.html copyrigh.html data.html gra.html	\
	math.html ngraph.html operate.html refer.html text.html	\
	tutorial.html object.html

nodist_html_DATA = intro.html script.html

EXTRA_DIST = object.html.in intro.html.in script.html.in

htmldir = $(docdir)/html/ja

object.html: $(srcdir)/../objlist.nsc $(OBJECT_DEFINITION) $(srcdir)/object.html.in
	cat $(srcdir)/object.html.in > $@
	$(top_builddir)/src/ngraph -i $< >> $@

intro.html: $(srcdir)/intro.html.in $(top_srcdir)/pathconv.sh
	$(SHELL) $(top_srcdir)/pathconv.sh $<

script.html: $(srcdir)/script.html.in $(top_srcdir)/pathconv.sh
	$(SHELL) $(top_srcdir)/pathconv.sh $<

CLEANFILES = intro.html script.html

SUBDIRS = tutorial dialogs

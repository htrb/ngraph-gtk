EXTRA_DIST = ngp2.in

bin_SCRIPTS = ngp2

ngp2: ngp2.in $(top_srcdir)/pathconv.sh
	$(SHELL) $(top_srcdir)/pathconv.sh $(srcdir)/ngp2.in

CLEANFILES = ngp2

demodir = $(pkgdatadir)/demo

dist_demo_DATA = demo1.dat demo2.dat demo3.dat demo4.dat demo6.dat	\
	demo5a.ngp demo5b.ngp demo5c.ngp demo5d.ngp demo5e.ngp demo7.ngp

nodist_demo_DATA = demo.ngp demo1.ngp demo2.ngp demo3.ngp demo4.ngp	\
	demo6.ngp

EXTRA_DIST = demo.ngp.in demo1.ngp.in demo2.ngp.in demo3.ngp.in	\
	demo4.ngp.in demo6.ngp.in windows_demo.sed

demo.ngp: Makefile
demo1.ngp: Makefile
demo2.ngp: Makefile
demo3.ngp: Makefile
demo4.ngp: Makefile
demo6.ngp: Makefile

SUFFIXES = .ngp.in .ngp
if OS_WIN32
.ngp.in.ngp:
	sed -f $(srcdir)/windows_demo.sed $< > $@
else
.ngp.in.ngp:
	sh $(top_srcdir)/pathconv.sh "$(demodir)" "$(demodir)" "$(demodir)" "$(demodir)" "$(demodir)" "$(demodir)" "$(demodir)" $<
endif

CLEANFILES = demo.ngp demo1.ngp demo2.ngp demo3.ngp demo4.ngp demo6.ngp

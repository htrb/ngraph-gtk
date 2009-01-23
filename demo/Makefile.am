
demodir = $(pkgdatadir)/demo

demo_DATA = demo1.dat demo1.ngp demo2.dat demo2.ngp demo3.dat demo3.ngp \
	demo4.dat demo4.ngp demo5a.ngp 	demo5b.ngp demo5c.ngp demo5d.ngp \
	demo5e.ngp demo.ngp encoding.ngp

EXTRA_DIST = ${demo_DATA} demo1.ngp.in demo2.ngp.in demo3.ngp.in demo4.ngp.in demo.ngp.in

demo1.ngp: demo1.ngp.in Makefile
	sh ../pathconv.sh $(pkgdatadir) $(pkgdatadir) $(demodir) demo1.ngp.in

demo2.ngp: demo2.ngp.in Makefile
	sh ../pathconv.sh $(pkgdatadir) $(pkgdatadir) $(demodir) demo2.ngp.in

demo3.ngp: demo3.ngp.in Makefile
	sh ../pathconv.sh $(pkgdatadir) $(pkgdatadir) $(demodir) demo3.ngp.in

demo4.ngp: demo4.ngp.in Makefile
	sh ../pathconv.sh $(pkgdatadir) $(pkgdatadir) $(demodir) demo4.ngp.in

demo.ngp: demo.ngp.in Makefile
	sh ../pathconv.sh $(pkgdatadir) $(pkgdatadir) $(demodir) demo.ngp.in

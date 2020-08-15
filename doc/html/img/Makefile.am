htmlimgdir = $(docdir)/html/img

dist_htmlimg_DATA = bcross.png barc.png baxispo.png bdataopn.png	\
	 bdatapo.png bdraw.png beval.png bframe.png bgauss.png		\
	 blgndpo.png bline.png bload.png bmark.png bmath.png		\
	 bpoint.png bprint.png brect.png bsave.png bscale.png		\
	 bscundo.png bsection.png bsingle.png btext.png btrim.png	\
	 bzoom.png icon.png aclear.png addgraph.png axispo.png		\
	 datapo.png draw.png eval.png fopen.png lapoint.png		\
	 legend.png math.png pointer.png scundo.png trimming.png

EXTRA_DIST = pixmaps.make

pixmaps: $(dist_htmlimg_DATA)
	make -f pixmaps.make $^

CLEANFILES = *.svg

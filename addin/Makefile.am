pkglib_DATA = append.nsc calc.nsc fft.nsc fitrslt.nsc legend.nsc	\
		math.nsc text-in.nsc

pkglib_SCRIPTS = calc.tcl fft.tcl fitrslt.tcl legend.tcl

EXTRA_DIST = ${pkgdata_DATA} ${pkgdata_SCRIPTS} fft.nsc.in

pkglib_PROGRAMS = fft

fft_SOURCES = fft.c

fft_CFLAGS = -D_FORTIFY_SOURCE

fft.nsc: fft.nsc.in Makefile
	sh ../pathconv.sh $(docdir) $(pkglibdir) $(demodir) fft.nsc.in

pkglib_DATA = append.nsc calc.nsc fft.nsc fitrslt.nsc legend.nsc		\
	math.nsc text-in.nsc cal.nsc tex_equation.nsc ps2gra.nsc	\
	fig2gra.nsc

pkglib_SCRIPTS = calc.tcl fft.tcl fitrslt.tcl legend.tcl fig2gra.rb

EXTRA_DIST = ${pkglib_DATA} ${pkglib_SCRIPTS} fft.nsc.in

pkglib_PROGRAMS = fft

fft_SOURCES = fft.c

fft_CFLAGS = -D_FORTIFY_SOURCE

fft.nsc: fft.nsc.in Makefile
	sh ../pathconv.sh $(docdir) $(pkglibdir) $(demodir) fft.nsc.in

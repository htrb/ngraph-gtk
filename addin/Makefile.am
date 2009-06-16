pkgdata_DATA = append.nsc calc.nsc fft.nsc fitrslt.nsc legend.nsc	\
		math.nsc text-in.nsc

pkgdata_SCRIPTS = calc.tcl fft.tcl fitrslt.tcl legend.tcl

EXTRA_DIST = ${pkgdata_DATA} ${pkgdata_SCRIPTS}

bin_PROGRAMS = nfft

nfft_SOURCES = fft.c

nfft_CFLAGS = -D_FORTIFY_SOURCE

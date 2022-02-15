addindir = $(pkgdatadir)/addin

addins = append.nsc fft.nsc fitrslt.nsc legend.nsc math.nsc		\
	text-in.nsc cal.nsc tex_equation.nsc ps2gra.nsc fig2gra.nsc	\
	timer.nsc fig2gra.rb load_prm.nsc img2gra.nsc

ruby_addins = ruby.nsc spellchecker.rb

if HAVE_RUBY_DEV
  addins += $(ruby_addins)
else
  EXTRA_DIST = $(ruby_addins)
endif

dist_addin_DATA = $(addins)

APP_CFLAGS = 

pkglibexec_PROGRAMS = fft fitrslt legend gimg2gra

fitrslt_SOURCES = fitrslt.c addin_common.c addin_common.h
fitrslt_CFLAGS = $(GTK_CFLAGS) $(APP_CFLAGS)
fitrslt_LDADD = $(GTK_LIBS) $(GLIB_LIBS)

legend_SOURCES = legend.c addin_common.c addin_common.h
legend_CFLAGS = $(GTK_CFLAGS) $(APP_CFLAGS)
legend_LDADD = $(GTK_LIBS) $(GLIB_LIBS)

gimg2gra_SOURCES = gimg2gra.c addin_common.c
gimg2gra_CFLAGS = $(GTK_CFLAGS) $(APP_CFLAGS)
gimg2gra_LDADD = $(GTK_LIBS) $(GLIB_LIBS)

fft_SOURCES = fft.c
fft_LDADD =

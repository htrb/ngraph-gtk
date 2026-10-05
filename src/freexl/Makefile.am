noinst_LTLIBRARIES = libfreexl.la

libfreexl_la_CFLAGS = -I$(top_srcdir) $(MINIZIP_CFLAGS)
libfreexl_la_LDFLAGS = $(MINIZIP_LIBS)

libfreexl_la_SOURCES = freexl.c freexl_xlsx.c freexl_ods.c freexl.h freexl_internals.h

libfreexl_la_LIBADD = -lm

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda

EXTRA_DIST = AUTHORS COPYING README_LICENSE.txt

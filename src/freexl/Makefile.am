noinst_LTLIBRARIES = libfreexl.la

AM_CPPFLAGS = -I$(top_srcdir)

libfreexl_la_SOURCES = freexl.c freexl_xlsx.c freexl_ods.c freexl.h freexl_internals.h

libfreexl_la_LDFLAGS = -version-info 3:0:2 -no-undefined

libfreexl_la_LIBADD = -lm

MOSTLYCLEANFILES = *.gcna *.gcno *.gcda

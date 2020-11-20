HTML_FILE = index.html ngraph.css

EXTRA_DIST = objlist.nsc

htmldir = $(docdir)/html
dist_html_DATA = ${HTML_FILE}
nodist_html_DATA = ngraph.svg

SUBDIRS = img ja

BUILT_SOURCES = $(nodist_html_DATA)

ngraph.svg: $(top_srcdir)/icons/scalable/apps/ngraph.svg
	cp $^ .

HTML_FILE = index.html ngraph.css

EXTRA_DIST = objlist.nsc

htmldir = $(docdir)/html
dist_html_DATA = ${HTML_FILE}
icon_file = ngraph.svg
nodist_html_DATA = $(icon_file)

SUBDIRS = img ja

BUILT_SOURCES = $(nodist_html_DATA)

$(icon_file): $(top_srcdir)/icons/scalable/apps/$(icon_file)
	cp $^ .

CLEANFILES = $(icon_file)

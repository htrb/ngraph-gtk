HTML_FILE = addin.html copyrigh.html data.html gra.html intro.html	\
	math.html ngraph.html object.html operate.html refer.html	\
	script.html text.html tutorial.html

EXTRA_DIST = ${HTML_FILE} object.html.in objlist.nsc

htmljadir = $(docdir)/html_ja

htmlja_DATA = ${HTML_FILE}

object.html: ../../src/ngraph
	cat object.html.in > object.html
	../../src/ngraph -i objlist.nsc >> object.html

SUBDIRS = tutorial img dialogs

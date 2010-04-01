ngraphconfdir = $(sysconfdir)/$(PACKAGE)

HTML_FILE = addin.html copyrigh.html data.html gra.html intro.html	\
	math.html ngraph.html object.html operate.html refer.html	\
	script.html text.html tutorial.html

EXTRA_DIST = ${HTML_FILE} object.html.in objlist.nsc objlist.txt	\
        intro.html.in script.html.in math.html.in ngraph.html.in

htmljadir = $(docdir)/html_ja

htmlja_DATA = ${HTML_FILE}

object.html: ../../src/ngraph object.html.in objlist.nsc
	cat object.html.in > object.html
	../../src/ngraph -i objlist.nsc >> object.html

intro.html: intro.html.in Makefile
	sh ../../pathconv.sh "$(docdir)" "$(pkglibdir)" "$(ngraphconfdir)" intro.html.in

script.html: script.html.in Makefile
	sh ../../pathconv.sh "$(docdir)" "$(pkgdatadir)" "$(ngraphconfdir)" script.html.in

math.html: math.html.in Makefile
	cpp -P math.html.in > math.html

ngraph.html: ngraph.html.in Makefile
	cpp -P ngraph.html.in > ngraph.html

SUBDIRS = tutorial img dialogs

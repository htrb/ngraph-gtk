s!$(libexecdir)/ngraph-gtk!$(bindir)!
s!$(libdir)/ngraph-gtk!$(libdir)!
s!$(datadir)/ngraph-gtk!$(datadir)!
s!${datarootdir}/doc/${PACKAGE_TARNAME}!${prefix}/doc!
s!$(sysconfdir)/$(PACKAGE)!$(sysconfdir)!
s!$(datadir)/pixmaps/$(PACKAGE)!$(datadir)/pixmaps!
s!^LIBS =!LIBS = -lwinspool !

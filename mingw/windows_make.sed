s!${prefix}/share!${prefix}!
s!$(libexecdir)/ngraph-gtk!$(bindir)!
s!$(datadir)/ngraph-gtk!$(datadir)!
s!${datarootdir}/locale!${datarootdir}/share/locale!
s!${datarootdir}/doc/${PACKAGE_TARNAME}!${datarootdir}/doc!
s!$(sysconfdir)/$(PACKAGE)!$(sysconfdir)!
s!$(datadir)/pixmaps/$(PACKAGE)!$(datadir)/share/pixmaps!
s!^LIBS =!LIBS = -lwinspool !

s!${prefix}/share!${prefix}!
s!$(libdir)/ngraph-gtk!$(libdir)!
s!${datarootdir}/locale!${datarootdir}/share/locale!
s!${datarootdir}/doc/${PACKAGE_TARNAME}!${datarootdir}/doc!
s!$(sysconfdir)/$(PACKAGE)!$(sysconfdir)!
s!$(datadir)/pixmaps/$(PACKAGE)!$(datadir)/share/pixmaps!
s!^LIBS =!LIBS = -lwinspool !

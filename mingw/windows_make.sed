s!${prefix}/share!${prefix}!
s!$(libdir)/Ngraph-gtk!$(libdir)!
s!${datarootdir}/locale!${datarootdir}/share/locale!
s!${datarootdir}/doc/${PACKAGE_TARNAME}!${datarootdir}/doc!
s!$(sysconfdir)/$(PACKAGE)!$(sysconfdir)!
s!$(datadir)/pixmaps/$(PACKAGE)!$(datadir)/pixmaps!
s!^LIBS =!LIBS = -lwinspool !

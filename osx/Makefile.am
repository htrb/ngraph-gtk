ICONSET = ngraph.iconset/icon_128x128.png	\
          ngraph.iconset/icon_128x128@2x.png	\
          ngraph.iconset/icon_16x16.png		\
          ngraph.iconset/icon_16x16@2x.png	\
          ngraph.iconset/icon_256x256.png	\
          ngraph.iconset/icon_256x256@2x.png	\
          ngraph.iconset/icon_32x32.png		\
          ngraph.iconset/icon_32x32@2x.png	\
          ngraph.iconset/icon_512x512.png	\
          ngraph.iconset/icon_512x512@2x.png

EXTRA_DIST = $(ICONSET)

if OS_OSX
ngraph.icns: $(ICONSET)
	iconutil -c icns ngraph.iconset

app_data = ngraph.icns

BUILT_SOURCES = $(app_data)
CLEANFILES = $(app_data)
endif

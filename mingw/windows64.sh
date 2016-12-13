#! /bin/sh

PKG_DIR=/`echo $HOMEDRIVE|sed -e's/://'`/ngraph-gtk64
WIN_PATH=/mingw64

ARCHIVE=`ls ngraph-gtk-*.tar.gz | tail -1`
if [ -z "$ARCHIVE" ]
then
    echo "Cannot find an archive file."
    exit
fi

makepkg-mingw -fs

VERSION=`basename $ARCHIVE '.tar.gz' | sed -e 's/ngraph-gtk-//'`
PKGFILE=mingw-w64-x86_64-ngraph-gtk-${VERSION}-1-any.pkg.tar.xz
if [ ! -f "$PKGFILE" ]
then
    echo "cannot find the package file."
    exit
fi

pacman -U $PKGFILE

BINFILES="libatk-1.0-0.dll libbz2-1.dll libcairo-2.dll
libcairo-gobject-2.dll libepoxy-0.dll libexpat-1.dll libffi-6.dll
libfontconfig-1.dll libfreetype-6.dll libgcc_s_seh-1.dll
libgdk_pixbuf-2.0-0.dll libgdk-3-0.dll libgio-2.0-0.dll
libglib-2.0-0.dll libgmodule-2.0-0.dll libgobject-2.0-0.dll
libgraphite2.dll libgsl-19.dll libgslcblas-0.dll libgtk-3-0.dll
libgtksourceview-3.0-1.dll libharfbuzz-0.dll libiconv-2.dll
libintl-8.dll libobjc-4.dll libp11-kit-0.dll libpango-1.0-0.dll
libpangocairo-1.0-0.dll libpangoft2-1.0-0.dll libpangowin32-1.0-0.dll
libpcre-1.dll libpixman-1-0.dll libpng16-16.dll libreadline7.dll
libstdc++-6.dll libtermcap-0.dll libwinpthread-1.dll zlib1.dll
libngraph-0.dll ngraph.exe ngp2"

mkdir $PKG_DIR
for subdir in bin etc lib share
do
    mkdir $PKG_DIR/$subdir/
    case $subdir in
	bin)
	    for i in $BINFILES
	    do
		cp $WIN_PATH/$subdir/$i $PKG_DIR/$subdir/
	    done
	;;
	etc)
	    for i in fonts gtk-3.0 ngraph-gtk
	    do
		cp -r $WIN_PATH/$subdir/$i $PKG_DIR/$subdir/
	    done
	;;
	lib)
	    for i in gdk-pixbuf-2.0 glib-2.0 gtk-3.0 ngraph-gtk
	    do
		cp -r $WIN_PATH/$subdir/$i $PKG_DIR/$subdir/
	    done
	;;
	share)
	    for i in GConf glib-2.0 icons locale themes ngraph-gtk pixmaps
	    do
		cp -r $WIN_PATH/$subdir/$i $PKG_DIR/$subdir/
	    done
	    mkdir $PKG_DIR/$subdir/doc
	    cp -r $WIN_PATH/$subdir/doc/ngraph-gtk $PKG_DIR/$subdir/doc
	;;
    esac
done

# cp src/ngraph.ico $PKG_DIR/share/icons

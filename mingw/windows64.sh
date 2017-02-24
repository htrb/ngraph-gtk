#! /bin/sh

PKG_DIR=ngraph-gtk
WIN_PATH=/mingw

ARCHIVE=`ls ngraph-gtk-*.tar.gz | tail -1`
if [ -z "$ARCHIVE" ]
then
    echo "Cannot find an archive file."
    exit
fi

MO_FILES="gtk30.mo ngraph-gtk.mo"

VERSION=`basename $ARCHIVE '.tar.gz' | sed -e 's/ngraph-gtk-//'`

BINFILES="libatk-1.0-0.dll libbz2-1.dll libcairo-2.dll
libcairo-gobject-2.dll libepoxy-0.dll libexpat-1.dll libffi-6.dll
libfontconfig-1.dll libfreetype-6.dll libgdk_pixbuf-2.0-0.dll
libgdk-3-0.dll libgio-2.0-0.dll libglib-2.0-0.dll libgmodule-2.0-0.dll
libgobject-2.0-0.dll libgraphite2.dll libgsl-19.dll libgslcblas-0.dll
libgtk-3-0.dll libharfbuzz-0.dll libiconv-2.dll libintl-8.dll
libobjc-4.dll libp11-kit-0.dll libpango-1.0-0.dll
libpangocairo-1.0-0.dll libpangoft2-1.0-0.dll libpangowin32-1.0-0.dll
libpcre-1.dll libpixman-1-0.dll libpng16-16.dll libreadline7.dll
libstdc++-6.dll libtermcap-0.dll libwinpthread-1.dll zlib1.dll
libcroco-0.6-3.dll liblzma-5.dll librsvg-2-2.dll libxml2-2.dll
libngraph-0.dll ngraph.exe ngp2"

BINFILES64="libgcc_s_seh-1.dll"
BINFILES32="libgcc_s_dw2-1.dll"

make_zip() {
    echo create win$1 archive.
    if [ -d $PKG_DIR ]
    then
	rm -rf $PKG_DIR
    fi

    mkdir $PKG_DIR
    win_path=$WIN_PATH$1
    for subdir in bin etc lib share
    do
	echo "  copy $subdir."
	mkdir $PKG_DIR/$subdir/
	case $subdir in
	    bin)
		for i in $BINFILES
		do
		    cp $win_path/$subdir/$i $PKG_DIR/$subdir/
		done
		if [ $1 = "64" ]
		then
		    for i in $BINFILES64
		    do
			cp $win_path/$subdir/$i $PKG_DIR/$subdir/
		    done
		else
		    for i in $BINFILES32
		    do
			cp $win_path/$subdir/$i $PKG_DIR/$subdir/
		    done
		fi
		;;
	    etc)
		for i in fonts gtk-3.0 ngraph-gtk
		do
		    cp -r $win_path/$subdir/$i $PKG_DIR/$subdir/
		done
		;;
	    lib)
		for i in gdk-pixbuf-2.0 glib-2.0 gtk-3.0 ngraph-gtk
		do
		    cp -r $win_path/$subdir/$i $PKG_DIR/$subdir/
		done
		;;
	    share)
		for i in GConf glib-2.0 icons themes ngraph-gtk pixmaps
		do
		    cp -r $win_path/$subdir/$i $PKG_DIR/$subdir/
		done

		locale_dir="$PKG_DIR/$subdir/locale"
		mkdir "$locale_dir"
		for i in $win_path/$subdir/locale/*
		do
		    if [ -d "$i" ]
		    then
			modir=`basename "$i"`/LC_MESSAGES
			mkdir -p "$locale_dir/$modir"
			for mo in $MO_FILES
			do
			    mofile="$i"/LC_MESSAGES/$mo
			    if [ -f $mofile ]
			    then
				cp $mofile $locale_dir/$modir/
			    fi
			done
		    else
			cp "$i" $PKG_DIR/$subdir/locale/
		    fi
		done
		mkdir $PKG_DIR/$subdir/doc
		cp -r $win_path/$subdir/doc/ngraph-gtk $PKG_DIR/$subdir/doc
		;;
	esac
    done

    archive=ngraph-gtk-${VERSION}-win$1.zip
    if [ -f $archive ]
    then
	rm $archive
    fi
    echo "  archiving."
    zip -qr9 $archive $PKG_DIR
}

for arch in 32 64
do
    make_zip $arch
done

#! /bin/sh

TMPFILE=ngraph_tmp
PKG_DIR=$HOMEDRIVE/ngraph-gtk
WIN_PATH=mingw

HAVE_RUBY="0"

DEFS="-D _USE_32BIT_TIME_T -D_FORTIFY_SOURCE -DGTK_DISABLE_DEPRECATED=1 -DGDK_DISABLE_DEPRECATED=1 -DGDK_PIXBUF_DISABLE_DEPRECATED=1 -DG_DISABLE_DEPRECATED=1 -DGTK_DISABLE_SINGLE_INCLUDES=1 -DG_DISABLE_SINGLE_INCLUDES=1 -DGDK_PIXBUF_DISABLE_SINGLE_INCLUDES=1 -DGSL_DISABLE_DEPRECATED=1"

CCOPT="-march=i686 -Wall -Wextra -Wpointer-arith -Wstrict-aliasing -Wno-unused-parameter -Wno-missing-field-initializers -Wdeprecated-declarations -g"

if which ruby
then
    RUBY_EXE=`which ruby`
    RUBY_BIN=`dirname "$RUBY_EXE"`
    RUBY_LIB=`dirname "$RUBY_BIN"`/lib
    RUBY_DLL=${RUBY_LIB}/libmsvcrt-ruby200.dll.a
    if [ -f $RUBY_DLL ]
    then
	HAVE_RUBY=1
    fi
fi

CFLAGS="$DEFS $CCOPT" ./configure --prefix=$PKG_DIR --libexecdir=$PKG_DIR/lib

for makefile in `find -name Makefile`
do
    sed -f $WIN_PATH/windows_make.sed $makefile > $TMPFILE
    mv $TMPFILE $makefile
done

for demo in demo/*.ngp.in
do
    sed -f $WIN_PATH/windows_demo.sed $demo > $TMPFILE
    mv $TMPFILE $demo
done

make

(cd initfile; cp Ngraph.ini.win Ngraph.ini; cp NgraphUI.xml.win NgraphUI.xml)

make install

mkdir -p $PKG_DIR/share/icons $PKG_DIR/lib/plugins $PKG_DIR/lib/ruby

cp /mingw/bin/*.dll             $PKG_DIR/bin
cp -r /mingw/share/locale       $PKG_DIR/share
cp -r /mingw/share/themes       $PKG_DIR/share
cp -r /mingw/share/glib-2.0     $PKG_DIR/share
cp -r /mingw/share/fontconfig   $PKG_DIR/share
cp -r /mingw/etc/gtk-3.0        $PKG_DIR/etc
cp -r /mingw/etc/pango          $PKG_DIR/etc
cp -r /mingw/etc/fonts          $PKG_DIR/etc
cp -r /mingw/lib/gtk-3.0        $PKG_DIR/lib
cp -r /mingw/lib/gdk-pixbuf-2.0 $PKG_DIR/lib

HICOLOR_ICONS="/mingw/share/icons/hicolor"
if [ -d "$HICOLOR_ICONS" ]
then
    cp -r "$HICOLOR_ICONS" $PKG_DIR/share/icons
fi

#cp $WIN_PATH/gtkrc         $PKG_DIR/share/themes/MS-Windows/gtk-2.0
#cp $WIN_PATH/gtkrc         $PKG_DIR/share/themes/Raleigh/gtk-2.0
cp $WIN_PATH/pango.aliases $PKG_DIR/etc/pango
cp $WIN_PATH/associate.bat $PKG_DIR
cp $WIN_PATH/echo.nsc      $PKG_DIR
cp src/ngraph.ico          $PKG_DIR/share/icons

if [ $HAVE_RUBY = "1" ]
then
    cp plugins/ruby/ngraph.so           $PKG_DIR/lib/ruby
    cp plugins/ruby/lib/ngraph.rb.win   $PKG_DIR/lib/ruby/ngraph.rb
    cp plugins/ruby/lib/ngraph/*.rb     $PKG_DIR/lib/ruby/
    mv $PKG_DIR/bin/plugins/libruby.dll $PKG_DIR/lib/plugins
    rm -rf $PKG_DIR/bin/plugins
fi

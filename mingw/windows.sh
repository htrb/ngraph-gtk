#! /bin/sh

TMPFILE=ngraph_tmp
PKG_DIR=$HOMEDRIVE/ngraph-gtk
WIN_PATH=mingw

(cd $WIN_PATH; windres -o ../src/windows_resource.o windows_resource.rc)

./configure --prefix=$PKG_DIR --libexecdir=$PKG_DIR/lib

for makefile in `find -name Makefile`
do
    sed -f $WIN_PATH/windows_make.sed $makefile > $TMPFILE
    mv $TMPFILE $makefile
done

sed 's/ngraph_LDADD =/ngraph_LDADD = windows_resource.o/' src/Makefile > $TMPFILE
mv $TMPFILE src/Makefile

for demo in demo/*.ngp.in
do
    sed -f $WIN_PATH/windows_demo.sed $demo > $TMPFILE
    mv $TMPFILE $demo
done

make

(cd initfile; cp Ngraph.ini.win Ngraph.ini; cp NgraphUI.xml.win NgraphUI.xml)

make install

mkdir -p $PKG_DIR/share/icons

rm $PKG_DIR/bin/terminal.exe

cp /mingw/bin/*.dll       $PKG_DIR/bin
cp -r /mingw/share/locale $PKG_DIR/share
cp -r /mingw/share/themes $PKG_DIR/share
cp -r /mingw/etc/gtk-2.0  $PKG_DIR/etc
cp -r /mingw/etc/pango    $PKG_DIR/etc
cp -r /mingw/lib/gtk-2.0  $PKG_DIR/lib

HICOLOR_ICONS="/mingw/share/icons/hicolor"
if [ -d "$HICOLOR_ICONS" ]
then
    cp -r "$HICOLOR_ICONS" $PKG_DIR/share/icons
fi

cp $WIN_PATH/gtkrc         $PKG_DIR/share/themes/MS-Windows/gtk-2.0
cp $WIN_PATH/pango.aliases $PKG_DIR/etc/pango
cp $WIN_PATH/ngraph.ico    $PKG_DIR/share/icons
cp $WIN_PATH/associate.bat $PKG_DIR
cp $WIN_PATH/echo.nsc      $PKG_DIR

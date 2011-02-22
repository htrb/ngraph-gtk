#! /bin/sh

case $# in
    "5")
	DOCDIR=$1
	shift
	LIBDIR=$1
	shift
	CONFDIR=$1
	shift
	PIXMAPDIR=$1
	shift
	;;
    "4")
	DOCDIR=$1
	shift
	LIBDIR=$1
	shift
	CONFDIR=$1
	shift
	;;
    "3")
	DOCDIR=$1
	shift
	LIBDIR=$1
	shift
	;;
    "2")
	DOCDIR=$1
	shift
	;;
    *)
	exit
	;;
esac

TARGET=`basename $1 .in`

cat $1 | sed -e "s!DOCDIRDEF!$DOCDIR!g" \
             -e "s!LIBDIRDEF!$LIBDIR!g" \
             -e "s!PIXMAPDIRDEF!$PIXMAPDIR!g" \
             -e "s!CONFDIRDEF!$CONFDIR!g" > $TARGET

echo $CONFDIR
#! /bin/sh

DOCDIR=$1
shift

LIBDIR=$1
shift

if [ "X$2" != "X" ]
then
    CONFDIR=$1
    shift
fi

TARGET=`basename $1 .in`

cat $1 | sed -e s!DOCDIRDEF!$DOCDIR!g \
             -e s!LIBDIRDEF!$LIBDIR!g \
             -e s!CONFDIRDEF!$CONFDIR!g > $TARGET

echo $CONFDIR
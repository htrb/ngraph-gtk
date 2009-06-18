#! /bin/sh

DOCDIR=`echo $1 | sed 's/\//\\\\\//g'`

shift

LIBDIR=`echo $1 | sed 's/\//\\\\\//g'`

shift

if [ "X$2" != "X" ]
then
    CONFDIR=`echo $1 | sed 's/\//\\\\\//g'`
    shift
fi

TARGET=`basename $1 .in`

cat $1 | sed -e s/DOCDIRDEF/$DOCDIR/g \
             -e s/LIBDIRDEF/$LIBDIR/g \
             -e s/CONFDIRDEF/$CONFDIR/g > $TARGET

echo $CONFDIR
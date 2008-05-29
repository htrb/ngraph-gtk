#! /bin/sh

DOCDIR=`echo $1 | sed 's/\//\\\\\//g'`

shift

DATADIR=`echo $1 | sed 's/\//\\\\\//g'`

shift

if [ "X$2" != "X" ]
then
  DEMODIR=`echo $1 | sed 's/\//\\\\\//g'`
  shift
fi

TARGET=`basename $1 .in`

cat $1 | sed -e s/DOCDIRDEF/$DOCDIR/g \
             -e s/DATADIRDEF/$DATADIR/g \
             -e s/DEMODIRDEF/$DEMODIR/g > $TARGET

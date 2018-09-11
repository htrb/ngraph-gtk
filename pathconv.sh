#! /bin/sh

if [ $# -ne 7 ]
then
    echo "error: wrong number of arguments ($0)."
    exit
fi

BINDIR=$1
shift
DOCDIR=$1
shift
LIBEXECDIR=$1
shift
LIBDIR=$1
shift
DATADIR=$1
shift
CONFDIR=$1
shift

TARGET=`basename $1 .in`

LC_ALL=C
export LC_ALL

cat $1 | sed -e "s!BINDIRDEF!$BINDIR!g" \
             -e "s!DOCDIRDEF!$DOCDIR!g" \
             -e "s!LIBEXECDIRDEF!$LIBEXECDIR!g" \
             -e "s!LIBDIRDEF!$LIBDIR!g" \
             -e "s!DATADIRDEF!$DATADIR!g" \
             -e "s!CONFDIRDEF!$CONFDIR!g" > $TARGET

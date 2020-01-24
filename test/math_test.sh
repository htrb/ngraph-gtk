#! /bin/sh
. ./test.sh
LANG=ja_JP.utf8 $NGRAPH -i math_test.nsc math_test_old.dat math_test_new.dat

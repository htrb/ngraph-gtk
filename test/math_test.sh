#! /bin/sh
. ./test.sh
LC_ALL=C.UTF-8 $NGRAPH -i math_test.nsc math_test_old.dat math_test_new.dat

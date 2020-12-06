#! /bin/sh

. ./test.sh

skip_on_windows

LANG=C.UTF-8 $NGRAPH -i math_test.nsc math_test_utf8.dat

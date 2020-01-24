#! /bin/sh

. ./test.sh

skip_on_windows
skip_on_ubuntu

LANG=ja_JP.utf8 $NGRAPH -i math_test.nsc math_test_utf8.dat

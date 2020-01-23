#! /bin/sh

if [ -n "$MSYSTEM" ]
then
	exit 77			# SKIP the test on Windows.
fi
LANG=ja_JP.utf8 ../src/ngraph -i math_test.nsc math_test_utf8.dat

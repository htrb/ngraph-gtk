#! /bin/sh

NGRAPH=../src/ngraph

skip_on_windows() {
	if [ -n "$MSYSTEM" ]
	then
		exit 77		# SKIP the test on Windows.
	fi
}

skip_on_ubuntu() {
	if uname -a | grep -q Ubuntu
	then
		exit 77		# SKIP the test on Ubuntu.
	fi
}

exec_script() {
	script=`basename $1 .sh`.nsc
	LC_ALL=C.UTF-8 $NGRAPH -i $script
}

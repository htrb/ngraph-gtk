require 'mkmf'

$CFLAGS = "-Wall -O2 -I../../src -L../../src/.libs"
$LDFLAGS = "-L../../src/.libs"
have_header("ngraph.h")
have_library("ngraph", "ngraph_get_object")
create_makefile("ngraph")

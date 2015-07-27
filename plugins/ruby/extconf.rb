prefix = ARGV.shift

require 'mkmf'

$CFLAGS = "-Wall -O2"
$configure_args["--vendor"] = true unless (prefix =~ /.*local.*/)

dir_config("ngraph")
have_header("ngraph.h")
have_library("ngraph", "ngraph_get_object")
create_makefile("ngraph")

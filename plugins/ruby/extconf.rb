if (ARGV.size > 2)
	prefix = ARGV.shift
else
	prefix = ""
end

require 'mkmf'

$configure_args["--vendor"] = true unless (prefix =~ /.*local.*/)

dir_config("ngraph")
have_header("ngraph.h")
have_library("ngraph", "ngraph_get_root_object")
have_library("ngraph-0", "ngraph_get_root_object")
create_makefile("ngraph")

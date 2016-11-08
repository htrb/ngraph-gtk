# calc.nsc written by S. ISHIZAKA. 1997/12
#
# This script assists plotting a mathematical function.
#
# The simplest add-in script using an external program.
#
# Description: _Calc...,making a data file,

require 'ngraph'
sys = Ngraph::System[0]
script = sys.temp_file
system("#{sys.lib_dir}/calc #{script}")
if (FileTest.size?(script))
  Ngraph::Shell.new {|shell|
    shell.shell(script)
  }
  Ngraph::Menu[0].clear
end
sys.unlink_temp_file

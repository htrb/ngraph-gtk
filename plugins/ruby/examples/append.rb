# append.nsc written by S. ISHIZAKA. 1997/12
#
# This script reads and appends a NGP-file without clearing the present graph.
#
# This script uses rather special techniques. 
# We don't recommend to edit this script.
#
# Description: _Append...,append NGP file,

require 'ngraph'

file = ""
if (ARGV.size > 0)
  file = ARGV[0]
else
  Ngraph::Dialog.new {|dialog|
    file = dialog.get_open_file('ngp')
  }
end
exit unless (FileTest.readable?(file))
system = Ngraph::System[0]
# hide exsisting 'axis', 'axisgrid' and 'file' instances
system.hide_instance("axis", "axisgrid", "file")
# execute file
Ngraph::Shell.new {|shell| shell.shell(file)}
# tighten connection between new instances
[Ngraph::File, Ngraph::Axis, Ngraph::Axisgrid].each {|obj|
  obj.each {|inst| inst.tight }
}
# recover hidden instances
system.recover_instance("axis", "axisgrid", "file")
# delete closed GRA instance
gra = Ngraph::Gra.current
if (gra && gra.gc == -1)
  gra.del
end
menu = Ngraph::Menu[0]
menu.clear
menu.modified = true


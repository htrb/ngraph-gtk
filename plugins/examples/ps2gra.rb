# Description: import _PS...,Import PostScript image,ps
# Description: import _EPS...,Import Encapsulated PostScript image,eps
# Description: import _PDF...,Import Portable Document Format image,pdf

require 'ngraph'

PSTOEDIT_COMMAND = "pstoedit"
PSTOEDIT_OPTION = "-q -flat 0.1 -nc -ssp -dt"

sys = Ngraph::System[0]
menu = Ngraph::Menu[0]

require "#{sys.data_dir}/addin/fig2gra.rb"

ps_file = Ngraph::Dialog.new {|dialog|
  dialog.get_open_file(ARGV[0])
}
exit unless (ps_file)
unless (FileTest.readable?(ps_file))
  Ngraph.err_puts("cannot open $ps_file")
  exit
end

gra_file = "#{Dir.pwd}/#{File.basename(ps_file, '.' + ARGV[0])}.gra"
if (FileTest.exist?(gra_file))
  response = Ngraph::Dialog.new {|dialog|
    dialog.yesno("The file '#{gra_file}' already exists.\nDo you want to replace it\?")
  }
  exit if (response != 1)
end

fig_file = sys.temp_file
cmd = %Q!#{PSTOEDIT_COMMAND} #{PSTOEDIT_OPTION} -f fig "#{ps_file}" "#{fig_file}"!
system(cmd)
unless (FileTest.readable?(fig_file))
  Ngraph.err_puts("Faital error occurred while execute pstoedit.")
  sys::unlink_temp_file(fig_file)
  exit
end

fig2gra = Fig2Gra.new
fig2gra.convert(fig_file, gra_file)

sys.unlink_temp_file(fig_file)
File.delete(*Dir.glob("#{fig_file}*"))

unless (FileTest.readable?(gra_file))
  Ngraph.err_puts("Fatal error occurred while executing fig2gra.")
  exit
end

exit if (Ngraph::Merge[gra_file].size > 0)

merge = Ngraph::Merge.new
merge.file = gra_file

if (menu)
  menu.draw
  menu.unfocus
  menu.focus(merge)
  menu.modified = true
end

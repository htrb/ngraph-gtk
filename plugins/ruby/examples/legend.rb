# This script generates a legend box.
#
# Please edit the following X, Y, and WIDTH variables as you like.
#
# Description: _Legend...,automatic legend generator,

require 'ngraph'

X = 5000      # top-left of the legend to be inserted.
Y = 5000      # top-left of the legend to be inserted.
WIDTH = 2000  # width of "type" field.

if (Ngraph::File.size < 1)
  Ngraph::err_puts("No data file.")
  exit
end

sys = Ngraph::System[0]
datalist = sys.temp_file
File.open(datalist, "w") {|f|
  f.puts(Ngraph::File.size)
  Ngraph::File.each {|file|
    f.puts(file.file)
    f.puts(file.id)
    f.puts(file.hidden.to_s)
    f.puts(file.x)
    f.puts(file.y)
    f.puts(Ngraph::File::Type[file.type])
    f.puts(file.mark_type)
    f.puts(file.mark_size)
    f.puts(file.line_width)
    f.puts(file.line_style.join(" "))
    f.puts(file.r)
    f.puts(file.g)
    f.puts(file.b)
    f.puts(file.r2)
    f.puts(file.g2)
    f.puts(file.b2)
    f.puts(file.math_x)
    f.puts(file.math_y)
  }
}

script = sys.temp_file
system("#{sys.lib_dir}/legend", "-x", X.to_s, "-y", Y.to_s, "-w", WIDTH.to_s, datalist, script)

if (FileTest.size?(script))
  Ngraph::Shell.new {|shell|
    shell.shell(script)
  }
  gra = Ngraph::Gra.current
  if (gra)
    gra.clear
    gra.draw
    gra.flush
  end
end
sys.unlink_temp_file(datalist)
sys.unlink_temp_file(script)

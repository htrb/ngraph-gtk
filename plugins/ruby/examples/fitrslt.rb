# fitrslt.nsc written by S. ISHIZAKA. 1999/04
#
# This script generates a legend box.
#
# Please edit the following X, Y, and WIDTH variables as you like.
#
# Description: _Fit result...,insert fitting results as legend-text,

require 'ngraph'

X = 5000      # top-left of the legend to be inserted.
Y = 5000      # top-left of the legend to be inserted.

fitdata = []
sys = Ngraph::System[0]

Ngraph::File.each {|file|
  fitdata.push(file) if (file.type == Ngraph::File::Type::FIT && file.fit)
}

if (fitdata.size < 1)
  Ngraph::err_puts("No fitting data.")
  exit
end

datalist = sys.temp_file
File.open(datalist, "w") {|f|
  f.puts(fitdata.size.to_s)
  fitdata.each {|file|
    f.puts(file.id.to_s)
    f.puts(file.file)

    fit = file.fit
    f.puts(fit.id)
    f.puts(Ngraph::Fit::Type[fit.type])
    f.puts(fit.poly_dimension)
    f.puts(fit.user_func)
    f.puts(fit.prm00)
    f.puts(fit.prm01)
    f.puts(fit.prm02)
    f.puts(fit.prm03)
    f.puts(fit.prm04)
    f.puts(fit.prm05)
    f.puts(fit.prm06)
    f.puts(fit.prm07)
    f.puts(fit.prm08)
    f.puts(fit.prm09)
  }
}

script = sys.temp_file
system("#{sys.lib_dir}/fitrslt -x #{X} -y #{Y} #{datalist} #{script}")
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
sys.unlink_temp_file(script)
sys.unlink_temp_file(datalist)

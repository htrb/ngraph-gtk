#
# fft.nsc written by S. ISHIZAKA. 1999/07
#
# Description: _FFT...,Fast Fourier Transform,

sys = Ngraph::System[0]
menu = Ngraph::Menu[0]
dialog = Ngraph::Dialog.new

source_file = dialog.get_open_file
unless source_file
  dialog.del
  exit
end

dest_file = dialog.get_save_file("dat", "_fft_.dat")
unless dest_file
  dialog.del
  exit
end

cmd = %Q!#{sys.lib_dir}/fft "#{source_file}" "#{dest_file}"!
unless (system(cmd))
  Ngraph::err_puts("some errors are occurred while calculation.")
  dialog.del
  exit
end

file = Ngraph::File.new
file.x = 1
file.y = 2
file.file = dest_file
file.type = Ngraph::File::Type::LINE
file.b = 255

file = Ngraph::File.new
file.file = dest_file
file.x = 1
file.y = 3
file.type = Ngraph::File::Type::LINE
file.r = 255

file = Ngraph::File.new
file.file = dest_file
file.x = 1
file.y = 2
file.type = Ngraph::File::Type::STAIRCASE_X
file.math_y = 'SQRT(%02^2+%03^2)'

menu.modified = true
dialog.del
menu.clear

require 'ngraph'

def show_usage
  puts("ngp2 version 1.10")
  puts("usage: ngp2 [option] ngpfile ...")
  puts
  puts("    -I            ignore path")
  puts("    -a            auto scale")
  puts("    -A            clear scale and auto scale")
  puts("    -c            change directory")
  puts("    -d dpi        set dot per inch")
  puts
  puts("    -ps, -ps3     output PostScript level 3")
  puts("    -ps2          output PostScript level 2")
  puts("    -eps, -eps3   output Encapsulated PostScript level 3")
  puts("    -eps2         output Encapsulated PostScript level 2")
  puts("    -pdf          output Portable Document Format")
  puts("    -svg, -svg1.1 output Scalable Vector Graphics version 1.1")
  puts("    -svg1.2       output Scalable Vector Graphics version 1.2")
  puts("    -png          output Portable Network Graphics")
  puts("    -gra          output GRA (default)")
  puts("    -             output GRA to stdout")
  puts("    -h, --help    show this usage")
  puts
  exit
end

ngp2 = Ngraph::Ngp2.new
format = nil
while (arg = ARGV.shift)
  case arg
  when "-I"
    ngp2.ignore_path = true
  when "-a"
    ngp2.auto_scale = Ngraph::Ngp2::AUTO_SCALE
  when "-A"
    ngp2.auto_scale = Ngraph::Ngp2::AUTO_SCALE_FORCE
  when "-c"
    ngp2.chdir = true
  when "-d"
    ngp2.dpi = ARGV.shift.to_i
  when "-h", "--help"
    show_usage
  when "-ps", "-ps3"
    format = Ngraph::Gra2cairofile::Format::PS3
  when "-ps2"
    format = Ngraph::Gra2cairofile::Format::PS2
  when "-eps", "-eps3"
    format = Ngraph::Gra2cairofile::Format::EPS3
  when "-eps2"
    format = Ngraph::Gra2cairofile::Format::EPS2
  when "-pdf"
    format = Ngraph::Gra2cairofile::Format::PDF
  when "-svg", "-svg1.1"
    format = Ngraph::Gra2cairofile::Format::SVG1_1
  when "-svg1.2"
    format = Ngraph::Gra2cairofile::Format::SVG1_2
  when "-png"
    format = Ngraph::Gra2cairofile::Format::PNG
  when "-gra"
  when "-"
    format = "-"
  when /-.*/
    puts("unrecognized option '#{arg}'")
  else
    ARGV.unshift(arg)
    break
  end
end

show_usage if (ARGV.size < 1)

ARGV.each {|ngp_file|
  puts("Drawing #{ngp_file}.") if (format != "-")
  ngp2.load_ngp(ngp_file)
  data = ngp2.convert(format)
  print(data) if (format != "-")
}

puts

class Ngp2
  NULL_DEVICE = "/dev/null"
  AUTO_SCALE = 1
  AUTO_SCALE_FORCE = 2

  attr_accessor :ign_path, :auto_scale, :format, :chdir, :dpi

  def initialize
    @ign_path = false
    @auto_scale = nil
    @format = "-gra"
    @chdir = false
    @dpi = 86

    @pwd = Dir.pwd
    @menu = Ngraph::Menu.new
    @system = Ngraph::System[0]
  end

  def clear_viewer
    Ngraph::Draw.derive(true).each {|obj|
      obj.del("0-!")
    }

    Ngraph::Gra.del("viewer")
  end

  def copy_viewer(gra)
    viewer = Ngraph::Gra["viewer"][-1]
    gra.copy(viewer) if (viewer)
    gra.name = nil
  end

  def suppress_fitting_messages
    Ngraph::Fit.each {|fit| fit.display = false }
  end

  def save_image(format, file)
    gra2cairofile = Ngraph::Gra2cairofile.new
    gra = Ngraph::Gra.new
    copy_viewer(gra)

    gra2cairofile.format = format
    gra2cairofile.file = file
    gra2cairofile.dpi = @dpi

    suppress_fitting_messages

    gra.device = gra2cairofile
    gra.open
    gra.draw
    gra.close
    gra.del
    gra2cairofile.del
  end

  def save_gra(file)
    gra2cairofile = Ngraph::Gra2cairofile.new
    gra2cairofile.file = NULL_DEVICE
    dummy = Ngraph::Gra.new
    dummy.device = gra2cairofile
    dummy.open

    gra2file = Ngraph::Gra2file.new
    gra2file.file = file
    gra = Ngraph::Gra.new
    copy_viewer(gra)

    suppress_fitting_messages

    gra.device = gra2file
    gra.open
    gra.draw
    gra.close
    gra.del
    gra2file.del

    dummy.close
    dummy.del
    gra2cairofile.del
  end

  def auto_scale
    return unless (@auto_scale)

    files = Ngraph::File.to_a
    Ngraph::Axis.each {|axis|
      case @auto_scale
      when AUTO_SCALE_FORCE
        axis.clear
        axis.auto_scale(files)
      when AUTO_SCALE
        axis.auto_scale(files)
      end
    }
  end

  def convert(ngp_file)
    return unless (FileTest.readable?(ngp_file))

    clear_viewer

    ngp_name = File.basename(ngp_file, ".ngp")
    path_name = File.dirname(ngp_file)

    @menu.fullpath_ngp = ngp_file if (@menu)

    puts("Drawing #{ngp_file}.") if (@format != "-")

    Ngraph::Shell.new {|shell|
      shell.shell(ngp_file)
    }

    Ngraph::File.each {|file| file.file = file.basename } if (@ign_path)
    Dir.chdir(path_name) if (@chdir)
    auto_scale
    tmpfile = @system.temp_file

    case @format
#======================================================================
# options
#
# tmpfile : temporary file (automatically deleted).
# ngp_name : base name of the ngp file.

    when "-ps", "-ps3"
      save_image(Ngraph::Gra2cairofile::Format::PS3, "#{ngp_name}.ps")
    when "-ps2"
      save_image(Ngraph::Gra2cairofile::Format::PS2, "#{ngp_name}.ps")
    when "-eps", "-eps3"
      save_image(Ngraph::Gra2cairofile::Format::EPS3, "#{$ngp_name}.eps")
    when "-eps2"
      save_image(Ngraph::Gra2cairofile::Format::EPS2, "#{ngp_name}.eps")
    when "-pdf"
      save_image(Ngraph::Gra2cairofile::Format::PDF, "#{ngp_name}.pdf")
    when "-svg", "-svg1.1"
      save_image(Ngraph::Gra2cairofile::Format::SVG1_1, "#{ngp_name}.svg")
    when "-svg1.2"
      save_image(Ngraph::Gra2cairofile::Format::SVG1_2, "#{ngp_name}.svg")
    when "-png"
      save_image(Ngraph::Gra2cairofile::Format::PNG, "#{ngp_name}.png")
    when "-"
      save_gra(tmpfile)
      IO.foreach(tmpfile) {|l| print(l) }
    else
      save_gra("#{ngp_name}.gra")
    end
#======================================================================
    @system.unlink_temp_file(tmpfile)

    Dir.chdir(@pwd) if (@chdir)
  end
end

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
  puts("    -h, --help    show this usage")
  puts
  Ngraph::System[0].del
end

ngp2 = Ngp2.new
opt_index = []
while (arg = ARGV.shift)
  case arg
  when "-I"
    ngp2.ign_path = true
  when "-a"
    ngp2.auto_scale = Ngp2::AUTO_SCALE
  when "-A"
    ngp2.auto_scale = Ngp2::AUTO_SCALE_FORCE
  when "-c"
    ngp2.chdir = true
  when "-d"
    ngp2.dpi = ARGV.shift.to_i
  when "-h", "--help"
    show_usage
  when /-.*/
    ngp2.format = arg
  else
    ARGV.unshift(arg)
    break
  end
end

show_usage if (ARGV.size < 1)

ARGV.each {|ngp_file|
  ngp2.convert(ngp_file)
}

puts

Ngraph::System[0].del

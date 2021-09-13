module Ngraph
  class Ngp2
    NULL_DEVICE = "/dev/null"
    AUTO_SCALE = 1
    AUTO_SCALE_FORCE = 2

    attr_accessor :ignore_path, :auto_scale, :chdir, :dpi

    def initialize
      @ignore_path = false
      @auto_scale = nil
      @chdir = false
      @dpi = 86

      @ngp_name = ""
      @path_name = "."

      @menu = Ngraph::Menu[0] || Ngraph::Menu.new
      @system = Ngraph::System[0]
    end

    def clear_viewer
      [Ngraph::Draw, Ngraph::Fit, Ngraph::Parameter].each {|nobj|
	nobj.derive(true).each {|obj|
	  obj.del("0-!")
	}
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
      Ngraph::Axis.each {|axis|
        axis.adjust
      }
    end

    def load_ngp(ngp_file)
      return unless (FileTest.readable?(ngp_file))

      clear_viewer

      @ngp_name = ::File.basename(ngp_file, ".ngp")
      @path_name = ::File.dirname(ngp_file)

      @menu.fullpath_ngp = ngp_file if (@menu)

      Ngraph::Shell.new {|shell|
        shell.shell(ngp_file)
      }

      Ngraph::File.each {|file| file.file = ::File.basename(file) } if (@ignore_path)

    end

    def convert(format = Ngraph::Gra2cairofile::Format::EPS3, img_file = nil)
      pwd = Dir.pwd

      Dir.chdir(@path_name) if (@chdir)
      auto_scale
      rval = nil

      case format
      when Ngraph::Gra2cairofile::Format::PS3
        save_image(format, img_file || "#{@ngp_name}.ps")
      when Ngraph::Gra2cairofile::Format::PS2
        save_image(format, img_file || "#{@ngp_name}.ps")
      when Ngraph::Gra2cairofile::Format::EPS3
        save_image(format, img_file || "#{@ngp_name}.eps")
      when Ngraph::Gra2cairofile::Format::EPS2
        save_image(format, img_file || "#{@ngp_name}.eps")
      when Ngraph::Gra2cairofile::Format::PDF
        save_image(format, img_file || "#{@ngp_name}.pdf")
      when Ngraph::Gra2cairofile::Format::SVG1_1
        save_image(format, img_file || "#{@ngp_name}.svg")
      when Ngraph::Gra2cairofile::Format::SVG1_2
        save_image(format, img_file || "#{@ngp_name}.svg")
      when Ngraph::Gra2cairofile::Format::PNG
        save_image(format, img_file || "#{@ngp_name}.png")
      when "-"
        tmpfile = @system.temp_file
        save_gra(tmpfile)
        rval = IO.readlines(tmpfile)
        @system.unlink_temp_file(tmpfile)
      else
        save_gra(img_file || "#{@ngp_name}.gra")
      end

      Dir.chdir(pwd) if (@chdir)
      rval
    end
  end
end

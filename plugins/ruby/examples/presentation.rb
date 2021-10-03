# -*- coding: utf-8 -*-
Encoding.default_external = Encoding.default_internal = Encoding::UTF_8
require 'ngraph'

class Presentation
  RGBA = Struct.new(:r, :g, :b, :a)
  TITLE_FG   = RGBA.new(  0,   0,   0, 255)
  TITLE_BG   = RGBA.new(255, 245, 167, 255)
  TITLE_LINE = RGBA.new(  0, 204, 129, 255)
  ENUM_COLOR = [  0,  78, 162].inject("") {|r, v| r + sprintf("%02x", v)}
  ENUM_CHAR  = "✵➢✲✔".chars
  LINE_SPACE = "%N{4}"

  MODE = [
    :CENTER,
    :LOAD,
    :LIST,
    :SUB_LIST,
    :CENTER_LIST,
    :VERB,
    :TEXT,
    :QUOTE,
    :ENUM,
    :TITLE,
    :COMMAND,
    :SLEEP,
    :OPACITY,
    :LINE_HEIGHT,
    :INCLUDE,
    :INCLUDE_RAW,
    :VERB_INCLUDE,
    :VERB_INCLUDE_N,
    :VSPACE,
  ]

  PDF_MODE = [
    :PDF,
    :PDF_EXPAND
  ]

  (PDF_MODE + MODE).each_with_index {|sym, i|
    const_set(sym, i)
  }

  PDF_PREFIX = "_npresentation_tmp_"

  attr_accessor :slide_show, :slide_show_wait, :pdf_out

  def initialize
    @page = 0
    @title_text_size = 4000
    @list_text_size = 3300
    @sub_list_text_size = 3000
    @center_text_size = 6200
    @title_x = 1000
    @title_y = 3400
    @title_h = 0
    @ofst_x = 400
    @ofst_y = 400
    @page_width = 29700
    @page_height = 21000
    @page_top_margin= 0
    @page_left_margin = 0
    @use_opacity = true
    @line_height = 100

    @pdf_out = false
    @total_time = nil

    @slide_show = false
    @slide_show_wait = 10

    @path = "."
    @enum = 0

    @pdf_file = nil
  end

  def pdf_filename=(filename)
    @pdf_file = filename
  end

  def save_image(file)
    Ngraph::Gra2cairofile.new {|gra2cairofile|
      gra2cairofile.format = Ngraph::Gra2cairofile::Format::PDF
      gra2cairofile.file = file
      gra2cairofile.dpi = 300
      gra2cairofile.use_opacity = @use_opacity

      Ngraph::Gra.new {|gra|
        gra.copy(@gra)
        gra.device = gra2cairofile

        gra.open
        gra.draw
        gra.close
      }
    }
  end

  def wait_action
    if (@slide_show)
      Ngraph.sleep(@slide_show_wait)
      "Page_Down"
    else
      @gra2gtk.wait_action
    end
  end

  def load_graph(file)
    Ngraph::Shell.new {|shell|
      shell::shell(@path + "/" + file)
    }

    Ngraph::Gra.del("viewer")
    Ngraph::Fit.each {|fit|
      fit.display = false
    }
    Ngraph::Data.each {|data|
      data.file = @path + "/" + data.file if (data.file && data.file[0] != "/")
    }
    Ngraph::Merge.each {|data|
      data.file = @path + "/" + data.file if (data.file && data.file[0] != "/")
    }
  end

  def clear
    @enum = 0
    [Ngraph::Draw, Ngraph::Fit, Ngraph::Parameter].each {|nobj|
      nobj.derive(true).each {|obj|
	obj.del("0-!")
      }
    }

    Ngraph::Gra.del("viewer")
  end

  def center_add(str)
    text = Ngraph::Text.new
    text.name = "CENTER"
    text.text = (str == "　") ? " " : str
    text.pt = @center_text_size

    margin = 800
    height = 0
    Ngraph::Text["CENTER"].each {|text|
      bbox = text.bbox
      text.x = (@page_width - bbox[2] + bbox[0]) / 2
      text.y = @ofst_y
      height += bbox[3] - bbox[1] + margin
    }

    ofst_y = (@page_height - height) / 2
    Ngraph::Text["CENTER"].each {|text|
      bbox = text.bbox
      text.y = text.y + ofst_y - bbox[1]
      ofst_y = ofst_y + bbox[3] - bbox[1] + margin
    }
  end

  def center_del
    Ngraph::Text.del("CENTER")
  end

  def list_add_sub(str, ofst_x, size, dot_char, raw = false, font = 'Sans-serif', color = 0)
    margin = size / 10

    ofst = @ofst_y + @title_h
    list = Ngraph::Text["LIST"][-1]
    if (list)
      ofst = list.bbox[3] + size * (@line_height - 100.0) / 100
      margin = list.pt / 10
    end
    text = Ngraph::Text.new
    text.name = "LIST"
    line_space = (raw)? "" : LINE_SPACE
    text.text = if (dot_char)
                  "#{line_space}%C{#{ENUM_COLOR}}#{dot_char}%C{0} #{str}"
                else
                  (str == "　") ? " " : str
                end
    text.x = ofst_x
    text.pt = size
    text.raw = raw
    text.font = font
    top = text.bbox[1]
    text.r = color >> 16
    text.g = (color >> 8) & 0xff
    text.b = color & 0xff
    text.y = text.y - top + ofst + margin
    text
  end

  def center_list_add(str)
    list_add_sub(str, 0, @sub_list_text_size, nil)
    text = Ngraph::Text["LIST"][-1]
    bbox = text.bbox
    text.x = (@page_width - bbox[2] + bbox[0]) / 2
  end

  def sub_list_add(str)
    ofst_x = @ofst_x + @list_text_size / 2
    dot_char = ENUM_CHAR[1]
    list_add_sub(str, ofst_x, @sub_list_text_size, dot_char)
  end

  def list_add(str)
    ofst_x = @ofst_x
    dot_char = ENUM_CHAR[0]
    list_add_sub(str, ofst_x, @list_text_size, dot_char)
  end

  def verb_add(str)
    ofst_x = @ofst_x
    list_add_sub(str, ofst_x, @list_text_size, nil, true, "Monospace")
  end

  def text_add(str)
    ofst_x = @ofst_x
    list_add_sub(str, ofst_x, @list_text_size, nil)
  end

  def quote_add(str)
    ofst_x = @ofst_x
    list_add_sub(str, ofst_x, @list_text_size, nil, false, "Sans-serif", 0x660000)
  end

  def enum_add(str)
    ofst_x = @ofst_x
    @enum += 1
    dot_char = "#{@enum}. "
    list_add_sub(str, ofst_x, @list_text_size, dot_char)
  end

  def create_title_path
    path = Ngraph::Path.new

    path.stroke_r = TITLE_LINE.r
    path.stroke_g = TITLE_LINE.g
    path.stroke_b = TITLE_LINE.b
    path.stroke_a = TITLE_LINE.a

    path.width = 300
    path.join = Ngraph::Path::Join::ROUND
    path
  end

  def title_add(str)
    text = Ngraph::Text.new
    text.name = "TITLE"
    text.text = str
    text.x = @title_x
    text.y = @title_y
    text.pt = @title_text_size
    text.r = TITLE_FG.r
    text.g = TITLE_FG.g
    text.b = TITLE_FG.b
    text.a = TITLE_FG.a

    margin_x = 600
    margin_y = 200

    bbox = text.bbox
    w = bbox[2] - bbox[0]
    h = bbox[3] - bbox[1]

    rectangle = Ngraph::Rectangle.new
    rectangle.hidden = false

    x1 = bbox[0] - margin_x
    y1 = bbox[3] - h / 2
    x2 = bbox[0] + margin_x + h
    y2 = bbox[3] + margin_y

    path = create_title_path
    path.points = [x1, y1, x1, y2, x2, y2]

    rectangle.x1 = x1
    rectangle.y1 = y2

    x1 = bbox[2] + margin_x
    y1 = bbox[1] + h / 2
    x2 = bbox[2] - margin_x - h
    y2 = bbox[1] - margin_y

    path = create_title_path
    path.points = [x1, y1, x1, y2, x2, y2]

    rectangle.x2 = x1
    rectangle.y2 = y2
    rectangle.fill_r = TITLE_BG.r
    rectangle.fill_g = TITLE_BG.g
    rectangle.fill_b = TITLE_BG.b
    rectangle.fill_a = TITLE_BG.a
    rectangle.fill = true
    rectangle.stroke = false

    @title_h = rectangle.y1
  end

  def list_del
    text = Ngraph::Text["LIST"]
    text.del if (text)
  end

  def list_del_all
    Ngraph::Text.del("LIST")
  end

  def verb_include(file)
    include(file, true, 'Monospace')
  end

  def verb_include_n(file)
    include_with_ln(file, true, 'Monospace')
  end

  def include(file, raw, font = 'Sans-serif')
    str = IO.read(@path + "/" + file)
    list_add_sub(str, @ofst_x, @sub_list_text_size, nil, raw, font)
  end

  def include_with_ln(file, raw, font = 'Sans-serif')
    str = IO.read(@path + "/" + file)
    stra = str.split("\n")
    ln = stra.size
    w = if (ln > 99)
          3
        elsif (ln > 9)
          2
        else
          1
        end
    i = 0
    str = stra.map {|s|
      i += 1
      sprintf("%#{w}d: %s", i, s)
    }.join("\n")
    list_add_sub(str, @ofst_x, @sub_list_text_size, nil, raw, font)
  end

  def command(mode, arg, skip_pause)
    case (mode)
    when LINE_HEIGHT
      @line_height = arg.to_f
    when CENTER
      center_add(arg)
    when LOAD
      load_graph(arg)
    when LIST
      list_add(arg)
    when SUB_LIST
      sub_list_add(arg)
    when CENTER_LIST
      center_list_add(arg)
    when VERB
      verb_add(arg)
    when TEXT
      text_add(arg)
    when QUOTE
      quote_add(arg)
    when ENUM
      enum_add(arg)
    when TITLE
      title_add(arg)
    when VERB_INCLUDE
      verb_include(arg)
    when VERB_INCLUDE_N
      verb_include_n(arg)
    when INCLUDE
      include(arg, false)
    when INCLUDE_RAW
      include(arg, true)
    when VSPACE
      arg.to_i.times {
        verb_add(" ")
      }
    when COMMAND
      eval(arg)
    when SLEEP
      @gra.clear
      @gra.draw
      Ngraph.sleep(arg.to_i)
      @gra.clear
      @gra.draw
    when OPACITY
      @use_opacity = (arg == "true")
      @gra2gtk.use_opacity = @use_opacity if (! @pdf_out && ! skip_pause)
    end
  end

  def set_footer2
    text = Ngraph::Text["FOOTER2"][-1]
    if (text)
      margin = 200
      t = (Time.now.to_i - @start_time) / 60
      text.text = "#{t}/#{@total_time}"
      bbox = text.bbox
      text.x = @page_width - bbox[2] + bbox[0] - margin
    end
  end

  def add_footer
    margin = 200
    size = 1600

    text = Ngraph::Text.new
    text.name = "FOOTER1"
    text.text = "#{@page + 1}/#{@last_page}"
    text.y = @page_height - margin
    text.pt = size
    bbox = text.bbox
    text.x = (@page_width - bbox[2] + bbox[0]) / 2

    text = Ngraph::Text.new
    text.name = "FOOTER2"
    if (@total_time && ! @pdf_out)
      text.pt = size
      text.y = @page_height - margin
      set_footer2
    end
  end

  def show_menu
    unless (@pdf_out)
      gra = Ngraph::Gra.new
      gra.name = "viewer"
      gra.copy(@gra)
      gra.device = nil

      Ngraph::Menu.new {|menu|
        menu.menu
      }

      Ngraph::Gra.del("viewer")
    end
  end

  def show_page(page, skip_pause)
    mode = nil
    response = nil

    Ngraph::Merge.new.file = @background if (@background && FileTest.readable?(@background))

    add_footer

    page_item = 0
    item_tottal = page.size
    while (page_item < item_tottal)
      str = page[page_item]

      case str
      when "@line_height"
        mode = LINE_HEIGHT
        page_item += 1
      when "@center"
        mode = CENTER
        page_item += 1
      when "@load"
        mode = LOAD
        page_item += 1
      when "@list"
        mode = LIST
        page_item += 1
      when "@verb"
        mode = VERB
        page_item += 1
      when "@text"
        mode = TEXT
        page_item += 1
      when "@quote"
        mode = QUOTE
        page_item += 1
      when "@enum"
        mode = ENUM
        page_item += 1
      when "@sub_list"
        mode = SUB_LIST
        page_item += 1
      when "@center_list"
        mode = CENTER_LIST
        page_item += 1
      when "@title"
        mode = TITLE
        page_item += 1
      when "@verb_include"
        mode = VERB_INCLUDE
        page_item += 1
      when "@verb_include_n"
        mode = VERB_INCLUDE_N
        page_item += 1
      when "@include_raw"
        mode = INCLUDE_RAW
        page_item += 1
      when "@include"
        mode = INCLUDE
        page_item += 1
      when "@vspace"
        mode = VSPACE
        page_item += 1
      when "@command"
        mode = COMMAND
        page_item += 1
      when "@sleep"
        mode = SLEEP
        page_item += 1
      when "@opacity"
        mode = OPACITY
        page_item += 1
      when "@menu"
        show_menu
        page_item += 1
      when "@pause"
        if (@pdf_out == PDF_EXPAND)
          save_image(sprintf("%s%04d_%04d.pdf", PDF_PREFIX, @page, page_item))
          page_item += 1
        elsif (@pdf_out == PDF)
          page_item += 1
        elsif (! skip_pause)
          set_footer2
          @gra.clear
          @gra.draw
          key = wait_action
          case key
          when "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
            @jump_num = @jump_num * 10 + key.to_i
          when "Page_Up", "BackSpace", "button3", "button8 |scroll_up", "q", "g", "p", "Home", "End"
            response = key
            break
          when "Escape"
            response = "Page_Down"
            break
          when "t"
            response = "Top"
            break
          when "R"
            response = "R"
            break
          when "b"
            skip_pause = true
            @jump_num = 0
          when "f"
            toggle_frame
            @jump_num = 0
          when /Alt_.+/, /Control_.+/
            @jump_num = 0
          else
            page_item += 1
            @jump_num = 0
          end
        else
          page_item += 1
        end
      else
        command(mode, str, skip_pause)
        page_item += 1
      end
    end

    if (@pdf_out)
      save_image(sprintf("%s%04d_%04d.pdf", PDF_PREFIX, @page, page_item))
    else
      @gra.clear
      @gra.draw
    end

    response
  end

  def toggle_frame
    @gra2gtk.frame = ! @gra2gtk.frame
  end

  def read_data(file)
    page = []
    File.open(file, "r") {|f|
      while (line = f.gets)
        l = line.chomp
        next if (l.size < 1)

        next if (l[0] == "#")

        case l
        when "@clearpage"
          page.push([])
        when "@background"
          @background = f.gets.chomp
        when "@total_time"
          @total_time = f.gets.to_i
        when "@line_height"
          @line_height = f.gets.to_f
        when "@title_y"
          @title_y = f.gets.to_f
        when "@page_size"
          size = f.gets.split.map {|i| i.to_i}
          @page_width = size[0]
          @page_height = size[1]
          @page_left_margin = size[2] if (size[2])
          @page_top_margin = size[3] if (size[3])
        else
          dat = page[-1]
          dat.push(l) if (dat)
        end
      end
    }

    exit if (page.size < 1)
    page
  end

  def create_gra(pdf_out)
    @gra = Ngraph::Gra.new
    @gra.name = "PRESENTATION"

    if (pdf_out)
      gra2cairofile = Ngraph::Gra2cairofile.new
      if (FileTest.exist?("/dev/null"))
        gra2cairofile.file = "/dev/null"
      else
        gra2cairofile.file = "nul"
      end
      @gra.device = gra2cairofile
    else
      @gra2gtk = Ngraph::Gra2gtk.new
      @gra2gtk.fit = true
      @gra2gtk.frame = true
      @gra2gtk.fullscreen(true)
      @gra.device = @gra2gtk
    end
    @gra.zoom = 10000
    @gra.paper_width = @page_width
    @gra.paper_height = @page_height
    @gra.top_margin = @page_top_margin
    @gra.left_margin = @page_left_margin
    @gra.draw_obj = ["merge", "axisgrid", "file", "axis", "legend", "rectangle", "arc", "path", "mark", "text"]
    @gra.open
  end

  def load(data_file)
    @pages = read_data(data_file)
    @last_page = @pages.size
    @data_file = data_file
    @page = @last_page - 1 if (@page > @last_page - 1)
    @path = File.dirname(data_file)
  end

  def reload
    load(@data_file)
  end

  def start
    create_gra(@pdf_out)

    @page = 0
    @start_time = Time.now.to_i
    dont_change = false
    @jump_num = 0

    while (true)
      response = show_page(@pages[@page], dont_change)

      if (@pdf_out)
        response = "Page_Down"
      else
        unless (response)
          response = wait_action
        end
      end

      case response
      when "Home"
        @page = 0
        dont_change = false
        @jump_num = 0
      when "End"
        @page = @last_page - 1
        dont_change = false
        @jump_num = 0
      when "Page_Up", "BackSpace", "button3", "button8", "scroll_up"
        if (@page > 0)
          @page -= 1
          dont_change = false
        else
          dont_change = true
        end
        @jump_num = 0
      when "Escape", "q"
        break
      when "f"
        toggle_frame
        dont_change = true
        @jump_num = 0
      when "0", "1", "2", "3", "4", "5", "6", "7", "8", "9"
        @jump_num = @jump_num * 10 + response.to_i
        dont_change = true
      when "n"
        @jump_num += @page
        @page = @jump_num if (@jump_num < @last_page)
        @jump_num = 0
        dont_change = false
      when "p"
        @jump_num = @page - @jump_num
        @page = @jump_num if (@jump_num >= 0)
        @jump_num = 0
        dont_change = false
      when "g"
        @page = @jump_num if (@jump_num < @last_page && @jump_num >= 0)
        @jump_num = 0
        dont_change = false
      when /Shift_.+/, /Alt_.+/, /Control_.+/, "b"
        dont_change = true
        @jump_num = 0
      when "Top", "t"
        dont_change = false
        @jump_num = 0
      when "R"
        reload
      else
        if (@page < @last_page - 1)
          @page += 1
          dont_change = false
        else
          @slide_show = false if (@slide_show)

          if (@pdf_out)
            pdf_file = if (@pdf_file)
                         @pdf_file
                       else
                         File.basename(@data_file, ".dat") + ".pdf"
                       end
            src_pdf_files = Dir.glob("#{PDF_PREFIX}*.pdf").sort
            if (FileTest.exist?("/dev/null"))
              `pdfunite #{src_pdf_files.join(" ")} #{pdf_file}`
            else
              pdf_file_tmp = "__tmp__" + pdf_file
              `pdfunite #{src_pdf_files.join(" ")} #{pdf_file_tmp}`
              `pdfopt #{pdf_file_tmp} #{pdf_file}`
              File.delete(pdf_file_tmp)
            end
            File.delete(*src_pdf_files)
            puts("save #{pdf_file}")
            break
          end
          dont_change = true
        end
        @jump_num = 0
      end
      clear
      @gra.clear
    end
  end
end

def usage
  puts("argument(s): [-pdf|-pdf_expand|-a time] datafile [output file]")
  exit
end

presentation = Presentation.new

while (ARGV[0] && ARGV[0][0] == "-")
  case (ARGV[0])
  when "-pdf"
    presentation.pdf_out = Presentation::PDF
  when "-pdf_expand"
    presentation.pdf_out = Presentation::PDF_EXPAND
  when "-a"
    presentation.slide_show = true
    ARGV.shift
    presentation.slide_show_wait = ARGV[0].to_i
  else
    usage
  end
  ARGV.shift
end

data_file = ARGV[0]
usage unless (data_file && FileTest.readable?(data_file))

presentation.pdf_filename = ARGV[1] if (ARGV[1])
presentation.load(data_file)
presentation.start

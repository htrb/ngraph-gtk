class Presentation
  PDF_PREFIX = "_npresentation_tmp_"

  MODE = [
          :CENTER,
          :LOAD,
          :LIST,
          :SUB_LIST,
          :TITLE,
          :COMMAND,
          :SLEEP,
          :OPACITY,
         ]

  PDF_MODE = [
              :PDF,
              :PDF_EXPAND
             ]

  (PDF_MODE + MODE).each_with_index {|sym, i|
    const_set(sym, i)
  }

  attr_accessor :slide_show, :slide_show_wait, :pdf_out

  def initialize
    @title_text_size = 4000
    @list_text_size = 3300
    @sub_list_text_size = 3000
    @center_text_size = 6200
    @title_x = 1000
    @title_y = 3400
    @ofst_x = 200
    @ofst_y = 4600
    @page_width = 29700
    @page_height = 21000
    @use_opacity = true

    @pdf_out = false
    @total_time = nil

    @slide_show = false
    @slide_show_wait = 10
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

  def load(file)
    Ngraph::Shell.new {|shell|
      shell::shell(file)
    }

    Ngraph::Gra.del("viewer")
    Ngraph::Fit.each {|fit|
      fit.display = false
    }
  end

  def clear
    Ngraph::Draw.derive(true).each {|obj|
      obj.del("0-!")
    }

    Ngraph::Fit.derive(true).each {|obj|
      obj.del("0-!")
    }

    Ngraph::Gra.del("viewer")
  end

  def center_add(str)
    text = Ngraph::Text.new
    text.name = "CENTER"
    text.text = str
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

  def list_add_sub(str)
    margin = @list_text_size / 2

    ofst = @ofst_y
    list = Ngraph::Text["LIST"][-1]
    ofst = list.bbox[3] if (list)

    text = Ngraph::Text.new
    text.name = "LIST"
    text.text = "%C{204A87}#{@dot_char}%C{0} #{str}"
    text.x = @ofst_x
    text.pt = @list_text_size
    text.y = ofst + margin
  end

  def sub_list_add(str)
    ofst_x = @ofst_x + @list_text_size / 2
    @dot_char = '\xb7'
    list_text_size = @sub_list_text_size
    list_add_sub(str)
  end

  def list_add(str)
    ofst_x = @ofst_x
    @dot_char = '\xbb'
    list_text_size = @list_text_size
    list_add_sub(str)
  end

  def create_title_path
    path = Ngraph::Path.new

    path.stroke_r = 78
    path.stroke_g = 154
    path.stroke_b = 6

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
    rectangle.fill_r = 255
    rectangle.fill_g = 245
    rectangle.fill_b = 167
    rectangle.fill_a = 255
    rectangle.fill = true
    rectangle.stroke = false
  end

  def list_del
    text = Ngraph::Text["LIST"]
    text.del if (text)
  end

  def list_del_all
    Ngraph::Text.del("LIST")
  end

  def command(mode, arg, skip_pause)
    case (mode)
    when CENTER
      center_add(arg)
    when LOAD
      load(arg)
    when LIST
      list_add(arg)
    when SUB_LIST
      sub_list_add(arg)
    when TITLE
      title_add(arg)
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
    text.x = margin
    text.y = @page_height - margin
    text.pt = size

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

    Ngraph::Merge.new.file = @background if (FileTest.readable?(@background))

    add_footer

    page_item = 0
    item_tottal = page.size
    while (page_item < item_tottal)
      str = page[page_item]

      case str
      when "@center"
        mode = CENTER
        page_item += 1
      when "@load"
        mode = LOAD
        page_item += 1
      when "@list"
        mode = LIST
        page_item += 1
      when "@sub_list"
        mode = SUB_LIST
        page_item += 1
      when "@title"
        mode = TITLE
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
    @gra2gtk.frame = ! @gra2gtk.frame?
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
      gra2cairofile.file = "/dev/null"
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
    @gra.draw_obj = ["merge", "axisgrid", "file", "axis", "legend", "rectangle", "arc", "path", "mark", "text"]
    @gra.open
  end

  def open(data_file)
    @pages = read_data(data_file)
    @last_page = @pages.size
    @data_file = data_file
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
      else
        if (@page < @last_page - 1)
          @page += 1
          dont_change = false
        else
          @slide_show = false if (@slide_show)

          if (@pdf_out)
            pdf_file = File.basename(@data_file, ".dat") + ".pdf"
            system("pdfjoin -q -o #{pdf_file} #{PDF_PREFIX}*.pdf")
            File.delete(*Dir.glob("#{PDF_PREFIX}*.pdf"))
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

presentation = Presentation.new

if (ARGV[0] == "-pdf")
  presentation.pdf_out = Presentation::PDF
  ARGV.shift
elsif (ARGV[0] == "-pdf_expand")
  presentation.pdf_out = Presentation::PDF_EXPAND
  ARGV.shift
end

if (ARGV[0] == "-a")
  presentation.slide_show = true
  ARGV.shift
  presentation.slide_show_wait = ARGV[0].to_i
  ARGV.shift
end

data_file = ARGV[0]
unless (data_file && FileTest.readable?(data_file))
  puts("argument(s): [-pdf|-pdf_expand|-a time] datafile")
  exit
end

presentation.open(data_file)
presentation.start

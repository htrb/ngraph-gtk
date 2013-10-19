# text-in.nsc written by S. ISHIZAKA. 1997/12
#
# This script inserts a column as legend-text.
#
# Description: _Text in...,inserts a column as legend-text,

SHIFTX = 0         # ammount of the shift from plotted data-points.
SHIFTY = 0         # ammount of the shift from plotted data-points.
PT = 2000          # Size
FONT = "Sans-serif"  # Font
DIR = 0            # Direction

if (Ngraph::File.size == 0)
  Ngraph::err_puts("No data file.")
  exit
end

dialog = Ngraph::Dialog.new

files = Ngraph::File.map {|file|
  "#{file.id} #{file.file}"
}

file_obj = if (files.size == 1)
             files[0]
           else
             dialog.title = "select data"
             dialog.caption = "select data object"
             file_obj = dialog.combo(files)
             unless (file_obj)
               dialog.del
               exit
             end
             file_obj
           end
id = file_obj.split[0].to_i

dialog.title = file_obj
dialog.caption = 'Input legend-text column'
column = dialog.integer_entry(0, 1000, 1, 1)
unless (column)
  dialog.del
  exit
end

dialog.caption = 'select horizontal position of the text'
align = dialog.radio("_Left", "_Center", "_Right")
unless (align)
  dialog.del
  exit
end

dialog.title = file_obj
dialog.caption = 'Input vertical offset (mm)'
offset = dialog.integer_entry(-50, 50, 1, 0)
unless (offset)
  dialog.del
  exit
end

dialog.del

i = 0
file = Ngraph::File[id]
file.opendatac
while true
  file.getdata
  break if (file.rcode != 0)
  i += 1
  text_name = "in_#{id}_#{i}"
  text = Ngraph::Text[text_name][-1]

  unless (text)
    text = Ngraph::Text.new
    text.name = text_name
    text.pt = PT
    text.r = file.r
    text.g = file.g
    text.b = file.b
    text.font = FONT
    text.direction = DIR
  end
  text.x = 0
  text.y = 0
  text.text = file.column(file.line, column)
  bbox = text.bbox
  text.x = file.coord_x + SHIFTX - (bbox[2] - bbox[0]).abs / 2 * (2 - align)
  text.y = file.coord_y + SHIFTY + (bbox[3] - bbox[1]).abs / 2 - bbox[3] + offset * 100
end
file.closedata

menu = Ngraph::Menu[0]
menu.modified = true
menu.draw

# Description: _Calendar,show calendar in the information window,

require 'ngraph'

lang = ENV["LANG"]
if (lang && lang =~ /UTF-?8/i)
    locale = lang
else
    locale = "C"
end

date = Time.now
m = date.mon
y = date.year

Ngraph::Dialog.new {|dialog|
  dialog.title = "Calendar"

  dialog.caption = "select Year"
  y = dialog.integer_entry(0, 9999, 1, y)
  exit unless (y)

  dialog.caption = "select Month"
  m = dialog.integer_entry(1, 12, 1, m)
  exit unless (m)
}

menu = Ngraph::Menu[0]
if (menu)
  menu.show_window(5)
  menu.echo
  menu.echo(`LANG="#{locale}" /usr/bin/cal -3 -h #{m} #{y}`)
  menu.echo
else
  puts(`LANG="#{locale}" /usr/bin/cal -3 -h #{m} #{y}`)
end

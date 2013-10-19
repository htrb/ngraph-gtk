# Description: _Calendar,show calendar in the information window,

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
  dialog.caption = "select MONTH"
  m = dialog.integer_entry(1, 12, 1, m)
}

exit unless (m)

menu = Ngraph::Menu[0]
menu.show_window(5)
menu.echo
menu.echo(`LANG="#{locale}" /usr/bin/cal -3 -h #{m} #{y}`)
menu.echo

# Description: _Timer,Countdown timer,

BAR_X = 1000
BAR_Y = 19000
BAR_W = 19000
BAR_H = 3000

def create_rectangle
  rectangle = Ngraph::Rectangle.new
  rectangle.x1 = BAR_X
  rectangle.y1 = BAR_Y
  rectangle.x2 = BAR_X + BAR_W
  rectangle.y2 = BAR_Y + BAR_H
  rectangle.fill_g = 255
  rectangle
end

system = Ngraph::System[0]
menu = Ngraph::Menu[0]
gra = Ngraph::Gra.current

t = nil
Ngraph::Dialog.new {|dialog|
  dialog.title = "Countdown timer"
  dialog.caption = "Countdown (minutes):"
  t = dialog.integer_entry(1, 120, 1, 30) * 60
}
exit unless (t)

Ngraph::Text.del("timer")

objs = Ngraph::Draw.derive(true).map {|obj|
  obj::NAME
}

system.hide_instance(objs)

text = Ngraph::Text.new
text.name = "timer"
text.hidden = false
text.text = system.time(0)
text.x = 600
text.y = 13000
text.pt = 14200
text.font = 'Sans-serif'

bar_rect = create_rectangle
bar_rect.fill = true
bar_rect.stroke = false

frame_rect = create_rectangle
frame_rect.fill = false
frame_rect.stroke = true
frame_rect.width = 120

t.downto(0) {|i|
  m = (i / 60).to_i
  s = sprintf("%02d", i % 60)
  text.text = "#{system.time(0)}\\&\\n#{m}:#{s}\\&"

  bar_rect.x2 = BAR_W * i / t + bar_rect.x1
  bar_rect.fill_g = 255.0 * i / t
  bar_rect.fill_r = 255.0 * (1 - i / t.to_f)

  gra.clear
  gra.draw
  Ngraph.sleep(1)
}

text.del
bar_rect.del
frame_rect.del

system.recover_instance(objs)

gra.clear
gra.draw

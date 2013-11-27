# -*- coding: utf-8 -*-
require 'ngraph'

def show_result(a, b, c)
  if (b == c)
    puts("OK. (#{a})")
  else
    puts("error. (#{a})  '#{b}'  '#{c}'")
  end
end

Ngraph::String.new {|string|

  string.value = " a b  b   b   c d  e f ＢＣＤ"

  show_result("strip",    string.strip,             "a b  b   b   c d  e f ＢＣＤ")
  show_result("upcase",   string.upcase,            " A B  B   B   C D  E F ＢＣＤ")
  show_result("downcase", string.downcase,          " a b  b   b   c d  e f ｂｃｄ")
  show_result("reverse",  string.reverse,           "ＤＣＢ f e  d c   b   b  b a ")
  show_result("slice",    string.slice(6, 9),       "b   b   c")
  show_result("slice",    string.slice(21, 4),      "f ＢＣ")
  show_result("index",    string.index("Ｄ", 25),   25)
  show_result("index",    string.index("a", 1),     1)
  show_result("rindex",   string.rindex("a", 1),    1)
  show_result("rindex",   string.rindex("b", 20),   10)
  show_result("match",    string.match?("f.Ｂ"),    true)
  show_result("match",    string.match?("a[^b]+c"), false)
}

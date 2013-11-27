require 'ngraph'

def show_result(a, b, c)
  if (b == c)
    puts("OK. (#{a})")
  else
    puts("error. (#{a})  #{b}  #{c}")
  end
end

Ngraph::Sarray.new {|sarray|
  sarray.delimiter = ",+"
  sarray.split("1,,2,,3,,4,,5,,6,,7,,8,,9,,0,,9,,8,,7,,6,,5,,4,,3,,2,,1")

  s = sarray.join
  show_result("join", "#{s}", "1,2,3,4,5,6,7,8,9,0,9,8,7,6,5,4,3,2,1")

  sarray.rsort
  s = sarray.join
  show_result("rsort", "#{s}", "9,9,8,8,7,7,6,6,5,5,4,4,3,3,2,2,1,1,0")

  sarray.sort
  s = sarray.join
  show_result("sort", "#{s}", "0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9")

  sarray.uniq
  s = sarray.join
  show_result("uniq", "#{s}", "0,1,2,3,4,5,6,7,8,9")

  s = sarray.seq
  show_result("seq", "#{s}", "0 1 2 3 4 5 6 7 8 9")

  s = sarray.rseq
  show_result("rseq", "#{s}", "9 8 7 6 5 4 3 2 1 0")

  s = sarray.pop
  show_result("pop", "#{s}", "9")

  s = sarray.join
  show_result("pop", "#{s}", "0,1,2,3,4,5,6,7,8")

  sarray.push("9")
  s = sarray.join
  show_result("push", "#{s}", "0,1,2,3,4,5,6,7,8,9")

  s = sarray.shift
  show_result("shift", "#{s}", "0")

  s = sarray.join
  show_result("shift", "#{s}", "1,2,3,4,5,6,7,8,9")

  sarray.unshift("0")
  s = sarray.join
  show_result("unshift", "#{s}", "0,1,2,3,4,5,6,7,8,9")

  s = sarray.get(-2)
  show_result("get", "#{s}", "8")

  sarray.reverse
  s = sarray.join
  show_result("reverse", "#{s}", "9,8,7,6,5,4,3,2,1,0")

  sarray.value = "1 2 3 4 5 6 7 8 9 0 9 8 7 6 5 4 3 2 1".split

  sarray.slice(1, 3)
  s = sarray.join
  show_result("slice", "#{s}", "2,3,4")

  sarray.value = "1 2 3 4 5 6 7 8 9 0 9 8 7 6 5 4 3 2 1".split
  sarray.slice(-4, 3)
  sarray.reverse
  s = sarray.join
  show_result("slice", "#{s}", "2,3,4")

  sarray.value = "0 9 8 7 6 5 4 3 2 1".split
  sarray.put(-4, "-4")
  s = sarray.join
  show_result("put", "#{s}", "0,9,8,7,6,5,-4,3,2,1")

  sarray.value = "0 9 8 7 6 5 4 3 2 1".split
  sarray.put(4, "-4")
  s = sarray.join
  show_result("put", "#{s}", "0,9,8,7,-4,5,4,3,2,1")

  sarray.value = "0 9 8 7 6 5 4 3 2 1".split
  sarray.ins(-4, "100")
  s = sarray.join
  show_result("ins", "#{s}", "0,9,8,7,6,5,100,4,3,2,1")

  sarray.value = "0 9 8 7 6 5 4 3 2 1".split
  sarray.ins(4, "100")
  s = sarray.join
  show_result("ins", "#{s}", "0,9,8,7,100,6,5,4,3,2,1")
}

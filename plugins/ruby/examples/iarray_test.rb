require 'ngraph'

def show_result(a, b, c)
  if (b.to_f ==  c.to_f)
    puts("OK. (#{a})")
  else
    puts("error. (#{a})  #{b}  #{c}")
  end
end

def show_result_val(a, b, c)
  if (sprintf("%.14f", b.to_f) == sprintf("%.14f", c.to_f))
    puts("OK. (#{a})")
  else
    puts("error. (#{a})  #{b}  #{c}")
  end
end

Ngraph::Iarray.new {|iarray|
  iarray.value = [1 ,2 ,3 ,4 ,5 ,6 ,7 ,8 ,9 ,0 ,9 ,8 ,7 ,6 ,5 ,4 ,3 ,2 ,1]

  s = iarray.join
  show_result("join", "#{s}", "1,2,3,4,5,6,7,8,9,0,9,8,7,6,5,4,3,2,1")

  iarray.rsort
  s = iarray.join
  show_result("rsort", "#{s}", "9,9,8,8,7,7,6,6,5,5,4,4,3,3,2,2,1,1,0")

  iarray.sort
  s = iarray.join
  show_result("sort", "#{s}", "0,1,1,2,2,3,3,4,4,5,5,6,6,7,7,8,8,9,9")

  iarray.uniq
  s = iarray.join
  show_result("uniq", "#{s}", "0,1,2,3,4,5,6,7,8,9")

  s = iarray.seq
  show_result("seq", "#{s}", "0 1 2 3 4 5 6 7 8 9")

  s = iarray.rseq
  show_result("rseq", "#{s}", "9 8 7 6 5 4 3 2 1 0")

  s = iarray.pop
  show_result("pop", "#{s}", "9")

  s = iarray.join
  show_result("pop", "#{s}", "0,1,2,3,4,5,6,7,8")

  iarray.push(9)
  s = iarray.join
  show_result("push", "#{s}", "0,1,2,3,4,5,6,7,8,9")

  s = iarray.shift
  show_result("shift", "#{s}", "0")

  s = iarray.join
  show_result("shift", "#{s}", "1,2,3,4,5,6,7,8,9")

  iarray.unshift(0)
  s = iarray.join
  show_result("unshift", "#{s}", "0,1,2,3,4,5,6,7,8,9")

  s = iarray.get(-2)
  show_result("get", "#{s}", "8")

  iarray.reverse
  s = iarray.join
  show_result("reverse", "#{s}", "9,8,7,6,5,4,3,2,1,0")



  iarray.value = [1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1]

  temp_file= Ngraph::System[0].temp_file
  File.open(temp_file, "w") {|f|
    f.puts(iarray.join('\n'))
  }
  avg = sig = min = max = rms = 0
  Ngraph::File.new {|file|
    file.file = temp_file
    file.y = 1
    avg = file.davy
    sig = file.dsigy
    min = file.dminy
    max = file.dmaxy
    rms = Math.sqrt(sig.to_f ** 2 + avg.to_f ** 2)
  }

  s = iarray.sum
  show_result("sum", "#{s}", "90")

  s = iarray.average
  show_result_val("average", "#{s}", "#{avg}")

  s = iarray.rms
  show_result_val("RMS", "#{s}", "#{rms}")

  s = iarray.sdev
  show_result_val("sdev", "#{s}", "#{sig}")

  s = iarray.min
  show_result_val("min", "#{s}", "#{min}")

  s = iarray.max
  show_result_val("max", "#{s}", "#{max}")

  iarray.slice(1, 3)
  s = iarray.join
  show_result("slice", "#{s}", "2,3,4")

  iarray.value = [1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1]
  iarray.slice(-4, 3)
  iarray.reverse
  s = iarray.join
  show_result("slice", "#{s}", "2,3,4")

  iarray.value = [1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1]
  iarray.map("x ^ 2")
  s = iarray.join
  show_result("map", "#{s}", "1,4,9,16,25,36,49,64,81,0,81,64,49,36,25,16,9,4,1")

  iarray.map("i")
  s = iarray.join
  show_result("map", "#{s}", "0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18")
}


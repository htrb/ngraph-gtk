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

Ngraph::Darray.new {|darray|
  darray.value = [1, 2, 3, 4, 3, 2, 1]

  s = darray.join
  show_result("join", "#{s}", "1.000000000000000e+00,2.000000000000000e+00,3.000000000000000e+00,4.000000000000000e+00,3.000000000000000e+00,2.000000000000000e+00,1.000000000000000e+00")

  darray.rsort
  s = darray.join
  show_result("rsort", "#{s}", "4.000000000000000e+00,3.000000000000000e+00,3.000000000000000e+00,2.000000000000000e+00,2.000000000000000e+00,1.000000000000000e+00,1.000000000000000e+00")

  darray.sort
  s = darray.join
  show_result("sort", "#{s}", "1.000000000000000e+00,1.000000000000000e+00,2.000000000000000e+00,2.000000000000000e+00,3.000000000000000e+00,3.000000000000000e+00,4.000000000000000e+00")

  darray.uniq
  s = darray.join
  show_result("uniq", "#{s}", "1.000000000000000e+00,2.000000000000000e+00,3.000000000000000e+00,4.000000000000000e+00")

  s = darray.seq
  show_result("seq", "#{s}", "0 1 2 3")

  s = darray.rseq
  show_result("rseq", "#{s}", "3 2 1 0")

  s = darray.pop
  show_result("pop", "#{s}", "4.000000000000000e+00")

  s = darray.join
  show_result("pop", "#{s}", "1.000000000000000e+00,2.000000000000000e+00,3.000000000000000e+00")

  darray.push(4)
  s = darray.join
  show_result("push", "#{s}", "1.000000000000000e+00,2.000000000000000e+00,3.000000000000000e+00,4.000000000000000e+00")

  s = darray.shift
  show_result("shift", "#{s}", "1.000000000000000e+00")

  s = darray.join
  show_result("shift", "#{s}", "2.000000000000000e+00,3.000000000000000e+00,4.000000000000000e+00")

  darray.unshift(0)
  s = darray.join
  show_result("unshift", "#{s}", "0.000000000000000e+00,2.000000000000000e+00,3.000000000000000e+00,4.000000000000000e+00")

  s = darray.get(-2)
  show_result("get", "#{s}", "3.000000000000000e+00")

  darray.reverse
  s = darray.join
  show_result("reverse", "#{s}", "4.000000000000000e+00,3.000000000000000e+00,2.000000000000000e+00,0.000000000000000e+00")

  darray.pop
  darray.reverse
  s = darray.join
  show_result("reverse", "#{s}", "2.000000000000000e+00,3.000000000000000e+00,4.000000000000000e+00")


  darray.value = [1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1]

  temp_file = Ngraph::System[0].temp_file
  File.open(temp_file, "w") {|f|
    f.puts(darray.join('\n'))
  }
  avg = sig = min = max = rms = 0
  Ngraph::File.new {|file|
    file.file = temp_file
    file.y = 1
    avg = file.davy
    sig = file.dsigy
    min = file.dminy
    max = file.dmaxy
    rms = Math::sqrt(sig.to_f ** 2 + avg.to_f ** 2)
  }

  s = darray.sum
  show_result("sum", "#{s}", "9.000000000000000e+01")

  s = darray.average
  show_result_val("average", "#{s}", "#{avg}")

  s = darray.rms
  show_result_val("RMS", "#{s}", "#{rms}")

  s = darray.sdev
  show_result_val("sdev", "#{s}", "#{sig}")

  s = darray.min
  show_result("min", "#{s}", "#{min}")

  s = darray.max
  show_result("max", "#{s}", "#{max}")

  darray.slice(1, 3)
  s = darray.join
  show_result("slice", "#{s}", "2.000000000000000e+00,3.000000000000000e+00,4.000000000000000e+00")

  darray.value = [1, 2, 3, 4, 5, 6, 7, 8, 9, 0, 9, 8, 7, 6, 5, 4, 3, 2, 1]
  darray.slice(-4, 3)
  darray.reverse
  s = darray.join
  show_result("slice", "#{s}", "2.000000000000000e+00,3.000000000000000e+00,4.000000000000000e+00")

  darray.value = [1, 2, 3]
  darray.map("x ^ 2")
  s = darray.join
  show_result("map", "#{s}", "1.000000000000000e+00,4.000000000000000e+00,9.000000000000000e+00")

  darray.map("i")
  s = darray.join
  show_result("map", "#{s}", "0.000000000000000e+00,1.000000000000000e+00,2.000000000000000e+00")
}

require 'ngraph'

def show_result(a, b, c)
  if (b == c)
    puts("OK. (#{a})")
  else
    puts("error. (#{a})  #{b}  #{c}")
  end
end

def check_match(regexp)
  n = regexp.num
  m = Ngraph::Sarray.size
  show_result("num", n, m)

  regexp.num.times {|i|
    sarray = Ngraph::Sarray[i]
    sarray.num.times {|j|
      s1 = regexp.get(i, j)
      s2 = sarray.get(j)
      show_result("get", s1, s2)
    }
  }
end

regexp = Ngraph::Regexp.new
sarray = []
sarray[0] = Ngraph::Sarray.new
sarray[1] = Ngraph::Sarray.new

regexp.value = "(foo)(bar)(BAZ)?"
regexp.match 'foobarbaz foobarBAZ'
sarray[0].value = "foobar foo bar".split
sarray[1].value = "foobarBAZ foo bar BAZ".split

check_match(regexp)


regexp.value = "(?i)(foo)(bar)(BAZ)?"
regexp.match 'FOObarbaz foobarBAZ'
sarray[0].value = "FOObarbaz FOO bar baz".split
sarray[1].value = "foobarBAZ foo bar BAZ".split

check_match(regexp)


regexp.value = "(foo)(bar)(BAZ)?"
show_result("replace", regexp.replace('foobarbaz'), "baz")
show_result("replace", regexp.replace('foobarbaz', '\\2\\1'), "barfoobaz")

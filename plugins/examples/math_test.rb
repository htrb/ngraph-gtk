require 'ngraph'

def check_val(res, state)
  case (state)
  when Ngraph::Math::Status::NAN
    "nan"
  when Ngraph::Math::Status::UNDEF
    "undefined"
  else
    sprintf("%.13E", res)
  end
end

math = Ngraph::Math.new
ARGV.each {|file|
  IO.foreach(file) {|l|
    d = l.chomp.split("#")

    eqn = d[0]
    if (d[1] =~ /^-?[0-9].*/)
      val = check_val(d[1].to_f, Ngraph::Math::Status::NOERR)
    else
      val = d[1]
    end

    math.formula = eqn
    res = math.calc
    result = check_val(res, math.status)

    if (result != val)
      puts()
      puts("error #{eqn}")
      puts("    expected: #{val}")
      puts("      result: #{result}")
      puts("  difference: #{result.to_f - val.to_f}")
    else
      puts("OK. (#{eqn})")
    end
  }
}

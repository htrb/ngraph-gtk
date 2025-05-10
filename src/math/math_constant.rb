#! /usr/bin/ruby
# frozen_string_literal: true

const_str = []
File.foreach(ARGV[0]) do |l|
  l.chomp!
  next if l.empty?

  const_str.push(l.split)
end

const_str = const_str.uniq.sort { |a, b| b[0].length - a[0].length }

File.open("#{ARGV[1]}.h", 'w') do |f|
  f.puts <<~HEADER
    #ifndef MATH_SCANNER_CONST_HEADER
    #define MATH_SCANNER_CONST_HEADER

    #include "math_expression.h"
    #include "math_equation.h"

    enum MATH_SCANNER_VAL_TYPE {
      MATH_SCANNER_VAL_TYPE_NORMAL,
  HEADER
  const_str.each do |s|
    f.puts("  MATH_SCANNER_VAL_TYPE_#{s[0]},") if s.length == 1
  end
  f.puts("  MATH_SCANNER_VAL_TYPE_UNKNOWN\n};\n\n")

  f.puts <<~FOOTER
    struct math_const_parameter {
      char *str;
      enum MATH_SCANNER_VAL_TYPE type;
      MathValue val;
    };

    int math_scanner_is_const(int chr);
    enum MATH_SCANNER_VAL_TYPE math_scanner_check_math_const_parameter(char *str, MathValue *val);
    int math_add_basic_constant(MathEquation *eq);

    #endif
  FOOTER
end

File.open("#{ARGV[1]}.c", 'w') do |f|
  f.puts <<~HEADER
    #include "config.h"

    #include <string.h>

    #include "math_expression.h"
    #include "math_equation.h"
    #include "math_constant.h"

    static struct math_const_parameter MathConstParameter[] = {
  HEADER
  const_str.each do |s|
    f.puts("  {\"#{s[0]}\", MATH_SCANNER_VAL_TYPE_NORMAL, {#{s[1]}, #{s[2]}}},") if s.length == 3
  end
  f.puts("};\n\n")
  f.puts <<~FOOTER
    int
    math_add_basic_constant(MathEquation *eq)
    {
      unsigned int i;

        for (i = 0; i < sizeof(MathConstParameter) / sizeof(*MathConstParameter); i++) {
          if (math_equation_add_const(eq, MathConstParameter[i].str, &MathConstParameter[i].val) < 0) {
            return 1;
        }
      }
      return 0;
    }

    enum MATH_SCANNER_VAL_TYPE
    math_scanner_check_math_const_parameter(char *str, MathValue *val)
    {
      unsigned int i;

      for (i = 0; i < sizeof(MathConstParameter) / sizeof(*MathConstParameter); i++) {
        if (strcmp(str, MathConstParameter[i].str) == 0) {
          *val = MathConstParameter[i].val;
          return MathConstParameter[i].type;
        }
      }

      return MATH_SCANNER_VAL_TYPE_UNKNOWN;
    }
  FOOTER
end

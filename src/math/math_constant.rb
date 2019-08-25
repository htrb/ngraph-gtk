#! /usr/bin/ruby

const_str = [];
IO.foreach(ARGV[0]) { |l|
  l.chomp!
  next if (l.length < 1)
  const_str.push(l.split)
}

const_str = const_str.uniq.sort {|a, b| b[0].length - a[0].length}

File.open("#{ARGV[1]}.h", "w") { |f|
  f.puts <<EOF
#ifndef MATH_SCANNER_CONST_HEADER
#define MATH_SCANNER_CONST_HEADER

#include "math_expression.h"
#include "math_equation.h"

enum MATH_SCANNER_VAL_TYPE {
  MATH_SCANNER_VAL_TYPE_NORMAL,
EOF
  const_str.each {|s|
    f.puts("  MATH_SCANNER_VAL_TYPE_#{s[0]},") if (s.length == 1)
  }
  f.puts("  MATH_SCANNER_VAL_TYPE_UNKNOWN\n};\n\n")

  f.puts <<EOF
struct math_const_parameter {
  char *str;
  enum MATH_SCANNER_VAL_TYPE type;
  MathValue val;
};

int math_scanner_is_const(int chr);
enum MATH_SCANNER_VAL_TYPE math_scanner_check_math_const_parameter(char *str, MathValue *val);
int math_add_basic_constant(MathEquation *eq);

#endif
EOF
}

File.open("#{ARGV[1]}.c", "w") { |f|
  f.puts <<EOF
#include "config.h"

#include <string.h>

#include "math_expression.h"
#include "math_equation.h"
#include "math_constant.h"

static struct math_const_parameter MathConstParameter[] = {
EOF
  const_str.each {|s|
    if (s.length == 3)
      f.puts("  {\"#{s[0]}\", MATH_SCANNER_VAL_TYPE_NORMAL, {#{s[1]}, #{s[2]}}},")
    end
  }
  f.puts("};\n\n")


  f.puts <<EOF

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
EOF
}

#! /usr/bin/ruby

func_str = [];
IO.foreach(ARGV[0]) { |l|
  l.chomp!
  next if (l.length < 1)
  func_str.push(l.split) 
}

func_str = func_str.uniq.sort {|a, b| b[0].length - a[0].length}

File.open("#{ARGV[1]}.h", "w") { |f|
  f.puts <<EOF
#ifndef MATH_SCANNER_FUNC_HEADER
#define MATH_SCANNER_FUNC_HEADER

enum MATH_FUNCTION_ARG_TYPE {
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_ARRAY,
  MATH_FUNCTION_ARG_TYPE_PROC,
};

typedef int (* math_function) (MathFunctionCallExpression *exp, MathEquation *eq, MathValue *r);

struct math_function_parameter {
  int argc;
  int side_effect, positional;
  math_function func;
  enum MATH_FUNCTION_ARG_TYPE *arg_type;
  MathExpression *opt_usr, *base_usr;
  char *name;
};

int math_scanner_is_func(int chr);
int math_add_basic_function(MathEquation *eq);

EOF
  func_str.each {|s|
    if (s.length == 5)
      f.puts("int math_func_#{s[0]}(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);")
    end
  }
  f.puts("#endif")
}

File.open("#{ARGV[1]}.c", "w") { |f|
  f.puts <<EOF
#include <stdlib.h>
#include <string.h>
#include "math_expression.h"
#include "math_equation.h"
#include "math_function.h"

struct funcs {
  char *name;
  struct math_function_parameter prm;
};
static struct funcs func_ary[] = {
EOF
  func = []
  i = 0;
  func_str.each {|s|
    if (s.length == 5)
      f.puts("  {\"#{s[0].upcase}\", {#{s[1]}, #{s[2]}, #{s[3]}, math_func_#{s[0]}, NULL, NULL, NULL, NULL}},")
      if (s[4] != "NULL")
        func.push([i, s[4].split(",")])
      end
      i += 1
    end
  }
  f.puts("};\n\n")

  f.puts <<EOF

int
math_add_basic_function(MathEquation *eq) {
  unsigned int i;
  int r;
  enum MATH_FUNCTION_ARG_TYPE *ptr;

  for (i = 0; i < sizeof(func_ary) / sizeof(*func_ary); i++) {
    switch (i) {
EOF
  func.each {|arg|
  f.puts <<EOF
    case #{arg[0]}:
      if (func_ary[i].prm.arg_type)
        break;
      ptr = g_malloc(sizeof(enum MATH_FUNCTION_ARG_TYPE) * #{arg[1].length});
      if (ptr == NULL) {
        return 1;
      }
EOF
    arg[1].each_with_index {|v, i|
      f.print("      ptr[#{i}] = ")
      f.puts(
             case (v)
             when "0":
                 "MATH_FUNCTION_ARG_TYPE_DOUBLE;"
             when "1":
                 "MATH_FUNCTION_ARG_TYPE_ARRAY;"
             when "2":
                 "MATH_FUNCTION_ARG_TYPE_PROC;"
             end
             )

    }
    f.puts("      func_ary[i].prm.arg_type = ptr;")
    f.puts("      break;")
  }
  f.puts("    }")
  f.puts <<EOF
    if (math_equation_add_func(eq, func_ary[i].name, &func_ary[i].prm) == NULL)
      return r;
  }
  return 0;
}
EOF
}

#! /usr/bin/ruby

func_str = [];
IO.foreach(ARGV[0]) { |l|
  next if (l[0] == ?#)
  l.chomp!
  next if (l.length < 1)
  func_str.push(l.split)
}

func_str = func_str.uniq.sort {|a, b| b[0].length - a[0].length}
MathFunctionType = [
  "MATH_FUNCTION_TYPE_NORMAL",
  "MATH_FUNCTION_TYPE_POSITIONAL",
  "MATH_FUNCTION_TYPE_CALLBACK",
]

File.open("#{ARGV[1]}.h", "w") { |f|
  f.puts <<EOF
#ifndef MATH_SCANNER_FUNC_HEADER
#define MATH_SCANNER_FUNC_HEADER

enum MATH_FUNCTION_ARG_TYPE {
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_STRING,
  MATH_FUNCTION_ARG_TYPE_ARRAY,
  MATH_FUNCTION_ARG_TYPE_STRING_ARRAY,
  MATH_FUNCTION_ARG_TYPE_ARRAY_COMMON,
  MATH_FUNCTION_ARG_TYPE_VARIABLE,
  MATH_FUNCTION_ARG_TYPE_STRING_VARIABLE,
  MATH_FUNCTION_ARG_TYPE_VARIABLE_COMMON,
  MATH_FUNCTION_ARG_TYPE_PROC,
};

enum MATH_FUNCTION_TYPE {
  #{MathFunctionType.join(', ')}
};

typedef int (* math_function) (MathFunctionCallExpression *exp, MathEquation *eq, MathValue *r);

struct math_function_parameter {
  int argc;
  int side_effect;
  enum MATH_FUNCTION_TYPE type;
  math_function func;
  enum MATH_FUNCTION_ARG_TYPE *arg_type;
  MathExpression *opt_usr, *base_usr;
  char *name;
};

int math_scanner_is_func(int chr);
int math_add_basic_function(MathEquation *eq);

EOF
  func_str.each {|s|
    case (s.length)
    when 5
        f.puts("int math_func_#{s[0]}(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);")
    when 6
        f.puts("#ifdef HAVE_LIBGSL")
        f.puts("int math_func_#{s[0]}(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);")
        f.puts("#endif")
    end
  }
  f.puts("#endif")
}

File.open("#{ARGV[1]}.c", "w") { |f|
  f.puts <<EOF
#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "math_expression.h"
#include "math_equation.h"
#include "math_function.h"

struct funcs {
  char *name;
  struct math_function_parameter prm;
};

static struct funcs FuncAry[] = {
EOF
  func = []
  i = 0;
  func_str.each {|s|
    type = MathFunctionType[s[3].to_i]
    if (s.length == 5)
      f.puts("  {\"#{s[0].upcase}\", {#{s[1]}, #{s[2]}, #{type}, math_func_#{s[0]}, NULL, NULL, NULL, NULL}},")
      if (s[4] != "NULL")
        func.push([i, s[4].split(","), s[0].upcase])
      end
      i += 1
    elsif (s.length == 6)
      f.puts("#ifdef HAVE_LIBGSL")
      f.puts("  {\"#{s[0].upcase}\", {#{s[1]}, #{s[2]}, #{type}, math_func_#{s[0]}, NULL, NULL, NULL, NULL}},")
      if (s[4] != "NULL")
        func.push([i, s[4].split(","), s[0].upcase])
      end
      f.puts("#else")
      f.puts("  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},")
      f.puts("#endif")
      i += 1
    end
  }
  f.puts("};\n\n")

  f.puts <<EOF

int
math_add_basic_function(MathEquation *eq) {
  unsigned int i;
  enum MATH_FUNCTION_ARG_TYPE *ptr;

  for (i = 0; i < sizeof(FuncAry) / sizeof(*FuncAry); i++) {
    if (FuncAry[i].name == NULL) {
      continue;
    }
    switch (i) {
EOF
  func.each {|arg|
  f.puts <<EOF
    case #{arg[0]}:  /*  #{arg[2].upcase}  */
      if (FuncAry[i].prm.arg_type) {
        break;
      }
      ptr = g_malloc(sizeof(enum MATH_FUNCTION_ARG_TYPE) * #{arg[1].length});
      if (ptr == NULL) {
        return 1;
      }
EOF
    arg[1].each_with_index {|v, i|
      f.print("      ptr[#{i}] = ")
      f.puts(
             case (v)
             when "0"
                 "MATH_FUNCTION_ARG_TYPE_DOUBLE;"
             when "1"
                 "MATH_FUNCTION_ARG_TYPE_STRING;"
             when "10"
                 "MATH_FUNCTION_ARG_TYPE_ARRAY;"
             when "11"
                 "MATH_FUNCTION_ARG_TYPE_STRING_ARRAY;"
             when "12"
                 "MATH_FUNCTION_ARG_TYPE_ARRAY_COMMON;"
             when "20"
                 "MATH_FUNCTION_ARG_TYPE_VARIABLE;"
             when "21"
                 "MATH_FUNCTION_ARG_TYPE_STRING_VARIABLE;"
             when "22"
                 "MATH_FUNCTION_ARG_TYPE_VARIABLE_COMMON;"
             when "30"
                 "MATH_FUNCTION_ARG_TYPE_PROC;"
             end
             )

    }
    f.puts("      FuncAry[i].prm.arg_type = ptr;")
    f.puts("      break;")
  }
  f.puts("    }")
  f.puts <<EOF
    if (math_equation_add_func(eq, FuncAry[i].name, &FuncAry[i].prm) == NULL)
      return 1;
  }
  return 0;
}
EOF
}

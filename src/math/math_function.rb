#! /usr/bin/ruby
# frozen_string_literal: true

MATH_FUNCTION_ARG_TYPE = {
  '0' => 'MATH_FUNCTION_ARG_TYPE_DOUBLE;',
  '1' => 'MATH_FUNCTION_ARG_TYPE_STRING;',
  '10' => 'MATH_FUNCTION_ARG_TYPE_ARRAY;',
  '11' => 'MATH_FUNCTION_ARG_TYPE_STRING_ARRAY;',
  '12' => 'MATH_FUNCTION_ARG_TYPE_ARRAY_COMMON;',
  '20' => 'MATH_FUNCTION_ARG_TYPE_VARIABLE;',
  '21' => 'MATH_FUNCTION_ARG_TYPE_STRING_VARIABLE;',
  '22' => 'MATH_FUNCTION_ARG_TYPE_VARIABLE_COMMON;',
  '30' => 'MATH_FUNCTION_ARG_TYPE_PROC;'
}.freeze

func_str = []
IO.foreach(ARGV[0]) do |l|
  next if l[0] == '#'

  l.chomp!
  next if l.empty?

  func_str.push(l.split)
end

func_str = func_str.uniq.sort { |a, b| b[0].length - a[0].length }
MATH_FUNCTION_TYPE = %w[MATH_FUNCTION_TYPE_NORMAL MATH_FUNCTION_TYPE_POSITIONAL MATH_FUNCTION_TYPE_CALLBACK].freeze

File.open("#{ARGV[1]}.h", 'w') do |f|
  f.puts <<~HEADER
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
      #{MATH_FUNCTION_TYPE.join(', ')}
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

  HEADER
  func_str.each do |s|
    case s.length
    when 5
      f.puts("int math_func_#{s[0]}(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);")
    when 6
      f.puts('#ifdef HAVE_LIBGSL')
      f.puts("int math_func_#{s[0]}(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);")
      f.puts('#endif')
    end
  end
  f.puts('#endif')
end

File.open("#{ARGV[1]}.c", 'w') do |f|
  f.puts <<~HEADER
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

  HEADER
  func = []
  i = 0
  func_str.each do |s|
    type = MATH_FUNCTION_TYPE[s[3].to_i]
    case s.length
    when 5
      f.puts("  {\"#{s[0].upcase}\", {#{s[1]}, #{s[2]}, #{type}, math_func_#{s[0]}, NULL, NULL, NULL, NULL}},")
      func.push([i, s[4].split(','), s[0].upcase]) if s[4] != 'NULL'
      i += 1
    when 6
      f.puts('#ifdef HAVE_LIBGSL')
      f.puts("  {\"#{s[0].upcase}\", {#{s[1]}, #{s[2]}, #{type}, math_func_#{s[0]}, NULL, NULL, NULL, NULL}},")
      func.push([i, s[4].split(','), s[0].upcase]) if s[4] != 'NULL'
      f.puts('#else')
      f.puts('  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},')
      f.puts('#endif')
      i += 1
    end
  end
  f.puts("};\n\n")

  f.puts <<~BODY1
    int
    math_add_basic_function(MathEquation *eq) {
      unsigned int i;
      enum MATH_FUNCTION_ARG_TYPE *ptr;

      for (i = 0; i < sizeof(FuncAry) / sizeof(*FuncAry); i++) {
        if (FuncAry[i].name == NULL) {
          continue;
        }
        switch (i) {
  BODY1
  func.each do |arg|
    f.puts <<~BODY2
      /*  #{arg[2].upcase}  */
          case #{arg[0]}:
            if (FuncAry[i].prm.arg_type) {
              break;
            }
            ptr = g_malloc(sizeof(enum MATH_FUNCTION_ARG_TYPE) * #{arg[1].length});
            if (ptr == NULL) {
              return 1;
            }
    BODY2
    arg[1].each_with_index do |v, j|
      f.print("      ptr[#{j}] = ")
      f.puts(MATH_FUNCTION_ARG_TYPE[v])
    end
    f.puts('      FuncAry[i].prm.arg_type = ptr;')
    f.puts('      break;')
  end
  f.puts('    }')
  f.puts <<~FOOTER
      if (math_equation_add_func(eq, FuncAry[i].name, &FuncAry[i].prm) == NULL)
        return 1;
      }
       return 0;
    }
  FOOTER
end

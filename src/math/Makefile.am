noinst_LTLIBRARIES = libmath.la

libmath_la_SOURCES = math_function.c math_function.h math_operator.c	\
	math_operator.h math_constant.c math_constant.h			\
	math_expression.c math_parser.c math_scanner.h			\
	parse_bin_expression.h math_basic_function.c math_equation.c	\
	math_expression.h math_parser.h math_equation.h			\
	math_scanner.c math_error.h math_error.c

EXTRA_DIST = math_operator.h.in math_operator.rb math_constant.h.in	\
	math_constant.rb math_function.h.in math_function.rb

BUILT_SOURCES = math_operator.h math_operator.c math_constant.h	\
	math_constant.c math_function.h math_function.c

math_operator.c: math_operator.h

math_operator.h: math_operator.h.in math_operator.rb
	ruby math_operator.rb math_operator.h.in math_operator

math_constant.c: math_constant.h

math_constant.h: math_constant.h.in math_constant.rb
	ruby math_constant.rb math_constant.h.in math_constant

math_function.c: math_function.h math_operator.h

math_function.h: math_function.h.in math_function.rb
	ruby math_function.rb math_function.h.in math_function

libmath_la_CFLAGS = $(GTK_CFLAGS) $(GSL_CFLAGS) -I$(srcdir)/..

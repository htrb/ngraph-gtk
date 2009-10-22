#ifndef MATH_PARSER_HEADER
#define MATH_PARSER_HEADER

#include "math_equation.h"

MathExpression *math_parser_parse(const char *line, MathEquation *eq, int *err);
#endif

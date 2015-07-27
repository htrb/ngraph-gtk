/* 
 * $Id: math_parser.h,v 1.3 2009-11-10 04:12:20 hito Exp $
 * 
 */

#ifndef MATH_PARSER_HEADER
#define MATH_PARSER_HEADER

#include "math_equation.h"

MathExpression *math_parser_parse(const char *line, MathEquation *eq, int *err);
#endif

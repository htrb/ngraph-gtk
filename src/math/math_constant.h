#ifndef MATH_SCANNER_CONST_HEADER
#define MATH_SCANNER_CONST_HEADER

#include "math_expression.h"
#include "math_equation.h"

enum MATH_SCANNER_VAL_TYPE {
  MATH_SCANNER_VAL_TYPE_NORMAL,
  MATH_SCANNER_VAL_TYPE_UNKNOWN
};

struct math_const_parameter {
  char *str;
  enum MATH_SCANNER_VAL_TYPE type;
  MathValue val;
};

int math_scanner_is_const(int chr);
enum MATH_SCANNER_VAL_TYPE math_scanner_check_math_const_parameter(char *str, MathValue *val);
int math_add_basic_constant(MathEquation *eq);

#endif

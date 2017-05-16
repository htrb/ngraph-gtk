#include "config.h"

#include <string.h>

#include "math_expression.h"
#include "math_equation.h"
#include "math_constant.h"

static struct math_const_parameter MathConstParameter[] = {
  {"EULER", MATH_SCANNER_VAL_TYPE_NORMAL, {0.57721566490153286061, MATH_VALUE_NORMAL}},
  {"UNDEF", MATH_SCANNER_VAL_TYPE_NORMAL, {0, MATH_VALUE_UNDEF}},
  {"BREAK", MATH_SCANNER_VAL_TYPE_NORMAL, {0, MATH_VALUE_BREAK}},
  {"CONT", MATH_SCANNER_VAL_TYPE_NORMAL, {0, MATH_VALUE_CONT}},
  {"NAN", MATH_SCANNER_VAL_TYPE_NORMAL, {0, MATH_VALUE_NAN}},
  {"PI", MATH_SCANNER_VAL_TYPE_NORMAL, {3.14159265358979323846, MATH_VALUE_NORMAL}},
  {"E", MATH_SCANNER_VAL_TYPE_NORMAL, {2.71828182845904523536, MATH_VALUE_NORMAL}},
};


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

#ifndef MATH_SCANNER_OPE_HEADER
#define MATH_SCANNER_OPE_HEADER

enum MATH_OPERATOR_TYPE {
  MATH_OPERATOR_TYPE_POW_ASSIGN,
  MATH_OPERATOR_TYPE_MOD_ASSIGN,
  MATH_OPERATOR_TYPE_DIV_ASSIGN,
  MATH_OPERATOR_TYPE_MUL_ASSIGN,
  MATH_OPERATOR_TYPE_PLUS_ASSIGN,
  MATH_OPERATOR_TYPE_MINUS_ASSIGN,
  MATH_OPERATOR_TYPE_AND,
  MATH_OPERATOR_TYPE_OR,
  MATH_OPERATOR_TYPE_NE,
  MATH_OPERATOR_TYPE_EQ,
  MATH_OPERATOR_TYPE_GE,
  MATH_OPERATOR_TYPE_LE,
  MATH_OPERATOR_TYPE_LT,
  MATH_OPERATOR_TYPE_GT,
  MATH_OPERATOR_TYPE_ASSIGN,
  MATH_OPERATOR_TYPE_FACT,
  MATH_OPERATOR_TYPE_POW,
  MATH_OPERATOR_TYPE_MOD,
  MATH_OPERATOR_TYPE_DIV,
  MATH_OPERATOR_TYPE_MUL,
  MATH_OPERATOR_TYPE_PLUS,
  MATH_OPERATOR_TYPE_MINUS,
  MATH_OPERATOR_TYPE_UNKNOWN
};

int math_scanner_is_ope(int chr);
enum MATH_OPERATOR_TYPE math_scanner_check_ope_str(const char *str, int *len);

#endif

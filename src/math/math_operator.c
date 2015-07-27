#include "config.h"

#include <string.h>
#include "math_operator.h"

struct ope_str {
  char *ope;
  int len;
  enum MATH_OPERATOR_TYPE type;
};

static struct ope_str OpeStr[] = {
  {"!:", 2, MATH_OPERATOR_TYPE_NE},
  {"^=", 2, MATH_OPERATOR_TYPE_POW_ASSIGN},
  {"^:", 2, MATH_OPERATOR_TYPE_POW_ASSIGN},
  {"\\=", 2, MATH_OPERATOR_TYPE_MOD_ASSIGN},
  {"\\:", 2, MATH_OPERATOR_TYPE_MOD_ASSIGN},
  {"/=", 2, MATH_OPERATOR_TYPE_DIV_ASSIGN},
  {"/:", 2, MATH_OPERATOR_TYPE_DIV_ASSIGN},
  {"*=", 2, MATH_OPERATOR_TYPE_MUL_ASSIGN},
  {":=", 2, MATH_OPERATOR_TYPE_ASSIGN},
  {"*:", 2, MATH_OPERATOR_TYPE_MUL_ASSIGN},
  {"+=", 2, MATH_OPERATOR_TYPE_PLUS_ASSIGN},
  {">:", 2, MATH_OPERATOR_TYPE_GE},
  {">=", 2, MATH_OPERATOR_TYPE_GE},
  {"<:", 2, MATH_OPERATOR_TYPE_LE},
  {"<=", 2, MATH_OPERATOR_TYPE_LE},
  {"::", 2, MATH_OPERATOR_TYPE_EQ},
  {"==", 2, MATH_OPERATOR_TYPE_EQ},
  {"+:", 2, MATH_OPERATOR_TYPE_PLUS_ASSIGN},
  {"!=", 2, MATH_OPERATOR_TYPE_NE},
  {"||", 2, MATH_OPERATOR_TYPE_OR},
  {"&&", 2, MATH_OPERATOR_TYPE_AND},
  {"-:", 2, MATH_OPERATOR_TYPE_MINUS_ASSIGN},
  {"-=", 2, MATH_OPERATOR_TYPE_MINUS_ASSIGN},
  {"-", 1, MATH_OPERATOR_TYPE_MINUS},
  {"<", 1, MATH_OPERATOR_TYPE_LT},
  {">", 1, MATH_OPERATOR_TYPE_GT},
  {":", 1, MATH_OPERATOR_TYPE_ASSIGN},
  {"!", 1, MATH_OPERATOR_TYPE_FACT},
  {"^", 1, MATH_OPERATOR_TYPE_POW},
  {"\\", 1, MATH_OPERATOR_TYPE_MOD},
  {"/", 1, MATH_OPERATOR_TYPE_DIV},
  {"*", 1, MATH_OPERATOR_TYPE_MUL},
  {"+", 1, MATH_OPERATOR_TYPE_PLUS},
  {"=", 1, MATH_OPERATOR_TYPE_EOEQ},
  {";", 1, MATH_OPERATOR_TYPE_EOEQ},
};

static char OpeChar[256] = {
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  1,  /* ! */
  0,
  0,
  0,
  0,
  1,  /* & */
  0,
  0,
  0,
  1,  /* * */
  1,  /* + */
  0,
  1,  /* - */
  0,
  1,  /* / */
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  1,  /* : */
  1,  /* ; */
  1,  /* < */
  1,  /* = */
  1,  /* > */
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  1,  /* \ */
  0,
  1,  /* ^ */
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  1,  /* | */
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
  0,
};

int
math_scanner_is_ope(int chr)
{
  if (chr < 0 || chr > (int) (sizeof(OpeChar) / sizeof(*OpeChar)))
    return 0;

  return OpeChar[chr];
}

enum MATH_OPERATOR_TYPE
math_scanner_check_ope_str(const char *str, int *len) {
  unsigned int i;

  for (i = 0; i < sizeof(OpeStr) / sizeof(*OpeStr); i++) {
    if (strncmp(str, OpeStr[i].ope, OpeStr[i].len) == 0) {
      *len = OpeStr[i].len;
      return OpeStr[i].type;
    }
  }

  return MATH_OPERATOR_TYPE_UNKNOWN;
}

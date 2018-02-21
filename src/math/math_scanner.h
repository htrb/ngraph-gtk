/*
 * $Id: math_scanner.h,v 1.2 2009-11-10 04:12:20 hito Exp $
 *
 */

#ifndef MATH_SCANNER_HEADER
#define MATH_SCANNER_HEADER

#include "math_operator.h"
#include "math_constant.h"

enum MATH_TOKEN_TYPE {
  MATH_TOKEN_TYPE_NUMERIC,
  MATH_TOKEN_TYPE_SYMBOL,
  MATH_TOKEN_TYPE_OPERATOR,
  MATH_TOKEN_TYPE_LP,
  MATH_TOKEN_TYPE_RP,
  MATH_TOKEN_TYPE_LB,
  MATH_TOKEN_TYPE_RB,
  MATH_TOKEN_TYPE_LC,
  MATH_TOKEN_TYPE_RC,
  MATH_TOKEN_TYPE_DEF,
  MATH_TOKEN_TYPE_CONST,
  MATH_TOKEN_TYPE_COMMA,
  MATH_TOKEN_TYPE_ARRAY_PREFIX,
  MATH_TOKEN_TYPE_EOEQ,
  MATH_TOKEN_TYPE_UNKNOWN,
};

struct math_token {
  enum MATH_TOKEN_TYPE type;
  union {
    struct {
      double dnum;
      enum MATH_SCANNER_VAL_TYPE type;
    } val;
    enum MATH_OPERATOR_TYPE op;
    char *sym;
  } data;
  const char *ptr;
  struct math_token *next;
};

struct math_string {
  const char *top, *cur;
  int line;
};

struct math_token *math_scanner_get_token(struct math_string *rstr);
void math_scanner_free_token(struct math_token *token);
void math_scanner_init_string(struct math_string *str, const char *line);

#endif

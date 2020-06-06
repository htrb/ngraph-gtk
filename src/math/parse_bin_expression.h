/*
 * $Id: parse_bin_expression.h,v 1.3 2009-11-10 04:12:20 hito Exp $
 *
 */

#define CREATE_PARSER_FUNC(a, b, c, d) parse_ ## a ## _expression(struct math_string *str, MathEquation *eq, int *err) \
{\
  struct math_token *token;\
  MathExpression *left, *right, *exp;\
\
  exp = parse_ ## b ## _expression(str, eq, err);	\
  if (exp == NULL) {\
    return NULL;\
  }\
\
  for (;;) {\
    token = my_get_token(str, eq);\
    if (token == NULL) {\
      *err = MATH_ERROR_MEMORY;\
      math_expression_free(exp);\
      return NULL;\
    }\
\
    switch (token->type) {\
    case MATH_TOKEN_TYPE_OPERATOR:\
      switch (token->data.op) {\
      case c:\
	right = parse_ ## b ## _expression(str, eq, err);	\
	if (right == NULL) {\
	  math_expression_free(exp);\
	  math_scanner_free_token(token);\
	  return NULL;\
	}\
	left = exp;\
	math_scanner_free_token(token);\
	exp = math_binary_expression_new(d, eq, left, right, err);	\
	if (exp == NULL) {					\
	  math_expression_free(left);				\
	  math_expression_free(right);				\
	  return NULL;						\
	}							\
        break;\
      default:\
        unget_token(token);\
        goto End;\
      }\
      break;\
    default:\
      unget_token(token);\
      goto End;\
    }\
  }\
\
 End:\
  return exp;\
}

#define CREATE_PARSER2_FUNC(a, b, c, d, e, f) parse_ ## a ## _expression(struct math_string *str, MathEquation *eq, int *err) \
{\
  struct math_token *token;\
  MathExpression *left, *right, *exp;\
  enum MATH_EXPRESSION_TYPE type;\
\
  exp = parse_ ## b ## _expression(str, eq, err);	\
  if (exp == NULL) {\
    return NULL;\
  }\
\
  for (;;) {\
    token = my_get_token(str, eq);\
    if (token == NULL) {\
      *err = MATH_ERROR_MEMORY;\
      math_expression_free(exp);\
      return NULL;\
    }\
\
    switch (token->type) {\
    case MATH_TOKEN_TYPE_OPERATOR:\
      switch (token->data.op) {\
      case c:\
      case e:\
	right = parse_ ## b ## _expression(str, eq, err);	\
	if (right == NULL) {\
	  math_expression_free(exp);\
	  math_scanner_free_token(token);\
	  return NULL;\
	}\
	if (token->data.op == c) {\
	  type = d;\
	} else if (token->data.op == e) {\
	  type = f;\
	} else {\
	  type = 0; /* never reached */\
	}\
	left = exp;\
	math_scanner_free_token(token);\
	exp = math_binary_expression_new(type, eq, left, right, err);	\
	if (exp == NULL) {\
	  math_expression_free(left);\
	  math_expression_free(right);\
	  return NULL;\
	}\
        break;\
      default:\
        unget_token(token);\
        goto End;\
      }\
      break;\
    default:\
      unget_token(token);\
      goto End;\
    }\
  }\
\
 End:\
  return exp;\
}

#define CREATE_PARSER3_FUNC(a, b, c, d, e, f, g, h) parse_ ## a ## _expression(struct math_string *str, MathEquation *eq, int *err) \
{\
  struct math_token *token;\
  MathExpression *left, *right, *exp;\
  enum MATH_EXPRESSION_TYPE type;\
\
  exp = parse_ ## b ## _expression(str, eq, err);	\
  if (exp == NULL) {\
    return NULL;\
  }\
\
  for (;;) {\
    token = my_get_token(str, eq);\
    if (token == NULL) {\
      *err = MATH_ERROR_MEMORY;\
      math_expression_free(exp);\
      return NULL;\
    }\
\
    switch (token->type) {\
    case MATH_TOKEN_TYPE_OPERATOR:\
      switch (token->data.op) {\
      case c:\
      case e:\
      case g:\
	right = parse_ ## b ## _expression(str, eq, err);	\
	if (right == NULL) {\
	  math_expression_free(exp);\
	  math_scanner_free_token(token);\
	  return NULL;\
	}\
	if (token->data.op == c) {\
	  type = d;\
	} else if (token->data.op == e) {\
	  type = f;\
	} else if (token->data.op == g) {\
	  type = h;\
	} else {\
	  type = 0; /* never reached */\
	}\
	left = exp;\
	math_scanner_free_token(token);			\
	exp = math_binary_expression_new(type, eq, left, right, err);	\
	if (exp == NULL) {			       \
	  math_expression_free(left);\
	  math_expression_free(right);\
	  return NULL;\
	}\
        break;\
      default:\
        unget_token(token);\
        goto End;\
      }\
      break;\
    default:\
      unget_token(token);\
      goto End;\
    }\
  }\
\
 End:\
  return exp;\
}

#define CREATE_PARSER4_FUNC(a, b, c, d, e, f, g, h, i, j) parse_ ## a ## _expression(struct math_string *str, MathEquation *eq, int *err) \
{\
  struct math_token *token;\
  MathExpression *left, *right, *exp;\
  enum MATH_EXPRESSION_TYPE type;\
\
  exp = parse_ ## b ## _expression(str, eq, err);	\
  if (exp == NULL) {\
    return NULL;\
  }\
\
  for (;;) {\
    token = my_get_token(str, eq);\
    if (token == NULL) {\
      *err = MATH_ERROR_MEMORY;\
      math_expression_free(exp);\
      return NULL;\
    }\
\
    switch (token->type) {\
    case MATH_TOKEN_TYPE_OPERATOR:\
      switch (token->data.op) {\
      case c:\
      case e:\
      case g:\
      case i:\
	right = parse_ ## b ## _expression(str, eq, err);	\
	if (right == NULL) {\
	  math_expression_free(exp);\
	  math_scanner_free_token(token);\
	  return NULL;\
	}\
	if (token->data.op == c) {\
	  type = d;\
	} else if (token->data.op == e) {\
	  type = f;\
	} else if (token->data.op == g) {\
	  type = h;\
	} else if (token->data.op == i) {\
	  type = j;\
	} else {\
	  type = 0; /* never reached */\
	}\
	left = exp;\
	math_scanner_free_token(token);				\
	exp = math_binary_expression_new(type, eq, left, right, err);	\
	if (exp == NULL) {\
	  math_expression_free(left);\
	  math_expression_free(right);\
	  return NULL;\
	}\
        break;\
      default:\
        unget_token(token);\
        goto End;\
      }\
      break;\
    default:\
      unget_token(token);\
      goto End;\
    }\
  }\
\
 End:\
  return exp;\
}

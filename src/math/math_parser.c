/*
 * $Id: math_parser.c,v 1.19 2010-03-04 08:30:17 hito Exp $
 *
 */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <glib.h>

#include "object.h"

#include "math_equation.h"
#include "math_scanner.h"
#include "math_function.h"
#include "parse_bin_expression.h"

#define MATH_ARG_NUM 16

#define DEBUG 1

MathValue MATH_VALUE_ZERO = {0.0, MATH_VALUE_NORMAL};

static struct math_token *st_look_ahead_token = NULL;

static MathExpression * parse_expression(struct math_string *mstr, MathEquation *eq, int *err);
static MathExpression * parse_expression_list(struct math_string *mstr, MathEquation *eq, int inside_block, int *err);
static MathExpression * parse_unary_expression(struct math_string *mstr, MathEquation *eq, int *err);
static MathExpression * parse_block_expression(struct math_string *str, MathEquation *eq, int *err);

struct parsing_info
{
  struct math_string str;
  struct math_token *st_look_ahead_token;
  MathExpression *exp;
};

static struct math_token *
my_get_token(struct math_string *str)
{
  struct math_token *token;

  if (st_look_ahead_token) {
    token = st_look_ahead_token;
    st_look_ahead_token = token->next;
    token->next = NULL;
  } else {
    token = math_scanner_get_token(str);
  }

  return token;
}

static void
unget_token(struct math_token *token)
{
  token->next = st_look_ahead_token;
  st_look_ahead_token = token;
}

static void
free_token(struct math_token *token)
{
  struct math_token *tmp;

  while (token) {
    tmp = token->next;
    math_scanner_free_token(token);
    token = tmp;
  }
}

static void
reset_token(struct math_token *cur)
{
  free_token(st_look_ahead_token);
  st_look_ahead_token = cur;
}

static struct math_token *
dup_token(struct math_token *token)
{
  struct math_token *new_token;
  new_token = math_scanner_dup_token(token);
  if (token->next) {
    new_token->next = dup_token(token->next);
  }
  return new_token;
}

static struct parsing_info *
save_parsing_info(struct math_string *str, MathExpression *exp)
{
  struct parsing_info *info;
  info = g_malloc0(sizeof(*info));
  if (info == NULL) {
    return NULL;
  }
  info->str = *str;
  if (st_look_ahead_token) {
    info->st_look_ahead_token = dup_token(st_look_ahead_token);
    if (info->st_look_ahead_token == NULL) {
      g_free(info);
      return NULL;
    }
  }
  info->exp = exp;
  return info;
}

static void
free_parsing_info(struct parsing_info *info)
{
  if (info == NULL) {
    return;
  }
  if (info->st_look_ahead_token) {
    free_token(info->st_look_ahead_token);
  }
  g_free(info);
}

static MathExpression *
restore_parsing_info(struct parsing_info *info, struct math_string *str)
{
  if (info == NULL) {
    return NULL;
  }
  *str = info->str;
  reset_token(info->st_look_ahead_token);
  info->st_look_ahead_token = NULL;
  return info->exp;
}

static MathExpression *
parse_array_expression(struct math_string *str, MathEquation *eq, const char *name, int is_string, int *err)
{
  struct math_token *token;
  MathExpression *operand, *exp;

  token = my_get_token(str);
  if (token == NULL) {
    *err = MATH_ERROR_MEMORY;
    return NULL;
  }
  math_scanner_free_token(token);

  operand = parse_expression(str, eq, err);
  if (operand == NULL)
    return NULL;

  token = my_get_token(str);
  if (token == NULL) {
    *err = MATH_ERROR_MEMORY;
    math_expression_free(operand);
    return NULL;
  }

  if (token->type != MATH_TOKEN_TYPE_RB) {
    *err = MATH_ERROR_MISS_RB;
    math_equation_set_parse_error(eq, token->ptr, str);
    math_scanner_free_token(token);
    math_expression_free(operand);
    return NULL;
  }

  math_scanner_free_token(token);

  exp = math_array_expression_new(eq, name, operand, is_string, err);
  if (exp == NULL) {
    math_expression_free(operand);
    return NULL;
  }

  return exp;
}

static MathExpression *
parse_primary_expression(struct math_string *str, MathEquation *eq, int *err)
{
  struct math_token *token, *token2;
  MathExpression *exp;
  MathValue val;

  token = my_get_token(str);
  if (token == NULL) {
    *err = MATH_ERROR_MEMORY;
    return NULL;
  }

  switch (token->type) {
  case MATH_TOKEN_TYPE_NUMERIC:
    val.val = token->data.val.dnum;
    val.type = MATH_VALUE_NORMAL;
    exp = math_double_expression_new(eq, &val, err);
    break;
  case MATH_TOKEN_TYPE_DOUBLE_QUOTED_STRING:
    exp = math_string_expression_new(eq, token->data.str->str, TRUE, err);
    if (exp == NULL) {
      math_equation_set_parse_error(eq, token->ptr, str);
      math_scanner_free_token(token);
      return NULL;
    }
    break;
  case MATH_TOKEN_TYPE_SINGLE_QUOTED_STRING:
    exp = math_string_expression_new(eq, token->data.str->str, FALSE, err);
    break;
  case MATH_TOKEN_TYPE_UNTERMINATED_STRING:
    *err = MATH_ERROR_UNTERMINATED_STRING;
    math_equation_set_parse_error(eq, token->ptr, str);
    math_scanner_free_token(token);
    return NULL;
  case MATH_TOKEN_TYPE_STRING_VARIABLE:
    token2 = my_get_token(str);
    unget_token(token2);
    if (token2->type == MATH_TOKEN_TYPE_LB) {
      exp = parse_array_expression(str, eq, token->data.sym, TRUE, err);
      if (exp == NULL) {
	math_scanner_free_token(token);
	return NULL;
      }
    } else {
      exp = math_string_variable_expression_new(eq, token->data.sym, err);
    }
    break;
  case MATH_TOKEN_TYPE_SYMBOL:
    token2 = my_get_token(str);
    unget_token(token2);
    if (token2->type == MATH_TOKEN_TYPE_LB) {
      exp = parse_array_expression(str, eq, token->data.sym, FALSE, err);
      if (exp == NULL) {
	math_scanner_free_token(token);
	return NULL;
      }
    } else {
      exp = math_constant_expression_new(eq, token->data.sym, err);
      if (exp == NULL) {
	if (token->data.sym[0] == '%') {
	  exp = math_parameter_expression_new(eq, token->data.sym, err);
	  if (exp == NULL) {
	    math_equation_set_parse_error(eq, token->ptr, str);
	    math_scanner_free_token(token);
	    return NULL;
	  }
#if 0				/* length of a variable expression is less than 3 other than start with "_". */
	} else if (token->data.sym[0] == '_' || strlen(token->data.sym) < 3) {
	  exp = math_variable_expression_new(eq, token->data.sym, err);
	} else {
	  *err = MATH_ERROR_UNEXP_TOKEN;
	  math_equation_set_parse_error(eq, token->ptr, str);
	  math_scanner_free_token(token);
	  return NULL;
#else				/* length of a variable expression is not fixed. */
	} else {
	  exp = math_variable_expression_new(eq, token->data.sym, err);
#endif
	}
      }
    }
    break;
  case MATH_TOKEN_TYPE_LP:
    math_scanner_free_token(token);
    exp = parse_expression(str, eq, err);
    if (exp == NULL)
      return NULL;

    token = my_get_token(str);
    if (token == NULL) {
      *err = MATH_ERROR_MEMORY;
      math_expression_free(exp);
      return NULL;
    }

    if (token->type != MATH_TOKEN_TYPE_RP) {
      *err = MATH_ERROR_MISS_RP;
      math_equation_set_parse_error(eq, token->ptr, str);
      math_scanner_free_token(token);
      math_expression_free(exp);
      return NULL;
    }
    break;
  case MATH_TOKEN_TYPE_LC:
    exp = parse_block_expression(str, eq, err);
    break;
  case MATH_TOKEN_TYPE_EOEQ_ASSIGN:
  case MATH_TOKEN_TYPE_EOEQ:
    *err = MATH_ERROR_EOEQ;
    math_scanner_free_token(token);
    return NULL;
  default:
    *err = MATH_ERROR_UNEXP_TOKEN;
    math_equation_set_parse_error(eq, token->ptr, str);
    math_scanner_free_token(token);
    return NULL;
  }

  math_scanner_free_token(token);

  return exp;
}

static void
free_arg_list(MathExpression **argv)
{
  int i;

  for (i = 0; argv[i]; i++) {
    math_expression_free(argv[i]);
  }
}

static MathExpression *
get_argument(struct math_string *str, MathEquation *eq, struct math_function_parameter *fprm, int i, int *err)
{
  MathExpression *exp;
  int argc;

  argc = math_function_get_arg_type_num(fprm);
  if (fprm->arg_type && i < argc) {
    struct math_token *token;
    switch (fprm->arg_type[i]) {
    case MATH_FUNCTION_ARG_TYPE_ARRAY:
      token = my_get_token(str);
      if (token->type != MATH_TOKEN_TYPE_SYMBOL) {
	*err = MATH_ERROR_INVALID_ARG;
	math_equation_set_parse_error(eq, token->ptr, str);
	math_scanner_free_token(token);
	/* invalid argument */
	return NULL;
      }
      exp = math_array_argument_expression_new(eq, token->data.sym, err);
      math_scanner_free_token(token);
      break;
    case MATH_FUNCTION_ARG_TYPE_STRING_ARRAY:
      token = my_get_token(str);
      if (token->type != MATH_TOKEN_TYPE_STRING_VARIABLE) {
	*err = MATH_ERROR_INVALID_ARG;
	math_equation_set_parse_error(eq, token->ptr, str);
	math_scanner_free_token(token);
	/* invalid argument */
	return NULL;
      }
      exp = math_string_array_argument_expression_new(eq, token->data.sym, err);
      math_scanner_free_token(token);
      break;
    case MATH_FUNCTION_ARG_TYPE_ARRAY_COMMON:
      token = my_get_token(str);
      if (token->type == MATH_TOKEN_TYPE_SYMBOL) {
        exp = math_array_argument_expression_new(eq, token->data.sym, err);
        math_scanner_free_token(token);
      } else if (token->type == MATH_TOKEN_TYPE_STRING_VARIABLE) {
        exp = math_string_array_argument_expression_new(eq, token->data.sym, err);
        math_scanner_free_token(token);
      } else {
	*err = MATH_ERROR_INVALID_ARG;
	math_equation_set_parse_error(eq, token->ptr, str);
	math_scanner_free_token(token);
        return NULL;
	/* invalid argument */
      }
      break;
    default:
      exp = parse_expression(str, eq, err);
      break;
    }
  } else {
    exp = parse_expression(str, eq, err);
  }

  return exp;
}

static int
parse_argument_list(struct math_string *str, MathEquation *eq, struct math_function_parameter *fprm, MathExpression ***buf, int argc, int *err)
{
  struct math_token *token;
  int i, n;
  MathExpression *exp, **tmp, **argv;

  argv = *buf;

  token = my_get_token(str);
  unget_token(token);
  if (token->type == MATH_TOKEN_TYPE_RP) {
    return 0;
  }

  n = 0;
  exp = get_argument(str, eq, fprm, n, err);
  if (exp == NULL) {
    return -1;
  }

  argv[0] = exp;
  argv[1] = NULL;
  for (i = 1; ; i++) {
    token = my_get_token(str);
    if (token == NULL) {
      *err = MATH_ERROR_MEMORY;
      free_arg_list(argv);
      return -1;
    }

    if (token->type != MATH_TOKEN_TYPE_COMMA) {
      unget_token(token);
      break;
    }
    math_scanner_free_token(token);

    n++;
    exp = get_argument(str, eq, fprm, n, err);
    if (exp == NULL) {
      free_arg_list(argv);
      return -1;
    }

    if (i >= argc) {
      int m;

      m = argc * 2 + 1;
      tmp = g_realloc(argv, m * sizeof(*argv));
      if (tmp == NULL) {
	math_expression_free(exp);
	free_arg_list(argv);
	return -1;
      }
      argv = tmp;
      *buf = tmp;
      memset(argv + argc, 0, m - argc);
      argc = m - 1;
    }

    argv[i] = exp;
    argv[i + 1] = NULL;
  }

  *buf = argv;

  return i;
}

static MathExpression *
create_math_func(struct math_string *str, MathEquation *eq, struct math_token *name, int *err)
{
  struct math_function_parameter *fprm;
  int arg_max, argc, pos_id;
  MathExpression **argv, *exp;
  struct math_token *token;

  fprm = math_equation_get_func(eq, name->data.sym);
  if (fprm == NULL) {
    *err = MATH_ERROR_UNKNOWN_FUNC;
    math_equation_set_parse_error(eq, name->ptr, str);
    return NULL;
  }

  arg_max = (fprm->argc < 0) ? MATH_ARG_NUM : fprm->argc;

  argv = g_malloc0((arg_max + 1) * sizeof(*argv));
  if (argv == NULL) {
    *err = MATH_ERROR_MEMORY;
    return NULL;
  }

  argc = parse_argument_list(str, eq, fprm, &argv, arg_max, err);
  if (argc < 0) {
    g_free(argv);
    return NULL;
  }

  token = my_get_token(str);
  if (token->type != MATH_TOKEN_TYPE_RP) {
    *err = MATH_ERROR_MISS_RP;
    math_equation_set_parse_error(eq, token->ptr, str);
    free_arg_list(argv);
    g_free(argv);
    math_scanner_free_token(token);
    return NULL;
  }

  if (argc < fprm->argc) {
    int i;
    for (i = argc; i < fprm->argc; i++) {
      argv[i] = math_double_expression_new(eq, &MATH_VALUE_ZERO, err);
      if (argv[i] == NULL) {
        math_scanner_free_token(token);
	free_arg_list(argv);
	g_free(argv);
	return NULL;
      }
    }
    argc = fprm->argc;
  } else if (fprm->argc >= 0 && argc > fprm->argc) {
    *err = MATH_ERROR_ARG_NUM;
    math_equation_set_parse_error(eq, token->ptr, str);
    math_equation_set_func_arg_num_error(eq, fprm, argc);
    math_scanner_free_token(token);
    free_arg_list(argv);
    g_free(argv);
    return NULL;
  }

  pos_id = math_equation_add_pos_func(eq, fprm);
  if (pos_id == MATH_ERROR_INVALID_FUNC) {
    *err = MATH_ERROR_INVALID_FUNC;
    math_equation_set_parse_error(eq, token->ptr, str);
    math_equation_set_func_error(eq, fprm);
    math_scanner_free_token(token);
    free_arg_list(argv);
    g_free(argv);
    return NULL;
  }

  if (fprm->type == MATH_FUNCTION_TYPE_CALLBACK && eq->func_def) {
    *err = MATH_ERROR_INVALID_FUNC;
    math_equation_set_parse_error(eq, token->ptr, str);
    math_equation_set_func_error(eq, fprm);
    math_scanner_free_token(token);
    free_arg_list(argv);
    g_free(argv);
    return NULL;
  }

  exp = math_func_call_expression_new(eq, fprm, argc, argv, pos_id, err);
  if (exp == NULL) {
    if (*err == MATH_ERROR_INVALID_ARG) {
      math_equation_set_parse_error(eq, token->ptr, str);
    }
    math_scanner_free_token(token);
    free_arg_list(argv);
    g_free(argv);
    return NULL;
  }
  math_scanner_free_token(token);

  return exp;
}

static MathExpression *
parse_func_expression(struct math_string *str, MathEquation *eq, int *err)
{
  struct math_token *token1, *token2;
  MathExpression *exp;

  token1 = my_get_token(str);
  if (token1 == NULL) {
    return NULL;
  }

  switch (token1->type) {
  case MATH_TOKEN_TYPE_SYMBOL:
    token2 = my_get_token(str);
    if (token2 == NULL) {
      math_scanner_free_token(token1);
      return NULL;
    }
    switch (token2->type) {
    case MATH_TOKEN_TYPE_LP:
      exp = create_math_func(str, eq, token1, err);
      math_scanner_free_token(token2);
      math_scanner_free_token(token1);
      break;
    default:
      unget_token(token2);
      unget_token(token1);
      exp = parse_primary_expression(str, eq, err);
    }
    break;
  default:
    unget_token(token1);
    exp = parse_primary_expression(str, eq, err);
  }

  return exp;
}


static MathExpression *
parse_factorial_expression(struct math_string *str, MathEquation *eq, int *err)
{
  struct math_token *token;
  MathExpression *operand, *exp;

  exp = parse_func_expression(str, eq, err);
  if (exp == NULL) {
    return NULL;
  }

  for (;;) {
    token = my_get_token(str);
    if (token == NULL) {
      *err = MATH_ERROR_MEMORY;
      math_expression_free(exp);
      return NULL;
    }

    switch (token->type) {
    case MATH_TOKEN_TYPE_OPERATOR:
      switch (token->data.op) {
      case MATH_OPERATOR_TYPE_FACT:
	math_scanner_free_token(token);

	operand = exp;
	exp = math_unary_expression_new(MATH_EXPRESSION_TYPE_FACT, eq, operand, err);
	if (exp == NULL) {
	  return NULL;
	}
	break;
      default:
	unget_token(token);
	goto End;
      }
      break;
    default:
      unget_token(token);
      goto End;
    }
  }

 End:
  return exp;
}

static MathExpression *
parse_power_expression(struct math_string *str, MathEquation *eq, int *err)
{
  struct math_token *token;
  MathExpression *left, *right, *exp;

  exp = parse_factorial_expression(str, eq, err);
  if (exp == NULL) {
    return NULL;
  }

  token = my_get_token(str);
  if (token == NULL) {
    *err = MATH_ERROR_MEMORY;
    math_expression_free(exp);
    return NULL;
  }

  switch (token->type) {
  case MATH_TOKEN_TYPE_OPERATOR:
    switch (token->data.op) {
    case MATH_OPERATOR_TYPE_POW:
      right = parse_unary_expression(str, eq, err);
      if (right == NULL) {
	math_scanner_free_token(token);
	return NULL;
      }
      left = exp;
      exp = math_binary_expression_new(MATH_EXPRESSION_TYPE_POW, eq, left, right, err);
      if (exp == NULL) {
	math_expression_free(left);
	math_expression_free(right);
	math_scanner_free_token(token);
	return NULL;
      }
      math_scanner_free_token(token);
      break;
    default:
      unget_token(token);
      goto End;
    }
    break;
  default:
    unget_token(token);
    goto End;
  }

 End:
  return exp;
}

static MathExpression *
parse_unary_expression(struct math_string *str, MathEquation *eq, int *err)
{
  struct math_token *token;
  MathExpression *exp, *operand;

  token = my_get_token(str);
  if (token == NULL) {
    *err = MATH_ERROR_MEMORY;
    return NULL;
  }

  switch (token->type) {
  case MATH_TOKEN_TYPE_OPERATOR:
    switch (token->data.op) {
    case MATH_OPERATOR_TYPE_PLUS:
      exp = parse_unary_expression(str, eq, err);
      break;
    case MATH_OPERATOR_TYPE_MINUS:
      operand = parse_unary_expression(str, eq, err);
      if (operand == NULL) {
	math_scanner_free_token(token);
	return NULL;
      }
      exp = math_unary_expression_new(MATH_EXPRESSION_TYPE_MINUS, eq, operand, err);
      if (exp == NULL) {
	math_scanner_free_token(token);
	math_expression_free(operand);
	return NULL;
      }
      break;
    case MATH_OPERATOR_TYPE_FACT:
      operand = parse_unary_expression(str, eq, err);
      if (operand == NULL) {
	math_scanner_free_token(token);
	return NULL;
      }
      exp = math_unary_expression_new(MATH_EXPRESSION_TYPE_NOT, eq, operand, err);
      if (exp == NULL) {
	math_scanner_free_token(token);
	math_expression_free(operand);
	return NULL;
      }
      break;
    default:
      *err = MATH_ERROR_UNEXP_OPE;
      math_equation_set_parse_error(eq, token->ptr, str);
      math_scanner_free_token(token);
      return NULL;
    }
    math_scanner_free_token(token);
    break;
  default:
    unget_token(token);
    exp = parse_power_expression(str, eq, err);
  }

  return exp;
}

static MathExpression *
parse_multiplicative_expression(struct math_string *str, MathEquation *eq, int *err)
{
  struct math_token *token;
  MathExpression *left, *right, *exp;
  enum MATH_EXPRESSION_TYPE type;

  exp = parse_unary_expression(str, eq, err);
  if (exp == NULL) {
    return NULL;
  }
  for (;;) {
    token = my_get_token(str);
    if (token == NULL) {
      *err = MATH_ERROR_MEMORY;
      math_expression_free(exp);
      return NULL;
    }
    switch (token->type) {
    case MATH_TOKEN_TYPE_OPERATOR:
      switch (token->data.op) {
      case MATH_OPERATOR_TYPE_MUL:
      case MATH_OPERATOR_TYPE_DIV:
      case MATH_OPERATOR_TYPE_MOD:
	right = parse_unary_expression(str, eq, err);
	if (right == NULL) {
	  math_expression_free(exp);
	  math_scanner_free_token(token);
	  return NULL;
	}
	if (token->data.op == MATH_OPERATOR_TYPE_MUL) {
	  type = MATH_EXPRESSION_TYPE_MUL;
	} else if (token->data.op == MATH_OPERATOR_TYPE_DIV) {
	  type = MATH_EXPRESSION_TYPE_DIV;
	} else if (token->data.op == MATH_OPERATOR_TYPE_MOD) {
	  type = MATH_EXPRESSION_TYPE_MOD;
	} else {
	  type = 0;
	}
	left = exp;
	math_scanner_free_token(token);
	exp = math_binary_expression_new(type, eq, left, right, err);
	if (exp == NULL) {
	  math_expression_free(left);
	  math_expression_free(right);
	  return NULL;
	}
	break;
      default:
	unget_token(token);
	goto End;
      }
      break;
    case MATH_TOKEN_TYPE_CONST:
    case MATH_TOKEN_TYPE_NUMERIC:
    case MATH_TOKEN_TYPE_SYMBOL:
    case MATH_TOKEN_TYPE_LP:
      unget_token(token);
      right = parse_unary_expression(str, eq, err);
      if (right == NULL) {
	math_expression_free(exp);
	return NULL;
      }
      left = exp;
      exp = math_binary_expression_new(MATH_EXPRESSION_TYPE_MUL, eq, left, right, err);
      if (exp == NULL) {
	math_expression_free(left);
	math_expression_free(right);
	return NULL;
      }
      break;
    default:
      unget_token(token);
      goto End;
    }
  }
 End:
  return exp;
}

static MathExpression *
CREATE_PARSER2_FUNC(additive, multiplicative,
		    MATH_OPERATOR_TYPE_PLUS, MATH_EXPRESSION_TYPE_ADD,
		    MATH_OPERATOR_TYPE_MINUS, MATH_EXPRESSION_TYPE_SUB);

static MathExpression *
CREATE_PARSER4_FUNC(relation, additive,
		    MATH_OPERATOR_TYPE_GT, MATH_EXPRESSION_TYPE_GT,
		    MATH_OPERATOR_TYPE_GE, MATH_EXPRESSION_TYPE_GE,
		    MATH_OPERATOR_TYPE_LE, MATH_EXPRESSION_TYPE_LE,
		    MATH_OPERATOR_TYPE_LT, MATH_EXPRESSION_TYPE_LT);

static MathExpression *
CREATE_PARSER2_FUNC(equality, relation,
		    MATH_OPERATOR_TYPE_EQ, MATH_EXPRESSION_TYPE_EQ,
		    MATH_OPERATOR_TYPE_NE, MATH_EXPRESSION_TYPE_NE);

static MathExpression *
CREATE_PARSER_FUNC(and, equality, MATH_OPERATOR_TYPE_AND, MATH_EXPRESSION_TYPE_AND);

static MathExpression *
CREATE_PARSER_FUNC(or, and, MATH_OPERATOR_TYPE_OR, MATH_EXPRESSION_TYPE_OR);

static MathExpression *
create_variable_assign_expression(MathEquation *eq, enum MATH_OPERATOR_TYPE op,
				  MathExpression *lexp, MathExpression *rexp, int *err)
{
  MathExpression *exp, *bin;

  bin = NULL;
  switch (op) {
  case MATH_OPERATOR_TYPE_ASSIGN:
    break;
  case MATH_OPERATOR_TYPE_POW_ASSIGN:
    bin = math_binary_expression_new(MATH_EXPRESSION_TYPE_POW, eq, lexp, rexp, err);
    if (bin == NULL)
      goto ErrEnd;

    rexp = bin;
    break;
  case MATH_OPERATOR_TYPE_MOD_ASSIGN:
    bin = math_binary_expression_new(MATH_EXPRESSION_TYPE_MOD, eq, lexp, rexp, err);
    if (bin == NULL)
      goto ErrEnd;

    rexp = bin;
    break;
  case MATH_OPERATOR_TYPE_DIV_ASSIGN:
    bin = math_binary_expression_new(MATH_EXPRESSION_TYPE_DIV, eq, lexp, rexp, err);
    if (bin == NULL)
      goto ErrEnd;

    rexp = bin;
    break;
  case MATH_OPERATOR_TYPE_MUL_ASSIGN:
    bin = math_binary_expression_new(MATH_EXPRESSION_TYPE_MUL, eq, lexp, rexp, err);
    if (bin == NULL)
      goto ErrEnd;

    rexp = bin;
    break;
  case MATH_OPERATOR_TYPE_PLUS_ASSIGN:
    bin = math_binary_expression_new(MATH_EXPRESSION_TYPE_ADD, eq, lexp, rexp, err);
    if (bin == NULL)
      goto ErrEnd;

    rexp = bin;
    break;
  case MATH_OPERATOR_TYPE_MINUS_ASSIGN:
    bin = math_binary_expression_new(MATH_EXPRESSION_TYPE_SUB, eq, lexp, rexp, err);
    if (bin == NULL)
      goto ErrEnd;

    rexp = bin;
    break;
  default:
    return NULL;
  }

  exp = math_expression_new(MATH_EXPRESSION_TYPE_VARIABLE, eq, err);
  if (exp == NULL) {
    math_expression_free(bin);
    return NULL;
  }

  exp->u.index = lexp->u.index;

  lexp = math_assign_expression_new(MATH_EXPRESSION_TYPE_ASSIGN, eq, exp, rexp, op, err);
  if (lexp == NULL) {
    math_expression_free(exp);
    math_expression_free(bin);
  }

  return lexp;

 ErrEnd:
  math_expression_free(lexp);
  math_expression_free(rexp);

  return NULL;
}

static MathExpression *
parse_assign_expression(struct math_string *str, MathEquation *eq, enum MATH_OPERATOR_TYPE op, MathExpression *lexp, int *err)
{
  MathExpression *exp, *rexp;

  rexp = parse_expression(str, eq, err);
  if (rexp == NULL) {
    return NULL;
  }

  if (lexp->type == MATH_EXPRESSION_TYPE_VARIABLE && op != MATH_OPERATOR_TYPE_ASSIGN) {
    exp = create_variable_assign_expression(eq, op, lexp, rexp, err);
  } else {
    exp = math_assign_expression_new(MATH_EXPRESSION_TYPE_ASSIGN, eq, lexp, rexp, op, err);
    if (exp == NULL) {
      math_expression_free(rexp);
    }
  }

  return exp;
}

static int
parse_parameter_list(struct math_string *str, MathExpression *func, int *err)
{
  struct math_token *token;
  enum MATH_FUNCTION_ARG_TYPE type;

  token = my_get_token(str);
  if (token == NULL) {
    *err = MATH_ERROR_MEMORY;
    return 1;
  }

  if (token->type != MATH_TOKEN_TYPE_LP) {
    math_scanner_free_token(token);
    return 1;
  }
  math_scanner_free_token(token);

  token = my_get_token(str);
  if (token->type == MATH_TOKEN_TYPE_RP) {
    return 0;
  }

  unget_token(token);
  for (;;) {
    token = my_get_token(str);
    if (token == NULL) {
      *err = MATH_ERROR_MEMORY;
      return 1;
    }

    switch (token->type) {
    case MATH_TOKEN_TYPE_ARRAY_PREFIX:
      math_scanner_free_token(token);
      token = my_get_token(str);
      if (token->type == MATH_TOKEN_TYPE_SYMBOL) {
	type = MATH_FUNCTION_ARG_TYPE_ARRAY;
      } else {
	type = MATH_FUNCTION_ARG_TYPE_STRING_ARRAY;
      }
      break;
    case MATH_TOKEN_TYPE_STRING_VARIABLE:
      type = MATH_FUNCTION_ARG_TYPE_STRING;
      break;
    default:
      type = MATH_FUNCTION_ARG_TYPE_DOUBLE;
      break;
    }

    if (token->type != MATH_TOKEN_TYPE_SYMBOL &&
	token->type != MATH_TOKEN_TYPE_STRING_VARIABLE) {
      math_scanner_free_token(token);
      return 1;
    }

    if (math_function_expression_add_arg(func, token->data.sym, type)) {
      math_scanner_free_token(token);
      return 1;
    }
    math_scanner_free_token(token);

    token = my_get_token(str);
    if (token == NULL) {
      *err = MATH_ERROR_MEMORY;
      return 1;
    }

    if (token->type != MATH_TOKEN_TYPE_COMMA) {
      unget_token(token);
      break;
    }
    math_scanner_free_token(token);
  }

  token = my_get_token(str);
  if (token == NULL) {
    *err = MATH_ERROR_MEMORY;
    return 1;
  }

  if (token->type != MATH_TOKEN_TYPE_RP) {
    math_scanner_free_token(token);
    return 1;
  }
  math_scanner_free_token(token);

  return 0;
}

static MathExpression *
parse_block_expression(struct math_string *str, MathEquation *eq, int *err)
{
  struct math_token *token;
  MathExpression *exp, *block;

  token = my_get_token(str);
  if (token->type == MATH_TOKEN_TYPE_RC) {
    MathValue val;
    math_scanner_free_token(token);
    val.val = 0;
    val.type = MATH_VALUE_NORMAL;
    return math_double_expression_new(eq, &val, err);
  }
  unget_token(token);

  exp = parse_expression_list(str, eq, 1, err);
  if (exp == NULL)
    return NULL;

  token = my_get_token(str);
  if (token->type != MATH_TOKEN_TYPE_RC) {
    *err = MATH_ERROR_MISS_RC;
    math_equation_set_parse_error(eq, token->ptr, str);
    math_scanner_free_token(token);
    math_expression_free(exp);
    return NULL;
  }
  math_scanner_free_token(token);

  block = math_expression_new(MATH_EXPRESSION_TYPE_BLOCK, eq, err);
  if (block == NULL) {
    math_expression_free(exp);
    return NULL;
  }
  block->u.exp = exp;
  return block;
}

static void
free_func_prm(struct math_function_parameter *prm)
{
  if (prm == NULL)
    return;

  if (prm->arg_type) {
    g_free(prm->arg_type);
  }

  if (prm->opt_usr) {
    math_expression_free(prm->opt_usr);
  }

  if (prm->name) {
    g_free(prm->name);
  }

  g_free(prm);
}

MathExpression *
parse_func_def_expression(struct math_string *str, MathEquation *eq, int *err)
{
  struct math_token *fname;
  MathExpression *exp, *block;
  struct math_token *token;

  /* get name of the function */
  fname = my_get_token(str);
  if (fname == NULL) {
    return NULL;
  }

  if (fname->type != MATH_TOKEN_TYPE_SYMBOL) {
    math_scanner_free_token(fname);
    return NULL;
  }

  exp = math_function_expression_new(eq, fname->data.sym, err);
  if (exp == NULL) {
    math_scanner_free_token(fname);
    return NULL;
  }

  /* get parameters */
  if (parse_parameter_list(str, exp, err)) {
    math_equation_finish_user_func_definition(eq, NULL, NULL, NULL, NULL);
    free_func_prm(exp->u.func.fprm);
    math_scanner_free_token(fname);
    math_expression_free(exp);
    return NULL;
  }

  if (math_function_expression_register_arg(exp)) {
    math_equation_finish_user_func_definition(eq, NULL, NULL, NULL, NULL);
    free_func_prm(exp->u.func.fprm);
    math_scanner_free_token(fname);
    math_expression_free(exp);
  }

  /* get block */
  token = my_get_token(str);
  if (token == NULL) {
    return NULL;
  }
  if (token->type != MATH_TOKEN_TYPE_LC) {
    math_scanner_free_token(token);
    return NULL;
  }
  math_scanner_free_token(token);
  block = parse_block_expression(str, eq, err);
  if (block == NULL) {
    math_equation_finish_user_func_definition(eq, NULL, NULL, NULL, NULL);
    free_func_prm(exp->u.func.fprm);
    math_scanner_free_token(fname);
    math_expression_free(exp);
    return NULL;
  }

  if (math_function_expression_set_function(eq, exp, fname->data.sym, block)) {
    math_equation_finish_user_func_definition(eq, NULL, NULL, NULL, NULL);
    free_func_prm(exp->u.func.fprm);
    math_scanner_free_token(fname);
    math_expression_free(exp);
    return NULL;
  }

  math_scanner_free_token(fname);

  return exp;
}

MathExpression *
parse_const_def_expression(struct math_string *str, MathEquation *eq, int *err)
{
  struct math_token *cname, *token;
  MathExpression *exp, *cdef;

  if (eq->func_def) {
    *err = MATH_ERROR_INVALID_CDEF;
    return NULL;
  }

  /* get name of the constant */
  cname = my_get_token(str);
  if (cname == NULL) {
    return NULL;
  }

  if (cname->type != MATH_TOKEN_TYPE_SYMBOL) {
    *err = MATH_ERROR_UNEXP_TOKEN;
    math_equation_set_parse_error(eq, cname->ptr, str);
    math_scanner_free_token(cname);
    return NULL;
  }

  token = my_get_token(str);
  if (token == NULL) {
    *err = MATH_ERROR_MEMORY;
    math_scanner_free_token(cname);
    return NULL;
  }

  if ((token->type != MATH_TOKEN_TYPE_OPERATOR &&
       token->type != MATH_TOKEN_TYPE_EOEQ_ASSIGN) ||
      token->data.op != MATH_OPERATOR_TYPE_ASSIGN) {
    if (token->type == MATH_TOKEN_TYPE_EOEQ) {
      *err = MATH_ERROR_EOEQ;
    } else {
      *err = MATH_ERROR_UNEXP_TOKEN;
      math_equation_set_parse_error(eq, token->ptr, str);
    }
    math_scanner_free_token(token);
    math_scanner_free_token(cname);
    return NULL;
  }

  eq->func_def = TRUE;
  exp = parse_expression(str, eq, err);
  eq->func_def = FALSE;
  if (exp == NULL) {
    math_scanner_free_token(cname);
    math_scanner_free_token(token);
    return NULL;
  }

  cdef = math_constant_definition_expression_new(eq, cname->data.sym, exp, err);
  math_scanner_free_token(cname);
  if (cdef == NULL) {
    math_equation_set_parse_error(eq, token->ptr, str);
    math_expression_free(exp);
    math_scanner_free_token(token);
    return NULL;
  }
  math_scanner_free_token(token);

  return cdef;
}

static MathExpression *
parse_string_assign_expression(struct math_string *str, MathEquation *eq, struct math_token *token, MathExpression *lexp, int *err)
{
  MathExpression *exp, *rexp;

  if ((token->type != MATH_TOKEN_TYPE_OPERATOR &&
       token->type != MATH_TOKEN_TYPE_EOEQ_ASSIGN) ||
      token->data.op != MATH_OPERATOR_TYPE_ASSIGN) {
    *err = MATH_ERROR_UNEXP_OPE;
    math_equation_set_parse_error(eq, token->ptr, str);
    return NULL;
  }

  rexp = parse_expression(str, eq, err);
  if (rexp == NULL) {
    return NULL;
  }

  exp = math_assign_expression_new(MATH_EXPRESSION_TYPE_STRING_ASSIGN, eq, lexp, rexp, token->data.op, err);
  if (exp == NULL) {
    math_expression_free(rexp);
    return NULL;
  }
  return exp;
}

static MathExpression *
parse_expression(struct math_string *str, MathEquation *eq, int *err)
{
  struct math_token *token;
  MathExpression *exp, *prev_exp;

  exp = parse_or_expression(str, eq, err);
  if (exp == NULL)
    return NULL;

  switch (exp->type) {
  case MATH_EXPRESSION_TYPE_VARIABLE:
  case MATH_EXPRESSION_TYPE_STRING_VARIABLE:
  case MATH_EXPRESSION_TYPE_ARRAY:
  case MATH_EXPRESSION_TYPE_STRING_ARRAY:
    break;
  default:
    goto End;
  }

  token = my_get_token(str);
  if (token == NULL) {
    *err = MATH_ERROR_MEMORY;
    math_expression_free(exp);
    return NULL;
  }

  switch (token->type) {
  case MATH_TOKEN_TYPE_EOEQ_ASSIGN:
  case MATH_TOKEN_TYPE_OPERATOR:
    switch (token->data.op) {
    case MATH_OPERATOR_TYPE_POW_ASSIGN:
    case MATH_OPERATOR_TYPE_MOD_ASSIGN:
    case MATH_OPERATOR_TYPE_DIV_ASSIGN:
    case MATH_OPERATOR_TYPE_MUL_ASSIGN:
    case MATH_OPERATOR_TYPE_PLUS_ASSIGN:
    case MATH_OPERATOR_TYPE_MINUS_ASSIGN:
      if (exp->type == MATH_EXPRESSION_TYPE_STRING_VARIABLE ||
	  exp->type == MATH_EXPRESSION_TYPE_STRING_ARRAY) {
	*err = MATH_ERROR_UNEXP_OPE;
	math_equation_set_parse_error(eq, token->ptr, str);
	math_scanner_free_token(token);
	math_expression_free(exp);
	return NULL;
      }
      /* fall through */
    case MATH_OPERATOR_TYPE_ASSIGN:
      prev_exp = exp;
      if (exp->type == MATH_EXPRESSION_TYPE_STRING_VARIABLE ||
	  exp->type == MATH_EXPRESSION_TYPE_STRING_ARRAY) {
	exp = parse_string_assign_expression(str, eq, token, exp, err);
      } else {
	exp = parse_assign_expression(str, eq, token->data.op, exp, err);
      }
      if (exp == NULL) {
	if (token->type == MATH_TOKEN_TYPE_EOEQ_ASSIGN && *err == MATH_ERROR_EOEQ) {
	  *err = MATH_ERROR_NONE; /* Fix-me: this accepts an invalid equation such as "b=2(" */
	  exp = prev_exp;
	} else {
	  math_expression_free(prev_exp);
	}
      }
      math_scanner_free_token(token);
      break;
    default:
      *err = MATH_ERROR_UNEXP_OPE;
      math_equation_set_parse_error(eq, token->ptr, str);
      math_scanner_free_token(token);
      math_expression_free(exp);
      return NULL;
    }
    break;
  case MATH_TOKEN_TYPE_RC:
  case MATH_TOKEN_TYPE_RP:
  case MATH_TOKEN_TYPE_RB:
  case MATH_TOKEN_TYPE_EOEQ:
  case MATH_TOKEN_TYPE_COMMA:
    unget_token(token);
    break;
  default:
    *err = MATH_ERROR_UNEXP_TOKEN;
    math_equation_set_parse_error(eq, token->ptr, str);
    math_scanner_free_token(token);
    math_expression_free(exp);
    return NULL;
    break;
  }

 End:
  return exp;
}

static MathExpression *
parse_expression_list(struct math_string *str, MathEquation *eq, int inside_block, int *err)
{
  struct math_token *token;
  MathExpression *exp, *prev, *top;

  top = prev = NULL;
  for (;;) {
    token = my_get_token(str);
    if (token == NULL) {
      *err = MATH_ERROR_MEMORY;
      math_expression_free(top);
      return NULL;
    }

    switch (token->type) {
    case MATH_TOKEN_TYPE_DEF:
      if (inside_block) {
	*err = MATH_ERROR_FDEF_NEST;
	math_equation_set_parse_error(eq, token->ptr, str);
	math_scanner_free_token(token);
	math_expression_free(top);
	return NULL;
      }
      math_scanner_free_token(token);
      exp = parse_func_def_expression(str, eq, err);
      if (exp == NULL) {
	math_expression_free(top);
	return NULL;
      }
      continue;
    case MATH_TOKEN_TYPE_CONST:
      if (inside_block) {
	*err = MATH_ERROR_INVALID_CDEF;
	math_equation_set_parse_error(eq, token->ptr, str);
	math_scanner_free_token(token);
	math_expression_free(top);
	return NULL;
      }
      math_scanner_free_token(token);
      exp = parse_const_def_expression(str, eq, err);
      if (exp == NULL) {
	math_expression_free(top);
	return NULL;
      }
      continue;
    case MATH_TOKEN_TYPE_EOEQ_ASSIGN:
    case MATH_TOKEN_TYPE_EOEQ:
      if (str->cur[0] == '\0') {
	if (inside_block) {
	  *err = MATH_ERROR_MISS_RC;
	  math_equation_set_parse_error(eq, token->ptr, str);
	  math_scanner_free_token(token);
	  math_expression_free(top);
	  return NULL;
	}
	math_scanner_free_token(token);
	goto End;
      }
      math_scanner_free_token(token);
      continue;
    case MATH_TOKEN_TYPE_RC:
      if (inside_block) {
	unget_token(token);
	goto End;
      }
      *err = MATH_ERROR_UNEXP_TOKEN;
      math_equation_set_parse_error(eq, token->ptr, str);
      math_scanner_free_token(token);
      math_expression_free(top);
      return NULL;
    default:
      unget_token(token);
      exp = parse_expression(str, eq, err);
   }

    if (exp == NULL) {
      math_expression_free(top);
      return NULL;
    }

    if (prev) {
      prev->next = exp;
    }

    if (top == NULL) {
      top = exp;
    }

    prev = exp;

    token = my_get_token(str);
    if (token == NULL) {
      *err = MATH_ERROR_MEMORY;
      math_expression_free(top);
      return NULL;
    }

    if (token->type != MATH_TOKEN_TYPE_EOEQ &&
	token->type != MATH_TOKEN_TYPE_EOEQ_ASSIGN) {
      if (token->type == MATH_TOKEN_TYPE_RC && inside_block) {
	unget_token(token);
	goto End;
      }
      *err = MATH_ERROR_UNEXP_TOKEN;
      math_equation_set_parse_error(eq, token->ptr, str);
      math_scanner_free_token(token);
      math_expression_free(top);
      return NULL;
    }
    unget_token(token);
  }

 End:
  return top;
}

MathExpression *
math_parser_parse(const char *line, MathEquation *eq, int *err)
{
  struct math_token *token, *tmp;
  MathExpression *exp;
  struct math_string str;

  *err = MATH_ERROR_NONE;

  math_scanner_init_string(&str, line);
  st_look_ahead_token = NULL;
  exp = parse_expression_list(&str, eq, 0, err);

  token = st_look_ahead_token;
  while (token) {
    tmp = token->next;
    math_scanner_free_token(token);
    token = tmp;
  }
  st_look_ahead_token = NULL;

  return exp;
}

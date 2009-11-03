#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "math_equation.h"

#define ERR_MSG_FUNC_NEST "function definition cannot be nested."
#define ERR_MSG_UNEXP_OP  "unexpected operator."
#define ERR_MSG_UNEXP_TOK "unexpected token."
#define ERR_MSG_ARG_NUM   "wrong number of arguments."
#define ERR_MSG_MISS_RP   "unexpected end of equation, expecting ')'."
#define ERR_MSG_MISS_RC   "unexpected end of equation, expecting '}'."
#define ERR_MSG_MISS_RB   "unexpected end of equation, expecting ']'."
#define ERR_MSG_UNKNOWN_F "unknown function."
#define ERR_MSG_INVARID_F "invarid function definition."
#define ERR_MSG_INVARID_P "invarid parameter."
#define ERR_MSG_MEMORY    "cannnot allocate enough memory."
#define ERR_MSG_UNKNOWN   "unknown error."
#define ERR_MSG_POS_FUNC  "the function annot be used in a user function."

static char *
check_error_position(MathEquation *eq, const char *code)
{
  int i, l, len;
  char *buf;

  if (code == NULL)
    return NULL;

  len = strlen(code);
  l = eq->err_info.pos - code;

  if (l < 0)
    return NULL;

  if (l >= len)
    return NULL;

  if (eq->err_info.pos[0] == ';' || eq->err_info.pos[0] == '=' || eq->err_info.pos[0] == '\0') 
    return NULL;

  len -= l;
  buf = malloc(len + 1);
  if (buf == NULL)
    return NULL;

  for (i = 0; i < len; i++) {
    if (eq->err_info.pos[i] == ';' || eq->err_info.pos[i] == '=')
      break;
    buf[i] = eq->err_info.pos[i];
  }
  buf[i] = '\0';

  return buf;
}

char *
math_err_get_error_message(MathEquation *eq, const char *code, int err)
{
  char *code_buf = NULL, *buf = NULL;

  switch (err) {
  case MATH_ERROR_NONE:
    break;
  case MATH_ERROR_EOEQ:
    break;
  case MATH_ERROR_FDEF_NEST:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf("%s\n  the error is found at: %s", ERR_MSG_FUNC_NEST, code_buf);
    } else {
      buf = strdup(ERR_MSG_FUNC_NEST);
    }
    break;
  case MATH_ERROR_UNEXP_OPE:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf("%s\n  the error is found at: %s", ERR_MSG_UNEXP_OP, code_buf);
    } else {
      buf = strdup(ERR_MSG_UNEXP_OP);
    }
    break;
  case MATH_ERROR_ARG_NUM:
    if (eq->err_info.func.fprm) {
      buf = g_strdup_printf("%s (%d for %d) '%s()'", ERR_MSG_ARG_NUM,
			    eq->err_info.func.arg_num,
			    eq->err_info.func.fprm->argc,
			    eq->err_info.func.fprm->name);
    } else {
      buf = strdup(ERR_MSG_ARG_NUM);
    }
    break;
  case MATH_ERROR_MISS_RP:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf("%s\n  the error is found at: %s", ERR_MSG_MISS_RP, code_buf);
    } else {
      buf = strdup(ERR_MSG_MISS_RP);
    }
    break;
  case MATH_ERROR_MISS_RB:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf("%s\n  the error is found at: %s", ERR_MSG_MISS_RB, code_buf);
    } else {
      buf = strdup(ERR_MSG_MISS_RB);
    }
    break;
  case MATH_ERROR_MISS_RC:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf("%s\n  the error is found at: %s", ERR_MSG_MISS_RC, code_buf);
    } else {
      buf = strdup(ERR_MSG_MISS_RC);
    }
    break;
  case MATH_ERROR_UNKNOWN_FUNC:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf("%s\n  the error is found at: %s", ERR_MSG_UNKNOWN_F, code_buf);
    } else {
      buf = strdup(ERR_MSG_UNKNOWN_F);
    }
    break;
  case MATH_ERROR_INVARID_FDEF:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf("%s\n  the error is found at: %s", ERR_MSG_INVARID_F, code_buf);
    } else {
      buf = strdup(ERR_MSG_INVARID_F);
    }
    break;
  case MATH_ERROR_UNEXP_TOKEN:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf("%s\n  the error is found at: %s", ERR_MSG_UNEXP_TOK, code_buf);
    } else {
      buf = strdup(ERR_MSG_UNEXP_TOK);
    }
    break;
  case MATH_ERROR_INVALID_PRM:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf("%s\n  the error is found at: %s", ERR_MSG_INVARID_P, code_buf);
    } else {
      buf = strdup(ERR_MSG_INVARID_P);
    }
    break;
  case MATH_ERROR_MEMORY:
    buf = strdup(ERR_MSG_MEMORY);
    break;
  case MATH_ERROR_UNKNOWN:
    buf = strdup(ERR_MSG_UNKNOWN);
    break;
  case MATH_ERROR_INVALID_FUNC:
    if (eq->err_info.func.fprm) {
      buf = g_strdup_printf("%s '%s()'", ERR_MSG_POS_FUNC, eq->err_info.func.fprm->name);
    } else {
      buf = strdup(ERR_MSG_POS_FUNC);
    }
    break;
  default:
    buf = strdup(ERR_MSG_UNKNOWN);
  }

  if (code_buf)
    free(code_buf);

  return buf;
}

/* 
 * $Id: math_error.c,v 1.10 2009-12-22 00:57:42 hito Exp $
 * 
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "common.h"
#include "math_equation.h"

#define ERR_MSG_EOEQ		N_("syntax error, unexpected end of equation.")
#define ERR_MSG_FUNC_NEST	N_("syntax error, function definition cannot be nested.")
#define ERR_MSG_UNEXP_OP	N_("syntax error, unexpected operator.")
#define ERR_MSG_UNEXP_TOK	N_("syntax error, unexpected token.")
#define ERR_MSG_ARG_NUM		N_("syntax error, wrong number of arguments.")
#define ERR_MSG_MISS_RP		N_("syntax error, unexpected end of equation, expecting ')'.")
#define ERR_MSG_MISS_RC		N_("syntax error, unexpected end of equation, expecting '}'.")
#define ERR_MSG_MISS_RB		N_("syntax error, unexpected end of equation, expecting ']'.")
#define ERR_MSG_INVALID_F	N_("syntax error, invalid function definition.")
#define ERR_MSG_INVALID_C	N_("syntax error, constant cannot be defined in a function definition.")
#define ERR_MSG_INVALID_P	N_("error, invalid parameter.")
#define ERR_MSG_PRM_IN_DEF	N_("error, a parameter cannot be used in a user function or a constant definition.")
#define ERR_MSG_UNKNOWN_F	N_("error, unknown function.")
#define ERR_MSG_MEMORY		N_("error, cannot allocate enough memory.")
#define ERR_MSG_UNKNOWN		N_("error, unknown error.")
#define ERR_MSG_POS_FUNC	N_("error, the function cannot be used in a user function or a constant definition.")
#define ERR_MSG_CONST_EXIST	N_("error, the constant is already defined.")
#define ERR_MSG_CALCULATION	N_("error, calculation error.")

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
  buf = g_malloc(len + 1);
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
  char *code_buf = NULL, *buf = NULL, *ptr;

  switch (err) {
  case MATH_ERROR_NONE:
    break;
  case MATH_ERROR_EOEQ:
    buf = g_strdup(_(ERR_MSG_EOEQ));
    break;
  case MATH_ERROR_FDEF_NEST:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s\n  the error is found at: %s"), _(ERR_MSG_FUNC_NEST), code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_FUNC_NEST));
    }
    break;
  case MATH_ERROR_UNEXP_OPE:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s\n  the error is found at: %s"), _(ERR_MSG_UNEXP_OP), code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_UNEXP_OP));
    }
    break;
  case MATH_ERROR_ARG_NUM:
    if (eq->err_info.func.fprm) {
      buf = g_strdup_printf("%s (%d for %d) '%s()'", _(ERR_MSG_ARG_NUM),
			    eq->err_info.func.arg_num,
			    eq->err_info.func.fprm->argc,
			    eq->err_info.func.fprm->name);
    } else {
      buf = g_strdup(_(ERR_MSG_ARG_NUM));
    }
    break;
  case MATH_ERROR_MISS_RP:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s\n  the error is found at: %s"), _(ERR_MSG_MISS_RP), code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_MISS_RP));
    }
    break;
  case MATH_ERROR_MISS_RB:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s\n  the error is found at: %s"), _(ERR_MSG_MISS_RB), code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_MISS_RB));
    }
    break;
  case MATH_ERROR_MISS_RC:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s\n  the error is found at: %s"), _(ERR_MSG_MISS_RC), code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_MISS_RC));
    }
    break;
  case MATH_ERROR_UNKNOWN_FUNC:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s\n  the error is found at: %s"), _(ERR_MSG_UNKNOWN_F), code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_UNKNOWN_F));
    }
    break;
  case MATH_ERROR_INVALID_FDEF:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s\n  the error is found at: %s"), _(ERR_MSG_INVALID_F), code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_INVALID_F));
    }
    break;
  case MATH_ERROR_INVALID_CDEF:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s\n  the error is found at: %s"), _(ERR_MSG_INVALID_C), code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_INVALID_C));
    }
    break;
  case MATH_ERROR_UNEXP_TOKEN:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s\n  the error is found at: %s"), _(ERR_MSG_UNEXP_TOK), code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_UNEXP_TOK));
    }
    break;
  case MATH_ERROR_INVALID_PRM:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s\n  the error is found at: %s"), _(ERR_MSG_INVALID_P), code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_INVALID_P));
    }
    break;
  case MATH_ERROR_PRM_IN_DEF:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s\n  the error is found at: %s"), _(ERR_MSG_PRM_IN_DEF), code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_PRM_IN_DEF));
    }
    break;
  case MATH_ERROR_MEMORY:
    buf = g_strdup(_(ERR_MSG_MEMORY));
    break;
  case MATH_ERROR_UNKNOWN:
    buf = g_strdup(_(ERR_MSG_UNKNOWN));
    break;
  case MATH_ERROR_INVALID_FUNC:
    if (eq->err_info.func.fprm) {
      buf = g_strdup_printf("%s '%s()'", _(ERR_MSG_POS_FUNC), eq->err_info.func.fprm->name);
    } else {
      buf = g_strdup(_(ERR_MSG_POS_FUNC));
    }
    break;
  case MATH_ERROR_CONST_EXIST:
    ptr = math_equation_get_const_name(eq, eq->err_info.const_id);
    if (ptr) {
      buf = g_strdup_printf("%s '%s'", _(ERR_MSG_CONST_EXIST), ptr);
    } else {
      buf = g_strdup(_(ERR_MSG_CONST_EXIST));
    }
    break;
  case MATH_ERROR_CALCULATION:
    buf = g_strdup(_(ERR_MSG_CALCULATION));
    break;
  default:
    buf = g_strdup(_(ERR_MSG_UNKNOWN));
  }

  if (code_buf)
    g_free(code_buf);

  if (buf) {
    gsize len;
    ptr = g_locale_from_utf8(buf, -1, NULL, &len, NULL);
    g_free(buf);
    buf = ptr;
  }

  return buf;
}

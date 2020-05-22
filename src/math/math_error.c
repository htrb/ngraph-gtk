/*
 * $Id: math_error.c,v 1.10 2009-12-22 00:57:42 hito Exp $
 *
 */

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
#define ERR_MSG_INVALID_A	N_("syntax error, invalid argument.")
#define ERR_MSG_INVALID_C	N_("syntax error, constant cannot be defined in a function definition.")
#define ERR_MSG_INVALID_P	N_("error, invalid parameter.")
#define ERR_MSG_PRM_IN_DEF	N_("error, a parameter cannot be used in a user function or a constant definition.")
#define ERR_MSG_UNKNOWN_F	N_("error, unknown function.")
#define ERR_MSG_MEMORY		N_("error, cannot allocate enough memory.")
#define ERR_MSG_UNKNOWN		N_("error, unknown error.")
#define ERR_MSG_POS_FUNC	N_("error, the function cannot be used in a user function or a constant definition.")
#define ERR_MSG_CONST_EXIST	N_("error, the constant is already defined.")
#define ERR_MSG_CALCULATION	N_("error, calculation error.")
#define ERR_MSG_UNTERMINATED_STRING	N_("syntax error, unterminated string.")

  static int ErrorLine = 1, ErrorOfst = 1;

static char *
check_error_position(MathEquation *eq, const char *code)
{
  int i, l, len;
  char *buf;

  if (code == NULL)
    return NULL;

  if (eq->err_info.pos.pos == NULL) {
    return NULL;
  }

  len = strlen(code);
  l = eq->err_info.pos.pos - code;

  if (l < 0)
    return NULL;

  if (l >= len)
    return NULL;

  if (eq->err_info.pos.pos[0] == ';' || eq->err_info.pos.pos[0] == '=' || eq->err_info.pos.pos[0] == '\0')
    return NULL;

  len -= l;
  buf = g_malloc(len + 1);
  if (buf == NULL)
    return NULL;

  for (i = 0; i < len; i++) {
    if (eq->err_info.pos.pos[i] == ';' || eq->err_info.pos.pos[i] == '=')
      break;
    buf[i] = eq->err_info.pos.pos[i];
  }
  buf[i] = '\0';

  ErrorLine = eq->err_info.pos.line;
  ErrorOfst = eq->err_info.pos.ofst;
  return buf;
}

char *
math_err_get_error_message(MathEquation *eq, const char *code, int err)
{
  char *code_buf = NULL, *buf = NULL, *ptr;

  ErrorLine = 1;
  switch (err) {
  case MATH_ERROR_NONE:
    break;
  case MATH_ERROR_EOEQ:
    buf = g_strdup(_(ERR_MSG_EOEQ));
    break;
  case MATH_ERROR_FDEF_NEST:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s (%d:%d)\n  the error is found at: %s"),
                            _(ERR_MSG_FUNC_NEST),
                            eq->err_info.pos.line, eq->err_info.pos.ofst,
                            code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_FUNC_NEST));
    }
    break;
  case MATH_ERROR_UNEXP_OPE:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s (%d:%d)\n  the error is found at: %s"),
                            _(ERR_MSG_UNEXP_OP),
                            eq->err_info.pos.line, eq->err_info.pos.ofst,
                            code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_UNEXP_OP));
    }
    break;
  case MATH_ERROR_ARG_NUM:
    code_buf = check_error_position(eq, code);
    if (eq->err_info.func.fprm) {
      buf = g_strdup_printf("%s (%d for %d) '%s()' (%d:%d)",
                            _(ERR_MSG_ARG_NUM),
			    eq->err_info.func.arg_num,
			    eq->err_info.func.fprm->argc,
			    eq->err_info.func.fprm->name,
                            eq->err_info.pos.line,
                            eq->err_info.pos.ofst);
    } else {
      buf = g_strdup(_(ERR_MSG_ARG_NUM));
    }
    break;
  case MATH_ERROR_MISS_RP:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s (%d:%d)\n  the error is found at: %s"),
                            _(ERR_MSG_MISS_RP),
                            eq->err_info.pos.line, eq->err_info.pos.ofst,
                            code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_MISS_RP));
    }
    break;
  case MATH_ERROR_MISS_RB:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s (%d:%d)\n  the error is found at: %s"),
                            _(ERR_MSG_MISS_RB),
                            eq->err_info.pos.line, eq->err_info.pos.ofst,
                            code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_MISS_RB));
    }
    break;
  case MATH_ERROR_MISS_RC:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s (%d:%d)\n  the error is found at: %s"),
                            _(ERR_MSG_MISS_RC),
                            eq->err_info.pos.line, eq->err_info.pos.ofst,
                            code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_MISS_RC));
    }
    break;
  case MATH_ERROR_UNKNOWN_FUNC:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s (%d:%d)\n  the error is found at: %s"),
                            _(ERR_MSG_UNKNOWN_F),
                            eq->err_info.pos.line, eq->err_info.pos.ofst,
                            code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_UNKNOWN_F));
    }
    break;
  case MATH_ERROR_INVALID_FDEF:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s (%d:%d)\n  the error is found at: %s"),
                            _(ERR_MSG_INVALID_F),
                            eq->err_info.pos.line, eq->err_info.pos.ofst,
                            code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_INVALID_F));
    }
    break;
  case MATH_ERROR_INVALID_ARG:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s (%d:%d)\n  the error is found at: %s"),
                            _(ERR_MSG_INVALID_A),
                            eq->err_info.pos.line, eq->err_info.pos.ofst,
                            code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_INVALID_A));
    }
    break;
  case MATH_ERROR_INVALID_CDEF:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s (%d:%d)\n  the error is found at: %s"),
                            _(ERR_MSG_INVALID_C),
                            eq->err_info.pos.line, eq->err_info.pos.ofst,
                            code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_INVALID_C));
    }
    break;
  case MATH_ERROR_UNEXP_TOKEN:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s (%d:%d)\n  the error is found at: %s"),
                            _(ERR_MSG_UNEXP_TOK),
                            eq->err_info.pos.line, eq->err_info.pos.ofst,
                            code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_UNEXP_TOK));
    }
    break;
  case MATH_ERROR_INVALID_PRM:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s (%d:%d)\n  the error is found at: %s"),
                            _(ERR_MSG_INVALID_P),
                            eq->err_info.pos.line, eq->err_info.pos.ofst,
                            code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_INVALID_P));
    }
    break;
  case MATH_ERROR_PRM_IN_DEF:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
      buf = g_strdup_printf(_("%s (%d:%d)\n  the error is found at: %s"),
                            _(ERR_MSG_PRM_IN_DEF),
                            eq->err_info.pos.line, eq->err_info.pos.ofst,
                            code_buf);
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
      buf = g_strdup_printf("%s '%s()' (%d:%d)",
                            _(ERR_MSG_POS_FUNC), eq->err_info.func.fprm->name,
                            eq->err_info.pos.line, eq->err_info.pos.ofst);
    } else {
      buf = g_strdup(_(ERR_MSG_POS_FUNC));
    }
    break;
  case MATH_ERROR_CONST_EXIST:
    ptr = math_equation_get_const_name(eq, eq->err_info.const_id);
    if (ptr) {
      buf = g_strdup_printf("%s '%s' (%d:%d)",
                            _(ERR_MSG_CONST_EXIST), ptr,
                            eq->err_info.pos.line, eq->err_info.pos.ofst);
    } else {
      buf = g_strdup(_(ERR_MSG_CONST_EXIST));
    }
    break;
  case MATH_ERROR_CALCULATION:
    buf = g_strdup(_(ERR_MSG_CALCULATION));
    break;
  case MATH_ERROR_UNTERMINATED_STRING:
    code_buf = check_error_position(eq, code);
    if (code_buf) {
    buf = g_strdup_printf("%s (%d:%d)\n  the error is found at: %s",
			  _(ERR_MSG_UNTERMINATED_STRING),
			  eq->err_info.pos.line,
			  eq->err_info.pos.ofst,
			  code_buf);
    } else {
      buf = g_strdup(_(ERR_MSG_UNTERMINATED_STRING));
    }
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

void
math_err_get_recent_error_position(int *line, int *ofst)
{
  if (line) {
    *line = ErrorLine;
  }
  if (ofst) {
    *ofst = ErrorOfst;
  }
}

/*
 * $Id: math_scanner.c,v 1.5 2009-11-16 09:13:06 hito Exp $
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <limits.h>
#include <glib.h>
#include <stdio.h>

#include "object.h"

#include "math_scanner.h"

static struct math_token *create_token(const char *str, enum MATH_TOKEN_TYPE type);
static struct math_token *get_array_prefix(const char *str, const char ** rstr);
static struct math_token *get_bracket(const char *str, const char ** rstr);
static struct math_token *get_unknown(const char *str, const char ** rstr);
static struct math_token *get_symbol(const char *str, const char ** rstr);
static struct math_token *get_string(const char *str, const char ** rstr, int quate);
static struct math_token *get_string_variable(const char *str, const char ** rstr);
static struct math_token *get_paren(const char *str, const char ** rstr);
static struct math_token *get_curly(const char *str, const char ** rstr);
static struct math_token *get_comma(const char *str, const char ** rstr);
static struct math_token *get_eoeq(const char *str, const char ** rstr);
static struct math_token *get_ope(const char *str, const char ** rstr, MathEquation *eq);
static struct math_token *get_num(const char *str, const char ** rstr);

struct reserved {
  char *name;
  enum MATH_TOKEN_TYPE type;
};

static struct reserved Reserved[] = {
  {"DEF", MATH_TOKEN_TYPE_DEF},
  {"CONST", MATH_TOKEN_TYPE_CONST},
};

static enum MATH_TOKEN_TYPE
check_reserved(char *str)
{
  unsigned int i;

  for (i = 0; i < sizeof(Reserved) / sizeof(*Reserved); i++) {
    if (strcmp(Reserved[i].name, str) == 0) {
      return Reserved[i].type;
    }
  }

  return MATH_TOKEN_TYPE_UNKNOWN;
}

void
math_scanner_free_token(struct math_token *token)
{
  if (token == NULL)
    return;

  switch (token->type) {
  case MATH_TOKEN_TYPE_NUMERIC:
  case MATH_TOKEN_TYPE_OPERATOR:
  case MATH_TOKEN_TYPE_COMMA:
  case MATH_TOKEN_TYPE_EOEQ:
  case MATH_TOKEN_TYPE_EOEQ_ASSIGN:
  case MATH_TOKEN_TYPE_LP:
  case MATH_TOKEN_TYPE_RP:
  case MATH_TOKEN_TYPE_LB:
  case MATH_TOKEN_TYPE_RB:
  case MATH_TOKEN_TYPE_LC:
  case MATH_TOKEN_TYPE_RC:
  case MATH_TOKEN_TYPE_DEF:
  case MATH_TOKEN_TYPE_CONST:
  case MATH_TOKEN_TYPE_ARRAY_PREFIX:
  case MATH_TOKEN_TYPE_UNKNOWN:
  case MATH_TOKEN_TYPE_UNTERMINATED_STRING:
    break;
  case MATH_TOKEN_TYPE_SYMBOL:
  case MATH_TOKEN_TYPE_STRING_VARIABLE:
    g_free(token->data.sym);
    break;
  case MATH_TOKEN_TYPE_DOUBLE_QUOTED_STRING:
  case MATH_TOKEN_TYPE_SINGLE_QUOTED_STRING:
    g_string_free(token->data.str, TRUE);
    break;
  }

  g_free(token);
}

struct math_token *
math_scanner_dup_token(struct math_token *token)
{
  struct math_token *new_token;
  if (token == NULL) {
    return NULL;
  }

  new_token = g_malloc0(sizeof(*new_token));
  if (new_token == NULL) {
    return NULL;
  }
  *new_token = *token;
  switch (token->type) {
  case MATH_TOKEN_TYPE_NUMERIC:
  case MATH_TOKEN_TYPE_OPERATOR:
  case MATH_TOKEN_TYPE_COMMA:
  case MATH_TOKEN_TYPE_EOEQ:
  case MATH_TOKEN_TYPE_EOEQ_ASSIGN:
  case MATH_TOKEN_TYPE_LP:
  case MATH_TOKEN_TYPE_RP:
  case MATH_TOKEN_TYPE_LB:
  case MATH_TOKEN_TYPE_RB:
  case MATH_TOKEN_TYPE_LC:
  case MATH_TOKEN_TYPE_RC:
  case MATH_TOKEN_TYPE_DEF:
  case MATH_TOKEN_TYPE_CONST:
  case MATH_TOKEN_TYPE_ARRAY_PREFIX:
  case MATH_TOKEN_TYPE_UNKNOWN:
  case MATH_TOKEN_TYPE_UNTERMINATED_STRING:
    break;
  case MATH_TOKEN_TYPE_SYMBOL:
  case MATH_TOKEN_TYPE_STRING_VARIABLE:
    new_token->data.sym = g_strdup(token->data.sym);
    if (new_token->data.sym == NULL) {
      g_free(new_token);
      new_token = NULL;
    }
    break;
  case MATH_TOKEN_TYPE_DOUBLE_QUOTED_STRING:
  case MATH_TOKEN_TYPE_SINGLE_QUOTED_STRING:
    new_token->data.str = g_string_new(token->data.str->str);
    if (new_token->data.str == NULL) {
      g_free(new_token);
      new_token = NULL;
    }
    break;
  }
  return new_token;
}

struct math_token *
math_scanner_get_token(struct math_string *mstr, MathEquation *eq)
{
  char c;
  const char *str;
  const char **rstr;
  struct math_token *token;

  str = mstr->cur;
  rstr = &(mstr->cur);
  if (str == NULL) {
    return NULL;
  }

  while (*str < 0 || *str == ' ' || *str == '\t' || *str == '\n') {
    mstr->ofst++;
    if (*str == '\n') {
      mstr->line++;
      mstr->ofst = 0;
    }
    str++;
  }

  c = str[0];
  if (c == '\0') {
    token = get_eoeq(str, rstr);
  } else  if (isdigit(c) || c == '.') {
    token = get_num(str, rstr);
  } else if (math_scanner_is_ope(c)) {
    token = get_ope(str, rstr, eq);
  } else if (isalpha(c) || c == '%' || c == '_') {
    token = get_symbol(str, rstr);
  } else if (c == '(' || c == ')') {
    token = get_paren(str, rstr);
  } else if (c == '[' || c == ']') {
    token = get_bracket(str, rstr);
  } else if (c == '{' || c == '}') {
    token = get_curly(str, rstr);
  } else if (c == ',') {
    token = get_comma(str, rstr);
  } else if (c == '@') {
    token = get_array_prefix(str, rstr);
  } else if (c == '"') {
    token = get_string(str, rstr, c);
  } else if (c == '\'') {
    token = get_string(str, rstr, c);
  } else if (c == '$') {
    token = get_string_variable(str, rstr);
  } else if (c == '#') {        /* comment */
    while (*str != '\0' && *str != '\n') {
      mstr->ofst++;
      str++;
    }
    mstr->cur = str;
    token = math_scanner_get_token(mstr, eq);
  } else {
    token = get_unknown(str, rstr);
  }
  mstr->ofst += (*rstr - str);
  return token;
}

static struct math_token *
get_unknown(const char *str,  const char ** rstr)
{
  struct math_token *tok;

  tok = create_token(str, MATH_TOKEN_TYPE_UNKNOWN);
  if (tok == NULL)
    return NULL;

  return tok;
}

static struct math_token *
get_paren(const char *str, const char ** rstr)
{
  struct math_token *tok;

  tok = create_token(str, (str[0] == ')') ? MATH_TOKEN_TYPE_RP : MATH_TOKEN_TYPE_LP);
  if (tok == NULL)
    return NULL;

  *rstr = str + 1;

  return tok;
}

static struct math_token *
get_bracket(const char *str,  const char ** rstr)
{
  struct math_token *tok;

  tok = create_token(str, (str[0] == ']') ? MATH_TOKEN_TYPE_RB : MATH_TOKEN_TYPE_LB);
  if (tok == NULL)
    return NULL;

  *rstr = str + 1;

  return tok;
}

static struct math_token *
get_curly(const char *str,  const char ** rstr)
{
  struct math_token *tok;

  tok = create_token(str, (str[0] == '}') ? MATH_TOKEN_TYPE_RC : MATH_TOKEN_TYPE_LC);
  if (tok == NULL)
    return NULL;

  *rstr = str + 1;

  return tok;
}

static struct math_token *
get_comma(const char *str,  const char ** rstr)
{
  struct math_token *tok;

  tok = create_token(str, MATH_TOKEN_TYPE_COMMA);
  if (tok == NULL)
    return NULL;

  *rstr = str + 1;
  return tok;
}

static struct math_token *
get_array_prefix(const char *str,  const char ** rstr)
{
  struct math_token *tok;

  tok = create_token(str, MATH_TOKEN_TYPE_ARRAY_PREFIX);
  if (tok == NULL)
    return NULL;

  *rstr = str + 1;
  return tok;
}

static struct math_token *
get_eoeq(const char *str,  const char ** rstr)
{
  struct math_token *tok;

  tok = create_token(str, MATH_TOKEN_TYPE_EOEQ);
  if (tok == NULL)
    return NULL;

  *rstr = str + ((*str) ? 1 : 0);
  return tok;
}

static struct math_token *
get_ope(const char *str,  const char ** rstr, MathEquation *eq)
{
  struct math_token *tok;
  int len;
  enum MATH_OPERATOR_TYPE ope;

  ope = math_scanner_check_ope_str(str, &len);
  switch (ope) {
  case MATH_OPERATOR_TYPE_EOEQ:
    tok = create_token(str, MATH_TOKEN_TYPE_EOEQ);
    break;
  case MATH_OPERATOR_TYPE_EOEQ_ASSIGN:
    switch (eq->eoeq_assign_type) {
    case EOEQ_ASSIGN_TYPE_BOTH:
      tok = create_token(str, MATH_TOKEN_TYPE_EOEQ_ASSIGN);
      if (tok) {
	tok->data.op = MATH_OPERATOR_TYPE_ASSIGN;
      }
      break;
    case EOEQ_ASSIGN_TYPE_EOEQ:
      tok = create_token(str, MATH_TOKEN_TYPE_EOEQ);
      break;
    case EOEQ_ASSIGN_TYPE_ASSIGN:
      tok = create_token(str, MATH_TOKEN_TYPE_OPERATOR);
      if (tok) {
	tok->data.op = MATH_OPERATOR_TYPE_ASSIGN;
      }
      break;
    }
    break;
  case MATH_OPERATOR_TYPE_UNKNOWN:
    tok = create_token(str, MATH_TOKEN_TYPE_UNKNOWN);
    break;
  default:
    tok = create_token(str, MATH_TOKEN_TYPE_OPERATOR);
    if (tok) {
      tok->data.op = ope;
    }
  }

  *rstr = str + len;

  return tok;
}

static char *
get_symbol_string(const char *str, char prefix, int *len)
{
  char *buf;
  int n, i;

  if (str[0] == prefix) {
    n = 1;
  } else {
    n = 0;
  }
  for (; isalnum(str[n]) || str[n] == '_'; n++);

  buf = g_malloc(n + 1);
  if (buf == NULL)
    return NULL;

  for (i = 0; i < n; i++) {
    buf[i] = toupper(str[i]);
  }
  buf[i] = '\0';
  *len = n;
  return buf;
}

static struct math_token *
get_symbol(const char *str, const char ** rstr)
{
  struct math_token *tok;
  char *buf;
  int n;
  enum MATH_TOKEN_TYPE type;

  buf = get_symbol_string(str, '%', &n);
  type = check_reserved(buf);
  if (type != MATH_TOKEN_TYPE_UNKNOWN) {
    g_free(buf);
    tok = create_token(str, type);
    if (tok == NULL) {
      return NULL;
    }
  } else {
    tok = create_token(str, MATH_TOKEN_TYPE_SYMBOL);
    if (tok == NULL) {
      g_free(buf);
      return NULL;
    }
    tok->data.sym = buf;
  }

  *rstr = str + n;

  return tok;
}

static GString *
get_single_quoted_string(const char *str, int *len)
{
  int n, escape;
  GString *gstr;

  gstr = g_string_new("");
  if (gstr == NULL) {
    return NULL;
  }

  escape = FALSE;
  for (n = 1; str[n]; n++) {
    if (escape) {
      switch (str[n]) {
      case '\'':
	g_string_append_c(gstr, '\'');
	break;
      case '\\':
	g_string_append_c(gstr, '\\');
	break;
      default:
	g_string_append_c(gstr, '\\');
	g_string_append_c(gstr, str[n]);
	break;
      }
      escape = FALSE;
      continue;
    }
    if (str[n] == '\'' || str[n] == '\n') {
      break;
    } else if (str[n] == '\\') {
      escape = TRUE;
    } else {
      g_string_append_c(gstr, str[n]);
    }
  }

  if (len) {
    * len = n;
  }
  return gstr;
}

static int
append_hex(GString *gstr, const char *str)
{
  int c, i;
  c = 0;
  for (i = 0; i < 2; i++) {
    if (g_ascii_isxdigit(str[i])) {
      c <<= 4;
      c += g_ascii_xdigit_value(str[i]);
    } else {
      break;
    }
  }
  if (c) {
    g_string_append_c(gstr, c);
  }
  return i;
}

static int
append_oct(GString *gstr, const char *str)
{
  int c, i;
  c = 0;
  for (i = 0; i < 3; i++) {
    if ('0' <= str[i] && str[i] <= '7') {
      c <<= 3;
      c += str[i] - '0';
    } else {
      break;
    }
  }
  if (c) {
    g_string_append_c(gstr, c);
  }
  return i;
}
static GString *
get_double_quoted_string(const char *str, int *len)
{
  int n, escape, chr;
  GString *gstr;

  gstr = g_string_new("");
  if (gstr == NULL) {
    return NULL;
  }

  escape = FALSE;
  for (n = 1; str[n]; n++) {
    if (escape) {
      switch (str[n]) {
      case 'a':
	g_string_append_c(gstr, '\a');
	break;
      case 'b':
	g_string_append_c(gstr, '\b');
	break;
      case 'f':
	g_string_append_c(gstr, '\f');
	break;
      case 'n':
	g_string_append_c(gstr, '\n');
	break;
      case 'r':
	g_string_append_c(gstr, '\r');
	break;
      case 't':
	g_string_append_c(gstr, '\t');
	break;
      case 'v':
	g_string_append_c(gstr, '\v');
	break;
      case 'x':
        n += append_hex(gstr, str + n + 1);
	break;
      case '0': case '1': case '2': case '3':
      case '4': case '5': case '6': case '7':
        n += append_oct(gstr, str + n) - 1;
	break;
      default:
	g_string_append_c(gstr, str[n]);
	break;
      }
      escape = FALSE;
      continue;
    }
    if (str[n] == '"' || str[n] == '\n') {
      break;
    } else if (str[n] == '\\') {
      escape = TRUE;
    } else {
      if (str[n] == '#' && str[n + 1] == '{') {
	chr = MATH_VARIABLE_EXPAND_PREFIX;
      } else {
	chr = str[n];
      }
      g_string_append_c(gstr, chr);
    }
  }

  if (len) {
    * len = n;
  }
  return gstr;
}

static struct math_token *
get_string(const char *str,  const char ** rstr, int quate)
{
  struct math_token *tok;
  enum MATH_TOKEN_TYPE type;
  int n;
  GString *gstr;

  if (quate == '"') {
    gstr = get_double_quoted_string(str, &n);
    type = MATH_TOKEN_TYPE_DOUBLE_QUOTED_STRING;
  } else {
    gstr = get_single_quoted_string(str, &n);
    type = MATH_TOKEN_TYPE_SINGLE_QUOTED_STRING;
  }
  if (gstr == NULL) {
    return NULL;
  }

  if (str[n] == '\0' || str[n] == '\n') {
    tok = create_token(str, MATH_TOKEN_TYPE_UNTERMINATED_STRING);
    if (tok == NULL) {
      return NULL;
    }
    return tok;
  }

  tok = create_token(str, type);
  if (tok == NULL) {
    g_string_free(gstr, TRUE);
    return NULL;
  }
  tok->data.str = gstr;

  n++;
  *rstr = str + n;
  return tok;
}

static struct math_token *
get_string_variable(const char *str, const char ** rstr)
{
  struct math_token *tok;
  char *buf;
  int n;
  buf = get_symbol_string(str, '$', &n);
  if (buf == NULL) {
    return NULL;
  }
  tok = create_token(str, MATH_TOKEN_TYPE_STRING_VARIABLE);
  if (tok == NULL) {
    g_free(buf);
    return NULL;
  }
  tok->data.sym = buf;
  *rstr = str + n;
  return tok;
}

static const char *
get_oct(const char *str, double *val)
{
  double oct;
  char c;

  oct = 0;
  for (; *str; str++) {
    c = *str;
    switch (c) {
    case ' ':
    case '_':
    case '\t':
      continue;
    case '0': case '1': case '2': case '3': case '4': case '5': case '6': case '7':
      oct *= 8;
      oct += c - '0';
      break;
    default:
      goto End;
    }
  }

 End:
  *val = oct;
  return str;
}

static const char *
get_hex(const char *str, double *val)
{
  double hex;
  char c;

  hex = 0;
  for (; *str; str++) {
    c = toupper(*str);
    switch (c) {
    case ' ':
    case '_':
    case '\t':
      continue;
    case '0': case '1': case '2': case '3': case '4':
    case '5': case '6': case '7': case '8': case '9':
      hex *= 16;
      hex += c - '0';
      break;
    case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
      hex *= 16;
      hex += c - 'A' + 10;
      break;
    default:
      goto End;
    }
  }

 End:
  *val = hex;
  return str;
}

static const char *
get_bin(const char *str, double *val)
{
  double bin;
  char c;

  bin = 0;
  for (; *str; str++) {
    c = str[0];

    switch (c) {
    case ' ':
    case '_':
    case '\t':
      continue;
    case '0':
    case '1':
      bin *= 2;
      bin += c - '0';
      break;
    default:
      goto End;
    }
  }

 End:
  *val = bin;
  return str;
}

static const char *
get_dec(const char *str, double *val)
{
  int in_decimal, in_pow, pow_sign, pow_val, dec_order;
  long double dec, flaction;

  pow_val = 0;
  pow_sign = 1;

  dec_order = 0;
  flaction = 0;
  dec = 0;

  in_decimal = 0;
  in_pow = 0;

  for (; *str; str++) {
    char c;

    c = toupper(*str);
    if (c < 0) {
      break;
    } else if (c == ' ' || c == '\t' || c == '_') {
      continue;
    } else if (c == '.') {
      if (in_decimal || in_pow) {
	break;
      }
      in_decimal = 1;
    } else if (c == 'E' || c == 'D') {
      if (in_pow) {
	break;
      }
      in_pow = 1;
    } else if (c == '+' || c =='-') {
      if (in_pow != 1) {
	break;
      }
      pow_sign = (c == '+') ? 1 : -1;
      in_pow = 2;
    } else if (isdigit(c)) {
      switch (in_pow) {
      case 1:
	in_pow = 2;
	/* fall through */
      case 2:
	if (pow_val < INT_MAX / 10) {
	  pow_val *= 10;
	  pow_val += c - '0';
	}
	break;
      default:
	if (in_decimal) {
	  if (flaction < 1E30) { /* dec and flaction is not negative */
	    dec_order++;
	    flaction *= 10;
	    flaction += c - '0';
	  }
	} else {
	  dec *= 10;
	  dec += c - '0';
	}
      }
    } else {
      break;
    }
  }

#if HAVE_POWL
  *val = dec * powl(10.0, pow_sign * pow_val) +
    flaction * powl(10.0, pow_sign * pow_val - dec_order);
#else  /* HAVE_POWL */
  *val = dec * pow(10.0, pow_sign * pow_val) +
    flaction * pow(10.0, pow_sign * pow_val - dec_order);
#endif	/* HAVE_POWL */

  return str;
}

static struct math_token *
get_num(const char *str,  const char ** rstr)
{
  struct math_token *tok;
  const char *ptr;
  double val;

  tok = create_token(str, MATH_TOKEN_TYPE_NUMERIC);
  if (tok == NULL) {
    return NULL;
  }

  if (str[0] == '0' && str[1] == 'x') {
    str += 2;
    ptr = get_hex(str, &val);
  } else if (str[0] == '0' && str[1] == 'b') {
    str += 2;
    ptr = get_bin(str, &val);
  } else if (str[0] == '0' && str[1] == 'o') {
    str += 2;
    ptr = get_oct(str, &val);
  } else if (str[0] == '0' && (str[1] == '.' || str[1] == 'E' || str[1] == 'D')) {
    ptr = get_dec(str, &val);
  } else {
    ptr = get_dec(str, &val);
  }

  if (ptr == NULL) {
    tok->type = MATH_TOKEN_TYPE_UNKNOWN;
    return tok;
  }

  tok->data.val.dnum = val;
  tok->data.val.type = MATH_SCANNER_VAL_TYPE_NORMAL;

  *rstr = ptr;

  return tok;
}

static struct math_token *
create_token(const char *str, enum MATH_TOKEN_TYPE type)
{
  struct math_token *tok;

  tok = g_malloc0(sizeof(*tok));
  if (tok == NULL) {
    return NULL;
  }

  tok->type = type;
  tok->ptr = str;

  return tok;
}

void
math_scanner_init_string(struct math_string *str, const char *line)
{
  str->top = line;
  str->cur = line;
  str->line = 0;
  str->ofst = 0;
}

void
replace_eoeq_token(char *eq)
{
  int i;
  if (eq == NULL) {
    return;
  }
  for (i = 0; eq[i]; i++) {
    const char *ptr;
    struct math_token *tok;
    switch (eq[i]) {
    case '=':
      if (eq[i + 1] == '=') {
	i++;
      } else {
	eq[i] = ';';
      }
      break;
    case '!':
    case '-':
    case '+':
    case '*':
    case '/':
    case '>':
    case '<':
    case ':':
    case '\\':
    case '^':
      if (eq[i + 1] == '=') {
	i++;
      }
      break;
    case '"':
    case '\'':
      tok = get_string(eq + i, &ptr, eq[i]);
      if (tok == NULL) {
	return;
      }
      i = ptr - eq - 1;
      math_scanner_free_token(tok);
      break;
    }
  }
}

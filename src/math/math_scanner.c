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

#include "object.h"

#include "math_scanner.h"

static struct math_token *create_token(const char *str, enum MATH_TOKEN_TYPE type);
static struct math_token *get_array_prefix(const char *str, const char ** rstr);
static struct math_token *get_bracket(const char *str, const char ** rstr);
static struct math_token *get_unknown(const char *str, const char ** rstr);
static struct math_token *get_symbol(const char *str, const char ** rstr);
static struct math_token *get_paren(const char *str, const char ** rstr);
static struct math_token *get_curly(const char *str, const char ** rstr);
static struct math_token *get_comma(const char *str, const char ** rstr);
static struct math_token *get_eoeq(const char *str, const char ** rstr);
static struct math_token *get_ope(const char *str, const char ** rstr);
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
    break;
  case MATH_TOKEN_TYPE_SYMBOL:
    g_free(token->data.sym);
    break;
  }

  g_free(token);
}

struct math_token *
math_scanner_get_token(struct math_string *mstr)
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
    token = get_ope(str, rstr);
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
  } else if (c == '#') {        /* comment */
    while (*str != '\0' && *str != '\n') {
      mstr->ofst++;
      if (*str == '\n') {
        mstr->line++;
        mstr->ofst = 0;
      }
      str++;
    }
    token = math_scanner_get_token(mstr);
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
get_ope(const char *str,  const char ** rstr)
{
  struct math_token *tok;
  int len;
  enum MATH_OPERATOR_TYPE ope;

  ope = math_scanner_check_ope_str(str, &len);
  switch (ope) {
  case MATH_OPERATOR_TYPE_EOEQ:
    tok = create_token(str, MATH_TOKEN_TYPE_EOEQ);
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

static struct math_token *
get_symbol(const char *str,  const char ** rstr)
{
  struct math_token *tok;
  char *buf;
  int n, i;
  enum MATH_TOKEN_TYPE type;

  for (n = (str[0] == '%') ? 1 : 0; isalnum(str[n]) || str[n] == '_'; n++);

  buf = g_malloc(n + 1);
  if (buf == NULL)
    return NULL;

  for (i = 0; i < n; i++) {
    buf[i] = toupper(str[i]);
  }
  buf[i] = '\0';

  type = check_reserved(buf);
  if (type != MATH_TOKEN_TYPE_UNKNOWN) {
    g_free(buf);
    tok = create_token(str, type);
    if (tok == NULL) {
      g_free(buf);
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

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "shell_arithmetic.h"
#include "shell.h"
#include "object.h"

static long long eval_comma(ShellArithmeticContext *ctx);
static long long eval_assign(ShellArithmeticContext *ctx);

#define STR(x) #x

static void
clear_var_name(char **var_name)
{
  if (var_name && *var_name) {
    g_free(*var_name);
    *var_name = NULL;
  }
}

static void
skip_whitespace(ShellArithmeticContext *ctx)
{
  while (ctx->src[0] == ' ' || ctx->src[0] == '\t') {
    ctx->src++;
  }
}

static void
skip_expr(ShellArithmeticContext *ctx)
{
  int old_skip = ctx->skip;
  ctx->skip = TRUE;
  eval_comma(ctx);
  ctx->skip = old_skip;
}

static void
update_shell_var(ShellArithmeticContext *ctx, char *var_name, long long val)
{
  if (ctx->skip) return;
  char *val_str = g_strdup_printf("%lld", val);
  addval(ctx->nshell, var_name, val_str);
  g_free(val_str);
}

static long long
get_shell_var(ShellArithmeticContext *ctx, char *var_name)
{
  if (ctx->skip) return 0;
  char *var_str = getval(ctx->nshell, var_name);
  return var_str ? strtoll(var_str, NULL, 0) : 0;
}

static long long
check_var_name(ShellArithmeticContext *ctx, char **pvar_name)
{
  const char *start = ctx->src;
  while (isalnum(ctx->src[0]) || ctx->src[0] == '_') {
    ctx->src++;
  }
  size_t len = ctx->src - start;

  char *var_name = g_malloc(len + 1);
  memcpy(var_name, start, len);
  var_name[len] = '\0';

  long long val = val = get_shell_var(ctx, var_name);

  if (pvar_name) {
    *pvar_name = var_name;
  } else {
    g_free (var_name);
  }

  return val;
}

// primary: numeric values, variable names, parentheses ( )
static long long
eval_primary(ShellArithmeticContext *ctx, char **pvar_name)
{
  char *endptr;

  if (pvar_name) *pvar_name = NULL;
  skip_whitespace(ctx);

  if (ctx->src[0] == '(') {
    ctx->src++;
    long long val = eval_comma(ctx);
    skip_whitespace(ctx);
    if (ctx->src[0] == ')') {
      ctx->src++;
    } else {
      printfstderr("sh: syntax error: rparen expected\n");
      ctx->err = TRUE;
    }
    return val;
  }

  if (isdigit(ctx->src[0])) {
    long long val = strtoll(ctx->src, &endptr, 0);
    ctx->src = endptr;
    return val;
  }

  if (isalpha(ctx->src[0]) || ctx->src[0] == '_') {
    return check_var_name(ctx, pvar_name);
  }
  if (ctx->src[0]) {
    printfstderr("sh: syntax error: expecting primary %s\n", ctx->src);
    ctx->err = TRUE;
  } else {
    printfstderr("sh: syntax error: operand expected\n");
    ctx->err = TRUE;
  }  return 0;
}

static int
check_operator(ShellArithmeticContext *ctx, const char *ope)
{
  int len = strlen(ope);
  if (strncmp(ctx->src, ope, len) == 0) {
    ctx->src += len;
    return TRUE;
  }
  return FALSE;
}

// postfix = primary [ "++" | "--" ]
static long long
eval_postfix(ShellArithmeticContext *ctx, char ** var_name)
{
  long long val = eval_primary(ctx, var_name);

  skip_whitespace(ctx);
  if (strncmp(ctx->src, "++", 2) == 0 && var_name && *var_name) {
    ctx->src += 2;
    long long old_val = val;
    update_shell_var(ctx, *var_name, val + 1);
    g_free(*var_name);
    *var_name = NULL;
    return old_val;
  } else if (strncmp(ctx->src, "--", 2) == 0 && var_name && *var_name) {
    ctx->src += 2;
    long long old_val = val;
    update_shell_var(ctx, *var_name, val - 1);
    g_free(*var_name);
    *var_name = NULL;
    return old_val;
  }
  return val;
}

// unary = [ "++" | "--" | "+" | "-" | "~" | "!" ] unary | postfix
static long long
eval_unary(ShellArithmeticContext *ctx, char ** var_name)
{
  skip_whitespace(ctx);

  if (check_operator(ctx, "++")) {
    char *local_var = NULL;
    long long val = eval_unary(ctx, &local_var);

    if (!local_var) {
      printfstderr("sh: lvalue required...\n");
      ctx->err = TRUE;
      return 0;
    }

    val++;
    update_shell_var(ctx, local_var, val);
    g_free(local_var);

    if (var_name) *var_name = NULL;

    return val;
  }

  if (check_operator(ctx, "--")) {
    char *local_var = NULL;
    long long val = eval_unary(ctx, &local_var);

    if (!local_var) {
      printfstderr("sh: lvalue required...\n");
      ctx->err = TRUE;
      return 0;
    }

    val--;
    update_shell_var(ctx, local_var, val);

    g_free(local_var);
    if (var_name) *var_name = NULL;

    return val;
  }

  if (ctx->src[0] == '-') {
    ctx->src++;
    clear_var_name(var_name);
    return -eval_unary(ctx, NULL);
  }
  if (ctx->src[0] == '~') {
    ctx->src++;
    clear_var_name(var_name);
    return ~eval_unary(ctx, NULL);
  }
  if (ctx->src[0] == '!') {
    ctx->src++;
    clear_var_name(var_name);
    return !eval_unary(ctx, NULL);
  }
  if (ctx->src[0] == '+') {
    ctx->src++;
    clear_var_name(var_name);
    return eval_unary(ctx, NULL);
  }

  return eval_postfix(ctx, var_name);
}

//   *. /, %
static long long
eval_mul(ShellArithmeticContext *ctx, char ** var_name)
{
  long long val = eval_unary(ctx, var_name);
  while (1) {
    if (ctx->err) break;
    skip_whitespace(ctx);

    if (ctx->src[0] == '*' && ctx->src[1] != '=') {
      ctx->src++;
      clear_var_name(var_name);
      val *= eval_unary(ctx, NULL);
    } else if (ctx->src[0] == '/' && ctx->src[1] != '=') {
      ctx->src++;
      clear_var_name(var_name);
      long long denom = eval_unary(ctx, NULL);
      if (denom == 0) {
        if (!ctx->skip) {
          printfstderr("sh: arithmetic exception: division by zero\n");
          ctx->err = TRUE;
        }
        return 0;
      }
      val /= denom;
    } else if (ctx->src[0] == '%' && ctx->src[1] != '=') {
      ctx->src++;
      clear_var_name(var_name);
      long long denom = eval_unary(ctx, NULL);
      if (denom == 0) {
        if (!ctx->skip) {
          printfstderr("sh: arithmetic exception: division by zero\n");
          ctx->err = TRUE;
        }
        return 0;
      }
      val %= denom;
    } else {
      break;
    }
  }
  return val;
}

// +, -
static long long
eval_add(ShellArithmeticContext *ctx, char ** var_name)
{
  long long val = eval_mul(ctx, var_name);
  while (1) {
    if (ctx->err) break;
    skip_whitespace(ctx);

    if (ctx->src[0] == '+' && ctx->src[1] != '=') {
      ctx->src++;
      clear_var_name(var_name);
      val += eval_mul(ctx, NULL);
    } else if (ctx->src[0] == '-' && ctx->src[1] != '=') {
      ctx->src++;
      clear_var_name(var_name);
      val -= eval_mul(ctx, NULL);
    } else {
      break;
    }
  }
  return val;
}

// shift = add ( "<<" add | ">>" add )*
static long long
eval_shift(ShellArithmeticContext *ctx, char ** var_name)
{
  long long val = eval_add(ctx, var_name);
  while (1) {
    skip_whitespace(ctx);
    if (strncmp(ctx->src, "<<" , 2) == 0 && ctx->src[2] != '=') {
      ctx->src += 2;
      clear_var_name(var_name);
      val = (val << eval_add(ctx, NULL));
    } else if (strncmp(ctx->src, ">>", 2) == 0 && ctx->src[2] != '=') {
      ctx->src += 2;
      clear_var_name(var_name);
      val = (val >> eval_add(ctx, NULL));
    } else {
      break;
    }
  }
  return val;
}

// relational = shift ( "<=" shift | ">=" shift | "<" shift | ">" shift )*
static long long
eval_relational(ShellArithmeticContext *ctx, char ** var_name)
{
  long long val = eval_shift(ctx, var_name);
  while (1) {
    skip_whitespace(ctx);
    if (check_operator(ctx, "<=")) {
      clear_var_name(var_name);
      val = (val <= eval_shift(ctx, NULL));
    } else if (check_operator(ctx, ">=")) {
      clear_var_name(var_name);
      val = (val >= eval_shift(ctx, NULL));
    } else if (ctx->src[0] == '<' && ctx->src[1] != '<') {
      ctx->src++;
      clear_var_name(var_name);
      val = (val < eval_shift(ctx, NULL));
    } else if (ctx->src[0] == '>' && ctx->src[1] != '>') {
      ctx->src++;
      clear_var_name(var_name);
      val = (val > eval_shift(ctx, NULL));
    } else {
      break;
    }
  }
  return val;
}

// equality = relational ( "==" relational | "!=" relational )*
static long long
eval_equality(ShellArithmeticContext *ctx, char **var_name)
{
  long long val = eval_relational(ctx, var_name);
  while (1) {
    skip_whitespace(ctx);
    if (check_operator(ctx, "==")) {
      clear_var_name(var_name);
      val = (val == eval_relational(ctx, NULL));
    } else if (check_operator(ctx, "!=")) {
      clear_var_name(var_name);
      val = (val != eval_relational(ctx, NULL));
    } else {
      break;
    }
  }
  return val;
}

// bit_and = equality ( "&" equality )*
static long long
eval_bit_and(ShellArithmeticContext *ctx, char **var_name)
{
  long long val = eval_equality(ctx, var_name);
  while (1) {
    skip_whitespace(ctx);
    if (ctx->src[0] == '&' && ctx->src[1] != '&' && ctx->src[1] != '=') {
      ctx->src++;
      clear_var_name(var_name);
      val = val & eval_equality(ctx, NULL);
    } else {
      break;
    }
  }
  return val;
}

// bit_xor = bit_and ( "^" bit_and )*
static long long
eval_bit_xor(ShellArithmeticContext *ctx, char **var_name)
{
  long long val = eval_bit_and(ctx, var_name);
  while (1) {
    skip_whitespace(ctx);
    if (ctx->src[0] == '^' && ctx->src[1] != '=') {
      ctx->src++;
      clear_var_name(var_name);
      val = val ^ eval_bit_and(ctx, NULL);
    } else {
      break;
    }
  }
  return val;
}

// bit_or = bit_xor ( "|" bit_xor )*
static long long
eval_bit_or(ShellArithmeticContext *ctx, char **var_name)
{
  long long val = eval_bit_xor(ctx, var_name);
  while (1) {
    skip_whitespace(ctx);
    if (ctx->src[0] == '|' && ctx->src[1] != '|' && ctx->src[1] != '=') {
      ctx->src++;
      clear_var_name(var_name);
      val = val | eval_bit_xor(ctx, NULL);
    } else {
      break;
    }
  }
  return val;
}

// and = bit_or ( "&&" bit_or )*
static long long
eval_logical_and(ShellArithmeticContext *ctx, char **var_name)
{
  long long val = eval_bit_or(ctx, var_name);
  while (1) {
    skip_whitespace(ctx);
    if (check_operator(ctx, "&&")) {
      clear_var_name(var_name);

      if (val == 0) {
        skip_expr(ctx);
        val = 0;
      } else {
        long long rhs_val = eval_bit_or(ctx, NULL);
        val = (rhs_val != 0) ? 1 : 0;
      }
    } else {
      break;
    }
  }
  return val;
}

// or = and ( "||" and )*
static long long
eval_logical_or(ShellArithmeticContext *ctx, char **var_name)
{
  long long val = eval_logical_and(ctx, var_name);
  while (1) {
    skip_whitespace(ctx);
    if (check_operator(ctx, "||")) {
      clear_var_name(var_name);

      if (val) {
        skip_expr(ctx);
        val = 1;
      } else {
        long long rhs_val = eval_logical_and(ctx, NULL);
        val = (rhs_val != 0) ? 1 : 0;
      }
    } else {
      break;
    }
  }
  return val;
}

// conditional = logical_or [ "?" assign ":" conditional ]
static long long
eval_conditional(ShellArithmeticContext *ctx, char **var_name)
{
  long long condition = eval_logical_or(ctx, var_name);
  skip_whitespace(ctx);
  if (ctx->src[0] == '?') {
    int old_skip = ctx->skip;

    ctx->src++;
    clear_var_name(var_name);
    if (condition) {
      long long true_val = eval_comma(ctx);
      if (ctx->src[0] != ':') {
	printfstderr("syntax error\n");
	ctx->err = TRUE;
	return 0;
      }
      ctx->src++;
      ctx->skip = TRUE;
      eval_assign(ctx);
      ctx->skip = old_skip;
      return true_val;
    } else {
      ctx->skip = TRUE;
      eval_comma(ctx);
      ctx->skip = old_skip;
      skip_whitespace(ctx);
      if (ctx->src[0] != ':') {
	printfstderr("syntax error\n");
	ctx->err = TRUE;
	return 0;
      }
      ctx->src++;
      return eval_conditional(ctx, NULL);
    }
  }
  return condition;
}

#define EVAL_ASSIN_OPERATOR(ctx, ope, var_name, lhs_val, check_zero)	\
  do {									\
    if (strncmp(ctx->src, STR(ope) "=", sizeof(STR(ope))) == 0) {	\
      ctx->src += sizeof(STR(ope));					\
      if (!var_name) {							\
        printfstderr("sh: lvalue required...\n");			\
        ctx->err = TRUE;						\
        return 0;							\
      }									\
      long long rhs_val = eval_assign(ctx);				\
      if (check_zero && rhs_val == 0) {					\
        if (!ctx->skip) {						\
          printfstderr("sh: arithmetic exception: division by zero\n"); \
          ctx->err = TRUE;						\
        }								\
        return 0;							\
      }									\
      long long new_val = lhs_val ope rhs_val;				\
      update_shell_var(ctx, var_name, new_val);				\
      g_free(var_name);							\
      return new_val;							\
    }									\
  } while(0)

// assign = conditional [ ( "=" | "+=" | "-=" ) assign ]
static long long
eval_assign(ShellArithmeticContext *ctx)
{
  char *var_name = NULL;
  long long lhs_val = eval_conditional(ctx, &var_name);
  skip_whitespace(ctx);
  if (ctx->src[0] == '=' && ctx->src[1] != '=') {
    ctx->src++;
    if (!var_name) {
      printfstderr("sh: lvalue required...\n");
      ctx->err = TRUE;
      return 0;
    }
    long long rhs_val = eval_assign(ctx);
    update_shell_var(ctx, var_name, rhs_val);
    g_free(var_name);
    return rhs_val;
  }
  EVAL_ASSIN_OPERATOR(ctx, +, var_name, lhs_val, FALSE);
  EVAL_ASSIN_OPERATOR(ctx, -, var_name, lhs_val, FALSE);
  EVAL_ASSIN_OPERATOR(ctx, *, var_name, lhs_val, FALSE);
  EVAL_ASSIN_OPERATOR(ctx, /, var_name, lhs_val, TRUE);
  EVAL_ASSIN_OPERATOR(ctx, %, var_name, lhs_val, TRUE);
  EVAL_ASSIN_OPERATOR(ctx, &, var_name, lhs_val, FALSE);
  EVAL_ASSIN_OPERATOR(ctx, |, var_name, lhs_val, FALSE);
  EVAL_ASSIN_OPERATOR(ctx, ^, var_name, lhs_val, FALSE);
  EVAL_ASSIN_OPERATOR(ctx, <<, var_name, lhs_val, FALSE);
  EVAL_ASSIN_OPERATOR(ctx, >>, var_name, lhs_val, FALSE);

  if (var_name) g_free(var_name);
  return lhs_val;
}

// eval_comma (comma = assign ( "," assign )*)
static long long
eval_comma(ShellArithmeticContext *ctx)
{
  long long val = eval_assign(ctx);

  skip_whitespace(ctx);
  while (ctx->src[0] == ',') {
    ctx->src++;
    val = eval_assign(ctx);
    skip_whitespace(ctx);
  }
  return val;
}

// entry point
long long
shell_arithmetic_eval(ShellArithmeticContext *ctx) {
  ctx->err = FALSE;
  ctx->skip = FALSE;
  long long val = eval_comma(ctx);

  skip_whitespace(ctx);
  if (*ctx->src != '\0' && !ctx->err) {
    printfstderr("sh: syntax error: unexpected extra token '%s'\n", ctx->src);
    ctx->err = TRUE;
  }
  return ctx->err ? 0 : val;
}

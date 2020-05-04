/*
 * $Id: math_expression.c,v 1.16 2010-02-24 00:52:44 hito Exp $
 *
 */

#include "config.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <limits.h>
#include <ctype.h>
#include <glib.h>

#include "nhash.h"
#include "odata.h"

#include "math_scanner.h"
#include "math_parser.h"
#include "math_expression.h"
#include "math_function.h"

static int calc(MathExpression *exp, MathValue *val);
static MathExpression *optimize(MathExpression *exp, int *err);

#define CALC_EXPRESSION(e, v)				\
  ((e->type == MATH_EXPRESSION_TYPE_DOUBLE) ?		\
   ((v = e->u.value), 0) :				\
   calc(e, &v))

MathExpression *
math_expression_new(enum MATH_EXPRESSION_TYPE type, MathEquation *eq, int *err)
{
  MathExpression *exp;

  exp = g_malloc(sizeof(*exp));
  if (exp == NULL) {
    if (err) {
      *err =MATH_ERROR_MEMORY;
    }
    return NULL;
  }

  exp->type = type;
  exp->next = NULL;
  exp->equation = eq;

  return exp;
}

MathExpression *
math_constant_definition_expression_new(MathEquation *eq, char *name, MathExpression *exp, int *err)
{
  MathExpression *cdef;
  int id;

  id = math_equation_get_const_by_name(eq, name, NULL);
  if (id >= 0) {
    /* already defined */
    *err = MATH_ERROR_CONST_EXIST;
    math_equation_set_const_error(eq, id);
    return NULL;
  }

  cdef = math_expression_new(MATH_EXPRESSION_TYPE_CONST_DEF, eq, err);
  if (cdef == NULL) {
    return NULL;
  }

  cdef->u.const_def.operand = exp;

  if (math_equation_add_const_definition(eq, name, cdef, err) < 0) {
    g_free(cdef);
    return NULL;
  }

  return cdef;
}

MathExpression *
math_function_expression_new(MathEquation *eq, const char *name, int *err)
{
  MathExpression *func;
  struct math_function_parameter *prm;

  prm = math_equation_get_func(eq, name);
  if (prm && prm->base_usr == NULL) {
    /* embedded function */
    return NULL;
  }

  func = math_expression_new(MATH_EXPRESSION_TYPE_FUNC, eq, err);
  if (func == NULL)
    return NULL;

  func->u.func.argc = 0;
  func->u.func.local_num = 0;
  func->u.func.exp = NULL;
  func->u.func.arg_list = NULL;

  func->u.func.fprm = math_equation_start_user_func_definition(eq, name);
  if (func->u.func.fprm == NULL)
    return NULL;

  return func;
}

static struct math_func_arg_list *
create_arg(const char *name, enum MATH_FUNCTION_ARG_TYPE type)
{
  struct math_func_arg_list *ptr;

  ptr = g_malloc(sizeof(*ptr));
  if (ptr == NULL) {
    return NULL;
  }

  ptr->name = g_strdup(name);
  if (ptr->name == NULL) {
    g_free(ptr);
    return NULL;
  }

  ptr->next = NULL;
  ptr->type = type;

  return ptr;
}

static int
register_arg(MathExpression *func, const char *arg_name, enum MATH_FUNCTION_ARG_TYPE type)
{
  switch (type) {
  case MATH_FUNCTION_ARG_TYPE_DOUBLE:
  case MATH_FUNCTION_ARG_TYPE_VARIABLE:
    if (math_equation_check_var(func->equation, arg_name) >= 0) {
      /* the variable already exist */
      return 1;
    }
    math_equation_add_var(func->equation, arg_name);
    break;
  case MATH_FUNCTION_ARG_TYPE_ARRAY:
    if (math_equation_check_array(func->equation, arg_name) >= 0) {
      /* the array already exist */
      return 1;
    }
    math_equation_add_array(func->equation, arg_name, TRUE);
    break;
  case MATH_FUNCTION_ARG_TYPE_STRING_ARRAY:
    if (math_equation_check_string_array(func->equation, arg_name) >= 0) {
      /* the array already exist */
      return 1;
    }
    math_equation_add_array(func->equation, arg_name, FALSE);
    break;
  case MATH_FUNCTION_ARG_TYPE_STRING:
  case MATH_FUNCTION_ARG_TYPE_STRING_VARIABLE:
    if (math_equation_check_string_var(func->equation, arg_name) >= 0) {
      /* the string variable already exists */
      return 1;
    }
    math_equation_add_var_string(func->equation, arg_name);
    break;
  case MATH_FUNCTION_ARG_TYPE_PROC:
    return 1;
  case MATH_FUNCTION_ARG_TYPE_VARIABLE_COMMON:
  case MATH_FUNCTION_ARG_TYPE_ARRAY_COMMON:
    /* thie function is called only at the user function definition. */
    /* never reached */
    return 1;
  }

  return 0;
}

int
math_function_expression_add_arg(MathExpression *func, const char *arg_name, enum MATH_FUNCTION_ARG_TYPE type)
{
  static struct math_func_arg_list *list;

  list = create_arg(arg_name, type);
  if (list == NULL)
    return 1;

  if (func->u.func.arg_list == NULL) {
    func->u.func.arg_list = list;
  } else {
    func->u.func.arg_last->next = list;
  }

  func->u.func.arg_last = list;
  return 0;
}

static int
func_set_arg_buf(MathExpression *func)
{
  static struct math_func_arg_list *list;
  int n, i, save_arg_type;
  enum MATH_FUNCTION_ARG_TYPE *arg_type_buf;

  save_arg_type = 0;
  list = func->u.func.arg_list;
  for (n = 0; list; n++) {
    switch (list->type) {
    case MATH_FUNCTION_ARG_TYPE_DOUBLE:
      break;
    case MATH_FUNCTION_ARG_TYPE_VARIABLE:
    case MATH_FUNCTION_ARG_TYPE_STRING:
    case MATH_FUNCTION_ARG_TYPE_STRING_VARIABLE:
    case MATH_FUNCTION_ARG_TYPE_VARIABLE_COMMON:
    case MATH_FUNCTION_ARG_TYPE_ARRAY:
    case MATH_FUNCTION_ARG_TYPE_STRING_ARRAY:
    case MATH_FUNCTION_ARG_TYPE_ARRAY_COMMON:
      save_arg_type = 1;
      break;
    case MATH_FUNCTION_ARG_TYPE_PROC:
      return 1;
      break;
    }
    list = list->next;
  }

  if (save_arg_type) {
    arg_type_buf = g_malloc(sizeof(*arg_type_buf) * n);
    if (arg_type_buf == NULL)
      return 1;

    list = func->u.func.arg_list;
    for (i = 0; list; i++) {
      arg_type_buf[i] = list->type;
      if (register_arg(func, list->name, list->type)) {
	g_free(arg_type_buf);
	return 1;
      }

      list = list->next;
    }

    func->u.func.fprm->arg_type = arg_type_buf;
  } else {
    list = func->u.func.arg_list;
    for (i = 0; list; i++) {
      if (register_arg(func, list->name, list->type)) {
	return 1;
      }
      list = list->next;
    }
  }

  func->u.func.argc = n;
  func->u.func.fprm->argc = func->u.func.argc;

  return 0;
}

static void
free_arg_list(struct math_func_arg_list *list)
{
  static struct math_func_arg_list *next;

  while (list) {
    next = list->next;
    g_free(list->name);
    g_free(list);
    list = next;
  }
}

int
math_function_expression_register_arg(MathExpression *func)
{
  return func_set_arg_buf(func);
}

int
math_function_expression_set_function(MathEquation *eq, MathExpression *func, const char *name, MathExpression *exp)
{
  int anum, str_anum, vnum, snum;

  func->u.func.exp = exp;

  if (func->u.func.fprm == NULL) {
    return 1;
  }

  if (math_equation_register_user_func_definition(eq, name, func)) {
    return 1;
  }

  func->u.func.fprm->base_usr = func;
  func->u.func.fprm->opt_usr = NULL;
  math_equation_finish_user_func_definition(eq, &vnum, &anum, &str_anum, &snum);
  func->u.func.local_num = vnum;
  func->u.func.local_string_num = snum;
  func->u.func.local_array_num = anum;
  func->u.func.local_string_array_num = str_anum;

  return 0;
}

MathExpression *
math_parameter_expression_new(MathEquation *eq, char *name, int *err)
{
  int ofst, i, type, n, id, id_pre, id_post, index;
  MathExpression *exp;
  MathEquationParametar *prm;

  if (isalpha(name[1])) {
    type = name[1];
    ofst = 2;
  } else {
    type = 0;
    ofst = 1;
  }

  prm = math_equation_get_parameter(eq, type, err);
  if (prm == NULL) {
    *err = MATH_ERROR_INVALID_PRM;
    return NULL;
  }

  for (i = ofst; isdigit(name[i]); i++);

  if (name[i] != '\0' || i == ofst) {
    *err = MATH_ERROR_INVALID_PRM;
    return NULL;
  }

  n = i - ofst;

  if (n < prm->min_length || n > prm->max_length) {
    *err = MATH_ERROR_INVALID_PRM;
    return NULL;
  }

  id_pre = id_post = 0;
  for (i = 0; i < prm->min_length - 1; i++) {
    id_pre *= 10;
    id_pre += name[ofst + i] - '0';
  }
  for (; i < n; i++) {
    id_post *= 10;
    id_post += name[ofst + i] - '0';
  }
  for (i = 0; i < prm->max_length - prm->min_length + 1; i++) {
    id_pre *= 10;
  }

  id = id_pre + id_post;

  index = math_equation_use_parameter(eq, type, id);
  if (index < 0) {
    *err = MATH_ERROR_MEMORY;
    return NULL;
  }

  exp = math_expression_new(MATH_EXPRESSION_TYPE_PRM, eq, err);
  if (exp == NULL) {
    return NULL;
  }

  exp->u.prm.type = type;
  exp->u.prm.prm = prm;
  exp->u.prm.id = id;
  exp->u.prm.index = (prm->use_index == MATH_EQUATION_PARAMETAR_USE_INDEX) ? index : id;

  return exp;
}

MathExpression *
math_constant_expression_new(MathEquation *eq, const char *name, int *err)
{
  MathExpression *exp;
  MathValue val;
  int i;

  i = math_equation_get_const_by_name(eq, name, &val);
  if (i < 0) {
    return NULL;
  }

  exp = math_expression_new(MATH_EXPRESSION_TYPE_CONST, eq, err);
  if (exp == NULL) {
    return NULL;
  }

  exp->u.index = i;

  return exp;
}

MathExpression *
math_variable_expression_new(MathEquation *eq, const char *name, int *err)
{
  MathExpression *exp;
  int i;

  exp = math_expression_new(MATH_EXPRESSION_TYPE_VARIABLE, eq, err);
  if (exp == NULL) {
    return NULL;
  }

  i = math_equation_add_var(eq, name);
  if (i < 0) {
    *err = MATH_ERROR_MEMORY;
    math_expression_free(exp);
    return NULL;
  }

  exp->u.index = i;

  return exp;
}

MathExpression *
math_string_variable_expression_new(MathEquation *eq, const char *str, int *err)
{
  MathExpression *exp;
  int i;

  exp = math_expression_new(MATH_EXPRESSION_TYPE_STRING_VARIABLE, eq, err);
  if (exp == NULL) {
    return NULL;
  }

  i = math_equation_add_var_string(eq, str);
  if (i < 0) {
    *err = MATH_ERROR_MEMORY;
    math_expression_free(exp);
    return NULL;
  }

  exp->u.index = i;

  return exp;
}

MathExpression *
math_array_expression_new(MathEquation *eq, const char *name, MathExpression *operand, int is_string, int *err)
{
  MathExpression *exp;
  int i;
  enum MATH_EXPRESSION_TYPE type;

  type = (is_string) ? MATH_EXPRESSION_TYPE_STRING_ARRAY : MATH_EXPRESSION_TYPE_ARRAY;
  exp = math_expression_new(type, eq, err);
  if (exp == NULL) {
    return NULL;
  }

  i = math_equation_add_array(eq, name, is_string);
  if (i < 0) {
    *err = MATH_ERROR_MEMORY;
    math_expression_free(exp);
    return NULL;
  }

  exp->u.array.index = i;
  exp->u.array.operand = operand;

  return exp;
}

MathExpression *
math_array_argument_expression_new(MathEquation *eq, const char *name, int *err)
{
  MathExpression *exp;
  int i;

  exp = math_expression_new(MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT, eq, err);
  if (exp == NULL) {
    return NULL;
  }

  i = math_equation_add_array(eq, name, FALSE);
  if (i < 0) {
    *err = MATH_ERROR_MEMORY;
    math_expression_free(exp);
    return NULL;
  }

  exp->u.array.index = i;

  return exp;
}

MathExpression *
math_string_array_argument_expression_new(MathEquation *eq, const char *name, int *err)
{
  MathExpression *exp;
  int i;

  exp = math_expression_new(MATH_EXPRESSION_TYPE_STRING_ARRAY_ARGUMENT, eq, err);
  if (exp == NULL) {
    return NULL;
  }

  i = math_equation_add_array(eq, name, TRUE);
  if (i < 0) {
    *err = MATH_ERROR_MEMORY;
    math_expression_free(exp);
    return NULL;
  }

  exp->u.array.index = i;

  return exp;
}

int
math_function_get_arg_type_num(struct math_function_parameter *fprm)
{
  if (fprm->argc < 0) {
    return - 1 - fprm->argc;
  }
  return fprm->argc;
}

static int
check_argument_sub(enum MATH_FUNCTION_ARG_TYPE arg_type, enum MATH_EXPRESSION_TYPE type)
{
  switch (arg_type) {
  case MATH_FUNCTION_ARG_TYPE_DOUBLE:
  case MATH_FUNCTION_ARG_TYPE_PROC:
    switch (type) {
    case MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT:
    case MATH_EXPRESSION_TYPE_STRING_ARRAY_ARGUMENT:
      return 1;
    default:
      break;
    }
    break;
  case MATH_FUNCTION_ARG_TYPE_STRING:
    switch (type) {
    case MATH_EXPRESSION_TYPE_STRING:
    case MATH_EXPRESSION_TYPE_STRING_VARIABLE:
    case MATH_EXPRESSION_TYPE_STRING_ARRAY:
      break;
    default:
      return 1;
    }
    break;
  case MATH_FUNCTION_ARG_TYPE_ARRAY:
    switch (type) {
    case MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT:
      break;
    default:
      return 1;
    }
    break;
  case MATH_FUNCTION_ARG_TYPE_STRING_ARRAY:
    switch (type) {
    case MATH_EXPRESSION_TYPE_STRING_ARRAY_ARGUMENT:
      break;
    default:
      return 1;
    }
    break;
  case MATH_FUNCTION_ARG_TYPE_ARRAY_COMMON:
    switch (type) {
    case MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT:
    case MATH_EXPRESSION_TYPE_STRING_ARRAY_ARGUMENT:
      break;
    default:
      return 1;
    }
    break;
  case MATH_FUNCTION_ARG_TYPE_VARIABLE:
    switch (type) {
    case MATH_EXPRESSION_TYPE_VARIABLE:
    case MATH_EXPRESSION_TYPE_ARRAY:
      break;
    default:
      return 1;
    }
    break;
  case MATH_FUNCTION_ARG_TYPE_STRING_VARIABLE:
    switch (type) {
    case MATH_EXPRESSION_TYPE_STRING_VARIABLE:
    case MATH_EXPRESSION_TYPE_STRING_ARRAY:
      break;
    default:
      return 1;
    }
    break;
  case MATH_FUNCTION_ARG_TYPE_VARIABLE_COMMON:
    switch (type) {
    case MATH_EXPRESSION_TYPE_VARIABLE:
    case MATH_EXPRESSION_TYPE_ARRAY:
    case MATH_EXPRESSION_TYPE_STRING_VARIABLE:
    case MATH_EXPRESSION_TYPE_STRING_ARRAY:
      break;
    default:
      return 1;
    }
    break;
  }
  return 0;
}

static int
check_argument(struct math_function_parameter *fprm, int argc, MathExpression **argv)
{
  int i, arg_type_num;
  enum MATH_FUNCTION_ARG_TYPE arg_type;

  arg_type_num = math_function_get_arg_type_num(fprm);
  for (i = 0; i < argc; i++) {
    if (fprm->arg_type && i < arg_type_num) {
      arg_type = fprm->arg_type[i];
    } else {
      arg_type = MATH_FUNCTION_ARG_TYPE_DOUBLE;
    }
    if (check_argument_sub(arg_type, argv[i]->type)) {
      return 1;
    }
  }

  return 0;
}

MathExpression *
math_func_call_expression_new(MathEquation *eq, struct math_function_parameter *fprm, int argc, MathExpression **argv, int pos_id, int *err)
{
  MathExpression *exp;
  MathFunctionArgument *buf;

  if (check_argument(fprm, argc, argv)) {
    *err = MATH_ERROR_INVALID_ARG;
    return NULL;
  }

  buf = NULL;
  if (argc > 0) {
    buf = g_malloc(sizeof(*buf) * argc);
    if (buf == NULL) {
      *err = MATH_ERROR_MEMORY;
      return NULL;
    }
  }

  exp = math_expression_new(MATH_EXPRESSION_TYPE_FUNC_CALL, eq, err);
  if (exp == NULL) {
    g_free(buf);
    return NULL;
  }

  exp->u.func_call.argv = argv;
  exp->u.func_call.argc = argc;
  exp->u.func_call.buf = buf;
  exp->u.func_call.pos_id = pos_id;
  exp->u.func_call.fprm = fprm;

  return exp;
}

MathExpression *
math_binary_expression_new(enum MATH_EXPRESSION_TYPE type, MathEquation *eq, MathExpression *left, MathExpression *right, int *err)
{
  MathExpression *exp;

  exp = math_expression_new(type, eq, err);
  if (exp == NULL)
    return NULL;

  exp->u.bin.left = left;
  exp->u.bin.right = right;

  return exp;
}

MathExpression *
math_assign_expression_new(enum MATH_EXPRESSION_TYPE type, MathEquation *eq,
			   MathExpression *left, MathExpression *right, enum MATH_OPERATOR_TYPE op, int *err)
{
  MathExpression *exp;

  exp = math_expression_new(type, eq, err);
  if (exp == NULL)
    return NULL;

  exp->u.assign.op = op;
  exp->u.assign.left = left;
  exp->u.assign.right = right;

  return exp;
}

MathExpression *
math_unary_expression_new(enum MATH_EXPRESSION_TYPE type, MathEquation *eq, MathExpression *operand, int *err)
{
  MathExpression *exp;

  exp = math_expression_new(type, eq, err);
  if (exp == NULL)
    return NULL;

  exp->u.unary.operand = operand;

  return exp;
}

MathExpression *
math_double_expression_new(MathEquation *eq, const MathValue *val, int *err)
{
  MathExpression *exp;

  exp = math_expression_new(MATH_EXPRESSION_TYPE_DOUBLE, eq, err);
  if (exp == NULL)
    return NULL;

  exp->u.value = *val;

  return exp;
}

static void
check_expand(MathEquation *eq, MathStringExpression *str)
{
  char *ptr, *eqn;
  int in_variable, start, end, i;
  struct embedded_variable variable;
  MathExpression *exp;

  in_variable = FALSE;
  ptr = str->string;
  for (i = 0; ptr[i]; i++) {
    switch (ptr[i]) {
    case MATH_VARIABLE_EXPAND_PREFIX:
      if (ptr[i + 1] == '{') {
	in_variable = TRUE;
	start = i;
	i++;
	continue;
      }
      break;
    case '}':
      if (in_variable) {
	end = i;
	variable.start = start;
	variable.end = end;
	eqn = g_strndup(ptr + start + 2, end - start - 2);
	if (eqn) {
	  int err;
	  exp = math_parser_parse(eqn, eq, &err);
	  if (exp) {
	    variable.exp = exp;
	    if (arrayadd(str->variables, &variable) == NULL) {
	      math_expression_free(exp);
	    }
	  }
	  g_free(eqn);
	}
	in_variable = FALSE;
      }
      break;
    }
  }
}

MathExpression *
math_string_expression_new(MathEquation *eq, const char *str, int expand, int *err)
{
  MathExpression *exp;

  exp = math_expression_new(MATH_EXPRESSION_TYPE_STRING, eq, err);
  if (exp == NULL)
    return NULL;

  exp->u.str.variables = NULL;
  exp->u.str.expanded = NULL;
  exp->u.str.string = g_strdup(str);
  if (exp->u.str.string == NULL) {
    *err = MATH_ERROR_MEMORY;
    math_expression_free(exp);
    return NULL;
  }
  if (! expand) {
    return exp;
  }

  exp->u.str.variables = arraynew(sizeof(struct embedded_variable));
  if (exp->u.str.variables == NULL) {
    *err = MATH_ERROR_MEMORY;
    math_expression_free(exp);
    return NULL;
  }
  check_expand(eq, &exp->u.str);
  if (arraynum(exp->u.str.variables) == 0) {
    arrayfree(exp->u.str.variables);
    exp->u.str.variables = NULL;
  }
  if (exp->u.str.variables) {
    exp->u.str.expanded = g_string_new("");
    if (exp->u.str.expanded == NULL) {
      arrayfree(exp->u.str.variables);
      exp->u.str.variables = NULL;
    }
  }
  return exp;
}

static const char *
math_expression_get_string(MathExpression *expression)
{
  int i, n;
  struct embedded_variable *var;
  GString *gstr;
  char *ptr;
  int top, len;
  MathStringExpression *str_exp;

  if (expression == NULL) {
    return NULL;
  }
  str_exp = &(expression->u.str);
  if (str_exp->variables == NULL) {
    return str_exp->string;
  }
  n = arraynum(str_exp->variables);
  if (n < 1) {
    return str_exp->string;
  }
  gstr = str_exp->expanded;
  g_string_set_size(gstr, 0);
  ptr = str_exp->string;
  top = 0;
  var = arraydata(str_exp->variables);
  for (i = 0; i < n; i++) {
    MathExpression *exp;
    GString *varstr;
    MathValue val;
    int id;
    len = var[i].start - top;
    g_string_append_len(gstr, ptr + top, len);
    exp = var[i].exp;
    switch (exp->type) {
    case MATH_EXPRESSION_TYPE_STRING:
      g_string_append(gstr, exp->u.str.string);
      break;
    case MATH_EXPRESSION_TYPE_STRING_VARIABLE:
      id = exp->u.index;
      math_equation_get_string_var(expression->equation, id, &varstr);
      if (varstr) {
	g_string_append(gstr, varstr->str);
      }
      break;
    case MATH_EXPRESSION_TYPE_STRING_ARRAY:
      id = exp->u.index;
      if (math_expression_calculate(exp->u.array.operand, &val) == 0) {
	if (val.type == MATH_VALUE_NORMAL) {
	  const char *str;
	  str = math_equation_get_array_cstr(expression->equation, id, val.val);
	  if (str) {
	    g_string_append(gstr, str);
	  }
	}
      }
      break;
    default:
      if (math_expression_calculate(exp, &val) == 0) {
	if (val.type == MATH_VALUE_NORMAL) {
	  g_string_append_printf(gstr, "%G", val.val);
	}
      }
    }
    top = var[i].end + 1;
    if (i == n - 1) {
      g_string_append(gstr, ptr + var[i].end + 1);
    }
  }
  return gstr->str;
}

static void
free_expand_variables(struct narray *array)
{
  int i, n;
  struct embedded_variable *data;
  if (array == NULL) {
    return;
  }
  n = arraynum(array);
  data = arraydata(array);
  for (i = 0; i < n; i++) {
    math_expression_free(data[i].exp);
  }
}

static const char *
math_string_expression_free(MathStringExpression *exp)
{
  if (exp->string) {
    g_free(exp->string);
  }
  if (exp->expanded) {
    g_string_free(exp->expanded, TRUE);
  }
  if (exp->variables) {
    free_expand_variables(exp->variables);
    arrayfree(exp->variables);
  }
  return exp->string;
}

MathExpression *
math_eoeq_expression_new(MathEquation *eq, int *err)
{
  MathExpression *exp;

  exp = math_expression_new(MATH_EXPRESSION_TYPE_EOEQ, eq, err);
  return exp;
}

static void
math_expression_free_sub(MathExpression *exp)
{
  int i;

  if (exp == NULL)
    return;

  switch (exp->type) {
  case MATH_EXPRESSION_TYPE_OR:
  case MATH_EXPRESSION_TYPE_AND:
  case MATH_EXPRESSION_TYPE_EQ:
  case MATH_EXPRESSION_TYPE_NE:
  case MATH_EXPRESSION_TYPE_ADD:
  case MATH_EXPRESSION_TYPE_SUB:
  case MATH_EXPRESSION_TYPE_MUL:
  case MATH_EXPRESSION_TYPE_DIV:
  case MATH_EXPRESSION_TYPE_MOD:
  case MATH_EXPRESSION_TYPE_POW:
  case MATH_EXPRESSION_TYPE_GT:
  case MATH_EXPRESSION_TYPE_GE:
  case MATH_EXPRESSION_TYPE_LT:
  case MATH_EXPRESSION_TYPE_LE:
    math_expression_free(exp->u.bin.left);
    math_expression_free(exp->u.bin.right);
    break;
  case MATH_EXPRESSION_TYPE_ASSIGN:
  case MATH_EXPRESSION_TYPE_STRING_ASSIGN:
    math_expression_free(exp->u.assign.left);
    math_expression_free(exp->u.assign.right);
    break;
  case MATH_EXPRESSION_TYPE_FUNC:
    math_expression_free(exp->u.func.exp);
    free_arg_list(exp->u.func.arg_list);
    break;
  case MATH_EXPRESSION_TYPE_CONST_DEF:
    math_expression_free(exp->u.const_def.operand);
    break;
  case MATH_EXPRESSION_TYPE_FUNC_CALL:
    for (i = 0; i < exp->u.func_call.argc; i++) {
      math_expression_free(exp->u.func_call.argv[i]);
    }
    g_free(exp->u.func_call.argv);
    g_free(exp->u.func_call.buf);
    break;
  case MATH_EXPRESSION_TYPE_MINUS:
  case MATH_EXPRESSION_TYPE_FACT:
    math_expression_free(exp->u.unary.operand);
    break;
  case MATH_EXPRESSION_TYPE_ARRAY:
  case MATH_EXPRESSION_TYPE_STRING_ARRAY:
    math_expression_free(exp->u.array.operand);
    break;
  case MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT:
  case MATH_EXPRESSION_TYPE_STRING_ARRAY_ARGUMENT:
  case MATH_EXPRESSION_TYPE_CONST:
  case MATH_EXPRESSION_TYPE_VARIABLE:
    break;
  case MATH_EXPRESSION_TYPE_STRING:
    math_string_expression_free(&exp->u.str);
    break;
  case MATH_EXPRESSION_TYPE_STRING_VARIABLE:
    break;
  case MATH_EXPRESSION_TYPE_DOUBLE:
  case MATH_EXPRESSION_TYPE_PRM:
  case MATH_EXPRESSION_TYPE_EOEQ:
    break;
  case MATH_EXPRESSION_TYPE_BLOCK:
    math_expression_free(exp->u.exp);
    break;
  }

  g_free(exp);
}

void
math_expression_free(MathExpression *exp)
{
  MathExpression *next;

  while (exp) {
    next = exp->next;
    math_expression_free_sub(exp);
    exp = next;
  }

}

static double Fact_data[] = {
  1.000000000000000e+00,
  1.000000000000000e+00,
  2.000000000000000e+00,
  6.000000000000000e+00,
  2.400000000000000e+01,
  1.200000000000000e+02,
  7.200000000000000e+02,
  5.040000000000000e+03,
  4.032000000000000e+04,
  3.628800000000000e+05,
  3.628800000000000e+06,
  3.991680000000000e+07,
  4.790016000000000e+08,
  6.227020800000000e+09,
  8.717829120000000e+10,
  1.307674368000000e+12,
  2.092278988800000e+13,
  3.556874280960000e+14,
  6.402373705728000e+15,
  1.216451004088320e+17,
  2.432902008176640e+18,
  5.109094217170944e+19,
  1.124000727777608e+21,
  2.585201673888498e+22,
  6.204484017332394e+23,
  1.551121004333099e+25,
  4.032914611266057e+26,
  1.088886945041835e+28,
  3.048883446117139e+29,
  8.841761993739702e+30,
  2.652528598121911e+32,
  8.222838654177922e+33,
  2.631308369336935e+35,
  8.683317618811886e+36,
  2.952327990396042e+38,
  1.033314796638615e+40,
  3.719933267899013e+41,
  1.376375309122635e+43,
  5.230226174666011e+44,
  2.039788208119744e+46,
  8.159152832478977e+47,
  3.345252661316381e+49,
  1.405006117752880e+51,
  6.041526306337383e+52,
  2.658271574788449e+54,
  1.196222208654802e+56,
  5.502622159812089e+57,
  2.586232415111682e+59,
  1.241391559253607e+61,
  6.082818640342675e+62,
  3.041409320171338e+64,
  1.551118753287382e+66,
  8.065817517094388e+67,
  4.274883284060025e+69,
  2.308436973392414e+71,
  1.269640335365828e+73,
  7.109985878048635e+74,
  4.052691950487721e+76,
  2.350561331282878e+78,
  1.386831185456898e+80,
  8.320987112741390e+81,
  5.075802138772248e+83,
  3.146997326038794e+85,
  1.982608315404440e+87,
  1.268869321858842e+89,
  8.247650592082472e+90,
  5.443449390774431e+92,
  3.647111091818868e+94,
  2.480035542436831e+96,
  1.711224524281413e+98,
  1.197857166996989e+100,
  8.504785885678623e+101,
  6.123445837688608e+103,
  4.470115461512684e+105,
  3.307885441519386e+107,
  2.480914081139540e+109,
  1.885494701666050e+111,
  1.451830920282859e+113,
  1.132428117820630e+115,
  8.946182130782976e+116,
  7.156945704626381e+118,
  5.797126020747368e+120,
  4.753643337012842e+122,
  3.945523969720659e+124,
  3.314240134565353e+126,
  2.817104114380550e+128,
  2.422709538367273e+130,
  2.107757298379528e+132,
  1.854826422573984e+134,
  1.650795516090846e+136,
  1.485715964481762e+138,
  1.352001527678403e+140,
  1.243841405464131e+142,
  1.156772507081642e+144,
  1.087366156656743e+146,
  1.032997848823906e+148,
  9.916779348709496e+149,
  9.619275968248212e+151,
  9.426890448883248e+153,
  9.332621544394415e+155,
  9.332621544394415e+157,
  9.425947759838360e+159,
  9.614466715035127e+161,
  9.902900716486180e+163,
  1.029901674514563e+166,
  1.081396758240291e+168,
  1.146280563734708e+170,
  1.226520203196138e+172,
  1.324641819451829e+174,
  1.443859583202494e+176,
  1.588245541522743e+178,
  1.762952551090245e+180,
  1.974506857221074e+182,
  2.231192748659814e+184,
  2.543559733472188e+186,
  2.925093693493016e+188,
  3.393108684451898e+190,
  3.969937160808721e+192,
  4.684525849754291e+194,
  5.574585761207606e+196,
  6.689502913449127e+198,
  8.094298525273444e+200,
  9.875044200833601e+202,
  1.214630436702533e+205,
  1.506141741511141e+207,
  1.882677176888926e+209,
  2.372173242880047e+211,
  3.012660018457659e+213,
  3.856204823625804e+215,
  4.974504222477287e+217,
  6.466855489220474e+219,
  8.471580690878821e+221,
  1.118248651196004e+224,
  1.487270706090686e+226,
  1.992942746161519e+228,
  2.690472707318050e+230,
  3.659042881952549e+232,
  5.012888748274992e+234,
  6.917786472619489e+236,
  9.615723196941089e+238,
  1.346201247571753e+241,
  1.898143759076171e+243,
  2.695364137888163e+245,
  3.854370717180073e+247,
  5.550293832739304e+249,
  8.047926057471992e+251,
  1.174997204390911e+254,
  1.727245890454639e+256,
  2.556323917872865e+258,
  3.808922637630570e+260,
  5.713383956445855e+262,
  8.627209774233240e+264,
  1.311335885683452e+267,
  2.006343905095682e+269,
  3.089769613847351e+271,
  4.789142901463394e+273,
  7.471062926282894e+275,
  1.172956879426414e+278,
  1.853271869493735e+280,
  2.946702272495038e+282,
  4.714723635992062e+284,
  7.590705053947219e+286,
  1.229694218739449e+289,
  2.004401576545303e+291,
  3.287218585534296e+293,
  5.423910666131589e+295,
  9.003691705778438e+297,
  1.503616514864999e+300,
  2.526075744973198e+302,
  4.269068009004705e+304,
};

#define FACT_MAX (sizeof(Fact_data) / sizeof(*Fact_data))

static double
factorial(unsigned int n)
{
  if (n < FACT_MAX) {
    return Fact_data[n];
  }

  return HUGE_VAL;
}

static int
set_string_argument(MathFunctionCallExpression *exp, MathEquation *eq, int i)
{
  MathValue v;
  const char *str;
  GString *gstr;

  switch (exp->argv[i]->type) {
  case MATH_EXPRESSION_TYPE_STRING:
    str = math_expression_get_string(exp->argv[i]);
    break;
  case MATH_EXPRESSION_TYPE_STRING_ARRAY:
    if (CALC_EXPRESSION(exp->argv[i]->u.array.operand, v)) {
      return 1;
    }
    str = math_equation_get_array_cstr(eq, exp->argv[i]->u.array.index, v.val);
    break;
  case MATH_EXPRESSION_TYPE_STRING_VARIABLE:
    math_equation_get_string_var(eq, exp->argv[i]->u.index, &gstr);
    if (gstr == NULL) {
      return 1;
    }
    str = gstr->str;
    break;
  default:
    return 1;
  }
  if (str == NULL) {
    return 1;
  }
  exp->buf[i].cstr = str;
  return 0;
}

static int
set_variable_argument(MathFunctionCallExpression *exp, MathEquation *eq, int i)
{
  MathValue v, *ptr;

  switch (exp->argv[i]->type) {
  case MATH_EXPRESSION_TYPE_ARRAY:
    if (CALC_EXPRESSION(exp->argv[i]->u.array.operand, v)) {
      return 1;
    }
    ptr = math_equation_get_array_ptr(eq, exp->argv[i]->u.array.index, v.val);
    break;
  case MATH_EXPRESSION_TYPE_VARIABLE:
    ptr = math_equation_get_var_ptr(eq, exp->argv[i]->u.index);
    break;
  default:
    return 1;
  }
  exp->buf[i].variable.data.vptr = ptr;
  exp->buf[i].variable.type = DATA_TYPE_VALUE;
  return 0;
}

static int
set_string_variable_argument(MathFunctionCallExpression *exp, MathEquation *eq, int i)
{
  MathValue v;
  GString *gstr;

  switch (exp->argv[i]->type) {
  case MATH_EXPRESSION_TYPE_STRING_ARRAY:
    if (CALC_EXPRESSION(exp->argv[i]->u.array.operand, v)) {
      return 1;
    }
    gstr = math_equation_get_array_str(eq, exp->argv[i]->u.array.index, v.val);
    break;
  case MATH_EXPRESSION_TYPE_STRING_VARIABLE:
    math_equation_get_string_var(eq, exp->argv[i]->u.index, &gstr);
    break;
  default:
    return 1;
  }
  if (gstr == NULL) {
    return 1;
  }
  exp->buf[i].variable.type = DATA_TYPE_STRING;
  exp->buf[i].variable.data.str = gstr;
  return 0;
}

static int
call_func(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *val)
{
  int i, n, arg_type_num;
  enum MATH_FUNCTION_ARG_TYPE *type;

  type = exp->fprm->arg_type;

  if (exp->fprm->argc > 0 && exp->argc != exp->fprm->argc) {
    val->type = MATH_VALUE_ERROR;
    return 1;
  }

  arg_type_num = math_function_get_arg_type_num(exp->fprm);
  n = exp->argc;
  for (i = 0; i < n; i++) {
    if (type && i < arg_type_num) {
      switch (type[i]) {
      case MATH_FUNCTION_ARG_TYPE_VARIABLE:
	if (set_variable_argument(exp, eq, i)) {
	  val->type = MATH_VALUE_ERROR;
	  return 1;
	}
	break;
      case MATH_FUNCTION_ARG_TYPE_VARIABLE_COMMON:
        switch (exp->argv[i]->type) {
        case MATH_EXPRESSION_TYPE_ARRAY:
        case MATH_EXPRESSION_TYPE_VARIABLE:
          if (set_variable_argument(exp, eq, i)) {
            val->type = MATH_VALUE_ERROR;
            return 1;
          }
          break;
        case MATH_EXPRESSION_TYPE_STRING_ARRAY:
        case MATH_EXPRESSION_TYPE_STRING_VARIABLE:
          if (set_string_variable_argument(exp, eq, i)) {
            val->type = MATH_VALUE_ERROR;
            return 1;
          }
          break;
        default:
          val->type = MATH_VALUE_ERROR;
          return 1;
          break;
        }
        break;
      case MATH_FUNCTION_ARG_TYPE_ARRAY:
        switch (exp->argv[i]->type) {
        case MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT:
          exp->buf[i].array.array_type = DATA_TYPE_VALUE;
          exp->buf[i].array.idx = exp->argv[i]->u.array.index;
          break;
        default:
	  val->type = MATH_VALUE_ERROR;
	  return 1;
          break;
        }
	break;
      case MATH_FUNCTION_ARG_TYPE_PROC:
	if (exp->argv[i]->type == MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT) {
	  val->type = MATH_VALUE_ERROR;
	  return 1;
	}
	exp->buf[i].exp = exp->argv[i];
	break;
      case MATH_FUNCTION_ARG_TYPE_DOUBLE:
	if (exp->argv[i]->type == MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT) {
	  val->type = MATH_VALUE_ERROR;
	  return 1;
	}
	if (CALC_EXPRESSION(exp->argv[i], exp->buf[i].val))
	  return 1;
	break;
      case MATH_FUNCTION_ARG_TYPE_STRING:
	if (set_string_argument(exp, eq, i)) {
	  val->type = MATH_VALUE_ERROR;
	  return 1;
	}
	break;
      case MATH_FUNCTION_ARG_TYPE_STRING_VARIABLE:
	if (set_string_variable_argument(exp, eq, i)) {
	  val->type = MATH_VALUE_ERROR;
	  return 1;
	}
	break;
      case MATH_FUNCTION_ARG_TYPE_STRING_ARRAY:
        switch (exp->argv[i]->type) {
        case MATH_EXPRESSION_TYPE_STRING_ARRAY_ARGUMENT:
          exp->buf[i].array.array_type = DATA_TYPE_STRING;
          exp->buf[i].array.idx = exp->argv[i]->u.array.index;
          break;
        default:
	  val->type = MATH_VALUE_ERROR;
	  return 1;
          break;
        }
	break;
      case MATH_FUNCTION_ARG_TYPE_ARRAY_COMMON:
        switch (exp->argv[i]->type) {
        case MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT:
          exp->buf[i].array.array_type = DATA_TYPE_VALUE;
          exp->buf[i].array.idx = exp->argv[i]->u.array.index;
          break;
        case MATH_EXPRESSION_TYPE_STRING_ARRAY_ARGUMENT:
          exp->buf[i].array.array_type = DATA_TYPE_STRING;
          exp->buf[i].array.idx = exp->argv[i]->u.array.index;
          break;
        default:
	  return 1;
          break;
        }
      }
    } else if (CALC_EXPRESSION(exp->argv[i], exp->buf[i].val)) {
      return 1;
    }
  }

  return exp->fprm->func(exp, eq, val);
}

static MathExpression *
reduce_expression(MathExpression *exp, int *err)
{
#if 1
  MathValue val;
  int r;
  MathExpression *rexp;

  r = math_expression_calculate(exp, &val);
  if (r) {
    math_expression_free(exp);
    *err = MATH_ERROR_CALCULATION;
    return NULL;
  }

  rexp = math_double_expression_new(exp->equation, &val, NULL);
  math_expression_free(exp);

  return rexp;
#else
  return exp;
#endif
}

static MathExpression *
optimize_usr_function(MathExpression *exp,int *err)
{
  MathExpression *new_exp;

  new_exp = math_expression_new(MATH_EXPRESSION_TYPE_FUNC, exp->equation, NULL);
  if (new_exp == NULL)
    return NULL;

  new_exp->u.func.argc = exp->u.func.argc;
  new_exp->u.func.local_num = exp->u.func.local_num;
  new_exp->u.func.local_array_num = exp->u.func.local_array_num;
  new_exp->u.func.local_string_num = exp->u.func.local_string_num;
  new_exp->u.func.local_string_array_num = exp->u.func.local_string_array_num;
  new_exp->u.func.exp = math_expression_optimize(exp->u.func.exp, err);
  new_exp->u.func.fprm = exp->u.func.fprm;
  new_exp->u.func.arg_list = NULL;

  if (new_exp->u.func.exp == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  return new_exp;
}

static void
free_arg_array(int argc, MathExpression **argv)
{
  int i;
  for (i = 0; i < argc; i++) {
    math_expression_free(argv[i]);
  }

  g_free(argv);
}

static MathExpression *
optimize_func_call(MathExpression *exp, int *err)
{
  MathExpression **argv, *new_exp;
  int can_reduce, i, error;

  can_reduce = 1;
  argv = g_malloc0(exp->u.func_call.argc * sizeof(*argv));
  if (argv == NULL) {
    return NULL;
  }

  error = 0;
  for (i = 0; i < exp->u.func_call.argc; i++) {
    argv[i] = optimize(exp->u.func_call.argv[i], err);
    if (argv[i] == NULL) {
      error = 1;
    } else if (argv[i]->type != MATH_EXPRESSION_TYPE_DOUBLE) {
      can_reduce = 0;
    }
  }

  if (error) {
    free_arg_array(exp->u.func_call.argc, argv);
    return NULL;
  }

  new_exp = math_func_call_expression_new(exp->equation,
					  exp->u.func_call.fprm,
					  exp->u.func_call.argc,
					  argv,
					  exp->u.func_call.pos_id,
					  &error);

  if (new_exp == NULL) {
    free_arg_array(exp->u.func_call.argc, argv);
    return NULL;
  }

  if (can_reduce && ! exp->u.func_call.fprm->side_effect) {
    new_exp = reduce_expression(new_exp, err);
  }

  return new_exp;
}

static MathExpression *
optimize_or_expression(MathExpression *exp, int *err)
{
  MathExpression *left, *right, *new_exp;

  new_exp = math_binary_expression_new(exp->type, exp->equation, NULL, NULL, err);
  if (new_exp == NULL)
    return NULL;

  left = optimize(exp->u.bin.left, err);
  new_exp->u.bin.left = left;
  if (left == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  right = optimize(exp->u.bin.right, err);
  new_exp->u.bin.right = right;
  if (right == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  if (left->type == MATH_EXPRESSION_TYPE_DOUBLE &&
      right->type == MATH_EXPRESSION_TYPE_DOUBLE) {
    new_exp = reduce_expression(new_exp, err);
  } else if (left->type == MATH_EXPRESSION_TYPE_DOUBLE) {
    if (left->u.value.val == 0.0) {
      math_expression_free(new_exp);
      new_exp = right;
    } else {
      new_exp->u.bin.left = NULL;
      math_expression_free(new_exp);
      new_exp = left;
    }
  }

  return new_exp;
}

static MathExpression *
optimize_and_expression(MathExpression *exp, int *err)
{
  MathExpression *left, *right, *new_exp;

  new_exp = math_binary_expression_new(exp->type, exp->equation, NULL, NULL, err);
  if (new_exp == NULL)
    return NULL;

  left = optimize(exp->u.bin.left, err);
  new_exp->u.bin.left = left;
  if (left == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  right = optimize(exp->u.bin.right, err);
  new_exp->u.bin.right = right;
  if (right == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  if (left->type == MATH_EXPRESSION_TYPE_DOUBLE &&
      right->type == MATH_EXPRESSION_TYPE_DOUBLE) {
    new_exp = reduce_expression(new_exp, err);
  } else if (left->type == MATH_EXPRESSION_TYPE_DOUBLE) {
    if (left->u.value.val == 0.0) {
      new_exp->u.bin.left = NULL;
      math_expression_free(new_exp);
      new_exp = left;
    } else {
      new_exp->u.bin.right = NULL;
      math_expression_free(new_exp);
      new_exp = right;
    }
  }
  return new_exp;
}

static MathExpression *
optimize_bin_expression(MathExpression *exp, int *err)
{
  MathExpression *left, *right, *new_exp;

  new_exp = math_binary_expression_new(exp->type, exp->equation, NULL, NULL, err);
  if (new_exp == NULL)
    return NULL;

  left = optimize(exp->u.bin.left, err);
  new_exp->u.bin.left = left;
  if (left == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  right = optimize(exp->u.bin.right, err);
  new_exp->u.bin.right = right;
  if (right == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  if (left->type == MATH_EXPRESSION_TYPE_DOUBLE &&
      right->type == MATH_EXPRESSION_TYPE_DOUBLE) {
    new_exp = reduce_expression(new_exp, err);
  }
  return new_exp;
}

static MathExpression *
optimize_assign_expression(MathExpression *exp, int *err)
{
  MathExpression *left, *right, *new_exp;

  new_exp = math_assign_expression_new(exp->type, exp->equation, NULL, NULL, exp->u.assign.op, err);
  if (new_exp == NULL)
    return NULL;

  left = optimize(exp->u.assign.left, err);
  new_exp->u.assign.left = left;
  if (left == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  right = optimize(exp->u.assign.right, err);
  new_exp->u.assign.right = right;
  if (right == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  if (left->type == MATH_EXPRESSION_TYPE_DOUBLE &&
      right->type == MATH_EXPRESSION_TYPE_DOUBLE) {
    new_exp = reduce_expression(new_exp, err);
  }
  return new_exp;
}

static MathExpression *
optimize_mul_expression(MathExpression *exp, int *err)
{
  MathExpression *left, *right, *new_exp;

  new_exp = math_binary_expression_new(exp->type, exp->equation, NULL, NULL, err);
  if (new_exp == NULL)
    return NULL;

  left = optimize(exp->u.bin.left, err);
  new_exp->u.bin.left = left;
  if (left == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  right = optimize(exp->u.bin.right, err);
  new_exp->u.bin.right = right;
  if (right == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  if (left->type == MATH_EXPRESSION_TYPE_DOUBLE &&
      right->type == MATH_EXPRESSION_TYPE_DOUBLE) {
    new_exp = reduce_expression(new_exp, err);
  } else if (left->type == MATH_EXPRESSION_TYPE_DOUBLE &&
	     left->u.value.val == 1.0) {
    new_exp->u.bin.right = NULL;
    math_expression_free(new_exp);
    new_exp = right;
  } else if (right->type == MATH_EXPRESSION_TYPE_DOUBLE &&
	     right->u.value.val == 1.0) {
    new_exp->u.bin.left = NULL;
    math_expression_free(new_exp);
    new_exp = left;
  }

  return new_exp;
}

static MathExpression *
optimize_div_expression(MathExpression *exp, int *err)
{
  MathExpression *left, *right, *new_exp;

  new_exp = math_binary_expression_new(exp->type, exp->equation, NULL, NULL, err);
  if (new_exp == NULL)
    return NULL;

  left = optimize(exp->u.bin.left, err);
  new_exp->u.bin.left = left;
  if (left == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  right = optimize(exp->u.bin.right, err);
  new_exp->u.bin.right = right;
  if (right == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  if (left->type == MATH_EXPRESSION_TYPE_DOUBLE &&
      right->type == MATH_EXPRESSION_TYPE_DOUBLE) {
    new_exp = reduce_expression(new_exp, err);
  } else if (right->type == MATH_EXPRESSION_TYPE_DOUBLE) {
    if (right->u.value.val == 1.0) {
      new_exp->u.bin.left = NULL;
      math_expression_free(new_exp);
      new_exp = left;
    } else if (right->u.value.val == 0.0) {
      new_exp->type = MATH_EXPRESSION_TYPE_DIV; /* noting to do (a run-time error will be caused) */
    } else {
      right->u.value.val = 1.0 / right->u.value.val;
      new_exp->type = MATH_EXPRESSION_TYPE_MUL;
    }
  }

  return new_exp;
}

static MathExpression *
optimize_una_expression(MathExpression *exp, int *err)
{
  MathExpression *operand, *new_exp;

  new_exp = math_unary_expression_new(exp->type, exp->equation, NULL, err);
  if (new_exp == NULL)
    return NULL;

  operand = optimize(exp->u.unary.operand, err);
  new_exp->u.unary.operand = operand;
  if (operand == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  if (operand->type == MATH_EXPRESSION_TYPE_DOUBLE) {
    new_exp = reduce_expression(new_exp, err);
  }

  return new_exp;
}

static MathExpression *
optimize_array_expression(MathExpression *exp, int *err)
{
  MathExpression *operand, *new_exp;

  new_exp = math_expression_new(exp->type, exp->equation, err);
  if (new_exp == NULL)
    return NULL;

  operand = optimize(exp->u.array.operand, err);
  new_exp->u.array.operand = operand;
  new_exp->u.array.index = exp->u.array.index;
  if (operand == NULL) {
    math_expression_free(new_exp);
    return NULL;
  }

  return new_exp;
}

static MathExpression *
optimize_block_expression(MathExpression *exp, int *err)
{
  MathExpression *operand, *new_exp;
  operand = math_expression_optimize(exp->u.exp, err);
  if (operand == NULL) {
    return NULL;
  }
  new_exp = math_expression_new(exp->type, exp->equation, err);
  if (new_exp == NULL) {
    return NULL;
  }
  new_exp->u.exp = operand;
  return new_exp;
}

static MathExpression *
optimize(MathExpression *exp, int *err)
{
  MathExpression *new_exp;

  switch (exp->type) {
  case MATH_EXPRESSION_TYPE_BLOCK:
    new_exp = optimize_block_expression(exp, err);
    break;
  case MATH_EXPRESSION_TYPE_OR:
    new_exp = optimize_or_expression(exp, err);
    break;
  case MATH_EXPRESSION_TYPE_AND:
    new_exp = optimize_and_expression(exp, err);
    break;
  case MATH_EXPRESSION_TYPE_EQ:
  case MATH_EXPRESSION_TYPE_NE:
  case MATH_EXPRESSION_TYPE_ADD:
  case MATH_EXPRESSION_TYPE_SUB:
  case MATH_EXPRESSION_TYPE_MOD:
  case MATH_EXPRESSION_TYPE_POW:
  case MATH_EXPRESSION_TYPE_GT:
  case MATH_EXPRESSION_TYPE_GE:
  case MATH_EXPRESSION_TYPE_LT:
  case MATH_EXPRESSION_TYPE_LE:
    new_exp = optimize_bin_expression(exp, err);
    break;
  case MATH_EXPRESSION_TYPE_ASSIGN:
    new_exp = optimize_assign_expression(exp, err);
    break;
  case MATH_EXPRESSION_TYPE_MUL:
    new_exp = optimize_mul_expression(exp, err);
    break;
  case MATH_EXPRESSION_TYPE_DIV:
    new_exp = optimize_div_expression(exp, err);
    break;
  case MATH_EXPRESSION_TYPE_MINUS:
  case MATH_EXPRESSION_TYPE_FACT:
    new_exp = optimize_una_expression(exp, err);
    break;
  case MATH_EXPRESSION_TYPE_FUNC_CALL:
    new_exp = optimize_func_call(exp, err);
    break;
  case MATH_EXPRESSION_TYPE_FUNC:
    new_exp = optimize_usr_function(exp, err);
    break;
  case MATH_EXPRESSION_TYPE_DOUBLE:
    new_exp = math_double_expression_new(exp->equation, &exp->u.value, err);
    break;
  case MATH_EXPRESSION_TYPE_CONST:
    new_exp = math_expression_new(exp->type, exp->equation, err);
    if (new_exp) {
      new_exp->u.index = exp->u.index;
      new_exp = reduce_expression(new_exp, err);
    }
    break;
  case MATH_EXPRESSION_TYPE_VARIABLE:
    new_exp = math_expression_new(exp->type, exp->equation, err);
    if (new_exp)
      new_exp->u.index = exp->u.index;
    break;
  case MATH_EXPRESSION_TYPE_PRM:
    new_exp = math_expression_new(exp->type, exp->equation, err);
    if (new_exp) {
      new_exp->u.prm.type = exp->u.prm.type;
      new_exp->u.prm.index = exp->u.prm.index;
      new_exp->u.prm.prm = exp->u.prm.prm;
      new_exp->u.prm.id = exp->u.prm.id;
    }
    break;
  case MATH_EXPRESSION_TYPE_EOEQ:
    new_exp = math_eoeq_expression_new(exp->equation, err);
    break;
  case MATH_EXPRESSION_TYPE_ARRAY:
    new_exp = optimize_array_expression(exp, err);
    break;
  case MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT:
    new_exp = math_expression_new(exp->type, exp->equation, err);
    if (new_exp) {
      new_exp->u.array.index = exp->u.array.index;
    }
    break;
  default:
    new_exp = NULL; /* never reached */
  }

  return new_exp;
}

MathExpression *
math_expression_optimize(MathExpression *exp, int *err)
{
  MathExpression *prev, *rexp, *top;

  *err = MATH_ERROR_NONE;

  top = prev = NULL;
  while (exp) {
    rexp = optimize(exp, err);
    if (rexp == NULL || *err != MATH_ERROR_NONE) {
      math_expression_free(top);
      return NULL;
    }

    if (prev) {
      prev->next = rexp;
    }

    if (top == NULL) {
      top = rexp;
    }

    prev = rexp;

    exp = exp->next;
  }

  return top;
}

static int
set_val_to_array(MathExpression *exp, MathValue *val, enum MATH_OPERATOR_TYPE op)
{
  MathValue v;
  int i;

  if (CALC_EXPRESSION(exp->u.array.operand, v)) {
    return 1;
  }

  i = v.val;

  if (op == MATH_OPERATOR_TYPE_ASSIGN) {
    if (math_equation_set_array_val(exp->equation, exp->u.array.index, i, val)) {
      return 1;
    }
  } else {
    if (math_equation_get_array_val(exp->equation, exp->u.array.index, i, &v)) {
      return 1;
    }
    switch (op) {
    case MATH_OPERATOR_TYPE_POW_ASSIGN:
      v.val = pow(v.val, val->val);
      break;
    case MATH_OPERATOR_TYPE_MOD_ASSIGN:
      v.val = fmod(v.val, val->val);
      break;
    case MATH_OPERATOR_TYPE_DIV_ASSIGN:
      v.val /= val->val;
      break;
    case MATH_OPERATOR_TYPE_MUL_ASSIGN:
      v.val *= val->val;
      break;
    case MATH_OPERATOR_TYPE_PLUS_ASSIGN:
      v.val += val->val;
      break;
    case MATH_OPERATOR_TYPE_MINUS_ASSIGN:
      v.val -= val->val;
      break;
    default:
      /* never reached */
      v.val = 0;
    }
    if (math_equation_set_array_val(exp->equation, exp->u.array.index, i, &v)) {
      return 1;
    }
    *val = v;
  }

  return 0;
}

#define MATH_CHECK_VAL(rval, v) if (v.type != MATH_VALUE_NORMAL) {	\
    *rval = v;								\
    break;								\
  }

static char *
val2str(MathValue *val)
{
  char *str;
  const char *tmp;

  if (val == NULL) {
    return NULL;
  }

  tmp = math_special_value_to_string(val);
  if (tmp == NULL) {
    str = g_strdup_printf("%G", val->val);
  } else {
    str = g_strdup(tmp);
  }
  return str;
}

static int
assign_string(MathExpression *exp)
{
  GString *gstr;
  int id;
  const char *str;
  char *tmp;
  MathValue operand;
  MathExpression *left, *right;

  tmp = NULL;
  right = exp->u.assign.right;
  switch (right->type) {
  case MATH_EXPRESSION_TYPE_STRING:
    str = math_expression_get_string(right);
    break;
  case MATH_EXPRESSION_TYPE_STRING_ARRAY:
    if (CALC_EXPRESSION(right->u.array.operand, operand)) {
      return 1;
    }
    str = math_equation_get_array_cstr(exp->equation, right->u.array.index, operand.val);
    if (str == NULL) {
      return 1;
    }
    break;
  case MATH_EXPRESSION_TYPE_STRING_VARIABLE:
    id = (int) right->u.index;
    math_equation_get_string_var(exp->equation, id, &gstr);
    if (gstr == NULL) {
      return 1;
    }
    str = gstr->str;
    break;
  default:
    if (CALC_EXPRESSION(right, operand)) {
      return 1;
    }
    str = tmp = val2str(&operand);
  }
  left = exp->u.assign.left;
  if (left->type == MATH_EXPRESSION_TYPE_STRING_VARIABLE) {
    if (math_equation_set_var_string(exp->equation, left->u.index, str)) {
      return 1;
    }
  } else {
    MathValue v;
    int i;
    if (exp->u.assign.op != MATH_OPERATOR_TYPE_ASSIGN) {
      return 1;
    }
    if (CALC_EXPRESSION(left->u.array.operand, v)) {
      return 1;
    }
    i = v.val;
    if (math_equation_set_array_str(exp->equation, left->u.array.index, i, str)) {
      return 1;
    }
  }
  if (tmp) {
    g_free(tmp);
  }
  return 0;
}

static int
calc(MathExpression *exp, MathValue *val)
{
  MathValue left, right, operand;

  val->type = MATH_VALUE_NORMAL;

  switch (exp->type) {
  case MATH_EXPRESSION_TYPE_BLOCK:
    if (math_expression_calculate(exp->u.exp, &operand)) {
      return 1;
    }
    *val = operand;
    break;
  case MATH_EXPRESSION_TYPE_OR:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (left.type == MATH_VALUE_NORMAL && left.val) {
      *val = left;
      break;
    }

    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    *val = right;
    break;
  case MATH_EXPRESSION_TYPE_AND:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (left.type != MATH_VALUE_NORMAL || ! left.val) {
      *val = left;
      break;
    }

    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    *val = right;
    break;
  case MATH_EXPRESSION_TYPE_EQ:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    MATH_CHECK_VAL(val, left);
    MATH_CHECK_VAL(val, right);
    val->val = (left.val == right.val);
    break;
  case MATH_EXPRESSION_TYPE_NE:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    MATH_CHECK_VAL(val, left);
    MATH_CHECK_VAL(val, right);
    val->val = (left.val != right.val);
    break;
  case MATH_EXPRESSION_TYPE_ADD:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    MATH_CHECK_VAL(val, left);
    MATH_CHECK_VAL(val, right);
    val->val = left.val + right.val;
    break;
  case MATH_EXPRESSION_TYPE_SUB:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    MATH_CHECK_VAL(val, left);
    MATH_CHECK_VAL(val, right);
    val->val = left.val - right.val;
    break;
  case MATH_EXPRESSION_TYPE_MUL:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    MATH_CHECK_VAL(val, left);
    MATH_CHECK_VAL(val, right);
    val->val = left.val * right.val;
    break;
  case MATH_EXPRESSION_TYPE_DIV:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    MATH_CHECK_VAL(val, left);
    MATH_CHECK_VAL(val, right);
    if (right.val == 0.0) {
      if (left.val < 0) {
	val->val = - HUGE_VAL;
      } else if (left.val > 0) {
	val->val = HUGE_VAL;
      } else {
	val->val = 0;
      }
      val->type = MATH_VALUE_NAN;
    } else {
      val->val = left.val / right.val;
    }
    break;
  case MATH_EXPRESSION_TYPE_MOD:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    MATH_CHECK_VAL(val, left);
    MATH_CHECK_VAL(val, right);
    if (right.val == 0.0) {
      return 1;
    }
    val->val = fmod(left.val, right.val);
    break;
  case MATH_EXPRESSION_TYPE_POW:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    MATH_CHECK_VAL(val, left);
    MATH_CHECK_VAL(val, right);
    val->val = pow(left.val, right.val);
    break;
  case MATH_EXPRESSION_TYPE_GT:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    MATH_CHECK_VAL(val, left);
    MATH_CHECK_VAL(val, right);
    val->val = (left.val > right.val);
    break;
  case MATH_EXPRESSION_TYPE_GE:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    MATH_CHECK_VAL(val, left);
    MATH_CHECK_VAL(val, right);
    val->val = (left.val >= right.val);
    break;
  case MATH_EXPRESSION_TYPE_LT:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    MATH_CHECK_VAL(val, left);
    MATH_CHECK_VAL(val, right);
    val->val = (left.val < right.val);
    break;
  case MATH_EXPRESSION_TYPE_LE:
    if (CALC_EXPRESSION(exp->u.bin.left, left)) {
      return 1;
    }
    if (CALC_EXPRESSION(exp->u.bin.right, right)) {
      return 1;
    }
    MATH_CHECK_VAL(val, left);
    MATH_CHECK_VAL(val, right);
    val->val = (left.val <= right.val);
    break;
  case MATH_EXPRESSION_TYPE_MINUS:
    if (CALC_EXPRESSION(exp->u.unary.operand, operand)) {
      return 1;
    }
    MATH_CHECK_VAL(val, operand);
    val->val = - operand.val;
    break;
  case MATH_EXPRESSION_TYPE_FACT:
    if (CALC_EXPRESSION(exp->u.unary.operand, operand)) {
      return 1;
    }

    MATH_CHECK_VAL(val, operand);

    if (operand.val < 0 || operand.val >= FACT_MAX) {
      return 1;
    }

    val->val = factorial(operand.val);
    break;
  case MATH_EXPRESSION_TYPE_FUNC_CALL:
    if (call_func(&exp->u.func_call, exp->equation, val))
      return 1;
    break;
  case MATH_EXPRESSION_TYPE_DOUBLE:
    *val = exp->u.value;
    break;
  case MATH_EXPRESSION_TYPE_CONST:
    if (math_equation_get_const(exp->equation, exp->u.index, val))
      return 1;
    break;
  case MATH_EXPRESSION_TYPE_VARIABLE:
    if (math_equation_get_var(exp->equation, exp->u.index, val))
      return 1;
    break;
  case MATH_EXPRESSION_TYPE_ASSIGN:
    if (CALC_EXPRESSION(exp->u.assign.right, right)) {
      return 1;
    }
    if (exp->u.assign.left->type == MATH_EXPRESSION_TYPE_VARIABLE) {
      if (math_equation_set_var(exp->equation, exp->u.assign.left->u.index, &right)) {
	return 1;
      }
    } else {
      if (set_val_to_array(exp->u.assign.left, &right, exp->u.assign.op)) {
	return 1;
      }
    }

    *val = right;
    break;
  case MATH_EXPRESSION_TYPE_ARRAY:
    if (CALC_EXPRESSION(exp->u.array.operand, operand)) {
      return 1;
    }
    if (math_equation_get_array_val(exp->equation, exp->u.array.index, operand.val, val)) {
      return 1;
    }
    break;
  case MATH_EXPRESSION_TYPE_STRING_ARRAY:
    if (CALC_EXPRESSION(exp->u.array.operand, operand)) {
      return 1;
    }
    val->val = 0;
    {
      const char *str;
      str = math_equation_get_array_cstr(exp->equation, exp->u.array.index, operand.val);
      if (str == NULL) {
        return 1;
      }
      n_strtod(str, val);
    }
    break;
  case MATH_EXPRESSION_TYPE_PRM:
    if (exp->u.prm.prm->data == NULL) {
      return 1;
    }
    *val = exp->u.prm.prm->data[exp->u.prm.index];
    break;
  case MATH_EXPRESSION_TYPE_EOEQ:
    val->val = 0;
    break;
  case MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT:
  case MATH_EXPRESSION_TYPE_STRING_ARRAY_ARGUMENT:
    return 1;
  case MATH_EXPRESSION_TYPE_STRING_VARIABLE:
    val->val = 0;
    {
      GString *gstr;
      math_equation_get_string_var(exp->equation, exp->u.index, &gstr);
      if (gstr && gstr->str) {
        n_strtod(gstr->str, val);
      }
    }
    break;
  case MATH_EXPRESSION_TYPE_STRING:
    val->val = 0;
    {
      const char *str;
      str = math_expression_get_string(exp);
      if (str) {
	n_strtod(str, val);
      }
    }
    break;
  case MATH_EXPRESSION_TYPE_STRING_ASSIGN:
    if (assign_string(exp)) {
      return 1;
    }
    val->val = 0;	    /* a string is always evaluated as zero */
    break;
  case MATH_EXPRESSION_TYPE_FUNC:
  case MATH_EXPRESSION_TYPE_CONST_DEF:
    /* never reached */
    val->val = 0;
    break;
  }

  return 0;
}

enum DATA_TYPE
math_expression_get_variable_type_from_argument(MathFunctionCallExpression *exp, int i)
{
  return exp->buf[i].variable.type;
}

const char *
math_expression_get_string_from_argument(MathFunctionCallExpression *exp, int i)
{
  return exp->buf[i].cstr;
}

GString *
math_expression_get_string_variable_from_argument(MathFunctionCallExpression *exp, int i)
{
  if (exp->buf[i].variable.type != DATA_TYPE_STRING) {
    return NULL;
  }
  return exp->buf[i].variable.data.str;
}

int
math_function_call_expression_get_variable(MathFunctionCallExpression *exp, int i, MathVariable *var)
{
  var->type = exp->buf[i].variable.type;
  switch (var->type) {
  case DATA_TYPE_VALUE:
    var->type = DATA_TYPE_VALUE;
    var->data.vptr = exp->buf[i].variable.data.vptr;
    break;
  case DATA_TYPE_STRING:
    var->type = DATA_TYPE_STRING;
    var->data.str = exp->buf[i].variable.data.str;
    break;
  }
  return 0;
}

int
math_variable_set_common_value(MathVariable *variable, MathCommonValue *val)
{
  if (variable->type != val->type) {
    return 1;
  }
  switch (variable->type) {
  case DATA_TYPE_VALUE:
    *(variable->data.vptr) = val->data.val;
    break;
  case DATA_TYPE_STRING:
    g_string_assign(variable->data.str, val->data.cstr);
    break;
  }
  return 0;
}

MathValue *
math_expression_get_variable_from_argument(MathFunctionCallExpression *exp, int i)
{
  if (exp->buf[i].variable.type != DATA_TYPE_VALUE) {
    return NULL;
  }
  return exp->buf[i].variable.data.vptr;
}

int
math_expression_calculate(MathExpression *exp, MathValue *val)
{
  MathExpression *ptr;
  MathValue v;

  if (exp == NULL) {
    return 1;
  }

  ptr = exp;
  while (ptr) {
    v.val = 0;
    v.type = MATH_VALUE_NORMAL;
    if (calc(ptr, &v)) {
      *val = v;
      if (val->type != MATH_VALUE_INTERRUPT) {
	val->type = MATH_VALUE_ERROR;
      }
      return 1;
    }
    ptr = ptr->next;
  }

  *val = v;
  return 0;
}

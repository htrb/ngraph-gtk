/*
 * $Id: math_equation.c,v 1.13 2009-11-24 06:32:37 hito Exp $
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <glib.h>

#include "math_parser.h"
#include "math_equation.h"

#include "object.h"
#include "nstring.h"

#define BUF_UNIT 64

struct usr_func_array_info {
  int prev_num;
  MathEquationArray *prev, *local;
};

struct scope_info
{
  int variable_offset;
  int string_offset;
  struct usr_func_array_info array, string_array;
};

static void init_parameter(MathEquation *eq);
static void free_parameter(MathEquation *eq);
static void free_func(NHASH func);
static void optimize_func(NHASH func);
static int optimize_const_definition(MathEquation *eq);
static int math_equation_call_user_func(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
static int check_const_in_expression_list(MathExpression *exp, int *constant, int n);
static int check_const_sub(MathExpression *exp, int *constant, int n);

static MathValue *
add_to_ary(MathValue *buf, int *num, int *size, const MathValue *val)
{
  MathValue *ptr;

  if (*num % BUF_UNIT == 0) {
    int n;

    n = *num / BUF_UNIT;
    ptr = g_realloc(buf, sizeof(*buf) * (n + 1) * BUF_UNIT);
    if (ptr == NULL) {
      g_free(buf);
      *num = 0;
      return NULL;
    }
    buf = ptr;

    if (size)
      *size = (n + 1) * BUF_UNIT;
  }

  if (val) {
    buf[*num] = *val;
  } else {
    buf[*num].val = 0;
    buf[*num].type = MATH_VALUE_NORMAL;
  }
  (*num)++;

  return buf;
}

static int *
add_to_ary_int(int *buf, int *num, int val)
{
  int *ptr;

  if (*num % BUF_UNIT == 0) {
    int n;

    n = *num / BUF_UNIT;
    ptr = g_realloc(buf, sizeof(*buf) * (n + 1) * BUF_UNIT);
    if (ptr == NULL) {
      g_free(buf);
      *num = 0;
      return NULL;
    }
    buf = ptr;
  }

  buf[*num] = val;
  (*num)++;

  return buf;
}

static MathEquationArray *
add_to_ary_array(MathEquationArray *buf, int *num)
{
  MathEquationArray *ptr;

  if (*num % BUF_UNIT == 0) {
    int n;

    n = *num / BUF_UNIT;
    ptr = g_realloc(buf, sizeof(*buf) * (n + 1) * BUF_UNIT);
    if (ptr == NULL) {
      g_free(buf);
      *num = 0;
      return NULL;
    }
    buf = ptr;
  }

  buf[*num].num = 0;
  buf[*num].size = 0;
  buf[*num].data.ptr = NULL;

  (*num)++;

  return buf;
}

MathEquation *
math_equation_basic_new(void)
{
  MathEquation *eq;

  eq = math_equation_new();
  if (eq == NULL) {
    return NULL;
  }

  if (math_add_basic_constant(eq))
    goto ErrEnd;

  if (math_add_basic_function(eq))
    goto ErrEnd;

  return eq;

 ErrEnd:
  math_equation_free(eq);

  return NULL;

}

static int
math_stack_init(MathStack *stack, int type)
{
  int size;
  if (type == DATA_TYPE_STRING) {
    size = sizeof(*(stack->stack.str));
  } else {
    size = sizeof(*(stack->stack.val));
  }
  stack->type = type;
  stack->variable = nhash_new();
  if (stack->variable == NULL) {
    return 1;
  }
  stack->local_variable = nhash_new();
  if (stack->local_variable == NULL) {
    nhash_free(stack->variable);
    stack->variable = NULL;
    return 1;
  }
  stack->num  = 0;
  stack->ofst = 0;
  stack->end  = 0;
  stack->size = 0;
  stack->element_size = size;
  return 0;
}

static void
free_stack_strings(MathStack *stack)
{
  int i;
  for (i = 0; i < stack->end; i++) {
    GString *str;
    str = stack->stack.str[i];
    if (str) {
      g_string_free(str, TRUE);
      stack->stack.str[i] = NULL;
    }
  }
}

static void
math_stack_free(MathStack *stack)
{
  if (stack->variable) {
    nhash_free(stack->variable);
    stack->variable = NULL;
  }
  if (stack->local_variable) {
    nhash_free(stack->local_variable);
    stack->local_variable = NULL;
  }
  if (stack->type == DATA_TYPE_STRING) {
    free_stack_strings(stack);
  }
  g_free(stack->stack.ptr);
  stack->stack.ptr = NULL;
}

static int
math_array_init(MathArray *array, enum DATA_TYPE type)
{
  array->type = type;
  array->array = nhash_new();
  array->local_array = nhash_new();
  if (array->array == NULL || array->local_array == NULL) {
    return 1;
  }
  array->buf = NULL;
  array->num = 0;
  array->local_num = 0;
  return 0;
}

MathEquation *
math_equation_new(void)
{
  MathEquation *eq;
  int r;

  eq = g_malloc(sizeof(*eq));
  if (eq == NULL)
    return NULL;

  memset(eq, 0, sizeof(*eq));

  eq->constant = nhash_new();
  eq->function = nhash_new();
  eq->scope_info = arraynew(sizeof(struct scope_info));
  r = math_stack_init(&(eq->stack), DATA_TYPE_VALUE);
  if (r) {
    math_equation_free(eq);
    return NULL;
  }
  r = math_stack_init(&(eq->string_stack), DATA_TYPE_STRING);
  if (r) {
    math_equation_free(eq);
    return NULL;
  }
  r = math_array_init(&(eq->array), DATA_TYPE_VALUE);
  if (r) {
    math_equation_free(eq);
    return NULL;
  }
  r = math_array_init(&(eq->string_array), DATA_TYPE_STRING);
  if (r) {
    math_equation_free(eq);
    return NULL;
  }

  if (eq->function == NULL ||
      eq->constant == NULL ||
      eq->scope_info == NULL) {
    math_equation_free(eq);
    return NULL;
  }


  return eq;
}

static void
clear_pos_func_buf(MathEquation *eq)
{
  int i;

  if (eq->pos_func_buf == NULL) {
    return;
  }
  for (i = 0; i < eq->pos_func_num; i++) {
    eq->pos_func_buf[i].val = 0;
    eq->pos_func_buf[i].type = MATH_VALUE_UNDEF;
  }
}

static void
clear_variable_array(MathValue *vbuf, int n)
{
  memset(vbuf, 0, sizeof(*vbuf) * n);
}

static void
clear_string_array(GString **gstr, int n)
{
  int i;
  for (i = 0; i < n; i++) {
    g_string_set_size(gstr[i], 0);
  }
}

static void
clear_arrays(MathArray *array)
{
  int i, n;
  n = array->num;
  if (n <= 0) {
    return;
  }
  if (array->buf == NULL) {
    return;
  }
  for (i = 0; i < n; i++) {
    if (array->buf[i].num <= 0 || array->buf[i].data.ptr == NULL) {
      continue;
    }
    if (array->type == DATA_TYPE_VALUE) {
      clear_variable_array(array->buf[i].data.val, array->buf[i].num);
    } else {
      clear_string_array(array->buf[i].data.str, array->buf[i].num);
    }
  }
}

void
math_equation_clear(MathEquation *eq)
{
  if (eq == NULL)
    return;

  clear_pos_func_buf(eq);

  clear_variable_array(eq->stack.stack.val, eq->stack.num);
  clear_arrays(&eq->array);
  clear_arrays(&eq->string_array);
}

void
math_equation_set_parse_error(MathEquation *eq, const char *ptr, const struct math_string *str)
{
  if (eq == NULL)
    return;

  eq->err_info.pos.pos = ptr;
  eq->err_info.pos.line = str->line + 1;
  eq->err_info.pos.ofst = str->ofst + 1;
}

void
math_equation_set_const_error(MathEquation *eq, int id)
{
  if (eq == NULL)
    return;

  eq->err_info.const_id = id;
}

void
math_equation_set_func_arg_num_error(MathEquation *eq, struct math_function_parameter *fprm, int arg_num)
{
  if (eq == NULL)
    return;

  eq->err_info.func.fprm = fprm;
  eq->err_info.func.arg_num = arg_num;
}

void
math_equation_set_func_error(MathEquation *eq, struct math_function_parameter *fprm)
{
  math_equation_set_func_arg_num_error(eq, fprm, 0);
}

int
math_equation_parse(MathEquation *eq, const char *str)
{
  int err;

  if (eq == NULL)
    return MATH_ERROR_UNKNOWN;

  memset(&eq->err_info, 0, sizeof(eq->err_info));

  if (eq->cnum > 0 && eq->cbuf == NULL)
    return MATH_ERROR_UNKNOWN;

  if (eq->pos_func_buf) {
    g_free(eq->pos_func_buf);
    eq->pos_func_buf = NULL;
  }

  eq->pos_func_num = 0;

  if (eq->opt_exp) {
    math_expression_free(eq->opt_exp);
    eq->opt_exp = NULL;
  }

  if (eq->exp) {
    math_expression_free(eq->exp);
  }

  init_parameter(eq);

  if (str) {
    eq->exp = math_parser_parse(str, eq, &err);
  } else {
    eq->exp = NULL;
  }

  if (eq->pos_func_num > 0) {
    eq->pos_func_buf = g_malloc(eq->pos_func_num * sizeof(*eq->pos_func_buf));
    if (eq->pos_func_buf == NULL) {
      math_expression_free(eq->exp);
      return MATH_ERROR_MEMORY;
    }
    clear_pos_func_buf(eq);
  }

  return err;
}

static void
free_array_buf(MathEquationArray *buf, int num)
{
  int i;

  if (buf == NULL)
    return;

  for (i = 0; i < num; i++) {
    g_free(buf[i].data.ptr);
  }

  g_free(buf);
}

static void
free_string_array_buf(MathEquationArray *buf, int num)
{
  int i;

  if (buf == NULL) {
    return;
  }

  for (i = 0; i < num; i++) {
    int j, n;
    GString **ary;
    ary = buf[i].data.str;
    n = buf[i].size;
    for (j = 0; j < n; j++) {
      if (ary[j]) {
	g_string_free(ary[j], TRUE);
      }
    }
    g_free(buf[i].data.ptr);
  }
  g_free(buf);
}

static void
math_array_free(MathArray *array)
{
  if (array == NULL) {
    return;
  }
  if (array->array) {
    nhash_free(array->array);
  }
  if (array->local_array) {
    nhash_free(array->local_array);
  }
  switch (array->type) {
  case DATA_TYPE_VALUE:
    free_array_buf(array->buf, array->num);
    break;
  case DATA_TYPE_STRING:
    free_string_array_buf(array->buf, array->num);
    break;
  }
}

void
math_equation_free(MathEquation *eq)
{
  if (eq == NULL)
    return;

  if (eq->constant)
    nhash_free(eq->constant);

  math_stack_free(&eq->stack);
  math_stack_free(&eq->string_stack);

  math_array_free(&eq->array);
  math_array_free(&eq->string_array);

  if (eq->function)
    free_func(eq->function);

  if (eq->const_def) {
    math_expression_free(eq->const_def);
  }

  free_parameter(eq);

  arrayfree(eq->scope_info);
  math_expression_free(eq->exp);
  math_expression_free(eq->opt_exp);
  g_free(eq->cbuf);
  g_free(eq->pos_func_buf);
  g_free(eq);
}

int
math_equation_optimize(MathEquation *eq)
{
  int err;

  if ((eq == NULL) ||
      (eq->exp == NULL) ||
      (eq->cnum > 0 && eq->cbuf == NULL) ||
      (eq->stack.num > 0 && eq->stack.stack.ptr == NULL) ||
      (eq->string_stack.num > 0 && eq->string_stack.stack.ptr == NULL)) {
    return 1;
  }

  /* optimize_const_definition() must be called before any other optimization */
  if (optimize_const_definition(eq)) {
    return 1;
  }

  /* optimize_func() must be called before optimize_expression() */
  optimize_func(eq->function);

  if (eq->opt_exp) {
    math_expression_free(eq->opt_exp);
  }

  eq->opt_exp = math_expression_optimize(eq->exp, &err);

  return err;
}

int
math_equation_calculate(MathEquation *eq, MathValue *val)
{
  int r;

  if (val == NULL) {
    return 1;
  }

  val->val = 0;
  val->type = MATH_VALUE_ERROR;

  if ((eq == NULL) || (eq->exp == NULL) ||
      (eq->cnum > 0 && eq->cbuf == NULL) ||
      (eq->stack.num > 0 && eq->stack.stack.val == NULL) ||
      (eq->string_stack.num > 0 && eq->string_stack.stack.ptr == NULL)) {
    return 1;
  }

  if (eq->opt_exp) {
    r = math_expression_calculate(eq->opt_exp, val);
  } else {
    r = math_expression_calculate(eq->exp, val);
  }

  return r;
}

int
math_equation_add_parameter(MathEquation *eq, int type, int min, int max, int use_index)
{
  MathEquationParametar *ptr, *prm;

  prm = g_malloc(sizeof(*prm));
  if (prm == NULL)
    return 1;

  prm->type = type;
  prm->min_length = min;
  prm->max_length = max;
  prm->use_index = use_index;
  prm->id_num = 0;
  prm->id_max = -1;
  prm->next = NULL;
  prm->id = NULL;

  if (eq->parameter == NULL) {
    eq->parameter = prm;
  } else {
    ptr = eq->parameter;
    while (ptr->next) {
      ptr = ptr->next;
    }
    ptr->next = prm;
    prm->next = NULL;
  }

  return 0;
}

static int
add_parameter_data(MathEquationParametar *ptr, int val)
{
  if (ptr->id == NULL) {
    ptr->id = add_to_ary_int(ptr->id, &ptr->id_num, val);
  } else {
    int i;
    for (i = 0; i < ptr->id_num; i++) {
      if (ptr->id[i] == val) {
	return i;
      }
    }
    ptr->id = add_to_ary_int(ptr->id, &ptr->id_num, val);
  }

  if (ptr->id == NULL) {
    /* error: cannot allocate enough memory */
    return -1;
  }

  if (val > ptr->id_max) {
    ptr->id_max = val;
  }

  return ptr->id_num - 1;
}

int
math_equation_use_parameter(MathEquation *eq, int type, int val)
{
  MathEquationParametar *ptr;

  if (eq->parameter == NULL) {
    /* error: the parameter is not exist */
    return -1;
  }

  ptr = eq->parameter;
  while (ptr) {
    if (ptr->type == type) {
      break;
    }
    ptr = ptr->next;
  }

  if (ptr == NULL) {
    /* error: the parameter is not exist */
    return -1;
  }

  return add_parameter_data(ptr, val);
}

int
math_equation_set_parameter_data(MathEquation *eq, int type, MathValue *data)
{
  MathEquationParametar *ptr;

  if (eq->parameter == NULL) {
    /* error: the parameter is not exist */
    return 1;
  }

  ptr = eq->parameter;
  while (ptr) {
    if (ptr->type == type) {
      ptr->data = data;
      return 0;
    }
    ptr = ptr->next;
  }

  return 1;
}

MathEquationParametar *
math_equation_get_parameter(MathEquation *eq, int type, int *err)
{
  MathEquationParametar *ptr;

  if (eq == NULL || eq->parameter == NULL) {
    if (err) {
      *err = MATH_ERROR_INVALID_PRM;
    }
    return NULL;
  }

  if (eq->func_def) {
    if (err) {
      *err = MATH_ERROR_PRM_IN_DEF;
    }
    return NULL;
  }

  ptr = eq->parameter;
  while (ptr) {
    if (ptr->type == type) {
      break;
    }
    ptr = ptr->next;
  }

  return ptr;
}

static void
init_parameter(MathEquation *eq)
{
  MathEquationParametar *ptr;

  if (eq->parameter == NULL) {
    return;
  }

  ptr = eq->parameter;
  while (ptr) {
    if (ptr->id) {
      g_free(ptr->id);
      ptr->id = NULL;
    }
    ptr->id_num = 0;
    ptr->id_max = 0;
    ptr = ptr->next;
  }
}

static void
free_parameter(MathEquation *eq)
{
  MathEquationParametar *ptr, *next;

  if (eq->parameter == NULL) {
    return;
  }

  ptr = eq->parameter;
  while (ptr) {
    next = ptr->next;
    if (ptr->id) {
      g_free(ptr->id);
    }
    g_free(ptr);
    ptr = next;
  }
}

int
math_equation_add_pos_func(MathEquation *eq, struct math_function_parameter *fprm)
{
  int n;

  if (! fprm->positional)
    return -1;

  if (eq->func_def)
    return MATH_ERROR_INVALID_FUNC;

  n = eq->pos_func_num;
  eq->pos_func_num++;

  return n;
}

static void
free_func_prm_sub(struct math_function_parameter *ptr)
{
  g_free(ptr->name);

  if (ptr->opt_usr) {
    math_expression_free(ptr->opt_usr);
    ptr->opt_usr = NULL;
  }

  if (ptr->base_usr) {
    math_expression_free(ptr->base_usr);
    ptr->base_usr = NULL;
  }

  g_free(ptr->arg_type);
}

static void
free_func_prm(struct math_function_parameter *ptr)
{

  free_func_prm_sub(ptr);
  g_free(ptr);
}

static int
free_func_cb(struct nhash *hash, void *ptr)
{
  struct math_function_parameter *fprm;

  fprm = (struct math_function_parameter *) hash->val.p;

  free_func_prm(fprm);

  return 0;
}

static void
free_func(NHASH func)
{
  nhash_each(func, free_func_cb, NULL);

  nhash_free(func);

  return;
}

static int
optimize_func_cb(struct nhash *hash, void *ptr)
{
  struct math_function_parameter *fprm;
  int err;

  fprm = (struct math_function_parameter *) hash->val.p;
  if (fprm->opt_usr) {
    math_expression_free(fprm->opt_usr);
    fprm->opt_usr = NULL;
  }

  if (fprm->base_usr) {
    fprm->opt_usr = math_expression_optimize(fprm->base_usr, &err);
  }

  if (fprm->opt_usr && fprm->opt_usr->u.func.exp->type == MATH_EXPRESSION_TYPE_DOUBLE) {
    fprm->side_effect = 0;
  } else {
    fprm->side_effect = 1;
  }

  return 0;
}

static void
optimize_func(NHASH func)
{
  nhash_each(func, optimize_func_cb, NULL);

  return;
}

void
math_equation_remove_func(MathEquation *eq, const char *name)
{
  int r;
  struct math_function_parameter *ptr;

  r = nhash_get_ptr(eq->function, name, (void *) &ptr);
  if (r == 0) {
    free_func_prm(ptr);
    nhash_del(eq->function, name);

    if (eq->exp) {
      math_expression_free(eq->exp);
      eq->exp = NULL;
    }

    if (eq->opt_exp) {
      math_expression_free(eq->opt_exp);
      eq->opt_exp = NULL;
    }
  }
}

struct math_function_parameter *
math_equation_start_user_func_definition(MathEquation *eq, const char *name)
{
  struct math_function_parameter *fprm;

  if (eq == NULL || eq->func_def)
    return NULL;

  fprm = g_malloc(sizeof(*fprm));
  if (fprm == NULL)
    return NULL;

  fprm->argc = 0;
  fprm->side_effect = 1;
  fprm->positional = 0;
  fprm->func = math_equation_call_user_func;
  fprm->base_usr = NULL;
  fprm->opt_usr = NULL;
  fprm->arg_type = NULL;
  fprm->name = g_strdup(name);

  if (fprm->name == NULL) {
    g_free(fprm);
    return NULL;
  }

  eq->stack.local_num = 0;
  eq->array.local_num = 0;
  eq->func_def = 1;

  return fprm;
}

int
math_equation_register_user_func_definition(MathEquation *eq, const char *name, MathExpression *exp)
{
  struct math_function_parameter *fprm;

  if (eq == NULL || ! eq->func_def || name == NULL || exp == NULL)
    return 1;

  fprm = math_equation_get_func(eq, name);
  if (fprm == NULL) {
    int r;
    r = nhash_set_ptr(eq->function, name, exp->u.func.fprm);
    return r;
  }

#if 0
  if (fprm->argc != exp->u.func.fprm->argc) {
    return 1;
  }

  if (fprm->argc > 0) {
    enum MATH_FUNCTION_ARG_TYPE *arg_type1, *arg_type2;

    arg_type1 = fprm->arg_type;
    arg_type2 = exp->u.func.fprm->arg_type;

    if (arg_type1 && arg_type2) {
      int i;
      for (i = 0; i < fprm->argc; i++) {
	if (arg_type1[i] != arg_type2[i]) {
	  return 1;
	}
      }
    } else if ((arg_type1 && arg_type2 == NULL) ||
	       (arg_type1 == NULL && arg_type2)) {
      return 1;
    }
  }
#endif

  free_func_prm_sub(fprm);
  memcpy(fprm, exp->u.func.fprm, sizeof(*fprm));

  g_free(exp->u.func.fprm);
  exp->u.func.fprm = fprm;

  return 0;
}

static void
clear_stack_local(MathStack *stack, int *vnum)
{
  if (vnum) {
    *vnum = nhash_num(stack->local_variable);
  }
  nhash_clear(stack->local_variable);
  stack->local_num = 0;
}

int
math_equation_finish_user_func_definition(MathEquation *eq, int *vnum, int *anum, int *str_anum, int *snum)
{
  if (eq == NULL || ! eq->func_def)
    return 1;

  clear_stack_local(&eq->stack, vnum);
  clear_stack_local(&eq->string_stack, snum);

  if (anum) {
    *anum = nhash_num(eq->array.local_array);
  }
  nhash_clear(eq->array.local_array);

  if (str_anum) {
    *str_anum = nhash_num(eq->string_array.local_array);
  }
  nhash_clear(eq->string_array.local_array);

  eq->array.local_num = 0;
  eq->string_array.local_num = 0;
  eq->func_def = 0;

  return 0;
}

struct math_function_parameter *
math_equation_add_func(MathEquation *eq, const char *name, struct math_function_parameter *prm)
{
  int r;
  struct math_function_parameter *ptr;

  if (eq == NULL)
    return NULL;

  r = nhash_get_ptr(eq->function, name, (void *) &ptr);
  if (r == 0) {
    free_func_prm(ptr);
  }

  ptr = g_malloc(sizeof(*ptr));
  if (ptr == NULL)
    return NULL;

  memcpy(ptr, prm, sizeof(*ptr));
  ptr->name = g_strdup(name);

  if (ptr->name == NULL) {
    g_free(ptr);
    return NULL;
  }

  if (prm->arg_type) {
    int argc;
    argc = math_function_get_arg_type_num(prm);
    if (argc > 0) {
      ptr->arg_type = g_malloc(sizeof(*prm->arg_type) * argc);
      if (ptr->arg_type == NULL) {
        g_free(ptr->name);
        g_free(ptr);
        return NULL;
      }
      memcpy(ptr->arg_type, prm->arg_type, sizeof(*prm->arg_type) * argc);
    }
  }

  r = nhash_set_ptr(eq->function, name, ptr);
  if (r) {
    g_free(ptr->name);
    g_free(ptr->arg_type);
    g_free(ptr);
    return NULL;
  }

  return ptr;
}

struct math_function_parameter *
math_equation_get_func(MathEquation *eq, const char *name)
{
  int r;
  struct math_function_parameter *ptr;

  r = nhash_get_ptr(eq->function, name, (void *) &ptr);
  if (r) {
    return NULL;
  }

  return (struct math_function_parameter *) ptr;
}

int
math_equation_add_const(MathEquation *eq, const char *name, const MathValue *val)
{
  int i, r;

  if (eq == NULL)
    return -1;

  r = nhash_get_int(eq->constant, name, &i);
  if (r) {
    i = eq->cnum;
    eq->cbuf = add_to_ary(eq->cbuf, &eq->cnum, NULL, val);

    if (eq->cbuf == NULL) {
      /* error: cannot allocate enough memory */
      return -1;
    }

    nhash_set_int(eq->constant, name, i);
  } else if (eq->cbuf && i < eq->cnum && val) {
    eq->cbuf[i] = *val;
  }

  return i;
}

int
math_equation_add_const_definition(MathEquation *eq, const char *name, MathExpression *exp, int *err)
{
  int i, r;
  MathValue val;

  if (eq == NULL)
    return -1;

  r = nhash_get_int(eq->constant, name, &i);
  if (r == 0) {
    /* error: the constant is already exist */
    *err = MATH_ERROR_CONST_EXIST;
    eq->err_info.const_id = i;
    return -1;
  }


  if (math_expression_calculate(exp->u.const_def.operand, &val)) {
    *err = MATH_ERROR_CALCULATION;
    return -1;
  }

  i = math_equation_add_const(eq, name, &val);
  if (i < 0) {
    *err = MATH_ERROR_MEMORY;
    return -1;
  }

  exp->u.const_def.id = i;

  exp->next = eq->const_def;
  eq->const_def = exp;

  return i;
}

static int
optimize_const_definition(MathEquation *eq)
{
  MathExpression *exp;
  MathValue val;

  if (eq->const_def == NULL) {
    return 0;
  }

  exp = eq->const_def;

  while (exp) {
    if (math_expression_calculate(exp->u.const_def.operand, &val))
      return 1;
    math_equation_set_const(eq, exp->u.const_def.id, &val);
    exp = exp->next;
  }

  return 0;
}

static void
init_string_array(GString **str, int n)
{
  int i;
  for (i = 0; i < n; i++) {
    str[i] = g_string_new("");
  }
}

static int
expand_stack(MathStack *stack, int size)
{
  char *ptr;
  int request_size;

  request_size = stack->end + size;

  if (stack->size <= request_size) {
    int n;
    n = (request_size / BUF_UNIT + 1) * BUF_UNIT;
    ptr = g_realloc(stack->stack.ptr, stack->element_size * n);
    if (ptr == NULL) {
      return 1;
    }
    stack->stack.ptr = ptr;
    stack->size = n;
  }

  if (stack->stack.ptr == NULL)
    return 1;

  if (stack->type == DATA_TYPE_STRING) {
    init_string_array(stack->stack.str + stack->end, size);
  } else {
    memset(stack->stack.val + stack->end, 0, stack->element_size * size);
  }
  stack->ofst = stack->end;
  stack->end = request_size;

  return 0;
}

int
math_equation_add_var(MathEquation *eq, const char *name)
{
  int i, r;

  if (eq == NULL)
    return -1;

  if (eq->func_def) {
    r = nhash_get_int(eq->stack.local_variable, name, &i);
    if (r) {
      i = eq->stack.local_num;
      nhash_set_int(eq->stack.local_variable, name, i);
      eq->stack.local_num++;
    }
    return i;
  }

  r = nhash_get_int(eq->stack.variable, name, &i);
  if (r) {
    i = eq->stack.num;
    if (expand_stack(&(eq->stack), 1)) {
      /* error: cannot allocate enough memory */
      return -1;
    }

    eq->stack.ofst = 0;
    eq->stack.num++;
    nhash_set_int(eq->stack.variable, name, i);
  }

  return i;
}

int
math_equation_add_var_string(MathEquation *eq, const char *name)
{
  int i, r;

  if (eq == NULL)
    return -1;

  if (eq->func_def) {
    r = nhash_get_int(eq->string_stack.local_variable, name, &i);
    if (r) {
      i = eq->string_stack.local_num;
      nhash_set_int(eq->string_stack.local_variable, name, i);
      eq->string_stack.local_num++;
    }
    return i;
  }

  r = nhash_get_int(eq->string_stack.variable, name, &i);
  if (r) {
    i = eq->string_stack.num;
    if (expand_stack(&(eq->string_stack), 1)) {
      /* error: cannot allocate enough memory */
      return -1;
    }

    eq->string_stack.ofst = 0;
    eq->string_stack.num++;
    nhash_set_int(eq->string_stack.variable, name, i);
  }

  return i;
}

int
math_equation_set_const_by_name(MathEquation *eq, const char *name, const MathValue *val)
{
  int i, r;

  r = nhash_get_int(eq->constant, name, &i);
  if (r) {
    return 1;
  }

  if (eq->cbuf == NULL) {
    return 1;
  }

  eq->cbuf[i] = *val;

  return 0;
}

struct search_val {
  int val;
  char *name;
};

int
search_val_cb(struct nhash *hash, void *ptr)
{
  struct search_val *v;

  v = (struct search_val *) ptr;
  if (hash->val.i == v->val) {
    v->name = hash->key;
  }
  return 0;
}

char *
math_equation_get_var_name(MathEquation *eq, int idx)
{
  struct search_val v;

  if (eq == NULL)
    return NULL;

  v.val = idx;
  v.name = NULL;

  nhash_each(eq->stack.variable, search_val_cb, &v);

  return v.name;
}

char *
math_equation_get_const_name(MathEquation *eq, int idx)
{
  struct search_val v;

  if (eq == NULL)
    return NULL;

  v.val = idx;
  v.name = NULL;

  nhash_each(eq->constant, search_val_cb, &v);

  return v.name;
}

int
math_equation_set_const(MathEquation *eq, int idx, const MathValue *val)
{
  if (eq->cbuf == NULL || idx >= eq->cnum || idx < 0) {
    return 1;
  }

  eq->cbuf[idx] = *val;

  return 0;
}

int
math_equation_set_var_string(MathEquation *eq, int idx, const char *str)
{
  int i;

  i = idx + eq->string_stack.ofst;
  if (eq->string_stack.stack.str[i] == NULL) {
    eq->string_stack.stack.str[i] = g_string_new(str);
  } else {
    g_string_assign(eq->string_stack.stack.str[i], str);
  }
  return 0;
}

int
math_equation_set_var(MathEquation *eq, int idx, const MathValue *val)
{
  if (eq->stack.stack.val == NULL || idx + eq->stack.ofst >= eq->stack.end) {
    return 1;
  }

  eq->stack.stack.val[idx + eq->stack.ofst] = *val;

  return 0;
}

int
math_equation_get_const_by_name(MathEquation *eq, const char *name, MathValue *val)
{
  int i, r;

  r = nhash_get_int(eq->constant, name, &i);
  if (r) {
    /* error: cannot find the constant */
    return -1;
  }

  if (eq->cbuf == NULL) {
    /* error: cannot find the constant */
    return -1;
  }

  if (val) {
    *val = eq->cbuf[i];
  }

  return i;
}

int
math_equation_get_const(MathEquation *eq, int idx, MathValue *val)
{
  if (eq->cbuf == NULL || idx >= eq->cnum) {
    return 1;
  }

  if (val) {
    *val = eq->cbuf[idx];
  }

  return 0;
}

#define USER_FUNC_NEST_MAX 8192

static int
local_array_alloc(MathFunctionExpression *func, MathFunctionArgument *argv, struct usr_func_array_info *info)
{
  int i, j;
  info->local = NULL;
  if (func->local_array_num < 0) {
    return 0;
  }
  info->local = g_malloc(sizeof(*info->local) * func->local_array_num);
  if (info->local == NULL) {
    return 1;
  }
  memset(info->local, 0, sizeof(*info->local) * func->local_array_num);

  if (func->fprm->arg_type == NULL) {
    return 0;
  }
  j = 0;
  for (i = 0; i < func->argc; i++) {
    if (func->fprm->arg_type[i] == MATH_FUNCTION_ARG_TYPE_ARRAY) {
      info->local[j] = info->prev[argv[i].array.idx];
      j++;
    }
  }
  return 0;
}

static int
local_string_array_alloc(MathFunctionExpression *func, MathFunctionArgument *argv, struct usr_func_array_info *info)
{
  int i, j;
  info->local = NULL;
  if (func->local_string_array_num < 0) {
    return 0;
  }
  info->local = g_malloc(sizeof(*info->local) * func->local_string_array_num);
  if (info->local == NULL) {
    return 1;
  }
  memset(info->local, 0, sizeof(*info->local) * func->local_string_array_num);

  if (func->fprm->arg_type == NULL) {
    return 0;
  }
  j = 0;
  for (i = 0; i < func->argc; i++) {
    if (func->fprm->arg_type[i] == MATH_FUNCTION_ARG_TYPE_STRING_ARRAY) {
      info->local[j] = info->prev[argv[i].array.idx];
      j++;
    }
  }
  return 0;
}

static void
local_array_free(MathFunctionExpression *func, MathFunctionArgument *argv, struct usr_func_array_info *info)
{
  int i, j;
  if (func->fprm->arg_type == NULL) {
    return;
  }
  j = 0;
  for (i = 0; i < func->argc; i++) {
    if (func->fprm->arg_type[i] == MATH_FUNCTION_ARG_TYPE_ARRAY) {
      info->prev[argv[i].array.idx] = info->local[j];
      info->local[j].num = 0;
      info->local[j].size = 0;
      info->local[j].data.val = NULL;
      j++;
    }
  }
  free_array_buf(info->local, func->local_array_num);
}

static void
local_string_array_free(MathFunctionExpression *func, MathFunctionArgument *argv, struct usr_func_array_info *info)
{
  int i, j;
  if (func->fprm->arg_type == NULL) {
    return;
  }
  j = 0;
  for (i = 0; i < func->argc; i++) {
    if (func->fprm->arg_type[i] == MATH_FUNCTION_ARG_TYPE_STRING_ARRAY) {
      info->prev[argv[i].array.idx] = info->local[j];
      info->local[j].num = 0;
      info->local[j].size = 0;
      info->local[j].data.str = NULL;
      j++;
    }
  }
  free_string_array_buf(info->local, func->local_string_array_num);
}

struct scope_info *
scope_info_push(MathFunctionExpression *func, MathFunctionArgument *argv, MathEquation *eq)
{
  struct narray *array;
  struct scope_info scope;
  scope.variable_offset = eq->stack.ofst;
  scope.string_offset = eq->string_stack.ofst;
  scope.array.prev = eq->array.buf;
  scope.array.prev_num = eq->array.num;
  scope.array.local = NULL;
  scope.string_array.prev = eq->string_array.buf;
  scope.string_array.prev_num = eq->string_array.num;
  scope.string_array.local = NULL;

  local_array_alloc(func, argv, &scope.array);
  if (func->local_array_num > 0 && scope.array.local == NULL) {
    return NULL;
  }

  local_string_array_alloc(func, argv, &scope.string_array);
  if (func->local_string_array_num > 0 && scope.string_array.local == NULL) {
    return NULL;
  }

  eq->array.buf = scope.array.local;
  eq->array.num = func->local_array_num;
  eq->string_array.buf = scope.string_array.local;
  eq->string_array.num = func->local_string_array_num;

  array = arrayadd(eq->scope_info, &scope);
  if (array == NULL) {
    return NULL;
  }
  return arraylast(eq->scope_info);
}

static void
scope_info_pop(MathFunctionExpression *func, MathFunctionArgument *argv, MathEquation *eq)
{
  struct scope_info *scope;
  scope = arraylast(eq->scope_info);
  if (scope == NULL) {
    return;
  }

  local_array_free(func, argv, &scope->array);
  local_string_array_free(func, argv, &scope->string_array);

  eq->stack.ofst = scope->variable_offset;
  eq->string_stack.ofst = scope->string_offset;
  eq->array.buf = scope->array.prev;
  eq->array.num = scope->array.prev_num;
  eq->string_array.buf = scope->string_array.prev;
  eq->string_array.num = scope->string_array.prev_num;
  eq->scope_info->num--;
}

static int
math_equation_call_user_func(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  static int nest = 0;
  int end, str_end, r, i, j, k;
  struct scope_info *scope;
  MathExpression *func;
  MathFunctionArgument *argv;
  const char *str;
  GString *gstr;
  enum MATH_FUNCTION_ARG_TYPE arg_type;

  if (nest > USER_FUNC_NEST_MAX)
    return 1;

  if (exp->fprm->opt_usr) {
    func = exp->fprm->opt_usr;
  } else if (exp->fprm->base_usr) {
    func = exp->fprm->base_usr;
  } else {
    return 1;
  }

  argv = exp->buf;

  scope = scope_info_push(&func->u.func, argv, eq);
  end = eq->stack.end;
  str_end = eq->string_stack.end;

  if (scope == NULL) {
    return 1;
  }

  if (expand_stack(&(eq->stack), func->u.func.local_num)) {
    scope_info_pop(&func->u.func, argv, eq);
    return 1;
  }

  if (expand_stack(&(eq->string_stack), func->u.func.local_string_num)) {
    scope_info_pop(&func->u.func, argv, eq);
    return 1;
  }

  j = k = 0;
  for (i = 0; i < exp->argc; i++) {
    arg_type = (func->u.func.fprm->arg_type) ? func->u.func.fprm->arg_type[i] : MATH_FUNCTION_ARG_TYPE_DOUBLE;
    switch (arg_type) {
    case MATH_FUNCTION_ARG_TYPE_DOUBLE:
      eq->stack.stack.val[eq->stack.ofst + j] = argv[i].val;
      j++;
      break;
    case MATH_FUNCTION_ARG_TYPE_STRING:
    case MATH_FUNCTION_ARG_TYPE_STRING_VARIABLE:
      str = math_expression_get_string_from_argument(exp, i);
      if (str == NULL) {
        scope_info_pop(&func->u.func, argv, eq);
        return 1;
      }
      gstr = eq->string_stack.stack.str[eq->string_stack.ofst + k];
      g_string_assign(gstr, str);
      k++;
      break;
    default:
      break;
    }
  }

  nest++;
  r = math_expression_calculate(func->u.func.exp, rval);
  nest--;

  for (i = 0; i < func->u.func.local_string_num; i++) {
    j = eq->string_stack.ofst + i;
    g_string_free(eq->string_stack.stack.str[j], TRUE);
    eq->string_stack.stack.str[j] = NULL;
  }

  scope_info_pop(&func->u.func, argv, eq);
  eq->stack.end = end;
  eq->string_stack.end = str_end;

  return r;
}

static int
math_equation_check_var_common(MathStack *stack, const char *name, int func_def)
{
  int r, i;

  if (func_def) {
    r = nhash_get_int(stack->local_variable, name, &i);
  } else {
    r = nhash_get_int(stack->variable, name, &i);
  }

  if (r) {
    return -1;
  }

  return i;
}

int
math_equation_check_var(MathEquation *eq, const char *name)
{
  return math_equation_check_var_common(&(eq->stack), name, eq->func_def);
}

int
math_equation_check_string_var(MathEquation *eq, const char *name)
{
  return math_equation_check_var_common(&(eq->string_stack), name, eq->func_def);
}

int
math_equation_get_var(MathEquation *eq, int idx, MathValue *val)
{
  if (eq->stack.stack.val == NULL || idx + eq->stack.ofst >= eq->stack.end) {
    return 1;
  }

  if (val) {
    *val = eq->stack.stack.val[idx + eq->stack.ofst];
  }

  return 0;
}

MathValue *
math_equation_get_var_ptr(MathEquation *eq, int idx)
{
  if (eq->stack.stack.val == NULL || idx + eq->stack.ofst >= eq->stack.end) {
    return NULL;
  }

  return eq->stack.stack.val + (idx + eq->stack.ofst);
}

int
math_equation_get_string_var(MathEquation *eq, int idx, GString **str)
{
  if (eq->string_stack.stack.str == NULL || idx + eq->string_stack.ofst >= eq->string_stack.end) {
    return 1;
  }

  if (str) {
    *str = eq->string_stack.stack.str[idx + eq->string_stack.ofst];
  }

  return 0;
}

void
math_equation_clear_variable(MathEquation *eq)
{
  if (eq->stack.stack.ptr) {
    switch (eq->stack.type) {
    case DATA_TYPE_VALUE:
      clear_variable_array(eq->stack.stack.val, eq->stack.num); /* which is better to use stack.num or stack.end? */
      break;
    case DATA_TYPE_STRING:
      free_stack_strings(&(eq->stack));
      break;
    }
    g_free(eq->stack.stack.ptr);
    eq->stack.num = 0;
    eq->stack.stack.ptr = NULL;
  }
  nhash_clear(eq->stack.variable);
}

int
math_equation_check_array(MathEquation *eq, const char *name)
{
  int r, i;

  if (eq->func_def) {
    r = nhash_get_int(eq->array.local_array, name, &i);
  } else {
    r = nhash_get_int(eq->array.array, name, &i);
  }

  if (r) {
    return -1;
  }

  return i;
}

int
math_equation_check_string_array(MathEquation *eq, const char *name)
{
  int r, i;

  if (eq->func_def) {
    r = nhash_get_int(eq->string_array.local_array, name, &i);
  } else {
    r = nhash_get_int(eq->string_array.array, name, &i);
  }

  if (r) {
    return -1;
  }

  return i;
}

int
math_equation_add_array(MathEquation *eq, const char *name, int is_string)
{
  int i, r;
  MathArray *array;

  if (eq == NULL)
    return -1;

  if (is_string) {
    array = &eq->string_array;
  } else {
    array = &eq->array;
  }
  if (eq->func_def) {
    r = nhash_get_int(array->local_array, name, &i);
    if (r) {
      i = array->local_num;
      if (nhash_set_int(array->local_array, name, i)) {
	/* error: cannot allocate enough memory */
	return -1;
      }
      array->local_num++;
    }
  } else {
    r = nhash_get_int(array->array, name, &i);
    if (r) {
      i = array->num;
      array->buf = add_to_ary_array(array->buf, &array->num);

      if (array->array == NULL) {
	/* error: cannot allocate enough memory */
	return -1;
      }

      if (nhash_set_int(array->array, name, i)) {
	/* error: cannot allocate enough memory */
	return -1;
      }
    }
  }

  return i;
}

static int
check_array(MathArray *array, int id, int index)
{
  int i;
  MathEquationArray *ary;
  void *ptr;
  size_t element_size;

  if (array->buf == NULL || id < 0 || id >= array->num) {
    /* error: the array is not exist */
    return -1;
  }

  ary = array->buf + id;

  if (index < 0) {
    i = ary->num + index;
  } else {
    i = index;
  }

  if (i < 0 || i > MATH_EQUATION_ARRAY_INDEX_MAX) {
    /* error: the index of the array is out of bound */
    return -1;
  }

  element_size = (array->type == DATA_TYPE_STRING) ? sizeof(*ary->data.str) : sizeof(*ary->data.val);
  if (i >= ary->size) {
    int n;

    n = (i / BUF_UNIT + 1) * BUF_UNIT;

    ptr = g_realloc(ary->data.val, element_size * n);
    if (ptr == NULL) {
      /* error: cannot allocate enough memory */
      return -1;
    }

    ary->data.ptr = ptr;
    if (array->type == DATA_TYPE_STRING) {
      init_string_array(ary->data.str + ary->num, n - ary->num);
    } else {
      memset(ary->data.val + ary->num, 0, element_size * (n - ary->num));
    }
    ary->size = n;
  }

  if (i >= ary->num) {
    ary->num = i + 1;
  }

  return i;
}

int
math_equation_clear_array(MathEquation *eq, int array)
{
  int i;

  if (eq == NULL) {
    return 1;
  }
  i = check_array(&eq->array, array, 0);
  if (i < 0)
    return 1;

  clear_variable_array(eq->array.buf[array].data.val, eq->array.buf[array].num);
  eq->array.buf[array].num = 0;

  return 0;
}

int
math_equation_clear_string_array(MathEquation *eq, int array)
{
  int i;

  if (eq == NULL) {
    return 1;
  }
  i = check_array(&eq->string_array, array, 0);
  if (i < 0)
    return 1;

  clear_string_array(eq->string_array.buf[array].data.str, eq->string_array.buf[array].num);
  eq->string_array.buf[array].num = 0;

  return 0;
}

int
math_equation_set_array_val(MathEquation *eq, int array, int index, const MathValue *val)
{
  int i;

  if (eq == NULL) {
    return 1;
  }
  i = check_array(&eq->array, array, index);
  if (i < 0)
    return 1;

  eq->array.buf[array].data.val[i] = *val;

  return 0;
}

int
math_equation_push_array_val(MathEquation *eq, int array, const MathValue *val)
{
  MathEquationArray *ary;

  ary = math_equation_get_array(eq, array);
  if (ary == NULL) {
    return 1;
  }

  return math_equation_set_array_val(eq, array, ary->num, val);
}

int
math_equation_get_array_val(MathEquation *eq, int array, int index, MathValue *val)
{
  int i;

  if (eq == NULL) {
    return 1;
  }
  i = check_array(&eq->array, array, index);
  if (i < 0)
    return 1;

  if (val) {
    *val = eq->array.buf[array].data.val[i];
  }

  return 0;
}

MathValue *
math_equation_get_array_ptr(MathEquation *eq, int array, int index)
{
  int i;

  if (eq == NULL) {
    return NULL;
  }
  i = check_array(&eq->array, array, index);
  if (i < 0)
    return NULL;

  return eq->array.buf[array].data.val + i;
}

int
math_equation_set_array_str(MathEquation *eq, int array, int index, const char *str)
{
  int i;

  if (str == NULL || eq == NULL) {
    return 1;
  }
  i = check_array(&eq->string_array, array, index);
  if (i < 0)
    return 1;

  g_string_assign(eq->string_array.buf[array].data.str[i], str);

  return 0;
}

int
math_equation_push_array_str(MathEquation *eq, int array, const char *str)
{
  MathEquationArray *ary;

  ary = math_equation_get_array(eq, array);
  if (ary == NULL) {
    return 1;
  }

  return math_equation_set_array_str(eq, array, ary->num, str);
}

GString *
math_equation_get_array_str(MathEquation *eq, int array, int index)
{
  int i;

  if (eq == NULL) {
    return NULL;
  }
  i = check_array(&eq->string_array, array, index);
  if (i < 0)
    return NULL;

  return eq->string_array.buf[array].data.str[i];
}

const char *
math_equation_get_array_cstr(MathEquation *eq, int array, int index)
{
  GString *str;
  str = math_equation_get_array_str(eq, array, index);
  if (str == NULL) {
    return NULL;
  }

  return str->str;
}

int
math_equation_get_array_common_value(MathEquation *eq, int array, int index, enum DATA_TYPE type, MathCommonValue *val)
{
  switch (type) {
  case DATA_TYPE_VALUE:
    val->type = DATA_TYPE_VALUE;
    return math_equation_get_array_val(eq, array, index, &val->data.val);
    break;
  case DATA_TYPE_STRING:
    val->type = DATA_TYPE_STRING;
    val->data.cstr = math_equation_get_array_cstr(eq, array, index);
    if (val->data.cstr == NULL) {
      return 1;
    }
    break;
  }
  return 0;
}

int
math_equation_set_array_common_value(MathEquation *eq, int array, int index, MathCommonValue *val)
{
  switch (val->type) {
  case DATA_TYPE_VALUE:
    return math_equation_set_array_val(eq, array, index, &val->data.val);
    break;
  case DATA_TYPE_STRING:
    return math_equation_set_array_str(eq, array, index, val->data.cstr);
    break;
  }
  return 0;
}

static MathEquationArray *
math_equation_get_array_common(MathArray *array, int id)
{
  if (id < 0 || id >= array->num || array->buf == NULL) {
    /* error: the array is not exist */
    return NULL;
  }

  return &array->buf[id];
}

MathEquationArray *
math_equation_get_array(MathEquation *eq, int id)
{
  return math_equation_get_array_common(&eq->array, id);
}

MathEquationArray *
math_equation_get_string_array(MathEquation *eq, int id)
{
  return math_equation_get_array_common(&eq->string_array, id);
}

MathEquationArray *
math_equation_get_type_array(MathEquation *eq, enum DATA_TYPE type, int id)
{
  switch (type) {
  case DATA_TYPE_VALUE:
    return math_equation_get_array(eq, id);
    break;
  case DATA_TYPE_STRING:
    return math_equation_get_string_array(eq, id);
    break;
  }
  return NULL;
}

void
math_equation_set_user_data(MathEquation *eq, void *user_data)
{
  if (eq) {
    eq->user_data = user_data;
  }
}

void *
math_equation_get_user_data(MathEquation *eq)
{
  return (eq) ? eq->user_data : NULL;
}

static int
check_const_in_string(MathStringExpression *exp, int *constant, int n)
{
  int i, array_size;
  struct embedded_expression *data;

  if (exp->variables == NULL) {
    return 0;
  }
  array_size = arraynum(exp->variables);
  data = arraydata(exp->variables);
  for (i = 0; i < array_size; i++) {
    int r;
    r = check_const_sub(data[i].exp, constant, n);
    if (r) {
      return r;
    }
  }
  return 0;
}

static int
check_const_sub(MathExpression *exp, int *constant, int n)
{
  int i, r;

  if (exp == NULL)
    return 0;

  r = 0;
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
    r = check_const_sub(exp->u.bin.left, constant, n);
    if (r) {
      return r;
    }
    r = check_const_sub(exp->u.bin.right, constant, n);
    break;
  case MATH_EXPRESSION_TYPE_STRING_ASSIGN:
  case MATH_EXPRESSION_TYPE_ASSIGN:
    r = check_const_sub(exp->u.assign.right, constant, n);
    break;
  case MATH_EXPRESSION_TYPE_FUNC:
    r = check_const_sub(exp->u.func.exp, constant, n);
    break;
  case MATH_EXPRESSION_TYPE_FUNC_CALL:
    for (i = 0; i < exp->u.func_call.argc; i++) {
      r = check_const_sub(exp->u.func_call.argv[i], constant, n);
      if (r) {
	return r;
      }
    }
    break;
  case MATH_EXPRESSION_TYPE_MINUS:
  case MATH_EXPRESSION_TYPE_FACT:
    r = check_const_sub(exp->u.unary.operand, constant, n);
    break;
  case MATH_EXPRESSION_TYPE_CONST:
    for (i = 0; i < n; i++) {
      if (constant[i] == exp->u.index) {
	return 1;
      }
    }
    break;
  case MATH_EXPRESSION_TYPE_ARRAY:
  case MATH_EXPRESSION_TYPE_STRING_ARRAY:
    r = check_const_sub(exp->u.array.operand, constant, n);
    break;
  case MATH_EXPRESSION_TYPE_CONST_DEF:
  case MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT:
  case MATH_EXPRESSION_TYPE_STRING_ARRAY_ARGUMENT:
  case MATH_EXPRESSION_TYPE_VARIABLE:
  case MATH_EXPRESSION_TYPE_DOUBLE:
  case MATH_EXPRESSION_TYPE_PRM:
  case MATH_EXPRESSION_TYPE_EOEQ:
  case MATH_EXPRESSION_TYPE_STRING_VARIABLE:
    break;
  case MATH_EXPRESSION_TYPE_STRING:
    r = check_const_in_string(&(exp->u.str), constant, n);
    break;
  case MATH_EXPRESSION_TYPE_BLOCK:
    r = check_const_in_expression_list(exp->u.exp, constant, n);
    break;
  }

  return r;
}

struct search_const {
  int *constant, n, r;
};

static int
check_counst_in_func(struct nhash *hash, void *ptr)
{
  int r;
  struct math_function_parameter *fprm;
  struct search_const *sc;

  fprm = (struct math_function_parameter *) hash->val.p;
  sc = (struct search_const *) ptr;

  r = check_const_sub(fprm->base_usr, sc->constant, sc->n);
  sc->r = r;

  return r;
}

static int
check_const_in_expression_list(MathExpression *exp, int *constant, int n)
{
  int r;
  r = 0;
  while (exp) {
    r = check_const_sub(exp, constant, n);
    if (r) {
      break;
    }
    exp = exp->next;
  }
  return r;
}

int
math_equation_check_const(MathEquation *eq, int *constant, int n)
{
  int r;
  struct search_const sc;

  if (eq == NULL || constant == NULL)
    return 0;

  sc.constant = constant;
  sc.n = n;
  sc.r = 0;
  nhash_each(eq->function, check_counst_in_func, &sc);

  if (sc.r) {
    return sc.r;
  }

  r = check_const_in_expression_list(eq->exp, constant, n);
  return r;
}

const char *
math_special_value_to_string(MathValue *val)
{
  char *str;
  if (val == NULL) {
    return NULL;
  }
  str = NULL;
  switch (val->type) {
  case MATH_VALUE_NORMAL:
    str = NULL;
    break;
  case MATH_VALUE_ERROR:
    str = "ERROR";
    break;
  case MATH_VALUE_NAN:
    str = "NAN";
    break;
  case MATH_VALUE_UNDEF:
    str = "UNDEF";
    break;
  case MATH_VALUE_CONT:
    str = "CONTINUE";
    break;
  case MATH_VALUE_BREAK:
    str = "BREAK";
    break;
  case MATH_VALUE_NONUM:
    str = "NO-NUMBER";
    break;
  case MATH_VALUE_MEOF:
    str = "EOF";
    break;
  case MATH_VALUE_INTERRUPT:
    str = "INTERRUPTED";
    break;
  }
  return str;
}

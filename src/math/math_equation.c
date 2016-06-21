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

static void init_parameter(MathEquation *eq);
static void free_parameter(MathEquation *eq);
static void free_func(NHASH func);
static void optimize_func(NHASH func);
static int optimize_const_definition(MathEquation *eq);
static int math_equation_call_user_func(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);

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
  buf[*num].data = NULL;

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

MathEquation *
math_equation_new(void)
{
  MathEquation *eq;

  eq = g_malloc(sizeof(*eq));
  if (eq == NULL)
    return NULL;

  memset(eq, 0, sizeof(*eq));

  eq->constant = nhash_new();
  eq->function = nhash_new();
  eq->variable = nhash_new();
  eq->array = nhash_new();
  eq->local_array = nhash_new();
  eq->local_variable = nhash_new();

  if (eq->function == NULL ||
      eq->constant == NULL ||
      eq->variable == NULL ||
      eq->array == NULL ||
      eq->local_variable == NULL ||
      eq->local_array == NULL) {
    math_equation_free(eq);
    return NULL;
  }


  return eq;
}

static void
clear_pos_func_buf(MathEquation *eq)
{
  int i;

  if (eq->pos_func_buf) {
    for (i = 0; i < eq->pos_func_num; i++) {
      eq->pos_func_buf[i].val = 0;
      eq->pos_func_buf[i].type = MATH_VALUE_UNDEF;
    }
  }
}

void
math_equation_clear(MathEquation *eq)
{
  int i;

  if (eq == NULL)
    return;

  clear_pos_func_buf(eq);

  if (eq->vnum > 0 && eq->vbuf) {
    memset(eq->vbuf, 0, sizeof(*eq->vbuf) * eq->vnum);
  }

  if (eq->array_num > 0 && eq->array_buf) {
    for (i = 0; i < eq->array_num; i++) {
      if (eq->array_buf[i].num > 0 && eq->array_buf[i].data) {
	memset(eq->array_buf[i].data, 0, sizeof(MathEquationArray) * eq->array_buf[i].num);
      }
    }
  }
}

void
math_equation_set_parse_error(MathEquation *eq, const char *ptr)
{
  if (eq == NULL)
    return;

  eq->err_info.pos = ptr;
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
    g_free(buf[i].data);
  }

  g_free(buf);
}

void
math_equation_free(MathEquation *eq)
{
  if (eq == NULL)
    return;

  if (eq->constant)
    nhash_free(eq->constant);

  if (eq->variable)
    nhash_free(eq->variable);

  if (eq->function)
    free_func(eq->function);

  if (eq->array)
    nhash_free(eq->array);

  if (eq->local_array)
    nhash_free(eq->local_array);

  if (eq->local_variable)
    nhash_free(eq->local_variable);

  if (eq->const_def) {
    math_expression_free(eq->const_def);
  }

  free_array_buf(eq->array_buf, eq->array_num);

  free_parameter(eq);

  math_expression_free(eq->exp);
  math_expression_free(eq->opt_exp);
  g_free(eq->cbuf);
  g_free(eq->vbuf);
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
      (eq->vnum > 0 && eq->vbuf == NULL)) {
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
      (eq->vnum > 0 && eq->vbuf == NULL)) {
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
  int i;

  if (ptr->id == NULL) {
    ptr->id = add_to_ary_int(ptr->id, &ptr->id_num, val);
  } else {
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

  eq->local_vnum = 0;
  eq->local_array_num = 0;
  eq->func_def = 1;

  return fprm;
}

int
math_equation_register_user_func_definition(MathEquation *eq, const char *name, MathExpression *exp)
{
  struct math_function_parameter *fprm;
  int r;

  if (eq == NULL || ! eq->func_def || name == NULL || exp == NULL)
    return 1;

  fprm = math_equation_get_func(eq, name);
  if (fprm == NULL) {
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

int
math_equation_finish_user_func_definition(MathEquation *eq, int *vnum, int *anum)
{
  if (eq == NULL || ! eq->func_def)
    return 1;

  if (vnum)
    *vnum = nhash_num(eq->local_variable);

  nhash_clear(eq->local_variable);

  if (anum)
    *anum = nhash_num(eq->local_array);

  nhash_clear(eq->local_array);

  eq->local_vnum = 0;
  eq->local_array_num = 0;
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
    ptr->arg_type = g_malloc(sizeof(*prm->arg_type) * prm->argc);
    if (ptr->arg_type == NULL) {
      g_free(ptr->name);
      g_free(ptr);
      return NULL;
    }
    memcpy(ptr->arg_type, prm->arg_type, sizeof(*prm->arg_type) * prm->argc);
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


static int
expand_stack(MathEquation *eq, int size)
{
  MathValue *ptr;
  int n, request_size;

  request_size = eq->stack_end + size;

  if (eq->vbuf_size <= request_size) {
    n = (request_size / BUF_UNIT + 1) * BUF_UNIT;
    ptr = g_realloc(eq->vbuf, sizeof(*ptr) * n);
    if (ptr == NULL)
      return 1;
    eq->vbuf = ptr;

    eq->vbuf_size = n;
  }

  if (eq->vbuf == NULL)
    return 1;

  memset(eq->vbuf + eq->stack_end, 0, sizeof(*eq->vbuf) * size);

  eq->stack_ofst = eq->stack_end;
  eq->stack_end = request_size;

  return 0;
}


int
math_equation_add_var(MathEquation *eq, const char *name)
{
  int i, r;

  if (eq == NULL)
    return -1;

  if (eq->func_def) {
    r = nhash_get_int(eq->local_variable, name, &i);
    if (r) {
      i = eq->local_vnum;
      nhash_set_int(eq->local_variable, name, i);
      eq->local_vnum++;
    }
    return i;
  }

  r = nhash_get_int(eq->variable, name, &i);
  if (r) {
    i = eq->vnum;
    if (expand_stack(eq, 1)) {
      /* error: cannot allocate enough memory */
      return -1;
    }

    eq->stack_ofst = 0;
    eq->vnum++;
    nhash_set_int(eq->variable, name, i);
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

  nhash_each(eq->variable, search_val_cb, &v);

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
math_equation_set_var(MathEquation *eq, int idx, const MathValue *val)
{
  if (eq->vbuf == NULL || idx + eq->stack_ofst >= eq->stack_end) {
    return 1;
  }

  eq->vbuf[idx + eq->stack_ofst] = *val;

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

  *val = eq->cbuf[idx];

  return 0;
}

#define USER_FUNC_NEST_MAX 8192

static int
math_equation_call_user_func(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  static int nest = 0;
  int ofst, end, r, i, j, prev_num;
  MathEquationArray *prev, *local = NULL;
  MathExpression *func;
  MathFunctionArgument *argv;

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

  ofst = eq->stack_ofst;
  end = eq->stack_end;
  prev = eq->array_buf;
  prev_num = eq->array_num;

  if (func->u.func.local_array_num > 0) {
    local = g_malloc(sizeof(*local) * func->u.func.local_array_num);
    if (local == NULL)
      return 1;

    memset(local, 0, sizeof(*local) * func->u.func.local_array_num);

    if (func->u.func.fprm->arg_type) {
      j = 0;
      for (i = 0; i < func->u.func.argc; i++) {
	if (func->u.func.fprm->arg_type[i] == MATH_FUNCTION_ARG_TYPE_ARRAY) {
	  local[j] = prev[argv[i].idx];
	  j++;
	}
      }
    }
  }

  if (expand_stack(eq, func->u.func.local_num)) {
    g_free(local);
    return 1;
  }

  eq->array_buf = local;
  eq->array_num = func->u.func.local_array_num;

  j = 0;
  for (i = 0; i < exp->argc; i++) {
    if (func->u.func.fprm->arg_type == NULL || func->u.func.fprm->arg_type[i] == MATH_FUNCTION_ARG_TYPE_DOUBLE) {
      eq->vbuf[eq->stack_ofst + j] = argv[i].val;
      j++;
    }
  }

  nest++;
  r = math_expression_calculate(func->u.func.exp, rval);
  nest--;

  eq->array_num = prev_num;
  eq->array_buf = prev;
  eq->stack_end = end;
  eq->stack_ofst = ofst;

  if (func->u.func.fprm->arg_type) {
    j = 0;
    for (i = 0; i < func->u.func.argc; i++) {
      if (func->u.func.fprm->arg_type[i] == MATH_FUNCTION_ARG_TYPE_ARRAY) {
	prev[argv[i].idx] = local[j];
	local[j].num = 0;
	local[j].size = 0;
	local[j].data = NULL;
	j++;
      }
    }
  }
  free_array_buf(local, func->u.func.local_array_num);

  return r;
}

int
math_equation_check_var(MathEquation *eq, const char *name)
{
  int r, i;

  if (eq->func_def) {
    r = nhash_get_int(eq->local_variable, name, &i);
  } else {
    r = nhash_get_int(eq->variable, name, &i);
  }

  if (r) {
    return -1;
  }

  return i;
}

int
math_equation_get_var(MathEquation *eq, int idx, MathValue *val)
{
  if (eq->vbuf == NULL || idx + eq->stack_ofst >= eq->stack_end) {
    return 1;
  }

  if (val) {
    *val = eq->vbuf[idx + eq->stack_ofst];
  }

  return 0;
}

void
math_equation_clear_variable(MathEquation *eq)
{
  if (eq->vbuf) {
    g_free(eq->vbuf);
    eq->vnum = 0;
    eq->vbuf = NULL;
  }

  nhash_clear(eq->variable);
}

char *
math_equation_get_array_name(MathEquation *eq, int index)
{
  return NULL;
}

int
math_equation_check_array(MathEquation *eq, const char *name)
{
  int r, i;

  if (eq->func_def) {
    r = nhash_get_int(eq->local_array, name, &i);
  } else {
    r = nhash_get_int(eq->array, name, &i);
  }

  if (r) {
    return -1;
  }

  return i;
}

int
math_equation_add_array(MathEquation *eq, const char *name)
{
  int i, r;

  if (eq == NULL)
    return -1;

  if (eq->func_def) {
    r = nhash_get_int(eq->local_array, name, &i);
    if (r) {
      i = eq->local_array_num;
      if (nhash_set_int(eq->local_array, name, i)) {
	/* error: cannot allocate enough memory */
	return -1;
      }
      eq->local_array_num++;
    }
  } else {
    r = nhash_get_int(eq->array, name, &i);
    if (r) {
      i = eq->array_num;
      eq->array_buf = add_to_ary_array(eq->array_buf, &eq->array_num);

      if (eq->array == NULL) {
	/* error: cannot allocate enough memory */
	return -1;
      }

      if (nhash_set_int(eq->array, name, i)) {
	/* error: cannot allocate enough memory */
	return -1;
      }
    }
  }

  return i;
}

static int
check_array(MathEquation *eq, int id, int index)
{
  int i;
  MathEquationArray *ary;
  MathValue *ptr;

  if (eq == NULL || eq->array_buf == NULL || id < 0 || id >= eq->array_num) {
    /* error: the array is not exist */
    return -1;
  }

  ary = eq->array_buf + id;

  if (index < 0) {
    i = ary->num + index;
  } else {
    i = index;
  }

  if (i < 0 || i > MATH_EQUATION_ARRAY_INDEX_MAX) {
    /* error: the index of the array is out of bound */
    return -1;
  }

  if (i >= ary->size) {
    int n;

    n = (i / BUF_UNIT + 1) * BUF_UNIT;

    ptr = g_realloc(ary->data, sizeof(*ary->data) * n);
    if (ptr == NULL) {
      /* error: cannot allocate enough memory */
      return -1;
    }

    memset(ptr + ary->num, 0, sizeof(*ptr) * (n - ary->num));

    ary->data = ptr;
    ary->size = n;
  }

  if (i >= ary->num) {
    ary->num = i + 1;
  }

  return i;
}

int
math_equation_set_array_val(MathEquation *eq, int array, int index, const MathValue *val)
{
  int i;

  i = check_array(eq, array, index);
  if (i < 0)
    return 1;

  eq->array_buf[array].data[i] = *val;

  return 0;
}

int
math_equation_get_array_val(MathEquation *eq, int array, int index, MathValue *val)
{
  int i;

  i = check_array(eq, array, index);
  if (i < 0)
    return 1;

  *val = eq->array_buf[array].data[i];

  return 0;
}

MathEquationArray *
math_equation_get_array(MathEquation *eq, int array)
{
  if (array < 0 || array >= eq->array_num || eq->array_buf == NULL) {
    /* error: the array is not exist */
    return NULL;
  }

  return &eq->array_buf[array];
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
  case MATH_EXPRESSION_TYPE_CONST_DEF:
  case MATH_EXPRESSION_TYPE_ARRAY:
  case MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT:
  case MATH_EXPRESSION_TYPE_VARIABLE:
  case MATH_EXPRESSION_TYPE_DOUBLE:
  case MATH_EXPRESSION_TYPE_PRM:
  case MATH_EXPRESSION_TYPE_EOEQ:
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


int
math_equation_check_const(MathEquation *eq, int *constant, int n)
{
  int r;
  struct search_const sc;
  MathExpression *exp;

  if (eq == NULL || constant == NULL)
    return 0;

  sc.constant = constant;
  sc.n = n;
  nhash_each(eq->function, check_counst_in_func, &sc);

  if (sc.r) {
   return sc.r;
  }

  r = 0;
  exp = eq->exp;
  while (exp) {
    r = check_const_sub(exp, constant, n);
    if (r)
      break;
    exp = exp->next;
  }

  return r;
}

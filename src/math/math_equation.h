/*
 * $Id: math_equation.h,v 1.8 2009-11-21 11:39:10 hito Exp $
 *
 */

#ifndef MATH_EQUATION_HEADER
#define MATH_EQUATION_HEADER

typedef struct _math_equation MathEquation;
typedef struct _math_parameter MathEquationParametar;
typedef struct _math_array MathEquationArray;
typedef struct _math_value MathValue;

#include "math_error.h"
#include "nhash.h"

#define MATH_EQUATION_ARRAY_INDEX_MAX 65535

struct _math_value {
  double val;
  enum {
    MATH_VALUE_NORMAL = 0,
    MATH_VALUE_ERROR  = 1,
    MATH_VALUE_NAN    = 2,
    MATH_VALUE_UNDEF  = 3,
    //    MATH_VALUE_SYNTAX = 4,
    MATH_VALUE_CONT   = 5,
    MATH_VALUE_BREAK  = 6,
    MATH_VALUE_NONUM  = 7,
    MATH_VALUE_MEOF   = 8,
    MATH_VALUE_INTERRUPT = 9,
  } type;
};

struct _math_array {
  int num, size;
  MathValue *data;
};

#include "math_expression.h"
#include "math_function.h"
#include "math_constant.h"

struct _math_equation {
  NHASH constant, variable, array, function;
  int cnum, vnum, array_num, pos_func_num;
  NHASH local_variable, local_array;
  int local_vnum, local_array_num, func_def;
  MathValue *cbuf, *vbuf, *pos_func_buf;
  int stack_ofst, stack_end, vbuf_size;
  MathExpression *exp, *opt_exp, *const_def;
  MathEquationParametar *parameter;
  MathEquationArray *array_buf;
  union {
    const char *pos;
    struct {
      int arg_num;
      struct math_function_parameter *fprm;
    } func;
    int const_id;
  } err_info;
  void *user_data;
};

struct _math_parameter {
  int type;
  int min_length, max_length;
  int *id, id_num, id_max;
  MathValue *data;
  int use_index;
  struct _math_parameter *next;
};

enum {
  MATH_EQUATION_PARAMETAR_USE_ID,
  MATH_EQUATION_PARAMETAR_USE_INDEX,
};

MathEquation *math_equation_new(void);
MathEquation *math_equation_basic_new(void);
void math_equation_free(MathEquation *eq);

int math_equation_optimize(MathEquation *eq);
int math_equation_parse(MathEquation *eq, const char *str);
void math_equation_clear(MathEquation *eq);
int math_equation_calculate(MathEquation *eq, MathValue *val);
void math_equation_clear_variable(MathEquation *eq);

MathEquationParametar *math_equation_get_parameter(MathEquation *eq, int type, int *err);
int math_equation_use_parameter(MathEquation *eq, int type, int val);
int math_equation_add_parameter(MathEquation *eq, int type, int min, int max, int use_index);
int math_equation_set_parameter_data(MathEquation *eq, int type, MathValue *data);

int math_equation_add_pos_func(MathEquation *eq, struct math_function_parameter *fprm);
struct math_function_parameter *math_equation_start_user_func_definition(MathEquation *eq, const char *name);
int math_equation_register_user_func_definition(MathEquation *eq, const char *name, MathExpression *exp);
int math_equation_finish_user_func_definition(MathEquation *eq, int *vnum, int *anum);
struct math_function_parameter *math_equation_add_func(MathEquation *eq, const char *name, struct math_function_parameter *prm);
struct math_function_parameter *math_equation_get_func(MathEquation *eq, const char *name);
void math_equation_remove_func(MathEquation *eq, const char *name);

int math_equation_add_const_definition(MathEquation *eq, const char *name, MathExpression *exp, int *err);

int math_equation_add_const(MathEquation *eq, const char *name, const MathValue *val);
int math_equation_set_const_by_name(MathEquation *eq, const char *name, const MathValue *val);
int math_equation_set_const(MathEquation *eq, int idx, const MathValue *val);
int math_equation_get_const_by_name(MathEquation *eq, const char *name, MathValue *val);
int math_equation_get_const(MathEquation *eq, int idx, MathValue *val);
char *math_equation_get_const_name(MathEquation *eq, int idx);

int math_equation_add_var(MathEquation *eq, const char *name);
int math_equation_set_var(MathEquation *eq, int idx, const MathValue *val);
int math_equation_check_var(MathEquation *eq, const char *name);
int math_equation_get_var(MathEquation *eq, int idx, MathValue *val);

int math_equation_check_array(MathEquation *eq, const char *name);
int math_equation_add_array(MathEquation *eq, const char *name);
int math_equation_set_array_val(MathEquation *eq, int array, int index, const MathValue *val);
int math_equation_push_array_val(MathEquation *eq, int array, const MathValue *val);
int math_equation_get_array_val(MathEquation *eq, int array, int index, MathValue *val);
int math_equation_clear_array(MathEquation *eq, int array);
MathEquationArray *math_equation_get_array(MathEquation *eq, int array);

void math_equation_set_user_data(MathEquation *eq, void *user_data);
void *math_equation_get_user_data(MathEquation *eq);

int math_equation_check_const(MathEquation *eq, int *constant, int n);

void math_equation_set_parse_error(MathEquation *eq, const char *ptr);
void math_equation_set_func_arg_num_error(MathEquation *eq, struct math_function_parameter *fprm, int arg_num);
void math_equation_set_func_error(MathEquation *eq, struct math_function_parameter *fprm);
void math_equation_set_const_error(MathEquation *eq, int id);


#endif

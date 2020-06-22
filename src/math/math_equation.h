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
typedef struct _math_stack MathStack;
typedef struct _math_array_info MathArray;
typedef struct _math_variable MathVariable;
typedef struct _math_common_value MathCommonValue;

#include <gmodule.h>
#include "math_error.h"
#include "nhash.h"
#include "object.h"

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
  union {
    void *ptr;			/* using for memory manipulation */
    MathValue *val;
    GString **str;
  } data;
};

enum DATA_TYPE {
  DATA_TYPE_VALUE,
  DATA_TYPE_STRING,
};

struct _math_variable {
  enum DATA_TYPE type;
  union {
    MathValue *vptr;
    GString *str;
  } data;
};

struct _math_common_value {
  enum DATA_TYPE type;
  union {
    MathValue val;
    const char *cstr;
  } data;
};

struct _math_array_info {
  enum DATA_TYPE type;
  NHASH array, local_array;
  int num, local_num;
  MathEquationArray *buf;
};

#include "math_scanner.h"
#include "math_expression.h"
#include "math_function.h"
#include "math_constant.h"

struct _math_stack {
  enum DATA_TYPE type;
  NHASH variable;
  int num, ofst, end, size;
  NHASH local_variable;
  int local_num;
  int element_size;
  union {
    void *ptr;			/* using for memory manipulation */
    MathValue *val;
    GString **str;
  } stack;
};

struct _math_equation {
  MathStack stack, string_stack;
  MathArray array, string_array;
  NHASH constant, function;
  int cnum, pos_func_num;
  int func_def;
  MathValue *cbuf, *pos_func_buf;
  MathExpression *exp, *opt_exp, *const_def;
  MathEquationParametar *parameter;
  enum EOEQ_ASSIGN_TYPE {
		       EOEQ_ASSIGN_TYPE_BOTH,
		       EOEQ_ASSIGN_TYPE_EOEQ,
		       EOEQ_ASSIGN_TYPE_ASSIGN,
  } eoeq_assign_type;
  int use_eoeq_assign;
  struct narray *scope_info;
  struct {
    struct {
      const char *pos;
      int line, ofst;
    } pos;
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
int math_equation_finish_user_func_definition(MathEquation *eq, int *vnum, int *anum, int *str_anum, int *snum);
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
MathValue *math_equation_get_var_ptr(MathEquation *eq, int idx);
int math_equation_check_string_var(MathEquation *eq, const char *name);
int math_equation_get_string_var(MathEquation *eq, int idx, GString **str);

int math_equation_add_var_string(MathEquation *eq, const char *name);
int math_equation_set_var_string(MathEquation *eq, int idx, const char *str);

int math_equation_check_array(MathEquation *eq, const char *name);
int math_equation_check_string_array(MathEquation *eq, const char *name);
int math_equation_add_array(MathEquation *eq, const char *name, int is_string);
int math_equation_set_array_val(MathEquation *eq, int array, int index, const MathValue *val);
int math_equation_push_array_val(MathEquation *eq, int array, const MathValue *val);
int math_equation_get_array_val(MathEquation *eq, int array, int index, MathValue *val);
MathValue * math_equation_get_array_ptr(MathEquation *eq, int array, int index);
int math_equation_clear_array(MathEquation *eq, int array);
int math_equation_clear_string_array(MathEquation *eq, int array);
MathEquationArray *math_equation_get_array(MathEquation *eq, int array);
MathEquationArray *math_equation_get_string_array(MathEquation *eq, int id);
MathEquationArray *math_equation_get_type_array(MathEquation *eq, enum DATA_TYPE type, int id);

void math_equation_set_user_data(MathEquation *eq, void *user_data);
void *math_equation_get_user_data(MathEquation *eq);

void math_equation_set_eoeq_assign_type(MathEquation *eq, enum EOEQ_ASSIGN_TYPE type);

int math_equation_check_const(MathEquation *eq, int *constant, int n);

void math_equation_set_parse_error(MathEquation *eq, const char *ptr, const struct math_string *str);
void math_equation_set_func_arg_num_error(MathEquation *eq, struct math_function_parameter *fprm, int arg_num);
void math_equation_set_func_error(MathEquation *eq, struct math_function_parameter *fprm);
void math_equation_set_const_error(MathEquation *eq, int id);

int math_equation_set_array_str(MathEquation *eq, int array, int index, const char *str);
int math_equation_push_array_str(MathEquation *eq, int array, const char *str);
GString *math_equation_get_array_str(MathEquation *eq, int array, int index);
const char *math_equation_get_array_cstr(MathEquation *eq, int array, int index);

int math_equation_get_array_common_value(MathEquation *eq, int array, int index, enum DATA_TYPE type, MathCommonValue *val);
int math_equation_set_array_common_value(MathEquation *eq, int array, int index, MathCommonValue *val);

int math_equation_pop_array(MathEquation *eq, int array, int type);
int math_equation_unshift_array_val(MathEquation *eq, int array, const MathValue *val);
int math_equation_unshift_array_str(MathEquation *eq, int array, const char *cstr);

const char *math_special_value_to_string(MathValue *val);

#endif

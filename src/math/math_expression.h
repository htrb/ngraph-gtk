/*
 * $Id: math_expression.h,v 1.3 2009-11-10 04:12:20 hito Exp $
 *
 */

#ifndef EXPRESSION_HEADER
#define EXPRESSION_HEADER

typedef struct _math_expression MathExpression;
typedef struct _math_function_call_expression MathFunctionCallExpression;
typedef union _math_function_argument MathFunctionArgument;

#include "nhash.h"
#include "object.h"
#include "math_equation.h"
#include "math_function.h"
#include "math_operator.h"

enum MATH_EXPRESSION_TYPE {
  MATH_EXPRESSION_TYPE_OR,
  MATH_EXPRESSION_TYPE_AND,
  MATH_EXPRESSION_TYPE_EQ,
  MATH_EXPRESSION_TYPE_NE,
  MATH_EXPRESSION_TYPE_ADD,
  MATH_EXPRESSION_TYPE_SUB,
  MATH_EXPRESSION_TYPE_MUL,
  MATH_EXPRESSION_TYPE_DIV,
  MATH_EXPRESSION_TYPE_MOD,
  MATH_EXPRESSION_TYPE_POW,
  MATH_EXPRESSION_TYPE_GT,
  MATH_EXPRESSION_TYPE_GE,
  MATH_EXPRESSION_TYPE_LT,
  MATH_EXPRESSION_TYPE_LE,
  MATH_EXPRESSION_TYPE_MINUS,
  MATH_EXPRESSION_TYPE_FUNC,
  MATH_EXPRESSION_TYPE_FUNC_CALL,
  MATH_EXPRESSION_TYPE_FACT,
  MATH_EXPRESSION_TYPE_DOUBLE,
  MATH_EXPRESSION_TYPE_CONST,
  MATH_EXPRESSION_TYPE_CONST_DEF,
  MATH_EXPRESSION_TYPE_VARIABLE,
  MATH_EXPRESSION_TYPE_ARRAY,
  MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT,
  MATH_EXPRESSION_TYPE_PRM,
  MATH_EXPRESSION_TYPE_ASSIGN,
  MATH_EXPRESSION_TYPE_STRING,
  MATH_EXPRESSION_TYPE_STRING_VARIABLE,
  MATH_EXPRESSION_TYPE_STRING_ASSIGN,
  MATH_EXPRESSION_TYPE_STRING_ARRAY,
  MATH_EXPRESSION_TYPE_STRING_ARRAY_ARGUMENT,
  MATH_EXPRESSION_TYPE_BLOCK,
  MATH_EXPRESSION_TYPE_EOEQ,
};

struct math_func_arg_list {
  char *name;
  enum MATH_FUNCTION_ARG_TYPE type;
  struct math_func_arg_list *next;
};

typedef struct _math_function_expression {
  int argc, local_num, local_string_num, local_array_num, local_string_array_num;
  MathExpression *exp;
  struct math_function_parameter *fprm;
  struct math_func_arg_list *arg_list, *arg_last;
} MathFunctionExpression ;

typedef struct _math_binary_expression {
  MathExpression  *left;
  MathExpression  *right;
} MathBinaryExpression;

typedef struct _math_assign_expression {
  enum MATH_OPERATOR_TYPE op;
  MathExpression  *left;
  MathExpression  *right;
} MathAssignExpression;

typedef struct _math_unary_expression {
  MathExpression  *operand;
} MathUnaryExpression;

typedef struct _math_constant_definition_expression {
  int id;
  MathExpression  *operand;
} MathConstantDefinitionExpression;

union _math_function_argument {
  MathValue val;
  struct math_variable_argument_common {
    enum DATA_TYPE type;
    union math_variable_argument_data {
      MathValue *vptr;
      GString *str;
    } data;
  } variable;
  MathExpression *exp;
  struct math_array_argument_common {
    enum DATA_TYPE array_type;
    int idx;
  } array;
  const char *cstr;
};

struct _math_function_call_expression {
  MathExpression **argv;
  MathFunctionArgument *buf;
  int argc, pos_id;
  struct math_function_parameter *fprm;
};

typedef struct _math_parameter_expression {
  int type, id, index;
  MathEquationParametar *prm;
} MathParameterExpression;

typedef struct _math_array_expression {
  int index;
  MathExpression *operand;
} MathArrayExpression;

struct embedded_variable {
  int start, end;
  char *variable;
};

typedef struct _math_string_expression {
  char *string;
  GString *expanded;
  struct narray *variables;
} MathStringExpression;

struct _math_expression {
  enum MATH_EXPRESSION_TYPE type;
  int line_number;
  MathExpression *next;
  MathEquation *equation;
  union {
    MathValue value;
    char *symbol;
    char *string;
    int index;
    MathBinaryExpression bin;
    MathUnaryExpression unary;
    MathFunctionCallExpression func_call;
    MathFunctionExpression func;
    MathParameterExpression prm;
    MathArrayExpression array;
    MathConstantDefinitionExpression const_def;
    MathAssignExpression assign;
    MathStringExpression str;
    MathExpression *exp;
  } u;
};

MathExpression *math_expression_new(enum MATH_EXPRESSION_TYPE type, MathEquation *eq, int *err);
MathExpression *math_eoeq_expression_new(MathEquation *eq, int *err);
MathExpression *math_array_expression_new(MathEquation *eq, const char *name, MathExpression *operand, int is_string, int *err);
MathExpression *math_array_argument_expression_new(MathEquation *eq, const char *name, int *err);
MathExpression *math_string_array_argument_expression_new(MathEquation *eq, const char *name, int *err);
MathExpression *math_unary_expression_new(enum MATH_EXPRESSION_TYPE type, MathEquation *eq, MathExpression *operand, int *err);
MathExpression *math_binary_expression_new(enum MATH_EXPRESSION_TYPE type, MathEquation *eq, MathExpression *left, MathExpression *right, int *err);
MathExpression *math_assign_expression_new(enum MATH_EXPRESSION_TYPE type, MathEquation *eq, MathExpression *left, MathExpression *right, enum MATH_OPERATOR_TYPE op, int *err);
MathExpression *math_double_expression_new(MathEquation *eq, const MathValue *val, int *err);
MathExpression *math_constant_expression_new(MathEquation *eq, const char *name, int *err);
MathExpression *math_variable_expression_new(MathEquation *eq, const char *name, int *err);
MathExpression *math_func_call_expression_new(MathEquation *eq, struct math_function_parameter *fprm,
					      int argc, MathExpression **argv, int pos_id, int *err);
MathExpression *math_parameter_expression_new(MathEquation *eq, char *name, int *err);
MathExpression *math_constant_definition_expression_new(MathEquation *eq, char *name, MathExpression *exp, int *err);
MathExpression *math_function_expression_new(MathEquation *eq, const char *name, int *err);
MathExpression *math_string_expression_new(MathEquation *eq, const char *str, int *err);
MathExpression *math_string_variable_expression_new(MathEquation *eq, const char *str, int *err);

int math_function_expression_add_arg(MathExpression *func, const char *arg_name, enum MATH_FUNCTION_ARG_TYPE type);
int math_function_expression_set_function(MathEquation *eq, MathExpression *func, const char *name, MathExpression *exp);
int math_function_expression_register_arg(MathExpression *func);

MathExpression *math_expression_optimize(MathExpression *exp, int *err);
void math_expression_free(MathExpression *exp);
int math_expression_calculate(MathExpression *exp, MathValue *val);

int math_function_get_arg_type_num(struct math_function_parameter *fprm);

const char *math_expression_get_string_from_argument(MathFunctionCallExpression *exp, int i);
GString *math_expression_get_string_variable_from_argument(MathFunctionCallExpression *exp, int i);
MathValue *math_expression_get_variable_from_argument(MathFunctionCallExpression *exp, int i);
enum DATA_TYPE math_expression_get_variable_type_from_argument(MathFunctionCallExpression *exp, int i);

int math_function_call_expression_get_variable(MathFunctionCallExpression *exp, int i, MathVariable *var);
int math_variable_set_common_value(MathVariable *variable, MathCommonValue *val);

#endif

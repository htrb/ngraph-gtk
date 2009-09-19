#ifndef EXPRESSION_HEADER
#define EXPRESSION_HEADER

typedef struct _math_expression MathExpression;
typedef struct _math_function_call_expression MathFunctionCallExpression;
typedef union _math_function_argument MathFunctionArgument;

#include "nhash.h"
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
  MATH_EXPRESSION_TYPE_DOBLE,
  MATH_EXPRESSION_TYPE_CONST,
  MATH_EXPRESSION_TYPE_CONST_DEF,
  MATH_EXPRESSION_TYPE_VARIABLE,
  MATH_EXPRESSION_TYPE_ARRAY,
  MATH_EXPRESSION_TYPE_ARRAY_ARGUMENT,
  MATH_EXPRESSION_TYPE_PRM,
  MATH_EXPRESSION_TYPE_ASSIGN,
  MATH_EXPRESSION_TYPE_EOEQ,
};

struct math_func_arg_list {
  char *name;
  enum MATH_FUNCTION_ARG_TYPE type;
  struct math_func_arg_list *next;
};

typedef struct _math_function_expression {
  int argc, local_num, local_array_num;
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
  MathExpression *exp;
  int idx;
} function_argument;

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

struct _math_expression {
  enum MATH_EXPRESSION_TYPE type;
  int line_number;
  MathExpression *next;
  MathEquation *equation;
  union {
    MathValue value;
    char *symbol;
    int index;
    MathBinaryExpression bin;
    MathUnaryExpression unary;
    MathFunctionCallExpression func_call;
    MathFunctionExpression func;
    MathParameterExpression prm;
    MathArrayExpression array;
    MathConstantDefinitionExpression const_def;
    MathAssignExpression assign;
  } u;
};

MathExpression *math_expression_new(enum MATH_EXPRESSION_TYPE type, MathEquation *eq);
MathExpression *math_eoeq_expression_new(MathEquation *eq);
MathExpression *math_array_expression_new(MathEquation *eq, const char *name, MathExpression *operand);
MathExpression *math_array_argument_expression_new(MathEquation *eq, const char *name);
MathExpression *math_unary_expression_new(enum MATH_EXPRESSION_TYPE type, MathEquation *eq, MathExpression *operand);
MathExpression *math_binary_expression_new(enum MATH_EXPRESSION_TYPE type, MathEquation *eq, MathExpression *left, MathExpression *right);
MathExpression *math_assign_expression_new(enum MATH_EXPRESSION_TYPE type, MathEquation *eq, MathExpression *left, MathExpression *right, enum MATH_OPERATOR_TYPE op);
MathExpression *math_double_expression_new(MathEquation *eq, const MathValue *val);
MathExpression *math_constant_expression_new(MathEquation *eq, const char *name);
MathExpression *math_variable_expression_new(MathEquation *eq, const char *name);
MathExpression *math_func_call_expression_new(MathEquation *eq, struct math_function_parameter *fprm, int argc, MathExpression **argv, int pos_id);
MathExpression *math_parameter_expression_new(MathEquation *eq, char *name);
MathExpression *math_constant_definition_expression_new(MathEquation *eq, char *name, MathExpression *exp);
MathExpression *math_function_expression_new(MathEquation *eq, const char *name);

int math_function_expression_add_arg(MathExpression *func, const char *arg_name, enum MATH_FUNCTION_ARG_TYPE type);
int math_function_expression_set_function(MathEquation *eq, MathExpression *func, const char *name, MathExpression *exp);
int math_function_expression_register_arg(MathExpression *func);

MathExpression *math_expression_optimize(MathExpression *exp);
void math_expression_free(MathExpression *exp);
int math_expression_calculate(MathExpression *exp, MathValue *val);

#endif

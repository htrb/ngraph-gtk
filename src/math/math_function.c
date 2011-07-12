#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "math_expression.h"
#include "math_equation.h"
#include "math_function.h"

struct funcs {
  char *name;
  struct math_function_parameter prm;
};

static struct funcs FuncAry[] = {
#ifdef HAVE_LIBGSL
  {"ZETAM1_INT", {1, 0, 0, math_func_zetam1_int, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
  {"MJD2MONTH", {1, 0, 0, math_func_mjd2month, NULL, NULL, NULL, NULL}},
  {"MJD2YEAR", {1, 0, 0, math_func_mjd2year, NULL, NULL, NULL, NULL}},
  {"MJD2WDAY", {1, 0, 0, math_func_mjd2wday, NULL, NULL, NULL, NULL}},
  {"UNIX2MJD", {1, 0, 0, math_func_unix2mjd, NULL, NULL, NULL, NULL}},
#ifdef HAVE_LIBGSL
  {"ZETA_INT", {1, 0, 0, math_func_zeta_int, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
  {"ISNORMAL", {1, 0, 0, math_func_isnormal, NULL, NULL, NULL, NULL}},
  {"MJD2YDAY", {1, 0, 0, math_func_mjd2yday, NULL, NULL, NULL, NULL}},
  {"UNSHIFT", {2, 0, 0, math_func_unshift, NULL, NULL, NULL, NULL}},
  {"ISUNDEF", {1, 0, 0, math_func_isundef, NULL, NULL, NULL, NULL}},
  {"MJD2DAY", {1, 0, 0, math_func_mjd2day, NULL, NULL, NULL, NULL}},
  {"ISBREAK", {1, 0, 0, math_func_isbreak, NULL, NULL, NULL, NULL}},
  {"UNLESS", {3, 0, 0, math_func_unless, NULL, NULL, NULL, NULL}},
#ifdef HAVE_LIBGSL
  {"ICBETA", {3, 0, 0, math_func_icbeta, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
#ifdef HAVE_LIBGSL
  {"ZETAM1", {1, 0, 0, math_func_zetam1, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
  {"ISCONT", {1, 0, 0, math_func_iscont, NULL, NULL, NULL, NULL}},
  {"ATANH", {1, 0, 0, math_func_atanh, NULL, NULL, NULL, NULL}},
  {"ISNAN", {1, 0, 0, math_func_isnan, NULL, NULL, NULL, NULL}},
  {"SHIFT", {1, 0, 0, math_func_shift, NULL, NULL, NULL, NULL}},
  {"GAUSS", {1, 0, 0, math_func_gauss, NULL, NULL, NULL, NULL}},
  {"ROUND", {1, 0, 0, math_func_round, NULL, NULL, NULL, NULL}},
  {"RSORT", {1, 1, 0, math_func_rsort, NULL, NULL, NULL, NULL}},
  {"ASINH", {1, 0, 0, math_func_asinh, NULL, NULL, NULL, NULL}},
  {"ACOSH", {1, 0, 0, math_func_acosh, NULL, NULL, NULL, NULL}},
  {"PROG2", {-1, 0, 0, math_func_prog2, NULL, NULL, NULL, NULL}},
  {"PROG1", {-1, 0, 0, math_func_prog1, NULL, NULL, NULL, NULL}},
  {"SRAND", {1, 0, 0, math_func_srand, NULL, NULL, NULL, NULL}},
  {"THETA", {1, 0, 0, math_func_theta, NULL, NULL, NULL, NULL}},
  {"DELTA", {1, 0, 0, math_func_delta, NULL, NULL, NULL, NULL}},
  {"GAMMA", {1, 0, 0, math_func_gamma, NULL, NULL, NULL, NULL}},
  {"ICGAM", {2, 0, 0, math_func_icgam, NULL, NULL, NULL, NULL}},
  {"PROGN", {-1, 0, 0, math_func_progn, NULL, NULL, NULL, NULL}},
  {"QINV", {1, 0, 0, math_func_qinv, NULL, NULL, NULL, NULL}},
  {"ERFC", {1, 0, 0, math_func_erfc, NULL, NULL, NULL, NULL}},
#ifdef HAVE_LIBGSL
  {"ZETA", {1, 0, 0, math_func_zeta, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
  {"BETA", {2, 0, 0, math_func_beta, NULL, NULL, NULL, NULL}},
  {"RAND", {1, 0, 0, math_func_rand, NULL, NULL, NULL, NULL}},
  {"SQRT", {1, 0, 0, math_func_sqrt, NULL, NULL, NULL, NULL}},
  {"ASIN", {1, 0, 0, math_func_asin, NULL, NULL, NULL, NULL}},
  {"PUSH", {2, 0, 0, math_func_push, NULL, NULL, NULL, NULL}},
  {"SIZE", {1, 1, 0, math_func_size, NULL, NULL, NULL, NULL}},
  {"TANH", {1, 0, 0, math_func_tanh, NULL, NULL, NULL, NULL}},
  {"SORT", {1, 1, 0, math_func_sort, NULL, NULL, NULL, NULL}},
  {"COSH", {1, 0, 0, math_func_cosh, NULL, NULL, NULL, NULL}},
  {"FRAC", {1, 0, 0, math_func_frac, NULL, NULL, NULL, NULL}},
  {"SINH", {1, 0, 0, math_func_sinh, NULL, NULL, NULL, NULL}},
  {"ATAN", {1, 0, 0, math_func_atan, NULL, NULL, NULL, NULL}},
  {"SIGN", {1, 0, 0, math_func_sign, NULL, NULL, NULL, NULL}},
  {"ACOS", {1, 0, 0, math_func_acos, NULL, NULL, NULL, NULL}},
  {"TIME", {0, 0, 0, math_func_time, NULL, NULL, NULL, NULL}},
  {"ABS", {1, 0, 0, math_func_abs, NULL, NULL, NULL, NULL}},
  {"NEQ", {3, 0, 0, math_func_neq, NULL, NULL, NULL, NULL}},
  {"INT", {1, 0, 0, math_func_int, NULL, NULL, NULL, NULL}},
  {"MIN", {-1, 0, 0, math_func_min, NULL, NULL, NULL, NULL}},
  {"POP", {1, 0, 0, math_func_pop, NULL, NULL, NULL, NULL}},
  {"MAX", {-1, 0, 0, math_func_max, NULL, NULL, NULL, NULL}},
  {"NOT", {1, 0, 0, math_func_not, NULL, NULL, NULL, NULL}},
  {"SQR", {1, 0, 0, math_func_sqr, NULL, NULL, NULL, NULL}},
  {"AND", {2, 0, 0, math_func_and, NULL, NULL, NULL, NULL}},
  {"XOR", {2, 0, 0, math_func_xor, NULL, NULL, NULL, NULL}},
  {"EXP", {1, 0, 0, math_func_exp, NULL, NULL, NULL, NULL}},
#ifdef HAVE_LIBGSL
  {"KNU", {2, 0, 0, math_func_knu, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
#ifdef HAVE_LIBGSL
  {"INU", {2, 0, 0, math_func_inu, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
#ifdef HAVE_LIBGSL
  {"YNU", {2, 0, 0, math_func_ynu, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
  {"MJD", {6, 0, 0, math_func_mjd, NULL, NULL, NULL, NULL}},
  {"FOR", {5, 1, 0, math_func_for, NULL, NULL, NULL, NULL}},
  {"DIF", {1, 1, 1, math_func_dif, NULL, NULL, NULL, NULL}},
  {"SUM", {1, 1, 1, math_func_sum, NULL, NULL, NULL, NULL}},
#ifdef HAVE_LIBGSL
  {"JNU", {2, 0, 0, math_func_jnu, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
  {"LOG", {1, 0, 0, math_func_log, NULL, NULL, NULL, NULL}},
  {"LGN", {3, 0, 0, math_func_lgn, NULL, NULL, NULL, NULL}},
  {"SIN", {1, 0, 0, math_func_sin, NULL, NULL, NULL, NULL}},
  {"COS", {1, 0, 0, math_func_cos, NULL, NULL, NULL, NULL}},
  {"TAN", {1, 0, 0, math_func_tan, NULL, NULL, NULL, NULL}},
#ifdef HAVE_LIBGSL
  {"ERF", {1, 0, 0, math_func_erf, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
  {"JN", {2, 0, 0, math_func_jn, NULL, NULL, NULL, NULL}},
  {"EI", {1, 0, 0, math_func_ei, NULL, NULL, NULL, NULL}},
  {"YN", {2, 0, 0, math_func_yn, NULL, NULL, NULL, NULL}},
  {"PN", {2, 0, 0, math_func_pn, NULL, NULL, NULL, NULL}},
  {"HN", {2, 0, 0, math_func_hn, NULL, NULL, NULL, NULL}},
#ifdef HAVE_LIBGSL
  {"IN", {2, 0, 0, math_func_in, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
#ifdef HAVE_LIBGSL
  {"KN", {2, 0, 0, math_func_kn, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
#ifdef HAVE_LIBGSL
  {"YL", {2, 0, 0, math_func_yl, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
#ifdef HAVE_LIBGSL
  {"JL", {2, 0, 0, math_func_jl, NULL, NULL, NULL, NULL}},
#else
  {NULL, {0, 0, 0, NULL, NULL, NULL, NULL, NULL}},
#endif
  {"TN", {2, 0, 0, math_func_tn, NULL, NULL, NULL, NULL}},
  {"IF", {3, 0, 0, math_func_if, NULL, NULL, NULL, NULL}},
  {"GE", {2, 0, 0, math_func_ge, NULL, NULL, NULL, NULL}},
  {"RM", {1, 1, 0, math_func_rm, NULL, NULL, NULL, NULL}},
  {"LN", {1, 0, 0, math_func_ln, NULL, NULL, NULL, NULL}},
  {"EQ", {3, 0, 0, math_func_eq, NULL, NULL, NULL, NULL}},
  {"OR", {2, 0, 0, math_func_or, NULL, NULL, NULL, NULL}},
  {"LT", {2, 0, 0, math_func_lt, NULL, NULL, NULL, NULL}},
  {"LE", {2, 0, 0, math_func_le, NULL, NULL, NULL, NULL}},
  {"GT", {2, 0, 0, math_func_gt, NULL, NULL, NULL, NULL}},
  {"CM", {1, 0, 0, math_func_cm, NULL, NULL, NULL, NULL}},
  {"M", {2, 1, 0, math_func_m, NULL, NULL, NULL, NULL}},
};


int
math_add_basic_function(MathEquation *eq) {
  unsigned int i;
  enum MATH_FUNCTION_ARG_TYPE *ptr;

  for (i = 0; i < sizeof(FuncAry) / sizeof(*FuncAry); i++) {
    if (FuncAry[i].name == NULL) {
      continue;
    }
    switch (i) {
    case 8:  /*  UNSHIFT  */
      if (FuncAry[i].prm.arg_type) {
        break;
      }
      ptr = g_malloc(sizeof(enum MATH_FUNCTION_ARG_TYPE) * 2);
      if (ptr == NULL) {
        return 1;
      }
      ptr[0] = MATH_FUNCTION_ARG_TYPE_ARRAY;
      ptr[1] = MATH_FUNCTION_ARG_TYPE_DOUBLE;
      FuncAry[i].prm.arg_type = ptr;
      break;
    case 12:  /*  UNLESS  */
      if (FuncAry[i].prm.arg_type) {
        break;
      }
      ptr = g_malloc(sizeof(enum MATH_FUNCTION_ARG_TYPE) * 3);
      if (ptr == NULL) {
        return 1;
      }
      ptr[0] = MATH_FUNCTION_ARG_TYPE_DOUBLE;
      ptr[1] = MATH_FUNCTION_ARG_TYPE_PROC;
      ptr[2] = MATH_FUNCTION_ARG_TYPE_PROC;
      FuncAry[i].prm.arg_type = ptr;
      break;
    case 18:  /*  SHIFT  */
      if (FuncAry[i].prm.arg_type) {
        break;
      }
      ptr = g_malloc(sizeof(enum MATH_FUNCTION_ARG_TYPE) * 1);
      if (ptr == NULL) {
        return 1;
      }
      ptr[0] = MATH_FUNCTION_ARG_TYPE_ARRAY;
      FuncAry[i].prm.arg_type = ptr;
      break;
    case 21:  /*  RSORT  */
      if (FuncAry[i].prm.arg_type) {
        break;
      }
      ptr = g_malloc(sizeof(enum MATH_FUNCTION_ARG_TYPE) * 1);
      if (ptr == NULL) {
        return 1;
      }
      ptr[0] = MATH_FUNCTION_ARG_TYPE_ARRAY;
      FuncAry[i].prm.arg_type = ptr;
      break;
    case 39:  /*  PUSH  */
      if (FuncAry[i].prm.arg_type) {
        break;
      }
      ptr = g_malloc(sizeof(enum MATH_FUNCTION_ARG_TYPE) * 2);
      if (ptr == NULL) {
        return 1;
      }
      ptr[0] = MATH_FUNCTION_ARG_TYPE_ARRAY;
      ptr[1] = MATH_FUNCTION_ARG_TYPE_DOUBLE;
      FuncAry[i].prm.arg_type = ptr;
      break;
    case 40:  /*  SIZE  */
      if (FuncAry[i].prm.arg_type) {
        break;
      }
      ptr = g_malloc(sizeof(enum MATH_FUNCTION_ARG_TYPE) * 1);
      if (ptr == NULL) {
        return 1;
      }
      ptr[0] = MATH_FUNCTION_ARG_TYPE_ARRAY;
      FuncAry[i].prm.arg_type = ptr;
      break;
    case 42:  /*  SORT  */
      if (FuncAry[i].prm.arg_type) {
        break;
      }
      ptr = g_malloc(sizeof(enum MATH_FUNCTION_ARG_TYPE) * 1);
      if (ptr == NULL) {
        return 1;
      }
      ptr[0] = MATH_FUNCTION_ARG_TYPE_ARRAY;
      FuncAry[i].prm.arg_type = ptr;
      break;
    case 54:  /*  POP  */
      if (FuncAry[i].prm.arg_type) {
        break;
      }
      ptr = g_malloc(sizeof(enum MATH_FUNCTION_ARG_TYPE) * 1);
      if (ptr == NULL) {
        return 1;
      }
      ptr[0] = MATH_FUNCTION_ARG_TYPE_ARRAY;
      FuncAry[i].prm.arg_type = ptr;
      break;
    case 65:  /*  FOR  */
      if (FuncAry[i].prm.arg_type) {
        break;
      }
      ptr = g_malloc(sizeof(enum MATH_FUNCTION_ARG_TYPE) * 5);
      if (ptr == NULL) {
        return 1;
      }
      ptr[0] = MATH_FUNCTION_ARG_TYPE_DOUBLE;
      ptr[1] = MATH_FUNCTION_ARG_TYPE_DOUBLE;
      ptr[2] = MATH_FUNCTION_ARG_TYPE_DOUBLE;
      ptr[3] = MATH_FUNCTION_ARG_TYPE_DOUBLE;
      ptr[4] = MATH_FUNCTION_ARG_TYPE_PROC;
      FuncAry[i].prm.arg_type = ptr;
      break;
    case 85:  /*  IF  */
      if (FuncAry[i].prm.arg_type) {
        break;
      }
      ptr = g_malloc(sizeof(enum MATH_FUNCTION_ARG_TYPE) * 3);
      if (ptr == NULL) {
        return 1;
      }
      ptr[0] = MATH_FUNCTION_ARG_TYPE_DOUBLE;
      ptr[1] = MATH_FUNCTION_ARG_TYPE_PROC;
      ptr[2] = MATH_FUNCTION_ARG_TYPE_PROC;
      FuncAry[i].prm.arg_type = ptr;
      break;
    }
    if (math_equation_add_func(eq, FuncAry[i].name, &FuncAry[i].prm) == NULL)
      return 1;
  }
  return 0;
}

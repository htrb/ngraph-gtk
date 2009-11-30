#ifndef MATH_SCANNER_FUNC_HEADER
#define MATH_SCANNER_FUNC_HEADER

enum MATH_FUNCTION_ARG_TYPE {
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_ARRAY,
  MATH_FUNCTION_ARG_TYPE_PROC,
};

typedef int (* math_function) (MathFunctionCallExpression *exp, MathEquation *eq, MathValue *r);

struct math_function_parameter {
  int argc;
  int side_effect, positional;
  math_function func;
  enum MATH_FUNCTION_ARG_TYPE *arg_type;
  MathExpression *opt_usr, *base_usr;
  char *name;
};

int math_scanner_is_func(int chr);
int math_add_basic_function(MathEquation *eq);

#ifdef HAVE_LIBGSL
int math_func_zetam1_int(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
#ifdef HAVE_LIBGSL
int math_func_zeta_int(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
int math_func_isnormal(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_isundef(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_isbreak(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#ifdef HAVE_LIBGSL
int math_func_icbeta(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
int math_func_unless(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#ifdef HAVE_LIBGSL
int math_func_zetam1(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
int math_func_iscont(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_delta(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_gamma(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_round(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_icgam(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_isnan(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_gauss(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_theta(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_atanh(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_acosh(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_asinh(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_rsort(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_atan(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_tanh(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_sinh(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_acos(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_asin(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_rand(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_size(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_sqrt(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#ifdef HAVE_LIBGSL
int math_func_zeta(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
int math_func_frac(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_sort(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_erfc(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_qinv(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_sign(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_beta(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_cosh(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_abs(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#ifdef HAVE_LIBGSL
int math_func_knu(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
#ifdef HAVE_LIBGSL
int math_func_inu(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
int math_func_lgn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#ifdef HAVE_LIBGSL
int math_func_ynu(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
#ifdef HAVE_LIBGSL
int math_func_jnu(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
int math_func_mjd(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_int(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_neq(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_min(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_max(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_sqr(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_exp(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_not(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_log(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_and(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_xor(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_sin(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_cos(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_tan(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#ifdef HAVE_LIBGSL
int math_func_erf(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
int math_func_for(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_sum(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_dif(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_ei(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_jn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_if(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_rm(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_yn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_or(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_ln(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_lt(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_le(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_gt(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_pn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_eq(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#ifdef HAVE_LIBGSL
int math_func_in(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
#ifdef HAVE_LIBGSL
int math_func_kn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
#ifdef HAVE_LIBGSL
int math_func_yl(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
#ifdef HAVE_LIBGSL
int math_func_jl(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif
int math_func_tn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_hn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_ge(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
int math_func_m(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval);
#endif

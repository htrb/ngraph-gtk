/*
 * $Id: math_basic_function.c,v 1.14 2010-03-04 08:30:17 hito Exp $
 *
 */

#include "config.h"

#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>
#include <glib.h>

#ifdef HAVE_LIBGSL
#include <gsl/gsl_sf.h>
#include <gsl/gsl_rng.h>
#include <gsl/gsl_math.h>
#endif

#include "ntime.h"
#include "odata.h"
#include "nstring.h"
#include "object.h"

#include "math_equation.h"
#include "math_function.h"

#define MPI 3.14159265358979323846
#define MEXP1 2.71828182845905
#define MEULER 0.57721566490153286

#define MATH_CHECK_ARG(rval, v) if (v.val.type != MATH_VALUE_NORMAL) {	\
    *rval = v.val;							\
    return 0;								\
  }


#define MATH_FUNCTION_MEMORY_NUM 65536
static MathValue *Memory = NULL;

static int compare_double_with_prec(long double a, long double b, int prec);
static int set_common_array(MathEquation *eq, int id, int i, enum DATA_TYPE type, MathVariable *variable);
static int get_common_array(MathFunctionCallExpression *exp, MathEquation *eq, int ary_arg, int var_arg, int *id, enum DATA_TYPE *type, MathVariable *variable, MathEquationArray **src);

int
math_func_time(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  rval->val = time(NULL);
  return 0;
}

int
math_func_atanh(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double v;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  v = exp->buf[0].val.val;
  if (v > 1) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = atanh(v);
  return 0;
}

int
math_func_asinh(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = asinh(exp->buf[0].val.val);
  return 0;
}

int
math_func_acosh(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double v;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  v = exp->buf[0].val.val;
  if (v < 1) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = acosh(v);
  return 0;
}

int
math_func_gauss(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int val;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  if (exp->buf[0].val.val >= 0) {
    val = (int) (exp->buf[0].val.val);
  } else if (exp->buf[0].val.val - (int) exp->buf[0].val.val == 0) {
    val = (int) exp->buf[0].val.val;
  } else {
    val = (int) exp->buf[0].val.val - 1;
  }

  rval->val = (double) val;
  return 0;
}

int
math_func_round(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
#if HAVE_ROUNDL
  long double val, prec;
#else
  double val, prec;
#endif
  int digit;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  val = exp->buf[0].val.val;
  digit = exp->buf[1].val.val;
  prec = powl(10, digit);

  val *= prec;
#if HAVE_ROUNDL
  val = roundl(val);
#else
  if (fabs(val - (int) val) >= 0.5) {
    if (val >= 0) {
      val = val - (val - (int) val) + 1;
    } else {
      val = val - (val - (int) val) - 1;
    }
  } else {
    val = val - (val - (int) val);
  }
#endif
  val /= prec;

  rval->val = val;
  return 0;
}

int
math_func_tanh(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = tanh(exp->buf[0].val.val);
  return 0;
}

int
math_func_sign(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = (exp->buf[0].val.val < 0) ? -1 : 1;
  return 0;
}

int
math_func_frac(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = exp->buf[0].val.val - (int) exp->buf[0].val.val;
  return 0;
}

int
math_func_cosh(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = cosh(exp->buf[0].val.val);
  return 0;
}

int
math_func_sqrt(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double v;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  v = exp->buf[0].val.val;
  if (v < 0) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = sqrt(v);
  return 0;
}

int
math_func_sinh(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = sinh(exp->buf[0].val.val);
  return 0;
}

int
math_func_atan(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = atan(exp->buf[0].val.val);
  return 0;
}

int
math_func_acos(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double v;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  v = exp->buf[0].val.val;
  if (v < -1 || v > 1) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = acos(v);
  return 0;
}

int
math_func_asin(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double v;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  v = exp->buf[0].val.val;
  if (v < -1 || v > 1) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = asin(v);
  return 0;
}

int
math_func_log(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double v;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  v = exp->buf[0].val.val;
  if (v <= 0) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  rval->val = log10(v);
  return 0;
}

int
math_func_log1p(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double v;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  v = exp->buf[0].val.val;
  if (v <= -1) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
#if HAVE_LOG1P
  rval->val = log1p(v) * M_LOG10E;
#else
  rval->val = log(1 + v) * M_LOG10E;
#endif
  return 0;
}

int
math_func_tan(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = tan(exp->buf[0].val.val);
  return 0;
}

int
math_func_sin(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = sin(exp->buf[0].val.val);
  return 0;
}

int
math_func_cos(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = cos(exp->buf[0].val.val);
  return 0;
}

int
math_func_abs(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = fabs(exp->buf[0].val.val);
  return 0;
}

int
math_func_exp(MathFunctionCallExpression *expl, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, expl->buf[0]);

  rval->val = exp(expl->buf[0].val.val);
  return 0;
}

int
math_func_int(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = (int) exp->buf[0].val.val;
  return 0;
}

int
math_func_max(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double max;
  int i;

  if (exp->argc < 1) {
    rval->val = 0;
    return 0;
  }

  max = exp->buf[0].val.val;
  for (i = 1; i < exp->argc; i++) {
    MATH_CHECK_ARG(rval, exp->buf[i]);
    if (exp->buf[i].val.val > max) {
      max = exp->buf[i].val.val;
    }
  }

  rval->val = max;
  return 0;
}

int
math_func_min(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double min;
  int i;

  if (exp->argc < 1) {
    rval->val = 0;
    return 0;
  }

  min = exp->buf[0].val.val;
  for (i = 1; i < exp->argc; i++) {
    MATH_CHECK_ARG(rval, exp->buf[i]);
    if (exp->buf[i].val.val < min) {
      min = exp->buf[i].val.val;
    }
  }

  rval->val = min;
  return 0;
}

int
math_func_sumsq(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double sum;
  int i;

  if (exp->argc < 1) {
    rval->val = 0;
    return 0;
  }

  sum = 0;
  for (i = 0; i < exp->argc; i++) {
    double val;
    MATH_CHECK_ARG(rval, exp->buf[i]);
    val = exp->buf[i].val.val;
    sum += val * val;
  }

  rval->val = sum;
  return 0;
}

int
math_func_progn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  if (exp->argc < 1) {
    rval->val = 0;
  } else {
    *rval = exp->buf[exp->argc - 1].val;
  }

  return 0;
}

int
math_func_prog2(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  if (exp->argc < 2) {
    rval->val = 0;
  } else {
    *rval = exp->buf[1].val;
  }

  return 0;
}

int
math_func_prog1(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  if (exp->argc < 1) {
    rval->val = 0;
  } else {
    *rval = exp->buf[0].val;
  }

  return 0;
}

int
math_func_sqr(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val =  exp->buf[0].val.val * exp->buf[0].val.val;
  return 0;
}

int
math_func_ln(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double v;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  v = exp->buf[0].val.val;
  if (v <= 0) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  rval->val = log(v);
  return 0;
}

int
math_func_theta(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = (exp->buf[0].val.val >= 0) ? 1 : 0;
  return 0;
}

int
math_func_delta(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val =  (exp->buf[0].val.val == 0) ? 1 : 0;
  return 0;
}

#ifndef HAVE_LIBGSL
static int
gamma2(double x, double *val)
{
  double m, s, x1, xx;
  unsigned int i;
  static double gammacoe[] = {
    -0.42278433509846714,
    -0.23309373642178674,
    0.19109110138769151,
    -0.24552490005400017E-1,
    -0.17645244550144320E-1,
    0.80232730222673465E-2,
    -0.80432977560424699E-3,
    -0.3608378162548E-3,
    0.1455961421399E-3,
    -0.175458597517E-4,
    -0.25889950224E-5,
    0.13385015466E-5,
    -0.2054743152E-6,
    -0.1595268E-9,
    0.62756218E-8,
    -0.12736143E-8,
    0.923397E-10,
    0.120028E-10,
    -0.42202E-11
  };

  if (((x <= 0) && ((x - (int) x) == 0)) || (x > 57))
    return -1;
  m = 1;
  if (1.5 < x)
    while (1.5 < x) {
      x -= 1;
      m *= x;
  } else if (x < 0.5)
    while (x < 0.5) {
      if (x <= 1E-300)
	return -1;
      m /= x;
      x++;
    }
  s = 1;
  xx = 1;
  x1 = x - 1;
  for (i = 0; i < sizeof(gammacoe) / sizeof(*gammacoe); i++) {
    xx *= x1;
    s += xx * gammacoe[i];
  }
  s *= x;
  if (s == 0)
    return -1;
  *val = m / s;
  return 0;
}

static int
exp1(double x, double *val)
{
  double x2, xx, xx2, qexp1, qexp2;
  int i, n;
  static double xo[18] = {
    3.3,   4.0,
    4.8,   5.8,
    7.1,   8.6,
    10.4, 12.5,
    15.2, 18.4,
    22.2, 26.9,
    32.5, 39.4,
    47.6, 57.6,
    69.7, 83.0
  };
  static double eipt[18] = {
    44.85377247567375, 35.95520078636207,
    28.55548567959244, 22.28304130069319,
    17.15083762120765, 13.52650411391756,
    10.81282973538746, 8.781902021178544,
    7.084719123528884, 5.769115302038587,
    4.728746710237675, 3.867300609441438,
    3.178040354772841, 2.606037129493851,
    2.146957943513479, 1.767357146269297,
    1.455922099180754, 1.219698244176420
  };
  static double eimt[18] = {
    24.23610327385172, 20.63456499010558,
    17.65538999222755, 14.96789725060241,
    12.50401823871386, 10.51365383249826,
    8.830924437781962, 7.443527518977505,
    6.194083826443568, 5.167187241655524,
    4.317774043309917, 3.588549395492134,
    2.987594411636582, 2.476696474779806,
    2.058451487317135, 1.706965828524471,
    1.414702600059552, 1.190641095161813
  };

  x2 = fabs(x);
  if (x == 0 || x2 > 174) {
    return -1;
  }

  if (x2 >= 90) {
    n = 1;
    xx = 1;
    qexp2 = 1;
    do {
      xx *= n / x;
      n++;
      qexp1 = qexp2;
      qexp2 = qexp1 + xx;
    } while (qexp1 != qexp2);
    qexp1 /= x;
    if (x < 0) {
      qexp1 = -qexp1 * exp(-x2);
    } else {
      qexp1 *= exp(x2);
    }
  } else if (x2 >= 3) {
    i = 0;
    while ((i < 18) && (x2 >= 0.5 * (xo[i] + xo[i + 1]))) {
      i++;
    }
    if (x > 0) {
      xx = eipt[i] / 100;
    } else{
      xx = eimt[i] / 100;
    }
    xx2 = 1 / (-xo[i]);
    qexp2 = xx;
    n = 1;
    do {
      qexp1 = qexp2;
      xx = (xo[i] - x2) / n * (xx + xx2);
      if (x < 0) {
	xx = -xx;
      }
      qexp2 = qexp1 + xx;
      xx2 *= (x2 - xo[i]) / (-xo[i]);
      n++;
    } while (qexp2 != qexp1);
    if (x < 0) {
      qexp1 = -qexp1 * exp(-x2);
    } else {
      qexp1 *= exp(x2);
    }
  } else {
    n = 1;
    xx = 1;
    qexp2 = 0;
    do {
      xx *= x / n;
      qexp1 = qexp2;
      qexp2 = qexp1 + xx / n;
      n++;
    } while (qexp1 != qexp2);
    qexp1 += MEULER + log(x2);
  }

  *val = qexp1;
  return 0;
}
#endif

int
math_func_icgam(MathFunctionCallExpression *expl, MathEquation *eq, MathValue *rval)
{
  double mu, x;
#ifdef HAVE_LIBGSL
  int r;
  gsl_sf_result val;
#else
  double a, u, p0, p1, q0, q1, val;
  int i, i2, i3;
#endif

  MATH_CHECK_ARG(rval, expl->buf[0]);
  MATH_CHECK_ARG(rval, expl->buf[1]);

  mu = expl->buf[0].val.val;
  x = expl->buf[1].val.val;

  if (x < 0 || mu < 0) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

#ifdef HAVE_LIBGSL
  r = gsl_sf_gamma_inc_e(mu, x, &val);
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
#else
  if (mu == 0) {
    if (exp1(-x, &val)) {
      rval->type = MATH_VALUE_ERROR;
      return -1;
    } else {
      val = -val;
    }
  } else if (x == 0) {
    if (gamma2(mu, &val)) {
      rval->type = MATH_VALUE_ERROR;
      return -1;
    }
  } else {
    i2 = 0;
    while (mu > 1) {
      i = 0;
      do {
	if (i == 56) {
	  rval->type = MATH_VALUE_ERROR;
	  return -1;
	}
	i++;
	mu--;
      } while (mu > 1);
      i2 = i;
    }
    u = exp(-x + mu * log(x));
    if (x <= 1) {
      if (gamma2(mu, &p1)) {
	rval->type = MATH_VALUE_ERROR;
	return -1;
      }
      p0 = 1;
      q0 = 1;
      i = 0;
      do {
	i++;
	q0 *= x / (mu + i);
	p0 += q0;
      } while ((q0 > 1E-17) && (i != 20));
      val = p1 - u / mu * p0;
    } else {
      i3 = (int) (120 / x + 5);
      p0 = 0;
      if (mu != 1) {
	p1 = 1E-78 / (1 - mu);
      } else {
	p1 = 1E-78;
      }
      q0 = p1;
      q1 = x * q0;
      for (i = 1; i <= i3; i++) {
	a = i - mu;
	p0 = p1 + a * p0;
	q0 = q1 + a * q0;
	p1 = x * p0 + i * p1;
	q1 = x * q0 + i * q1;
	if (i >= 50) {
	  q0 /= i;
	  p0 /= i;
	  q1 /= i;
	  p1 /= i;
	}
      }
      val = u / q1 * p1;
    }
    for (i = 1; i < i2; i++) {
      val = u + mu * val;
      u *= x;
      mu++;
    }
  }

  rval->val = val;
  return 0;
#endif
}

int
math_func_gamma(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
#ifdef HAVE_LIBGSL
  int r;
  gsl_sf_result val;
#endif

  MATH_CHECK_ARG(rval, exp->buf[0]);

#ifdef HAVE_LIBGSL
  r = gsl_sf_gamma_e (exp->buf[0].val.val, &val);
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
#else
  return gamma2(exp->buf[0].val.val, &rval->val);
#endif
}

#ifdef HAVE_LIBGSL
int
math_func_erf(MathFunctionCallExpression *expl, MathEquation *eq, MathValue *rval)
{
  double x;
  int r;
  gsl_sf_result val;

  MATH_CHECK_ARG(rval, expl->buf[0]);

  x = expl->buf[0].val.val;

  r = gsl_sf_erf_e(x, &val);
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}
#endif

int
math_func_erfc(MathFunctionCallExpression *expl, MathEquation *eq, MathValue *rval)
{
  double x;
#ifdef HAVE_LIBGSL
  int r;
  gsl_sf_result val;
#else
  int i, i2, sg;
  double x2, x3, sum, h, h2, val;
#endif

  MATH_CHECK_ARG(rval, expl->buf[0]);

  x = expl->buf[0].val.val;

#ifdef HAVE_LIBGSL
  r = gsl_sf_erfc_e(x, &val);
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
#else
  x2 = fabs(x);
  if (x2 <= 0.1) {
    sum = 0;
    sg = 1;
    x3 = x2;
    for (i = 0; i <= 5; i++) {
      if (i == 0) {
	i2 = 1;
      } else {
	i2 *= i;
      }
      sum += sg * x3 / i2 / (2 * i + 1);
      sg *= -1;
      x3 *= x2 * x2;
    }
    sum *= 2 / sqrt(MPI);
    val = 1 - sum;
  } else if (x2 <= 100) {
    h = 0.5;
    h2 = h * h;
    sum = 0;
    for (i = 1; i <= 13; i++) {
      i2 = i * i;
      sum += exp(-i2 * h2) / (i2 * h2 + x2 * x2);
    }
    if (x2 < 6) {
      sum += 0.5 / (x2 * x2);
      sum *= 2 * x2 / MPI * exp(-x2 * x2) * h;
      sum -= 2 / expm1(2 * MPI * x2 / h);
    } else {
      sum += 0.5 / (x2 * x2);
      sum *= 2 * x2 / MPI * exp(-x2 * x2) * h;
    }
    val = sum;
  } else {
    sum = 1;
    sg = -1;
    x3 = 2 * x2 * x2;
    i2 = 1;
    for (i = 1; i <= 5; i++) {
      i2 *= (2 * i - 1);
      sum += sg * i2 / x3;
      sg *= -1;
      x3 *= 2 * x2 * x2;
    }
    sum *= exp(-x2 * x2) / sqrt(MPI) / x2;
    val = sum;
  }

  if (x < 0) {
    val = 2 - (val);
  }

  rval->val = val;

  return 0;
#endif
}

static double
qinv3(double y)
{
  double n, sum, y2, y3, i;

  sum = 0;
  y2 = y;
  y3 = y * y;
  i = 1;
  n = 0;
  do {
    i *= (2 * n + 1);
    sum += y2 / i;
    n = n + 1;
    y2 *= y3;
  } while (fabs(y2 / i) > 1e-16);

  return sum;
}

static double
qinv2(double y)
{
  int n, i;
  double a;

  if (y < 7.5) {
    n = (int) (110 / (y - 1));
  } else if (y < 12.5) {
    n = (int) (-1.6 * y + 30);
  } else {
    n = 10;
  }
  a = 0;
  for (i = n; i > 0; i--) {
    a = i / (y + a);
  }

  return 1 / (y + a);
}

int
math_func_qinv(MathFunctionCallExpression *expl, MathEquation *eq, MathValue *rval)
{
  double x, x2, m2, y0, y1, val;

  MATH_CHECK_ARG(rval, expl->buf[0]);

  x = expl->buf[0].val.val;

  if (x <= 0 || x >= 1) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  if (x > 0.5) {
    x2 = 1 - x;
  } else {
    x2 = x;
  }

  m2 = sqrt(2 * MPI);
  if (x2 <= 0.01) {
    double c0, c1;
    c0 = sqrt(-2 * log(m2 * 3 * x2));
    c1 = c0 + 1 / c0;
    y1 = sqrt(-2 * log(m2 * c1 * x2));
    do {
      y0 = y1;
      y1 = y0 + (qinv2(y0) - m2 * x2 * exp(0.5 * y0 * y0));
    } while (fabs(y0 - y1) >= 1e-14);
  } else if (x2 <= 0.5) {
    y1 = m2 * (0.5 - x2) * 1.5;
    do {
      y0 = y1;
      y1 = y0 - (qinv3(y0) - m2 * (0.5 - x2) * exp(0.5 * y0 * y0));
    } while (fabs(y0 - y1) >= 1e-14);
  } else {
    /* never reached */
    y1 = 0;
  }

  if (x > 0.5) {
    val = -y1;
  } else {
    val = y1;
  }

  rval->val = val;
  return 0;
}

int
math_func_rand(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = g_random_double() * exp->buf[0].val.val;

  return 0;
}

int
math_func_srand(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  g_random_set_seed(exp->buf[0].val.val);
  rval->val = exp->buf[0].val.val;

  return 0;
}

#ifdef HAVE_LIBGSL
int
math_func_icbeta(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int r;
  gsl_sf_result val;
  double a, b, x;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);
  MATH_CHECK_ARG(rval, exp->buf[2]);

  a = exp->buf[0].val.val;
  b = exp->buf[1].val.val;
  x = exp->buf[2].val.val;

  if (x < 0 || x > 1) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  r =  gsl_sf_beta_inc_e(a, b, x, &val);

  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}

int
math_func_zeta(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int r;
  gsl_sf_result val;
  double n;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  n = exp->buf[0].val.val;

  if (n == 1) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  r =  gsl_sf_zeta_e(n, &val);

  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}

int
math_func_zetam1(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int r;
  gsl_sf_result val;
  double n;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  n = exp->buf[0].val.val;

  if (n == 1) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  r =  gsl_sf_zetam1_e(n, &val);

  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}

int
math_func_zeta_int(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int r;
  gsl_sf_result val;
  int n;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  n = exp->buf[0].val.val;

  if (n == 1) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  r =  gsl_sf_zeta_int_e(n, &val);

  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}

int
math_func_zetam1_int(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int r;
  gsl_sf_result val;
  int n;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  n = exp->buf[0].val.val;

  if (n == 1) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  r =  gsl_sf_zetam1_int_e(n, &val);

  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}
#endif

int
math_func_beta(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double p, q;
#ifdef HAVE_LIBGSL
  int r;
  gsl_sf_result val;
#else
  double a, b, c;
#endif

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  p = exp->buf[0].val.val;
  q = exp->buf[1].val.val;

#ifdef HAVE_LIBGSL
  r =  gsl_sf_beta_e(p, q, &val);
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
#else

  if (gamma2(p, &a)) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  if (gamma2(q, &b)) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  if (gamma2(p + q, &c)) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  if (fabs(c) < 1E-300) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  rval->val = a * b / c;

  return 0;
#endif
}

int
math_func_lgn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double x, alp;
  int n;
#ifdef HAVE_LIBGSL
  int r;
  gsl_sf_result val;
#else
  double l1, val;
#endif

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);
  MATH_CHECK_ARG(rval, exp->buf[2]);

  n = exp->buf[0].val.val;
  alp = exp->buf[1].val.val;
  x = exp->buf[2].val.val;

  if (n < 0) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

#ifdef HAVE_LIBGSL
  if (n == 1) {
    r = gsl_sf_laguerre_1_e(alp, x, &val);
  } else if (n == 2) {
    r = gsl_sf_laguerre_2_e(alp, x, &val);
  } else if (n == 3) {
    r = gsl_sf_laguerre_3_e(alp, x, &val);
  } else {
    r = gsl_sf_laguerre_n_e(n, alp, x, &val);
  }
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
#else
  l1 = 1;
  if (n == 0) {
    val = l1;
  } else {
    double l2;
    l2 = 1 - x + alp;
    if (n == 1) {
      val = l2;
    } else {
      double tmp1, tmp2;
      int i;
      tmp1 = 1 + x - alp;
      tmp2 = 1 - alp;
      for (i = 2; i <= n; i++) {
	val = (l2 - l1) + l2 - (tmp1 * l2 - tmp2 * l1) / i;
	l1 = l2;
	l2 = val;
      }
    }
  }

  rval->val = val;

  return 0;
#endif
}

static double
mjd(int y, int m, int d, int hh, int mm, int ss)
{
  int d0, d1, d2, d3, d4;

  if (m < 1) {
    m = 1;
  }

  if (d < 1) {
    d = 1;
  }

  d0 = (14 - m) / 12;
  d1 = (y - d0) * 365.25;
  d2 = (m + d0 * 12 - 2) * 30.59;
  d3 = (y - d0) / 100;
  d4 = (y - d0) / 400;

  return  d1 + d2 - d3 + d4 + d + 1721088 - 2400000 + hh / 24.0 + mm / 1440.0  + ss / 86400.0;
}

int
math_func_mjd(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);
  MATH_CHECK_ARG(rval, exp->buf[2]);
  MATH_CHECK_ARG(rval, exp->buf[3]);
  MATH_CHECK_ARG(rval, exp->buf[4]);
  MATH_CHECK_ARG(rval, exp->buf[5]);


  rval->val = mjd(exp->buf[0].val.val,
		  exp->buf[1].val.val,
		  exp->buf[2].val.val,
		  exp->buf[3].val.val,
		  exp->buf[4].val.val,
		  exp->buf[5].val.val);

  return 0;
}

int
math_func_unix2mjd(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct tm *tm;
  time_t t;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  t = exp->buf[0].val.val;

  tm = gmtime(&t);
  if (tm == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = mjd(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);

  return 0;
}

int
math_func_mjd2unix(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = (exp->buf[0].val.val - 40587) * 86400;

  return 0;
}

int
math_func_mjd2year(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct tm t;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  mjd2gd(exp->buf[0].val.val, &t);

  if (t.tm_year < MJD2GD_YEAR_MIN) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = t.tm_year + 1900;
  return 0;
}

int
math_func_mjd2month(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct tm t;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  mjd2gd(exp->buf[0].val.val, &t);

  if (t.tm_year < MJD2GD_YEAR_MIN) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = t.tm_mon + 1;
  return 0;
}

int
math_func_mjd2day(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct tm t;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  mjd2gd(exp->buf[0].val.val, &t);

  if (t.tm_year < MJD2GD_YEAR_MIN) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = t.tm_mday;
  return 0;
}

int
math_func_mjd2wday(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct tm t;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  mjd2gd(exp->buf[0].val.val, &t);

  if (t.tm_year < MJD2GD_YEAR_MIN) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = (t.tm_wday == 0) ? 7 : t.tm_wday;
  return 0;
}

int
math_func_mjd2yday(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct tm t;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  mjd2gd(exp->buf[0].val.val, &t);

  if (t.tm_year < MJD2GD_YEAR_MIN) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = t.tm_yday + 1;
  return 0;
}

int
math_func_xor(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  rval->val = ((exp->buf[0].val.val != 0) ^ (exp->buf[1].val.val != 0)) ? 1 : 0;
  return 0;
}

int
math_func_and(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  rval->val = (exp->buf[0].val.val && exp->buf[1].val.val);
  return 0;
}

int
math_func_not(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  rval->val = ! exp->buf[0].val.val;
  return 0;
}

int
math_func_lt(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int prec, r;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);
  MATH_CHECK_ARG(rval, exp->buf[2]);

  prec = exp->buf[2].val.val;

  r = compare_double_with_prec(exp->buf[0].val.val, exp->buf[1].val.val, prec);
  rval->val = (r < 0) ? 1 : 0;

  return 0;
}

int
math_func_le(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int prec, r;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);
  MATH_CHECK_ARG(rval, exp->buf[2]);

  prec = exp->buf[2].val.val;

  r = compare_double_with_prec(exp->buf[0].val.val, exp->buf[1].val.val, prec);
  rval->val = (r <= 0) ? 1 : 0;

  return 0;
}

int
math_func_or(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  rval->val = (exp->buf[0].val.val || exp->buf[1].val.val);
  return 0;
}

int
math_func_ge(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int prec, r;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);
  MATH_CHECK_ARG(rval, exp->buf[2]);

  prec = exp->buf[2].val.val;

  r = compare_double_with_prec(exp->buf[0].val.val, exp->buf[1].val.val, prec);
  rval->val = (r >= 0) ? 1 : 0;

  return 0;
}

static int
compare_double_with_prec(long double a, long double b, int prec)
{
#ifndef HAVE_LIBGSL
  long double scale, r, dif;
  int order, order_a, order_b;
#endif

  if (a == b) {
    return 0;
  }

  if (prec < 1 || prec > 34) {	/* long double (IEEE754-2008 binary128) */
    return (a > b) ? 1 : -1;
  }
#ifdef HAVE_LIBGSL
  return gsl_fcmp(a, b, pow(10, -prec));
#else
  if (a == 0.0 || b == 0.0) {
    return (a > b) ? 1 : -1;
  }

  if ((a > 0 && b < 0) || (a < 0 && b > 0)) {
    return (a > b) ? 1 : -1;
  }

#if HAVE_FLOORL && HAVE_LOG10L && HAVE_FABSL
  order_a = floorl(log10l(fabsl(a)));
  order_b = floorl(log10l(fabsl(b)));
#else  /* HAVE_FLOORL && HAVE_LOG10L && HAVE_FABSL */
  order_a = floor(log10(fabs(a)));
  order_b = floor(log10(fabs(b)));
#endif

  if (order_a - order_b > 1) {
    return 1;
  } else if (order_a - order_b < -1) {
    return -1;
  }

  order = (order_a > order_b) ? order_a : order_b;

#if HAVE_POWL
  scale = powl(10, - order);
#else  /* HAVE_POWL */
  scale = pow(10, - order);
#endif
  dif = a *scale - b * scale;

#if HAVE_POWL
  scale = powl(10, prec - 1);
#else  /* HAVE_POWL */
  scale = pow(10, prec - 1);
#endif

  r = dif * scale;

  if (r >= 0.5) {
    return 1;
  } else if (r <= -0.5) {
    return -1;
  }

  return 0;
#endif
}

int
math_func_eq(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int prec, r;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);
  MATH_CHECK_ARG(rval, exp->buf[2]);

  prec = exp->buf[2].val.val;

  r = compare_double_with_prec(exp->buf[0].val.val, exp->buf[1].val.val, prec);
  rval->val = (r == 0) ? 1 : 0;

  return 0;
}

int
math_func_neq(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int prec, r;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);
  MATH_CHECK_ARG(rval, exp->buf[2]);

  prec = exp->buf[2].val.val;

  r = compare_double_with_prec(exp->buf[0].val.val, exp->buf[1].val.val, prec);
  rval->val = (r != 0) ? 1 : 0;

  return 0;
}

int
math_func_tn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double l1, x, val;
  int n;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  if (n < 0) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  l1 = 1;
  if (n == 0) {
    val = l1;
  } else {
    double l2;
    l2 = x;
    if (n == 1) {
      val = l2;
    } else {
      int i;
      double tmp1;
      tmp1 = x + x;
      for (i = 2; i <= n; i++) {
	val = tmp1 * l2 - l1;
	l1 = l2;
	l2 = val;
      }
    }
  }

  rval->val = val;

  return 0;
}

int
math_func_hn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double l1, val, x;
  int n;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  if (n < 0) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  l1 = 1;
  if (n == 0) {
    val = l1;
  } else {
    double l2;
    l2 = 2 * x;
    if (n == 1) {
      val = l2;
    } else {
      int i;
      for (i = 2; i <= n; i++) {
	val = x * l2 - (i - 1) * l1;
	val += val;
	l1 = l2;
	l2 = val;
      }
    }
  }

  rval->val = val;

  return 0;
}

int
math_func_gt(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int prec, r;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);
  MATH_CHECK_ARG(rval, exp->buf[2]);

  prec = exp->buf[2].val.val;

  r = compare_double_with_prec(exp->buf[0].val.val, exp->buf[1].val.val, prec);
  rval->val = (r > 0) ? 1 : 0;

  return 0;
}

int
math_func_if(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n;

  n = (exp->buf[0].val.type == MATH_VALUE_NORMAL && exp->buf[0].val.val) ? 1 : 2;
  return math_expression_calculate(exp->buf[n].exp, rval);
}

int
math_func_unless(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n;

  n = (exp->buf[0].val.type == MATH_VALUE_NORMAL && exp->buf[0].val.val) ? 2 : 1;
  return math_expression_calculate(exp->buf[n].exp, rval);
}

#define CHECK_VAL_TYPE(R,V,T) (R)->val = ((V).val.type == T);	\
				 (R)->type = MATH_VALUE_NORMAL;


int
math_func_isnormal(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  CHECK_VAL_TYPE(rval, exp->buf[0], MATH_VALUE_NORMAL);
  return 0;
}

int
math_func_isnan(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  rval->val = (exp->buf[0].val.type == MATH_VALUE_NAN || (exp->buf[0].val.val != exp->buf[0].val.val));
  rval->type = MATH_VALUE_NORMAL;

  return 0;
}

int
math_func_isundef(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  CHECK_VAL_TYPE(rval, exp->buf[0], MATH_VALUE_UNDEF);
  return 0;
}

int
math_func_isbreak(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  CHECK_VAL_TYPE(rval, exp->buf[0], MATH_VALUE_BREAK);
  return 0;
}

int
math_func_iscont(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  CHECK_VAL_TYPE(rval, exp->buf[0], MATH_VALUE_CONT);
  return 0;
}

int
math_func_pn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double x;
  int n;
#ifdef HAVE_LIBGSL
  int r;
  gsl_sf_result val;
#else
  double l1, val;
#endif

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

#ifdef HAVE_LIBGSL
  if (n < 0 || fabs(x) > 1) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  switch (n) {
  case 1:
    r = gsl_sf_legendre_P1_e(x, &val);
    break;
  case 2:
    r = gsl_sf_legendre_P2_e(x, &val);
    break;
  case 3:
    r = gsl_sf_legendre_P3_e(x, &val);
    break;
  default:
    r = gsl_sf_legendre_Pl_e(n, x, &val);
  }
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
#else
  if (n < 0 || fabs(x) > 1000) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  l1 = 1;
  if (n == 0) {
    val = l1;
  } else {
    double l2;

    l2 = x;
    if (n == 1) {
      val = l2;
    } else {
      int i;
      for (i = 2; i <= n; i++) {
        double tmp1, tmp2;
	tmp1 = x * l2;
	tmp2 = tmp1 - l1;
	val = tmp2 + tmp1 - tmp2 / i;
	l1 = l2;
	l2 = val;
      }
    }
  }

  rval->val = val;

  return 0;
#endif
}

int
math_func_yn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n2, n;
  double x2, x;
#ifdef HAVE_LIBGSL
  int r;
  gsl_sf_result val;
#else
  int i;
  double t1, j0, j1, y0, y1, y2, val;
#endif

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  x2 = fabs(x);
  n2 = abs(n);

#ifdef HAVE_LIBGSL
  if (n2 == 0) {
    r = gsl_sf_bessel_Y0_e(x2, &val);
  } else if (n2 == 1) {
    r = gsl_sf_bessel_Y1_e(x2, &val);
  } else {
    r = gsl_sf_bessel_Yn_e(n2, x2, &val);
  }
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
#else
  if ((n2 > 1000) || (x2 > 1000) || (x2 <= 1E-300)) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  if (x2 <= 2E-5) {
    t1 = x2 * 0.5;
    j0 = 1 - t1 * t1;
    y0 = 2 / MPI * ((MEULER + log(t1)) * j0 + t1 * t1);
    if (n2 != 0) {
      j1 = t1 - 0.5 * t1 * t1 * t1;
      y1 = (j1 * y0 - 2 / MPI / x2) / j0;
      if (n2 >= -65 / log10(t1)) {
	rval->type = MATH_VALUE_ERROR;
	return -1;
      }
      for (i = 2; i <= n2; i++) {
	y2 = 2 * (i - 1) / x2 * y1 - y0;
	y0 = y1;
	y1 = y2;
      }
    }
  } else {
    int m;
    double t2, t3, s, ss;
    if (x2 >= 100) {
      m = (int) (1.073 * x2 + 47);
    } else if (x2 >= 10) {
      m = (int) (1.27 * x2 + 28);
    } else if (x2 >= 1) {
      m = (int) (2.4 * x2 + 15);
    } else if (x2 >= 0.1) {
      m = 17;
    } else {
      m = 10;
    }
    t3 = 0;
    t2 = 1E-75;
    s = 0;
    ss = 0;
    for (i = m - 1; i >= 0; i--) {
      t1 = 2 * (i + 1) / x2 * t2 - t3;
      if ((i % 2 == 0) && (i != 0)) {
        int l;
	s += t1;
	l = i / 2;
	if (l % 2 == 0) {
	  ss -= t1 / l;
	} else {
	  ss += t1 / l;
	}
	if (fabs(s) >= 1E55) {
	  t1 *= 1E-55;
	  t2 *= 1E-55;
	  s *= 1E-55;
	  ss *= 1E-55;
	}
      }
      t3 = t2;
      t2 = t1;
    }
    s = 2 * s + t1;
    j0 = t1 / s;
    j1 = t3 / s;
    ss /= s;
    y0 = 2 / MPI * ((MEULER + log(x2 * 0.5)) * j0 + 2 * ss);
    if (n2 != 0) {
      y1 = (j1 * y0 - 2 / MPI / x2) / j0;
      for (i = 2; i <= n2; i++) {
	if (y1 <= -1E65) {
          double w;
	  w = y1 * 1E-10;
	  y2 = 2 * (i - 1) / x2 * w - y0 * 1E-10;
	  if (fabs(y2) >= 1E65) {
	    rval->type = MATH_VALUE_ERROR;
	    return -1;
	  }
	  y2 *= 1E10;
	} else {
	  y2 = 2 * (i - 1) / x2 * y1 - y0;
	}
	y0 = y1;
	y1 = y2;
      }
    }
  }
  if (n == 0) {
    val = y0;
  } else {
    if ((n < 0) && (n2 % 2 == 1)) {
      y1 = -y1;
    }

    if ((x < 0) && (n2 % 2 == 1)){
      y1 = -y1;
    }

    val = y1;
  }

  rval->val = val;

  return 0;
#endif
}

int
math_func_jn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n2, n;
  double x2, x;
#ifdef HAVE_LIBGSL
  int r;
  gsl_sf_result val;
#else
  int i;
  double t1, t2, t3, j;
#endif

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  x2 = fabs(x);
  n2 = abs(n);
#ifdef HAVE_LIBGSL
  if (n2 == 0) {
    r = gsl_sf_bessel_J0_e(x2, &val);
  } else if (n2 == 1) {
    r = gsl_sf_bessel_J1_e(x2, &val);
  } else {
    r = gsl_sf_bessel_Jn_e(n2, x2, &val);
  }
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
#else
  if (n2 > 1000 || x2 > 1000) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

  if (x2 <= 2E-5) {
    t1 = x2 * 0.5;
    if (n2 == 0) {
      j = 1 - t1 * t1;
    } else if (x2 <= 1E-77) {
      j = 0;
    } else {
      t2 = 1;
      t3 = 1;
      i = 1;
      while ((i <= n2) && (fabs(t3) > fabs(t2 / t1) * 1E-77)) {
	t3 *= t1 / t2;
	t2++;
	i++;
      }

      if (i <= n2) {
	j = 0;
      } else {
	j = t3 * (1 - t1 * t1 / t2);
      }
    }
  } else {
    double s;
    int m, l;
    if (n2 > x2) {
      m = n2;
    } else {
      m = (int) x2;
    }

    if (x2 >= 100) {
      l = (int) (0.073 * x2 + 47);
    } else if (x2 >= 10) {
      l = (int) (0.27 * x2 + 27);
    } else if (x2 > 1) {
      l = (int) (1.4 * x2 + 14);
    } else {
      l = 14;
    }

    m += l;
    t3 = 0;
    t2 = 1E-75;
    s = 0;
    for (i = m - 1; i >= 0; i--) {
      t1 = 2 * (i + 1) / x2 * t2 - t3;
      if (i == n2) {
	j = t1;
      }

      if ((i % 2 == 0) && (i != 0)) {
	s += t1;
	if (fabs(s) >= 1E55) {
	  t1 *= 1E-55;
	  t2 *= 1E-55;
	  s *= 1E-55;
	  j *= 1E-55;
	}
      }
      t3 = t2;
      t2 = t1;
    }
    s = 2 * s + t1;
    j /= s;
  }

  if ((n < 0) && (n2 % 2 == 1)) {
    j = -j;
  }

  if ((x < 0) && (n2 % 2 == 1)) {
    j = -j;
  }

  rval->val = j;

  return 0;
#endif
}

#ifdef HAVE_LIBGSL
int
math_func_ynu(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double x2, x, n2, n;
  int r;
  gsl_sf_result val;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  x2 = fabs(x);
  n2 = fabs(n);

  r = gsl_sf_bessel_Ynu_e(n2, x2, &val);
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}

int
math_func_jnu(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double x2, x, n2, n;
  int r;
  gsl_sf_result val;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  x2 = fabs(x);
  n2 = fabs(n);

  r = gsl_sf_bessel_Jnu_e(n2, x2, &val);
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}

int
math_func_inu(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double x2, x, n2, n;
  int r;
  gsl_sf_result val;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  x2 = fabs(x);
  n2 = fabs(n);

  r = gsl_sf_bessel_Inu_e(n2, x2, &val);
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}

int
math_func_knu(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double x2, x, n2, n;
  int r;
  gsl_sf_result val;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  x2 = fabs(x);
  n2 = fabs(n);

  r = gsl_sf_bessel_Knu_e(n2, x2, &val);
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}

int
math_func_in(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n2, n;
  double x2, x;
  int r;
  gsl_sf_result val;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  x2 = fabs(x);
  n2 = abs(n);

  switch (n2) {
  case 0:
    r = gsl_sf_bessel_I0_e(x2, &val);
    break;
  case 1:
    r = gsl_sf_bessel_I1_e(x2, &val);
    break;
  default:
    r = gsl_sf_bessel_In_e(n2, x2, &val);
  }
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}

int
math_func_kn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n2, n;
  double x2, x;
  int r;
  gsl_sf_result val;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  x2 = fabs(x);
  n2 = abs(n);

  switch (n2) {
  case 0:
    r = gsl_sf_bessel_K0_e(x2, &val);
    break;
  case 1:
    r = gsl_sf_bessel_K1_e(x2, &val);
    break;
  default:
    r = gsl_sf_bessel_Kn_e(n2, x2, &val);
  }
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}

int
math_func_yl(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n2, n;
  double x2, x;
  int r;
  gsl_sf_result val;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  x2 = fabs(x);
  n2 = abs(n);

  switch (n2) {
  case 0:
    r = gsl_sf_bessel_y0_e(x2, &val);
    break;
  case 1:
    r = gsl_sf_bessel_y1_e(x2, &val);
    break;
  default:
    r = gsl_sf_bessel_yl_e(n2, x2, &val);
  }
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}

int
math_func_jl(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n2, n;
  double x2, x;
  int r;
  gsl_sf_result val;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  x2 = fabs(x);
  n2 = abs(n);

  switch (n2) {
  case 0:
    r = gsl_sf_bessel_j0_e(x2, &val);
    break;
  case 1:
    r = gsl_sf_bessel_j1_e(x2, &val);
    break;
  default:
    r = gsl_sf_bessel_jl_e(n2, x2, &val);
  }
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}

int
math_func_choose(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  unsigned int m, n;
  int r;
  gsl_sf_result val;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  if (exp->buf[0].val.val < 0 || exp->buf[1].val.val < 0) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  n = exp->buf[0].val.val;
  m = exp->buf[1].val.val;

  r = gsl_sf_choose_e(n, m, &val);
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
}
#endif

int
math_func_ei(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
#ifdef HAVE_LIBGSL
  int r;
  gsl_sf_result val;
#endif

  MATH_CHECK_ARG(rval, exp->buf[0]);

#ifdef HAVE_LIBGSL
  r = gsl_sf_expint_Ei_e(exp->buf[0].val.val, &val);
  rval->val = val.val;
  if (r) {
    rval->type = MATH_VALUE_ERROR;
  }
  return r;
#else
  return exp1(exp->buf[0].val.val, &rval->val);
#endif
}

static int
init_memory(void)
{
  if (Memory) {
    return 0;
  }

  Memory = g_malloc0(sizeof(*Memory) * MATH_FUNCTION_MEMORY_NUM);
  if (Memory == NULL) {
    return 1;
  }

  return 0;
}

int
math_func_cm(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int start, n;
  MathValue val;

  if (Memory == NULL && init_memory()) {
    return 1;
  }

  MATH_CHECK_ARG(rval, exp->buf[1]);

  start = 0;

  n = exp->buf[1].val.val;
  if (abs(n) >= MATH_FUNCTION_MEMORY_NUM) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (n == 0) {
    n = MATH_FUNCTION_MEMORY_NUM;
  } else if (n < 0) {
    start = MATH_FUNCTION_MEMORY_NUM + n;
    n = MATH_FUNCTION_MEMORY_NUM;
  }

  val = exp->buf[0].val;
  if (val.val == 0 && val.type == MATH_VALUE_NORMAL) {
    memset(Memory + start, 0, sizeof(*Memory) * (n - start));
  } else {
    int i;
    for (i = start; i < n; i++) {
      Memory[i] = val;
    }
  }

  *rval = exp->buf[0].val;

  return 0;
}

int
math_func_rm(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  if (Memory == NULL && init_memory()) {
    return 1;
  }

  n = exp->buf[0].val.val;

  if (abs(n) >= MATH_FUNCTION_MEMORY_NUM) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  } else if (n < 0) {
    n += MATH_FUNCTION_MEMORY_NUM;
  }

  *rval = Memory[n];

  return 0;
}

int
math_func_m(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  if (Memory == NULL && init_memory()) {
    return 1;
  }

  n = exp->buf[0].val.val;

  if (abs(n) >= MATH_FUNCTION_MEMORY_NUM) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  } else if (n < 0) {
    n += MATH_FUNCTION_MEMORY_NUM;
  }

  *rval = Memory[n] = exp->buf[1].val;

  return 0;
}

int
math_func_am(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id, n;
  MathEquationArray *ary;

  rval->val = 0;
  if (Memory == NULL && init_memory()) {
    return 1;
  }

  id = (int) exp->buf[0].array.idx;
  ary = math_equation_get_array(eq, id);

  if (ary == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  n = (ary->num > MATH_FUNCTION_MEMORY_NUM) ? MATH_FUNCTION_MEMORY_NUM : ary->num;
  if (n > 0) {
    memcpy(Memory, ary->data.val, sizeof(*Memory) * n);
  }
  rval->val = n;

  return 0;
}

int
math_func_for(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n, i;
  double v, start, stop, step;
  MathFunctionArgument *argv;

  if (Memory == NULL && init_memory()) {
    return 1;
  }

  argv = exp->buf;

  MATH_CHECK_ARG(rval, argv[0]);
  MATH_CHECK_ARG(rval, argv[1]);
  MATH_CHECK_ARG(rval, argv[2]);
  MATH_CHECK_ARG(rval, argv[3]);

  n = argv[0].val.val;
  start = argv[1].val.val;
  stop = argv[2].val.val;
  step = argv[3].val.val;

  if (n >= MATH_FUNCTION_MEMORY_NUM || (stop - start) * step <= 0) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (n >= 0) {
    Memory[n].type = MATH_VALUE_NORMAL;
  }

  i = 0;
  rval->val = 0;
  for (v = start; (step < 0) ?  (v >= stop) : (v <= stop); v += step) {
    int r;
    if ((i & 0xff) == 0 && ninterrupt()) {
      rval->type = MATH_VALUE_INTERRUPT;
      return 1;
    }
    if (n >= 0) {
      Memory[n].val = v;
    }
    r = math_expression_calculate(argv[4].exp, rval);
    if(r) {
      return r;
    }
    if (rval->type == MATH_VALUE_ERROR) {
      return 1;
    }
    i++;
  }

  return 0;
}

int
math_func_times(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n, i;
  MathFunctionArgument *argv;
  MathValue *index;

  argv = exp->buf;

  MATH_CHECK_ARG(rval, argv[0]);
  n = argv[0].val.val;
  if (n <= 0) {
    return 0;
  }

  index = math_expression_get_variable_from_argument(exp, 1);
  if (index == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  index->type = MATH_VALUE_NORMAL;
  for (i = 0; i < n; i++) {
    int r;
    if ((i & 0xff) == 0 && ninterrupt()) {
      rval->type = MATH_VALUE_INTERRUPT;
      return 1;
    }
    index->val = i;
    r = math_expression_calculate(argv[2].exp, rval);
    if(r) {
      return r;
    }
    if (rval->type == MATH_VALUE_ERROR) {
      return 1;
    }
  }

  return 0;
}

int
math_func_while(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n, i;
  MathFunctionArgument *argv;
  MathValue condition;

  if (Memory == NULL && init_memory()) {
    return 1;
  }

  argv = exp->buf;

  MATH_CHECK_ARG(rval, argv[0]);

  n = argv[0].val.val;

  if (n >= MATH_FUNCTION_MEMORY_NUM) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (n >= 0) {
    Memory[n].type = MATH_VALUE_NORMAL;
  }

  rval->val = 0;
  rval->type = MATH_VALUE_UNDEF;
  condition.val = 0;
  condition.type = MATH_VALUE_NORMAL;
  for (i = 0; ; i++) {
    int r;
    if ((i & 0xff) == 0 && ninterrupt()) {
      rval->type = MATH_VALUE_INTERRUPT;
      return 1;
    }

    if (n >= 0) {
      Memory[n].val = i;
    }

    r = math_expression_calculate(argv[1].exp, &condition);
    if(r) {
      return r;
    }
    if (condition.type == MATH_VALUE_ERROR) {
      return 1;
    }
    if (condition.val == 0.0) {
      break;
    }

    r = math_expression_calculate(argv[2].exp, rval);
    if(r) {
      return r;
    }
  }

  return 0;
}

static int
compare_double(const void *p1, const void *p2)
{
  MathValue *v1, *v2;

  v1 = (MathValue *) p1;
  v2 = (MathValue *) p2;

  if (v1->type == MATH_VALUE_NORMAL && v2->type == MATH_VALUE_NORMAL) {
    if (v1->val > v2->val) {
      return 1;
    } else if (v1->val < v2->val) {
      return -1;
    }
  } else if (v1->type != MATH_VALUE_NORMAL && v2->type == MATH_VALUE_NORMAL) {
    return 1;
  } else if (v1->type == MATH_VALUE_NORMAL && v2->type != MATH_VALUE_NORMAL) {
    return -1;
  }

  return 0;
}

static int
rcompare_double(const void *p1, const void *p2)
{
  return -compare_double(p1, p2);
}

static int
compare_str(const void *p1, const void *p2)
{
  GString *s1, *s2;

  s1 = *(GString **) p1;
  s2 = *(GString **) p2;

  return g_strcmp0(s1->str, s2->str);
}

static int
case_compare_str(const void *p1, const void *p2)
{
  GString *s1, *s2;

  s1 = *(GString **) p1;
  s2 = *(GString **) p2;

  return g_ascii_strcasecmp(s1->str, s2->str);
}

static int
rcompare_str(const void *p1, const void *p2)
{
  return -compare_str(p1, p2);
}

static int
rcase_compare_str(const void *p1, const void *p2)
{
  return -case_compare_str(p1, p2);
}

int
math_func_push(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  enum DATA_TYPE type;
  int id;
  MathEquationArray *ary;
  MathExpression *arg_exp;
  MathValue val;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  type = exp->buf[0].array.array_type;
  ary = math_equation_get_type_array(eq, type, id);
  if (ary == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  arg_exp = exp->buf[1].exp;
  if (type == DATA_TYPE_VALUE) {
    math_expression_calculate(arg_exp, &val);
    if (math_equation_push_array_val(eq, id, &val)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
  } else {
    const char *str;
    str = math_expression_get_cstring(arg_exp);
    if (str == NULL) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    if (math_equation_push_array_str(eq, id, str)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
  }

  rval->val = ary->num;
  rval->type = MATH_VALUE_NORMAL;

  return 0;
}

int
math_func_pop(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  enum DATA_TYPE type;
  int id, i, n;
  MathEquationArray *ary;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  type = exp->buf[0].array.array_type;
  ary = math_equation_get_type_array(eq, type, id);

  MATH_CHECK_ARG(rval, exp->buf[1]);
  n = exp->buf[1].val.val;

  if (ary == NULL || ary->num < 1) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (n < 0) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  if (n == 0) {
    n = 1;
  }
  if (n > ary->num) {
    n = ary->num;
  }

  for (i = 0; i < n; i++) {
    math_equation_pop_array(eq, id, type);
  }

  rval->val = ary->num;
  rval->type = MATH_VALUE_NORMAL;
  return 0;
}

int
math_func_shift(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  enum DATA_TYPE type;
  int id, i, n;
  MathEquationArray *ary;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  type = exp->buf[0].array.array_type;
  ary = math_equation_get_type_array(eq, type, id);

  MATH_CHECK_ARG(rval, exp->buf[1]);
  n = exp->buf[1].val.val;

  if (ary == NULL || ary->num < 1 || ary->data.val == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (n < 0) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  if (n == 0) {
    n = 1;
  }
  if (n > ary->num) {
    n = ary->num;
  }

  for (i = 0; i < n; i++) {
    math_equation_shift_array(eq, id, type);
  }

  rval->val = ary->num;
  rval->type = MATH_VALUE_NORMAL;
  return 0;
}

int
math_func_unshift(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  enum DATA_TYPE type;
  int id;
  MathEquationArray *ary;
  MathExpression *arg_exp;
  MathValue val;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  type = exp->buf[0].array.array_type;
  ary = math_equation_get_type_array(eq, type, id);
  if (ary == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  arg_exp = exp->buf[1].exp;
  if (type == DATA_TYPE_VALUE) {
    math_expression_calculate(arg_exp, &val);
    if (math_equation_unshift_array_val(eq, id, &val)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
  } else {
    const char *str;
    str = math_expression_get_cstring(arg_exp);
    if (str == NULL) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    if (math_equation_unshift_array_str(eq, id, str)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
  }

  rval->val = ary->num;
  rval->type = MATH_VALUE_NORMAL;

  return 0;
}

int
math_func_sort(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id;
  MathEquationArray *ary;
  enum DATA_TYPE type;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  type = exp->buf[0].array.array_type;
  if (type == DATA_TYPE_VALUE) {
    ary = math_equation_get_array(eq, id);
  } else {
    ary = math_equation_get_string_array(eq, id);
  }

  if (ary == NULL || ary->num < 1 || ary->data.str == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = ary->num;

  if (ary->num < 2) {
    return 0;
  }

  if (type == DATA_TYPE_VALUE) {
    qsort(ary->data.val, ary->num, sizeof(MathValue), compare_double);
  } else {
    int ignore_case;
    ignore_case = exp->buf[1].val.val;
    if (ignore_case) {
      qsort(ary->data.str, ary->num, sizeof(GString *), case_compare_str);
    } else {
      qsort(ary->data.str, ary->num, sizeof(GString *), compare_str);
    }
  }

  return 0;
}

int
math_func_rsort(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id;
  MathEquationArray *ary;
  enum DATA_TYPE type;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  type = exp->buf[0].array.array_type;
  if (type == DATA_TYPE_VALUE) {
    ary = math_equation_get_array(eq, id);
  } else {
    ary = math_equation_get_string_array(eq, id);
  }

  if (ary == NULL || ary->num < 1 || ary->data.str == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = ary->num;

  if (ary->num < 2) {
    return 0;
  }

  if (type == DATA_TYPE_VALUE) {
    qsort(ary->data.val, ary->num, sizeof(MathValue), rcompare_double);
  } else {
    int ignore_case;
    ignore_case = exp->buf[1].val.val;
    if (ignore_case) {
      qsort(ary->data.str, ary->num, sizeof(GString *), rcase_compare_str);
    } else {
      qsort(ary->data.str, ary->num, sizeof(GString *), rcompare_str);
    }
  }

  return 0;
}

int
math_func_size(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id;
  MathEquationArray *ary;
  enum DATA_TYPE type;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  type = exp->buf[0].array.array_type;
  if (type == DATA_TYPE_VALUE) {
    ary = math_equation_get_array(eq, id);
  } else {
    ary = math_equation_get_string_array(eq, id);
  }

  if (ary == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = ary->num;

  return 0;
}

int
math_func_array(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id, i, r;
  MathEquationArray *ary;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  ary = math_equation_get_array(eq, id);
  if (ary == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  r = math_equation_clear_array(eq, id);
  if (r) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  for (i = 1; i < exp->argc; i++) {
    if (math_equation_push_array_val(eq, id, &exp->buf[i].val)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
  }

  rval->val = exp->argc - 1;

  return 0;
}

int
math_func_array_sum(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id, i;
  MathEquationArray *ary;
  double sum;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  ary = math_equation_get_array(eq, id);

  if (ary == NULL || ary->data.val == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  sum = 0;
  for (i = 0; i < ary->num; i++) {
    if (ary->data.val[i].type != MATH_VALUE_NORMAL) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    sum += ary->data.val[i].val;
  }
  rval->val = sum;

  return 0;
}

int
math_func_array_sumsq(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id, i;
  MathEquationArray *ary;
  double sum;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  ary = math_equation_get_array(eq, id);

  if (ary == NULL || ary->data.val == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  sum = 0;
  for (i = 0; i < ary->num; i++) {
    double val;
    if (ary->data.val[i].type != MATH_VALUE_NORMAL) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    val = ary->data.val[i].val;
    sum += val * val;
  }
  rval->val = sum;

  return 0;
}

int
math_func_array_min(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id, i;
  MathEquationArray *ary;
  double min;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  ary = math_equation_get_array(eq, id);

  if (ary == NULL || ary->num < 1 || ary->data.val == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  min = ary->data.val[0].val;
  for (i = 0; i < ary->num; i++) {
    if (ary->data.val[i].type != MATH_VALUE_NORMAL) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    if (min > ary->data.val[i].val) {
      min = ary->data.val[i].val;
    }
  }
  rval->val = min;

  return 0;
}

int
math_func_array_average(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n;

  rval->val = 0;

  if (math_func_size(exp, eq, rval)) {
    return 1;
  }
  n = rval->val;

  if (math_func_array_sum(exp, eq, rval)) {
    return 1;
  }
  rval->val /= n;
  return 0;
}

static int
moving_average(MathEquation *eq, int dest, MathEquationArray *src, int avg_n, int n, int type)
{
  int i, r, dest_n;
  struct narray array;
  MathValue *val_array;

  arrayinit(&array, sizeof(*val_array));

  r = (type) ? 1 : 0;
  avg_n = abs(avg_n);
  for (i = 0; i < n; i++) {
    int j, sum_n, weight;
    double sum;
    MathValue val;

    if (src->data.val[i].type != MATH_VALUE_NORMAL) {
      arrayadd(&array, src->data.val + i);
      continue;
    }

    weight = avg_n * r + 1;
    sum = src->data.val[i].val * weight;
    sum_n = weight;
    for (j = 0; j < avg_n; j++) {
      int k;
      weight = (avg_n - j - 1) * r + 1;
      k = i - j - 1;
      if (k >= 0 && src->data.val[k].type == MATH_VALUE_NORMAL) {
        sum += src->data.val[k].val * weight;
        sum_n += weight;
      }
      k = i + j + 1;
      if (k < n && src->data.val[k].type == MATH_VALUE_NORMAL) {
        sum += src->data.val[k].val * weight;
        sum_n += weight;
      }
    }
    val.val = sum / sum_n;
    val.type = MATH_VALUE_NORMAL;
    arrayadd(&array, &val);
  }
  dest_n = arraynum(&array);
  val_array = arraydata(&array);
  for (i = 0; i < n; i++) {
    if (math_equation_set_array_val(eq, dest, i, val_array + i)) {
      arraydel(&array);
      return 1;
    }
  }
  arraydel(&array);

  if (n != dest_n) {
    return 1;
  }

  return 0;
}

int
math_func_array_moving_average(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n, dest_id, src_id, type, avg_n;
  MathEquationArray *src;

  MATH_CHECK_ARG(rval, exp->buf[2]);
  MATH_CHECK_ARG(rval, exp->buf[3]);

  dest_id = (int) exp->buf[0].array.idx;
  src_id = (int) exp->buf[1].array.idx;
  avg_n = exp->buf[2].val.val;
  type = exp->buf[3].val.val;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  src = math_equation_get_array(eq, src_id);
  if (src == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  n = src->num;
  if (moving_average(eq, dest_id, src, avg_n, n, type)) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  rval->val = n;
  return 0;
}

int
math_func_array_stdevp(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n;
  double sum, sumsq, stdev;

  rval->val = 0;

  if (math_func_size(exp, eq, rval)) {
    return 1;
  }
  n = rval->val;
  if (n < 1) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (math_func_array_sum(exp, eq, rval)) {
    return 1;
  }
  sum = rval->val;

  if (math_func_array_sumsq(exp, eq, rval)) {
    return 1;
  }
  sumsq = rval->val;

  sum /= n;
  stdev = sumsq / n - sum * sum;
  if (stdev < 0) {
    stdev = 0;
  }
  rval->val = sqrt(stdev);
  return 0;
}

int
math_func_array_stdev(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n;
  double stdev;

  rval->val = 0;

  if (math_func_size(exp, eq, rval)) {
    return 1;
  }
  n = rval->val;
  if (n < 2) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (math_func_array_stdevp(exp, eq, rval)) {
    return 1;
  }
  stdev = rval->val;
  stdev *= stdev * n;
  stdev /= n - 1;
  rval->val = sqrt(stdev);
  return 0;
}

int
math_func_array_max(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id, i;
  MathEquationArray *ary;
  double max;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  ary = math_equation_get_array(eq, id);

  if (ary == NULL || ary->num < 1 || ary->data.val == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  max = ary->data.val[0].val;
  for (i = 0; i < ary->num; i++) {
    if (ary->data.val[i].type != MATH_VALUE_NORMAL) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    if (max < ary->data.val[i].val) {
      max = ary->data.val[i].val;
    }
  }
  rval->val = max;

  return 0;
}

int
math_func_array_compact(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id, i, n;
  MathEquationArray *ary;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  ary = math_equation_get_array(eq, id);

  if (ary == NULL || ary->num < 1 || ary->data.val == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  n = 0;
  for (i = 0; i < ary->num; i++) {
    if (ary->data.val[i].type != MATH_VALUE_NORMAL) {
      ary->num--;
      n++;
      if (i < ary->num) {
        memmove(ary->data.val + i, ary->data.val + i + 1, sizeof(*ary->data.val) * (ary->num - i));
      }
    }
  }
  for (i = 0; i < n; i++) {
    ary->data.val[i + ary->num].type = MATH_VALUE_NORMAL;
    ary->data.val[i + ary->num].val = 0;
  }
  rval->val = ary->num;

  return 0;
}

int
math_func_array_clear(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id, r, type;

  rval->val = 0;

  id = (int) exp->buf[0].array.idx;
  type = exp->buf[0].array.array_type;
  if (type == DATA_TYPE_VALUE) {
    r = math_equation_clear_array(eq, id);
  } else {
    r = math_equation_clear_string_array(eq, id);
  }

  if (r) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  return 0;
}

int
math_func_sum(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  if (exp->buf[0].val.type == MATH_VALUE_NORMAL) {
    MathValue *ptr = eq->pos_func_buf + exp->pos_id;
    ptr->val += exp->buf[0].val.val;
    ptr->type = MATH_VALUE_NORMAL;
    *rval = *ptr;
  } else {
    *rval = exp->buf[0].val;
  }

  return 0;
}

int
math_func_dif(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  if (exp->buf[0].val.type == MATH_VALUE_NORMAL) {
    MathValue *ptr = eq->pos_func_buf + exp->pos_id;
    if (ptr->type == MATH_VALUE_NORMAL) {
      rval->val = exp->buf[0].val.val - ptr->val;
    } else {
      rval->val = 0;
    }
    rval->type = ptr->type;
    *ptr = exp->buf[0].val;
  } else {
    *rval = exp->buf[0].val;
  }

  return 0;
}

int
math_func_fmod(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double x, y;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  x = exp->buf[0].val.val;
  y = exp->buf[1].val.val;

  if (y == 0.0) {
    rval->type = MATH_VALUE_NAN;
    rval->val = 0.0;
    return 0;
  }

  rval->val = fmod(x, y);

  return 0;
}

int
math_func_string_append(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  const char *str;
  GString *dest;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  dest = math_expression_get_string_variable_from_argument(exp, 0);
  str = math_expression_get_string_from_argument(exp, 1);
  if (dest == NULL || str == NULL) {
    return 1;
  }
  g_string_append(dest, str);
  return 0;
}

int
math_func_string_prepend(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  const char *str;
  GString *dest;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  dest = math_expression_get_string_variable_from_argument(exp, 0);
  str = math_expression_get_string_from_argument(exp, 1);
  if (dest == NULL || str == NULL) {
    return 1;
  }
  g_string_prepend(dest, str);
  return 0;
}

int
math_func_string_compare(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  const char *str1, *str2;
  int ignore_case;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  str1 = math_expression_get_string_from_argument(exp, 0);
  str2 = math_expression_get_string_from_argument(exp, 1);
  if (str1 == NULL || str2 == NULL) {
    return 1;
  }
  MATH_CHECK_ARG(rval, exp->buf[2]);
  ignore_case = exp->buf[2].val.val;

  if (ignore_case) {
    rval->val = g_ascii_strcasecmp(str1, str2);
  } else {
    rval->val = strcmp(str1, str2);
  }
  return 0;
}

int
math_func_string_up(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  GString *dest;
  const char *src;
  char *tmp;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  dest = math_expression_get_string_variable_from_argument(exp, 0);
  src  = math_expression_get_string_from_argument(exp, 1);
  if (dest == NULL || src == NULL) {
    return 1;
  }
  tmp = g_ascii_strup(src, -1);
  if (tmp == NULL) {
    return 1;
  }
  g_string_assign(dest, tmp);
  g_free(tmp);
  return 0;
}

int
math_func_string_down(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  GString *dest;
  const char *src;
  char *tmp;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  dest = math_expression_get_string_variable_from_argument(exp, 0);
  src  = math_expression_get_string_from_argument(exp, 1);
  if (dest == NULL || src == NULL) {
    return 1;
  }
  tmp = g_ascii_strdown(src, -1);
  if (tmp == NULL) {
    return 1;
  }
  g_string_assign(dest, tmp);
  g_free(tmp);
  return 0;
}

int
math_func_string_length(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  const char *str;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  str = math_expression_get_string_from_argument(exp, 0);
  if (str == NULL) {
    return 1;
  }
  if (! g_utf8_validate(str, -1, NULL)) {
    return 1;
  }
  rval->val = g_utf8_strlen(str, -1);
  return 0;
}

int
math_func_string_float(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  const char *str;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  str = math_expression_get_string_from_argument(exp, 0);
  if (str == NULL) {
    return 1;
  }
  n_strtod(str, rval);
  return 0;
}

int
math_func_map(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int src_id, dest_id, i, n;
  enum DATA_TYPE src_type;
  MathEquationArray *src;
  MathValue val;
  MathVariable variable;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  dest_id = (int) exp->buf[0].array.idx;
  if (get_common_array(exp, eq, 1, 2, &src_id, &src_type, &variable, &src)) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  n = src->num;
  for (i = 0; i < n; i++) {
    if (set_common_array(eq, src_id, i, src_type, &variable)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    math_expression_calculate(exp->buf[3].exp, &val);
    if (math_equation_set_array_val(eq, dest_id, i, &val)) {
      return 1;
    }
  }

  rval->val = n;
  return 0;
}

int
math_func_array_copy(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int src_id, dest_id, i, n;
  enum DATA_TYPE src_type, dest_type;
  MathEquationArray *src;
  MathCommonValue val;
  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  dest_id = (int) exp->buf[0].array.idx;
  dest_type = exp->buf[0].array.array_type;
  src_id = (int) exp->buf[1].array.idx;
  src_type = exp->buf[1].array.array_type;

  if (dest_type != src_type) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  src = math_equation_get_type_array(eq, src_type, src_id);
  n = src->num;
  for (i = 0; i < n; i++) {
    if(math_equation_get_array_common_value(eq, src_id, i, src_type, &val)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    if (math_equation_set_array_common_value(eq, dest_id, i, &val)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
  }
  rval->val = n;
  return 0;
}

int
math_func_filter(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int src_id, dest_id, i, j, n;
  MathEquationArray *src;
  MathCommonValue cval;
  MathValue val;
  MathVariable variable;
  enum DATA_TYPE src_type, dest_type;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  dest_id = (int) exp->buf[0].array.idx;
  dest_type = exp->buf[0].array.array_type;
  src_id = (int) exp->buf[1].array.idx;
  src_type = exp->buf[1].array.array_type;
  if (src_type != dest_type) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  if (math_function_call_expression_get_variable(exp, 2, &variable)) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  if (src_type != variable.type) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  src = math_equation_get_type_array(eq, src_type, src_id);
  j = 0;
  n = src->num;
  for (i = 0; i < n; i++) {
    if(math_equation_get_array_common_value(eq, src_id, i, src_type, &cval)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    if (math_variable_set_common_value(&variable, &cval)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    math_expression_calculate(exp->buf[3].exp, &val);
    if (val.type == MATH_VALUE_NORMAL && val.val) {
      if (math_equation_set_array_common_value(eq, dest_id, j, &cval)) {
	rval->type = MATH_VALUE_ERROR;
	return 1;
      }
      j++;
    }
  }
  rval->val = j;
  return 0;
}

int
math_func_find(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int src_id, i, n;
  MathEquationArray *src;
  MathValue val, *vptr;

  rval->val = 0;
  rval->type = MATH_VALUE_UNDEF;

  src_id = (int) exp->buf[0].array.idx;
  src = math_equation_get_array(eq, src_id);
  vptr = math_expression_get_variable_from_argument(exp, 1);
  if (vptr == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  n = src->num;
  for (i = 0; i < n; i++) {
    if (math_equation_get_array_val(eq, src_id, i, vptr)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    math_expression_calculate(exp->buf[2].exp, &val);
    if (val.type == MATH_VALUE_NORMAL && val.val) {
      *rval = *vptr;
      return 0;
    }
  }
  return 0;
}

int
math_func_index(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  enum DATA_TYPE src_type;
  int src_id, i, n;
  MathEquationArray *src;
  MathValue val;
  MathVariable variable;

  rval->val = 0;
  rval->type = MATH_VALUE_UNDEF;

  if (get_common_array(exp, eq, 0, 1, &src_id, &src_type, &variable, &src)) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  n = src->num;
  for (i = 0; i < n; i++) {
    if (set_common_array(eq, src_id, i, src_type, &variable)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    math_expression_calculate(exp->buf[2].exp, &val);
    if (val.type == MATH_VALUE_NORMAL && val.val) {
      rval->type = MATH_VALUE_NORMAL;
      rval->val = i;
      return 0;
    }
  }
  return 0;
}

static int
get_common_array(MathFunctionCallExpression *exp, MathEquation *eq, int ary_arg, int var_arg, int *id, enum DATA_TYPE *type, MathVariable *variable, MathEquationArray **src)
{
  int src_id;
  enum DATA_TYPE src_type;

  src_id = (int) exp->buf[ary_arg].array.idx;
  src_type = exp->buf[ary_arg].array.array_type;
  if (math_function_call_expression_get_variable(exp, var_arg, variable)) {
    return 1;
  }
  if (src_type != variable->type) {
    return 1;
  }
  *src = math_equation_get_type_array(eq, src_type, src_id);
  *id = src_id;
  *type = src_type;
  return 0;
}

static int
set_common_array(MathEquation *eq, int id, int i, enum DATA_TYPE type, MathVariable *variable)
{
  MathCommonValue cval;
  if(math_equation_get_array_common_value(eq, id, i, type, &cval)) {
    return 1;
  }
  if (math_variable_set_common_value(variable, &cval)) {
    return 1;
  }
  return 0;
}

int
math_func_each(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int src_id, i, n;
  MathEquationArray *src;
  MathValue val;
  MathVariable variable;
  enum DATA_TYPE src_type;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  if (get_common_array(exp, eq, 0, 1, &src_id, &src_type, &variable, &src)) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  n = src->num;
  for (i = 0; i < n; i++) {
    if (set_common_array(eq, src_id, i, src_type, &variable)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    math_expression_calculate(exp->buf[2].exp, &val);
  }
  rval->val = n;
  return 0;
}

int
math_func_each_with_index(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int src_id, i, n;
  MathEquationArray *src;
  MathValue val;
  MathVariable variable;
  enum DATA_TYPE src_type;
  MathValue *index;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  if (get_common_array(exp, eq, 0, 1, &src_id, &src_type, &variable, &src)) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  index = math_expression_get_variable_from_argument(exp, 2);
  if (index == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  index->type = MATH_VALUE_NORMAL;
  n = src->num;
  for (i = 0; i < n; i++) {
    if (set_common_array(eq, src_id, i, src_type, &variable)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    index->val = i;
    math_expression_calculate(exp->buf[3].exp, &val);
  }
  rval->val = n;
  return 0;
}

int
math_func_zip(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int src1_id, src2_id, i, n;
  MathEquationArray *src1, *src2;
  MathValue val;
  MathVariable variable1, variable2;
  enum DATA_TYPE src1_type, src2_type;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  if (get_common_array(exp, eq, 0, 2, &src1_id, &src1_type, &variable1, &src1)) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  if (get_common_array(exp, eq, 1, 3, &src2_id, &src2_type, &variable2, &src2)) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  n = MIN(src1->num, src2->num);
  for (i = 0; i < n; i++) {
    if (set_common_array(eq, src1_id, i, src1_type, &variable1)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    if (set_common_array(eq, src2_id, i, src2_type, &variable2)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    math_expression_calculate(exp->buf[4].exp, &val);
  }
  rval->val = n;
  return 0;
}

int
math_func_zip_map(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int dest_id, src1_id, src2_id, i, n;
  MathEquationArray *src1, *src2;
  MathValue val;
  MathVariable variable1, variable2;
  enum DATA_TYPE src1_type, src2_type;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  dest_id = (int) exp->buf[0].array.idx;
  if (get_common_array(exp, eq, 1, 3, &src1_id, &src1_type, &variable1, &src1)) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  if (get_common_array(exp, eq, 2, 4, &src2_id, &src2_type, &variable2, &src2)) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  n = MIN(src1->num, src2->num);
  for (i = 0; i < n; i++) {
    if (set_common_array(eq, src1_id, i, src1_type, &variable1)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    if (set_common_array(eq, src2_id, i, src2_type, &variable2)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    math_expression_calculate(exp->buf[5].exp, &val);
    if (math_equation_set_array_val(eq, dest_id, i, &val)) {
      return 1;
    }
  }

  rval->val = n;
  return 0;
}

int
math_func_reduce(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int src_id, i, n;
  MathValue *result;
  MathEquationArray *src;
  MathCommonValue cval;
  MathValue val;
  MathVariable variable;
  enum DATA_TYPE src_type;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  src_id = (int) exp->buf[0].array.idx;
  src_type = exp->buf[0].array.array_type;
  if (math_function_call_expression_get_variable(exp, 1, &variable)) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  if (src_type != variable.type) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  result = math_expression_get_variable_from_argument(exp, 2);
  if (result == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  src = math_equation_get_type_array(eq, src_type, src_id);
  n = src->num;
  for (i = 0; i < n; i++) {
    if(math_equation_get_array_common_value(eq, src_id, i, src_type, &cval)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    if (math_variable_set_common_value(&variable, &cval)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    math_expression_calculate(exp->buf[3].exp, &val);
    *result = val;
  }
  *rval = *result;
  return 0;
}

int
math_func_string_float_array(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int src_id, dest_id, i, n;
  MathEquationArray *src;
  MathValue val;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  dest_id = (int) exp->buf[0].array.idx;

  src_id = (int) exp->buf[1].array.idx;
  src = math_equation_get_string_array(eq, src_id);

  if (src == NULL || src->data.str == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  val.type = MATH_VALUE_NORMAL;
  n = src->num;
  for (i = 0; i < n; i++) {
    n_strtod(src->data.str[i]->str, &val);
    if (math_equation_set_array_val(eq, dest_id, i, &val)) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
  }
  rval->val = n;
  return 0;
}

int
math_func_string_replace(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  GString *dest;
  const char *src, *pattern, *replacement;
  char *tmp;
  GRegex *regex;
  int compile_options, ignore_case;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  dest = math_expression_get_string_variable_from_argument(exp, 0);
  src         = math_expression_get_string_from_argument(exp, 1);
  pattern     = math_expression_get_string_from_argument(exp, 2);
  replacement = math_expression_get_string_from_argument(exp, 3);

  MATH_CHECK_ARG(rval, exp->buf[4]);
  ignore_case = exp->buf[4].val.val;

  if (dest == NULL || src == NULL || pattern == NULL || replacement == NULL) {
    return 1;
  }
  compile_options = (ignore_case) ? G_REGEX_CASELESS : 0;
  regex = g_regex_new(pattern, compile_options, 0, NULL);
  if (regex == NULL) {
    return 1;
  }
  tmp = g_regex_replace(regex, src, -1, 0, replacement, 0, NULL);
  g_regex_unref(regex);
  if (tmp == NULL) {
    return 1;
  }
  g_string_assign(dest, tmp);
  g_free(tmp);
  return 0;
}

int
math_func_string_substring(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  GString *dest;
  const char *src;
  char *tmp;
  int start, end, n;

  MATH_CHECK_ARG(rval, exp->buf[2]);
  MATH_CHECK_ARG(rval, exp->buf[3]);
  start = exp->buf[2].val.val;
  end = exp->buf[3].val.val;
  if (start < 0 || end < 0) {
    return 1;
  }
  if (start > end) {
    n = start;
    start = end;
    end = n;
  }

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  dest = math_expression_get_string_variable_from_argument(exp, 0);
  src  = math_expression_get_string_from_argument(exp, 1);
  if (dest == NULL || src == NULL) {
    return 1;
  }
  if (! g_utf8_validate(src, -1, NULL)) {
    return 1;
  }
  n = g_utf8_strlen(src, -1);
  if (start >= n) {
    return 1;
  }
  tmp = g_utf8_substring(src, start, end);
  if (tmp == NULL) {
    return 1;
  }
  g_string_assign(dest, tmp);
  g_free(tmp);
  return 0;
}

int
math_func_string_reverse(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  GString *dest;
  const char *src;
  char *tmp;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  dest = math_expression_get_string_variable_from_argument(exp, 0);
  src  = math_expression_get_string_from_argument(exp, 1);
  if (dest == NULL || src == NULL) {
    return 1;
  }
  if (! g_utf8_validate(src, -1, NULL)) {
    return 1;
  }
  tmp = g_utf8_strreverse(src, -1);
  if (tmp == NULL) {
    return 1;
  }
  g_string_assign(dest, tmp);
  g_free(tmp);
  return 0;
}

int
math_func_string_strip(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  GString *src;
  char *tmp;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  src = math_expression_get_string_variable_from_argument(exp, 0);
  if (src == NULL || src->str == NULL) {
    return 1;
  }
  tmp = g_strdup(src->str);
  if (tmp == NULL) {
    return 1;
  }
  g_strstrip(tmp);
  g_string_assign(src, tmp);
  g_free(tmp);
  return 0;
}

static int
math_func_printf_common(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval, GString *gstr, int index)
{
  const char *str;
  char *format;
  int po, len, i;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  str = math_expression_get_string_from_argument(exp, index);
  if (gstr == NULL || str == NULL) {
    return 1;
  }
  format = g_strdup(str);
  if (format == NULL) {
    return 1;
  }
  g_string_set_size(gstr, 0);
  po = 0;
  i = index + 1;
  for (po = 0; format[po]; po++) {
    if (format[po] == '%') {
      if (format[po + 1] == '%') {
        po++;
      } else if (i < exp->argc) {
        add_printf_formated_double(gstr, format + po, &exp->buf[i].val, &len);
        po += len;
        i++;;
        continue;
      }
    }
    g_string_append_c(gstr, format[po]);
  }
  g_free(format);
  if (g_utf8_validate(gstr->str, -1, NULL)) {
    rval->val = g_utf8_strlen(gstr->str, -1);
  }
  return 0;
}

int
math_func_sprintf(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  GString *gstr;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  gstr = math_expression_get_string_variable_from_argument(exp, 0);
  if (gstr == NULL) {
    return 1;
  }
  return math_func_printf_common(exp, eq, rval, gstr, 1);
}

int
math_func_puts(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  const char *str;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  str = math_expression_get_string_from_argument(exp, 0);
  if (str == NULL) {
    str = "";
  }
  putstdout(str);
  if (g_utf8_validate(str, -1, NULL)) {
    rval->val = g_utf8_strlen(str, -1);
  }
  return 0;
}

int
math_func_printf(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  GString *gstr;
  int r;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  gstr = g_string_new("");
  if (gstr == NULL) {
    return 1;
  }
  r = math_func_printf_common(exp, eq, rval, gstr, 0);
  if (r) {
    g_string_free(gstr, TRUE);
    return r;
  }
  printfstdout("%s", gstr->str);
  g_string_free(gstr, TRUE);
  return 0;
}

int
math_func_string(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  GString *gstr;
  double val;

  MATH_CHECK_ARG(rval, exp->buf[1]);
  val = exp->buf[1].val.val;

  rval->val = val;
  rval->type = MATH_VALUE_NORMAL;

  gstr = math_expression_get_string_variable_from_argument(exp, 0);
  if (gstr == NULL) {
    return 1;
  }
  g_string_printf(gstr, "%g", val);
  return 0;
}

static char **
string_split(MathFunctionCallExpression *exp, MathValue *rval)
{
  const char *delm, *src;
  char **ary;
  int use_regexp;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  src  = math_expression_get_string_from_argument(exp, 1);
  delm = math_expression_get_string_from_argument(exp, 2);
  if (src == NULL || delm == NULL) {
    return NULL;
  }
  MATH_CHECK_ARG(rval, exp->buf[3]);
  use_regexp = exp->buf[3].val.val;

  if (use_regexp) {
    int compile_options;
    compile_options = (use_regexp == 2) ? G_REGEX_CASELESS : 0;
    ary = g_regex_split_simple(delm, src, compile_options, 0);
  } else {
    ary = g_strsplit(src, delm, -1);
  }
  return ary;
}

int
math_func_string_split(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  char **ary;
  int i, id;
  enum DATA_TYPE type;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  id = (int) exp->buf[0].array.idx;
  ary = string_split(exp, rval);
  if (ary == NULL) {
    return 1;
  }
  type = exp->buf[0].array.array_type;
  if (type == DATA_TYPE_VALUE) {
    for (i = 0; ary[i]; i++) {
      MathValue val;
      n_strtod(ary[i], &val);
      math_equation_set_array_val(eq, id, i, &val);
    }
  } else {
    for (i = 0; ary[i]; i++) {
      math_equation_set_array_str(eq, id, i, ary[i]);
    }
  }
  g_strfreev(ary);
  rval->val = i;
  return 0;
}

int
math_func_string_split_float(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  return math_func_string_split(exp, eq, rval);
}

static int
get_pos_from_upos(const char *src, int upos)
{
  const char *ptr;
  int n;

  n = g_utf8_strlen(src, -1);
  if (upos > n) {
    return -1;
  }
  if (upos < 0) {
    upos += n;
  }
  if (upos < 0) {
    return -1;
  }
  ptr = g_utf8_offset_to_pointer(src, upos);
  return ptr - src;
}

int
math_func_string_insert(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  GString *dest;
  const char *str;
  int upos, pos;

  MATH_CHECK_ARG(rval, exp->buf[2]);
  upos = exp->buf[2].val.val;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  dest = math_expression_get_string_variable_from_argument(exp, 0);
  str  = math_expression_get_string_from_argument(exp, 1);
  if (dest == NULL || str == NULL) {
    return 1;
  }
  if (! g_utf8_validate(dest->str, -1, NULL)) {
    return 1;
  }
  pos = get_pos_from_upos(dest->str, upos);
  if (pos < 0) {
    return 1;
  }
  g_string_insert(dest, pos, str);
  return 0;
}

int
math_func_string_erase(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  GString *src;
  int upos, ulen, pos, len;

  MATH_CHECK_ARG(rval, exp->buf[1]);
  MATH_CHECK_ARG(rval, exp->buf[2]);
  upos = exp->buf[1].val.val;
  ulen = exp->buf[2].val.val;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  if (ulen < 1) {
    return 1;
  }
  src = math_expression_get_string_variable_from_argument(exp, 0);
  if (src == NULL || src->str == NULL) {
    return 1;
  }
  if (! g_utf8_validate(src->str, -1, NULL)) {
    return 1;
  }
  pos = get_pos_from_upos(src->str, upos);
  if (pos < 0) {
    return 1;
  }
  len = get_pos_from_upos(src->str, upos + ulen);
  if (len < 0) {
    return 1;
  }
  len -= pos;
  g_string_erase(src, pos, len);
  return 0;
}

int
math_func_string_truncate(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  GString *src;
  int ulen, len;

  MATH_CHECK_ARG(rval, exp->buf[1]);
  ulen = exp->buf[1].val.val;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  if (ulen < 1) {
    return 1;
  }
  src = math_expression_get_string_variable_from_argument(exp, 0);
  if (src == NULL || src->str == NULL) {
    return 1;
  }
  if (! g_utf8_validate(src->str, -1, NULL)) {
    return 1;
  }
  len = get_pos_from_upos(src->str, ulen);
  if (len < 0) {
    return 1;
  }
  g_string_truncate(src, len);
  return 0;
}

int
math_func_string_match(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  const char *src, *pattern;
  int compile_options, ignore_case;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  src     = math_expression_get_string_from_argument(exp, 0);
  pattern = math_expression_get_string_from_argument(exp, 1);

  MATH_CHECK_ARG(rval, exp->buf[2]);
  ignore_case = exp->buf[2].val.val;

  if (src == NULL || pattern == NULL) {
    return 1;
  }
  compile_options = (ignore_case) ? G_REGEX_CASELESS : 0;
  rval->val = g_regex_match_simple(pattern, src, compile_options, 0);

  return 0;
}

int
math_func_string_join(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  GString *dest;
  const char *sep;
  char **strv, *str;
  int i, n, id;
  MathEquationArray *ary;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;


  dest = math_expression_get_string_variable_from_argument(exp, 0);
  sep  = math_expression_get_string_from_argument(exp, 1);
  id = (int) exp->buf[2].array.idx;
  ary = math_equation_get_string_array(eq, id);

  if (ary == NULL || ary->data.str == NULL || dest == NULL || sep == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (ary->num < 1) {
    g_string_set_size(dest, 0);
    return 0;
  }

  n = ary->num;
  strv = g_malloc(sizeof(*strv) * (n + 1));
  if (strv == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  for (i = 0; i < n; i++) {
    strv[i] = ary->data.str[i]->str;
  }
  strv[i] = NULL;
  str = g_strjoinv(sep, strv);
  g_free(strv);
  g_string_assign(dest, str);
  g_free(str);
  return 0;
}

static struct objlist *
math_func_getobj_common(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval, int start, int *id, int *ret, enum ngraph_object_field_type *type)
{
  const char *objname, *field;
  struct objlist *obj;
  int last, i, permission;

  rval->val = 0;
  *ret = 0;
  i = start;
  objname = math_expression_get_string_from_argument(exp, i);
  i++;
  field = math_expression_get_string_from_argument(exp, i);
  i++;
  if (objname ==  NULL || field == NULL) {
    return NULL;
  }

  if (exp->buf[i].val.type != MATH_VALUE_NORMAL) {
    return NULL;
  }
  *id = exp->buf[i].val.val;
  if (*id < 0) {
    rval->type = MATH_VALUE_ERROR;
    *ret = 1;
    return NULL;
  }

  obj = getobject(objname);
  if (obj == NULL) {
    rval->type = MATH_VALUE_ERROR;
    *ret = 1;
    return NULL;
  }
  last = chkobjlastinst(obj);
  if (*id > last) {
    return NULL;
  }
  if (chkobjfield(obj, field)) {
    rval->type = MATH_VALUE_ERROR;
    *ret = 1;
    return NULL;
  }

  permission = chkobjperm(obj, field);
  if ((permission & NREAD) == 0) {
    rval->type = MATH_VALUE_ERROR;
    *ret = 1;
    return NULL;
  }
  if (permission & NEXEC) {
    rval->type = MATH_VALUE_ERROR;
    *ret = 1;
    return NULL;
  }

  *type = chkobjfieldtype(obj, field);
  return obj;
}

int
math_func_getobj(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  const char *field;
  struct objlist *obj;
  enum ngraph_object_field_type type;
  int ival, id, ret;
  double dval;

  rval->val = 0;
  obj = math_func_getobj_common(exp, eq, rval, 0, &id, &ret, &type);
  if (obj == NULL) {
    return ret;
  }
  field = math_expression_get_string_from_argument(exp, 1);
  if (field == NULL) {
    return 0;
  }
  switch (type) {
  case NINT:
  case NENUM:
  case NBOOL:
    if (getobj(obj, field, id, 0, NULL, &ival) < 0) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    rval->val = ival;
    break;
  case NDOUBLE:
    if (getobj(obj, field, id, 0, NULL, &dval) < 0) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    rval->val = dval;
    break;
  default:
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  return 0;
}

int
math_func_getobj_string(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  const char *field;
  struct objlist *obj;
  enum ngraph_object_field_type type;
  char *str;
  GString *gstr;
  int id, ret;

  rval->val = 0;

  gstr = math_expression_get_string_variable_from_argument(exp, 0);
  if (gstr == NULL) {
    return 0;
  }

  obj = math_func_getobj_common(exp, eq, rval, 1, &id, &ret, &type);
  if (obj == NULL) {
    return ret;
  }
  field = math_expression_get_string_from_argument(exp, 2);
  if (field == NULL) {
    return 0;
  }
  switch (type) {
  case NSTR:
    if (getobj(obj, field, id, 0, NULL, &str) < 0) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    if (str == NULL) {
      str = "";
    } else if (g_utf8_validate(str, -1, NULL)) {
      rval->val = g_utf8_strlen(str, -1);
    }
    g_string_assign(gstr, str);
    break;
  default:
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  return 0;
}

int
math_func_getobj_array(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  const char *field;
  struct objlist *obj;
  enum ngraph_object_field_type type;
  enum DATA_TYPE array_type;
  int array_id;
  struct narray *array;
  int id, ret, i, n;
  int *idata;
  double *ddata;
  char **sdata;
  MathValue val;

  rval->val = 0;

  array_id = exp->buf[0].array.idx;
  array_type = exp->buf[0].array.array_type;
  if (array_id < 0) {
    return 0;
  }
  obj = math_func_getobj_common(exp, eq, rval, 1, &id, &ret, &type);
  if (obj == NULL) {
    return ret;
  }
  field = math_expression_get_string_from_argument(exp, 2);
  if (field == NULL) {
    return 0;
  }
  val.type = MATH_VALUE_NORMAL;
  switch (type) {
  case NIARRAY:
    if (array_type != DATA_TYPE_VALUE) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    if (getobj(obj, field, id, 0, NULL, &array) < 0) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    n = arraynum(array);
    if (n < 0) {
      break;
    }
    idata = arraydata(array);
    if (idata == NULL) {
      break;
    }
    for (i = 0; i < n; i++) {
      val.val = idata[i];
      math_equation_set_array_val(eq, array_id, i, &val);
    }
    rval->val = n;
    break;
  case NDARRAY:
    if (array_type != DATA_TYPE_VALUE) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    if (getobj(obj, field, id, 0, NULL, &array) < 0) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    n = arraynum(array);
    if (n < 0) {
      break;
    }
    ddata = arraydata(array);
    if (ddata == NULL) {
      break;
    }
    for (i = 0; i < n; i++) {
      val.val = ddata[i];
      math_equation_set_array_val(eq, array_id, i, &val);
    }
    rval->val = n;
    break;
  case NSARRAY:
    if (array_type != DATA_TYPE_STRING) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    if (getobj(obj, field, id, 0, NULL, &array) < 0) {
      rval->type = MATH_VALUE_ERROR;
      return 1;
    }
    n = arraynum(array);
    if (n < 0) {
      break;
    }
    sdata = arraydata(array);
    if (sdata == NULL) {
      break;
    }
    for (i = 0; i < n; i++) {
      char *str;
      str = sdata[i];
      if (str == NULL) {
	str = "";
      }
      math_equation_set_array_str(eq, array_id, i, str);
    }
    rval->val = n;
    break;
  default:
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  return 0;
}

int
math_func_parameter(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id;
  double prm;
  struct objlist *obj;

  rval->val = 0;
  rval->type = MATH_VALUE_ERROR;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  id = exp->buf[0].val.val;
  obj = chkobject("parameter");
  if (obj == NULL) {
    return 1;
  }
  if (getobj(obj, "parameter", id, 0, NULL, &prm) < 0) {
    return -1;
  }

  rval->type = MATH_VALUE_NORMAL;
  rval->val = prm;
  return 0;
}

int
math_func_strftime(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double date;
  const char *format;
  char *str;
  GString *gstr;
  int is_utc;
  time_t t;
  struct tm *tm;

  rval->val = 0;
  rval->type = MATH_VALUE_ERROR;

  gstr = math_expression_get_string_variable_from_argument(exp, 0);
  if (gstr == NULL) {
    return 0;
  }

  format = math_expression_get_string_from_argument(exp, 1);
  if (format == NULL) {
    return 0;
  }

  MATH_CHECK_ARG(rval, exp->buf[2]);
  t = exp->buf[2].val.val;

  MATH_CHECK_ARG(rval, exp->buf[3]);
  is_utc = exp->buf[3].val.val;

  if (is_utc) {
    tm = gmtime(&t);
  } else {
    tm = localtime(&t);
  }
  if (tm == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  date = mjd(tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday, tm->tm_hour, tm->tm_min, tm->tm_sec);
  str = nstrftime(format, date);
  if (str) {
    g_string_assign(gstr, str);
    if (g_utf8_validate(str, -1, NULL)) {
      rval->val = g_utf8_strlen(str, -1);
    }
    g_free(str);
  }

  rval->type = MATH_VALUE_NORMAL;
  return 0;
}

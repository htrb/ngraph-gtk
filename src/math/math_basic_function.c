#include <stdlib.h>
#include <math.h>
#include <error.h>
#include "math_equation.h"
#include "math_function.h"

#define MPI 3.14159265358979323846
#define MEXP1 2.71828182845905
#define MEULER 0.57721566490153286

#define MATH_CHECK_ARG(rval, v) if (v.val.type != MATH_VALUE_NORMAL) {	\
    *rval = v.val;							\
    return 0;								\
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
  double val;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  if (fabs(exp->buf[0].val.val - (int) exp->buf[0].val.val) >= 0.5) {
    if (exp->buf[0].val.val >= 0) {
      val = exp->buf[0].val.val - (exp->buf[0].val.val - (int) exp->buf[0].val.val) + 1;
    } else {
      val = exp->buf[0].val.val - (exp->buf[0].val.val - (int) exp->buf[0].val.val) - 1;
    }
  } else {
    val = exp->buf[0].val.val - (exp->buf[0].val.val - (int) exp->buf[0].val.val);
  }

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

  if (v < 1 || v > 1) {
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

  if (v < 1 || v > 1) {
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

int
math_func_icgam(MathFunctionCallExpression *expl, MathEquation *eq, MathValue *rval)
{
  double a, u, p0, p1, q0, q1, mu, x, val;
  int i, i2, i3;

  MATH_CHECK_ARG(rval, expl->buf[0]);
  MATH_CHECK_ARG(rval, expl->buf[1]);

  mu = expl->buf[0].val.val;
  x = expl->buf[1].val.val;

  if (x < 0 || mu < 0) {
    rval->type = MATH_VALUE_ERROR;
    return -1;
  }

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
}

int
math_func_gamma(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);

  return gamma2(exp->buf[0].val.val, &rval->val);
}

int
math_func_erfc(MathFunctionCallExpression *expl, MathEquation *eq, MathValue *rval)
{
  int i, i2, sg;
  double x, x2, x3, sum, h, h2, val;

  MATH_CHECK_ARG(rval, expl->buf[0]);

  x = expl->buf[0].val.val;

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
      sum -= 2 / (exp(2 * MPI * x2 / h) - 1);
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
  double x, x2, m2, c0, c1, y0, y1, val;

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

  rval->val =  (rand() / ((double) RAND_MAX + 1)) * exp->buf[0].val.val;
  return 0;
}

int
math_func_beta(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double p, q, a, b, c;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  p = exp->buf[0].val.val;
  q = exp->buf[1].val.val;

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
}

int
math_func_lgn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int i, n;
  double l1, l2, tmp1, tmp2, val, x, alp;

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

  l1 = 1;
  if (n == 0) {
    val = l1;
  } else {
    l2 = 1 - x + alp;
    if (n == 1) {
      val = l2;
    } else {
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
}

int
math_func_mjd(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int d, d1, d2, d3, d4;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);
  MATH_CHECK_ARG(rval, exp->buf[2]);

  d = (14 - exp->buf[1].val.val) / 12;
  d1 = (exp->buf[0].val.val - d) * 365.25;
  d2 = (exp->buf[1].val.val + d * 12 - 2) * 30.59;
  d3 = (exp->buf[0].val.val - d) / 100;
  d4 = (exp->buf[0].val.val - d) / 400;

  rval->val =  d1 + d2 - d3 + d4 + exp->buf[2].val.val + 1721088 - 2400000;

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
math_func_neq(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  rval->val = (exp->buf[0].val.val != exp->buf[1].val.val);
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
  MATH_CHECK_ARG(rval, exp->buf[1]);

  rval->val = ! exp->buf[0].val.val;
  return 0;
}

int
math_func_lt(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  rval->val = (exp->buf[0].val.val < exp->buf[1].val.val);
  return 0;
}

int
math_func_le(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  rval->val = (exp->buf[0].val.val <= exp->buf[1].val.val);
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
  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  rval->val =  (exp->buf[0].val.val >= exp->buf[1].val.val);
  return 0;
}

int
math_func_eq(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  rval->val = ( exp->buf[0].val.val == exp->buf[1].val.val);

  return 0;
}

int
math_func_tn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  double l1, l2, tmp1, x, val;
  int i, n;

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
    l2 = x;
    if (n == 1) {
      val = l2;
    } else {
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
  double l1, l2, val, x;
  int i, n;

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
    l2 = 2 * x;
    if (n == 1) {
      val = l2;
    } else {
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
  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  rval->val = (exp->buf[0].val.val >= exp->buf[1].val.val);
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
  double l1, l2, tmp1, tmp2, val;
  int i, n, x;

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
    l2 = x;
    if (n == 1) {
      val = l2;
    } else {
      for (i = 2; i <= n; i++) {
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
}

int
math_func_yn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n2, m, i, l, n;
  double x2, t1, t2, t3, s, ss, w, j0, j1, y0, y1, y2, x, val;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  x2 = fabs(x);
  n2 = abs(n);
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
}

int
math_func_jn(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n2, m, l, i, n;
  double x2, t1, t2, t3, s, j, x;

  MATH_CHECK_ARG(rval, exp->buf[0]);
  MATH_CHECK_ARG(rval, exp->buf[1]);

  n = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  x2 = fabs(x);
  n2 = abs(n);
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
}

int
math_func_ei(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MATH_CHECK_ARG(rval, exp->buf[0]);
  return exp1(exp->buf[0].val.val, &rval->val);
}

int
math_func_rm(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  n = exp->buf[0].val.val;

  if (n < 0 || n >= MATH_EQUATION_MEMORY_NUM) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  *rval = eq->memory[n];

  return 0;
}

int
math_func_m(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n;

  MATH_CHECK_ARG(rval, exp->buf[0]);

  n = exp->buf[0].val.val;

  if (n < 0 || n >= MATH_EQUATION_MEMORY_NUM) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  *rval = eq->memory[n] = exp->buf[1].val;

  return 0;
}

int
math_func_for(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n, r;
  double v;
  MathFunctionArgument *argv;

  argv = exp->buf;

  MATH_CHECK_ARG(rval, argv[0]);
  MATH_CHECK_ARG(rval, argv[1]);
  MATH_CHECK_ARG(rval, argv[2]);
  MATH_CHECK_ARG(rval, argv[3]);

  n = argv[0].val.val;
  if (n < 0 || n >= MATH_EQUATION_MEMORY_NUM || (argv[2].val.val - argv[1].val.val) * argv[3].val.val <= 0) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  
  eq->memory[n].type= MATH_VALUE_NORMAL;

  rval->val = 0;
  for (v = argv[1].val.val; (argv[3].val.val < 0) ?  (v >= argv[2].val.val) : (v <= argv[2].val.val); v += argv[3].val.val) {
    eq->memory[n].val = v;
    r = math_expression_calculate(argv[4].exp, rval);
    if(r)
      return r;
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
  MathValue *v1, *v2;

  v1 = (MathValue *) p1;
  v2 = (MathValue *) p2;

  if (v1->type == MATH_VALUE_NORMAL && v2->type == MATH_VALUE_NORMAL) {
    if (v1->val > v2->val) {
      return -1;
    } else if (v1->val < v2->val) {
      return 1;
    }
  } else if (v1->type != MATH_VALUE_NORMAL && v2->type == MATH_VALUE_NORMAL) {
    return -1;
  } else if (v1->type == MATH_VALUE_NORMAL && v2->type != MATH_VALUE_NORMAL) {
    return 1;
  }

  return 0;
}

int
math_func_sort(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id;
  MathEquationArray *ary;

  rval->val = 0;

  id = (int) exp->buf[0].idx;
  ary = math_equation_get_array(eq, id);

  if (ary == NULL || ary->num < 1 || ary->data == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = ary->num;

  if (ary->num < 2) {
    return 0;
  }

  qsort(ary->data, ary->num, sizeof(MathValue), compare_double);

  return 0;
}

int
math_func_rsort(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id;
  MathEquationArray *ary;

  rval->val = 0;

  id = (int) exp->buf[0].idx;
  ary = math_equation_get_array(eq, id);

  if (ary == NULL || ary->num < 1 || ary->data == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = ary->num;

  if (ary->num < 2) {
    return 0;
  }

  qsort(ary->data, ary->num, sizeof(double), rcompare_double);

  return 0;
}

int
math_func_size(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id;
  MathEquationArray *ary;

  rval->val = 0;

  id = (int) exp->buf[0].idx;
  ary = math_equation_get_array(eq, id);

  if (ary == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = ary->num;

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
    rval->val = exp->buf[0].val.val - ptr->val;
    rval->type = ptr->type;
    *ptr = exp->buf[0].val;
  } else {
    *rval = exp->buf[0].val;
  }

  return 0;
}

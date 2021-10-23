/*
 * $Id: ofit.c,v 1.37 2010-02-24 00:52:44 hito Exp $
 *
 * This file is part of "Ngraph for X11".
 *
 * Copyright (C) 2002, Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 *
 * "Ngraph for X11" is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * "Ngraph for X11" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <glib.h>

#include "nstring.h"
#include "ioutil.h"
#include "object.h"
#include "mathfn.h"
#include "oroot.h"
#include "ofit.h"
#include "odata.h"
#include "shell.h"

#include "math/math_equation.h"

#define NAME "fit"
#define PARENT "object"
#define OVERSION  "1.00.01"

#define ERRSYNTAX 100
#define ERRILLEGAL 101
#define ERRNEST   102
#define ERRMANYPARAM 103
#define ERRLN 104
#define ERRSMLDATA 105
#define ERRPOINT 106
#define ERRMATH 107
#define ERRSINGULAR 108
#define ERRNOEQS 109
#define ERRTHROUGH 110
#define ERRRANGE 111
#define ERRNEGATIVEWEIGHT 112
#define ERRCONVERGE 113
#define ERR_INCONSISTENT_DATA_NUM 114
#define ERRINTERRUPT 115

static char *fiterrorlist[]={
  "syntax error.",
  "not allowed function.",
  "sum() or dif(): deep nest.",
  "too many parameters.",
  "illegal data -> ignored.",
  "too small number of data.",
  "illegal point",
  "math error -> ignored.",
  "singular matrix.",
  "no fitting equation.",
  "`through_point' for type `user' is not supported.",
  "math range check.",
  "negative or zero weight -> ignored.",
  "convergence error.",
  "number of the data is not consistent.",
};

#define ERRNUM (sizeof(fiterrorlist) / sizeof(*fiterrorlist))

static char *fittypechar[]={
  N_("poly"),
  N_("pow"),
  N_("exp"),
  N_("log"),
  N_("user"),
  NULL
};

#define PRECISION_DISP 7
#define PRECISION_SAVE 15

struct fitlocal {
  int id, oid;
  MathEquation *codedf[FIT_DIMENSION_MAX];
  MathEquation *codef, *result_code;
  int dim;
  double coe[11];
  int num;
  double derror,correlation;
  char *equation;
};

static int
fitinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int div,dimension;
  double converge;
  struct fitlocal *fitlocal;
  int i,disp, oid;

  if (_getobj(obj, "oid", inst, &oid) == -1) return 1;
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  div=500;
  dimension=1;
  converge=1;
  disp=TRUE;
  if (_putobj(obj,"div",inst,&div)) return 1;
  if (_putobj(obj,"poly_dimension",inst,&dimension)) return 1;
  if (_putobj(obj,"converge",inst,&converge)) return 1;
  if (_putobj(obj,"display",inst,&disp)) return 1;

  if ((fitlocal=g_malloc(sizeof(struct fitlocal)))==NULL) return 1;
  fitlocal->codef=NULL;
  fitlocal->result_code = NULL;
  for (i=0;i<FIT_DIMENSION_MAX;i++) fitlocal->codedf[i]=NULL;
  fitlocal->equation=NULL;
  fitlocal->oid = oid;
  if (_putobj(obj,"_local",inst,fitlocal)) {
    g_free(fitlocal);
    return 1;
  }
  return 0;
}

static int
fitdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct fitlocal *fitlocal;
  int i;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"_local",inst,&fitlocal);
  g_free(fitlocal->equation);
  math_equation_free(fitlocal->codef);
  math_equation_free(fitlocal->result_code);
  for (i = 0; i < FIT_DIMENSION_MAX; i++)
    math_equation_free(fitlocal->codedf[i]);
  return 0;
}

static int
set_equation(struct objlist *obj, N_VALUE *inst, struct fitlocal *fitlocal, char *equation)
{
  int r;
  char *old_equation;

  _getobj(obj, "equation", inst, &old_equation);

  r = _putobj(obj, "equation", inst, equation);
  if (r) {
    return 1;
  }

  if (old_equation) {
    g_free(old_equation);
  }

  if (fitlocal && fitlocal->result_code) {
    math_equation_free(fitlocal->result_code);
    fitlocal->result_code = NULL;
  }

  return 0;
}

static void
show_eqn_error(struct objlist *obj, MathEquation *code, const char *math, const char *field, int rcode)
{
  char *err_msg;

  err_msg = math_err_get_error_message(code, math, rcode);
  if (err_msg) {
    error22(obj, ERRUNKNOWN, field, err_msg);
    g_free(err_msg);
  } else {
    error(obj, ERRSYNTAX);
  }
}

static int
fitequation(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv)
{
  struct fitlocal *fitlocal;

  _getobj(obj,"_local",inst,&fitlocal);

  if (fitlocal->result_code) {
    math_equation_free(fitlocal->result_code);
    fitlocal->result_code = NULL;
  }

  return 0;
}

static MathEquation *
create_math_equation(struct objlist *obj, char *math, const char *field)
{
  MathEquation *code;
  int rcode, security;
  enum EOEQ_ASSIGN_TYPE type;

  security = get_security();
  type = (security) ? EOEQ_ASSIGN_TYPE_BOTH : EOEQ_ASSIGN_TYPE_ASSIGN;
  code = ofile_create_math_equation(NULL, type, 2, FALSE, FALSE, FALSE, FALSE, TRUE);
  if (code == NULL) {
    return NULL;
  }

  rcode = math_equation_parse(code, math);
  if (rcode == 0 && code->use_eoeq_assign) {
    replace_eoeq_token(math);
  } else if (rcode) {
    show_eqn_error(obj, code, math, field, rcode);
    math_equation_free(code);
    return NULL;
  }

  if (math_equation_optimize(code)) {
    math_equation_free(code);
    return NULL;
  }
  return code;
}

static int
fitput(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
           int argc,char **argv)
{
  char *field;
  char *math;
  struct fitlocal *fitlocal;
  MathEquation *code;

  _getobj(obj,"_local",inst,&fitlocal);
  field=argv[1];
  if (strcmp(field,"poly_dimension")==0) {
    if (*(int *)(argv[2])<1) *(int *)(argv[2])=1;
    else if (*(int *)(argv[2])>=FIT_DIMENSION_MAX) *(int *)(argv[2])=FIT_DIMENSION_MAX - 1;
  } else if ((strcmp(field,"user_func")==0)
          || ((strncmp(field,"derivative",10)==0) && (field[10]!='\0'))) {
    math=(char *)(argv[2]);
    if (math!=NULL) {
      MathEquationParametar *prm;
      code = create_math_equation(obj, math, field);
      if (code == NULL) {
	return 1;
      }

      prm = math_equation_get_parameter(code, 0, NULL);
      if (prm == NULL) {
	math_equation_free(code);
	return 1;
      }

      if (prm->id_max >= FIT_DIMENSION_MAX) {
        error(obj,ERRMANYPARAM);
	math_equation_free(code);
        return 1;
      }
    } else {
      code=NULL;
    }
    if (field[0]=='u') {
      math_equation_free(fitlocal->codef);
      fitlocal->codef = code;
    } else {
      math_equation_free(fitlocal->codedf[field[10] - '0']);
      fitlocal->codedf[field[10] - '0'] = code;
    }
  }
  if (set_equation(obj, inst, fitlocal, NULL)) return 1;
  return 0;
}

static int
fit_put_weight_func(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
           int argc,char **argv)
{
  char *math;
  MathEquation *code;
  int rcode, security;
  enum EOEQ_ASSIGN_TYPE type;

  math = argv[2];
  if (math == NULL) {
    return 0;
  }

  g_strstrip(math);
  if (math[0] == '\0') {
    g_free(argv[2]);
    argv[2] = NULL;
    return 0;
  }

  security = get_security();
  type = (security) ? EOEQ_ASSIGN_TYPE_BOTH : EOEQ_ASSIGN_TYPE_ASSIGN;
  code = ofile_create_math_equation(NULL, type, 3, FALSE, TRUE, FALSE, FALSE, TRUE);
  if (code == NULL)
    return 1;

  rcode = math_equation_parse(code, math);
  if (rcode == 0 && code->use_eoeq_assign) {
    replace_eoeq_token(math);
  } else if (rcode) {
    show_eqn_error(obj, code, math, argv[1], rcode);
  }

  math_equation_free(code);

  return rcode;
}

enum FitError {
  FitError_Interrupt = -7,
  /* FitError_Convergence = -6, */
  FitError_Range    = -5,	/* range check */
  FitError_Syntax   = -4,	/* syntax error */
  FitError_Equation = -3,	/* equation is not specified */
  FitError_Matrix   = -2,	/* singular matrix */
  FitError_Small    = -1,	/* too small data */
  FitError_Success  =  0,	/* no error */
  FitError_Fatal    =  1,	/* fatal error */
};

static void
display_equation(const char *equation)
{
  if (equation == NULL)
    return;

  ndisplaydialog("\nEquation:\n");
  ndisplaydialog(equation);
  ndisplaydialog("\n");
}

static char *
get_poly_equation(struct fitlocal *fitlocal, enum FIT_OBJ_TYPE type, vector coe, int disp)
{
  GString *equation;
  int i, precision;

  equation = g_string_new("");
  if (equation == NULL) {
    return NULL;
  }

  precision = (disp) ? PRECISION_DISP : PRECISION_SAVE;
  switch (type) {
  case FIT_TYPE_POLY:
    for (i = fitlocal->dim - 1; i > 0; i--) {
      g_string_append_printf(equation,
			     (i == fitlocal->dim - 1) ? "%.*gx" : "%+.*gx",
			     precision, coe[i]);
      if (i > 1) {
	g_string_append_printf(equation, "^%d", i);
      }
    }
    g_string_append_printf(equation, "%+.*g", precision, coe[0]);
    break;
  case FIT_TYPE_POW:
    g_string_printf(equation, "exp(%.*g)x^%.*g", precision, coe[0], precision, coe[1]);
    break;
  case FIT_TYPE_EXP:
    g_string_printf(equation, "exp(%.*gx%+.*g)", precision, coe[1], precision, coe[0]);
    break;
  case  FIT_TYPE_LOG:
    g_string_printf(equation, "%.*gLn(x)%+.*g", precision, coe[1], precision, coe[0]);
    break;
  case FIT_TYPE_USER:
    /* never reached */
    break;
  }
  return g_string_free(equation, FALSE);
}

static int
show_poly_result(struct fitlocal *fitlocal, enum FIT_OBJ_TYPE type, vector coe)
{
  int j;
  GString *info;
  char *eqn;

  info = g_string_new("");
  if (info == NULL) {
    return 1;
  }

  g_string_append_printf(info, "--------\nfit:%d (^%d)\n", fitlocal->id, fitlocal->oid);
  switch (type) {
  case FIT_TYPE_POLY:
    g_string_append_printf(info,"Eq: %%0i*x^i (i=0-%d)\n\n", fitlocal->dim - 1);
    break;
  case FIT_TYPE_POW:
    g_string_append(info,"Eq: exp(%00)*x^%01\n\n");
    break;
  case FIT_TYPE_EXP:
    g_string_append(info,"Eq: exp(%01*x+%00)\n\n");
    break;
  case FIT_TYPE_LOG:
    g_string_append(info,"Eq: %01*Ln(x)+%00\n\n");
    break;
  case FIT_TYPE_USER:
    /* never reached */
    break;
  }
  for (j = 0; j < fitlocal->dim; j++) {
    g_string_append_printf(info, "       %%0%d = %+.7e\n", j, coe[j]);
  }
  g_string_append_c(info, '\n');
  g_string_append_printf(info, "    points = %d\n", fitlocal->num);
  g_string_append_printf(info, "    <DY^2> = %+.7e\n", fitlocal->derror);
  if (fitlocal->correlation >= 0) {
    g_string_append_printf(info, "|r| or |R| = %+.7e\n", fitlocal->correlation);
  } else {
    g_string_append(info, "|r| or |R| = -------------\n");
  }
  ndisplaydialog(info->str);
  g_string_free(info, TRUE);

  eqn = get_poly_equation(fitlocal, type, coe, TRUE);
  if (eqn == NULL) {
    return 1;
  }
  display_equation(eqn);
  g_free(eqn);

  return 0;
}

static enum FitError
fitpoly(struct fitlocal *fitlocal,
	enum FIT_OBJ_TYPE type,int dimension,int through,double x0,double y0,
	double *data,int num,int disp,int weight,double *wdata)
/*
  return
         FitError_Range
         FitError_Matrix
         FitError_Small
         FitError_Success
         FitError_Fatal
*/
{
  int i,j,k,dim;
  double yy,derror,sy,correlation,wt,sum;
  vector b,x1,x2,coe;
  matrix m;
  char *eqn;

  if (type == FIT_TYPE_POLY) dim=dimension+1;
  else dim=2;
  if (!through && (num<dim)) return FitError_Small;
  if (through && (num<dim+1)) return FitError_Small;
  yy=0;
  for (i=0;i<dim+1;i++) {
    b[i]=0;
    for (j=0;j<dim+1;j++) m[i][j]=0;
  }
  sum=0;
  for (k=0;k<num;k++) {
    double y1;
    if (weight) wt=wdata[k];
    else wt=1;
    sum+=wt;
    y1=data[k*2+1];
    x1[0]=1;
    for (i=1;i<dim;i++) {
      if ((fabs(x1[i-1])>1e100) || (fabs(data[k*2])>1e100)) return FitError_Range;
      x1[i]=x1[i-1]*data[k*2];
      if (fabs(x1[i])>1e100) return FitError_Range;
    }
    yy+=wt*y1*y1;
    if (fabs(yy)>1e100) return FitError_Range;
    for (i=0;i<dim;i++) {
      if (fabs(y1)>1e100) return FitError_Range;
      b[i]+=wt*y1*x1[i];
      if (fabs(b[i])>1e100) return FitError_Range;
    }
    for (i=0;i<dim;i++)
      for (j=0;j<dim;j++) {
        m[i][j]=m[i][j]+wt*x1[i]*x1[j];
        if (fabs(m[i][j])>1e100) return FitError_Range;
      }
  }
  if (through) {
    double y2;
    y2=y0;
    x2[0]=1;
    for (i=1;i<dim;i++) x2[i]=x2[i-1]*x0;
    for (i=0;i<dim;i++) {
      m[i][dim]=x2[i];
      m[dim][i]=x2[i];
    }
    b[dim]=y2;
    dim++;
  }
  if (matsolv(dim,m,b,coe)) return FitError_Matrix;
  derror=yy;
  for (i=0;i<dim;i++) derror-=coe[i]*b[i];
  derror=fabs(derror)/sum;
  sy=yy/sum-(b[0]/sum)*(b[0]/sum);
  if ((sy==0) || (1-derror/sy<0)) correlation=-1;
  else correlation=sqrt(1-derror/sy);
  derror=sqrt(derror);
  if (through) dim--;
  for (i=0;i<dim;i++) fitlocal->coe[i]=coe[i];
  for (;i<FIT_DIMENSION_MAX;i++) fitlocal->coe[i]=0;
  fitlocal->dim=dim;
  fitlocal->derror=derror;
  fitlocal->correlation=correlation;
  fitlocal->num=num;

  eqn = get_poly_equation(fitlocal, type, coe, FALSE);
  if (eqn == NULL) {
    return FitError_Fatal;
  }
  fitlocal->equation = eqn;

  if (disp && show_poly_result(fitlocal, type, coe)) {
    return FitError_Fatal;
  }


  return FitError_Success;
}

static int
show_user_result(struct fitlocal *fitlocal, const char *func, int dim, int *tbl, MathValue *par, int n, double dxxc, double derror, double correlation)
{
  GString *info;
  int j;

  info = g_string_sized_new(1024);
  if (info == NULL) {
    return 1;
  }
  g_string_append_printf(info, "--------\nfit:%d (^%d)\n", fitlocal->id, fitlocal->oid);
  g_string_append(info, "Eq: User defined\n");
  g_string_append_printf(info, "    %s\n\n", func);
  for (j = 0; j < dim; j++) {
    g_string_append_printf(info, "       %%0%d = %+.7e\n", tbl[j],  par[tbl[j]].val);
  }
  g_string_append_c(info, '\n');
  g_string_append_printf(info, "    points = %d\n", n);
  g_string_append_printf(info, "     delta = %+.7e\n", dxxc);
  g_string_append_printf(info, "    <DY^2> = %+.7e\n", derror);
  if (correlation >= 0) {
    g_string_append_printf(info, "|r| or |R| = %+.7e\n", correlation);
  } else {
    g_string_append(info, "|r| or |R| = -------------\n");
  }
  ndisplaydialog(info->str);
  g_string_free(info, TRUE);
  return 0;
}

static char *
get_user_equation(const char *func, MathValue *par, int disp)
{
  int i, prev_char, precision;
  GString *equation;

  equation = g_string_sized_new(256);
  if (equation == NULL) {
    return NULL;
  }

  precision = (disp) ? PRECISION_DISP : PRECISION_SAVE;
  prev_char = '\0';
  for (i = 0; func[i] != '\0'; i++) {
    double val;
    char *format;
    int prm_index;

    switch (func[i]) {
    case '%':
      if (isdigit(func[i + 1]) && isdigit(func[i + 2]) && isdigit(func[i + 3])) {
	prm_index = (func[i + 1] - '0') * 100 + (func[i + 2] - '0') * 10 + (func[i + 3] - '0');
	i += 3;
      } else if (isdigit(func[i + 1]) && isdigit(func[i + 2])) {
	prm_index = (func[i + 1] - '0') * 10 + (func[i + 2] - '0');
	i += 2;
      } else {
	prm_index = (func[i + 1] - '0');
	i += 1;
      }

      val = par[prm_index].val;
      switch (prev_char) {
      case '-':
	val = - val;
	/* fall-through */
      case '+':
	g_string_truncate(equation, equation->len - 1);
	format = "%+.*g";
	break;
      default:
	format = "%.*g";
      }
      prev_char = '\0';
      g_string_append_printf(equation, format, precision, val);
      break;
    case ' ':
      break;
    default:
      prev_char = func[i];
      g_string_append_c(equation, toupper(prev_char));
    }
  }
  return g_string_free(equation, FALSE);
}

static int
show_user_equation(struct fitlocal *fitlocal, const char *func, MathValue *par, int disp)
{
  char *eqn;
  if (disp) {
    eqn = get_user_equation(func, par, disp);
    if (eqn == NULL) {
      return 1;
    }
    display_equation(eqn);
    g_free(eqn);
  }
  eqn = get_user_equation(func, par, FALSE);
  if (eqn == NULL) {
    return 1;
  }
  fitlocal->equation = eqn;
  return 0;
}

static enum FitError
fituser(struct objlist *obj,struct fitlocal *fitlocal, const char *func,
	int deriv,double converge,double *data,int num,int disp,
	int weight,double *wdata)
/*
  return
         FitError_Interrupt
         FitError_Convergence
         FitError_Range
         FitError_syntax
         FitError_Equation
         FitError_Matrix
         FitError_Small
         FitError_Success
         FitError_Fatal
*/
{
  int ecode;
  int *needdata;
  int tbl[FIT_DIMENSION_MAX],dim,n,count,err,err2,err3;
  double yy,y,y1,y2,y3,sy,spx,spy,dxx,dxxc,xx,derror,correlation;
  vector b, x2, parerr;
  MathValue par[FIT_DIMENSION_MAX], par2[FIT_DIMENSION_MAX], var;
  MathEquationParametar *prm;
  matrix m;
  int i,j,k;
  char buf[1024];
  double wt,sum;
/*
  matrix m2;
  int count2;
  double sum2;
  double lambda,s0;
*/
  if (num<1) return FitError_Small;
  if (fitlocal->codef==NULL) return FitError_Equation;

  prm = math_equation_get_parameter(fitlocal->codef, 0, NULL);
  dim = prm->id_num;
  needdata = prm->id;
  if (deriv) {
    for (i = 0; i < dim; i++) {
      if (fitlocal->codedf[needdata[i]] == NULL) return FitError_Equation;
    }
  }
  for (i = 0; i < dim; i++) tbl[i] = needdata[i];
  for (i = 0; i < FIT_DIMENSION_MAX; i++) {
    par[i].type = MATH_VALUE_NORMAL;
    par[i].val = fitlocal->coe[i];
  }
  ecode=FitError_Success;
/*
  err2=FALSE;
  n=0;
  sum=0;
  yy=0;
  lambda=0.001;
  for (k=0;k<num;k++) {
    if (weight) wt=wdata[k];
    else wt=1;
    sum+=wt;
    spx=data[k*2];
    spy=data[k*2+1];
    err=FALSE;
    rcode=calculate(fitlocal->codef,1,spx,MATH_VALUE_NORMAL,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,
                    par,parstat,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                    NULL,NULL,NULL,0,NULL,NULL,NULL,0,&y1);
    if (rcode!=MATH_VALUE_NORMAL) err=TRUE;
    if (!err) {
      if (fabs(yy)>1e100) return FitError_Range;
      y2=spy-y1;
      if ((fabs(y2)>1e50) || (fabs(spy)>1e50)) err=TRUE;
    }
    if (!err) {
      n++;
      yy+=wt*y2*y2;
    } else err2=TRUE;
  }
  if (err2) error(obj,ERRMATH);
  if (n<1) {
    ecode=FitError_Small;
    goto errexit;
  }
  s0=yy/sum;
*/

  count=0;
  err3=FALSE;
  do {
    yy=0;
    y=0;
    y3=0;
    for (i=0;i<dim;i++) {
      b[i]=0;
      for (j=0;j<dim;j++) m[j][i]=0;
    }
    err2=FALSE;
    n=0;
    sum=0;
    for (k=0;k<num;k++) {
      if (weight) wt=wdata[k];
      else wt=1;
      sum+=wt;
      spx=data[k*2];
      spy=data[k*2+1];
      err=FALSE;
      var.val = spx;
      var.type = MATH_VALUE_NORMAL;
      math_equation_set_parameter_data(fitlocal->codef, 0, par);
      math_equation_set_var(fitlocal->codef, 0, &var);
      math_equation_calculate(fitlocal->codef, &var);
      y1 = var.val;
      if (var.type!=MATH_VALUE_NORMAL) err=TRUE;
      if (deriv) {
        for (j=0;j<dim;j++) {
	  var.val = spx;
	  var.type = MATH_VALUE_NORMAL;
	  math_equation_set_parameter_data(fitlocal->codedf[tbl[j]], 0, par);
	  math_equation_set_var(fitlocal->codedf[tbl[j]], 0, &var);
	  math_equation_calculate(fitlocal->codedf[tbl[j]], &var);
	  x2[j] = var.val;
	  if (var.type!=MATH_VALUE_NORMAL) err=TRUE;
        }
      } else {
        for (j=0;j<dim;j++) {
          for (i=0;i<FIT_DIMENSION_MAX;i++) par2[i]=par[i];
          dxx = par2[j].val * converge * 1e-6;
          if (dxx == 0) dxx = 1e-6;
          par2[tbl[j]].val += dxx;
	  var.val = spx;
	  var.type = MATH_VALUE_NORMAL;
	  math_equation_set_parameter_data(fitlocal->codef, 0, par2);
	  math_equation_set_var(fitlocal->codef, 0, &var);
	  math_equation_calculate(fitlocal->codef, &var);
	  x2[j] = var.val;
	  if (var.type!=MATH_VALUE_NORMAL) err=TRUE;
          x2[j]=(x2[j]-y1)/dxx;
        }
      }
      if (!err) {
        if ((fabs(yy)>1e100) || (fabs(y3)>1e100)) return FitError_Range;
        y2=spy-y1;
        if ((fabs(y2)>1e50) || (fabs(spy)>1e50)) err=TRUE;
      }
      if (!err) {
        n++;
        yy+=wt*y2*y2;
        y+=wt*spy;
        y3+=wt*spy*spy;
        for (j=0;j<dim;j++) b[j]+=wt*x2[j]*y2;
        for (j=0;j<dim;j++)
          for (i=0;i<dim;i++) m[j][i]=m[j][i]+wt*x2[j]*x2[i];
      } else err2=TRUE;
    }
    if (!err3 && err2) {
      error(obj,ERRMATH);
      err3=TRUE;
    }
    if (n<1) {
      ecode=FitError_Small;
      goto errexit;
    }

    count++;

    derror=fabs(yy)/sum;
    sy=y3/sum-(y/sum)*(y/sum);
    if ((sy==0) || (1-derror/sy<0)) correlation=-1;
    else correlation=sqrt(1-derror/sy);
    if (correlation>1) correlation=-1;
    derror=sqrt(derror);
    /*
    if (disp) {
      i=0;
      i += sprintf(buf + i, "fit:^%d\n", fitlocal->oid);
      i+=sprintf(buf+i,"Eq: User defined\n\n");
      for (j=0;j<dim;j++)
        i+=sprintf(buf+i,"       %%0%d = %+.7e\n",tbl[j],par[tbl[j]]);
      i+=sprintf(buf+i,"\n");
      i+=sprintf(buf+i,"    points = %d\n",n);
      if (count==1) i+=sprintf(buf+i,"     delta = \n");
      else i+=sprintf(buf+i,"     delta = %+.7e\n",dxxc);
      i+=sprintf(buf+i,"    <DY^2> = %+.7e\n",derror);
      if (correlation>=0)
        i+=sprintf(buf+i,"|r| or |R| = %+.7e\n",correlation);
      else
        i+=sprintf(buf+i,"|r| or |R| = -------------\n");
      i+=sprintf(buf+i," Iteration = %d\n",count);
      ndisplaydialog(buf);
    }
    */
    if (count & 0x100) {
      sprintf(buf,"fit:^%d Iteration = %d delta = %g", fitlocal->oid, count, dxxc);
      set_progress(0, buf, -1);
    }
    if (ninterrupt()) {
      ecode=FitError_Interrupt;
      goto errexit;
    }

/*
    count2=0;
    sum2=sum;
    while (TRUE) {
      count2++;
      sprintf(buf,"Iteration = %d:%d",count,count2);
      ndisplaystatus(buf);
      for (j=0;j<dim;j++)
        for (i=0;i<dim;i++) m2[j][i]=m[j][i];
      for (i=0;i<dim;i++) m2[i][i]=m[i][i]+sum2*lambda;
      if (matsolv(dim,m2,b,parerr)) goto repeat;
      for (i=0;i<dim;i++) par2[tbl[i]]=par[tbl[i]]+parerr[i];
      n=0;
      err2=FALSE;
      sum=0;
      yy=0;
      for (k=0;k<num;k++) {
        if (weight) wt=wdata[k];
        else wt=1;
        sum+=wt;
        spx=data[k*2];
        spy=data[k*2+1];
        err=FALSE;
        rcode=calculate(fitlocal->codef,1,spx,MATH_VALUE_NORMAL,0,0,0,0,
                        0,0,0,0,0,0,0,0,0,0,0,0,
                     par2,parstat,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                        NULL,NULL,NULL,0,NULL,NULL,NULL,0,&y1);
        if (rcode!=MATH_VALUE_NORMAL) err=TRUE;
        if (!err) {
          if (fabs(yy)>1e100) goto repeat;
          y2=spy-y1;
          if ((fabs(y2)>1e50) || (fabs(spy)>1e50)) err=TRUE;
        }
        if (!err) {
          n++;
          yy+=wt*y2*y2;
        }
      }
      if (n<1) goto repeat;
      if (yy/sum<=s0) {
        s0=yy/sum;
        break;
      }
repeat:
      lambda*=10;
      if (lambda>1e100) return FitError_Convergence;
    }

    lambda/=10;
*/
    if (matsolv(dim,m,b,parerr)) return FitError_Matrix;

    dxxc=0;
    xx=0;
    for (i=0;i<dim;i++) {
      dxxc += parerr[i] * parerr[i];
      par[tbl[i]].val += parerr[i];
      xx += par[tbl[i]].val * par[tbl[i]].val;
    }
    dxxc=sqrt(dxxc);
    xx=sqrt(xx);


  } while ((dxxc>xx*converge/100) && ((xx>1e-6) || (dxxc>1e-6*converge/100)));

  if (disp && show_user_result(fitlocal, func, dim, tbl, par, n, dxxc, derror, correlation)) {;
    return FitError_Fatal;
  }

errexit:
  if ((ecode==FitError_Success) || (ecode==FitError_Range)) {
    for (i = 0; i < FIT_DIMENSION_MAX; i++) {
      fitlocal->coe[i] = par[i].val;
    }
    fitlocal->dim=dim;
    fitlocal->derror=derror;
    fitlocal->correlation=correlation;
    fitlocal->num=n;
    if (show_user_equation(fitlocal, func, par, disp)) {
      ecode = FitError_Fatal;
    }
  }
  return ecode;
}

static int
fitfit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct fitlocal *fitlocal;
  int i,through,dimension,deriv,disp;
  enum FIT_OBJ_TYPE type;
  double x0,y0,converge,wt;
  struct narray *darray;
  double *data,*wdata;
  char *equation,*func,prm[32];
  int dnum,num,err,err2,err3;
  enum FitError rcode;
  double derror,correlation,pp;
  int ecode;
  int weight,anum;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"_local",inst,&fitlocal);
  if (set_equation(obj, inst, fitlocal, NULL)) return 1;
  equation = g_strdup("undef");
  if (equation == NULL) return 1;
  if (set_equation(obj, inst, fitlocal, equation)) {
    g_free(equation);
    return 1;
  }
  num=0;
  derror=0;
  correlation=0;
  pp=0;
  if (_putobj(obj,"number",inst,&num)) return 1;
  if (_putobj(obj,"error",inst,&derror)) return 1;
  if (_putobj(obj,"correlation",inst,&correlation)) return 1;

  for (i = 0; i < 10; i++) {
    snprintf(prm, sizeof(prm), "%%%02d", i);
    if (_putobj(obj, prm, inst, &pp)) return 1;
  }

  _getobj(obj,"type",inst,&type);
  _getobj(obj,"through_point",inst,&through);
  _getobj(obj,"point_x",inst,&x0);
  _getobj(obj,"point_y",inst,&y0);
  _getobj(obj,"poly_dimension",inst,&dimension);

  _getobj(obj,"user_func",inst,&func);
  _getobj(obj,"derivative",inst,&deriv);
  _getobj(obj,"converge",inst,&converge);
  _getobj(obj, "id", inst, &(fitlocal->id));

  for (i = 0; i < 10; i++) {
    snprintf(prm, sizeof(prm), "parameter%d", i);
    _getobj(obj, prm, inst, &(fitlocal->coe[i]));
  }

  _getobj(obj,"display",inst,&disp);

  if (through && (type == FIT_TYPE_USER)) {
    error(obj,ERRTHROUGH);
    return 1;
  }

  darray = (struct narray *) (argv[2]);
  if (arraynum(darray) < 1)
    return FitError_Small;

  data=arraydata(darray);
  anum=arraynum(darray)-1;
  dnum=nround(data[0]);
  data += 1;
  if (dnum == (anum / 2)) {
    weight = FALSE;
    wt = 0; 			/* dummy code to avoid compile warnings */
    wdata = NULL;		/* dummy code to avoid compile warnings */
  } else if (dnum == (anum / 3)) {
    weight = TRUE;
    wdata = data + 2 * dnum;
  } else {
    error(obj, ERR_INCONSISTENT_DATA_NUM);
    return 1;
  }

  num=0;
  err2=err3=FALSE;
  for (i=0;i<dnum;i++) {
    double x, y;
    x=data[i*2];
    y=data[i*2+1];
    if (weight) {
      wt = wdata[i];
    }
    err=FALSE;
    switch (type) {
    case  FIT_TYPE_POW:
      if (y<=0) err=TRUE;
      else y=log(y);
      if (x<=0) err=TRUE;
      else x=log(x);
      break;
    case FIT_TYPE_EXP:
      if (y<=0) err=TRUE;
      else y=log(y);
      break;
    case FIT_TYPE_LOG:
      if (x<=0) err=TRUE;
      else x=log(x);
      break;
    case FIT_TYPE_POLY:
    case FIT_TYPE_USER:
      /* nothing to do */
      break;
    }
    if (err) {
      err2 = TRUE;
    } else if (weight && (wt <= 0)) {
      err=TRUE;
      err3=TRUE;
    }
    if (!err) {
      data[num*2]=x;
      data[num*2+1]=y;
      if (weight) {
	wdata[num] = wt;
      }
      num++;
    }
  }
  if (err2) error(obj,ERRLN);
  if (err3) error(obj,ERRNEGATIVEWEIGHT);
  if (through) {
    err=FALSE;
    switch (type) {
    case FIT_TYPE_POW:
      if (y0<=0) err=TRUE;
      else y0=log(y0);
      if (x0<=0) err=TRUE;
      else x0=log(x0);
      break;
    case FIT_TYPE_EXP:
      if (y0<=0) err=TRUE;
      else y0=log(y0);
      break;
    case FIT_TYPE_LOG:
      if (x0<=0) err=TRUE;
      else x0=log(x0);
      break;
    case FIT_TYPE_POLY:
      /* nothing to do */
      break;
    case FIT_TYPE_USER:
      /* never reached */
      break;
    }
    if (err) {
      error(obj,ERRPOINT);
      return 1;
    }
  }

  if (type != FIT_TYPE_USER) {
    rcode=fitpoly(fitlocal,type,dimension,through,x0,y0,data,num,disp,weight,wdata);
  } else {
    rcode=fituser(obj,fitlocal,func,deriv,converge,data,num,disp,weight,wdata);
  }

  switch (rcode) {
  case FitError_Fatal:
    return 1;
    break;
  case FitError_Small:
    ecode = ERRSMLDATA;
    break;
  case FitError_Matrix:
    ecode = ERRSINGULAR;
    break;
  case FitError_Equation:
    ecode = ERRNOEQS;
    break;
  case FitError_Syntax:
    ecode = ERRSYNTAX;
    break;
  case FitError_Range:
    ecode = ERRRANGE;
    break;
    /*
      case FitError_Convergence:
      ecode = ERRCONVERGE;
      break;
    */
  case FitError_Interrupt:
    ecode = ERRINTERRUPT;
    break;
  default:
    ecode = 0;
  }
  if (ecode!=0) {
    error(obj,ecode);
    return 1;
  }
  if (_putobj(obj,"number",inst,&(fitlocal->num))) return 1;
  if (_putobj(obj,"error",inst,&(fitlocal->derror))) return 1;
  if (_putobj(obj,"correlation",inst,&(fitlocal->correlation))) return 1;

  for (i = 0; i < 10; i++) {
    snprintf(prm, sizeof(prm), "%%%02d", i);
    if (_putobj(obj, prm, inst, &(fitlocal->coe[i]))) return 1;
  }

  if (set_equation(obj, inst, fitlocal, fitlocal->equation)) return 1;
  fitlocal->equation = NULL;

  return 0;
}

static int
fitcalc(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  MathValue val;
  int r;
  char *equation;
  double x;
  struct fitlocal *fitlocal;

  if (_exeparent(obj, argv[1], inst, rval, argc, argv)) return 1;

  rval->d = 0;

  x = arg_to_double(argv, 2);

  _getobj(obj,"_local",inst,&fitlocal);
  if (fitlocal->result_code == NULL) {
    MathEquation *eq;
    _getobj(obj, "equation", inst, &equation);
    if (equation == NULL) {
      return 0;
    }

    eq = ofile_create_math_equation(NULL, EOEQ_ASSIGN_TYPE_ASSIGN, 0, FALSE, FALSE, FALSE, FALSE, TRUE);
    if (eq == NULL) {
      return 1;
    }

    if (math_equation_parse(eq, equation)) {
      math_equation_free(eq);
      return 1;
    }

    math_equation_optimize(eq);
    fitlocal->result_code = eq;
  }

  val.val = x;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_var(fitlocal->result_code, 0, &val);

  r = math_equation_calculate(fitlocal->result_code, &val);
  if (r) {
    return 1;
  }

  rval->d = val.val;

  return 0;
}

static int
fit_inst_dup(struct objlist *obj, N_VALUE *src, N_VALUE *dest)
{
  int i, pos, len;
  struct fitlocal *local_new, *local_src;
  char deriv[] = "derivative0", *math;

  pos = obj_get_field_pos(obj, "_local");
  if (pos < 0) {
    return 1;
  }
  local_src = src[pos].ptr;
#if GLIB_CHECK_VERSION(2, 68, 0)
  local_new = g_memdup2(local_src, sizeof(struct fitlocal));
#else
  local_new = g_memdup(local_src, sizeof(struct fitlocal));
#endif
  if (local_new == NULL) {
    return 1;
  }
  dest[pos].ptr = local_new;
  local_new->equation = g_strdup(local_src->equation);
  local_new->result_code = NULL;
  if (local_src->codef) {
    _getobj(obj, "user_func", src, &math);
    local_new->codef = create_math_equation(obj, math, "user_func");
    if (local_new->codef == NULL) {
      return 1;
    }
  }
  len = sizeof(deriv);
  for (i = 0; i < 10; i++) {
    deriv[len - 2] = '0' + i;
    if (local_src->codedf[i]) {
      _getobj(obj, deriv, src, &math);
      local_new->codedf[i] = create_math_equation(obj, math, deriv);
      if (local_new->codedf[i] == NULL) {
	return 1;
      }
    }
  }
  return 0;
}

static int
fit_inst_free(struct objlist *obj, N_VALUE *inst)
{
  int i, pos;
  struct fitlocal *local;

  pos = obj_get_field_pos(obj, "_local");
  if (pos < 0) {
    return 1;
  }
  local = inst[pos].ptr;
  if (local->equation) {
    g_free(local->equation);
  }
  if (local->result_code) {
    math_equation_free(local->result_code);
  }
  if (local->codef) {
    math_equation_free(local->codef);
  }
  for (i = 0; i < 10; i++) {
    if (local->codedf[i]) {
      math_equation_free(local->codedf[i]);
    }
  }
  g_free(local);
  return 0;
}

static struct objtable fit[] = {
  {"init",NVFUNC,NEXEC,fitinit,NULL,0},
  {"done",NVFUNC,NEXEC,fitdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"profile",NSTR,NREAD|NWRITE,NULL,NULL,0},

  {"type",NENUM,NREAD|NWRITE,fitput,fittypechar,0},
  {"min",NDOUBLE,NREAD|NWRITE,NULL,NULL,0},
  {"max",NDOUBLE,NREAD|NWRITE,NULL,NULL,0},
  {"div",NINT,NREAD|NWRITE,oputge1,NULL,0},
  {"interpolation",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"through_point",NBOOL,NREAD|NWRITE,fitput,NULL,0},
  {"point_x",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"point_y",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"equation",NSTR,NREAD|NWRITE,fitequation,NULL,0},

  {"poly_dimension",NINT,NREAD|NWRITE,fitput,NULL,0},

  {"weight_func",NSTR,NREAD|NWRITE,fit_put_weight_func,NULL,0},
  {"user_func",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative",NBOOL,NREAD|NWRITE,fitput,NULL,0},
  {"derivative0",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative1",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative2",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative3",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative4",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative5",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative6",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative7",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative8",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative9",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"converge",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter0",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter1",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter2",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter3",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter4",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter5",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter6",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter7",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter8",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter9",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"%00",NDOUBLE,NREAD,NULL,NULL,0},
  {"%01",NDOUBLE,NREAD,NULL,NULL,0},
  {"%02",NDOUBLE,NREAD,NULL,NULL,0},
  {"%03",NDOUBLE,NREAD,NULL,NULL,0},
  {"%04",NDOUBLE,NREAD,NULL,NULL,0},
  {"%05",NDOUBLE,NREAD,NULL,NULL,0},
  {"%06",NDOUBLE,NREAD,NULL,NULL,0},
  {"%07",NDOUBLE,NREAD,NULL,NULL,0},
  {"%08",NDOUBLE,NREAD,NULL,NULL,0},
  {"%09",NDOUBLE,NREAD,NULL,NULL,0},
  {"number",NINT,NREAD,NULL,NULL,0},
  {"error",NDOUBLE,NREAD,NULL,NULL,0},
  {"correlation",NDOUBLE,NREAD,NULL,NULL,0},
  {"display",NBOOL,NREAD|NWRITE,NULL,NULL,0},

  {"fit",NVFUNC,NREAD|NEXEC,fitfit,"da",0},
  {"calc",NDFUNC,NREAD|NEXEC,fitcalc,"d",0},
  {"_local",NPOINTER,0,NULL,NULL,0},
};

#define TBLNUM (sizeof(fit) / sizeof(*fit))

void *
addfit(void)
/* addfit() returns NULL on error */
{
  struct objlist *fit_obj;
  fit_obj = addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,fit,ERRNUM,fiterrorlist,NULL,NULL);
  obj_set_undo_func(fit_obj, fit_inst_dup, fit_inst_free);
  return fit_obj;
}

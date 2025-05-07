/*
 * $Id: omath.c,v 1.22 2010-03-04 08:30:16 hito Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <glib.h>

#include "object.h"
#include "mathcode.h"
#include "mathfn.h"

#include "math/math_equation.h"

#define NAME "math"
#define PARENT "object"
#define OVERSION  "1.00.00"

#define ERRSYNTAX 100
#define ERRILLEGAL 101
#define ERRNEST   102
#define ERRARG    103
#define ERRSMLARG 104

static char *matherrorlist[]={
  "syntax error.",
  "not allowed function.",
  "sum() or dif(): deep nest.",
  "illegal argument",
  "not enough argument."
};

#define ERRNUM (sizeof(matherrorlist) / sizeof(*matherrorlist))

struct mlocal {
  double x;
  double y;
  double z;
  double val;
  int maxdim;
  MathEquation *code;
  int rcode;
  int idpx;
  int idpy;
  int idpz;
  int idpr;
};

static void
msettbl(N_VALUE *inst, const struct mlocal *mlocal)
{
  inst[mlocal->idpx].d=mlocal->x;
  inst[mlocal->idpy].d=mlocal->y;
  inst[mlocal->idpz].d=mlocal->z;
  inst[mlocal->idpr].i=mlocal->rcode;
}

static void
mlocalclear(struct mlocal *mlocal)
{
  math_equation_clear(mlocal->code);
}

static int
minit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct mlocal *mlocal;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  if ((mlocal=g_malloc(sizeof(struct mlocal)))==NULL) goto errexit;
  if (_putobj(obj,"_local",inst,mlocal)) goto errexit;
  mlocal->x=0;
  mlocal->y=0;
  mlocal->z=0;
  mlocal->val=0;

  mlocal->code = math_equation_basic_new();
  if (mlocal->code == NULL)
    goto errexit;

  if (math_equation_add_parameter(mlocal->code, 0, 1, 2, MATH_EQUATION_PARAMETAR_USE_ID)) {
    math_equation_free(mlocal->code);
    goto errexit;
  }

  if (math_equation_add_var(mlocal->code, "X") != 0) {
    math_equation_free(mlocal->code);
    goto errexit;
  }

  if (math_equation_add_var(mlocal->code, "Y") != 1) {
    math_equation_free(mlocal->code);
    goto errexit;
  }

  if (math_equation_add_var(mlocal->code, "Z") != 2) {
    math_equation_free(mlocal->code);
    goto errexit;
  }

  math_equation_parse(mlocal->code, "def f(x,y,z){0}");
  math_equation_parse(mlocal->code, "def g(x,y,z){0}");
  math_equation_parse(mlocal->code, "def h(x,y,z){0}");

  mlocal->rcode=MATH_VALUE_NORMAL;
  mlocalclear(mlocal);
  if (_putobj(obj,"formula",inst,NULL)) goto errexit;
  if (_putobj(obj,"f",inst,NULL)) goto errexit;
  if (_putobj(obj,"g",inst,NULL)) goto errexit;
  if (_putobj(obj,"h",inst,NULL)) goto errexit;
  mlocal->idpx=chkobjoffset(obj,"x");
  mlocal->idpy=chkobjoffset(obj,"y");
  mlocal->idpz=chkobjoffset(obj,"z");

  mlocal->idpr=chkobjoffset(obj,"status");
  msettbl(inst,mlocal);
  return 0;

errexit:
  g_free(mlocal);
  return 1;
}

static int
mdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct mlocal *mlocal;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"_local",inst,&mlocal);
  math_equation_free(mlocal->code);
  return 0;
}

static char *
create_func_def_str(char *name, char *code)
{
  return g_strdup_printf("def %s(x,y,z) {%s;}", name, code);
}

static void
parse_original_formula(struct objlist *obj,N_VALUE *inst, struct mlocal *mlocal)
{
  char *ptr;
  const MathEquationParametar *prm;

  _getobj(obj, "formula", inst, &ptr);
  math_equation_parse(mlocal->code, ptr);
  math_equation_optimize(mlocal->code);

  prm = math_equation_get_parameter(mlocal->code, 0, NULL);
  if (prm == NULL) {
    mlocal->maxdim = 0;
  } else {
    mlocal->maxdim = prm->id_max;
  }
}

/*
   # following procedure causes error.

   new math
   math::formula="const A:1;A"
   get math: calc                  # calc:1.000000000000000e+00
   math::formula="const A=1;A"
   get math: formula               # formula:const A:1;A
   get math: calc                  # calc:0.000000000000000e+00
 */
static int
mformula(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *math;
  struct mlocal *mlocal;
  int rcode;
  char *err_msg;
  const MathEquationParametar *prm;

  math=argv[2];
  _getobj(obj,"_local",inst,&mlocal);
  if (strcmp("formula",argv[1])==0) {
    if (math) {
      rcode = math_equation_parse(mlocal->code, math);
      if (rcode) {
	err_msg = math_err_get_error_message(mlocal->code, math, rcode);
	error22(obj, ERRUNKNOWN, argv[1], err_msg);
	g_free(err_msg);
	parse_original_formula(obj, inst, mlocal);
	return 1;
      }
      if (math_equation_optimize(mlocal->code)) {
	parse_original_formula(obj, inst, mlocal);
	return 1;
      }

      prm = math_equation_get_parameter(mlocal->code, 0, NULL);
      if (prm == NULL) {
	mlocal->maxdim = 0;
      } else {
	mlocal->maxdim = prm->id_max;
      }
    }
  } else {
    char *ptr;
    if (math) {
      ptr = create_func_def_str(argv[1], math);
    } else {
      ptr = create_func_def_str(argv[1], "0");
    }
    if (ptr == NULL) {
      return 1;
    }
    rcode = math_equation_parse(mlocal->code, ptr);
    if (rcode) {
      err_msg = math_err_get_error_message(mlocal->code, math, rcode);
      error22(obj, ERRUNKNOWN, argv[1], err_msg);
      g_free(err_msg);
      parse_original_formula(obj, inst, mlocal);
      return 1;
    }
    if (math_equation_optimize(mlocal->code)) {
      parse_original_formula(obj, inst, mlocal);
      return 1;
    }
    g_free(ptr);

    parse_original_formula(obj, inst, mlocal);
  }
  msettbl(inst,mlocal);
  return 0;
}

static int
mparam(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  const char *arg;
  struct mlocal *mlocal;

  _getobj(obj, "_local", inst, &mlocal);
  arg = argv[1];
  switch (arg[0]) {
  case 'x':
    mlocal->x = arg_to_double(argv, 2);
    break;
  case 'y':
    mlocal->y = arg_to_double(argv, 2);
    break;
  case 'z':
    mlocal->z = arg_to_double(argv, 2);
    break;
  }
  msettbl(inst, mlocal);
  return 0;
}

static int
mcalc(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct mlocal *mlocal;
  int i,num;
  const double *adata;
  struct narray *darray;
  int maxdim;
  MathValue val, *data;

  _getobj(obj,"_local",inst,&mlocal);
  maxdim=mlocal->maxdim;
  darray=(struct narray *)argv[2];
  num=arraynum(darray);
  adata=arraydata(darray);
  if (num<maxdim) {
    error(obj,ERRSMLARG);
    return 1;
  }

  data = g_malloc(sizeof(MathValue) * (num + 1));

  if (data == NULL) {
    g_free(data);
    return 1;
  }

  data[0].val = 0;
  data[0].type = MATH_VALUE_NORMAL;
  for (i=0; i < num; i++) {
    data[i + 1].val = adata[i];
    data[i + 1].type = MATH_VALUE_NORMAL;
  }

  val.type = MATH_VALUE_NORMAL;

  val.val = mlocal->x;
  math_equation_set_var(mlocal->code, 0, &val);

  val.val = mlocal->y;
  math_equation_set_var(mlocal->code, 1, &val);

  val.val = mlocal->z;
  math_equation_set_var(mlocal->code, 2, &val);

  math_equation_set_parameter_data(mlocal->code, 0, data);
  math_equation_calculate(mlocal->code, &val);
  mlocal->val = val.val;
  mlocal->rcode = val.type;
  g_free(data);
  msettbl(inst,mlocal);
  rval->d=mlocal->val;
  return 0;
}

static int
mclear(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct mlocal *mlocal;

  _getobj(obj,"_local",inst,&mlocal);
  mlocalclear(mlocal);
  msettbl(inst,mlocal);
  return 0;
}


static struct objtable math_obj[] = {
  {"init",NVFUNC,NEXEC,minit,NULL,0},
  {"done",NVFUNC,NEXEC,mdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"formula",NSTR,NREAD|NWRITE,mformula,NULL,0},
  {"f",NSTR,NREAD|NWRITE,mformula,NULL,0},
  {"g",NSTR,NREAD|NWRITE,mformula,NULL,0},
  {"h",NSTR,NREAD|NWRITE,mformula,NULL,0},
  {"x",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"y",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"z",NDOUBLE,NREAD|NWRITE,mparam,NULL,0},
  {"status",NENUM,NREAD,NULL,matherrorchar,0},
  {"calc",NDFUNC,NREAD|NEXEC,mcalc,"da",0},
  {"clear",NVFUNC,NREAD|NEXEC,mclear,"",0},
  {"_local",NPOINTER,0,NULL,NULL,0},
};

#define TBLNUM (sizeof(math_obj) / sizeof(*math_obj))

void *
addmath(void)
/* addmath() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,math_obj,ERRNUM,matherrorlist,NULL,NULL);
}

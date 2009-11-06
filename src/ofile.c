/* 
 * $Id: ofile.c,v 1.96 2009/11/06 03:50:13 hito Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <fcntl.h>
#include <utime.h>
#include <time.h>
#include <errno.h>
#ifndef WINDOWS
#include <unistd.h>
#else
#include <io.h>
#include <sys/stat.h>
#endif

#include "common.h"

#include "ngraph.h"
#include "nhash.h"
#include "object.h"
#include "ioutil.h"
#include "nstring.h"
#include "mathcode.h"
#include "mathfn.h"
#include "spline.h"
#include "gra.h"
#include "ntime.h"
#include "oroot.h"
#include "odraw.h"
#include "ofile.h"
#include "axis.h"
#include "nconfig.h"

#include "math/math_equation.h"

#define MATH_CONST_SIZE		17
#define TWOPASS_CONST_SIZE	(MATH_CONST_SIZE - 3)

#define NAME		"file"
#define ALIAS		"data"
#define PARENT		"draw"
#define OVERSION	"1.00.00"
#define F2DCONF		"[file]"
#define TRUE  1
#define FALSE 0

#define ERRFILE		100
#define ERROPEN		101
#define ERRSYNTAX	102
#define ERRILLEGAL	103
#define ERRNEST		104
#define ERRNOAXIS	105
#define ERRNOAXISINST	106
#define ERRNOSETAXIS	107
#define ERRMINMAX	108
#define ERRAXISDIR	109
#define ERRMSYNTAX	110
#define ERRMERR		111
#define ERRMNONUM	112
#define ERRSPL		113
#define ERRNOFIT	114
#define ERRNOFITINST	115
#define ERRIFS		116
#define ERRILOPTION	117
#define ERRIGNORE	118
#define ERRNEGATIVE	119
#define ERRREAD		120
#define ERRWRITE	121

#if ! NEW_MATH_CODE
static int MathErrorCodeArray[MATH_CODE_ERROR_NUM];
#endif

static char *f2derrorlist[]={
  "file is not specified.",
  "I/O error: open file",
  "syntax error in math.",
  "not allowed function in math.",
  "sum() or dif(): deep nest in math.",
  "`axis' is not specified.",
  "no instance for axis",
  "axis parameter is not set.",
  "illegal axis min/max.",
  "illegal axis direction.",
  "syntax error in math:",
  "illegal value in math:",
  "unnumeric data:",
  "error: spline interpolation.",
  "`fit' is not specified.",
  "no instance for fit",
  "illegal ifs.",
  "illegal file option",
  "illegal value for axis ---> ignored",
  "negative value in LOG-axis ---> ABS()",
  "I/O error: read file",
  "I/O error: write file",
};

#define ERRNUM (sizeof(f2derrorlist) / sizeof(*f2derrorlist))

static char *f2dtypechar[]={
  N_("mark"),
  N_("line"),
  N_("polygon"),
  N_("curve"),
  N_("diagonal"),
  N_("arrow"),
  N_("rectangle"),
  N_("rectangle_fill"),
  N_("rectangle_solid_fill"),
  N_("errorbar_x"),
  N_("errorbar_y"),
  N_("staircase_x"),
  N_("staircase_y"),
  N_("bar_x"),
  N_("bar_y"),
  N_("bar_fill_x"),
  N_("bar_fill_y"),
  N_("bar_solid_fill_x"),
  N_("bar_solid_fill_y"),
  N_("fit"),
  NULL
};

static int set_math_config(struct objlist *obj, char *inst, char *field, char *str);
#if NEW_MATH_CODE
static MathEquation *create_math_equation(int *id, int use_prm, int use_fprm, int usr_func, int use_fobj_func);
#endif

static struct obj_config FileConfig[] = {
  {"R",                OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"G",                OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"B",                OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"R2",               OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"G2",               OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"B2",               OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"x",                OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"y",                OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"type",             OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"smooth_x",         OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"smooth_y",         OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"mark_type",        OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"mark_size",        OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"line_width",       OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"line_join",        OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"line_miter_limit", OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"head_skip",        OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"read_step",        OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"final_line",       OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"csv",              OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"data_clip",        OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"interpolation",    OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"remark",           OBJ_CONFIG_TYPE_STRING,  NULL, NULL},
  {"ifs",              OBJ_CONFIG_TYPE_STRING,  NULL, NULL},
  {"axis_x",           OBJ_CONFIG_TYPE_STRING,  NULL, NULL},
  {"axis_y",           OBJ_CONFIG_TYPE_STRING,  NULL, NULL},
  {"line_style",       OBJ_CONFIG_TYPE_STYLE,   NULL, NULL},
  {"math_x",           OBJ_CONFIG_TYPE_OTHER, set_math_config, obj_save_config_string},
  {"math_y",           OBJ_CONFIG_TYPE_OTHER, set_math_config, obj_save_config_string},
  {"func_f",           OBJ_CONFIG_TYPE_OTHER, set_math_config, obj_save_config_string},
  {"func_g",           OBJ_CONFIG_TYPE_OTHER, set_math_config, obj_save_config_string},
  {"func_h",           OBJ_CONFIG_TYPE_OTHER, set_math_config, obj_save_config_string},
};

#define MASK_SERACH_METHOD_LINER  0
#define MASK_SERACH_METHOD_BINARY 1
#define MASK_SERACH_METHOD_CONST  2
#define MASK_SERACH_METHOD MASK_SERACH_METHOD_CONST

static NHASH FileConfigHash = NULL;

#define DXBUFSIZE 101

#define USE_BUF_PTR  1
#define USE_RING_BUF 2
#define BUF_TYPE     USE_BUF_PTR

#define USE_MEMMOVE 1

#if BUF_TYPE == USE_RING_BUF
#define RING_BUF_INC(i) (((i) < DXBUFSIZE) ? ((i) + 1) : 0)
#endif

#if HAVE_ISFINITE
#define check_infinite(v) (! isfinite(v))
#elsif HAVE_FINITE
#define check_infinite(v) (! finite(v))
#else
#define check_infinite(v) ((v) != (v) || (v) == HUGE_VAL || (v) == - HUGE_VAL)
#endif


struct f2ddata_buf {
  double dx, dy, d2, d3;
  int colr, colg, colb, marksize, marktype, line;
#if NEW_MATH_CODE
  int colr2, colg2, colb2;
  int dxstat, dystat, d2stat, d3stat;
#else
  char dxstat, dystat, d2stat, d3stat;
#endif
};

struct f2ddata {
  struct objlist *obj;
  int id;
  char *file;
  FILE *fd;
  int x,y;
  enum {TYPE_NORMAL, TYPE_DIAGONAL, TYPE_ERR_X, TYPE_ERR_Y} type;
/*
  fp->type: 0 normal (dx ... dy)
            1 diagonal (dx dy ... d2 d3)
            2 error bar x (dx d2 d3 ... dy)
            3 error bar y (dx ... dy d2 d3)
*/
  int hskip;
  int rstep;
  int final;
  int csv;
  char *remark,*ifs, ifs_buf[256];
  int line,dline;
  double count;
  int eof;
  int dataclip;
  double axmin,axmax,aymin,aymax;
  double axmin2,axmax2,aymin2,aymax2;
  double axdir,aydir;
  double axvx,axvy,ayvx,ayvy;
  int axtype,aytype,axposx,axposy,ayposx,ayposy,axlen,aylen;
  double ratex,ratey;
#if NEW_MATH_CODE
  MathEquation *codex[3], *codey[3];
  MathValue minx, maxx, miny, maxy;
  int *const_id;
  int color2[3], colr2, colg2, colb2;
  int fnumx, fnumy;
  int *needx, *needy;
  int br, bg, bb;
  int dxstat,dystat,d2stat,d3stat;
#else
  char *codex,*codey,*codef,*codeg,*codeh;
  struct narray *needfilex,*needfiley;
  double memoryx[MEMORYNUM];
  char memorystatx[MEMORYNUM];
  double sumdatax[10];
  char sumstatx[10];
  double difdatax[10];
  char difstatx[10];
  double memoryy[MEMORYNUM];
  char memorystaty[MEMORYNUM];
  double sumdatay[10];
  char sumstaty[10];
  double difdatay[10];
  char difstaty[10];
  double memory2[MEMORYNUM];
  char memorystat2[MEMORYNUM];
  double sumdata2[10];
  char sumstat2[10];
  double difdata2[10];
  char difstat2[10];
  double memory3[MEMORYNUM];
  char memorystat3[MEMORYNUM];
  double sumdata3[10];
  char sumstat3[10];
  double difdata3[10];
  char difstat3[10];
  double minx,maxx,miny,maxy;
  char minxstat,maxxstat,minystat,maxystat;
  char dxstat,dystat,d2stat,d3stat;
#endif
  int color[3];
  double dx,dy,d2,d3;
  int maxdim;
  int need2pass;
  double sumx,sumy,sumxx,sumyy,sumxy;
  int num,datanum,prev_datanum;
  int fr, fg, fb;
  int marksize0,marksize;
  int marktype0,marktype;
  int ignore,negative;
  int colr, colg, colb;
  int msize,mtype;
  struct f2ddata_buf buf[DXBUFSIZE];
#if BUF_TYPE == USE_BUF_PTR
  struct f2ddata_buf *buf_ptr[DXBUFSIZE];
#elif BUF_TYPE == USE_RING_BUF
  int ringbuf_top;
#endif
  int bufnum,bufpo;
  int smooth,smoothx,smoothy;
  int masknum;
  int *mask;
#if MASK_SERACH_METHOD == MASK_SERACH_METHOD_CONST
  int mask_index;
#endif
  int movenum;
  double *movex,*movey;
  int *move;
  time_t mtime;
  int interrupt;
  struct narray fileopen;
};

struct f2dlocal {
#if NEW_MATH_CODE
  MathEquation *codex[3], *codey[3];
  MathValue minx, maxx, miny, maxy;
  int const_id[MATH_CONST_SIZE];
#else
  char *codex;
  char *codey;
  char *codef;
  char *codeg;
  char *codeh;
  struct narray *needfilex,*needfiley;
  double minx, maxx, miny, maxy;
  char minxstat, maxxstat, minystat, maxystat;
#endif
  int maxdimx,maxdimy;
  int need2passx,need2passy,total_line;
  struct f2ddata *data;
  int coord,idx,idy,id2,id3,icx,icy,ic2,ic3,isx,isy,is2,is3,iline;
  FILE *storefd;
  int endstore;
  double sumx, sumy, sumxx, sumyy, sumxy;
  double dminx, dmaxx, dminy, dmaxy, davx, davy, dsigx, dsigy;
  int num, rcode;
  time_t mtime, mtime_stat;
};

static int set_data_progress(struct f2ddata *fp);
static int getminmaxdata(struct f2ddata *fp, struct f2dlocal *local);

#if BUF_TYPE == USE_RING_BUF
int 
ring_buf_index(struct f2ddata *fp, int i)
{
  int n;
  n = fp->ringbuf_top + i;
  return (n < DXBUFSIZE) ? n : n % DXBUFSIZE;
}
#endif

static void 
check_ifs_init(struct f2ddata *fp)
{
  char *ifs, *remark;
  int i;

  ifs = fp->ifs;
  remark = fp->remark;

  memset(fp->ifs_buf, 0, sizeof(fp->ifs_buf));
  if (ifs) {
    for (; *ifs; ifs++) {
      i = *ifs;
      fp->ifs_buf[i] |= 1;
    }
  }
  if (remark) {
    for (; *remark; remark++) {
      i = *remark;
      fp->ifs_buf[i] |= 2;
    }
  }
}

#define CHECK_IFS(buf, ch) (buf[(unsigned char) ch] & 1)
#define CHECK_REMARK(buf, ch) (buf[(unsigned char) ch] & 2)

#if NEW_MATH_CODE
static int
file_color(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;
  int color, val;

  rval->val = exp->buf[1].val.val;

  fp = (struct f2ddata *) math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  color = exp->buf[0].val.val;
  val = exp->buf[1].val.val;

  if (val < 0 || val > 255) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  switch (color) {
  case 0:
  case 1:
  case 2:
    fp->color[color] = val;
    break;
  case 3:
    fp->color[0] = fp->color[1] = fp->color[2] = val;
    break;
  case 4:
  case 5:
  case 6:
    fp->color2[color - 4] = val;
    break;
  case 7:
    fp->color2[0] = fp->color2[1] = fp->color2[2] = val;
    break;
  default:
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  return 0;
}

static int
file_rgb_sub(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval, int color2)
{
  struct f2ddata *fp;
  int r, g, b;

  rval->val = exp->buf[2].val.val;

  fp = (struct f2ddata *) math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  r = exp->buf[0].val.val * 255;
  g = exp->buf[1].val.val * 255;
  b = exp->buf[2].val.val * 255;

  if (r < 0 || r > 255 ||
      g < 0 || g > 255 ||
      b < 0 || b > 255) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (color2) {
    fp->color2[0] = r;
    fp->color2[1] = g;
    fp->color2[2] = b;
  } else {
    fp->color[0] = r;
    fp->color[1] = g;
    fp->color[2] = b;
  }

  return 0;
}

static int
file_rgb(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  return file_rgb_sub(exp, eq, rval, FALSE);
}

static int
file_rgb2(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  return file_rgb_sub(exp, eq, rval, TRUE);
}

static int
file_hsb_sub(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval, int color2)
{
  struct f2ddata *fp;
  double h, s, b;
  int r, g, bb;

  rval->val = exp->buf[2].val.val;

  fp = (struct f2ddata *) math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  h = exp->buf[0].val.val;
  s = exp->buf[1].val.val;
  b = exp->buf[2].val.val;

  if (h < 0 || h > 1 ||
      s < 0 || s > 1 ||
      b < 0 || b > 1) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  HSB2RGB(h, s, b, &r, &g, &bb);

  if (color2) {
    fp->color2[0] = r;
    fp->color2[1] = g;
    fp->color2[2] = bb;
  } else {
    fp->color[0] = r;
    fp->color[1] = g;
    fp->color[2] = bb;
  }

  return 0;
}

static int
file_hsb(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  return file_hsb_sub(exp, eq, rval, FALSE);
}

static int
file_hsb2(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  return file_hsb_sub(exp, eq, rval, TRUE);
}

static int
file_marksize(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;
  int size;

  rval->val = exp->buf[0].val.val;

  fp = (struct f2ddata *) math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  size = exp->buf[0].val.val;
  if (size < 0 || size > 65536) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  fp->marksize = size;

  return 0;
}

static int
file_marktype(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;
  int type;

  rval->val = exp->buf[0].val.val;

  fp = (struct f2ddata *) math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  type = exp->buf[0].val.val;
  if (type < 0 || type > 89) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  fp->marktype = type;

  return 0;
}

struct funcs {
  char *name;
  struct math_function_parameter prm;
};

static struct funcs file_func[] = {
  {"COLOR",    {2, 0, 0, file_color,    NULL, NULL, NULL, NULL}},
  {"RGB",      {3, 0, 0, file_rgb,      NULL, NULL, NULL, NULL}},
  {"RGB2",     {3, 0, 0, file_rgb2,     NULL, NULL, NULL, NULL}},
  {"HSB",      {3, 0, 0, file_hsb,      NULL, NULL, NULL, NULL}},
  {"HSB2",     {3, 0, 0, file_hsb2,     NULL, NULL, NULL, NULL}},
  {"MARKSIZE", {1, 0, 0, file_marksize, NULL, NULL, NULL, NULL}},
  {"MARKTYPE", {1, 0, 0, file_marktype, NULL, NULL, NULL, NULL}},
};

static int
add_file_func(MathEquation *eq) {
  unsigned int i;

  for (i = 0; i < sizeof(file_func) / sizeof(*file_func); i++) {
    if (math_equation_add_func(eq, file_func[i].name, &file_func[i].prm) == NULL)
      return 1;
  }
  return 0;
}

#endif

static struct f2ddata *
opendata(struct objlist *obj,char *inst,
	 struct f2dlocal *f2dlocal,int axis,int raw)
{
  int fid;
  char *file;
  int fr, fg, fb;
  int x,y,type,hskip,rstep,final,csv;
  char *remark,*ifs;
  char *axisx,*axisy;
  int smoothx,smoothy;
  struct narray *mask;
  struct narray *move,*movex,*movey;
  struct f2ddata *fp;
  int i,j;
  struct objlist *aobj;
  int anum,id;
  struct narray iarray;
  double axmin,axmax,aymin,aymax,axmin2,axmax2,aymin2,aymax2,ratex,ratey;
  double axdir,aydir;
  double axvx,axvy,ayvx,ayvy;
  int axtype,aytype,axposx,axposy,ayposx,ayposy,axlen,aylen,dirx,diry;
  char *inst1;
  int marksize,marktype;
#if NEW_MATH_CODE
  int br, bg, bb;
#else
  int num;
  int *data;
#endif
  int num2,prev_datanum;
  int *data2;
  char *raxis;
  double ip1,ip2;
  int dataclip;
  struct stat stat_buf;

  _getobj(obj,"id",inst,&fid);
  _getobj(obj,"file",inst,&file);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"type",inst,&type);
  _getobj(obj,"head_skip",inst,&hskip);
  _getobj(obj,"read_step",inst,&rstep);
  _getobj(obj,"final_line",inst,&final);
  _getobj(obj,"remark",inst,&remark);
  _getobj(obj,"ifs",inst,&ifs);
  _getobj(obj,"csv",inst,&csv);
  _getobj(obj,"axis_x",inst,&axisx);
  _getobj(obj,"axis_y",inst,&axisy);
  _getobj(obj,"smooth_x",inst,&smoothx);
  _getobj(obj,"smooth_y",inst,&smoothy);
  _getobj(obj,"mask",inst,&mask);
  _exeobj(obj,"move_data_adjust",inst,0,NULL);
  _getobj(obj,"move_data",inst,&move);
  _getobj(obj,"move_data_x",inst,&movex);
  _getobj(obj,"move_data_y",inst,&movey);
  _getobj(obj,"R",inst,&fr);
  _getobj(obj,"G",inst,&fg);
  _getobj(obj,"B",inst,&fb);
#if NEW_MATH_CODE
  _getobj(obj,"R2",inst,&br);
  _getobj(obj,"G2",inst,&bg);
  _getobj(obj,"B2",inst,&bb);
#endif
  _getobj(obj,"mark_size",inst,&marksize);
  _getobj(obj,"mark_type",inst,&marktype);
  _getobj(obj,"data_clip",inst,&dataclip);
  _getobj(obj,"data_num",inst,&prev_datanum);

  if (file==NULL) {
    error(obj,ERRFILE);
    return NULL;
  }

  if (axis) {
    if (axisx == NULL) {
      error(obj, ERRNOAXIS);
      return NULL;
    }

    arrayinit(&iarray,sizeof(int));
    if (getobjilist(axisx,&aobj,&iarray,FALSE,NULL)) return NULL;
    anum=arraynum(&iarray);
    if (anum<1) {
      arraydel(&iarray);
      error2(obj,ERRNOAXISINST,axisx);
      return NULL;
    }
    id=*(int *)arraylast(&iarray);
    arraydel(&iarray);
    if ((inst1=getobjinst(aobj,id))==NULL) return NULL;
    if (_getobj(aobj,"x",inst1,&axposx)) return NULL;
    if (_getobj(aobj,"y",inst1,&axposy)) return NULL;
    if (_getobj(aobj,"length",inst1,&axlen)) return NULL;
    if (_getobj(aobj,"direction",inst1,&dirx)) return NULL;
    if (_getobj(aobj,"min",inst1,&axmin)) return NULL;
    if (_getobj(aobj,"max",inst1,&axmax)) return NULL;
    if (_getobj(aobj,"type",inst1,&axtype)) return NULL;
    if ((axmin==0) && (axmax==0)) {
      if (_getobj(aobj,"reference",inst1,&raxis)) return NULL;
      if (raxis!=NULL) {
	arrayinit(&iarray,sizeof(int));
	if (!getobjilist(raxis,&aobj,&iarray,FALSE,NULL)) {
	  anum=arraynum(&iarray);
	  if (anum>0) {
	    id=*(int *)arraylast(&iarray);
	    arraydel(&iarray);
	    if ((anum>0) && ((inst1=getobjinst(aobj,id))!=NULL)) {
	      _getobj(aobj,"min",inst1,&axmin);
	      _getobj(aobj,"max",inst1,&axmax);
	      _getobj(aobj,"type",inst1,&axtype);
	    }
	  }
	}
      }
    }
    axdir=dirx/18000.0*MPI;
    axvx=cos(axdir);
    axvy=-sin(axdir);
    axmin2=axmin;
    axmax2=axmax;
    if (axmin==axmax) {
      error(obj,ERRNOSETAXIS);
      return NULL;
    } else if (axtype==1) {
      if ((axmin<=0) || (axmax<=0)) {
	error(obj,ERRMINMAX);
	return NULL;
      } else {
	axmin=log10(axmin);
	axmax=log10(axmax);
      }
    } else if (axtype==2) {
      if (axmin*axmax<=0) {
	error(obj,ERRMINMAX);
	return NULL;
      } else {
	axmin=1/axmin;
	axmax=1/axmax;
      }
    }
    ratex=axlen/(axmax-axmin);

    if (axisy==NULL) {
      error(obj,ERRNOAXIS);
      return NULL;
    }

    arrayinit(&iarray,sizeof(int));
    if (getobjilist(axisy,&aobj,&iarray,FALSE,NULL)) return NULL;
    anum=arraynum(&iarray);
    if (anum<1) {
      arraydel(&iarray);
      error2(obj,ERRNOAXISINST,axisy);
      return NULL;
    }
    id=*(int *)arraylast(&iarray);
    arraydel(&iarray);
    if ((inst1=getobjinst(aobj,id))==NULL) return NULL;
    if (_getobj(aobj,"x",inst1,&ayposx)) return NULL;
    if (_getobj(aobj,"y",inst1,&ayposy)) return NULL;
    if (_getobj(aobj,"length",inst1,&aylen)) return NULL;
    if (_getobj(aobj,"direction",inst1,&diry)) return NULL;
    if (_getobj(aobj,"min",inst1,&aymin)) return NULL;
    if (_getobj(aobj,"max",inst1,&aymax)) return NULL;
    if (_getobj(aobj,"type",inst1,&aytype)) return NULL;
    if ((aymin==0) && (aymax==0)) {
      if (_getobj(aobj,"reference",inst1,&raxis)) return NULL;
      if (raxis!=NULL) {
	arrayinit(&iarray,sizeof(int));
	if (!getobjilist(raxis,&aobj,&iarray,FALSE,NULL)) {
	  anum=arraynum(&iarray);
	  if (anum>0) {
	    id=*(int *)arraylast(&iarray);
	    arraydel(&iarray);
	    if ((anum>0) && ((inst1=getobjinst(aobj,id))!=NULL)) {
	      _getobj(aobj,"min",inst1,&aymin);
	      _getobj(aobj,"max",inst1,&aymax);
	      _getobj(aobj,"type",inst1,&aytype);
	    }
	  }
	}
      }
    }
    aydir=diry/18000.0*MPI;
    ayvx=cos(aydir);
    ayvy=-sin(aydir);
    aymin2=aymin;
    aymax2=aymax;
    if (aymin==aymax) {
      error(obj,ERRNOSETAXIS);
      return NULL;
    } else if (aytype==1) {
      if ((aymin<=0) || (aymax<=0)) {
	error(obj,ERRMINMAX);
	return NULL;
      } else {
	aymin=log10(aymin);
	aymax=log10(aymax);
      }
    } else if (aytype==2) {
      if (aymin*aymax<=0) {
	error(obj,ERRMINMAX);
	return NULL;
      } else {
	aymin=1/aymin;
	aymax=1/aymax;
      }
    }
    ratey=aylen/(aymax-aymin);

    ip1=-axvy*ayvx+axvx*ayvy;
    ip2=-ayvy*axvx+ayvx*axvy;

    if ((fabs(ip1)<=1e-15) || (fabs(ip2)<=1e-15)) {
      error(obj,ERRAXISDIR);
      return NULL;
    }
  } else {
    /* these initialization exist to avoid compile warnings. */
    axposx = 0;
    axposy = 0;
    axlen = 0;
    dirx = 0;
    axmin = 0;
    axmax = 0;
    axtype = 0;

    ayposx = 0;
    ayposy = 0;
    aylen = 0;
    diry = 0;
    aymin = 0;
    aymax = 0;
    aytype = 0;

    axvx = 0;
    axvy = 0;
    ayvx = 0;
    ayvy = 0;
    aydir = 0;
    axdir = 0;
    ratey = 0;
    ratex = 0;
    aymax2 = 0;
    aymin2 = 0;
    axmax2 = 0;
    axmin2 = 0;
  }
  if ((fp=memalloc(sizeof(struct f2ddata)))==NULL) return NULL;
  fp->file=file;
  if ((fp->fd=nfopen(file,"rt"))==NULL) {
    error2(obj,ERROPEN,file);
    memfree(fp);
    return NULL;
  }

  if (fstat(fileno(fp->fd), &stat_buf)) {
    error2(obj,ERROPEN,file);
    fclose(fp->fd);
    memfree(fp);
    return NULL;
  }
  fp->mtime = stat_buf.st_mtime;

  fp->obj=obj;
  fp->id=fid;
  fp->prev_datanum = prev_datanum;
#if BUF_TYPE == USE_BUF_PTR
  for (i = 0; i < DXBUFSIZE; i++) {
    fp->buf_ptr[i] = &fp->buf[i];
  }
#elif BUF_TYPE == USE_RING_BUF
  fp->ringbuf_top = 0;
#endif
  fp->bufnum=0;
  fp->bufpo=0;
  fp->masknum=arraynum(mask);
  fp->mask=arraydata(mask);
#if MASK_SERACH_METHOD == MASK_SERACH_METHOD_CONST
  fp->mask_index = 0;
#endif

  fp->movenum=arraynum(move);
  fp->move=arraydata(move);
  fp->movex=arraydata(movex);
  fp->movey=arraydata(movey);

  if (smoothx>smoothy) fp->smooth=smoothx;
  else fp->smooth=smoothy;
  fp->smoothx=smoothx;
  fp->smoothy=smoothy;
  fp->dataclip=dataclip;

  fp->x=x;
  fp->y=y;

  switch (type) {
  case PLOT_TYPE_DIAGONAL:
  case PLOT_TYPE_ARROW:
  case PLOT_TYPE_RECTANGLE:
  case PLOT_TYPE_RECTANGLE_FILL:
  case PLOT_TYPE_RECTANGLE_SOLID_FILL:
    fp->type=TYPE_DIAGONAL;
    break;
  case PLOT_TYPE_ERRORBAR_X:
    fp->type=TYPE_ERR_X;
    break;
  case PLOT_TYPE_ERRORBAR_Y:
    fp->type=TYPE_ERR_Y;
    break;
  default:
    fp->type=TYPE_NORMAL;
    break;
  }

  fp->hskip=hskip;
  fp->rstep=rstep;
  fp->final=final;
  fp->remark=remark;
  fp->ifs=ifs;
  check_ifs_init(fp);

  fp->csv=csv;
  fp->line=0;
  fp->datanum=0;
  fp->num=0;
  fp->dline=0;
  fp->count=0;
  fp->eof=FALSE;

  fp->axmin=axmin;
  fp->axmax=axmax;
  fp->axmin2=axmin2;
  fp->axmax2=axmax2;
  fp->axdir=axdir;
  fp->axvx=axvx;
  fp->axvy=axvy;
  fp->axtype=axtype;
  fp->axposx=axposx;
  fp->axposy=axposy;
  fp->axlen=axlen;
  fp->aymin=aymin;
  fp->aymax=aymax;
  fp->aymin2=aymin2;
  fp->aymax2=aymax2;
  fp->aydir=aydir;
  fp->ayvx=ayvx;
  fp->ayvy=ayvy;
  fp->aytype=aytype;
  fp->ayposx=ayposx;
  fp->ayposy=ayposy;
  fp->aylen=aylen;
  fp->ratex=ratex;
  fp->ratey=ratey;
  fp->interrupt = FALSE;

#if NEW_MATH_CODE
  for (i = 0; i < (int) (sizeof(fp->codex) / sizeof(*fp->codex)); i++) {
    fp->codex[i] = f2dlocal->codex[i];
  }
  for (i = 0; i < (int) (sizeof(fp->codey) / sizeof(*fp->codey)); i++) {
    fp->codey[i] = f2dlocal->codey[i];
  }
  fp->const_id = f2dlocal->const_id;
#else
  fp->codex=f2dlocal->codex;
  fp->codey=f2dlocal->codey;
  fp->codef=f2dlocal->codef;
  fp->codeg=f2dlocal->codeg;
  fp->codeh=f2dlocal->codeh;
  fp->needfilex=f2dlocal->needfilex;
  fp->needfiley=f2dlocal->needfiley;
#endif
  switch (fp->type) {
  case TYPE_NORMAL:
    break;
  case TYPE_DIAGONAL:
    x++;
    y++;
    break;
  case TYPE_ERR_X:
    x+=2;
    break;
  case TYPE_ERR_Y:
    y+=2;
    break;
  }
  fp->maxdim=f2dlocal->maxdimx;
  if (fp->maxdim<x) fp->maxdim=x;
  if (fp->maxdim<f2dlocal->maxdimy) fp->maxdim=f2dlocal->maxdimy;
  if (fp->maxdim<y) fp->maxdim=y;
  if (f2dlocal->need2passx || f2dlocal->need2passy) fp->need2pass=TRUE;
  else fp->need2pass=FALSE;
#if NEW_MATH_CODE
  fp->br=br;
  fp->bg=bg;
  fp->bb=bb;
  fp->color2[0]=br;
  fp->color2[1]=bg;
  fp->color2[2]=bb;
  fp->fnumx = 0;
  fp->needx = NULL;
  fp->fnumy = 0;
  fp->needy = NULL;
#else
  for (i=0;i<MEMORYNUM;i++) {
    fp->memoryx[i]=0;
    fp->memorystatx[i]=MNOERR;
    fp->memoryy[i]=0;
    fp->memorystaty[i]=MNOERR;
    fp->memory2[i]=0;
    fp->memorystat2[i]=MNOERR;
    fp->memory3[i]=0;
    fp->memorystat3[i]=MNOERR;
  }
  for (i=0;i<10;i++) {
    fp->sumdatax[i]=0;
    fp->sumstatx[i]=MNOERR;
    fp->difdatax[i]=0;
    fp->difstatx[i]=MUNDEF;
    fp->sumdatay[i]=0;
    fp->sumstaty[i]=MNOERR;
    fp->difdatay[i]=0;
    fp->difstaty[i]=MUNDEF;
    fp->sumdata2[i]=0;
    fp->sumstat2[i]=MNOERR;
    fp->difdata2[i]=0;
    fp->difstat2[i]=MUNDEF;
    fp->sumdata3[i]=0;
    fp->sumstat3[i]=MNOERR;
    fp->difdata3[i]=0;
    fp->difstat3[i]=MUNDEF;
  }
#endif
  fp->dx=fp->dy=fp->d2=fp->d3=0;
  fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MUNDEF;
  fp->fr=fr;
  fp->fg=fg;
  fp->fb=fb;
  fp->color[0]=fp->fr;
  fp->color[1]=fp->fg;
  fp->color[2]=fp->fb;
  fp->marksize0=marksize;
  fp->marksize=marksize;
  fp->marktype0=marktype;
  fp->marktype=marktype;
  fp->ignore=fp->negative=FALSE;
  arrayinit(&(fp->fileopen),sizeof(int));
  if (!raw) {
#if NEW_MATH_CODE
    MathEquationParametar *prm;

    if (fp->codex[0] && fp->codex[0]->exp) {
      prm = math_equation_get_parameter(fp->codex[0], 'F');
      if (prm) {
	fp->fnumx = prm->id_num;
	fp->needx = prm->id;
      }
    }
    for (i = 0; i < fp->fnumx; i++) {
      id = fp->needx[i] / 1000;
      if (id == fp->id) {
        if (fp->maxdim < (fp->needx[i] % 1000)) {
	  fp->maxdim = fp->needx[i] % 1000;
	}
      } else if (id <= chkobjlastinst(fp->obj)) {
        num2 = arraynum(&(fp->fileopen));
        data2 = arraydata(&(fp->fileopen));
        for (j = 0; j < num2; j++) {
	  if (id == data2[j]) {
	    break;
	  }
	}
        if (j == num2) {
	  inst1 = chkobjinst(fp->obj, id);
          if (inst1 && _exeobj(fp->obj, "opendata_raw", inst1, 0, NULL) == 0) {
	    arrayadd(&fp->fileopen, &id);
	  }
        }
      }
    }

    if (fp->codey[0] && fp->codey[0]->exp) {
      prm = math_equation_get_parameter(fp->codey[0], 'F');
      if (prm) {
	fp->fnumy = prm->id_num;
	fp->needy = prm->id;
      }
    }
    for (i = 0; i < fp->fnumy; i++) {
      id = fp->needy[i] / 1000;
      if (id == fp->id) {
        if (fp->maxdim < (fp->needy[i] % 1000)) {
	  fp->maxdim = fp->needy[i] % 1000;
	}
      } else if (id <= chkobjlastinst(fp->obj)) {
        num2 = arraynum(&(fp->fileopen));
        data2 = arraydata(&(fp->fileopen));
        for (j = 0; j < num2; j++) {
	  if (id == data2[j]) {
	    break;
	  }
	}
        if (j == num2) {
	  inst1 = chkobjinst(fp->obj, id);
          if (inst1 && _exeobj(fp->obj, "opendata_raw", inst1, 0, NULL) == 0) {
	    arrayadd(&fp->fileopen, &id);
          }
        }
      }
    }
#else
    num=arraynum(fp->needfilex);
    data=arraydata(fp->needfilex);
    for (i=0;i<num;i++) {
      id=data[i]/1000;
      if (id==fp->id) {
        if (fp->maxdim<(data[i]%1000)) fp->maxdim=data[i]%1000;
      } else if (id<=chkobjlastinst(fp->obj)) {
        num2=arraynum(&(fp->fileopen));
        data2=arraydata(&(fp->fileopen));
        for (j=0;j<num2;j++) if (id==data2[j]) break;
        if (j==num2) {
          if ((inst1=chkobjinst(fp->obj,id))!=NULL) {
            if (_exeobj(fp->obj,"opendata_raw",inst1,0,NULL)==0)
              arrayadd(&(fp->fileopen),&id);
          }
        }
      }
    }
    num=arraynum(fp->needfiley);
    data=arraydata(fp->needfiley);
    for (i=0;i<num;i++) {
      id=data[i]/1000;
      if (id==fp->id) {
        if (fp->maxdim<(data[i]%1000)) fp->maxdim=data[i]%1000;
      } else if (id<=chkobjlastinst(fp->obj)) {
        num2=arraynum(&(fp->fileopen));
        data2=arraydata(&(fp->fileopen));
        for (j=0;j<num2;j++) if (id==data2[j]) break;
        if (j==num2) {
          if ((inst1=chkobjinst(fp->obj,id))!=NULL) {
            if (_exeobj(fp->obj,"opendata_raw",inst1,0,NULL)==0)
              arrayadd(&(fp->fileopen),&id);
          }
        }
      }
    }
#endif
  }
  return fp;
}

static void 
reopendata(struct f2ddata *fp)
{
#if ! NEW_MATH_CODE
  int i;
#endif

  fseek(fp->fd,0,SEEK_SET);
  fp->bufnum=0;
  fp->bufpo=0;
  fp->line=0;
  fp->prev_datanum = fp->datanum;
  fp->datanum=0;
#if MASK_SERACH_METHOD == MASK_SERACH_METHOD_CONST
  fp->mask_index = 0;
#endif
  fp->dline=0;
  fp->count=0;
  fp->eof=FALSE;
  fp->interrupt = FALSE;
  fp->ignore=fp->negative=FALSE;
  fp->dx=fp->dy=fp->d2=fp->d3=0;
  fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MUNDEF;
#if NEW_MATH_CODE
  fp->color2[0]=fp->br;
  fp->color2[1]=fp->bg;
  fp->color2[2]=fp->bb;
#else
  for (i=0;i<MEMORYNUM;i++) {
    fp->memoryx[i]=0;
    fp->memorystatx[i]=MNOERR;
    fp->memoryy[i]=0;
    fp->memorystaty[i]=MNOERR;
    fp->memory2[i]=0;
    fp->memorystat2[i]=MNOERR;
    fp->memory3[i]=0;
    fp->memorystat3[i]=MNOERR;
  }
  for (i=0;i<10;i++) {
    fp->sumdatax[i]=0;
    fp->sumstatx[i]=MNOERR;
    fp->difdatax[i]=0;
    fp->difstatx[i]=MUNDEF;
    fp->sumdatay[i]=0;
    fp->sumstaty[i]=MNOERR;
    fp->difdatay[i]=0;
    fp->difstaty[i]=MUNDEF;
    fp->sumdata2[i]=0;
    fp->sumstat2[i]=MNOERR;
    fp->difdata2[i]=0;
    fp->difstat2[i]=MUNDEF;
    fp->sumdata3[i]=0;
    fp->sumstat3[i]=MNOERR;
    fp->difdata3[i]=0;
    fp->difstat3[i]=MUNDEF;
  }
#endif
  fp->color[0]=fp->fr;
  fp->color[1]=fp->fg;
  fp->color[2]=fp->fb;
  fp->marksize=fp->marksize0;
  fp->marktype=fp->marktype0;
}

static void 
closedata(struct f2ddata *fp, struct f2dlocal *f2dlocal)
{
  int j,num2,*data2;
  char *inst1,*inst;

  if (fp==NULL) return;

  set_data_progress(fp);

  num2=arraynum(&(fp->fileopen));
  data2=arraydata(&(fp->fileopen));
  for (j=0;j<num2;j++) {
    if ((inst1=chkobjinst(fp->obj,data2[j]))!=NULL) {
      _exeobj(fp->obj,"closedata_raw",inst1,0,NULL);
    }
  }
  arraydel(&(fp->fileopen));
  fclose(fp->fd);
  if ((inst=chkobjinst(fp->obj,fp->id))!=NULL)
    _putobj(fp->obj,"data_num",inst,&(fp->datanum));

  f2dlocal->num = fp->datanum;

  memfree(fp);
}

#if NEW_MATH_CODE
static char *
create_func_def_str(const char *name, const char *code)
{
  char func_def[] = "def %s(x,y,z) {%s;}";
  int nlen, clen, len;
  char *ptr;

  nlen = strlen(name);
  clen = strlen(code);

  len = nlen + clen + sizeof(func_def);
  ptr = memalloc(len);
  if (ptr == NULL)
    return NULL;

  snprintf(ptr, len, func_def, name, code);

  return ptr;
}

static int
set_user_fnc(MathEquation **eq, const char *str, const char *fname, char **err_msg)
{
  int r;
  char *buf, default_func[] = "def f(x,y,z){0}";
  //                     01234

  if (err_msg)
    *err_msg = NULL;

  if (eq[0] == NULL || eq[1] == NULL || eq[2] == NULL) {
    return 0;
  }

  default_func[4] = fname[0];

  if (str) {
    buf = create_func_def_str(fname, str);
    if (buf == NULL)
      return MATH_ERROR_MEMORY;

    r = math_equation_parse(eq[0], buf);
    if (r) {
      if (err_msg) {
	*err_msg = math_err_get_error_message(eq[0], buf, r);
      }
      memfree(buf);
      return r;
    }

    r = math_equation_parse(eq[1], buf);
    if (r) {
      if (err_msg) {
	*err_msg = math_err_get_error_message(eq[0], buf, r);
      }
      memfree(buf);
      return r;
    }

    r = math_equation_parse(eq[2], buf);
    if (r) {
      if (err_msg) {
	*err_msg = math_err_get_error_message(eq[0], buf, r);
      }
      memfree(buf);
      return r;
    }
    memfree(buf);
  } else {
    math_equation_parse(eq[0], default_func);
    math_equation_parse(eq[1], default_func);
    math_equation_parse(eq[2], default_func);
  }

  return 0;
}

static int
set_equation(struct f2dlocal *f2dlocal, MathEquation **eq, const char *f, const char *g, const char *h, const char *str, char **err_msg)
{
  int i, rcode;

  if (err_msg) {
    *err_msg = NULL;
  }

  for (i = 0; i < 3; i++) {
    if (eq[i]) {
      math_equation_free(eq[i]);
    }
    eq[i] = NULL;
  }

  for (i = 0; i < 3; i++) {
    eq[i] = create_math_equation(f2dlocal->const_id, TRUE, TRUE, TRUE, TRUE);
  }

  if (eq[0] == NULL || eq[1] == NULL || eq[2] == NULL)
    return MATH_ERROR_MEMORY;

  if (f) {
    rcode = set_user_fnc(eq, f, "f", err_msg);
    if (rcode)
      return rcode;
  }
  if (g) {
    rcode = set_user_fnc(eq, g, "g", err_msg);
    if (rcode)
      return rcode;
  }
  if (h) {
    rcode = set_user_fnc(eq, g, "h", err_msg);
    if (rcode)
      return rcode;
  }

  if (str == NULL)
    return 0;

  for (i = 0; i < 3; i++) {
    rcode = math_equation_parse(eq[i], str);
    if (rcode) {
      if (err_msg) {
	*err_msg = math_err_get_error_message(eq[i], str, rcode);
      }
      return rcode;
    }
  }

  return rcode;
}

static int
put_func(struct objlist *obj, char *inst, struct f2dlocal *f2dlocal, char *field, char *eq)
{
  int rcode, type;
  char *x, *y, *f, *g, *h;
  char default_func[] = "def f(x,y,z){0}", fname[] = "F", *err_msg;
  //                     01234

  type = field[5];
  default_func[4] = type;
  fname[0] = toupper(type);

  _getobj(obj, "math_x", inst, &x);
  _getobj(obj, "math_y", inst, &y);
  _getobj(obj, "func_f", inst, &f);
  _getobj(obj, "func_g", inst, &g);
  _getobj(obj, "func_h", inst, &h);

  switch (type) {
  case 'x':
    f2dlocal->need2passx = FALSE;
    rcode = set_equation(f2dlocal, f2dlocal->codex, f, g, h, eq, &err_msg);
    if (err_msg) {
      error22(obj, ERRUNKNOWN, field, err_msg);
      free(err_msg);
    }
    f2dlocal->need2passx = math_equation_check_const(f2dlocal->codex[0], f2dlocal->const_id, TWOPASS_CONST_SIZE);
    break;
  case 'y':
    f2dlocal->need2passy = FALSE;
    rcode = set_equation(f2dlocal, f2dlocal->codey, f, g, h, eq, &err_msg);
    if (err_msg) {
      error22(obj, ERRUNKNOWN, field, err_msg);
      free(err_msg);
    }
    f2dlocal->need2passy = math_equation_check_const(f2dlocal->codey[0], f2dlocal->const_id, TWOPASS_CONST_SIZE);
    break;
  case 'f':
  case 'g':
  case 'h':
    switch (type) {
    case 'f':
      f = eq;
      break;
    case 'g':
      g = eq;
      break;
    case 'h':
      h = eq;
      break;
    }
    rcode = set_equation(f2dlocal, f2dlocal->codex, f, g, h, x, NULL);
    rcode = set_equation(f2dlocal, f2dlocal->codey, f, g, h, y, &err_msg);
    if (err_msg) {
      error22(obj, ERRUNKNOWN, field, err_msg);
      free(err_msg);
    }

    if (rcode) {
      set_user_fnc(f2dlocal->codex, NULL, fname, NULL);
      set_user_fnc(f2dlocal->codey, NULL, fname, NULL);
    }

    f2dlocal->need2passx = math_equation_check_const(f2dlocal->codex[0], f2dlocal->const_id, TWOPASS_CONST_SIZE);
    f2dlocal->need2passy = math_equation_check_const(f2dlocal->codey[0], f2dlocal->const_id, TWOPASS_CONST_SIZE);
    break;
  default:
    return 1;
  }

  return rcode;
}
#endif

static int 
f2dputmath(struct objlist *obj,char *inst,char *field,char *math)
{
#if NEW_MATH_CODE
  int rcode;
#else
  enum MATH_CODE_ERROR_NO rcode;
  int maxdim,need2pass;
  char *code;
  struct narray *needfile;
#endif 
  struct f2dlocal *f2dlocal;

  _getobj(obj,"_local",inst,&f2dlocal);

#if NEW_MATH_CODE
  rcode = put_func(obj, inst, f2dlocal, field, math);
  if (rcode) {
    return 1;
  }
#endif

  if ((strcmp(field,"math_x")==0) || (strcmp(field,"math_y")==0)) {
#if ! NEW_MATH_CODE
    if (math!=NULL) {
      needfile=arraynew(sizeof(int));
      rcode=mathcode(math,&code,NULL,needfile,&maxdim,&need2pass,
                   TRUE,TRUE,FALSE,TRUE,TRUE,TRUE,TRUE,TRUE,TRUE,TRUE,TRUE);
      if (arraynum(needfile)==0) {
        arrayfree(needfile);
        needfile=NULL;
      }
      if (mathcode_error(obj, rcode, MathErrorCodeArray)) {
        return 1;
      }
    } else {
      code=NULL;
      needfile=NULL;
      need2pass=FALSE;
      maxdim=0;
    }
#endif
    if (field[5]=='x') {
#if NEW_MATH_CODE
      f2dlocal->maxdimx = 0;
      if (f2dlocal->codex[0]) {
	MathEquationParametar *prm;

	prm = math_equation_get_parameter(f2dlocal->codex[0], 0);
	if (prm == NULL)
	  return 1;

	f2dlocal->maxdimx = prm->id_max;
      }
#else
      memfree(f2dlocal->codex);
      f2dlocal->codex=code;
      f2dlocal->maxdimx=maxdim;
      arrayfree(f2dlocal->needfilex);
      f2dlocal->needfilex=needfile;
#endif
    } else {
#if NEW_MATH_CODE
      f2dlocal->maxdimy = 0;
      if (f2dlocal->codey[0]) {
	MathEquationParametar *prm;

	prm = math_equation_get_parameter(f2dlocal->codey[0], 0);
	if (prm == NULL)
	  return 1;

	f2dlocal->maxdimy = prm->id_max;
      }
#else
      memfree(f2dlocal->codey);
      f2dlocal->codey=code;
      f2dlocal->maxdimy=maxdim;
      f2dlocal->need2passy=need2pass;
      arrayfree(f2dlocal->needfiley);
      f2dlocal->needfiley=needfile;
#endif
    }
#if ! NEW_MATH_CODE
  } else if ((strcmp(field,"func_f")==0) || (strcmp(field,"func_g")==0)
          || (strcmp(field,"func_h")==0)) {
    if (math!=NULL) {
      rcode=mathcode(math,&code,NULL,NULL,NULL,NULL,
                     TRUE,TRUE,TRUE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE);
      if (mathcode_error(obj, rcode, MathErrorCodeArray)) {
        return 1;
      }
    } else code=NULL;
    switch (field[5]) {
    case 'f':
      memfree(f2dlocal->codef);
      f2dlocal->codef=code;
      break;
    case 'g':
      memfree(f2dlocal->codeg);
      f2dlocal->codeg=code;
      break;
    case 'h':
      memfree(f2dlocal->codeh);
      f2dlocal->codeh=code;
      break;
    }
#endif
  }
  return 0;
}

static int 
set_math_config(struct objlist *obj, char *inst, char *field, char *str)
{
  char *f1;
  int len;

  f1 = getitok2(&str, &len, "");
  if (f2dputmath(obj, inst, field, f1) == 0) {
    _putobj(obj, field, inst, f1);
  } else {
    memfree(f1);
  }
  return 0;
}

static int 
f2dloadconfig(struct objlist *obj,char *inst)
{
  return obj_load_config(obj, inst, F2DCONF, FileConfigHash);
}

static int 
f2dsaveconfig(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  return obj_save_config(obj, inst, F2DCONF, FileConfig, sizeof(FileConfig) / sizeof(*FileConfig));
}

#if NEW_MATH_CODE
enum {
  MATH_CONST_NUM,
  MATH_CONST_MINX,
  MATH_CONST_MAXX,
  MATH_CONST_MINY,
  MATH_CONST_MAXY,
  MATH_CONST_SUMX,
  MATH_CONST_SUMY,
  MATH_CONST_SUMXX,
  MATH_CONST_SUMYY,
  MATH_CONST_SUMXY,
  MATH_CONST_AVX,
  MATH_CONST_AVY,
  MATH_CONST_SGX,
  MATH_CONST_SGY,

  MATH_CONST_FIRST,
  MATH_CONST_D,
  MATH_CONST_N,
};

static MathEquation *
create_math_equation(int *id, int use_prm, int use_fprm, int usr_func, int use_fobj_func)
{
  MathEquation *code;
  unsigned int i;
  int f_id;
  char *file_constant[MATH_CONST_SIZE] = {
    "NUM",
    "MINX",
    "MAXX",
    "MINY",
    "MAXY",
    "SUMX",
    "SUMY",
    "SUMXX",
    "SUMYY",
    "SUMXY",
    "AVX",
    "AVY",
    "SGX",
    "SGY",

    "FIRST",
    "%D",
    "%N",
  };

  code = math_equation_basic_new();
  if (code == NULL)
    return NULL;;

  if (use_prm) {
    if (math_equation_add_parameter(code, 0, 1, 3, MATH_EQUATION_PARAMETAR_USE_ID)) {
      math_equation_free(code);
      return NULL;;
    }
  }

  if (use_fprm) {
    if (math_equation_add_parameter(code, 'F', 3, 5, MATH_EQUATION_PARAMETAR_USE_INDEX)) {
      math_equation_free(code);
      return NULL;;
    }
  }

  if (math_equation_add_var(code, "X") != 0) {
    math_equation_free(code);
    return NULL;;
  }

  if (math_equation_add_var(code, "Y") != 1) {
    math_equation_free(code);
    return NULL;;
  }

  for (i = 0; i < sizeof(file_constant) / sizeof(*file_constant); i++) {
    f_id = math_equation_add_const(code, file_constant[i], NULL);
    if (f_id < 0) {
      math_equation_free(code);
      return NULL;
    }
    if (id) {
      id[i] = f_id;
    }
  }

  if (usr_func) {
    math_equation_parse(code, "def f(x,y,z){0}");
    math_equation_parse(code, "def g(x,y,z){0}");
    math_equation_parse(code, "def h(x,y,z){0}");
  }

  if (use_fobj_func) {
    add_file_func(code);
  }

  return code;
}
#endif

static int 
f2dinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int x,y,rstep,final,msize,r2,g2,b2,lwidth,miter;
  char *s1,*s2,*s3,*s4;
  struct f2dlocal *f2dlocal;
  int stat,minmaxstat,dataclip,num,ljoin;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  x=1;
  y=2;
  rstep=1;
  final=-1;
  msize=200;
  lwidth=40;
  r2=255;
  g2=255;
  b2=255;
  miter=1000;
  num=0;
  stat=MEOF;
  minmaxstat=MUNDEF;
  dataclip=TRUE;
  ljoin = 2;
  if (_putobj(obj,"x",inst,&x)) return 1;
  if (_putobj(obj,"y",inst,&y)) return 1;
  if (_putobj(obj,"read_step",inst,&rstep)) return 1;
  if (_putobj(obj,"final_line",inst,&final)) return 1;
  if (_putobj(obj,"mark_size",inst,&msize)) return 1;
  if (_putobj(obj,"R2",inst,&r2)) return 1;
  if (_putobj(obj,"G2",inst,&g2)) return 1;
  if (_putobj(obj,"B2",inst,&b2)) return 1;
  if (_putobj(obj,"line_width",inst,&lwidth)) return 1;
  if (_putobj(obj,"line_miter_limit",inst,&miter)) return 1;
  if (_putobj(obj,"line_join",inst,&ljoin)) return 1;
  if (_putobj(obj,"data_num",inst,&num)) return 1;
  if (_putobj(obj,"stat_x",inst,&stat)) return 1;
  if (_putobj(obj,"stat_y",inst,&stat)) return 1;
  if (_putobj(obj,"stat_2",inst,&stat)) return 1;
  if (_putobj(obj,"stat_3",inst,&stat)) return 1;
  if (_putobj(obj,"stat_minx",inst,&minmaxstat)) return 1;
  if (_putobj(obj,"stat_maxx",inst,&minmaxstat)) return 1;
  if (_putobj(obj,"stat_miny",inst,&minmaxstat)) return 1;
  if (_putobj(obj,"stat_maxy",inst,&minmaxstat)) return 1;
  if (_putobj(obj,"data_clip",inst,&dataclip)) return 1;

  s1 = s2 = s3 = s4 = NULL;
  f2dlocal=NULL;

  s1 = nstrdup("#%'");
  if (s1 == NULL) goto errexit;
  if (_putobj(obj, "remark", inst, s1)) goto errexit;

  s2 = nstrdup(" ,\t()");
  if (s2 == NULL) goto errexit;
  if (_putobj(obj, "ifs", inst, s2)) goto errexit;

  s3 = nstrdup("axis:0");
  if (s3 == NULL) goto errexit;
  if (_putobj(obj, "axis_x", inst, s3)) goto errexit;

  s4 = nstrdup("axis:1");
  if (s4 == NULL) goto errexit;
  if (_putobj(obj, "axis_y", inst, s4)) goto errexit;

  f2dlocal=memalloc(sizeof(struct f2dlocal));
  if (f2dlocal == NULL) goto errexit;
  memset(f2dlocal, 0, sizeof(struct f2dlocal));
  if (_putobj(obj,"_local",inst,f2dlocal)) goto errexit;

#if NEW_MATH_CODE
  f2dlocal->codex[0] = NULL;
  f2dlocal->codex[1] = NULL;
  f2dlocal->codex[2] = NULL;
  f2dlocal->codey[0] = NULL;
  f2dlocal->codey[1] = NULL;
  f2dlocal->codey[2] = NULL;
#else
  f2dlocal->codex=NULL;
  f2dlocal->codey=NULL;
  f2dlocal->codef=NULL;
  f2dlocal->codeg=NULL;
  f2dlocal->codeh=NULL;
  f2dlocal->needfilex=NULL;
  f2dlocal->needfiley=NULL;
#endif
  f2dlocal->maxdimx=0;
  f2dlocal->maxdimy=0;
  f2dlocal->need2passx=FALSE;
  f2dlocal->need2passy=FALSE;
  f2dlocal->data=NULL;
  f2dlocal->idx=chkobjoffset(obj,"data_x");
  f2dlocal->idy=chkobjoffset(obj,"data_y");
  f2dlocal->id2=chkobjoffset(obj,"data_2");
  f2dlocal->id3=chkobjoffset(obj,"data_3");
  f2dlocal->icx=chkobjoffset(obj,"coord_x");
  f2dlocal->icy=chkobjoffset(obj,"coord_y");
  f2dlocal->ic2=chkobjoffset(obj,"coord_2");
  f2dlocal->ic3=chkobjoffset(obj,"coord_3");
  f2dlocal->isx=chkobjoffset(obj,"stat_x");
  f2dlocal->isy=chkobjoffset(obj,"stat_y");
  f2dlocal->is2=chkobjoffset(obj,"stat_2");
  f2dlocal->is3=chkobjoffset(obj,"stat_3");
  f2dlocal->iline=chkobjoffset(obj,"line");
  f2dlocal->storefd=NULL;
  f2dlocal->endstore=FALSE;

  f2dlocal->sumx = 0;
  f2dlocal->sumy = 0;
  f2dlocal->sumxx = 0;
  f2dlocal->sumyy = 0;
  f2dlocal->sumxy = 0;
  f2dlocal->dminx = HUGE_VAL;
  f2dlocal->dmaxx = HUGE_VAL;
  f2dlocal->dminy = HUGE_VAL;
  f2dlocal->dmaxy = HUGE_VAL;
  f2dlocal->davx = HUGE_VAL;
  f2dlocal->davy = HUGE_VAL;
  f2dlocal->dsigx = HUGE_VAL;
  f2dlocal->dsigy = HUGE_VAL;
  f2dlocal->num = 0;
  f2dlocal->rcode = 0;
#if NEW_MATH_CODE
  f2dlocal->minx.val = 0;
  f2dlocal->maxx.val = 0;
  f2dlocal->miny.val = 0;
  f2dlocal->maxy.val = 0;
  f2dlocal->minx.type = MUNDEF;
  f2dlocal->maxx.type = MUNDEF;
  f2dlocal->miny.type = MUNDEF;
  f2dlocal->maxy.type = MUNDEF;
#else
  f2dlocal->minx = 0;
  f2dlocal->maxx = 0;
  f2dlocal->miny = 0;
  f2dlocal->maxy = 0;
  f2dlocal->minxstat = MUNDEF;
  f2dlocal->maxxstat = MUNDEF;
  f2dlocal->minystat = MUNDEF;
  f2dlocal->maxystat = MUNDEF;
#endif
  f2dlocal->mtime = 0;
  f2dlocal->mtime_stat = 0;
  f2dlocal->total_line = 0;

  f2dloadconfig(obj,inst);
  return 0;

errexit:
  memfree(s1);
  memfree(s2);
  memfree(s3);
  memfree(s4);
  memfree(f2dlocal);
  return 1;
}

static int 
f2ddone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"_local",inst,&f2dlocal);
  closedata(f2dlocal->data, f2dlocal);
#if NEW_MATH_CODE
  math_equation_free(f2dlocal->codex[0]);
  math_equation_free(f2dlocal->codex[1]);
  math_equation_free(f2dlocal->codex[2]);
  math_equation_free(f2dlocal->codey[0]);
  math_equation_free(f2dlocal->codey[1]);
  math_equation_free(f2dlocal->codey[2]);
#else
  memfree(f2dlocal->codex);
  memfree(f2dlocal->codey);
  memfree(f2dlocal->codef);
  memfree(f2dlocal->codeg);
  memfree(f2dlocal->codeh);
  arrayfree(f2dlocal->needfilex);
  arrayfree(f2dlocal->needfiley);
#endif
  return 0;
}

static int 
f2dfile(struct objlist *obj,char *inst,char *rval,
	int argc,char **argv)
{
  struct objlist *sys;
  int ignorepath;
  char *file,*file2;
  int num2;
  struct f2dlocal *f2dlocal;

  _getobj(obj,"_local",inst,&f2dlocal);
  f2dlocal->mtime = 0;
  f2dlocal->mtime_stat = 0;
  sys=getobject("system");
  getobj(sys,"ignore_path",0,0,NULL,&ignorepath);
  if (!ignorepath) return 0;
  file=(char *)(argv[2]);
  file2=getbasename(file);
  memfree(file);
  argv[2]=file2;
  num2=0;
  _putobj(obj,"data_num",inst,&num2);
  return 0;
}

static int 
f2dbasename(struct objlist *obj,char *inst,char *rval,
                int argc,char **argv)
{
  char *file,*file2;

  memfree(*(char **)rval);
  *(char **)rval=NULL;
  _getobj(obj,"file",inst,&file);
  if (file==NULL) return 0;
  file2=getbasename(file);
  *(char **)rval=file2;
  return 0;
}

static int 
f2dput(struct objlist *obj,char *inst,char *rval, int argc,char **argv)
{
  char *field;
  char *math;
  struct f2dlocal *f2dlocal;

  _getobj(obj,"_local",inst,&f2dlocal);
  f2dlocal->mtime = 0;
  f2dlocal->mtime_stat = 0;
  field=argv[1];
  if (strcmp(field,"final_line")==0) {
#if 0
    if (*(int *)(argv[2])<-1) *(int *)(argv[2])=-1;
#endif
  } else if ((strcmp(field,"x")==0) || (strcmp(field,"y")==0)) {
    if (*(int *)(argv[2])<0) *(int *)(argv[2])=0;
    else if (*(int *)(argv[2])>FILE_OBJ_MAXCOL) *(int *)(argv[2])=FILE_OBJ_MAXCOL;
  } else if ((strcmp(field,"math_x")==0) || (strcmp(field,"math_y")==0)
          || (strcmp(field,"func_f")==0) || (strcmp(field,"func_g")==0)
          || (strcmp(field,"func_h")==0)) {
    math=(char *)(argv[2]);
    return f2dputmath(obj,inst,field,math);
  } else if (strcmp(field,"ifs")==0) {
    if (strlen((char *)argv[2])==0) {
      error(obj,ERRIFS);
      return 1;
    }
  } else if ((strcmp(field,"smooth_x")==0) || (strcmp(field,"smooth_y")==0)) {
    if (*(int *)(argv[2])<0) *(int *)(argv[2])=0;
    else if (*(int *)(argv[2])>FILE_OBJ_SMOOTH_MAX) *(int *)(argv[2])=FILE_OBJ_SMOOTH_MAX;
  }
  return 0;
}

static int 
#if NEW_MATH_CODE
getdataarray(char *buf, int maxdim, double *count, MathValue *data, char *ifs, int csv)
#else
getdataarray(char *buf,int maxdim,double *count,double *data,char *stat, char *ifs,int csv)
#endif
{
/*
   return: 0 no error
          -1 fatal error
           1 too small column
*/
  char *po,*po2,*endptr;
  char st;
  double val;
  int i;
  int dim;

  (*count)++;
  dim=0;
#if NEW_MATH_CODE
  data[dim].val = *count;
  data[dim].type = MATH_VALUE_NORMAL;
#else
  data[dim]=*count;
  stat[dim]=MNOERR;
#endif
  po=buf;
  while (*po!='\0') {
    if (dim>=maxdim) return 0;
    if (csv) {
      for (;(*po==' ');po++);
      if (*po=='\0') break;
      if (CHECK_IFS(ifs, *po)) {
        po2=po;
        po2++;
        val=0;
        st=MNONUM;
      } else {
#if 0
        for (po2=po;(*po2!='\0') && ! CHECK_IFS(ifs, *po2) && (*po2!=' ');po2++)
          *po2=toupper(*po2);
        for (i=0;i<po2-po;i++) if (*(po+i)=='D') *(po+i)='e';
#else
        for (po2=po;(*po2!='\0') && ! CHECK_IFS(ifs, *po2) && (*po2!=' ');po2++) {
	  *po2=toupper(*po2);
	  if (*po == 'D')
	    *po = 'e';
	}
#endif
        val=strtod(po,&endptr);
        if (endptr>=po2) {
	  if (check_infinite(val)) {
	    st = MNAN;
	  } else {
	    st=MNOERR;
	  }
	} else {
          if (((po2-po)==1) && (*po=='|')) st=MSCONT;
          else if (((po2-po)==1) && (*po=='=')) st=MSBREAK;
          else if (((po2-po)==3) && (strncmp(po,"NAN",3)==0)) st=MNAN;
          else if (((po2-po)==5) && (strncmp(po,"UNDEF",5)==0)) st=MUNDEF;
          else if (((po2-po)==4) && (strncmp(po,"CONT",4)==0)) st=MSCONT;
          else if (((po2-po)==5) && (strncmp(po,"BREAK",5)==0)) st=MSBREAK;
          else st=MNONUM;
        }
        for (;(*po2==' ');po2++);
        if (CHECK_IFS(ifs, *po2)) po2++;
     }
    } else {
      for (;(*po!='\0') && CHECK_IFS(ifs, *po);po++);
      if (*po=='\0') break;
#if 0
      for (po2=po;(*po2!='\0') && ! CHECK_IFS(ifs, *po2);po2++)
        *po2=toupper(*po2);
      for (i=0;i<po2-po;i++) if (*(po+i)=='D') *(po+i)='e';
#else
      for (po2=po;(*po2!='\0') && ! CHECK_IFS(ifs, *po2);po2++) {
        *po2=toupper(*po2);
	if (*po == 'D')
	  *po = 'e';
      }
#endif
      val=strtod(po,&endptr);
      if (endptr>=po2) {
	if (check_infinite(val)) {
	  st = MNAN;
	} else {
	  st=MNOERR;
	}
      } else {
        if (((po2-po)==1) && (*po=='|')) st=MSCONT;
        else if (((po2-po)==1) && (*po=='=')) st=MSBREAK;
        else if (((po2-po)==3) && (strncmp(po,"NAN",3)==0)) st=MNAN;
        else if (((po2-po)==5) && (strncmp(po,"UNDEF",5)==0)) st=MUNDEF;
        else if (((po2-po)==4) && (strncmp(po,"CONT",4)==0)) st=MSCONT;
        else if (((po2-po)==5) && (strncmp(po,"BREAK",5)==0)) st=MSBREAK;
        else st=MNONUM;
      }
    }
    po=po2;
    dim++;
#if NEW_MATH_CODE
    data[dim].val = val;
    data[dim].type = st;
#else
    data[dim]=val;
    stat[dim]=st;
#endif
  }
  for (i=dim+1;i<=maxdim;i++) {
#if NEW_MATH_CODE
    data[i].val = 0;
    data[i].type = MATH_VALUE_NONUM;
#else
    data[i]=0;
    stat[i]=MNONUM;
#endif
  }
  return 1;
}


static int 
hskipdata(struct f2ddata *fp)
{
  int skip,rcode;
  char *buf;

  skip=0;
  while (skip<fp->hskip) {
    if ((fp->line & 0x1fff) == 0 && set_data_progress(fp)) {
      return 0;
    }
    rcode=fgetline(fp->fd,&buf);
    if (rcode==-1) return -1;
    if (rcode==1) {
      fp->eof=TRUE;
      return 0;
    }
    memfree(buf);
    fp->line++;
    skip++;
  }
  return 0;
}

static int
set_data_progress(struct f2ddata *fp)
{
  static char msgbuf[256];
  double frac;

  if (fp->final > 0) {
    frac = 1.0 * fp->line / fp->final;
  } else if (fp->prev_datanum > 0) {
    if (fp->datanum <= fp->prev_datanum) {
      frac = 1.0 * fp->datanum / fp->prev_datanum;
    } else {
      frac = -1;
    }
  } else {
    frac = -1;
  }

  if (frac > 1)
    frac = 1.0;

  snprintf(msgbuf, sizeof(msgbuf), "%d: %s (%d)", fp->id, fp->file, fp->line);
  set_progress(0, msgbuf, frac);
  if (ninterrupt()) {
    fp->eof=TRUE;
    fp->interrupt = TRUE;
    return TRUE;
  }
  return FALSE;
}

static void
#if NEW_MATH_CODE
getdata_get_other_files(struct f2ddata *fp, int fnumx, int fnumy, int *needx, int *needy,
			MathValue *datax, MathValue *datay, int filenum, int *openfile)
#else
getdata_get_other_files(struct f2ddata *fp, int fnumx, int fnumy,
	     int *needx, int *needy, double *datax, double *datay,
	     char *statx, char *staty, int filenum, int *openfile)
#endif
{
  int *idata,inum,argc;
  char *argv[2];
  int i,j,k;
  double *ddata;
  int colnum;
  char *inst1;
  struct narray iarray;
  int id,col;
  struct narray *coldata;

  for (i = 0; i < filenum; i++) {
    id = openfile[i];
    arrayinit(&iarray,sizeof(int));
    for (j = 0; j < fnumx; j++) {
      if (needx[j] / 1000 == id) {
	col = needx[j] % 1000;
	arrayadd(&iarray, &col);
      }
    }
    for (j = 0; j < fnumy; j++) {
      if (needy[j] / 1000 == id) {
	col = needy[j] % 1000;
	arrayadd(&iarray, &col);
      }
    }
    inum = arraynum(&iarray);
    idata = arraydata(&iarray);
    argv[0] = (char *)&iarray;
    argv[1] = NULL;
    argc=1;
    if ((inst1 = chkobjinst(fp->obj, id)) &&
	_exeobj(fp->obj, "getdata_raw", inst1, argc, argv) == 0) {
      _getobj(fp->obj,"getdata_raw", inst1, &coldata);
      colnum = arraynum(coldata);
      ddata = arraydata(coldata);
      if (colnum == inum * 2) {
	int n;
	for (j = 0; j < fnumx; j++) {
	  if (needx[j] / 1000 == id) {
	    n = needx[j] % 1000;
	    for (k = 0; k < inum; k++) {
	      if (idata[k] == n) {
#if NEW_MATH_CODE
		datax[j].val = ddata[k];
		datax[j].type = nround(ddata[k + inum]);
#else
		datax[j] = ddata[k];
		statx[j] = nround(ddata[k + inum]);
#endif
	      }
	    }
	  }
	}
	for (j = 0; j < fnumy; j++) {
	  if (needy[j] / 1000 == id) {
	    n = needy[j] % 1000;
	    for (k = 0; k < inum; k++){
	      if (idata[k] == n) {
#if NEW_MATH_CODE
		datay[j].val = ddata[k];
		datay[j].type = nround(ddata[k + inum]);
#else
		datay[j] = ddata[k];
		staty[j] = nround(ddata[k + inum]);
#endif
	      }
	    }
	  }
	}
      }
    }
    arraydel(&iarray);
  }
}

static int
getdata_skip_step(struct f2ddata *fp)
{
  char *buf;
  int i, step, rcode;;

  step = 1;
  while (step < fp->rstep) {
    rcode = fgetline(fp->fd, &buf);
    if (rcode == 1) {
      fp->eof = TRUE;
      memfree(buf);
      break;
    } else if (rcode == -1) {
      memfree(buf);
      return -1;
    }
    fp->line++;
    for (i = 0; buf[i] != '\0' && CHECK_IFS(fp->ifs_buf, buf[i]); i++);
    if (buf[i] != '\0'
	&& (fp->remark == NULL || ! CHECK_REMARK(fp->ifs_buf, buf[i])))
      step++;
    memfree(buf);
  }

  return 0;
}

#if MASK_SERACH_METHOD == MASK_SERACH_METHOD_CONST
static int
search_mask(int *mask, int n, int *index, int line)
{
  int i;

  i = *index;

  if (mask == NULL || i >= n)
    return FALSE;

  if (mask[i] < line) {
    for (; i < n; i++) {
      if (mask[i] >= line)
	break;
    }
  }

  if (i == n) {
    *index = n;
    return FALSE;
  }

  if (mask[i] > line) {
    *index = i;
    return FALSE;
  } else if (mask[i] < line) {
    *index = n;
    return FALSE;
  }

  *index = i + 1;
  return TRUE;
}
#endif

#if NEW_MATH_CODE
static void
set_var(MathEquation *eq, MathValue *x, MathValue *y)
{
  math_equation_set_var(eq, 0, x);
  math_equation_set_var(eq, 1, y);
}

static int
set_const(MathEquation *eq, struct f2ddata *fp, int first)
{
  MathValue val;

  if (eq == NULL || eq->exp == NULL)
    return 0;

  math_equation_clear(eq);

  if (fp->need2pass) {
    math_equation_set_const(eq, fp->const_id[MATH_CONST_MINX], &fp->minx);
    math_equation_set_const(eq, fp->const_id[MATH_CONST_MINY], &fp->miny);

    math_equation_set_const(eq, fp->const_id[MATH_CONST_MAXX], &fp->maxx);
    math_equation_set_const(eq, fp->const_id[MATH_CONST_MAXY], &fp->maxy);

    val.val = fp->num;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_const(eq, fp->const_id[MATH_CONST_NUM], &val);

    val.val = fp->sumx;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_const(eq, fp->const_id[MATH_CONST_SUMX], &val);

    val.val = fp->sumy;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_const(eq, fp->const_id[MATH_CONST_SUMY], &val);

    val.val = fp->sumxx;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_const(eq, fp->const_id[MATH_CONST_SUMXX], &val);

    val.val = fp->sumyy;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_const(eq, fp->const_id[MATH_CONST_SUMYY], &val);

    val.val = fp->sumxy;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_const(eq, fp->const_id[MATH_CONST_SUMXY], &val);

    if (fp->num > 0) {
      val.val = fp->sumx / fp->num;
      val.type = MATH_VALUE_NORMAL;
      math_equation_set_const(eq, fp->const_id[MATH_CONST_AVX], &val);

      val.val = fp->sumy / fp->num;
      val.type = MATH_VALUE_NORMAL;
      math_equation_set_const(eq, fp->const_id[MATH_CONST_AVY], &val);

      val.val = sqrt(fp->sumxx / fp->num - (fp->sumx / fp->num) * (fp->sumx / fp->num));
      val.type = MATH_VALUE_NORMAL;
      math_equation_set_const(eq, fp->const_id[MATH_CONST_SGX], &val);

      val.val = sqrt(fp->sumyy / fp->num - (fp->sumy / fp->num) * (fp->sumy / fp->num));
      val.type = MATH_VALUE_NORMAL;
      math_equation_set_const(eq, fp->const_id[MATH_CONST_SGY], &val);
    }
  }

  val.val = chkobjlastinst(fp->obj) + 1;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, fp->const_id[MATH_CONST_N], &val);

  val.val = fp->id;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, fp->const_id[MATH_CONST_D], &val);

  val.val = first;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, fp->const_id[MATH_CONST_FIRST], &val);

  return math_equation_optimize(eq);
}

static int
file_calculate(struct f2ddata *fp, MathEquation *eq, MathValue *x, MathValue *y, MathValue *prm, MathValue *fprm, MathValue *val)
{
  if (eq == NULL || eq->exp == NULL)
    return 0;

  math_equation_set_parameter_data(eq, 0, prm);
  math_equation_set_parameter_data(eq, 'F', fprm);
  set_var(eq, x, y);
  math_equation_set_user_data(eq, fp);
  return math_equation_calculate(eq, val);
}
#endif

static int
#if NEW_MATH_CODE
getdata_sub2(struct f2ddata *fp, int fnumx, int fnumy, int *needx, int *needy, MathValue *datax, MathValue *datay,
	     MathValue *gdata, int filenum, int *openfile)
#else
getdata_sub2(struct f2ddata *fp, int fnumx, int fnumy, int *needx, int *needy, double *datax, double *datay,
	     char *statx, char *staty, double *gdata, char *gstat, int filenum, int *openfile)
#endif
{
  int i,j;
  int masked,moved,moven;
  struct f2ddata_buf *buf;
#if NEW_MATH_CODE
  MathValue dx, dy, dx2, dy2, dx3, dy3, d2, d3, *data2, *data3;
#else
  int first2,first3;
  char st;
  char *code2,*code3;
  int fnum2,fnum3,*need2,*need3;
  double *data2,*data3;
  char *stat2,*stat3;
  double val;
  double dx2,dy2,dx3,dy3;
  char dx2stat,dy2stat,dx3stat,dy3stat;
  double dx, dy, d2, d3;
  char dxstat, dystat, d2stat, d3stat;
#endif

#if MASK_SERACH_METHOD == MASK_SERACH_METHOD_LINER
  masked = FALSE;
  for (j = 0; j < fp->masknum; j++) {
    if (fp->mask[j] == fp->line) {
      masked = TRUE;
      break;
    }
  }
#elif MASK_SERACH_METHOD == MASK_SERACH_METHOD_BINARY
  masked = bsearch_int(fp->mask, fp->masknum, fp->line, NULL);
#else
  masked = search_mask(fp->mask, fp->masknum, &(fp->mask_index), fp->line);
#endif

  moved = FALSE;
  if (! masked) {
    for (j = 0; j < fp->movenum; j++) {
      if (fp->move[j] == fp->line) {
	moved = TRUE;
	moven = j;
	break;
      }
    }

    for (i = 0; i < fnumx; i++) {
      if (needx[i] / 1000 == fp->id) {
	j = needx[i] % 1000;
#if NEW_MATH_CODE
	datax[i] = gdata[j];
#else
	datax[i] = gdata[j];
	statx[i] = gstat[j];
#endif
      } else {
#if NEW_MATH_CODE
	datax[i].val = 0;
	datax[i].type = MNONUM;
#else
	datax[i] = 0;
	statx[i] = MNONUM;
#endif
      }
    }
    for (i = 0; i < fnumy; i++) {
      if (needy[i] / 1000 == fp->id) {
	j = needy[i] % 1000;
#if NEW_MATH_CODE
	datay[i] = gdata[j];
#else
	datay[i] = gdata[j];
	staty[i] = gstat[j];
#endif
      } else {
#if NEW_MATH_CODE
	datay[i].val = 0;
	datay[i].type = MNONUM;
#else
	datay[i] = 0;
	staty[i] = MNONUM;
#endif
      }
    }
    if (filenum) {
#if NEW_MATH_CODE
      getdata_get_other_files(fp, fnumx, fnumy, needx, needy, datax, datay, filenum, openfile);
#else
      getdata_get_other_files(fp, fnumx, fnumy, needx, needy, datax, datay, statx, staty, filenum, openfile);
#endif
    }
  }

#if NEW_MATH_CODE
  d2.val = d3.val = 0;
  d2.type = d3.type = MATH_VALUE_UNDEF;
  dx = gdata[fp->x];
  switch (fp->type) {
  case TYPE_DIAGONAL:
    dy = gdata[fp->x + 1];
    break;
  default:
    dy = gdata[fp->y];
  }

  dx2 = dx;
  dy2 = dy;
  if (fp->codex[0]) {
    file_calculate(fp, fp->codex[0], &dx2, &dy2, gdata, datax, &dx);
  }
  if (fp->codey[0]) {
    file_calculate(fp, fp->codey[0], &dx2, &dy2, gdata, datay, &dy);
  }

  switch (fp->type) {
  case TYPE_NORMAL:
    break;
  case TYPE_DIAGONAL:
    d2 = gdata[fp->y];
    d3 = gdata[fp->y + 1];

    data2 = datax;
    dx2 = d2;
    dy2 = d3;

    data3 = datay;
    dx3 = d2;
    dy3 = d3;

    if (fp->codex[1]) {
      file_calculate(fp, fp->codex[1], &dx2, &dy2, gdata, datax, &d2);
    }
    if (fp->codey[2]) {
      file_calculate(fp, fp->codey[2], &dx3, &dy3, gdata, datay, &d3);
    }
    break;
  case TYPE_ERR_X:
    d2.val = gdata[fp->x].val + gdata[fp->x + 1].val;

    if (gdata[fp->x].type < gdata[fp->x + 1].type) {
      d2.type = gdata[fp->x].type;
    } else {
      d2.type = gdata[fp->x + 1].type;
    }

    d3.val = gdata[fp->x].val + gdata[fp->x + 2].val;

    if (gdata[fp->x].type < gdata[fp->x + 2].type) {
      d3.type = gdata[fp->x].type;
    } else {
      d3.type = gdata[fp->x + 2].type;
    }

    dx2 = d2;
    dy2 = gdata[fp->y];

    dx3 = d3;
    dy3 = gdata[fp->y];

    if (fp->codex[1]) {
      file_calculate(fp, fp->codex[1], &dx2, &dy2, gdata, datax, &d2);
    }
    if (fp->codex[2]) {
      file_calculate(fp, fp->codex[2], &dx3, &dy3, gdata, datax, &d3);
    }
    break;
  case TYPE_ERR_Y:
    d2.val = gdata[fp->y].val + gdata[fp->y + 1].val;

    if (gdata[fp->y].type < gdata[fp->y + 1].type) {
      d2.type = gdata[fp->y].type;
    } else {
      d2.type = gdata[fp->y + 1].type;
    }

    d3.val = gdata[fp->y].val + gdata[fp->y + 2].val;

    if (gdata[fp->y].type < gdata[fp->y + 2].type) {
      d3.type = gdata[fp->y].type;
    } else {
      d3.type = gdata[fp->y + 2].type;
    }

    dx2 = gdata[fp->x];
    dy2 = d2;

    dx3 = gdata[fp->x];
    dy3 = d3;

    file_calculate(fp, fp->codey[1], &dx2, &dy2, gdata, datax, &d2);
    file_calculate(fp, fp->codey[2], &dx3, &dy3, gdata, datax, &d3);
    break;
  }

  if (masked) {
    dx.type = dy.type = d2.type = d3.type = MATH_VALUE_CONT;
  }
  if (moved) {
    dx.type = dy.type = d2.type = d3.type = MATH_VALUE_NORMAL;
    dx.val = fp->movex[moven];
    dy.val = fp->movey[moven];
    switch (fp->type) {
    case TYPE_ERR_X:
      d2 = dx;
      d3 = dx;
      break;
    case TYPE_ERR_Y:
      d2 = dy;
      d3 = dy;
      break;
    default:
      d2 = dx;
      d3 = dy;
      break;
    }
  }

#else
  d2 = d3 = 0;
  d2stat = d3stat = MUNDEF;
  dx = gdata[fp->x];
  dxstat = gstat[fp->x];
  switch (fp->type) {
  case TYPE_DIAGONAL:
    dy = gdata[fp->x+1];
    dystat = gstat[fp->x+1];
    break;
  default:
    dy = gdata[fp->y];
    dystat = gstat[fp->y];
  }
  dx2 = dx;
  dx2stat = dxstat;
  dy2 = dy;
  dy2stat = dystat;

  if (fp->codex) {
    st=calculate(fp->codex,1,
		 dx2,dx2stat,dy2,dy2stat,0,MNOERR,
		 fp->minx,fp->minxstat,fp->maxx,fp->maxxstat,
		 fp->miny,fp->minystat,fp->maxy,fp->maxystat,
		 fp->num,fp->sumx,fp->sumy,fp->sumxx,fp->sumyy,fp->sumxy,
		 gdata,gstat,
		 fp->memoryx,fp->memorystatx,
		 fp->sumdatax,fp->sumstatx,
		 fp->difdatax,fp->difstatx,
		 fp->color,&(fp->marksize),&(fp->marktype),
		 fp->codef,fp->codeg,fp->codeh,
		 fnumx,needx,datax,statx,fp->id,&val);
    dx = val;
    dxstat = st;
  }
  if (fp->codey) {
    st=calculate(fp->codey,1,
		 dx2,dx2stat,dy2,dy2stat,0,MNOERR,
		 fp->minx,fp->minxstat,fp->maxx,fp->maxxstat,
		 fp->miny,fp->minystat,fp->maxy,fp->maxystat,
		 fp->num,fp->sumx,fp->sumy,fp->sumxx,fp->sumyy,fp->sumxy,
		 gdata,gstat,
		 fp->memoryy,fp->memorystaty,
		 fp->sumdatay,fp->sumstaty,
		 fp->difdatay,fp->difstaty,
		 fp->color,&(fp->marksize),&(fp->marktype),
		 fp->codef,fp->codeg,fp->codeh,
		 fnumy,needy,datay,staty,fp->id,&val);
    dy=val;
    dystat=st;
  }

  switch (fp->type) {
  case TYPE_NORMAL:
    break;
  case TYPE_DIAGONAL:
    d2=gdata[fp->y];
    d2stat=gstat[fp->y];
    d3=gdata[fp->y+1];
    d3stat=gstat[fp->y+1];
    code2=fp->codex;
    fnum2=fnumx;
    need2=needx;
    data2=datax;
    stat2=statx;
    dx2=d2;
    dx2stat=d2stat;
    dy2=d3;
    dy2stat=d3stat;
    first2=0;
    code3=fp->codey;
    fnum3=fnumy;
    need3=needy;
    data3=datay;
    stat3=staty;
    dx3=d2;
    dx3stat=d2stat;
    dy3=d3;
    dy3stat=d3stat;
    first3=0;
    break;
  case TYPE_ERR_X:
    d2=gdata[fp->x]+gdata[fp->x+1];
    if (gstat[fp->x]<gstat[fp->x+1]) d2stat=gstat[fp->x];
    else d2stat=gstat[fp->x+1];
    d3=gdata[fp->x]+gdata[fp->x+2];
    if (gstat[fp->x]<gstat[fp->x+2]) d3stat=gstat[fp->x];
    else d3stat=gstat[fp->x+2];
    code2=fp->codex;
    fnum2=fnumx;
    need2=needx;
    data2=datax;
    stat2=statx;
    dx2=d2;
    dx2stat=d2stat;
    dy2=gdata[fp->y];
    dy2stat=gstat[fp->y];
    first2=1;
    code3=fp->codex;
    fnum3=fnumx;
    need3=needx;
    data3=datax;
    stat3=statx;
    dx3=d3;
    dx3stat=d3stat;
    dy3=gdata[fp->y];
    dy3stat=gstat[fp->y];
    first3=0;
    break;
  case TYPE_ERR_Y:
    d2=gdata[fp->y]+gdata[fp->y+1];
    if (gstat[fp->y]<gstat[fp->y+1]) d2stat=gstat[fp->y];
    else d2stat=gstat[fp->y+1];
    d3=gdata[fp->y]+gdata[fp->y+2];
    if (gstat[fp->y]<gstat[fp->y+2]) d3stat=gstat[fp->y];
    else d3stat=gstat[fp->y+2];
    code2=fp->codey;
    fnum2=fnumy;
    need2=needy;
    data2=datay;
    stat2=staty;
    dx2=gdata[fp->x];
    dx2stat=gstat[fp->x];
    dy2=d2;
    dy2stat=d2stat;
    first2=1;
    code3=fp->codey;
    fnum3=fnumy;
    need3=needy;
    data3=datay;
    stat3=staty;
    dx3=gdata[fp->x];
    dx3stat=gstat[fp->x];
    dy3=d3;
    dy3stat=d3stat;
    first3=0;
    break;
  }

  if (fp->type != TYPE_NORMAL) {
    if (code2) {
      st=calculate(code2,first2,
		   dx2,dx2stat,dy2,dy2stat,0,MNOERR,
		   fp->minx,fp->minxstat,fp->maxx,fp->maxxstat,
		   fp->miny,fp->minystat,fp->maxy,fp->maxystat,
		   fp->num,fp->sumx,fp->sumy,fp->sumxx,fp->sumyy,fp->sumxy,
		   gdata,gstat,
		   fp->memory2,fp->memorystat2,
		   fp->sumdata2,fp->sumstat2,
		   fp->difdata2,fp->difstat2,
		   fp->color,&(fp->marksize),&(fp->marktype),
		   fp->codef,fp->codeg,fp->codeh,
		   fnum2,need2,data2,stat2,fp->id,&val);
      d2=val;
      d2stat=st;
    }
    if (code3) {
      st=calculate(code3,first3,
		   dx3,dx3stat,dy3,dy3stat,0,MNOERR,
		   fp->minx,fp->minxstat,fp->maxx,fp->maxxstat,
		   fp->miny,fp->minystat,fp->maxy,fp->maxystat,
		   fp->num,fp->sumx,fp->sumy,fp->sumxx,fp->sumyy,fp->sumxy,
		   gdata,gstat,
		   fp->memory3,fp->memorystat3,
		   fp->sumdata3,fp->sumstat3,
		   fp->difdata3,fp->difstat3,
		   fp->color,&(fp->marksize),&(fp->marktype),
		   fp->codef,fp->codeg,fp->codeh,
		   fnum3,need3,data3,stat3,fp->id,&val);
      d3=val;
      d3stat=st;
    }
  }
  if (masked) {
    dxstat = dystat = d2stat = d3stat = MSCONT;
  }
  if (moved) {
    dxstat = dystat = d2stat = d3stat = MNOERR;
    dx = fp->movex[moven];
    dy = fp->movey[moven];
    switch (fp->type) {
    case TYPE_ERR_X:
      d2=dx;
      d3=dx;
      break;
    case TYPE_ERR_Y:
      d2=dy;
      d3=dy;
      break;
    default:
      d2=dx;
      d3=dy;
      break;
    }
  }

#endif

#if BUF_TYPE == USE_BUF_PTR
  buf = fp->buf_ptr[fp->bufnum];
#elif BUF_TYPE == USE_RING_BUF
  buf = &fp->buf[ring_buf_index(fp, fp->bufnum)];
#else
  buf = &fp->buf[fp->bufnum];
#endif
  buf->colr = fp->color[0];
  buf->colg = fp->color[1];
  buf->colb = fp->color[2];
  buf->marksize = fp->marksize;
  buf->marktype = fp->marktype;
  buf->line = fp->line;
#if NEW_MATH_CODE
  buf->colr2 = fp->color2[0];
  buf->colg2 = fp->color2[1];
  buf->colb2 = fp->color2[2];
  buf->dx = dx.val;
  buf->dy = dy.val;
  buf->d2 = d2.val;
  buf->d3 = d3.val;
  buf->dxstat = dx.type;
  buf->dystat = dy.type;
  buf->d2stat = d2.type;
  buf->d3stat = d3.type;
#else
  buf->dx = dx;
  buf->dy = dy;
  buf->d2 = d2;
  buf->d3 = d3;
  buf->dxstat = dxstat;
  buf->dystat = dystat;
  buf->d2stat = d2stat;
  buf->d3stat = d3stat;
#endif
  fp->bufnum++;

  if (fp->rstep > 1)
    return getdata_skip_step(fp);

  return 0;
}

static int
#if NEW_MATH_CODE
getdata_sub1(struct f2ddata *fp, int fnumx, int fnumy, int *needx, int *needy,
	     MathValue *datax, MathValue *datay, MathValue *gdata, int filenum, int *openfile)
#else
getdata_sub1(struct f2ddata *fp, int fnumx, int fnumy, int *needx, int *needy, double *datax, double *datay,
	     char *statx, char *staty, double *gdata, char *gstat, int filenum, int *openfile)
#endif
{
  char *buf;
  int i, rcode;

  while (! fp->eof && fp->bufnum < DXBUFSIZE) {
    if (fp->final >= 0 && fp->line >= fp->final) {
      fp->eof=TRUE;
      break;
    }

    if ((fp->line & 0x1fff) == 0 && set_data_progress(fp)) {
      break;
    }

    rcode = fgetline(fp->fd,&buf);
    if (rcode == 1) {
      fp->eof=TRUE;
      break;
    } else if (rcode==-1) {
      return -1;
    }

    fp->line++;
    for (i = 0; buf[i] != '\0' && CHECK_IFS(fp->ifs_buf, buf[i]); i++);

    if (buf[i] == '\0' || (fp->remark && CHECK_REMARK(fp->ifs_buf, buf[i]))) {
      memfree(buf);
    } else {
#if NEW_MATH_CODE
      rcode = getdataarray(buf, fp->maxdim, &fp->count, gdata, fp->ifs_buf, fp->csv);
#else
      rcode = getdataarray(buf, fp->maxdim, &(fp->count), gdata, gstat, fp->ifs_buf, fp->csv);
#endif
      memfree(buf);
      if (rcode==-1) {
	return -1;
      }
#if NEW_MATH_CODE
      getdata_sub2(fp, fnumx, fnumy, needx, needy, datax, datay, gdata, filenum, openfile);
#else
      getdata_sub2(fp, fnumx, fnumy, needx, needy, datax, datay,
		   statx, staty, gdata, gstat, filenum, openfile);
#endif
    }
    if ((fp->final>=0) && (fp->line>=fp->final)) fp->eof=TRUE;
  }
  return 0;
}

static int 
getdata(struct f2ddata *fp)
/*
  return -1: fatal error
          0: no error
          1: EOF
*/
{
  int i,rcode;
  double sumx,sumy,sum2,sum3;
  int numx,numy,num2,num3,num,smx,smy,sm2,sm3;
  int filenum,*openfile,*needx,*needy;
  struct narray filedatax,filedatay;
  unsigned int fnumx,fnumy,j;
  struct f2ddata_buf *buf;
#if BUF_TYPE == USE_RING_BUF
  int n;
#endif
#if NEW_MATH_CODE
  MathValue *datax,*datay;
  static MathValue gdata[FILE_OBJ_MAXCOL + 1];
  static MathValue math_value_zero = {0, 0};
#else
  double *datax,*datay;
  char *statx,*staty;
  double dx,dy;
  char st;
  struct narray filestatx,filestaty;
  double *gdata;
  char *gstat;
#endif

#if ! NEW_MATH_CODE
  gdata = (double *) memalloc(sizeof(double) * (FILE_OBJ_MAXCOL + 1));
  gstat = (char *) memalloc(sizeof(char) * (FILE_OBJ_MAXCOL + 1));
  if (gdata == NULL || gstat == NULL) {
   memfree(gdata);
   memfree(gstat);
   return -1;
  }
#endif

  fp->dx=fp->dy=fp->d2=fp->d3=0;
  fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MUNDEF;
  filenum=arraynum(&(fp->fileopen));
  openfile=arraydata(&(fp->fileopen));

#if NEW_MATH_CODE
  fnumx = fp->fnumx;
  needx = fp->needx;
  arrayinit(&filedatax, sizeof(MathValue));
  for (j = 0; j < fnumx; j++) {
    arrayadd(&filedatax, &math_value_zero);
  }
  if (arraynum(&filedatax) < fnumx) {
    fnumx = arraynum(&filedatax);
  }
  datax = arraydata(&filedatax);

  fnumy = fp->fnumy;
  needy = fp->needy;
  arrayinit(&filedatay, sizeof(MathValue));
  for (j = 0; j < fnumy; j++) {
    arrayadd(&filedatay, &math_value_zero);
  }
  if (arraynum(&filedatay) < fnumy) {
    fnumy = arraynum(&filedatay);
  }
  datay = arraydata(&filedatay);

  rcode = getdata_sub1(fp, fnumx, fnumy, needx, needy, datax, datay, gdata, filenum, openfile);
#else
  fnumx=arraynum(fp->needfilex);
  needx=arraydata(fp->needfilex);
  arrayinit(&filedatax,sizeof(double));
  arrayinit(&filestatx,sizeof(char));
  for (j=0;j<fnumx;j++) {
    dx=0;
    st=MNONUM;
    arrayadd(&filedatax,&dx);
    arrayadd(&filestatx,&st);
  }
  if (arraynum(&filedatax)<fnumx) fnumx=arraynum(&filedatax);
  if (arraynum(&filestatx)<fnumx) fnumx=arraynum(&filestatx);
  datax=arraydata(&filedatax);
  statx=arraydata(&filestatx);

  fnumy=arraynum(fp->needfiley);
  needy=arraydata(fp->needfiley);
  arrayinit(&filedatay,sizeof(double));
  arrayinit(&filestaty,sizeof(char));
  for (j=0;j<fnumy;j++) {
    dy=0;
    st=MNONUM;
    arrayadd(&filedatay,&dy);
    arrayadd(&filestaty,&st);
  }
  if (arraynum(&filedatay)<fnumy) fnumy=arraynum(&filedatay);
  if (arraynum(&filestaty)<fnumy) fnumy=arraynum(&filestaty);
  datay=arraydata(&filedatay);
  staty=arraydata(&filestaty);

  rcode = getdata_sub1(fp, fnumx, fnumy, needx, needy, datax, datay,
		       statx, staty, gdata, gstat,
		       filenum, openfile);
#endif


  if (rcode) {
#if ! NEW_MATH_CODE
    memfree(gdata);
    memfree(gstat);
#endif
    return rcode;
  }

  arraydel(&filedatax);
  arraydel(&filedatay);
#if ! NEW_MATH_CODE
  arraydel(&filestatx);
  arraydel(&filestaty);
#endif
  if ((fp->bufnum==0) || (fp->bufpo>=fp->bufnum)) {
    fp->dx=fp->dy=fp->d2=fp->d3=0;
    fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MEOF;
#if ! NEW_MATH_CODE
    memfree(gdata);
    memfree(gstat);
#endif
    return 1;
  }
  switch (fp->type) {
  case TYPE_NORMAL:
    smx=fp->smoothx;
    smy=fp->smoothy;
    sm2=0;
    sm3=0;
    break;
  case TYPE_DIAGONAL:
    smx=fp->smoothx;
    smy=fp->smoothy;
    sm2=fp->smoothx;
    sm3=fp->smoothy;
    break;
  case TYPE_ERR_X:
    smx=fp->smoothx;
    smy=fp->smoothy;
    sm2=fp->smoothx;
    sm3=fp->smoothx;
    break;
  case TYPE_ERR_Y:
    smx=fp->smoothx;
    smy=fp->smoothy;
    sm2=fp->smoothy;
    sm3=fp->smoothy;
    break;
  default:
    /* never reached */
    smx = 0;
    smy = 0;
    sm2 = 0;
    sm3 = 0;
    break;
  }
  sumx=sumy=sum2=sum3=0;
  numx=numy=num2=num3=0;
  if (fp->bufpo+fp->smooth>=fp->bufnum) num=fp->bufnum-1;
  else num=fp->bufpo+fp->smooth;

#if BUF_TYPE == USE_BUF_PTR
  for (i = 0; i <= num; i++) {
    buf = fp->buf_ptr[i];
    if (buf->dxstat == MNOERR &&
	i >= fp->bufpo - smx &&
	i <= fp->bufpo + smx) {
      sumx += buf->dx;
      numx++;
    }
    if (buf->dystat == MNOERR &&
	i >= fp->bufpo - smy &&
	i <= fp->bufpo + smy) {
      sumy += buf->dy;
      numy++;
    }
    if (buf->d2stat == MNOERR &&
	i >= fp->bufpo - sm2 &&
	i <= fp->bufpo + sm2) {
      sum2 += buf->d2;
      num2++;
    }
    if (buf->d3stat == MNOERR &&
	i >= fp->bufpo - sm3 &&
	i <= fp->bufpo + sm3) {
      sum3 += buf->d3;
      num3++;
    }
  }

  buf = fp->buf_ptr[fp->bufpo];

  if (numx != 0)
    fp->dx = sumx / numx;
  fp->dxstat = buf->dxstat;

  if (numy != 0)
    fp->dy=sumy/numy;
  fp->dystat = buf->dystat;

  if (num2 != 0)
    fp->d2 = sum2/num2;
  fp->d2stat = buf->d2stat;

  if (num3 != 0)
    fp->d3 = sum3 / num3;
  fp->d3stat = buf->d3stat;
  fp->dline = buf->line;
  fp->colr = buf->colr;
  fp->colg = buf->colg;
  fp->colb = buf->colb;
#if NEW_MATH_CODE
  fp->colr2 = buf->colr2;
  fp->colg2 = buf->colg2;
  fp->colb2 = buf->colb2;
#endif
  fp->msize = buf->marksize;
  fp->mtype = buf->marktype;
#elif BUF_TYPE == USE_RING_BUF
  for (i=0;i<=num;i++) {
    n = ring_buf_index(fp, i);
    if ((fp->buf[n].dxstat==MNOERR)
     && (i>=fp->bufpo-smx) && (i<=fp->bufpo+smx)) {
      sumx+=fp->buf[n].dx;
      numx++;
    }
    if ((fp->buf[n].dystat==MNOERR)
     && (i>=fp->bufpo-smy) && (i<=fp->bufpo+smy)) {
      sumy+=fp->buf[n].dy;
      numy++;
    }
    if ((fp->buf[n].d2stat==MNOERR)
     && (i>=fp->bufpo-sm2) && (i<=fp->bufpo+sm2)) {
      sum2+=fp->buf[n].d2;
      num2++;
    }
    if ((fp->buf[n].d3stat==MNOERR)
     && (i>=fp->bufpo-sm3) && (i<=fp->bufpo+sm3)) {
      sum3+=fp->buf[n].d3;
      num3++;
    }
  }
  n = ring_buf_index(fp, fp->bufpo);
  if (numx!=0) fp->dx=sumx/numx;
  fp->dxstat=fp->buf[n].dxstat;
  if (numy!=0) fp->dy=sumy/numy;
  fp->dystat=fp->buf[n].dystat;
  if (num2!=0) fp->d2=sum2/num2;
  fp->d2stat=fp->buf[n].d2stat;
  if (num3!=0) fp->d3=sum3/num3;
  fp->d3stat=fp->buf[n].d3stat;
  fp->dline=fp->buf[n].line;
  fp->colr=fp->buf[n].colr;
  fp->colg=fp->buf[n].colg;
  fp->colb=fp->buf[n].colb;
#if NEW_MATH_CODE
  fp->colr2=fp->buf[n].colr2;
  fp->colg2=fp->buf[n].colg2;
  fp->colb2=fp->buf[n].colb2;
#endif
  fp->msize=fp->buf[n].marksize;
  fp->mtype=fp->buf[n].marktype;
#else  /* BUF_TYPE */
  for (i=0;i<=num;i++) {
    if ((fp->buf[i].dxstat==MNOERR)
     && (i>=fp->bufpo-smx) && (i<=fp->bufpo+smx)) {
      sumx+=fp->buf[i].dx;
      numx++;
    }
    if ((fp->buf[i].dystat==MNOERR)
     && (i>=fp->bufpo-smy) && (i<=fp->bufpo+smy)) {
      sumy+=fp->buf[i].dy;
      numy++;
    }
    if ((fp->buf[i].d2stat==MNOERR)
     && (i>=fp->bufpo-sm2) && (i<=fp->bufpo+sm2)) {
      sum2+=fp->buf[i].d2;
      num2++;
    }
    if ((fp->buf[i].d3stat==MNOERR)
     && (i>=fp->bufpo-sm3) && (i<=fp->bufpo+sm3)) {
      sum3+=fp->buf[i].d3;
      num3++;
    }
  }
  if (numx!=0) fp->dx=sumx/numx;
  fp->dxstat=fp->buf[fp->bufpo].dxstat;
  if (numy!=0) fp->dy=sumy/numy;
  fp->dystat=fp->buf[fp->bufpo].dystat;
  if (num2!=0) fp->d2=sum2/num2;
  fp->d2stat=fp->buf[fp->bufpo].d2stat;
  if (num3!=0) fp->d3=sum3/num3;
  fp->d3stat=fp->buf[fp->bufpo].d3stat;
  fp->dline=fp->buf[fp->bufpo].line;
  fp->colr=fp->buf[fp->bufpo].colr;
  fp->colg=fp->buf[fp->bufpo].colg;
  fp->colb=fp->buf[fp->bufpo].colb;
#if NEW_MATH_CODE
  fp->colr2=fp->buf[fp->bufpo].colr2;
  fp->colg2=fp->buf[fp->bufpo].colg2;
  fp->colb2=fp->buf[fp->bufpo].colb2;
#endif
  fp->msize=fp->buf[fp->bufpo].marksize;
  fp->mtype=fp->buf[fp->bufpo].marktype;
#endif	/* BUF_TYPE */

  switch (fp->type) {
  case TYPE_NORMAL:
    if (fp->dxstat==MNOERR && fp->dystat==MNOERR)
      fp->datanum++;
    break;
  case TYPE_DIAGONAL:
    if (fp->dxstat==MNOERR && fp->dystat==MNOERR && fp->d2stat==MNOERR && fp->d3stat==MNOERR)
      fp->datanum++;
    break;
  case TYPE_ERR_X:
    if (fp->dystat==MNOERR && fp->d2stat==MNOERR && fp->d3stat==MNOERR)
      fp->datanum++;
    break;
  case TYPE_ERR_Y:
    if (fp->dxstat==MNOERR && fp->d2stat==MNOERR && fp->d3stat==MNOERR)
      fp->datanum++;
    break;
  }

  if (fp->bufpo<fp->smooth) {
    fp->bufpo++;
  } else {
#if BUF_TYPE == USE_BUF_PTR
    if (fp->bufnum > 0) {
      struct f2ddata_buf *tmp;

      fp->bufnum--;
      tmp = fp->buf_ptr[0];
#if USE_MEMMOVE
      memmove(fp->buf_ptr, fp->buf_ptr + 1, sizeof(*fp->buf_ptr) * fp->bufnum);
#else
      for (i = 0; i < fp->bufnum; i++) {
	fp->buf_ptr[i] = fp->buf_ptr[i + 1];
      }
#endif
      fp->buf_ptr[fp->bufnum] = tmp;
    }
#elif BUF_TYPE == USE_RING_BUF
    if (fp->bufnum > 0) {
      fp->bufnum--;
      fp->ringbuf_top = RING_BUF_INC(fp->ringbuf_top);
    }
#else  /* BUF_TYPE */
    if (fp->bufnum > 0) {
      fp->bufnum--;
#if USE_MEMMOVE
      memmove(fp->buf, fp->buf + 1, sizeof(*fp->buf) * fp->bufnum);
#else
      for (i = 0; i < fp->bufnum; i++) {
	fp->buf[i] = fp->buf[i + 1];
      }
#endif
    }
#endif	/* BUF_TYPE */
  }
#if ! NEW_MATH_CODE
  memfree(gdata);
  memfree(gstat);
#endif
  return 0;
}

static int 
#if NEW_MATH_CODE
getdata2(struct f2ddata *fp, MathEquation *code, int maxdim, double *dd, char *ddstat)
#else
getdata2(struct f2ddata *fp,char *code,int maxdim,double *dd,char *ddstat)
#endif
/*
  return -1: fatal error
          0: no error
          1: EOF
*/
{
  char *buf;
  int i,step,rcode;
#if NEW_MATH_CODE
  MathValue val, *gdata, dx2, dy2;
#else
  double *gdata;
  char *gstat;
  double val;
  char st;
  double dx2,dy2;
  char dx2stat,dy2stat;
#endif
  int masked;
  int find;

#if NEW_MATH_CODE
  gdata = memalloc(sizeof(MathValue) * (FILE_OBJ_MAXCOL + 1));
  if (gdata == NULL) {
    memfree(gdata);
   return -1;
  }
#else
  gdata = (double *)memalloc(sizeof(double)*(FILE_OBJ_MAXCOL+1));
  gstat = (char *)memalloc(sizeof(char)*(FILE_OBJ_MAXCOL+1));
  if (gdata == NULL || gstat  == NULL) {
    memfree(gdata);
    memfree(gstat);
   return -1;
  }
#endif

  *dd=0;
  *ddstat=MUNDEF;
  find=FALSE;
  while (!fp->eof && (!find)) {
    if ((fp->final>=0) && (fp->line>=fp->final)) {
      fp->eof=TRUE;
      break;
    }
    if ((rcode=fgetline(fp->fd,&buf))==1) {
      fp->eof=TRUE;
      break;
    }
    if (rcode==-1) {
      memfree(gdata);
#if ! NEW_MATH_CODE
      memfree(gstat);
#endif
      return -1;
    }
    fp->line++;
    for (i=0;(buf[i]!='\0') && CHECK_IFS(fp->ifs_buf, buf[i]); i++);
    if ((buf[i]!='\0')
    && ((fp->remark==NULL) || ! CHECK_REMARK(fp->ifs_buf, buf[i]))) {
#if NEW_MATH_CODE
      rcode = getdataarray(buf, maxdim, &fp->count, gdata, fp->ifs_buf, fp->csv);
#else
      rcode=getdataarray(buf,maxdim,&(fp->count),gdata,gstat,fp->ifs_buf,fp->csv);
#endif
      memfree(buf);
#if MASK_SERACH_METHOD == MASK_SERACH_METHOD_LINER
      for (j=0;j<fp->masknum;j++)
        if ((fp->mask)[j]==fp->line) break;
      if (j!=fp->masknum) masked=TRUE;
      else masked=FALSE;
#elif MASK_SERACH_METHOD == MASK_SERACH_METHOD_BINARY
      masked = bsearch_int(fp->mask, fp->masknum, fp->line, NULL);
#else
      masked = search_mask(fp->mask, fp->masknum, &(fp->mask_index), fp->line);
#endif
      if (rcode==-1) {
	memfree(gdata);
#if ! NEW_MATH_CODE
	memfree(gstat);
#endif
	return -1;
      }
      *dd=0;
      *ddstat=MUNDEF;
#if NEW_MATH_CODE
      dx2 = gdata[fp->x];
      dy2 = gdata[fp->y];
      if (code && code->exp) {
	file_calculate(fp, code, &dx2, &dy2, gdata, NULL, &val);
        *dd = val.val;
        *ddstat = val.type;
      }
#else
      dx2=gdata[fp->x];
      dx2stat=gstat[fp->x];
      dy2=gdata[fp->y];
      dy2stat=gstat[fp->y];
      if (code!=NULL) {
        st=calculate(code,1,
                     dx2,dx2stat,dy2,dy2stat,0,MNOERR,
                     fp->minx,fp->minxstat,fp->maxx,fp->maxxstat,
                     fp->miny,fp->minystat,fp->maxy,fp->maxystat,
                     fp->num,fp->sumx,fp->sumy,fp->sumxx,fp->sumyy,fp->sumxy,
                     gdata,gstat,
                     fp->memoryx,fp->memorystatx,
                     fp->sumdatax,fp->sumstatx,
                     fp->difdatax,fp->difstatx,
                     NULL,NULL,NULL,
                     fp->codef,fp->codeg,fp->codeh,
                     0,NULL,NULL,NULL,fp->id,&val);
        *dd=val;
        *ddstat=st;
      }
#endif
      if (masked) *ddstat=MSCONT;
      find=TRUE;
      fp->dline=fp->line;
      step=1;
      while (step<fp->rstep) {
        if ((rcode=fgetline(fp->fd,&buf))==1) {
          fp->eof=TRUE;
          break;
        }
        if (rcode==-1) {
          memfree(gdata);
#if ! NEW_MATH_CODE
          memfree(gstat);
#endif
          return -1;
        }
        fp->line++;
        for (i=0;(buf[i]!='\0') && CHECK_IFS(fp->ifs_buf, buf[i]);i++);
        if ((buf[i]!='\0')
        && ((fp->remark==NULL) || ! CHECK_REMARK(fp->ifs_buf, buf[i])))
          step++;
        memfree(buf);
      }
    } else memfree(buf);
    if ((fp->final>=0) && (fp->line>=fp->final)) fp->eof=TRUE;
  }
  memfree(gdata);
#if ! NEW_MATH_CODE
  memfree(gstat);
#endif
  if (!find) {
    *dd=0;
    *ddstat=MEOF;
    return 1;
  }
  return 0;
}

static int 
#if NEW_MATH_CODE
getdataraw(struct f2ddata *fp, int maxdim, MathValue *data)
#else
getdataraw(struct f2ddata *fp,int maxdim,double *data,char *stat)
#endif
/*
  return -1: fatal error
		  0: no error
		  1: EOF
*/
{
  char *buf;
  int i,rcode;
  int masked;
  double dx,dy,d2,d3;
  char dxstat,dystat,d2stat,d3stat;
  int datanum;

  fp->dx=fp->dy=fp->d2=fp->d3=0;
  fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MUNDEF;
  datanum=0;
  while (!fp->eof && (datanum==0)) {
    if ((fp->final>=0) && (fp->line>=fp->final)) {
      fp->eof=TRUE;
      break;
    }

    if ((fp->line & 0x1fff) == 0 && set_data_progress(fp)) {
      break;
    }

    if ((rcode=fgetline(fp->fd,&buf))==1) {
      fp->eof=TRUE;
      break;
    }
    if (rcode==-1) return -1;
    fp->line++;
    for (i=0;(buf[i]!='\0') && CHECK_IFS(fp->ifs_buf, buf[i]); i++);
    if ((buf[i]!='\0')
    && ((fp->remark==NULL) || ! CHECK_REMARK(fp->ifs_buf, buf[i]))) {
#if NEW_MATH_CODE
      rcode = getdataarray(buf, maxdim, &fp->count, data, fp->ifs_buf, fp->csv);
#else
      rcode=getdataarray(buf,maxdim,&(fp->count),data,stat,fp->ifs_buf,fp->csv);
#endif
      memfree(buf);
#if MASK_SERACH_METHOD == MASK_SERACH_METHOD_LINER
      for (j=0;j<fp->masknum;j++)
        if ((fp->mask)[j]==fp->line) break;
      if (j!=fp->masknum) masked=TRUE;
      else masked=FALSE;
#elif MASK_SERACH_METHOD == MASK_SERACH_METHOD_BINARY
      masked = bsearch_int(fp->mask, fp->masknum, fp->line, NULL);
#else
      masked = search_mask(fp->mask, fp->masknum, &(fp->mask_index), fp->line);
#endif
      if (rcode==-1) return -1;
      dx=dy=d2=d3=0;
      dxstat=dystat=d2stat=d3stat=MUNDEF;
#if NEW_MATH_CODE
      dx = data[fp->x].val;
      dxstat = data[fp->x].type;;
#else
      dx=data[fp->x];
      dxstat=stat[fp->x];
#endif

      switch (fp->type) {
      case TYPE_DIAGONAL:
#if NEW_MATH_CODE
        dy = data[fp->x + 1].val;
        dystat = data[fp->x + 1].type;
#else
        dy=data[fp->x+1];
        dystat=stat[fp->x+1];
#endif
	break;
      default:
#if NEW_MATH_CODE
        dy = data[fp->y].val;
        dystat = data[fp->y].type;
#else
        dy=data[fp->y];
        dystat=stat[fp->y];
#endif
      }

      switch (fp->type) {
      case TYPE_NORMAL:
	break;
      case TYPE_DIAGONAL:
#if NEW_MATH_CODE
	d2 = data[fp->y].val;
        d2stat = data[fp->y].type;
        d3 = data[fp->y + 1].val;
        d3stat = data[fp->y + 1].type;
#else
        d2=data[fp->y];
        d2stat=stat[fp->y];
        d3=data[fp->y+1];
        d3stat=stat[fp->y+1];
#endif
	break;
      case TYPE_ERR_X:
#if NEW_MATH_CODE
        d2 = data[fp->x].val + data[fp->x + 1].val;
        if (data[fp->x].type < data[fp->x + 1].type) d2stat = data[fp->x].type;
        else d2stat = data[fp->x + 1].type;
        fp->d3 = data[fp->x].val + data[fp->x + 2].val;
        if (data[fp->x].type < data[fp->x + 2].type) d3stat = data[fp->x].type;
        else d3stat = data[fp->x + 2].type;
#else
        d2=data[fp->x]+data[fp->x+1];
        if (stat[fp->x]<stat[fp->x+1]) d2stat=stat[fp->x];
        else d2stat=stat[fp->x+1];
        fp->d3=data[fp->x]+data[fp->x+2];
        if (stat[fp->x]<stat[fp->x+2]) d3stat=stat[fp->x];
        else d3stat=stat[fp->x+2];
#endif
	break;
      case TYPE_ERR_Y:
#if NEW_MATH_CODE
        d2 = data[fp->y].val + data[fp->y + 1].val;
        if (data[fp->y].type < data[fp->y + 1].type) d2stat = data[fp->y].type;
        else d2stat = data[fp->y + 1].type;
	d3 = data[fp->y].val + data[fp->y + 2].val;
        if (data[fp->y].type < data[fp->y + 2].type) d3stat = data[fp->y].type;
        else d3stat = data[fp->y + 2].type;
	break;
#else
        d2=data[fp->y]+data[fp->y+1];
        if (stat[fp->y]<stat[fp->y+1]) d2stat=stat[fp->y];
        else d2stat=stat[fp->y+1];
        d3=data[fp->y]+data[fp->y+2];
        if (stat[fp->y]<stat[fp->y+2]) d3stat=stat[fp->y];
        else d3stat=stat[fp->y+2];
	break;
#endif
      }
      if (masked) {
        dxstat=dystat=d2stat=d3stat=MSCONT;
        for (i=0;i<=maxdim;i++)
#if NEW_MATH_CODE
          data[i].type = MATH_VALUE_CONT;
#else
          stat[i]=MSCONT;
#endif
      }
      fp->dx=dx;
      fp->dy=dy;
      fp->d2=d2;
      fp->d3=d3;
      fp->dxstat=dxstat;
      fp->dystat=dystat;
      fp->d2stat=d2stat;
      fp->d3stat=d3stat;
      fp->colr=fp->color[0];
      fp->colg=fp->color[1];
      fp->colb=fp->color[2];
#if NEW_MATH_CODE
      fp->colr2=fp->color2[0];
      fp->colg2=fp->color2[1];
      fp->colb2=fp->color2[2];
#endif
      datanum++;

      if (fp->rstep > 1 && getdata_skip_step(fp))
	return -1;

      fp->datanum++;

    } else memfree(buf);
    if ((fp->final>=0) && (fp->line>=fp->final)) fp->eof=TRUE;
  }
  if (datanum==0) {
    fp->dx=fp->dy=fp->d2=fp->d3=0;
    fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MEOF;
    return 1;
  }
  return 0;
}

static void
set_final_line(struct f2ddata *fp, struct f2dlocal *local)
{
  if (fp->final < -1) {
    fp->final += local->total_line + 1;
    if (fp->final < 0) {
      fp->final = 0;
    }
  }
}

static int
check_mtime(struct f2ddata *fp, struct f2dlocal *local)
{
  if (local->mtime != fp->mtime) {
    return 1;
  }

  fp->datanum = local->num;
  fp->dx = fp->dy = fp->d2 = fp->d3=0;
  fp->dxstat = fp->dystat = fp->d2stat = fp->d3stat = MUNDEF;
  fp->dline = 0;
#if ! NEW_MATH_CODE
  fp->minxstat = local->minxstat;
  fp->maxxstat = local->maxxstat;
  fp->minystat = local->minystat;
  fp->maxystat = local->maxystat;
#endif
  fp->minx = local->minx;
  fp->maxx = local->maxx;
  fp->miny = local->miny;
  fp->maxy = local->maxy;
  fp->sumx = local->sumx;
  fp->sumy = local->sumy;
  fp->sumxx = local->sumxx;
  fp->sumyy = local->sumyy;
  fp->sumxy = local->sumxy;
  fp->num = local->num;

  if (local->rcode == -1)
    return -1;

  set_final_line(fp, local);

  return 0;
}


static int 
getminmaxdata(struct f2ddata *fp, struct f2dlocal *local)
/*
  return -1: fatal error
          0: no error
*/
{
  int rcode;
#if NEW_MATH_CODE
  MathValue *gdata = NULL;
#else
  double *gdata = NULL;
  char *gstat = NULL;
#endif

  if (check_mtime(fp, local) == 0) {
    return 0;
  }

  if (hskipdata(fp)!=0) {
    closedata(fp, local);
    return 1;
  }


#if NEW_MATH_CODE
  gdata = memalloc(sizeof(MathValue) * (FILE_OBJ_MAXCOL + 1));
  if (gdata == NULL) {
   memfree(gdata);
   return -1;
  }
#else
  if (((gdata=(double *)memalloc(sizeof(double)*(FILE_OBJ_MAXCOL+1)))==NULL)
  || ((gstat=(char *)memalloc(sizeof(char)*(FILE_OBJ_MAXCOL+1)))==NULL)) {
   memfree(gdata);
   memfree(gstat);
   return -1;
  }
#endif

#if NEW_MATH_CODE
  fp->minx.type = MUNDEF;
  fp->maxx.type = MUNDEF;
  fp->miny.type = MUNDEF;
  fp->maxy.type = MUNDEF;
#else
  fp->minxstat=MUNDEF;
  fp->maxxstat=MUNDEF;
  fp->minystat=MUNDEF;
  fp->maxystat=MUNDEF;
#endif
  fp->sumx=0;
  fp->sumy=0;
  fp->sumxx=0;
  fp->sumyy=0;
  fp->sumxy=0;
  fp->num=0;
  while (
#if NEW_MATH_CODE
	 (rcode = getdataraw(fp, fp->maxdim, gdata)) == 0
#else
	 (rcode=getdataraw(fp,fp->maxdim,gdata,gstat))==0
#endif
	 ) {
    switch (fp->type) {
    case TYPE_NORMAL:
    case TYPE_DIAGONAL:
#if NEW_MATH_CODE
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)) {
	if ((fp->minx.type == MATH_VALUE_UNDEF) || (fp->minx.val > fp->dx)) {
	  fp->minx.val = fp->dx;
	}
	if ((fp->maxx.type == MATH_VALUE_UNDEF) || (fp->maxx.val < fp->dx)) {
	  fp->maxx.val = fp->dx;
	}
	fp->minx.type = MATH_VALUE_NORMAL;
	fp->maxx.type = MATH_VALUE_NORMAL;

	if ((fp->miny.type == MATH_VALUE_UNDEF) || (fp->miny.val > fp->dy)) {
	  fp->miny.val = fp->dy;
	}
	if ((fp->maxy.type == MATH_VALUE_UNDEF) || (fp->maxy.val < fp->dy)) {
	  fp->maxy.val = fp->dy;
	}
	fp->miny.type = MATH_VALUE_NORMAL;
	fp->maxy.type = MATH_VALUE_NORMAL;

	fp->sumx += fp->dx;
	fp->sumxx +=  (fp->dx) * (fp->dx);

	fp->sumy += fp->dy;
	fp->sumyy += (fp->dy) * (fp->dy);
	fp->sumxy += (fp->dx) * (fp->dy);

	fp->num++;
      }
#else
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)) {
	if ((fp->minxstat==MUNDEF) || (fp->minx>fp->dx)) fp->minx=fp->dx;
	if ((fp->maxxstat==MUNDEF) || (fp->maxx<fp->dx)) fp->maxx=fp->dx;
	fp->minxstat=MNOERR;
	fp->maxxstat=MNOERR;
	if ((fp->minystat==MUNDEF) || (fp->miny>fp->dy)) fp->miny=fp->dy;
	if ((fp->maxystat==MUNDEF) || (fp->maxy<fp->dy)) fp->maxy=fp->dy;
	fp->minystat=MNOERR;
	fp->maxystat=MNOERR;
	fp->sumx+=fp->dx;
	fp->sumxx+= (fp->dx)*(fp->dx);
	fp->sumy+=fp->dy;
	fp->sumyy+=(fp->dy)*(fp->dy);
	fp->sumxy+=(fp->dx)*(fp->dy);
	fp->num++;
      }
#endif

      if (fp->type == TYPE_NORMAL)
	break;
#if NEW_MATH_CODE
      if ((fp->d2stat == MATH_VALUE_NORMAL) && (fp->d3stat == MATH_VALUE_NORMAL)) {
	if ((fp->minx.type == MATH_VALUE_UNDEF) || (fp->minx.val > fp->d2)) {
	  fp->minx.val = fp->d2;
	}
	if ((fp->maxx.type == MATH_VALUE_UNDEF) || (fp->maxx.val < fp->d2)) {
	  fp->maxx.val = fp->d2;
	}
	fp->minx.type = MATH_VALUE_NORMAL;
	fp->maxx.type = MATH_VALUE_NORMAL;

	if ((fp->miny.type == MATH_VALUE_UNDEF) || (fp->miny.val > fp->d3)) {
	  fp->miny.val = fp->d3;
	}
	if ((fp->maxy.type == MATH_VALUE_UNDEF) || (fp->maxy.val < fp->d3)) {
	  fp->maxx.val = fp->d3;
	}
	fp->miny.type = MATH_VALUE_NORMAL;
	fp->maxy.type = MATH_VALUE_NORMAL;

	fp->sumx += fp->d2;
	fp->sumxx += (fp->d2) * (fp->d2);

	fp->sumy += fp->d3;
	fp->sumyy += (fp->d3) * (fp->d3);
	fp->sumxy += (fp->d2) * (fp->d3);

	fp->num++;
      }
#else
      if ((fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)) {
	if ((fp->minxstat==MUNDEF) || (fp->minx>fp->d2)) fp->minx=fp->d2;
	if ((fp->maxxstat==MUNDEF) || (fp->maxx<fp->d2)) fp->maxx=fp->d2;
	fp->minxstat=MNOERR;
	fp->maxxstat=MNOERR;
	if ((fp->minystat==MUNDEF) || (fp->miny>fp->d3)) fp->miny=fp->d3;
	if ((fp->maxystat==MUNDEF) || (fp->maxy<fp->d3)) fp->maxx=fp->d3;
	fp->minystat=MNOERR;
	fp->maxystat=MNOERR;
	fp->sumx+=fp->d2;
	fp->sumxx+=(fp->d2)*(fp->d2);
	fp->sumy+=fp->d3;
	fp->sumyy+=(fp->d3)*(fp->d3);
	fp->sumxy+=(fp->d2)*(fp->d3);
	fp->num++;
      }
#endif
      break;
    case TYPE_ERR_X:
#if NEW_MATH_CODE
      if ((fp->d2stat == MATH_VALUE_NORMAL) &&
	  (fp->d3stat == MATH_VALUE_NORMAL) &&
	  (fp->dystat == MATH_VALUE_NORMAL)) {
	if ((fp->minx.type == MATH_VALUE_UNDEF) || (fp->minx.val > fp->d2)) {
	  fp->minx.val = fp->d2;
	}
	if ((fp->maxx.type == MATH_VALUE_UNDEF) || (fp->maxx.val < fp->d2)) {
	  fp->maxx.val = fp->d2;
	}
	fp->minx.type = MATH_VALUE_NORMAL;
	fp->maxx.type = MATH_VALUE_NORMAL;

	if ((fp->minx.type == MATH_VALUE_UNDEF) || (fp->minx.val > fp->d3)) {
	  fp->minx.val = fp->d3;
	}
	if ((fp->maxx.type == MATH_VALUE_UNDEF) || (fp->maxx.val < fp->d3)) {
	  fp->maxx.val = fp->d3;
	}
	fp->minx.type=MATH_VALUE_NORMAL;
	fp->maxx.type=MATH_VALUE_NORMAL;

	if ((fp->miny.type == MATH_VALUE_UNDEF) || (fp->miny.val > fp->dy)) {
	  fp->miny.val = fp->dy;
	}
	if ((fp->maxy.type == MATH_VALUE_UNDEF) || (fp->maxy.val < fp->dy)) {
	  fp->maxy.val = fp->dy;
	}
	fp->miny.type = MATH_VALUE_NORMAL;
	fp->maxy.type = MATH_VALUE_NORMAL;

	fp->sumx += fp->d2;
	fp->sumxx += (fp->d2) * (fp->d2);
	fp->sumy += fp->dy;
	fp->sumyy += (fp->dy) * (fp->dy);
	fp->sumxy += (fp->d2) * (fp->dy);
	fp->num++;

	fp->sumx += fp->d3;
	fp->sumxx += (fp->d3) * (fp->d3);
	fp->sumy += fp->dy;
	fp->sumyy += (fp->dy) * (fp->dy);
	fp->sumxy += (fp->d3) * (fp->dy);
	fp->num++;
      }
#else
      if ((fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)
	  && (fp->dystat==MNOERR)) {
	if ((fp->minxstat==MUNDEF) || (fp->minx>fp->d2)) fp->minx=fp->d2;
	if ((fp->maxxstat==MUNDEF) || (fp->maxx<fp->d2)) fp->maxx=fp->d2;
	fp->minxstat=MNOERR;
	fp->maxxstat=MNOERR;
	if ((fp->minxstat==MUNDEF) || (fp->minx>fp->d3)) fp->minx=fp->d3;
	if ((fp->maxxstat==MUNDEF) || (fp->maxx<fp->d3)) fp->maxx=fp->d3;
	fp->minxstat=MNOERR;
	fp->maxxstat=MNOERR;
	if ((fp->minystat==MUNDEF) || (fp->miny>fp->dy)) fp->miny=fp->dy;
	if ((fp->maxystat==MUNDEF) || (fp->maxy<fp->dy)) fp->maxy=fp->dy;
	fp->minystat=MNOERR;
	fp->maxystat=MNOERR;
	fp->sumx+=fp->d2;
	fp->sumxx+=(fp->d2)*(fp->d2);
	fp->sumy+=fp->dy;
	fp->sumyy+=(fp->dy)*(fp->dy);
	fp->sumxy+=(fp->d2)*(fp->dy);
	fp->num++;
	fp->sumx+=fp->d3;
	fp->sumxx+=(fp->d3)*(fp->d3);
	fp->sumy+=fp->dy;
	fp->sumyy+=(fp->dy)*(fp->dy);
	fp->sumxy+=(fp->d3)*(fp->dy);
	fp->num++;
      }
#endif
      break;
    case TYPE_ERR_Y:
#if NEW_MATH_CODE
      if ((fp->d2stat == MATH_VALUE_NORMAL) &&
	  (fp->d3stat == MATH_VALUE_NORMAL) &&
	  (fp->dxstat == MATH_VALUE_NORMAL)) {
	if ((fp->miny.type == MATH_VALUE_UNDEF) || (fp->miny.val > fp->d2)) {
	  fp->miny.val = fp->d2;
	}
	if ((fp->maxy.type == MATH_VALUE_UNDEF) || (fp->maxy.val < fp->d2)) {
	  fp->maxy.val = fp->d2;
	}
	fp->miny.type = MATH_VALUE_NORMAL;
	fp->maxy.type = MATH_VALUE_NORMAL;

	if ((fp->miny.type == MATH_VALUE_UNDEF) || (fp->miny.val > fp->d3)) {
	  fp->miny.val = fp->d3;
	}
	if ((fp->maxy.type == MATH_VALUE_UNDEF) || (fp->maxy.val < fp->d3)) {
	  fp->maxx.val = fp->d3;
	}
	fp->miny.type = MATH_VALUE_NORMAL;
	fp->maxy.type = MATH_VALUE_NORMAL;

	if ((fp->minx.type == MATH_VALUE_UNDEF) || (fp->minx.val > fp->dx)) {
	  fp->minx.val = fp->dx;
	}
	if ((fp->maxx.type == MATH_VALUE_UNDEF) || (fp->maxx.val < fp->dx)) {
	  fp->maxx.val = fp->dx;
	}
	fp->minx.type = MATH_VALUE_NORMAL;
	fp->maxx.type = MATH_VALUE_NORMAL;

	fp->sumx += fp->dx;
	fp->sumxx += (fp->dx) * (fp->dx);
	fp->sumy += fp->d2;
	fp->sumyy += (fp->d2) * (fp->d2);
	fp->sumxy += (fp->dx) * (fp->d2);
	fp->num++;

	fp->sumx += fp->dx;
	fp->sumxx += (fp->dx) * (fp->dx);
	fp->sumy += fp->d3;
	fp->sumyy += (fp->d3) * (fp->d3);
	fp->sumxy += (fp->dx) * (fp->d3);
	fp->num++;
      }
#else
      if ((fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)
	  && (fp->dxstat==MNOERR)) {
	if ((fp->minystat==MUNDEF) || (fp->miny>fp->d2)) fp->miny=fp->d2;
	if ((fp->maxystat==MUNDEF) || (fp->maxy<fp->d2)) fp->maxy=fp->d2;
	fp->minystat=MNOERR;
	fp->maxystat=MNOERR;
	if ((fp->minystat==MUNDEF) || (fp->miny>fp->d3)) fp->miny=fp->d3;
	if ((fp->maxystat==MUNDEF) || (fp->maxy<fp->d3)) fp->maxx=fp->d3;
	fp->minystat=MNOERR;
	fp->maxystat=MNOERR;
	if ((fp->minxstat==MUNDEF) || (fp->minx>fp->dx)) fp->minx=fp->dx;
	if ((fp->maxxstat==MUNDEF) || (fp->maxx<fp->dx)) fp->maxx=fp->dx;
	fp->minxstat=MNOERR;
	fp->maxxstat=MNOERR;
	fp->sumx+=fp->dx;
	fp->sumxx+=(fp->dx)*(fp->dx);
	fp->sumy+=fp->d2;
	fp->sumyy+=(fp->d2)*(fp->d2);
	fp->sumxy+=(fp->dx)*(fp->d2);
	fp->num++;
	fp->sumx+=fp->dx;
	fp->sumxx+=(fp->dx)*(fp->dx);
	fp->sumy+=fp->d3;
	fp->sumyy+=(fp->d3)*(fp->d3);
	fp->sumxy+=(fp->dx)*(fp->d3);
	fp->num++;
      }
#endif
      break;
    }
  }
  fp->dx=fp->dy=fp->d2=fp->d3=0;
  fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MUNDEF;
  fp->dline=0;
  memfree(gdata);
#if ! NEW_MATH_CODE
  memfree(gstat);
  local->minxstat = fp->minxstat;
  local->maxxstat = fp->maxxstat;
  local->minystat = fp->minystat;
  local->maxystat = fp->maxystat;
#endif
  local->minx = fp->minx;
  local->maxx = fp->maxx;
  local->miny = fp->miny;
  local->maxy = fp->maxy;
  local->sumx = fp->sumx;
  local->sumy = fp->sumy;
  local->sumxx = fp->sumxx;
  local->sumyy = fp->sumyy;
  local->sumxy = fp->sumxy;
  local->num = fp->num;
  local->rcode = rcode;
  local->total_line = fp->line;

  if (rcode==-1)
    return -1;

  if (fp->interrupt == FALSE)
    local->mtime = fp->mtime;

  set_final_line(fp, local);

  return 0;
}

static int 
getposition(struct f2ddata *fp,double x,double y,int *gx,int *gy)
/*
  return -1: unable to transform
          0: normal
          1: outside region
*/
{
  double minx,maxx,miny,maxy;
  double v1x,v1y,v2x,v2y,vx,vy;
  double a,b,c,d;

  *gx=*gy=0;
  minx=fp->axmin;
  maxx=fp->axmax;
  miny=fp->aymin;
  maxy=fp->aymax;
  if (fp->axtype==1) {
    if (x==0) {
      fp->ignore=TRUE;
      return -1;
    } else if (x<0) {
      fp->negative=TRUE;
      x=fabs(x);
    }
    x=log10(x);
  } else if (fp->axtype==2) {
    if (x==0) {
      fp->ignore=TRUE;
      return -1;
    }
    x=1/x;
  }
  if (fp->aytype==1) {
    if (y==0) {
      fp->ignore=TRUE;
      return -1;
    } else if (y<0) {
      fp->negative=TRUE;
      y=fabs(y);
    }
    y=log10(y);
  } else if (fp->aytype==2) {
    if (y==0) {
      fp->ignore=TRUE;
      return -1;
    }
    y=1/y;
  }
  if (fp->dataclip &&
  ((((minx>x) || (x>maxx)) && ((maxx>x) || (x>minx)))
   || (((miny>y) || (y>maxy)) && ((maxy>y) || (y>miny))))) return 1;
  v1x=fp->ratex*(x-minx)*fp->axvx;
  v1y=fp->ratex*(x-minx)*fp->axvy;
  v2x=fp->ratey*(y-miny)*fp->ayvx;
  v2y=fp->ratey*(y-miny)*fp->ayvy;
  vx=fp->ayposx-fp->axposx+v2x-v1x;
  vy=fp->ayposy-fp->axposy+v2y-v1y;
  a=fp->ayvy*fp->axvx-fp->ayvx*fp->axvy;
  c=-fp->ayvy*vx+fp->ayvx*vy;
  b=fp->axvy*fp->ayvx-fp->axvx*fp->ayvy;
  d=fp->axvy*vx-fp->axvx*vy;
  if ((fabs(a)<=1e-16) && (fabs(b)<=1e-16)) {
    fp->ignore=TRUE;
    return -1;
  } else if (fabs(b)<=1e-16) {
    a=c/a;
    *gx=fp->ayposx+nround(v2x+a*fp->axvx);
    *gy=fp->ayposy+nround(v2y+a*fp->axvy);
  } else {
    b=d/b;
    *gx=fp->axposx+nround(v1x+b*fp->ayvx);
    *gy=fp->axposy+nround(v1y+b*fp->ayvy);
  }
  return 0;
}

static int 
getposition2(struct f2ddata *fp,int axtype,int aytype,double *x,double *y)
/*
  return -1: unable to transform
          0: normal
*/
{
  if (axtype==1) {
    if (*x==0) {
      fp->ignore=TRUE;
      return -1;
    } else if (*x<0) {
      fp->negative=TRUE;
      *x=fabs(*x);
    }
    *x=log10(*x);
  } else if (axtype==2) {
    if (*x==0) {
      fp->ignore=TRUE;
      return -1;
    }
    *x=1 / *x;
  }
  if (aytype==1) {
    if (*y==0) {
      fp->ignore=TRUE;
      return -1;
    } else if (*y<0) {
      fp->negative=TRUE;
      *y=fabs(*y);
    }
    *y=log10(*y);
  } else if (aytype==2) {
    if (*y==0) {
      fp->ignore=TRUE;
      return -1;
    }
    *y=1 / *y;
  }
  return 0;
}

static void 
f2dtransf(double x,double y,int *gx,int *gy,void *local)
{
  struct f2ddata *fp;
  double minx,miny;
  double v1x,v1y,v2x,v2y,vx,vy;
  double a,b,c,d;

  fp=local;
  minx=fp->axmin;
  miny=fp->aymin;
  v1x=fp->ratex*(x-minx)*fp->axvx;
  v1y=fp->ratex*(x-minx)*fp->axvy;
  v2x=fp->ratey*(y-miny)*fp->ayvx;
  v2y=fp->ratey*(y-miny)*fp->ayvy;
  vx=fp->ayposx-fp->axposx+v2x-v1x;
  vy=fp->ayposy-fp->axposy+v2y-v1y;
  a=fp->ayvy*fp->axvx-fp->ayvx*fp->axvy;
  c=-fp->ayvy*vx+fp->ayvx*vy;
  b=fp->axvy*fp->ayvx-fp->axvx*fp->ayvy;
  d=fp->axvy*vx-fp->axvx*vy;
  if ((fabs(a)<=1e-16) && (fabs(b)<=1e-16)) {
    return;
  } else if (fabs(b)<=1e-16) {
    a=c/a;
    *gx=fp->ayposx+nround(v2x+a*fp->axvx);
    *gy=fp->ayposy+nround(v2y+a*fp->axvy);
  } else {
    b=d/b;
    *gx=fp->axposx+nround(v1x+b*fp->ayvx);
    *gy=fp->axposy+nround(v1y+b*fp->ayvy);
  }
}

static int 
f2dlineclipf(double *x0,double *y0,double *x1,double *y1,void *local)
{
  double a,xl,yl,xg,yg;
  double minx,miny,maxx,maxy;
  struct f2ddata *fp;

  fp=local;
  if (!fp->dataclip) return 0;
  if (fp->axmin>fp->axmax) {
    minx=fp->axmax;
    maxx=fp->axmin;
  } else {
    minx=fp->axmin;
    maxx=fp->axmax;
  }
  if (fp->aymax>fp->aymin) {
    miny=fp->aymin;
    maxy=fp->aymax;
  } else {
    miny=fp->aymax;
    maxy=fp->aymin;
  }
  if (*x0<*x1) {
    xl=*x0;  yl=*y0; xg=*x1;  yg=*y1;
  } else {
    xl=*x1;  yl=*y1; xg=*x0;  yg=*y0;
  }
  if ((xg<minx) || (xl>maxx)) return 1;
  if (xg>maxx) {
    xg=maxx; yg=(*y1-*y0)*(maxx-*x0)/(*x1-*x0)+*y0;
  }
  if (xl<minx) {
    xl=minx; yl=(*y1-*y0)*(minx-*x0)/(*x1-*x0)+*y0;
  }
  if (yl>yg) {
    a=yl;  yl=yg;  yg=a;  a=xl;  xl=xg;  xg=a;
  }
  if ((yg<miny) || (yl>maxy)) return 1;
  if (yg>maxy) {
    yg=maxy; xg=(*x1-*x0)*(maxy-*y0)/(*y1-*y0)+*x0;
  }
  if (yl<miny) {
    yl=miny; xl=(*x1-*x0)*(miny-*y0)/(*y1-*y0)+*x0;
  }
  if ((*y0<*y1) || ((*y0==*y1) && (*x0<*x1))) {
    *x0=xl; *y0=yl;   *x1=xg; *y1=yg;
  } else {
    *x0=xg; *y0=yg;   *x1=xl; *y1=yl;
  }
  return 0;
}

static int 
f2drectclipf(double *x0,double *y0,double *x1,double *y1,void *local)
{
  double xl,yl,xg,yg;
  double minx,miny,maxx,maxy;
  struct f2ddata *fp;

  fp=local;
  if (!fp->dataclip) return 0;
  if (fp->axmin>fp->axmax) {
    minx=fp->axmax;
    maxx=fp->axmin;
  } else {
    minx=fp->axmin;
    maxx=fp->axmax;
  }
  if (fp->aymax>fp->aymin) {
    miny=fp->aymin;
    maxy=fp->aymax;
  } else {
    miny=fp->aymax;
    maxy=fp->aymin;
  }
  if (*x0<*x1) {
    xl=*x0; xg=*x1;
  } else {
    xl=*x1; xg=*x0;

  }
  if (*y0<*y1) {
    yl=*y0; yg=*y1;
  } else {
    yl=*y1; yg=*y0;
  }
  if ((xg<minx) || (xl>maxx)) return 1;
  if ((yg<miny) || (yl>maxy)) return 1;
  if ((xg>maxx) && (xl<minx) && (yg>maxy) && (yl<miny)) return 1;
  if (xg>maxx) xg=maxx;
  if (xl<minx) xl=minx;
  if (yg>maxy) yg=maxy;
  if (yl<miny) yl=miny;
  *x0=xl;  *y0=yl;
  *x1=xg;  *y1=yg;
  return 0;
}

static void 
f2dsplinedif(double d,double c[],
	     double *dx,double *dy,double *ddx,double *ddy,void *local)
{
  struct f2ddata *fp;

  fp=local;
  splinedif(d,c,dx,dy,ddx,ddy,NULL);
  (*dx)*=fp->ratex;
  (*dy)*=fp->ratey;
  (*ddx)*=fp->ratex;
  (*ddy)*=fp->ratey;
}

static void 
f2dbsplinedif(double d,double c[],
	      double *dx,double *dy,double *ddx,double *ddy,void *local)
{
  struct f2ddata *fp;

  fp=local;
  bsplinedif(d,c,dx,dy,ddx,ddy,NULL);
  (*dx)*=fp->ratex;
  (*dy)*=fp->ratey;
  (*ddx)*=fp->ratex;
  (*ddy)*=fp->ratey;
}

static void 
f2derror(struct objlist *obj,struct f2ddata *fp,int code,char *s)
{
  char buf[256];

  sprintf(buf,"#%d: %s (%d:%s)",fp->id,fp->file,fp->dline,s);
  error2(obj,code,buf);
}

static void 
errordisp(struct objlist *obj,
	  struct f2ddata *fp,
	  int *emerr,int *emserr,int *emnonum,int *emig,int *emng)
{
  int x,y;
  char *s;

  if (!*emerr) {
    x=FALSE;
    y=FALSE;
    if ((fp->dxstat==MERR) || (fp->dxstat==MNAN)) x=TRUE;
    if ((fp->dystat==MERR) || (fp->dystat==MNAN)) y=TRUE;

    switch (fp->type) {
    case TYPE_NORMAL:
      break;
    case TYPE_DIAGONAL:
      if ((fp->d2stat==MERR) || (fp->d2stat==MNAN)) x=TRUE;
      if ((fp->d3stat==MERR) || (fp->d3stat==MNAN)) y=TRUE;
      break;
    case TYPE_ERR_X:
      if ((fp->d2stat==MERR) || (fp->d2stat==MNAN)) x=TRUE;
      if ((fp->d3stat==MERR) || (fp->d3stat==MNAN)) x=TRUE;
      break;
    case TYPE_ERR_Y:
      if ((fp->d2stat==MERR) || (fp->d2stat==MNAN)) y=TRUE;
      if ((fp->d3stat==MERR) || (fp->d3stat==MNAN)) y=TRUE;
      break;
    }
    if (x || y) {
      if (x && (!y)) s="x";
      else if ((!x) && y) s="y";
      else s="xy";
      f2derror(obj,fp,ERRMERR,s);
      *emerr=TRUE;
    }
  }
  if (!*emserr) {
    x=FALSE;
    y=FALSE;
    if (fp->dxstat==MSERR) x=TRUE;
    if (fp->dystat==MSERR) y=TRUE;
    switch (fp->type) {
    case TYPE_NORMAL:
      break;
    case TYPE_DIAGONAL:
      if (fp->d2stat==MSERR) x=TRUE;
      if (fp->d3stat==MSERR) y=TRUE;
      break;
    case TYPE_ERR_X:
      if (fp->d2stat==MSERR) x=TRUE;
      if (fp->d3stat==MSERR) x=TRUE;
      break;
    case TYPE_ERR_Y:
      if (fp->d2stat==MSERR) y=TRUE;
      if (fp->d3stat==MSERR) y=TRUE;
      break;
    }
    if (x || y) {
      if (x && (!y)) s="x";
      else if ((!x) && y) s="y";
      else s="xy";
      f2derror(obj,fp,ERRMSYNTAX,s);
      *emserr=TRUE;
    }
  }
  if (!*emnonum) {
    x=FALSE;
    y=FALSE;
    if (fp->dxstat==MNONUM) x=TRUE;
    if (fp->dystat==MNONUM) y=TRUE;
    switch (fp->type) {
    case TYPE_NORMAL:
      break;
    case TYPE_DIAGONAL:
      if (fp->d2stat==MNONUM) x=TRUE;
      if (fp->d3stat==MNONUM) y=TRUE;
      break;
    case TYPE_ERR_X:
      if (fp->d2stat==MNONUM) x=TRUE;
      if (fp->d3stat==MNONUM) x=TRUE;
      break;
    case TYPE_ERR_Y:
      if (fp->d2stat==MNONUM) y=TRUE;
      if (fp->d3stat==MNONUM) y=TRUE;
      break;
    }
    if (x || y) {
      if (x && (!y)) s="x";
      else if ((!x) && y) s="y";
      else s="xy";
      f2derror(obj,fp,ERRMNONUM,s);
      *emnonum=TRUE;
    }
  }

  if (!*emig && fp->ignore) {
    error(obj,ERRIGNORE);
    *emig=TRUE;
  }
  if (!*emng && fp->negative) {
    error(obj,ERRNEGATIVE);
    *emng=TRUE;
  }
}

static void 
errordisp2(struct objlist *obj,
	   struct f2ddata *fp,
	   int *emerr,int *emserr,int *emnonum,int *emig,int *emng,
	   char ddstat,char *s)
{
  if (!*emerr && (ddstat==MERR)) {
    f2derror(obj,fp,ERRMERR,s);
    *emerr=TRUE;
  }
  if (!*emerr && (ddstat==MNAN)) {
    f2derror(obj,fp,ERRMERR,s);
    *emerr=TRUE;
  }
  if (!*emserr && (ddstat==MSERR)) {
    f2derror(obj,fp,ERRMSYNTAX,s);
    *emserr=TRUE;
  }
  if (!*emnonum && (ddstat==MNONUM)) {
    f2derror(obj,fp,ERRMNONUM,s);
    *emnonum=TRUE;
  }
  if (!*emig && fp->ignore) {
    error(obj,ERRIGNORE);
    *emig=TRUE;
  }
  if (!*emng && fp->negative) {
    error(obj,ERRNEGATIVE);
    *emng=TRUE;
  }
}

#define SPBUFFERSZ 1024

static double *
dataadd(double dx,double dy,double dz,
	int fr,int fg,int fb,int *size,
	double **x,double **y,double **z,
	int **r,int **g,int **b,
	double **c1,double **c2,double **c3,
	double **c4,double **c5,double **c6)
{
  double *xb,*yb,*zb,*c1b,*c2b,*c3b,*c4b,*c5b,*c6b;
  int *rb,*gb,*bb;
  int bz;

  if (*size==0) {
    if (((*x=memalloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*y=memalloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*z=memalloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*r=memalloc(sizeof(int)*SPBUFFERSZ))==NULL)
     || ((*g=memalloc(sizeof(int)*SPBUFFERSZ))==NULL)
     || ((*b=memalloc(sizeof(int)*SPBUFFERSZ))==NULL)
     || ((*c1=memalloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*c2=memalloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*c3=memalloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*c4=memalloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*c5=memalloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*c6=memalloc(sizeof(double)*SPBUFFERSZ))==NULL)) {
      memfree(*x);  memfree(*y);  memfree(*z);
      memfree(*r);  memfree(*g);  memfree(*b);
      memfree(*c1); memfree(*c2); memfree(*c3);
      memfree(*c4); memfree(*c5); memfree(*c6);
      return NULL;
    }
  } else if ((*size%SPBUFFERSZ)==0) {
    bz=*size/SPBUFFERSZ+1;
    if (((xb=memrealloc(*x,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((yb=memrealloc(*y,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((zb=memrealloc(*z,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((rb=memrealloc(*r,sizeof(int)*SPBUFFERSZ*bz))==NULL)
     || ((gb=memrealloc(*g,sizeof(int)*SPBUFFERSZ*bz))==NULL)
     || ((bb=memrealloc(*b,sizeof(int)*SPBUFFERSZ*bz))==NULL)
     || ((c1b=memrealloc(*c1,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((c2b=memrealloc(*c2,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((c3b=memrealloc(*c3,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((c4b=memrealloc(*c4,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((c5b=memrealloc(*c5,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((c6b=memrealloc(*c6,sizeof(double)*SPBUFFERSZ*bz))==NULL)) {
      memfree(*x);  memfree(*y);  memfree(*z);
      memfree(*r);  memfree(*g);  memfree(*b);
      memfree(*c1); memfree(*c2); memfree(*c3);
      memfree(*c4); memfree(*c5); memfree(*c6);
      return NULL;
    } else {
      *x=xb;   *y=yb;   *z=zb;
      *r=rb;   *g=gb;   *b=bb;
      *c1=c1b; *c2=c2b; *c3=c3b;
      *c4=c4b; *c5=c5b; *c6=c6b;
    }
  }
  (*x)[*size]=dx;
  (*y)[*size]=dy;
  (*z)[*size]=dz;
  (*r)[*size]=fr;
  (*g)[*size]=fg;
  (*b)[*size]=fb;
  (*size)++;
  return *x;
}

static int 
#if NEW_MATH_CODE
markout(struct objlist *obj,struct f2ddata *fp,int GC, int width,int snum,int *style)
#else
markout(struct objlist *obj,struct f2ddata *fp,int GC,
	int fr2,int fg2,int fb2, int width,int snum,int *style)
#endif
{
  int emerr,emserr,emnonum,emig,emng;
  int gx,gy;

  emerr=emserr=emnonum=emig=emng=FALSE;
  GRAlinestyle(GC,snum,style,width,0,0,1000);
  while (getdata(fp)==0) {
    if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR) &&
	(getposition(fp,fp->dx,fp->dy,&gx,&gy)==0)) {
      if (fp->msize>0)
#if NEW_MATH_CODE
        GRAmark(GC,fp->mtype, gx, gy, fp->msize,
		fp->colr, fp->colg, fp->colb,
		fp->colr2, fp->colg2, fp->colb2);
#else
        GRAmark(GC,fp->mtype,gx,gy,fp->msize,fp->colr,fp->colg,fp->colb,fr2,fg2,fb2);
#endif
    } else errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
  }
  errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
  return 0;
}

static int 
lineout(struct objlist *obj,struct f2ddata *fp,int GC,
	int width,int snum,int *style,
	int join,int miter,int close)
{
  int emerr,emserr,emnonum,emig,emng;
  int first;
  double x0,y0;

  emerr=emserr=emnonum=emig=emng=FALSE;
  GRAlinestyle(GC,0,NULL,width,0,join,miter);
  first=TRUE;
  while (getdata(fp)==0) {
    GRAcolor(GC,fp->colr,fp->colg,fp->colb);
    if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
    && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
      if (first) {
        GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,NULL,NULL,fp,
                      fp->dx,fp->dy);
        first=FALSE;
        x0=fp->dx;
        y0=fp->dy;
      } else  GRAdashlinetod(GC,fp->dx,fp->dy);
    } else {
      if ((fp->dxstat!=MSCONT) && (fp->dystat!=MSCONT)) {
        if (! first && close) GRAdashlinetod(GC,x0,y0);
        first=TRUE;
      }
      errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
    }
  }
  if (!first && close) GRAdashlinetod(GC,x0,y0);
  errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
  return 0;
}

static int 
curveout(struct objlist *obj,struct f2ddata *fp,int GC,
	 int width,int snum,int *style,
	 int join,int miter,int intp)
{
  int emerr,emserr,emnonum,emig,emng;
  int j,num;
  int first;
  double *x,*y,*z,*c1,*c2,*c3,*c4,*c5,*c6,count;
  int *r,*g,*b;
  double c[8];
  double bs1[7],bs2[7],bs3[4],bs4[4];
  int bsr[7],bsg[7],bsb[7],bsr2[4],bsg2[4],bsb2[4];
  int spcond;

  emerr=emserr=emnonum=emig=emng=FALSE;
  GRAlinestyle(GC,0,NULL,width,0,join,miter);
  switch (intp) {
  case INTERPOLATION_TYPE_SPLINE:
  case INTERPOLATION_TYPE_SPLINE_CLOSE:
    num=0;
    count=0;
    x=y=z=c1=c2=c3=c4=c5=c6=NULL;
    r=g=b=NULL;
    while (getdata(fp)==0) {
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
      && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
        if (dataadd(fp->dx,fp->dy,count,fp->colr,fp->colg,fp->colb,&num,
                   &x,&y,&z,&r,&g,&b,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) return -1;
        count++;
      } else {
        if ((fp->dxstat!=MSCONT) && (fp->dystat!=MSCONT)) {
          if (num>=2) {
            if (intp==0) spcond=SPLCND2NDDIF;
            else {
              spcond=SPLCNDPERIODIC;
              if ((x[num-1]!=x[0]) || (y[num-1]!=y[0])) {
                if (dataadd(x[0],y[0],count,r[0],g[0],b[0],&num,
                   &x,&y,&z,&r,&g,&b,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) return -1;
              }
            }
            if (spline(z,x,c1,c2,c3,num,spcond,spcond,0,0)
             || spline(z,y,c4,c5,c6,num,spcond,spcond,0,0)) {
              memfree(x);  memfree(y);  memfree(z);
              memfree(r);  memfree(g);  memfree(b);
              memfree(c1); memfree(c2); memfree(c3);
              memfree(c4); memfree(c5); memfree(c6);
              error(obj,ERRSPL);
              return -1;
            }
            GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,
                          f2dsplinedif,splineint,fp,x[0],y[0]);
            for (j=0;j<num-1;j++) {
              c[0]=c1[j]; c[1]=c2[j]; c[2]=c3[j];
              c[3]=c4[j]; c[4]=c5[j]; c[5]=c6[j];
              GRAcolor(GC,r[j],g[j],b[j]);
              if (!GRAcurve(GC,c,x[j],y[j])) break;
            }
          }
          memfree(x);  memfree(y);  memfree(z);
          memfree(r);  memfree(g);  memfree(b);
          memfree(c1); memfree(c2); memfree(c3);
          memfree(c4); memfree(c5); memfree(c6);
          num=0;
          count=0;
          x=y=z=c1=c2=c3=c4=c5=c6=NULL;
          r=g=b=NULL;
        }
        errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
      }
    }
    if (num!=0) {
      if (intp==0) {
	spcond=SPLCND2NDDIF;
      } else {
        spcond=SPLCNDPERIODIC;
        if ((x[num-1]!=x[0]) || (y[num-1]!=y[0])) {
          if (dataadd(x[0],y[0],count,r[0],g[0],b[0],&num,
                  &x,&y,&z,&r,&g,&b,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) return -1;
        }
      }
      if (spline(z,x,c1,c2,c3,num,spcond,spcond,0,0)
       || spline(z,y,c4,c5,c6,num,spcond,spcond,0,0)) {
        memfree(x);  memfree(y);  memfree(z);
        memfree(r);  memfree(g);  memfree(b);
        memfree(c1); memfree(c2); memfree(c3);
        memfree(c4); memfree(c5); memfree(c6);
        error(obj,ERRSPL);
        return -1;
      }
      GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,
                    f2dsplinedif,splineint,fp,x[0],y[0]);
      for (j=0;j<num-1;j++) {
        c[0]=c1[j]; c[1]=c2[j]; c[2]=c3[j];
        c[3]=c4[j]; c[4]=c5[j]; c[5]=c6[j];
        GRAcolor(GC,r[j],g[j],b[j]);
        if (!GRAcurve(GC,c,x[j],y[j])) break;
      }
    }
    memfree(x);  memfree(y);  memfree(z);
    memfree(r);  memfree(g);  memfree(b);
    memfree(c1); memfree(c2); memfree(c3);
    memfree(c4); memfree(c5); memfree(c6);
    break;
  case INTERPOLATION_TYPE_BSPLINE:
    first=TRUE;
    num=0;
    while (getdata(fp)==0) {
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
      && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
        if (first) {
          bs1[num]=fp->dx;
          bs2[num]=fp->dy;
          bsr[num]=fp->colr;
          bsg[num]=fp->colg;
          bsb[num]=fp->colb;
          num++;
          if (num>=7) {
            for (j=0;j<2;j++) {
              bspline(j+1,bs1+j,c);
              bspline(j+1,bs2+j,c+4);
              if (j==0) {
                GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,
                              f2dbsplinedif,bsplineint,fp,c[0],c[4]);
              }
              GRAcolor(GC,bsr[j],bsg[j],bsb[j]);
              if (!GRAcurve(GC,c,c[0],c[4])) return -1;
            }
            first=FALSE;
          }
        } else {
          for (j=1;j<7;j++) {
            bs1[j-1]=bs1[j];
            bs2[j-1]=bs2[j];
            bsr[j-1]=bsr[j];
            bsg[j-1]=bsg[j];
            bsb[j-1]=bsb[j];
          }
          bs1[6]=fp->dx;
          bs2[6]=fp->dy;
          bsr[6]=fp->colr;
          bsg[6]=fp->colg;
          bsb[6]=fp->colb;
          num++;
          bspline(0,bs1+1,c);
          bspline(0,bs2+1,c+4);
          GRAcolor(GC,bsr[1],bsg[1],bsb[1]);
          if (!GRAcurve(GC,c,c[0],c[4])) return -1;
        }
      } else {
        if ((fp->dxstat!=MSCONT) && (fp->dystat!=MSCONT)) {
          if (!first) {
            for (j=0;j<2;j++) {
              bspline(j+3,bs1+j+2,c);
              bspline(j+3,bs2+j+2,c+4);
              GRAcolor(GC,bsr[j+2],bsg[j+2],bsb[j+2]);
              if (!GRAcurve(GC,c,c[0],c[4])) return -1;
            }
          }
          first=TRUE;
          num=0;
        }
        errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
      }
    }
    if (!first) {
      for (j=0;j<2;j++) {
        bspline(j+3,bs1+j+2,c);
        bspline(j+3,bs2+j+2,c+4);
        GRAcolor(GC,bsr[j+2],bsg[j+2],bsb[j+2]);
        if (!GRAcurve(GC,c,c[0],c[4])) return -1;
      }
    }
    break;
  case INTERPOLATION_TYPE_BSPLINE_CLOSE:
    first=TRUE;
    num=0;
    while (getdata(fp)==0) {
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
      && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
        if (first) {
          bs1[num]=fp->dx;
          bs3[num]=fp->dx;
          bs2[num]=fp->dy;
          bs4[num]=fp->dy;
          bsr[num]=fp->colr;
          bsg[num]=fp->colg;
          bsb[num]=fp->colb;
          bsr2[num]=fp->colr;
          bsg2[num]=fp->colg;
          bsb2[num]=fp->colb;
          num++;
          if (num>=4) {
            bspline(0,bs1,c);
            bspline(0,bs2,c+4);
            GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,
                          f2dbsplinedif,bsplineint,fp,c[0],c[4]);
            GRAcolor(GC,bsr[0],bsg[0],bsb[0]);
            if (!GRAcurve(GC,c,c[0],c[4])) return -1;
            first=FALSE;
          }
        } else {
          for (j=1;j<4;j++) {
            bs1[j-1]=bs1[j];
            bs2[j-1]=bs2[j];
            bsr[j-1]=bsr[j];
            bsg[j-1]=bsg[j];
            bsb[j-1]=bsb[j];
          }
          bs1[3]=fp->dx;
          bs2[3]=fp->dy;
          bsr[3]=fp->colr;
          bsg[3]=fp->colg;
          bsb[3]=fp->colb;
          num++;
          bspline(0,bs1,c);
          bspline(0,bs2,c+4);
          GRAcolor(GC,bsr[0],bsg[0],bsb[0]);
          if (!GRAcurve(GC,c,c[0],c[4])) return -1;
        }
      } else {
        if ((fp->dxstat!=MSCONT) && (fp->dystat!=MSCONT)) {
          if (!first) {
            for (j=0;j<3;j++) {
              bs1[4+j]=bs3[j];
              bs2[4+j]=bs4[j];
              bsr[4+j]=bsr2[j];
              bsg[4+j]=bsg2[j];
              bsb[4+j]=bsb2[j];
              bspline(0,bs1+j+1,c);
              bspline(0,bs2+j+1,c+4);
              GRAcolor(GC,bsr[j+1],bsg[j+1],bsb[j+1]);
              if (!GRAcurve(GC,c,c[0],c[4])) return -1;
            }
          }
          first=TRUE;
          num=0;
        }
        errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
      }
    }
    if (!first) {
      for (j=0;j<3;j++) {
        bs1[4+j]=bs3[j];
        bs2[4+j]=bs4[j];
        bsr[4+j]=bsr2[j];
        bsg[4+j]=bsg2[j];
        bsb[4+j]=bsb2[j];
        bspline(0,bs1+j+1,c);
        bspline(0,bs2+j+1,c+4);
        GRAcolor(GC,bsr[j+1],bsg[j+1],bsb[j+1]);
        if (!GRAcurve(GC,c,c[0],c[4])) return -1;
      }
    }
    break;
  }
  errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
  return 0;
}

static int 
#if NEW_MATH_CODE
rectout(struct objlist *obj,struct f2ddata *fp,int GC,
	int width,int snum,int *style,int type)
#else
rectout(struct objlist *obj,struct f2ddata *fp,int GC,
	int fr2,int fg2,int fb2,
	int width,int snum,int *style,int type)
#endif
{
  int emerr,emserr,emnonum,emig,emng;
  double x0,y0,x1,y1;
  int gx0,gy0,gx1,gy1,ax0,ay0;
  double dx,dy,len,alen,awidth;
  int ap[6],headlen,headwidth;

  emerr=emserr=emnonum=FALSE;
  headlen=72426;
  headwidth=60000;

  if (type == PLOT_TYPE_DIAGONAL) GRAlinestyle(GC,snum,style,width,0,0,1000);
  else GRAlinestyle(GC,snum,style,width,2,0,1000);
  while (getdata(fp)==0) {
    GRAcolor(GC,fp->colr,fp->colg,fp->colb);
    if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
     && (fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)
     && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)
     && (getposition2(fp,fp->axtype,fp->aytype,&(fp->d2),&(fp->d3))==0)) {
      if (type == PLOT_TYPE_DIAGONAL) {
        x0=fp->dx;
        y0=fp->dy;
        x1=fp->d2;
        y1=fp->d3;
        if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)==0) {
          f2dtransf(x0,y0,&gx0,&gy0,fp);
          f2dtransf(x1,y1,&gx1,&gy1,fp);
          GRAline(GC,gx0,gy0,gx1,gy1);
        }
      }
      if (type == PLOT_TYPE_ARROW) {
        x0=fp->dx;
        y0=fp->dy;
        x1=fp->d2;
        y1=fp->d3;
        if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)==0) {
          f2dtransf(x0,y0,&gx0,&gy0,fp);
          f2dtransf(x1,y1,&gx1,&gy1,fp);
          if ((x1==fp->d2) && (y1==fp->d3) && (fp->msize>0)) {
            alen=fp->msize;
            awidth=fp->msize*(double )headwidth/(double )headlen/2;
            dx=gx1-gx0;
            dy=gy1-gy0;
            len=sqrt(dx*dx+dy*dy);
            if (len>0) {
              ax0=nround(gx1-dx*alen/len);
              ay0=nround(gy1-dy*alen/len);
              ap[0]=nround(ax0-dy/len*awidth);
              ap[1]=nround(ay0+dx/len*awidth);
              ap[2]=gx1;
              ap[3]=gy1;
              ap[4]=nround(ax0+dy/len*awidth);
              ap[5]=nround(ay0-dx/len*awidth);
              GRAline(GC,gx0,gy0,ax0,ay0);
              GRAdrawpoly(GC,3,ap,1);
            }
          } else GRAline(GC,gx0,gy0,gx1,gy1);
        }
      }
      if (type == PLOT_TYPE_RECTANGLE_FILL || type == PLOT_TYPE_RECTANGLE_SOLID_FILL) {
        if (type == PLOT_TYPE_RECTANGLE_FILL) {
#if NEW_MATH_CODE
	  GRAcolor(GC, fp->colr2, fp->colg2, fp->colb2);
#else
	  GRAcolor(GC, fr2, fg2, fb2);
#endif
	}
        x0=fp->dx;
        y0=fp->dy;
        x1=fp->d2;
        y1=fp->d3;
        if (f2drectclipf(&x0,&y0,&x1,&y1,fp)==0) {
          f2dtransf(x0,y0,&gx0,&gy0,fp);
          f2dtransf(x1,y1,&gx1,&gy1,fp);
          GRArectangle(GC,gx0,gy0,gx1,gy1,1);
        }
        if (type == PLOT_TYPE_RECTANGLE_FILL) GRAcolor(GC,fp->colr,fp->colg,fp->colb);
      }
      if (type == PLOT_TYPE_RECTANGLE || type == PLOT_TYPE_RECTANGLE_FILL) {
        x0=fp->dx;
        y0=fp->dy;
        x1=fp->dx;
        y1=fp->d3;
        if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)==0) {
          f2dtransf(x0,y0,&gx0,&gy0,fp);
          f2dtransf(x1,y1,&gx1,&gy1,fp);
          GRAline(GC,gx0,gy0,gx1,gy1);
        }
        x0=fp->dx;
        y0=fp->d3;
        x1=fp->d2;
        y1=fp->d3;
        if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)==0) {
          f2dtransf(x0,y0,&gx0,&gy0,fp);
          f2dtransf(x1,y1,&gx1,&gy1,fp);
          GRAline(GC,gx0,gy0,gx1,gy1);
        }
        x0=fp->d2;
        y0=fp->d3;
        x1=fp->d2;
        y1=fp->dy;
        if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)==0) {
          f2dtransf(x0,y0,&gx0,&gy0,fp);
          f2dtransf(x1,y1,&gx1,&gy1,fp);
          GRAline(GC,gx0,gy0,gx1,gy1);
        }
        x0=fp->d2;
        y0=fp->dy;
        x1=fp->dx;
        y1=fp->dy;
        if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)==0) {
          f2dtransf(x0,y0,&gx0,&gy0,fp);
          f2dtransf(x1,y1,&gx1,&gy1,fp);
          GRAline(GC,gx0,gy0,gx1,gy1);
        }
      }
    } else errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
  }
  errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
  return 0;
}

static int 
errorbarout(struct objlist *obj,struct f2ddata *fp,int GC,
	    int width,int snum,int *style,int type)
{
  int emerr,emserr,emnonum,emig,emng;
  double x0,y0,x1,y1;
  int gx0,gy0,gx1,gy1;
  int size;

  emerr=emserr=emnonum=emig=emng=FALSE;
  GRAlinestyle(GC,snum,style,width,0,0,1000);
  while (getdata(fp)==0) {
    size=fp->marksize0/2;
    GRAcolor(GC,fp->colr,fp->colg,fp->colb);
    if (type == PLOT_TYPE_ERRORBAR_X) {
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
       && (fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)
       && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)
       && (getposition2(fp,fp->axtype,fp->axtype,&(fp->d2),&(fp->d3))==0)) {
        x0=fp->d2;
        y0=fp->dy;
        x1=fp->d3;
        y1=fp->dy;
        if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)==0) {
          f2dtransf(x0,y0,&gx0,&gy0,fp);
          f2dtransf(x1,y1,&gx1,&gy1,fp);
          GRAline(GC,gx0,gy0,gx1,gy1);
          if (fp->d2==x0) {
            GRAline(GC,gx0+nround(size*fp->axvy),
                       gy0-nround(size*fp->axvx),
                       gx0-nround(size*fp->axvy),
                       gy0+nround(size*fp->axvx));
          }
          if (fp->d3==x1) {
            GRAline(GC,gx1+nround(size*fp->axvy),
                       gy1-nround(size*fp->axvx),
                       gx1-nround(size*fp->axvy),
                       gy1+nround(size*fp->axvx));
          }
        }
      } else errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
    } else if (type == PLOT_TYPE_ERRORBAR_Y) {
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
       && (fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)
       && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)
       && (getposition2(fp,fp->aytype,fp->aytype,&(fp->d2),&(fp->d3))==0)) {
        x0=fp->dx;
        y0=fp->d2;
        x1=fp->dx;
        y1=fp->d3;
        if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)==0) {
          f2dtransf(x0,y0,&gx0,&gy0,fp);
          f2dtransf(x1,y1,&gx1,&gy1,fp);
          GRAline(GC,gx0,gy0,gx1,gy1);
          if (fp->d2==y0) {
            GRAline(GC,gx0+nround(size*fp->ayvy),
                       gy0-nround(size*fp->ayvx),
                       gx0-nround(size*fp->ayvy),
                       gy0+nround(size*fp->ayvx));
          }
          if (fp->d3==y1) {
            GRAline(GC,gx1+nround(size*fp->ayvy),
                       gy1-nround(size*fp->ayvx),
                       gx1-nround(size*fp->ayvy),
                       gy1+nround(size*fp->ayvx));
          }
        }
      } else errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
    }
  }
  errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
  return 0;
}

static int 
stairout(struct objlist *obj,struct f2ddata *fp,int GC,
	 int width,int snum,int *style,
	 int join,int miter,int type)
{
  int emerr,emserr,emnonum,emig,emng;
  int num;
  double x0,y0,x1,y1,x,y,dx,dy;

  emerr=emserr=emnonum=emig=emng=FALSE;
  GRAlinestyle(GC,0,NULL,width,0,join,miter);
  num=0;
  while (getdata(fp)==0) {
    GRAcolor(GC,fp->colr,fp->colg,fp->colb);
    if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
     && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
      if (num==0) {
        x0=fp->dx;
        y0=fp->dy;
        num++;
      } else if (num==1) {
        x1=fp->dx;
        y1=fp->dy;
        if (type == PLOT_TYPE_STAIRCASE_X) {
          dx=(x1-x0)*0.5;
          y=y0;
          x=x0-dx;
          GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,NULL,NULL,fp,x,y);
          x=x0+dx;
          GRAdashlinetod(GC,x,y);
          y=y1;
          GRAdashlinetod(GC,x,y);
        } else {
          dy=(y1-y0)*0.5;
          x=x0;
          y=y0-dy;
          GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,NULL,NULL,fp,x,y);
          y=y0+dy;
          GRAdashlinetod(GC,x,y);
          x=x1;
          GRAdashlinetod(GC,x,y);
        }
        x0=x1;
        y0=y1;
        num++;
      } else {
        x1=fp->dx;
        y1=fp->dy;
        if (type == PLOT_TYPE_STAIRCASE_X) {
          dx=(x1-x0)*0.5;
          y=y0;
          x=x0+dx;
          GRAdashlinetod(GC,x,y);
          y=y1;
          GRAdashlinetod(GC,x,y);
        } else {
          dy=(y1-y0)*0.5;
          x=x0;
          y=y0+dy;
          GRAdashlinetod(GC,x,y);
          x=x1;
          GRAdashlinetod(GC,x,y);
        }
        x0=x1;
        y0=y1;
      }
    } else {
      if ((fp->dxstat!=MSCONT) && (fp->dystat!=MSCONT)) {
        if (num!=0) {
          if (type == PLOT_TYPE_STAIRCASE_X) {
            dx=x0-x;
            x=x0+dx;
            GRAdashlinetod(GC,x,y);
          } else {
            dy=y0-y;
            y=y0+dy;
            GRAdashlinetod(GC,x,y);
          }
        }
        num=0;
      }
      errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
    }
  }
  if (num!=0) {
    if (type == PLOT_TYPE_STAIRCASE_X) {
      dx=x0-x;
      x=x0+dx;
      GRAdashlinetod(GC,x,y);
    } else {
      dy=y0-y;
      y=y0+dy;
      GRAdashlinetod(GC,x,y);
    }
  }
  errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
  return 0;
}

static int 
#if NEW_MATH_CODE
barout(struct objlist *obj,struct f2ddata *fp,int GC,
       int width,int snum,int *style,int type)
#else
barout(struct objlist *obj,struct f2ddata *fp,int GC,
       int fr2,int fg2,int fb2,
       int width,int snum,int *style,int type)
#endif
{
  int emerr,emserr,emnonum,emig,emng;
  double x0,y0,x1,y1;
  int gx0,gy0,gx1,gy1;
  int size;
  int ap[8];

  emerr=emserr=emnonum=emig=emng=FALSE;
  if (type <= PLOT_TYPE_BAR_FILL_Y) GRAlinestyle(GC,snum,style,width,2,0,1000);
  while (getdata(fp)==0) {
    size=fp->marksize0/2;
    GRAcolor(GC,fp->colr,fp->colg,fp->colb);
    if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
     && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
      if ((type == PLOT_TYPE_BAR_FILL_X) || (type == PLOT_TYPE_BAR_SOLID_FILL_X)) {
        if (type == PLOT_TYPE_BAR_FILL_X) {
#if NEW_MATH_CODE
	  GRAcolor(GC, fp->colr2, fp->colg2, fp->colb2);
#else
	  GRAcolor(GC, fr2, fg2, fb2);
#endif
	}
        x0=0;
        y0=fp->dy;
        x1=fp->dx;
        y1=fp->dy;
        if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)==0) {
          f2dtransf(x0,y0,&gx0,&gy0,fp);
          f2dtransf(x1,y1,&gx1,&gy1,fp);
          ap[0]=gx0+nround(size*fp->ayvx);
          ap[1]=gy0+nround(size*fp->ayvy);
          ap[2]=gx1+nround(size*fp->ayvx);
          ap[3]=gy1+nround(size*fp->ayvy);
          ap[4]=gx1-nround(size*fp->ayvx);
          ap[5]=gy1-nround(size*fp->ayvy);
          ap[6]=gx0-nround(size*fp->ayvx);
          ap[7]=gy0-nround(size*fp->ayvy);
          GRAdrawpoly(GC,4,ap,1);
        }
        if (type == PLOT_TYPE_BAR_FILL_X) GRAcolor(GC,fp->colr,fp->colg,fp->colb);
      }
      if ((type == PLOT_TYPE_BAR_FILL_Y) || (type == PLOT_TYPE_BAR_SOLID_FILL_Y)) {
        if (type == PLOT_TYPE_BAR_FILL_Y) {
#if NEW_MATH_CODE
	  GRAcolor(GC, fp->colr2, fp->colg2, fp->colb2);
#else
	  GRAcolor(GC, fr2, fg2, fb2);
#endif
	}
        x0=fp->dx;
        y0=0;
        x1=fp->dx;
        y1=fp->dy;
        if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)==0) {
          f2dtransf(x0,y0,&gx0,&gy0,fp);
          f2dtransf(x1,y1,&gx1,&gy1,fp);
          ap[0]=gx0+nround(size*fp->axvx);
          ap[1]=gy0+nround(size*fp->axvy);
          ap[2]=gx1+nround(size*fp->axvx);
          ap[3]=gy1+nround(size*fp->axvy);
          ap[4]=gx1-nround(size*fp->axvx);
          ap[5]=gy1-nround(size*fp->axvy);
          ap[6]=gx0-nround(size*fp->axvx);
          ap[7]=gy0-nround(size*fp->axvy);
          GRAdrawpoly(GC,4,ap,1);
        }
        if (type == PLOT_TYPE_BAR_FILL_Y) GRAcolor(GC,fp->colr,fp->colg,fp->colb);
      }
      if ((type == PLOT_TYPE_BAR_X) || (type == PLOT_TYPE_BAR_FILL_X)) {
        x0=0;
        y0=fp->dy;
        x1=fp->dx;
        y1=fp->dy;
        if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)==0) {
          f2dtransf(x0,y0,&gx0,&gy0,fp);
          f2dtransf(x1,y1,&gx1,&gy1,fp);
          ap[0]=gx0+nround(size*fp->ayvx);
          ap[1]=gy0+nround(size*fp->ayvy);
          ap[2]=gx1+nround(size*fp->ayvx);
          ap[3]=gy1+nround(size*fp->ayvy);
          ap[4]=gx1-nround(size*fp->ayvx);
          ap[5]=gy1-nround(size*fp->ayvy);
          ap[6]=gx0-nround(size*fp->ayvx);
          ap[7]=gy0-nround(size*fp->ayvy);
          GRAdrawpoly(GC,4,ap,0);
        }
      }
      if ((type == PLOT_TYPE_BAR_Y) || (type == PLOT_TYPE_BAR_FILL_Y)) {
        x0=fp->dx;
        y0=0;
        x1=fp->dx;
        y1=fp->dy;
        if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)==0) {
          f2dtransf(x0,y0,&gx0,&gy0,fp);
          f2dtransf(x1,y1,&gx1,&gy1,fp);
          ap[0]=gx0+nround(size*fp->axvx);
          ap[1]=gy0+nround(size*fp->axvy);
          ap[2]=gx1+nround(size*fp->axvx);
          ap[3]=gy1+nround(size*fp->axvy);
          ap[4]=gx1-nround(size*fp->axvx);
          ap[5]=gy1-nround(size*fp->axvy);
          ap[6]=gx0-nround(size*fp->axvx);
          ap[7]=gy0-nround(size*fp->axvy);
          GRAdrawpoly(GC,4,ap,0);
        }
      }
    } else errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
  }
  errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
  return 0;
}

static int 
fitout(struct objlist *obj,struct f2ddata *fp,int GC,
       int width,int snum,int *style,
       int join,int miter,char *fit,int redraw)
{
  int emerr,emserr,emnonum,emig,emng;
  int j,num;
  double *x,*y,*z,*c1,*c2,*c3,*c4,*c5,*c6,count;
  int *r,*g,*b;
  double c[8];
  int spcond;
  struct objlist *fitobj;
  int anum;
  struct narray iarray;
  int id;
  char *inst;
  struct narray data;
  char *argv[2];
  char *equation;
  double min,max,dx,dy;
  int div;
#if NEW_MATH_CODE
  MathEquation *code;
  MathValue val;
#else
  char *code;
  int need2pass;
  struct narray *needdata;
#endif
  int i,rcode;
  double dnum;
  char *weight;
  int maxdim;
  double dd;
  char ddstat;
  int interpolation;
  int first;
  int datanum2;

  if (fit==NULL) {
    error(obj,ERRNOFIT);
    return -1;
  }
  arrayinit(&iarray,sizeof(int));
  if (getobjilist(fit,&fitobj,&iarray,FALSE,NULL)) return -1 ;
  anum=arraynum(&iarray);
  if (anum<1) {
    arraydel(&iarray);
    error2(obj,ERRNOFITINST,fit);
    return -1 ;
  }
  id=*(int *)arraylast(&iarray);
  arraydel(&iarray);
  if ((inst=getobjinst(fitobj,id))==NULL) return -1 ;
  if (_getobj(fitobj,"equation",inst,&equation)) return -1;
  if ((equation==NULL) && (!redraw)) {
    arrayinit(&data,sizeof(double));
    emerr=emserr=emnonum=emig=emng=FALSE;
    while (getdata(fp)==0) {
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)) {
        if ((arrayadd(&data,&(fp->dx))==NULL)
         || (arrayadd(&data,&(fp->dy))==NULL)) {
           arraydel(&data);
           return -1;
        }
      } else errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
    }
    errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
    if ((dnum=(double )arraynum(&data))==0) {
      arraydel(&data);
      return -1;
    }
    dnum/=2;
    if (arrayins(&data,&dnum,0)==NULL) {
      arraydel(&data);
      return -1;
    }
    if (_getobj(fitobj,"weight_func",inst,&weight)) {
      arraydel(&data);
      return -1;
    }
    if (weight!=NULL) {
#if NEW_MATH_CODE
      MathEquationParametar *prm;

      code = create_math_equation(NULL, FALSE, FALSE, FALSE, FALSE);
      if (code == NULL) {
	return 1;
      }
      prm = math_equation_get_parameter(code, 0);
      maxdim = (prm) ? prm->id_max : 0;
#else
      needdata=arraynew(sizeof(int));
      rcode=mathcode(weight,&code,needdata,NULL,&maxdim,&need2pass,
                   TRUE,TRUE,FALSE,TRUE,TRUE,TRUE,TRUE,FALSE,FALSE,FALSE,FALSE);
      if (mathcode_error(obj, rcode, MathErrorCodeArray)) {
	arraydel(&data);
        return 1;
      }
#endif
      if (maxdim<fp->x) maxdim=fp->x;
      if (maxdim<fp->y) maxdim=fp->y;
      datanum2=fp->datanum;
      reopendata(fp);
      if (hskipdata(fp)!=0) {
        arraydel(&data);
        return 1;
      }
      fp->datanum=datanum2;
      emerr=emserr=emnonum=emig=emng=FALSE;
      while (getdata2(fp,code,maxdim,&dd,&ddstat)==0) {
        if (ddstat==MNOERR) {
          if (arrayadd(&data,&dd)==NULL) {
             arraydel(&data);
             return -1;
          }
        } else errordisp2(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng,ddstat,"weight");
      }
      if (arraynum(&data)==0) {
        arraydel(&data);
        return -1;
      }
#if NEW_MATH_CODE
      math_equation_free(code);
#else
      memfree(code);
      arrayfree(needdata);
#endif
    }
    argv[0]=(void *)(&data);
    argv[1]=NULL;
    if (exeobj(fitobj,"fit",id,1,argv)) {
      arraydel(&data);
      return -1;
    }
    arraydel(&data);
  }
  if (_getobj(fitobj,"equation",inst,&equation)) return -1;
  if (_getobj(fitobj,"min",inst,&min)) return -1;
  if (_getobj(fitobj,"max",inst,&max)) return -1;
  if (_getobj(fitobj,"div",inst,&div)) return -1;
  if (_getobj(fitobj,"interpolation",inst,&interpolation)) return -1;
  if (equation==NULL) return -1;
  if ((min==0) && (max==0)) {
    min=fp->axmin2;
    max=fp->axmax2;
  } else if (min==max) return 0;
#if NEW_MATH_CODE
  code = create_math_equation(NULL, FALSE, FALSE, FALSE, FALSE);
  if (code == NULL) {
    math_equation_free(code);
    return 1;
  }

  rcode = math_equation_parse(code, equation);
  if (rcode) {
    math_equation_free(code);
    return 1;
  }

  rcode = math_equation_optimize(code);
  if (rcode) {
    return 1;
  }
#else
  rcode=mathcode(equation,&code,NULL,NULL,NULL,NULL,
                 TRUE,FALSE,FALSE,FALSE,FALSE,FALSE,
                 FALSE,FALSE,FALSE,FALSE,FALSE);
  if (mathcode_error(obj, rcode, MathErrorCodeArray)) {
    return 1;
  }
#endif
  GRAcolor(GC,fp->fr,fp->fg,fp->fb);
  GRAlinestyle(GC,0,NULL,width,0,join,miter);
  num=0;
  count=0;
  emerr=FALSE;
  x=y=z=c1=c2=c3=c4=c5=c6=NULL;
  r=g=b=NULL;
  first=TRUE;
  for (i=0;i<=div;i++) {
    dx=min+(max-min)/div*i;
#if NEW_MATH_CODE
    val.val = dx;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_var(code, 0, &val);
    rcode = math_equation_calculate(code, &val);
    dy = val.val;
#else
    rcode=calculate(code,1,dx,MNOERR,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                    NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                    NULL,NULL,NULL,0,NULL,NULL,NULL,0,&dy);
#endif
    if (interpolation) {
      if ((rcode==MNOERR)
      && (getposition2(fp,fp->axtype,fp->aytype,&dx,&dy)==0)) {
        if (dataadd(dx,dy,count,0,0,0,&num,
                    &x,&y,&z,&r,&g,&b,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) {
#if NEW_MATH_CODE
	  math_equation_free(code);
#else
          memfree(code);
#endif
          return -1;
        }
        count++;
      } else if (interpolation) { /* redundant condition ? */
        if (num>=2) {
          spcond=SPLCND2NDDIF;
          if (spline(z,x,c1,c2,c3,num,spcond,spcond,0,0)
           || spline(z,y,c4,c5,c6,num,spcond,spcond,0,0)) {
            memfree(x);  memfree(y);  memfree(z);
            memfree(r);  memfree(g);  memfree(b);
            memfree(c1); memfree(c2); memfree(c3);
            memfree(c4); memfree(c5); memfree(c6);
#if NEW_MATH_CODE
	    math_equation_free(code);
#else
            memfree(code);
#endif
            error(obj,ERRSPL);
            return -1;
          }
          GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,
                        f2dsplinedif,splineint,fp,x[0],y[0]);
          for (j=0;j<num-1;j++) {
            c[0]=c1[j]; c[1]=c2[j]; c[2]=c3[j];
            c[3]=c4[j]; c[4]=c5[j]; c[5]=c6[j];
            if (!GRAcurve(GC,c,x[j],y[j])) break;
          }
        }
        memfree(x);  memfree(y);  memfree(z);
        memfree(r);  memfree(g);  memfree(b);
        memfree(c1); memfree(c2); memfree(c3);
        memfree(c4); memfree(c5); memfree(c6);
        num=0;
        count=0;
        x=y=z=c1=c2=c3=c4=c5=c6=NULL;
        r=g=b=NULL;
      }
    } else {
      if ((rcode==MNOERR) && (getposition2(fp,fp->axtype,fp->aytype,&dx,&dy)==0)) {
        if (first) {
          GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,NULL,NULL,fp,dx,dy);
          first=FALSE;
        } else GRAdashlinetod(GC,dx,dy);
      } else first=TRUE;
    }
    if ((!emerr) && (rcode!=MNOERR) && (rcode!=MUNDEF)) {
      error(obj,ERRMERR);
      emerr=TRUE;
    }
  }
#if NEW_MATH_CODE
  math_equation_free(code);
#else
  memfree(code);
#endif
  if (interpolation) {
    if (num!=0) {
      spcond=SPLCND2NDDIF;
      if (spline(z,x,c1,c2,c3,num,spcond,spcond,0,0)
       || spline(z,y,c4,c5,c6,num,spcond,spcond,0,0)) {
        memfree(x);  memfree(y);  memfree(z);
        memfree(r);  memfree(g);  memfree(b);
        memfree(c1); memfree(c2); memfree(c3);
        memfree(c4); memfree(c5); memfree(c6);
        error(obj,ERRSPL);
        return -1;
      }
      GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,
                    f2dsplinedif,splineint,fp,x[0],y[0]);
      for (j=0;j<num-1;j++) {
        c[0]=c1[j]; c[1]=c2[j]; c[2]=c3[j];
        c[3]=c4[j]; c[4]=c5[j]; c[5]=c6[j];
        if (!GRAcurve(GC,c,x[j],y[j])) break;
      }
    }
    memfree(x);  memfree(y);  memfree(z);
    memfree(r);  memfree(g);  memfree(b);
    memfree(c1); memfree(c2); memfree(c3);
    memfree(c4); memfree(c5); memfree(c6);
  }
  return 0;
}

static int 
f2ddraw(struct objlist *obj, char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  int GC;
  int type;
  int mtype;
#if ! NEW_MATH_CODE
  int fr2, fg2, fb2;
#endif
  int lwidth, ljoin, lmiter, intp;
  struct narray *lstyle;
  int snum, *style;
  struct f2ddata *fp;
  int rcode;
  int tm, lm, w, h, clip, zoom;
  char *fit, *field;
  char *file;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  _getobj(obj, "_local", inst, &f2dlocal);
  _getobj(obj, "GC", inst, &GC);
  if (GC<0)
    return 0;

  _getobj(obj, "file", inst, &file);
  if (file == NULL)
    return 0;

  _getobj(obj, "type", inst, &type);
  _getobj(obj, "mark_type", inst, &mtype);
#if ! NEW_MATH_CODE
  _getobj(obj, "R2", inst, &fr2);
  _getobj(obj, "G2", inst, &fg2);
  _getobj(obj, "B2", inst, &fb2);
#endif
  _getobj(obj, "line_width", inst, &lwidth);
  _getobj(obj, "line_style", inst, &lstyle);
  _getobj(obj, "line_join", inst, &ljoin);
  _getobj(obj, "line_miter_limit", inst, &lmiter);
  _getobj(obj, "interpolation", inst, &intp);
  _getobj(obj, "fit", inst, &fit);
  _getobj(obj, "clip", inst, &clip);

  snum = arraynum(lstyle);
  style = arraydata(lstyle);

  fp = opendata(obj, inst, f2dlocal, TRUE, FALSE);
  if (fp == NULL)
    return 1;

  if (fp->need2pass || fp->final < -1) {
    if (getminmaxdata(fp, f2dlocal) == -1) {
      closedata(fp,  f2dlocal);
      return 1;
    }
    reopendata(fp);
  }

  if (hskipdata(fp)) {
    closedata(fp,  f2dlocal);
    return 1;
  }

#if NEW_MATH_CODE
  if (set_const(fp->codex[0], fp, FALSE))
    return 1;

  if (set_const(fp->codey[0], fp, FALSE))
    return 1;
#endif

  GRAregion(GC, &lm, &tm, &w, &h, &zoom);
  GRAview(GC, 0, 0, w*10000.0/zoom, h*10000.0/zoom, clip);
  switch (type) {
  case PLOT_TYPE_MARK:
#if NEW_MATH_CODE
    rcode = markout(obj, fp, GC, lwidth, snum, style);
#else
    rcode = markout(obj, fp, GC, fr2, fg2, fb2, lwidth, snum, style);
#endif
    break;
  case PLOT_TYPE_LINE:
    rcode = lineout(obj, fp, GC, lwidth, snum, style, ljoin, lmiter, FALSE);
    break;
  case PLOT_TYPE_POLYGON:
    rcode = lineout(obj, fp, GC, lwidth, snum, style, ljoin, lmiter, TRUE);
    break;
  case PLOT_TYPE_CURVE:
    rcode = curveout(obj, fp, GC, lwidth, snum, style, ljoin, lmiter, intp);
    break;
  case PLOT_TYPE_DIAGONAL:
  case PLOT_TYPE_ARROW:
  case PLOT_TYPE_RECTANGLE:
  case PLOT_TYPE_RECTANGLE_FILL:
  case PLOT_TYPE_RECTANGLE_SOLID_FILL:
#if NEW_MATH_CODE
    rcode = set_const(fp->codex[1], fp, FALSE);
    if (rcode)
      break;

    rcode = set_const(fp->codey[2], fp, FALSE);
    if (rcode)
      break;

    rcode = rectout(obj, fp, GC, lwidth, snum, style, type);
#else
    rcode = rectout(obj, fp, GC, fr2, fg2, fb2, lwidth, snum, style, type);
#endif
    break;
  case PLOT_TYPE_ERRORBAR_X:
#if NEW_MATH_CODE
    rcode = set_const(fp->codex[1], fp, FALSE);
    if (rcode)
      break;

    rcode = set_const(fp->codex[2], fp, FALSE);
    if (rcode)
      break;
#endif
    rcode = errorbarout(obj, fp, GC, lwidth, snum, style, type);
    break;
  case PLOT_TYPE_ERRORBAR_Y:
#if NEW_MATH_CODE
    rcode = set_const(fp->codey[1], fp, FALSE);
    if (rcode)
      break;

    rcode = set_const(fp->codey[2], fp, FALSE);
    if (rcode)
      break;
#endif
    rcode = errorbarout(obj, fp, GC, lwidth, snum, style, type);
    break;
  case PLOT_TYPE_STAIRCASE_X:
  case PLOT_TYPE_STAIRCASE_Y:
    rcode = stairout(obj, fp, GC, lwidth, snum, style, ljoin, lmiter, type);
    break;
  case PLOT_TYPE_BAR_X:
  case PLOT_TYPE_BAR_Y:
  case PLOT_TYPE_BAR_FILL_X:
  case PLOT_TYPE_BAR_FILL_Y:
  case PLOT_TYPE_BAR_SOLID_FILL_X:
  case PLOT_TYPE_BAR_SOLID_FILL_Y:
#if NEW_MATH_CODE
    rcode = barout(obj, fp, GC, lwidth, snum, style, type);
#else
    rcode = barout(obj, fp, GC, fr2, fg2, fb2, lwidth, snum, style, type);
#endif
    break;
  case PLOT_TYPE_FIT:
    field = (char *)argv[1];
    rcode = fitout(obj, fp, GC, lwidth, snum, style, ljoin, lmiter, fit, field[0] == 'r');
    if (fp->datanum == 0)
      fp->datanum = f2dlocal->num;
    break;
  default:
    /* not reachable */
    rcode = -1;
  }

  closedata(fp, f2dlocal);
  if (rcode == -1)
    return 1;

  GRAaddlist(GC, obj, inst, (char *) argv[0], "redraw");
  return 0;
}

static int 
f2dgetcoord(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  double x,y;
  int gx,gy;
  char *axisx,*axisy,*raxis;
  struct narray iarray;
  struct objlist *aobj;
  int anum,id;
  char *inst1;
  int axtype,aytype,axposx,axposy,ayposx,ayposy,axlen,aylen,dirx,diry;
  double axmin,axmax,aymin,aymax,ratex,ratey;
  double axdir,aydir;
  double axvx,axvy,ayvx,ayvy;
  double ip1,ip2;
  int dataclip;
  double minx,maxx,miny,maxy;
  double v1x,v1y,v2x,v2y,vx,vy;
  double a,b,c,d;
  struct narray *array;

  x=*(double *)argv[2];
  y=*(double *)argv[3];
  _getobj(obj,"data_clip",inst,&dataclip);
  _getobj(obj,"axis_x",inst,&axisx);
  _getobj(obj,"axis_y",inst,&axisy);
  if (axisx==NULL) return 1;
  else {
    arrayinit(&iarray,sizeof(int));
    if (getobjilist(axisx,&aobj,&iarray,FALSE,NULL)) return 1;
    anum=arraynum(&iarray);
    if (anum<1) {
      arraydel(&iarray);
      return 1;
    }
    id=*(int *)arraylast(&iarray);
    arraydel(&iarray);
    if ((inst1=getobjinst(aobj,id))==NULL) return 1;
    if (_getobj(aobj,"x",inst1,&axposx)) return 1;
    if (_getobj(aobj,"y",inst1,&axposy)) return 1;
    if (_getobj(aobj,"length",inst1,&axlen)) return 1;
    if (_getobj(aobj,"direction",inst1,&dirx)) return 1;
    if (_getobj(aobj,"min",inst1,&axmin)) return 1;
    if (_getobj(aobj,"max",inst1,&axmax)) return 1;
    if (_getobj(aobj,"type",inst1,&axtype)) return 1;
    if ((axmin==0) && (axmax==0)) {
      if (_getobj(aobj,"reference",inst1,&raxis)) return 1;
      if (raxis!=NULL) {
        arrayinit(&iarray,sizeof(int));
        if (!getobjilist(raxis,&aobj,&iarray,FALSE,NULL)) {
          anum=arraynum(&iarray);
          if (anum>0) {
            id=*(int *)arraylast(&iarray);
            arraydel(&iarray);
            if ((anum>0) && ((inst1=getobjinst(aobj,id))!=NULL)) {
              _getobj(aobj,"min",inst1,&axmin);
              _getobj(aobj,"max",inst1,&axmax);
              _getobj(aobj,"type",inst1,&axtype);
            }
          }
        }
      }
    }
    axdir=dirx/18000.0*MPI;
    axvx=cos(axdir);
    axvy=-sin(axdir);
    if (axmin==axmax) return 1;
    else if (axtype==1) {
      if ((axmin<=0) || (axmax<=0)) return 1;
      else {
          axmin=log10(axmin);
          axmax=log10(axmax);
      }
    } else if (axtype==2) {
      if (axmin*axmax<=0) return 1;
      else {
        axmin=1/axmin;
        axmax=1/axmax;
      }
    }
    ratex=axlen/(axmax-axmin);
  }
  if (axisy==NULL) return 1;
  else {
    arrayinit(&iarray,sizeof(int));
    if (getobjilist(axisy,&aobj,&iarray,FALSE,NULL)) return 1;
    anum=arraynum(&iarray);
    if (anum<1) {
      arraydel(&iarray);
      return 1;
    }
    id=*(int *)arraylast(&iarray);
    arraydel(&iarray);
    if ((inst1=getobjinst(aobj,id))==NULL) return 1;
    if (_getobj(aobj,"x",inst1,&ayposx)) return 1;
    if (_getobj(aobj,"y",inst1,&ayposy)) return 1;
    if (_getobj(aobj,"length",inst1,&aylen)) return 1;
    if (_getobj(aobj,"direction",inst1,&diry)) return 1;
    if (_getobj(aobj,"min",inst1,&aymin)) return 1;
    if (_getobj(aobj,"max",inst1,&aymax)) return 1;
    if (_getobj(aobj,"type",inst1,&aytype)) return 1;
    if ((aymin==0) && (aymax==0)) {
      if (_getobj(aobj,"reference",inst1,&raxis)) return 1;
      if (raxis!=NULL) {
        arrayinit(&iarray,sizeof(int));
        if (!getobjilist(raxis,&aobj,&iarray,FALSE,NULL)) {
          anum=arraynum(&iarray);
          if (anum>0) {
            id=*(int *)arraylast(&iarray);
            arraydel(&iarray);
            if ((anum>0) && ((inst1=getobjinst(aobj,id))!=NULL)) {
              _getobj(aobj,"min",inst1,&aymin);
              _getobj(aobj,"max",inst1,&aymax);
              _getobj(aobj,"type",inst1,&aytype);
            }
          }
        }
      }
    }
    aydir=diry/18000.0*MPI;
    ayvx=cos(aydir);
    ayvy=-sin(aydir);
    if (aymin==aymax) return 1;
    else if (aytype==1) {
      if ((aymin<=0) || (aymax<=0)) return 1;
      else {
        aymin=log10(aymin);
        aymax=log10(aymax);
      }
    } else if (aytype==2) {
      if (aymin*aymax<=0) return 1;
      else {
        aymin=1/aymin;
        aymax=1/aymax;
      }
    }
    ratey=aylen/(aymax-aymin);
  }
  ip1=-axvy*ayvx+axvx*ayvy;
  ip2=-ayvy*axvx+ayvx*axvy;
  if ((fabs(ip1)<=1e-15) || (fabs(ip2)<=1e-15)) return 1;

  gx=gy=0;
  minx=axmin;
  maxx=axmax;
  miny=aymin;
  maxy=aymax;

  if (axtype==1) {
    if (x==0) return -1;
    else if (x<0) {
      x=fabs(x);
    }
    x=log10(x);
  } else if (axtype==2) {
    if (x==0) return -1;
    x=1/x;
  }
  if (aytype==1) {
    if (y==0) return -1;
    else if (y<0) {
      y=fabs(y);
    }
    y=log10(y);
  } else if (aytype==2) {
    if (y==0) return -1;
    y=1/y;
  }

  if (dataclip &&
  ((((minx>x) || (x>maxx)) && ((maxx>x) || (x>minx)))
   || (((miny>y) || (y>maxy)) && ((maxy>y) || (y>miny))))) return 1;
  v1x=ratex*(x-minx)*axvx;
  v1y=ratex*(x-minx)*axvy;
  v2x=ratey*(y-miny)*ayvx;
  v2y=ratey*(y-miny)*ayvy;
  vx=ayposx-axposx+v2x-v1x;
  vy=ayposy-axposy+v2y-v1y;
  a=ayvy*axvx-ayvx*axvy;
  c=-ayvy*vx+ayvx*vy;
  b=axvy*ayvx-axvx*ayvy;
  d=axvy*vx-axvx*vy;
  if ((fabs(a)<=1e-16) && (fabs(b)<=1e-16)) {
    return -1;
  } else if (fabs(b)<=1e-16) {
    a=c/a;
    gx=ayposx+nround(v2x+a*axvx);
    gy=ayposy+nround(v2y+a*axvy);
  } else {
    b=d/b;
    gx=axposx+nround(v1x+b*ayvx);
    gy=axposy+nround(v1y+b*ayvy);
  }

  array=*(struct narray **)rval;
  if (arraynum(array)!=2) {
    arraydel(array);
  }
  if ((array==NULL) && ((array=arraynew(sizeof(double)))==NULL)) return 1;
  arrayins(array,&gy,0);
  arrayins(array,&gx,0);
  if (arraynum(array)==0) {
    arrayfree(array);
    *(struct narray **) rval = NULL;
    return 1;
  }
  *(struct narray **)rval=array;
  return 0;
}

#if NEW_MATH_CODE
static int
set_const_all(struct f2ddata *fp)
{
  if (set_const(fp->codex[0], fp, TRUE))
    return 1;

  if (set_const(fp->codey[0], fp, TRUE))
    return 1;

  if (set_const(fp->codex[1], fp, FALSE))
    return 1;

  if (set_const(fp->codex[2], fp, FALSE))
    return 1;

  if (set_const(fp->codey[1], fp, FALSE))
    return 1;

  if (set_const(fp->codey[2], fp, FALSE))
    return 1;

  return 0;
}
#endif

static int 
f2devaluate(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;
  char *file;
  struct narray *array;
  int gx0,gy0,gx1,gy1,num,limit;
  int minx,miny,maxx,maxy,err;
  double line,dx,dy,d2,d3;

  array=*(struct narray **)rval;
  arrayfree(array);
  array=NULL;
  *(struct narray **)rval=array;
  _getobj(obj,"_local",inst,&f2dlocal);
  _getobj(obj,"file",inst,&file);
  if (file==NULL) return 0;
  if ((fp=opendata(obj,inst,f2dlocal,TRUE,FALSE))==NULL) return 1;
  if (fp->need2pass || fp->final < -1) {
    if (getminmaxdata(fp, f2dlocal)==-1) {
      closedata(fp, f2dlocal);
      return 1;
    }
    reopendata(fp);
  }
  if (hskipdata(fp)!=0) {
    closedata(fp, f2dlocal);
    return 1;
  }
  array=arraynew(sizeof(double));
  minx=*(int *)argv[2];
  miny=*(int *)argv[3];
  maxx=*(int *)argv[4];
  maxy=*(int *)argv[5];
  err=*(int *)argv[6];
  limit=*(int *)argv[7];
  if ((minx==maxx) && (miny==maxy)) {
    minx-=err;
    maxx+=err;
    miny-=err;
    maxy+=err;
  }

#if NEW_MATH_CODE
  if (set_const_all(fp))
    return 1;
#endif
  while (getdata(fp)==0) {
    switch (fp->type) {
    case TYPE_NORMAL:
      dx=fp->dx;
      dy=fp->dy;
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
      && (getposition2(fp,fp->axtype,fp->aytype,&dx,&dy)==0)) {
        f2dtransf(dx,dy,&gx0,&gy0,fp);
        line=fp->dline;
        if ((minx<=gx0) && (gx0<maxx) && (miny<=gy0) && (gy0<=maxy)) {
          arrayadd(array,&line);
          arrayadd(array,&(fp->dx));
          arrayadd(array,&(fp->dy));
        }
      }
      break;
    case TYPE_DIAGONAL:
      dx=fp->dx;
      dy=fp->dy;
      d2=fp->d2;
      d3=fp->d3;
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
      && (fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)
      && (getposition2(fp,fp->axtype,fp->aytype,&dx,&dy)==0)
      && (getposition2(fp,fp->axtype,fp->aytype,&d2,&d3)==0)) {
        f2dtransf(dx,dy,&gx0,&gy0,fp);
        f2dtransf(d2,d3,&gx1,&gy1,fp);
        line=fp->dline;
        if ((minx<=gx0) && (gx0<maxx) && (miny<=gy0) && (gy0<=maxy)) {
          arrayadd(array,&line);
          arrayadd(array,&(fp->dx));
          arrayadd(array,&(fp->dy));
        }
        if ((minx<=gx1) && (gx1<maxx) && (miny<=gy1) && (gy1<=maxy)) {
          arrayadd(array,&line);
          arrayadd(array,&(fp->d2));
          arrayadd(array,&(fp->d3));
        }
      }
      break;
    case TYPE_ERR_X:
      dx=fp->dx;
      dy=fp->dy;
      d2=fp->d2;
      d3=fp->d3;
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
      && (fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)
      && (getposition2(fp,fp->axtype,fp->aytype,&dx,&dy)==0)
      && (getposition2(fp,fp->axtype,fp->axtype,&d2,&d3)==0)) {
        f2dtransf(d2,dy,&gx0,&gy0,fp);
        f2dtransf(d3,dy,&gx1,&gy1,fp);
        line=fp->dline;
        if ((minx<=gx0) && (gx0<maxx) && (miny<=gy0) && (gy0<=maxy)) {
          arrayadd(array,&line);
          arrayadd(array,&(fp->d2));
          arrayadd(array,&(fp->dy));
        }
        if ((minx<=gx1) && (gx1<maxx) && (miny<=gy1) && (gy1<=maxy)) {
          arrayadd(array,&line);
          arrayadd(array,&(fp->d3));
          arrayadd(array,&(fp->dy));
        }
      }
      break;
    case TYPE_ERR_Y:
      dx=fp->dx;
      dy=fp->dy;
      d2=fp->d2;
      d3=fp->d3;
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
       && (fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)
       && (getposition2(fp,fp->axtype,fp->aytype,&dx,&dy)==0)
       && (getposition2(fp,fp->aytype,fp->aytype,&d2,&d3)==0)) {
        f2dtransf(dx,d2,&gx0,&gy0,fp);
        f2dtransf(dx,d3,&gx1,&gy1,fp);
        line=fp->dline;
        if ((minx<=gx0) && (gx0<maxx) && (miny<=gy0) && (gy0<=maxy)) {
          arrayadd(array,&line);
          arrayadd(array,&(fp->dx));
          arrayadd(array,&(fp->d2));
        }
        if ((minx<=gx1) && (gx1<maxx) && (miny<=gy1) && (gy1<=maxy)) {
          arrayadd(array,&line);
          arrayadd(array,&(fp->dx));
          arrayadd(array,&(fp->d3));
        }
      }
      break;
    }
    if (arraynum(array) >= (limit * 3)) break;
  }
  closedata(fp, f2dlocal);
  num=arraynum(array);
  if (num/3>0) *(struct narray **)rval=array;
  else arrayfree(array);
  return 0;
}

static int 
f2dredraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int redrawf, num, dmax;
  int GC;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"redraw_flag",inst,&redrawf);
  _getobj(obj,"data_num",inst,&num);
  _getobj(obj,"redraw_num",inst,&dmax);

  if (num > 0 && num <= dmax && redrawf) {
    f2ddraw(obj,inst,rval,argc,argv);
  } else {
    _getobj(obj,"GC",inst,&GC);
    if (GC<0) return 0;
    GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  }
  return 0;
}

static int 
f2dcolumn(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *file,*ifs;
  int csv;
  int cline,ccol;
  int line,col;
  FILE *fd;
  int rcode;
  char *buf,*buf2;
  char *po,*po2;

  memfree(*(char **)rval);
  *(char **)rval=NULL;
  _getobj(obj,"file",inst,&file);
  _getobj(obj,"ifs",inst,&ifs);
  _getobj(obj,"csv",inst,&csv);
  line=*(int *)argv[2];
  col=*(int *)argv[3];
  if (line<=0) return 0;
  if (col<=0) col=0;
  if (file==NULL) return 0;
  if ((fd=nfopen(file,"rt"))==NULL) return 0;
  cline=0;
  while (TRUE) {
    if ((rcode=fgetline(fd,&buf))!=0) {
      fclose(fd);
      return 1;
    }
    cline++;
    if (cline==line) {
      if (col==0) *(char **)rval=buf;
      else {
        ccol=0;
        po=buf;
        while (TRUE) {
          ccol++;
          if (*po=='\0') break;
          if (csv) {
            for (;*po==' ';po++);
            if (*po=='\0') break;
            if (strchr(ifs,*po)!=NULL) {
              if (ccol==col) break;
              else po++;
            } else {
              for (po2=po;(*po2!='\0') && (strchr(ifs,*po2)==NULL) && (*po2!=' ');po2++);
              if (ccol==col) {
                if ((buf2=memalloc(po2-po+1))==NULL) {
                  fclose(fd);
                  memfree(buf);
                  return 1;
                }
                strncpy(buf2,po,po2-po);
                buf2[po2-po]='\0';
                *(char **)rval=buf2;
                break;
              } else {
                for (;(*po2==' ');po2++);
                if (strchr(ifs,*po2)!=NULL) po2++;
                po=po2;
              }  
            }
          } else {
            for (;(*po!='\0') && (strchr(ifs,*po)!=NULL);po++);
            if (*po=='\0') break;
            for (po2=po;(*po2!='\0') && (strchr(ifs,*po2)==NULL);po2++);
            if (ccol==col) {
              if ((buf2=memalloc(po2-po+1))==NULL) {
                fclose(fd);
                memfree(buf);
                return 1;
              }
              strncpy(buf2,po,po2-po);
              buf2[po2-po]='\0';
              *(char **)rval=buf2;
              break;
            } else po=po2;
          }
        }
        memfree(buf);
      }
      break;
    } else memfree(buf);
  }
  fclose(fd);
  return 0;
}

static int 
f2dhead(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int cline, line, n, p;
  char *file, buf[256], *s;
  FILE *fd;

  memfree(*(char **)rval);

  *(char **) rval = NULL;

  _getobj(obj, "file", inst, &file);

  line = *(int *) argv[2];

  if (line <= 0)
    return 0;

  if (file == NULL)
    return 0;

  fd = nfopen(file, "rt");
  if (fd == NULL)
    return 0;

  s = nstrnew();
  if (s == NULL) {
    fclose(fd);
    return 0;
  }

  p = ceil(log10(line + 1));

  for (cline = 0; cline < line; cline++) {
    n = sprintf(buf, "%*d ", p, cline + 1);

    if (fgetnline(fd, buf + n, sizeof(buf) - n))
      break;

    if (cline)
      s = nstrccat(s,'\n');

    s = nstrcat(s,buf);
  }

  fclose(fd);

  *(char **) rval = s;

  return 0;
}

static int 
f2dsettings(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *file;
  FILE *fd;
  int err;
  char *buf, *rem;
  char *po,*endptr;
  int d1,d2,d3;
  double f1,f2,f3;
  int i,j,id;
  char *s;
  struct narray *iarray,iarray2;
  struct objlist *aobj;
  int aid,x;

  _getobj(obj,"file",inst,&file);
  if (file==NULL) return 0;
  if ((fd=nfopen(file,"rt"))==NULL) return 0;
  if (fgetline(fd,&buf)!=0) {
    fclose(fd);
    return 1;
  }
  _getobj(obj,"id",inst,&id);
  fclose(fd);
  po=buf;
  err=FALSE;

  sgetobjfield(obj, id, "remark", NULL, &rem, FALSE, FALSE, FALSE);
  if (rem && strchr(rem, po[0]))
    po++;
  memfree(rem);

  while (po[0]!='\0') {
    for (;(po[0]!='\0') && (strchr(" \t",po[0])!=NULL);po++);
    if (po[0]=='-') {
      switch (po[1]) {
      case 'x':
        d1=strtol(po+2,&endptr,10);
        if (endptr==(po+2)) err=TRUE;
        else {
          po=endptr;
          putobj(obj,"x",id,&d1);
        }
        break;
      case 'y':
        d1=strtol(po+2,&endptr,10);
        if (endptr==(po+2)) err=TRUE;
        else {
          po=endptr;
          putobj(obj,"y",id,&d1);
        }
        break;
      case 'd':
        for (i = PLOT_TYPE_FIT; i >= 0; i--) {
          if (strncmp(f2dtypechar[i], po + 2, strlen(f2dtypechar[i])) == 0)
	    break;
	}
        if ((i==-1) ||
	    ((i == PLOT_TYPE_MARK) && (po[2+4]!=',')) ||
	    ((i == PLOT_TYPE_CURVE) && (po[2+5]!=','))) {
	  err=TRUE;
	} else {
          if (i == PLOT_TYPE_MARK) {
            d1=strtol(po+2+5,&endptr,10);
            if (endptr==(po+2+5)) err=TRUE;
            else {
              putobj(obj,"type",id,&i);
              putobj(obj,"mark_type",id,&d1);
              po=endptr;
            }
          } else if (i == PLOT_TYPE_CURVE) {
            for (j = INTERPOLATION_TYPE_BSPLINE_CLOSE; j >= 0; j--) {
              if (strncmp(intpchar[j], po + 2 + 6, strlen(intpchar[j])) == 0)
		break;
	    }
            if (j == -1) {
	      err = TRUE;
	    } else {
              putobj(obj,"type",id,&i);
              putobj(obj,"interpolation",id,&j);
              po=po+2+6+strlen(intpchar[j]);
            }
          } else {
            putobj(obj,"type",id,&i);
            po=po+2+strlen(f2dtypechar[i]);
          }
        }
        break;
      case 'o':
        d1=strtol(po+2,&endptr,10);
        if (endptr==(po+2)) err=TRUE;
        else {
          po=endptr;
          putobj(obj,"mark_size",id,&d1);
        }
        break;
      case 'l':
        iarray=arraynew(sizeof(int));
        po+=1;
        do {
          po++;
          d1=strtol(po,&endptr,10);
          if (endptr==po) err=TRUE;
          else {
            po=endptr;
            arrayadd(iarray,&d1);
          }
        } while ((!err) && (po[0]==','));
        if (err) arrayfree(iarray);
        else putobj(obj,"line_style",id,iarray);
        break;
      case 'w':
        d1=strtol(po+2,&endptr,10);
        if (endptr==(po+2)) err=TRUE;
        else {
          po=endptr;
          putobj(obj,"line_width",id,&d1);
        }
        break;
      case 'c':
        po+=2;
        d1=strtol(po,&endptr,10);
        if ((endptr==po) || (endptr[0]!=',')) err=TRUE;
        else {
          po=endptr+1;
          d2=strtol(po,&endptr,10);
          if ((endptr==po) || (endptr[0]!=',')) err=TRUE;
          else {
            po=endptr+1;
            d3=strtol(po,&endptr,10);
            if (endptr==po) err=TRUE;
            else po=endptr;
          }
        }
        if (!err) {
          putobj(obj,"R",id,&d1);
          putobj(obj,"G",id,&d2);
          putobj(obj,"B",id,&d3);
        }
        break;
      case 'C':
        po+=2;
        d1=strtol(po,&endptr,10);
        if ((endptr==po) || (endptr[0]!=',')) err=TRUE;
        else {
          po=endptr+1;
          d2=strtol(po,&endptr,10);
          if ((endptr==po) || (endptr[0]!=',')) err=TRUE;
          else {
            po=endptr+1;
            d3=strtol(po,&endptr,10);
            if (endptr==po) err=TRUE;
            else po=endptr;
          }
        }
        if (!err) {
          putobj(obj,"R2",id,&d1);
          putobj(obj,"G2",id,&d2);
          putobj(obj,"B2",id,&d3);
        }
        break;
      case 'v':
        if (po[2]=='x') {
          d1=strtol(po+3,&endptr,10);
          if (endptr==(po+3)) err=TRUE;
          else {
            po=endptr;
            putobj(obj,"smooth_x",id,&d1);
          }
          break;
        } else if (po[2]=='y') {
          d1=strtol(po+3,&endptr,10);
          if (endptr==(po+3)) err=TRUE;
          else {
            po=endptr;
            putobj(obj,"smooth_y",id,&d1);
          }
          break;
        } else err=TRUE;
        break;
      case 's':
        d1=strtol(po+2,&endptr,10);
        if (endptr==(po+2)) err=TRUE;
        else {
          po=endptr;
          putobj(obj,"head_skip",id,&d1);
        }
        break;
      case 'r':
        d1=strtol(po+2,&endptr,10);
        if (endptr==(po+2)) err=TRUE;
        else {
          po=endptr;
          putobj(obj,"read_step",id,&d1);
        }
        break;
      case 'f':
        d1=strtol(po+2,&endptr,10);
        if (endptr==(po+2)) err=TRUE;
        else {
          po=endptr;
          putobj(obj,"final_line",id,&d1);
        }
        break;
      case 'z':
        if ((po[2]=='x') || (po[2]=='y')) {
          x = (po[2] == 'x');
          po+=3;
          f1=strtod(po,&endptr);
          if (check_infinite(f1) || endptr == po || endptr[0] != ',') {
	    err=TRUE;
	  } else {
            po=endptr+1;
            f2=strtod(po,&endptr);
            if (check_infinite(f2) || endptr == po || endptr[0] != ',') {
	      err=TRUE;
	    } else {
              po=endptr+1;
              f3=strtod(po,&endptr);
              if (check_infinite(f3) || endptr == po) {
		err=TRUE;
	      } else {
		po=endptr;
	      }
	    }
          }
          if (!err) {
            if (x) _getobj(obj,"axis_x",inst,&s);
            else _getobj(obj,"axis_y",inst,&s);
            if (s!=NULL) {
              arrayinit(&iarray2,sizeof(int));
              if (!getobjilist(s,&aobj,&iarray2,FALSE,NULL)) {
                if (arraynum(&iarray2)>=1) {
                  aid=*(int *)arraylast(&iarray2);
                  putobj(aobj,"min",aid,&f1);
                  putobj(aobj,"max",aid,&f2);
                  putobj(aobj,"inc",aid,&f3);
                }
              }
              arraydel(&iarray2);
            }
          }
        } else err=TRUE;
        break;
      case 'e':
        if ((po[2]=='x') || (po[2]=='y')) {
          for (i=0;i<3;i++)
            if (strncmp(axistypechar[i],po+3,strlen(axistypechar[i]))==0) break;
          if (i!=3) {
            if (po[2]=='x') _getobj(obj,"axis_x",inst,&s);
            else _getobj(obj,"axis_y",inst,&s);
            if (s!=NULL) {
              arrayinit(&iarray2,sizeof(int));
              if (!getobjilist(s,&aobj,&iarray2,FALSE,NULL)) {
                if (arraynum(&iarray2)>=1) {
                  aid=*(int *)arraylast(&iarray2);
                  putobj(aobj,"type",aid,&i);
                }
              }
              arraydel(&iarray2);
            }
            po=po+3+strlen(axistypechar[i]);
          } else err=TRUE;
        } else err=TRUE;
        break;
      case 'm':
        if (po[2] != 'x' && po[2] != 'y') {
	  err = TRUE;
	  break;
	}

	for (i=3;(po[i]!='\0') && (strchr(" \t",po[i])==NULL);i++);
	if (i>3) {
	  if ((s=memalloc(i-2))!=NULL) {
	    strncpy(s,po+3,i-3);
	    s[i-3]='\0';
	  } else {
	    err=TRUE;
	    break;
	  }
	} else s=NULL;
	putobj(obj, (po[2] == 'x') ?  "math_x" : "math_y", id, s);
	po+=i;
        break;
      default:
        break;
      }
    } else err=TRUE;
    if (err) {
      error2(obj,ERRILOPTION,buf);
      memfree(buf);
      return 1;
    }
  }
  memfree(buf);
  return 0;
}


static int 
f2dtime(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *file;
  struct stat buf;
  int style;

  memfree(*(char **)rval);
  *(char **)rval=NULL;
  _getobj(obj,"file",inst,&file);
  if (file==NULL) return 0;
  if (stat(file,&buf)!=0) return 1;
  style=*(int *)(argv[2]);
  *((char **)rval)=ntime((time_t *)&(buf.st_mtime),style);
  return 0;
}

static int 
f2ddate(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *file;
  struct stat buf;
  int style;

  memfree(*(char **)rval);
  *(char **)rval=NULL;
  _getobj(obj,"file",inst,&file);
  if (file==NULL) return 0;
  if (stat(file,&buf)!=0) return 1;
  style=*(int *)(argv[2]);
  *(char **)rval=ndate((time_t *)&(buf.st_mtime),style);
  return 0;
}

static void 
f2dsettbl(char *inst,struct f2dlocal *f2dlocal,struct f2ddata *fp)
{
  int gx,gy,g2,g3;

  switch (fp->type) {
  case TYPE_NORMAL:
    *(double *)(inst+f2dlocal->idx)=fp->dx;
    *(double *)(inst+f2dlocal->idy)=fp->dy;
    if (f2dlocal->coord) {
      getposition(fp,fp->dx,fp->dy,&gx,&gy);
      *(int *)(inst+f2dlocal->icx)=gx;
      *(int *)(inst+f2dlocal->icy)=gy;
    }
    break;
  case TYPE_DIAGONAL:
    *(double *)(inst+f2dlocal->idx)=fp->dx;
    *(double *)(inst+f2dlocal->idy)=fp->dy;
    *(double *)(inst+f2dlocal->id2)=fp->d2;
    *(double *)(inst+f2dlocal->id3)=fp->d3;
    if (f2dlocal->coord) {
      getposition(fp,fp->dx,fp->dy,&gx,&gy);
      getposition(fp,fp->d2,fp->d3,&g2,&g3);
      *(int *)(inst+f2dlocal->icx)=gx;
      *(int *)(inst+f2dlocal->icy)=gy;
      *(int *)(inst+f2dlocal->ic2)=g2;
      *(int *)(inst+f2dlocal->ic3)=g3;
    }
    break;
  case TYPE_ERR_X:
    *(double *)(inst+f2dlocal->idx)=fp->dx;
    *(double *)(inst+f2dlocal->idy)=fp->dy;
    *(double *)(inst+f2dlocal->id2)=fp->d2;
    *(double *)(inst+f2dlocal->id3)=fp->d3;
    if (f2dlocal->coord) {
      getposition(fp,fp->dx,fp->dy,&gx,&gy);
      getposition(fp,fp->d2,fp->dy,&g2,&gy);
      getposition(fp,fp->d3,fp->dy,&g3,&gy);
      *(int *)(inst+f2dlocal->icx)=gx;
      *(int *)(inst+f2dlocal->icy)=gy;
      *(int *)(inst+f2dlocal->ic2)=g2;
      *(int *)(inst+f2dlocal->ic3)=g3;
    }
    break;
  case TYPE_ERR_Y:
    *(double *)(inst+f2dlocal->idx)=fp->dx;
    *(double *)(inst+f2dlocal->idy)=fp->dy;
    *(double *)(inst+f2dlocal->id2)=fp->d2;
    *(double *)(inst+f2dlocal->id3)=fp->d3;
    if (f2dlocal->coord) {
      getposition(fp,fp->dx,fp->dy,&gx,&gy);
      getposition(fp,fp->dx,fp->d2,&gx,&g2);
      getposition(fp,fp->dx,fp->d3,&gx,&g3);
      *(int *)(inst+f2dlocal->icx)=gx;
      *(int *)(inst+f2dlocal->icy)=gy;
      *(int *)(inst+f2dlocal->ic2)=g2;
      *(int *)(inst+f2dlocal->ic3)=g3;
    }
    break;
  }
  *(int *)(inst+f2dlocal->isx)=fp->dxstat;
  *(int *)(inst+f2dlocal->isy)=fp->dystat;
  *(int *)(inst+f2dlocal->is2)=fp->d2stat;
  *(int *)(inst+f2dlocal->is3)=fp->d3stat;
  *(int *)(inst+f2dlocal->iline)=fp->dline;
}

static int 
f2dopendata(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;
  int num2;

  _getobj(obj,"_local",inst,&f2dlocal);
  if (strcmp0((char *)argv[1],"opendatac")==0) f2dlocal->coord=TRUE;
  else f2dlocal->coord=FALSE;
  fp=f2dlocal->data;
  if (fp!=NULL) closedata(fp, f2dlocal);
  f2dlocal->data=NULL;
  if ((fp=opendata(obj,inst,f2dlocal,f2dlocal->coord,FALSE))==NULL) return 1;
  if (fp->need2pass || fp->final < -1) {
    if (getminmaxdata(fp, f2dlocal)==-1) {
      closedata(fp, f2dlocal);
      f2dlocal->data=NULL;
      return 1;
    }
    reopendata(fp);
  }
  if (hskipdata(fp)!=0) {
    closedata(fp, f2dlocal);
    f2dlocal->data=NULL;
    return 1;
  }
  f2dsettbl(inst,f2dlocal,fp);
  num2=0;
  _putobj(obj,"data_num",inst,&num2);
  f2dlocal->data=fp;
  return 0;
}

static int 
f2dgetdata(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;
  int rcode;

  _getobj(obj,"_local",inst,&f2dlocal);
  fp=f2dlocal->data;
  if (fp==NULL) return 1;
  rcode=getdata(fp);
  f2dsettbl(inst,f2dlocal,fp);
  if (rcode!=0) {
    closedata(fp, f2dlocal);
    f2dlocal->data=NULL;
    return 1;
  }
  return 0;
}

static int 
f2dclosedata(struct objlist *obj,char *inst,char *rval,
                 int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;

  _getobj(obj,"_local",inst,&f2dlocal);
  fp=f2dlocal->data;
  if (fp==NULL) return 0;
  fp->dx=fp->dy=fp->d2=fp->d3=0;
  fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MEOF;
  f2dsettbl(inst,f2dlocal,fp);
  closedata(fp, f2dlocal);
  f2dlocal->data=NULL;
  return 0;
}

static int 
f2dopendataraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;

  _getobj(obj,"_local",inst,&f2dlocal);
  fp=f2dlocal->data;
  if (fp!=NULL) closedata(fp, f2dlocal);
  f2dlocal->data=NULL;
  if ((fp=opendata(obj,inst,f2dlocal,FALSE,TRUE))==NULL) return 1;

  if (fp->final < -1) {
    if (getminmaxdata(fp, f2dlocal) == -1) {
      closedata(fp,  f2dlocal);
      return 1;
    }
    reopendata(fp);
  }

  if (hskipdata(fp)!=0) {
    closedata(fp, f2dlocal);
    f2dlocal->data=NULL;
    return 1;
  }
  f2dlocal->data=fp;
  return 0;
}

static int 
f2dgetdataraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;
  int rcode;
  struct narray *iarray;
  struct narray *darray;
  int i,num,*data,maxdim;
  double d;
#if NEW_MATH_CODE
  MathValue *gdata = NULL;
#else
  double *gdata = NULL;
  char *gstat = NULL;
#endif

#if NEW_MATH_CODE
  gdata = memalloc(sizeof(MathValue) * (FILE_OBJ_MAXCOL + 1));
  if (gdata == NULL) {
   memfree(gdata);
   return -1;
  }
#else
  if (((gdata=(double *)memalloc(sizeof(double)*(FILE_OBJ_MAXCOL+1)))==NULL)
  || ((gstat=(char *)memalloc(sizeof(char)*(FILE_OBJ_MAXCOL+1)))==NULL)) {
   memfree(gdata);
   memfree(gstat);
   return -1;
  }
#endif
  _getobj(obj,"_local",inst,&f2dlocal);
  fp=f2dlocal->data;
  if (fp==NULL) {
    memfree(gdata);
#if ! NEW_MATH_CODE
    memfree(gstat);
#endif
    return 0;
  }
  darray=*(struct narray **)rval;
  arrayfree(darray);
  *(struct narray **)rval=NULL;
  iarray=(struct narray *)argv[2];
  num=arraynum(iarray);
  data=arraydata(iarray);
  maxdim=-1;
  for (i=0;i<num;i++) {
    if (data[i]>maxdim) {
      maxdim=data[i];
    }
  }
  if (maxdim==-1) {
    memfree(gdata);
#if ! NEW_MATH_CODE
    memfree(gstat);
#endif
    return 0;
  }
  if (maxdim>FILE_OBJ_MAXCOL) maxdim=FILE_OBJ_MAXCOL;
#if NEW_MATH_CODE
  rcode = getdataraw(fp, maxdim, gdata);
#else
  rcode=getdataraw(fp,maxdim,gdata,gstat);
#endif
  if (rcode!=0) {
    closedata(fp, f2dlocal);
    f2dlocal->data=NULL;
    memfree(gdata);
#if ! NEW_MATH_CODE
    memfree(gstat);
#endif
    return 1;
  }
  darray=arraynew(sizeof(double));
  for (i=0;i<num;i++) {
    if (data[i]>FILE_OBJ_MAXCOL) {
      d=0;
    } else {
#if NEW_MATH_CODE
      d = gdata[data[i]].val;
#else
      d=gdata[data[i]];
#endif
    }
    arrayadd(darray,&d);
  }
  for (i=0;i<num;i++) {
    if (data[i]>FILE_OBJ_MAXCOL) {
      d=MNONUM;
    } else {
#if NEW_MATH_CODE
      d = gdata[data[i]].type;
#else
      d=gstat[data[i]];
#endif
    }
    arrayadd(darray,&d);
  }
  *(struct narray **)rval=darray;
  memfree(gdata);
#if ! NEW_MATH_CODE
  memfree(gstat);
#endif
  return 0;
}

static int 
f2dclosedataraw(struct objlist *obj,char *inst,char *rval,
                 int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;

  _getobj(obj,"_local",inst,&f2dlocal);
  fp=f2dlocal->data;
  if (fp==NULL) return 0;
  closedata(fp, f2dlocal);
  f2dlocal->data=NULL;
  return 0;
}

static time_t 
get_mtime(struct objlist *obj, char *inst, time_t *mtime)
{
  struct stat buf;
  int r;
  char *file;

  _getobj(obj, "file", inst, &file);
  
  if (file == NULL)
    return 1;

  r = stat(file, &buf);
  if (r)
    return 1;

  *mtime = buf.st_mtime;
  return 0;
}

static int 
f2dstat(struct objlist *obj,char *inst,char *rval,
	int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;
  int rcode, interrupt;
  int dnum,minxstat,maxxstat,minystat,maxystat;
  double minx,maxx,miny,maxy;
  double sumx,sumxx,sumy,sumyy;
  char *field;
  char str[32], *ptr;
  time_t mtime;

  memfree(*(char **)rval);
  *(char **)rval=NULL;
  field=argv[1];
  _getobj(obj,"_local",inst,&f2dlocal);

  /* evaluation of mtime must be done before call opendata(). */

  if (get_mtime(obj, inst, &mtime))
    return 1;

  if (f2dlocal->mtime_stat == mtime) {
    minx = f2dlocal->dminx;
    maxx = f2dlocal->dmaxx;
    miny = f2dlocal->dminy;
    maxy = f2dlocal->dmaxy;
    sumx = f2dlocal->davx;
    sumy = f2dlocal->davy;
    sumxx = f2dlocal->dsigx;
    sumyy = f2dlocal->dsigy;
    dnum = f2dlocal->num;

    if (f2dlocal->dminx != HUGE_VAL || strcmp(field, "dnum") == 0) {
      goto End;
    }
  }

  fp = opendata(obj, inst, f2dlocal, FALSE, FALSE);
  if (fp == NULL) return 1;

  if (fp->need2pass || fp->final < -1) {
    if (getminmaxdata(fp, f2dlocal)==-1) {
      closedata(fp, f2dlocal);
      return 1;
    }
    reopendata(fp);
  }
  if (hskipdata(fp)!=0) {
    closedata(fp, f2dlocal);
    return 1;
  }
  minxstat=MUNDEF;
  maxxstat=MUNDEF;
  minystat=MUNDEF;
  maxystat=MUNDEF;
  dnum=0;
  minx=maxx=miny=maxy=0;
  sumx=sumxx=sumy=sumyy=0;

#if NEW_MATH_CODE
  if (set_const_all(fp))
    return 1;
#endif
  while ((rcode=getdata(fp))==0) {
    switch (fp->type) {
    case TYPE_NORMAL:
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)) {
        if ((minxstat==MUNDEF) || (minx>fp->dx)) minx=fp->dx;
        if ((maxxstat==MUNDEF) || (maxx<fp->dx)) maxx=fp->dx;
        minxstat=MNOERR;
        maxxstat=MNOERR;
        if ((minystat==MUNDEF) || (miny>fp->dy)) miny=fp->dy;
        if ((maxystat==MUNDEF) || (maxy<fp->dy)) maxy=fp->dy;
        minystat=MNOERR;
        maxystat=MNOERR;
        sumx=sumx+fp->dx;
        sumxx=sumxx+(fp->dx)*(fp->dx);
        sumy=sumy+fp->dy;
        sumyy=sumyy+(fp->dy)*(fp->dy);
        dnum++;
      }
      break;
    case TYPE_DIAGONAL:
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
       && (fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)) {
        if ((minxstat==MUNDEF) || (minx>fp->dx)) minx=fp->dx;
        if ((maxxstat==MUNDEF) || (maxx<fp->dx)) maxx=fp->dx;
        minxstat=MNOERR;
        maxxstat=MNOERR;
        if ((minystat==MUNDEF) || (miny>fp->dy)) miny=fp->dy;
        if ((maxystat==MUNDEF) || (maxy<fp->dy)) maxy=fp->dy;
        minystat=MNOERR;
        maxystat=MNOERR;
        if ((minxstat==MUNDEF) || (minx>fp->d2)) minx=fp->d2;
        if ((maxxstat==MUNDEF) || (maxx<fp->d2)) maxx=fp->d2;
        minxstat=MNOERR;
        maxxstat=MNOERR;
        if ((minystat==MUNDEF) || (miny>fp->d3)) miny=fp->d3;
        if ((maxystat==MUNDEF) || (maxy<fp->d3)) maxy=fp->d3;
        minystat=MNOERR;
        maxystat=MNOERR;
        sumx=sumx+fp->dx;
        sumxx=sumxx+(fp->dx)*(fp->dx);
        sumy=sumy+fp->dy;
        sumyy=sumyy+(fp->dy)*(fp->dy);
        dnum++;
        sumx=sumx+fp->d2;
        sumxx=sumxx+(fp->d2)*(fp->d2);
        sumy=sumy+fp->d3;
        sumyy=sumyy+(fp->d3)*(fp->d3);
        dnum++;
      }
      break;
    case TYPE_ERR_X:
      if ((fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)
       && (fp->dystat==MNOERR)) {
        if ((minystat==MUNDEF) || (miny>fp->dy)) miny=fp->dy;
        if ((maxystat==MUNDEF) || (maxy<fp->dy)) maxy=fp->dy;
        minystat=MNOERR;
        maxystat=MNOERR;
        if ((minxstat==MUNDEF) || (minx>fp->d2)) minx=fp->d2;
        if ((maxxstat==MUNDEF) || (maxx<fp->d2)) maxx=fp->d2;
        minxstat=MNOERR;
        maxxstat=MNOERR;
        if ((minxstat==MUNDEF) || (minx>fp->d3)) minx=fp->d3;
        if ((maxxstat==MUNDEF) || (maxx<fp->d3)) maxx=fp->d3;
        minxstat=MNOERR;
        maxxstat=MNOERR;
        sumx=sumx+fp->d2;
        sumxx=sumxx+(fp->d2)*(fp->d2);
        sumy=sumy+fp->dy;
        sumyy=sumyy+(fp->dy)*(fp->dy);
        dnum++;
        sumx=sumx+fp->d3;
        sumxx=sumxx+(fp->d3)*(fp->d3);
        sumy=sumy+fp->dy;
        sumyy=sumyy+(fp->dy)*(fp->dy);
        dnum++;
      }
      break;
    case TYPE_ERR_Y:
      if ((fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)
       && (fp->dxstat==MNOERR)) {
        if ((minxstat==MUNDEF) || (minx>fp->dx)) minx=fp->dx;
        if ((maxxstat==MUNDEF) || (maxx<fp->dx)) maxx=fp->dx;
        minxstat=MNOERR;
        maxxstat=MNOERR;
        if ((minystat==MUNDEF) || (miny>fp->d2)) miny=fp->d2;
        if ((maxystat==MUNDEF) || (maxy<fp->d2)) maxy=fp->d2;
        minystat=MNOERR;
        maxystat=MNOERR;
        if ((minystat==MUNDEF) || (miny>fp->d3)) miny=fp->d3;
        if ((maxystat==MUNDEF) || (maxy<fp->d3)) maxy=fp->d3;
        minystat=MNOERR;
        maxystat=MNOERR;
        sumx=sumx+fp->dx;
        sumxx=sumxx+(fp->dx)*(fp->dx);
        sumy=sumy+fp->d2;
        sumyy=sumyy+(fp->d2)*(fp->d2);
        dnum++;
        sumx=sumx+fp->dx;
        sumxx=sumxx+(fp->dx)*(fp->dx);
        sumy=sumy+fp->d3;
        sumyy=sumyy+(fp->d3)*(fp->d3);
        dnum++;
      }
      break;
    }
  }
  interrupt = fp->interrupt;
  mtime = fp->mtime;
  closedata(fp, f2dlocal);
  if (rcode==-1) return -1;
  if (dnum!=0) {
    sumx=sumx/(double )dnum;
    sumy=sumy/(double )dnum;
    sumxx=sqrt((sumxx/(double )dnum)-sumx*sumx);
    sumyy=sqrt((sumyy/(double )dnum)-sumy*sumy);
  } else {
    sumx=sumy=sumxx=sumyy=0;
  }
  if (minxstat!=MNOERR) minx=0;
  if (minystat!=MNOERR) miny=0;
  if (maxxstat!=MNOERR) maxx=0;
  if (maxystat!=MNOERR) maxy=0;

  f2dlocal->dminx = minx;
  f2dlocal->dmaxx = maxx;
  f2dlocal->dminy = miny;
  f2dlocal->dmaxy = maxy;
  f2dlocal->davx = sumx;
  f2dlocal->davy = sumy;
  f2dlocal->dsigx = sumxx;
  f2dlocal->dsigy = sumyy;

  if (interrupt == FALSE)
    f2dlocal->mtime_stat = mtime;

 End:
  if (strcmp(field, "dnum") == 0) {
    snprintf(str, sizeof(str), "%d", dnum);
  } else if (strcmp(field,"dminx")==0) {
    snprintf(str, sizeof(str), "%.15e", minx);
  } else if (strcmp(field,"dmaxx")==0) {
    snprintf(str, sizeof(str), "%.15e", maxx);
  } else if (strcmp(field,"dminy")==0) {
    snprintf(str, sizeof(str), "%.15e", miny);
  } else if (strcmp(field,"dmaxy")==0) {
    snprintf(str, sizeof(str), "%.15e", maxy);
  } else if (strcmp(field,"davx")==0) {
    snprintf(str, sizeof(str), "%.15e", sumx);
  } else if (strcmp(field,"davy")==0) {
    snprintf(str, sizeof(str), "%.15e", sumy);
  } else if (strcmp(field,"dsigx")==0) {
    snprintf(str, sizeof(str), "%.15e", sumxx);
  } else if (strcmp(field,"dsigy")==0) {
    snprintf(str, sizeof(str), "%.15e", sumyy);
  }

  ptr = nstrdup(str);
  if (ptr == NULL) return -1;

  *(char **)rval = ptr;

  return 0;
}

static int 
f2dstat2(struct objlist *obj,char *inst,char *rval,
	 int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;
  int rcode;
  int line;
  double dx,dy,d2,d3;
  int find;
  char *field;
  char str[32], *ptr;

  memfree(*(char **)rval);
  *(char **)rval=NULL;
  field=argv[1];
  line=*(int *)(argv[2]);
  _getobj(obj,"_local",inst,&f2dlocal);
  if ((fp=opendata(obj,inst,f2dlocal,FALSE,FALSE))==NULL) return 1;
  if (fp->need2pass || fp->final < -1) {
    if (getminmaxdata(fp, f2dlocal)==-1) {
      closedata(fp, f2dlocal);
      return 1;
    }
    reopendata(fp);
  }
  if (hskipdata(fp)!=0) {
    closedata(fp, f2dlocal);
    return 1;
  }
  dx=dy=d2=d3=0;
  find=FALSE;

#if NEW_MATH_CODE
  if (set_const_all(fp))
    return 1;
#endif
  while ((rcode=getdata(fp))==0) {
    if (fp->dline==line) {
      switch (fp->type) {
      case TYPE_NORMAL:
        if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)) {
          dx=fp->dx;
          dy=fp->dy;
          find=TRUE;
        }
	break;
      default:
        if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
         && (fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)) {
          dx=fp->dx;
          dy=fp->dy;
          d2=fp->d2;
          d3=fp->d3;
          find=TRUE;
        }
	break;
      }
    }
  }
  closedata(fp, f2dlocal);
  if (!find) return -1;

  if (strcmp(field,"dx")==0) {
    snprintf(str, sizeof(str), "%.15e", dx);
  } else if (strcmp(field,"dy")==0) {
    snprintf(str, sizeof(str), "%.15e", dy);
  } else if (strcmp(field,"d2")==0) {
    snprintf(str, sizeof(str), "%.15e", d2);
  } else if (strcmp(field,"d3")==0) {
    snprintf(str, sizeof(str), "%.15e", d3);
  }

  ptr = nstrdup(str);
  if (ptr == NULL) return -1;

  *(char **) rval = ptr;
  return 0;
}

static int 
f2dboundings(struct objlist *obj,char *inst,char *rval, int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;
  int rcode;
  int minxstat,maxxstat,minystat,maxystat,minmaxstat;
  double minx,maxx,miny,maxy,mm;
  int abs;

  abs=*(int *)argv[2];
  mm=0;
  minmaxstat=MUNDEF;
  if (_putobj(obj,"minx",inst,&mm)) return 1;
  if (_putobj(obj,"maxx",inst,&mm)) return 1;
  if (_putobj(obj,"miny",inst,&mm)) return 1;
  if (_putobj(obj,"maxy",inst,&mm)) return 1;
  if (_putobj(obj,"stat_minx",inst,&minmaxstat)) return 1;
  if (_putobj(obj,"stat_maxx",inst,&minmaxstat)) return 1;
  if (_putobj(obj,"stat_miny",inst,&minmaxstat)) return 1;
  if (_putobj(obj,"stat_maxy",inst,&minmaxstat)) return 1;
  _getobj(obj,"_local",inst,&f2dlocal);

  fp = opendata(obj,inst,f2dlocal,FALSE,FALSE);
  if (fp == NULL)
    return 1;

  if (fp->need2pass || fp->final < -1) {
    if (getminmaxdata(fp, f2dlocal)==-1) {
      closedata(fp, f2dlocal);
      return 1;
    }
    reopendata(fp);
  }
  if (hskipdata(fp)!=0) {
    closedata(fp, f2dlocal);
    return 1;
  }
  minxstat=MUNDEF;
  maxxstat=MUNDEF;
  minystat=MUNDEF;
  maxystat=MUNDEF;
  minx=maxx=miny=maxy=0;

#if NEW_MATH_CODE
  if (set_const_all(fp))
    return 1;
#endif
  while ((rcode=getdata(fp))==0) {
    switch (fp->type) {
    case TYPE_NORMAL:
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)) {
        if ((!abs) || (fp->dx>0)) {
          if ((minxstat==MUNDEF) || (minx>fp->dx)) minx=fp->dx;
          if ((maxxstat==MUNDEF) || (maxx<fp->dx)) maxx=fp->dx;
          minxstat=MNOERR;
          maxxstat=MNOERR;
        }
        if ((!abs) || (fp->dy>0)) {
          if ((minystat==MUNDEF) || (miny>fp->dy)) miny=fp->dy;
          if ((maxystat==MUNDEF) || (maxy<fp->dy)) maxy=fp->dy;
          minystat=MNOERR;
          maxystat=MNOERR;
        }
      }
      break;
    case TYPE_DIAGONAL:
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
       && (fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)) {
        if ((!abs) || (fp->dx>0)) {
          if ((minxstat==MUNDEF) || (minx>fp->dx)) minx=fp->dx;
          if ((maxxstat==MUNDEF) || (maxx<fp->dx)) maxx=fp->dx;
          minxstat=MNOERR;
          maxxstat=MNOERR;
        }
        if ((!abs) || (fp->dy>0)) {
          if ((minystat==MUNDEF) || (miny>fp->dy)) miny=fp->dy;
          if ((maxystat==MUNDEF) || (maxy<fp->dy)) maxy=fp->dy;
          minystat=MNOERR;
          maxystat=MNOERR;
        }
        if ((!abs) || (fp->d2>0)) {
          if ((minxstat==MUNDEF) || (minx>fp->d2)) minx=fp->d2;
          if ((maxxstat==MUNDEF) || (maxx<fp->d2)) maxx=fp->d2;
          minxstat=MNOERR;
          maxxstat=MNOERR;
        }
        if ((!abs) || (fp->d3>0)) {
          if ((minystat==MUNDEF) || (miny>fp->d3)) miny=fp->d3;
          if ((maxystat==MUNDEF) || (maxy<fp->d3)) maxy=fp->d3;
          minystat=MNOERR;
          maxystat=MNOERR;
        }
      }
      break;
    case TYPE_ERR_X:
      if ((fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)
       && (fp->dystat==MNOERR)) {
        if ((!abs) || (fp->dy>0)) {
          if ((minystat==MUNDEF) || (miny>fp->dy)) miny=fp->dy;
          if ((maxystat==MUNDEF) || (maxy<fp->dy)) maxy=fp->dy;
          minystat=MNOERR;
          maxystat=MNOERR;
        }
        if ((!abs) || (fp->d2>0)) {
          if ((minxstat==MUNDEF) || (minx>fp->d2)) minx=fp->d2;
          if ((maxxstat==MUNDEF) || (maxx<fp->d2)) maxx=fp->d2;
          minxstat=MNOERR;
          maxxstat=MNOERR;
        }
        if ((!abs) || (fp->d3>0)) {
          if ((minxstat==MUNDEF) || (minx>fp->d3)) minx=fp->d3;
          if ((maxxstat==MUNDEF) || (maxx<fp->d3)) maxx=fp->d3;
          minxstat=MNOERR;
          maxxstat=MNOERR;
        }
      }
      break;
    case TYPE_ERR_Y:
      if ((fp->d2stat==MNOERR) && (fp->d3stat==MNOERR)
       && (fp->dxstat==MNOERR)) {
        if ((!abs) || (fp->dx>0)) {
          if ((minxstat==MUNDEF) || (minx>fp->dx)) minx=fp->dx;
          if ((maxxstat==MUNDEF) || (maxx<fp->dx)) maxx=fp->dx;
          minxstat=MNOERR;
          maxxstat=MNOERR;
        }
        if ((!abs) || (fp->d2>0)) {
          if ((minystat==MUNDEF) || (miny>fp->d2)) miny=fp->d2;
          if ((maxystat==MUNDEF) || (maxy<fp->d2)) maxy=fp->d2;
          minystat=MNOERR;
          maxystat=MNOERR;
        }
        if ((!abs) || (fp->d3>0)) {
          if ((minystat==MUNDEF) || (miny>fp->d3)) miny=fp->d3;
          if ((maxystat==MUNDEF) || (maxy<fp->d3)) maxy=fp->d3;
          minystat=MNOERR;
          maxystat=MNOERR;
        }
      }
      break;
    }
  }
  closedata(fp, f2dlocal);
  if (rcode==-1) return -1;
  if (_putobj(obj,"minx",inst,&minx)) return 1;
  if (_putobj(obj,"maxx",inst,&maxx)) return 1;
  if (_putobj(obj,"miny",inst,&miny)) return 1;
  if (_putobj(obj,"maxy",inst,&maxy)) return 1;
  if (_putobj(obj,"stat_minx",inst,&minxstat)) return 1;
  if (_putobj(obj,"stat_maxx",inst,&maxxstat)) return 1;
  if (_putobj(obj,"stat_miny",inst,&minystat)) return 1;
  if (_putobj(obj,"stat_maxy",inst,&maxystat)) return 1;
  _getobj(obj,"_local",inst,&f2dlocal);
  return 0;
}

static int 
f2dbounding(struct objlist *obj,char *inst,char *rval,
	    int argc,char **argv)
{
  struct narray *minmax;
  char *axiss;
  char *axisx,*axisy;
  struct narray iarray,iarray2;
  struct objlist *aobj;
  int i,anum,anum2,id,r = 1;
  int *adata;
  double min,max;
  int minstat,maxstat;
  int hidden;
  int type,abs;
  char *argv2[2];

  minmax=*(struct narray **)rval;
  if (minmax!=NULL) arrayfree(minmax);
  *(struct narray **)rval=NULL;
  axiss=(char *)argv[2];
  if (axiss==NULL) return 0;
  _getobj(obj,"hidden",inst,&hidden);
  if (hidden) return 0;
  arrayinit(&iarray,sizeof(int));
  if (getobjilist(axiss,&aobj,&iarray,FALSE,NULL)) return 0;
  anum=arraynum(&iarray);
  if (anum<1) {
    arraydel(&iarray);
    return 0;
  }
  adata=arraydata(&iarray);
  _getobj(obj,"axis_x",inst,&axisx);
  if (axisx!=NULL) {
    arrayinit(&iarray2,sizeof(int));

    if (getobjilist(axisx,&aobj,&iarray2,FALSE,NULL))
      goto exit;

    anum2=arraynum(&iarray2);

    if (anum2<1) {
      arraydel(&iarray2);
    } else {
      id=*(int *)arraylast(&iarray2);
      arraydel(&iarray2);
      for (i=0;i<anum;i++) if (adata[i]==id) break;
      if (i!=anum) {
        if (getobj(aobj,"type",id,0,NULL,&type)==-1) goto exit;
        abs=(type==1) ? TRUE:FALSE;
        argv2[0]=(char *)&abs;
        argv2[1]=NULL;
        if (_exeobj(obj,"boundings",inst,1,argv2)) goto exit;
        _getobj(obj,"minx",inst,&min);
        _getobj(obj,"maxx",inst,&max);
        _getobj(obj,"stat_minx",inst,&minstat);
        _getobj(obj,"stat_maxx",inst,&maxstat);
        if ((minstat==MNOERR) && (maxstat==MNOERR)) {
          minmax=arraynew(sizeof(double));
          arrayadd(minmax,&min);
          arrayadd(minmax,&max);
          *(struct narray **)rval=minmax;
        }
	r = 0;
        goto exit;
      }
    }
  }
  _getobj(obj,"axis_y",inst,&axisy);
  if (axisy!=NULL) {
    arrayinit(&iarray2,sizeof(int));
    if (getobjilist(axisy,&aobj,&iarray2,FALSE,NULL)) goto exit;
    anum2=arraynum(&iarray2);
    if (anum2<1) arraydel(&iarray2);
    else {
      id=*(int *)arraylast(&iarray2);
      arraydel(&iarray2);
      for (i=0;i<anum;i++) if (adata[i]==id) break;
      if (i!=anum) {
        if (getobj(aobj,"type",id,0,NULL,&type)==-1) goto exit;
        abs=(type==1) ? TRUE:FALSE;
        argv2[0]=(char *)&abs;
        argv2[1]=NULL;
        if (_exeobj(obj,"boundings",inst,1,argv2)) goto exit;
        _getobj(obj,"miny",inst,&min);
        _getobj(obj,"maxy",inst,&max);
        _getobj(obj,"stat_miny",inst,&minstat);
        _getobj(obj,"stat_maxy",inst,&maxstat);
        if ((minstat==MNOERR) && (maxstat==MNOERR)) {
          minmax=arraynew(sizeof(double));
          arrayadd(minmax,&min);
          arrayadd(minmax,&max);
          *(struct narray **)rval=minmax;
        }
      }
    }
  }
  r = 0;
exit:
  arraydel(&iarray);
  return r;
}

static int 
f2dsave(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct objlist *fitobj;
  struct narray iarray,*array,*array2;
  int anum,idnum;
  char **adata;
  int i,j;
  char *argv2[4];
  char *s,*s2,*fit,*fitsave;
  int id;

  array=(struct narray *)argv[2];
  anum=arraynum(array);
  adata=arraydata(array);
  for (j=0;j<anum;j++)
    if (strcmp("fit",adata[j])==0) return pathsave(obj,inst,rval,argc,argv);
  _getobj(obj,"fit",inst,&fit);
  if (fit==NULL) return pathsave(obj,inst,rval,argc,argv);
  arrayinit(&iarray,sizeof(int));
  if (getobjilist(fit,&fitobj,&iarray,FALSE,NULL)) return 1;
  idnum=arraynum(&iarray);
  if (idnum<1) {
    arraydel(&iarray);
    return pathsave(obj,inst,rval,argc,argv);
  }
  id=*(int *)arraylast(&iarray);
  arraydel(&iarray);
  if (getobj(fitobj,"save",id,0,NULL,&fitsave)==-1) return 1;
  if ((s=nstrnew())==NULL) return 1;
  if ((s=nstrcat(s,fitsave))==NULL) return 1;

  array2=arraynew(sizeof(char *));
  for (i=0;i<anum;i++) arrayadd(array2,&(adata[i]));
  s2="fit";
  arrayadd(array2,&s2);
  argv2[0]=argv[0];
  argv2[1]=argv[1];
  argv2[2]=(char *)array2;
  argv2[3]=NULL;
  if (pathsave(obj,inst,rval,3,argv2)!=0) {
    arrayfree(array2);
    return 1;
  }
  arrayfree(array2);
  if ((s=nstrcat(s,*(char **)rval))==NULL) {
    memfree(*(char **)rval);
    *(char **)rval=NULL;
    return 1;
  }
  if ((s=nstrccat(s,'\t'))==NULL) return 1;
  if ((s=nstrcat(s,"file::fit='fit:^'${fit::oid}\n"))==NULL) return 1;
  memfree(*(char **)rval);
  *(char **)rval=s;
  return 0;
}

static int 
f2dstore(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  char *file,*base,*date,*time;
  int style;
  char *buf;
  char *argv2[2];

  memfree(*(char **)rval);
  *(char **)rval=NULL;
  _getobj(obj,"_local",inst,&f2dlocal);
  if (f2dlocal->endstore) {
    f2dlocal->endstore=FALSE;
    return 1;
  } else if (f2dlocal->storefd==NULL) {
    _getobj(obj,"file",inst,&file);
    if (file==NULL) return 1;
    style=3;
    argv2[0]=(char *)&style;
    argv2[1]=NULL;
    if (_exeobj(obj,"date",inst,1,argv2)) return 1;
    style=0;
    argv2[0]=(char *)&style;
    argv2[1]=NULL;
    if (_exeobj(obj,"time",inst,1,argv2)) return 1;
    _getobj(obj,"date",inst,&date);
    _getobj(obj,"time",inst,&time);
    if ((base=getbasename(file))==NULL) return 1;
    if ((f2dlocal->storefd=nfopen(file,"rt"))==NULL) {
      memfree(base);
      return 1;
    }
    if ((buf=memalloc(strlen(file)+50))==NULL) {
      fclose(f2dlocal->storefd);
      f2dlocal->storefd=NULL;
      memfree(base);
      return 1;
    }
    sprintf(buf,"file::load_data '%s' '%s %s' <<'[EOF]'",base,date,time);
    memfree(base);
    *(char **)rval=buf;
    return 0;
  } else {
    if (fgetline(f2dlocal->storefd,&buf)!=0) {
      fclose(f2dlocal->storefd);
      f2dlocal->storefd=NULL;
      buf = nstrdup("[EOF]\n");
      if (buf == NULL) return 1;
      f2dlocal->endstore=TRUE;
      *(char **)rval=buf;
      return 0;
    } else {
      *(char **)rval=buf;
      return 0;
    }
  }
}

static int
f2dload_sub(struct objlist *obj, char *inst, char **s, int *expand, char **fullname)
{
  struct objlist *sys;
  char *exdir, *file2, *file, *oldfile, *fname;
  int len;

  if (s == NULL)
    return 1;

  sys = getobject("system");
  if (expand)
    getobj(sys, "expand_file", 0, 0, NULL, expand);

  if ((file = getitok2(s, &len, " \t")) == NULL)
    return 1;

  getobj(sys, "expand_dir", 0, 0, NULL, &exdir);

  file2 = getfilename(CHK_STR(exdir), "/", file);
  memfree(file);

  fname = getfullpath(file2);
  if (fname == NULL) {
    memfree(file2);
    return 1;
  }
  if (fullname)
    *fullname = fname;

  memfree(file2);

  _getobj(obj, "file", inst, &oldfile);
  memfree(oldfile);

  _putobj(obj, "file", inst, fname);

  return 0;
}

static int 
f2dload(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int expand;
  char *s, *fullname, *mes;
  int mkdata;
  time_t ftime;

  s = argv[2];

  if (f2dload_sub(obj, inst, &s, &expand, &fullname))
    return 1;

  if (! expand)
    return 0;

  if (gettimeval(s, &ftime))
    return 1;

  if (access(fullname, R_OK) != 0) {
    mkdata = TRUE;
  } else {
    int len;

    len = strlen(fullname) + 256;
    mes = memalloc(len);
    if (mes == NULL)
      return 1;

    snprintf(mes, len, "`%s' Overwrite existing file?", fullname);
    mkdata = inputyn(mes);
    memfree(mes);
  }

  if (mkdata) {
    int len;
    struct utimbuf tm;
    FILE *fp;
    HANDLE fd;
    char buf[257];

    fp = nfopen(fullname, "wt");
    if (fp == NULL) {
      error2(obj, ERROPEN, fullname);
      return 1;
    }
    fd = stdinfd();
    while ((len = nread(fd, buf, sizeof(buf) - 1)) > 0) {
      buf[len] = '\0';
      fputs(buf, fp);
    }
    fclose(fp);
    tm.actime = ftime;
    tm.modtime = ftime;
    utime(fullname, &tm);
  }

  return 0;
}

static int 
f2dstoredum(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  char *file,*base,*date,*time;
  int style;
  char *buf;
  char *argv2[2];

  memfree(*(char **)rval);
  *(char **)rval=NULL;
  _getobj(obj,"_local",inst,&f2dlocal);
  if (f2dlocal->endstore) {
    f2dlocal->endstore=FALSE;
    return 1;
  } else {
    _getobj(obj,"file",inst,&file);
    if (file==NULL) return 1;
    style=3;
    argv2[0]=(char *)&style;
    argv2[1]=NULL;
    if (_exeobj(obj,"date",inst,1,argv2)) return 1;
    style=0;
    argv2[0]=(char *)&style;
    argv2[1]=NULL;
    if (_exeobj(obj,"time",inst,1,argv2)) return 1;
    _getobj(obj,"date",inst,&date);
    _getobj(obj,"time",inst,&time);
    if ((base=getbasename(file))==NULL) return 1;
    if ((buf=memalloc(strlen(file)+50))==NULL) {
      f2dlocal->storefd=NULL;
      memfree(base);
      return 1;
    }
    sprintf(buf,"file::load_dummy '%s' '%s %s'\n",base,date,time);
    memfree(base);
    *(char **)rval=buf;
    f2dlocal->endstore=TRUE;
    return 0;
  }
}

static int 
f2dloaddum(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *s = argv[2];
  return f2dload_sub(obj, inst, &s, NULL, NULL);
}

static int 
f2dtight(struct objlist *obj,char *inst,char *rval, int argc,char **argv)
{
  obj_do_tighten(obj, inst, "axis_x");
  obj_do_tighten(obj, inst, "axis_y");
  obj_do_tighten(obj, inst, "fit");

  return 0;
}

static int 
curveoutfile(struct objlist *obj,struct f2ddata *fp,FILE *fp2,
                 int intp,int div)
{
  int emerr,emserr,emnonum,emig,emng;
  int j,k,num;
  int first;
  double *x,*y,*z,*c1,*c2,*c3,*c4,*c5,*c6,count,dd,dx,dy;
  int *r,*g,*b;
  double c[8];
  double bs1[7],bs2[7],bs3[4],bs4[4];
  int bsr[7],bsg[7],bsb[7],bsr2[4],bsg2[4],bsb2[4];
  int spcond;

  emerr=emserr=emnonum=emig=emng=FALSE;
  switch (intp) {
  case INTERPOLATION_TYPE_SPLINE:
  case INTERPOLATION_TYPE_SPLINE_CLOSE:
    num=0;
    count=0;
    x=y=z=c1=c2=c3=c4=c5=c6=NULL;
    r=g=b=NULL;
    while (getdata(fp)==0) {
      if (fp->dxstat==MNOERR && fp->dystat==MNOERR) {
        if (dataadd(fp->dx,fp->dy,count,fp->colr,fp->colg,fp->colb,&num,
                   &x,&y,&z,&r,&g,&b,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) return -1;
        count++;
      } else {
        if ((fp->dxstat!=MSCONT) && (fp->dystat!=MSCONT)) {
          if (num>=2) {
            if (intp==0) spcond=SPLCND2NDDIF;
            else {
              spcond=SPLCNDPERIODIC;
              if ((x[num-1]!=x[0]) || (y[num-1]!=y[0])) {
                if (dataadd(x[0],y[0],count,r[0],g[0],b[0],&num,
                   &x,&y,&z,&r,&g,&b,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) return -1;
              }
            }
            if (spline(z,x,c1,c2,c3,num,spcond,spcond,0,0)
             || spline(z,y,c4,c5,c6,num,spcond,spcond,0,0)) {
              memfree(x);  memfree(y);  memfree(z);
              memfree(r);  memfree(g);  memfree(b);
              memfree(c1); memfree(c2); memfree(c3);
              memfree(c4); memfree(c5); memfree(c6);
              error(obj,ERRSPL);
              return -1;
            }
            fprintf(fp2,"%.15e %.15e\n",x[0],y[0]);
            for (j=0;j<num-1;j++) {
              c[0]=c1[j]; c[1]=c2[j]; c[2]=c3[j];
              c[3]=c4[j]; c[4]=c5[j]; c[5]=c6[j];
              for (k=1;k<=div;k++) {
                dd=1.0/div*k;
                splineint(dd,c,x[j],y[j],&dx,&dy,NULL);
                fprintf(fp2,"%.15e %.15e\n",dx,dy);
              }
            }
          }
          memfree(x);  memfree(y);  memfree(z);
          memfree(r);  memfree(g);  memfree(b);
          memfree(c1); memfree(c2); memfree(c3);
          memfree(c4); memfree(c5); memfree(c6);
          num=0;
          count=0;
          x=y=z=c1=c2=c3=c4=c5=c6=NULL;
          r=g=b=NULL;
        }
        errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
      }
    }
    if (num!=0) {
      if (intp==0) spcond=SPLCND2NDDIF;
      else {
        spcond=SPLCNDPERIODIC;
        if ((x[num-1]!=x[0]) || (y[num-1]!=y[0])) {
          if (dataadd(x[0],y[0],count,r[0],g[0],b[0],&num,
                  &x,&y,&z,&r,&g,&b,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) return -1;
        }
      }
      if (spline(z,x,c1,c2,c3,num,spcond,spcond,0,0)
       || spline(z,y,c4,c5,c6,num,spcond,spcond,0,0)) {
        memfree(x);  memfree(y);  memfree(z);
        memfree(r);  memfree(g);  memfree(b);
        memfree(c1); memfree(c2); memfree(c3);
        memfree(c4); memfree(c5); memfree(c6);
        error(obj,ERRSPL);
        return -1;
      }
      fprintf(fp2,"%.15e %.15e\n",x[0],y[0]);
      for (j=0;j<num-1;j++) {
        c[0]=c1[j]; c[1]=c2[j]; c[2]=c3[j];
        c[3]=c4[j]; c[4]=c5[j]; c[5]=c6[j];
        for (k=1;k<=div;k++) {
          dd=1.0/div*k;
          splineint(dd,c,x[j],y[j],&dx,&dy,NULL);
          fprintf(fp2,"%.15e %.15e\n",dx,dy);
        }
      }
    }
    memfree(x);  memfree(y);  memfree(z);
    memfree(r);  memfree(g);  memfree(b);
    memfree(c1); memfree(c2); memfree(c3);
    memfree(c4); memfree(c5); memfree(c6);
    break;
  case INTERPOLATION_TYPE_BSPLINE:
    first=TRUE;
    num=0;
    while (getdata(fp)==0) {
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
      && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
        if (first) {
          bs1[num]=fp->dx;
          bs2[num]=fp->dy;
          bsr[num]=fp->colr;
          bsg[num]=fp->colg;
          bsb[num]=fp->colb;
          num++;
          if (num>=7) {
            for (j=0;j<2;j++) {
              bspline(j+1,bs1+j,c);
              bspline(j+1,bs2+j,c+4);
              if (j==0) {
                fprintf(fp2,"%.15e %.15e\n",c[0],c[4]);
              }
              for (k=1;k<=div;k++) {
                dd=1.0/div*k;
                bsplineint(dd,c,c[0],c[4],&dx,&dy,NULL);
                fprintf(fp2,"%.15e %.15e\n",dx,dy);
              }
            }
            first=FALSE;
          }
        } else {
          for (j=1;j<7;j++) {
            bs1[j-1]=bs1[j];
            bs2[j-1]=bs2[j];
            bsr[j-1]=bsr[j];
            bsg[j-1]=bsg[j];
            bsb[j-1]=bsb[j];
          }
          bs1[6]=fp->dx;
          bs2[6]=fp->dy;
          bsr[6]=fp->colr;
          bsg[6]=fp->colg;
          bsb[6]=fp->colb;
          num++;
          bspline(0,bs1+1,c);
          bspline(0,bs2+1,c+4);
          for (k=1;k<=div;k++) {
            dd=1.0/div*k;
            bsplineint(dd,c,c[0],c[4],&dx,&dy,NULL);
            fprintf(fp2,"%.15e %.15e\n",dx,dy);
          }
        }
      } else {
        if ((fp->dxstat!=MSCONT) && (fp->dystat!=MSCONT)) {
          if (!first) {
            for (j=0;j<2;j++) {
              bspline(j+3,bs1+j+2,c);
              bspline(j+3,bs2+j+2,c+4);
              for (k=1;k<=div;k++) {
                dd=1.0/div*k;
                bsplineint(dd,c,c[0],c[4],&dx,&dy,NULL);
                fprintf(fp2,"%.15e %.15e\n",dx,dy);
              }
            }
          }
          first=TRUE;
          num=0;
        }
        errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
      }
    }
    if (!first) {
      for (j=0;j<2;j++) {
        bspline(j+3,bs1+j+2,c);
        bspline(j+3,bs2+j+2,c+4);
        for (k=1;k<=div;k++) {
          dd=1.0/div*k;
          bsplineint(dd,c,c[0],c[4],&dx,&dy,NULL);
          fprintf(fp2,"%.15e %.15e\n",dx,dy);
        }
      }
    }
    break;
  case INTERPOLATION_TYPE_BSPLINE_CLOSE:
    first=TRUE;
    num=0;
    while (getdata(fp)==0) {
      if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
      && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
        if (first) {
          bs1[num]=fp->dx;
          bs3[num]=fp->dx;
          bs2[num]=fp->dy;
          bs4[num]=fp->dy;
          bsr[num]=fp->colr;
          bsg[num]=fp->colg;
          bsb[num]=fp->colb;
          bsr2[num]=fp->colr;
          bsg2[num]=fp->colg;
          bsb2[num]=fp->colb;
          num++;
          if (num>=4) {
            bspline(0,bs1,c);
            bspline(0,bs2,c+4);
            fprintf(fp2,"%.15e %.15e\n",c[0],c[4]);
            for (k=1;k<=div;k++) {
              dd=1.0/div*k;
              bsplineint(dd,c,c[0],c[4],&dx,&dy,NULL);
              fprintf(fp2,"%.15e %.15e\n",dx,dy);
            }
            first=FALSE;
          }
        } else {
          for (j=1;j<4;j++) {
            bs1[j-1]=bs1[j];
            bs2[j-1]=bs2[j];
            bsr[j-1]=bsr[j];
            bsg[j-1]=bsg[j];
            bsb[j-1]=bsb[j];
          }
          bs1[3]=fp->dx;
          bs2[3]=fp->dy;
          bsr[3]=fp->colr;
          bsg[3]=fp->colg;
          bsb[3]=fp->colb;
          num++;
          bspline(0,bs1,c);
          bspline(0,bs2,c+4);
          for (k=1;k<=div;k++) {
            dd=1.0/div*k;
            bsplineint(dd,c,c[0],c[4],&dx,&dy,NULL);
            fprintf(fp2,"%.15e %.15e\n",dx,dy);
          }
        }
      } else {
        if ((fp->dxstat!=MSCONT) && (fp->dystat!=MSCONT)) {
          if (!first) {
            for (j=0;j<3;j++) {
              bs1[4+j]=bs3[j];
              bs2[4+j]=bs4[j];
              bsr[4+j]=bsr2[j];
              bsg[4+j]=bsg2[j];
              bsb[4+j]=bsb2[j];
              bspline(0,bs1+j+1,c);
              bspline(0,bs2+j+1,c+4);
              for (k=1;k<=div;k++) {
                dd=1.0/div*k;
                bsplineint(dd,c,c[0],c[4],&dx,&dy,NULL);
                fprintf(fp2,"%.15e %.15e\n",dx,dy);
              }
            }
          }
          first=TRUE;
          num=0;
        }
        errordisp(obj,fp,&emerr,&emserr,&emnonum,&emig,&emng);
      }
    }
    if (!first) {
      for (j=0;j<3;j++) {
        bs1[4+j]=bs3[j];
        bs2[4+j]=bs4[j];
        bsr[4+j]=bsr2[j];
        bsg[4+j]=bsg2[j];
        bsb[4+j]=bsb2[j];
        bspline(0,bs1+j+1,c);
        bspline(0,bs2+j+1,c+4);
        for (k=1;k<=div;k++) {
          dd=1.0/div*k;
          bsplineint(dd,c,c[0],c[4],&dx,&dy,NULL);
          fprintf(fp2,"%.15e %.15e\n",dx,dy);
        }
      }
    }
    break;
  }
  return 0;
}

static int 
f2doutputfile(struct objlist *obj,char *inst,char *rval,
                  int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;
  char *file, *data_file;
  int rcode,type,intp,div,append,r;
  FILE *fp2;

  _getobj(obj,"_local",inst,&f2dlocal);
  _getobj(obj,"file", inst, &data_file);

  file=(char *)argv[2];
  div=*(int *)argv[3];
  append = *(int *) argv[4];

  if (div<1) div=1;

  fp = opendata(obj,inst,f2dlocal,FALSE,FALSE);
  if (fp == NULL) {
    return 1;
  }

  if (fp->need2pass || fp->final < -1) {
    if (getminmaxdata(fp, f2dlocal)==-1) {
      closedata(fp, f2dlocal);
      error2(obj, ERRREAD, data_file);
      return 1;
    }
    reopendata(fp);
  }
  if (hskipdata(fp)!=0) {
    closedata(fp, f2dlocal);
    error2(obj, ERRREAD, data_file);
    return 1;
  }

  fp2 = nfopen(file, (append) ? "at" : "wt");
  if (fp2 == NULL) {
    //    error2(obj, ERROPEN, file);
    error22(obj, ERRUNKNOWN, strerror(errno), file);
    closedata(fp, f2dlocal);
    return 1;
  }

#if NEW_MATH_CODE
  if (set_const_all(fp))
    return 1;
#endif
  _getobj(obj,"type",inst,&type);
  if (type==3) {
    _getobj(obj,"interpolation",inst,&intp);
    if (curveoutfile(obj,fp,fp2,intp,div)!=0) {
      closedata(fp, f2dlocal);
      fclose(fp2);
      error2(obj, ERRWRITE, file);
      return 1;
    }
  } else {
    while ((rcode=getdata(fp))==0) {
      switch (fp->type) {
      case TYPE_NORMAL:
        if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR))
          fprintf(fp2,"%.15e %.15e\n",fp->dx,fp->dy);
	break;
      case TYPE_DIAGONAL:
        if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
         && (fp->d2stat==MNOERR) && (fp->d3stat==MNOERR))
          fprintf(fp2,"%.15e %.15e %.15e %.15e\n",fp->dx,fp->dy,fp->d2,fp->d3);
	break;
      case TYPE_ERR_X:
        if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
         && (fp->d2stat==MNOERR) && (fp->d3stat==MNOERR))
          fprintf(fp2,"%.15e %.15e %.15e %.15e\n",
                     fp->dx,fp->d2-fp->dx,fp->d3-fp->dx,fp->dy);
	break;
      case TYPE_ERR_Y:
        if ((fp->dxstat==MNOERR) && (fp->dystat==MNOERR)
         && (fp->d2stat==MNOERR) && (fp->d3stat==MNOERR))
          fprintf(fp2,"%.15e %.15e %.15e %.15e\n",
                     fp->dx,fp->dy,fp->d2-fp->dy,fp->d3-fp->dy);
	break;
      }
    }
  }

  r = 0;
  if (ferror(fp2)) {
    error2(obj, ERRWRITE, file);
    r = 1;
  }
  closedata(fp, f2dlocal);
  fclose(fp2);
  return 0;
}

static int 
update_field(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct f2dlocal *f2dlocal;

  _getobj(obj, "_local", inst, &f2dlocal);
  f2dlocal->mtime = 0;
  f2dlocal->mtime_stat = 0;

  return 0;
}

static int 
update_mask(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;

  update_field(obj, inst, rval, argc, argv);

  array = (struct narray *) argv[2];
  arraysort_int(array);
  arrayuniq_int(array);
  return 0;
}

static int 
foputabs(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  update_field(obj, inst, rval, argc, argv);
  return oputabs(obj, inst, rval, argc, argv);
}

static int 
foputge1(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  update_field(obj, inst, rval, argc, argv);
  return oputge1(obj, inst, rval, argc, argv);
}

static int
adjust_move_num(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct narray *move, *move_x, *move_y;
  int i, n, nx, ny;

  _getobj(obj, "move_data",   inst, &move);
  _getobj(obj, "move_data_x", inst, &move_x);
  _getobj(obj, "move_data_y", inst, &move_y);

  if (move == NULL || move_x == NULL || move_y == NULL) {
    arrayfree(move);
    arrayfree(move_x);
    arrayfree(move_y);
    move = NULL;
    _putobj(obj, "move_data",   inst, move);
    _putobj(obj, "move_data_x", inst, move);
    _putobj(obj, "move_data_y", inst, move);

    return 0;
  }

  n = arraynum(move);
  nx = arraynum(move_x);
  ny = arraynum(move_y);

  if (n == nx && n == ny)
    return 0;

  if (nx < n) {
    for (i = n - 1; i >= nx; i--) {
      arrayndel(move, i);
    }
    n = nx;
  } else if (nx > n) {
    for (i = nx - 1; i >= n; i--) {
      arrayndel(move_x, i);
    }
  }

  if (ny < n) {
    for (i = n - 1; i >= ny; i--) {
      arrayndel(move, i);
      arrayndel(move_x, i);
    }
    n = ny;
  } else if (ny > n) {
    for (i = ny - 1; i >= n; i--) {
      arrayndel(move_y, i);
    }
  }

  return 0;
}

static struct objtable file2d[] = {
  {"init",NVFUNC,NEXEC,f2dinit,NULL,0},
  {"done",NVFUNC,NEXEC,f2ddone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"file",NSTR,NREAD|NWRITE,f2dfile,NULL,0},
  {"save_path",NENUM,NREAD|NWRITE,NULL,pathchar,0},
  {"x",NINT,NREAD|NWRITE,f2dput,NULL,0},
  {"y",NINT,NREAD|NWRITE,f2dput,NULL,0},

  {"type",NENUM,NREAD|NWRITE,NULL,f2dtypechar,0},
  {"interpolation",NENUM,NREAD|NWRITE,NULL,intpchar,0},
  {"fit",NOBJ,NREAD|NWRITE,NULL,NULL,0},

  {"math_x",NSTR,NREAD|NWRITE,f2dput,NULL,0},
  {"math_y",NSTR,NREAD|NWRITE,f2dput,NULL,0},
  {"func_f",NSTR,NREAD|NWRITE,f2dput,NULL,0},
  {"func_g",NSTR,NREAD|NWRITE,f2dput,NULL,0},
  {"func_h",NSTR,NREAD|NWRITE,f2dput,NULL,0},
  {"smooth_x",NINT,NREAD|NWRITE,f2dput,NULL,0},
  {"smooth_y",NINT,NREAD|NWRITE,f2dput,NULL,0},

  {"mark_type",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"mark_size",NINT,NREAD|NWRITE,oputabs,NULL,0},

  {"line_width",NINT,NREAD|NWRITE,oputge1,NULL,0},
  {"line_style",NIARRAY,NREAD|NWRITE,oputstyle,NULL,0},
  {"line_join",NENUM,NREAD|NWRITE,NULL,joinchar,0},
  {"line_miter_limit",NINT,NREAD|NWRITE,oputge1,NULL,0},
  {"R2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"G2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"B2",NINT,NREAD|NWRITE,oputcolor,NULL,0},

  {"remark",NSTR,NREAD|NWRITE,update_field,NULL,0},
  {"ifs",NSTR,NREAD|NWRITE,update_field,NULL,0},
  {"csv",NBOOL,NREAD|NWRITE,update_field,NULL,0},
  {"head_skip",NINT,NREAD|NWRITE,foputabs,NULL,0},
  {"read_step",NINT,NREAD|NWRITE,foputge1,NULL,0},
  {"final_line",NINT,NREAD|NWRITE,f2dput,NULL,0},
  {"mask",NIARRAY,NREAD|NWRITE,update_mask,NULL,0},
  {"move_data",NIARRAY,NREAD|NWRITE,update_field,NULL,0},
  {"move_data_x",NDARRAY,NREAD|NWRITE,update_field,NULL,0},
  {"move_data_y",NDARRAY,NREAD|NWRITE,update_field,NULL,0},
  {"move_data_adjust",NVFUNC,NREAD|NEXEC,adjust_move_num,NULL,0},

  {"axis_x",NOBJ,NREAD|NWRITE,NULL,NULL,0},
  {"axis_y",NOBJ,NREAD|NWRITE,NULL,NULL,0},
  {"data_clip",NBOOL,NREAD|NWRITE,NULL,NULL,0},

  {"dnum",NSFUNC,NREAD|NEXEC,f2dstat,NULL,0},
  {"dminx",NSFUNC,NREAD|NEXEC,f2dstat,NULL,0},
  {"dmaxx",NSFUNC,NREAD|NEXEC,f2dstat,NULL,0},
  {"davx",NSFUNC,NREAD|NEXEC,f2dstat,NULL,0},
  {"dsigx",NSFUNC,NREAD|NEXEC,f2dstat,NULL,0},
  {"dminy",NSFUNC,NREAD|NEXEC,f2dstat,NULL,0},
  {"dmaxy",NSFUNC,NREAD|NEXEC,f2dstat,NULL,0},
  {"davy",NSFUNC,NREAD|NEXEC,f2dstat,NULL,0},
  {"dsigy",NSFUNC,NREAD|NEXEC,f2dstat,NULL,0},
  {"dx",NSFUNC,NREAD|NEXEC,f2dstat2,"i",0},
  {"dy",NSFUNC,NREAD|NEXEC,f2dstat2,"i",0},
  {"d2",NSFUNC,NREAD|NEXEC,f2dstat2,"i",0},
  {"d3",NSFUNC,NREAD|NEXEC,f2dstat2,"i",0},

  {"data_num",NINT,NREAD,NULL,NULL,0},
  {"data_x",NDOUBLE,NREAD,NULL,NULL,0},
  {"data_y",NDOUBLE,NREAD,NULL,NULL,0},
  {"data_2",NDOUBLE,NREAD,NULL,NULL,0},
  {"data_3",NDOUBLE,NREAD,NULL,NULL,0},
  {"coord_x",NINT,NREAD,NULL,NULL,0},
  {"coord_y",NINT,NREAD,NULL,NULL,0},
  {"coord_2",NINT,NREAD,NULL,NULL,0},
  {"coord_3",NINT,NREAD,NULL,NULL,0},
  {"stat_x",NENUM,NREAD,NULL,matherrorchar,0},
  {"stat_y",NENUM,NREAD,NULL,matherrorchar,0},
  {"stat_2",NENUM,NREAD,NULL,matherrorchar,0},
  {"stat_3",NENUM,NREAD,NULL,matherrorchar,0},
  {"minx",NDOUBLE,NREAD,NULL,NULL,0},
  {"maxx",NDOUBLE,NREAD,NULL,NULL,0},
  {"miny",NDOUBLE,NREAD,NULL,NULL,0},
  {"maxy",NDOUBLE,NREAD,NULL,NULL,0},
  {"stat_minx",NENUM,NREAD,NULL,matherrorchar,0},
  {"stat_maxx",NENUM,NREAD,NULL,matherrorchar,0},
  {"stat_miny",NENUM,NREAD,NULL,matherrorchar,0},
  {"stat_maxy",NENUM,NREAD,NULL,matherrorchar,0},
  {"line",NINT,NREAD,NULL,NULL,0},

  {"draw",NVFUNC,NREAD|NEXEC,f2ddraw,"i",0},
  {"getcoord",NIAFUNC,NREAD|NEXEC,f2dgetcoord,"dd",0},
  {"redraw",NVFUNC,NREAD|NEXEC,f2dredraw,"i",0},
  {"opendata",NVFUNC,NREAD|NEXEC,f2dopendata,NULL,0},
  {"opendatac",NVFUNC,NREAD|NEXEC,f2dopendata,NULL,0},
  {"getdata",NVFUNC,NREAD|NEXEC,f2dgetdata,NULL,0},
  {"closedata",NVFUNC,NREAD|NEXEC,f2dclosedata,NULL,0},
  {"opendata_raw",NVFUNC,NEXEC,f2dopendataraw,NULL,0},
  {"getdata_raw",NDAFUNC,NEXEC,f2dgetdataraw,"ia",0},
  {"closedata_raw",NVFUNC,NEXEC,f2dclosedataraw,NULL,0},
  {"column",NSFUNC,NREAD|NEXEC,f2dcolumn,"ii",0},
  {"basename",NSFUNC,NREAD|NEXEC,f2dbasename,NULL,0},
  {"head_lines",NSFUNC,NREAD|NEXEC,f2dhead,"i",0},
  {"boundings",NVFUNC,NREAD|NEXEC,f2dboundings,"b",0},
  {"bounding",NDAFUNC,NREAD|NEXEC,f2dbounding,"o",0},
  {"load_settings",NVFUNC,NREAD|NEXEC,f2dsettings,NULL,0},
  {"time",NSFUNC,NREAD|NEXEC,f2dtime,"i",0},
  {"date",NSFUNC,NREAD|NEXEC,f2ddate,"i",0},
  {"save",NSFUNC,NREAD|NEXEC,f2dsave,"sa",0},
  {"evaluate",NDAFUNC,NREAD|NEXEC,f2devaluate,"iiiiii",0},
  {"store_data",NSFUNC,NREAD|NEXEC,f2dstore,NULL,0},
  {"load_data",NVFUNC,NREAD|NEXEC,f2dload,"s",0},
  {"store_dummy",NSFUNC,NREAD|NEXEC,f2dstoredum,NULL,0},
  {"load_dummy",NVFUNC,NREAD|NEXEC,f2dloaddum,"s",0},
  {"tight",NVFUNC,NREAD|NEXEC,f2dtight,NULL,0},
  {"save_config",NVFUNC,NREAD|NEXEC,f2dsaveconfig,NULL,0},
  {"output_file",NVFUNC,NREAD|NEXEC,f2doutputfile,"sib",0},
  {"modified",NVFUNC,NEXEC,update_field,NULL,0},
  {"_local",NPOINTER,0,NULL,NULL,0},
};

#define TBLNUM (sizeof(file2d) / sizeof(*file2d))

void *
addfile(void)
/* addfile() returns NULL on error */
{
  unsigned int i;

  if (FileConfigHash == NULL) {
    FileConfigHash = nhash_new();
    if (FileConfigHash == NULL)
      return NULL;

    for (i = 0; i < sizeof(FileConfig) / sizeof(*FileConfig); i++) {
      if (nhash_set_ptr(FileConfigHash, FileConfig[i].name, (void *) &FileConfig[i])) {
	nhash_free(FileConfigHash);
	return NULL;
      }
    }
  }

#if ! NEW_MATH_CODE
  MathErrorCodeArray[MCNOERR] = 0;
  MathErrorCodeArray[MCSYNTAX] = ERRSYNTAX;
  MathErrorCodeArray[MCILLEGAL] = ERRILLEGAL;
  MathErrorCodeArray[MCNEST] = ERRNEST;
#endif

  return addobject(NAME,ALIAS,PARENT,OVERSION,TBLNUM,file2d,ERRNUM,f2derrorlist,NULL,NULL);
}


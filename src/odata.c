/*
 * $Id: ofile.c,v 1.113 2010-03-04 08:30:16 hito Exp $
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
#include <glib.h>
#include <unistd.h>

#include "common.h"
#include "nhash.h"
#include "ngraph.h"
#include "shell.h"
#include "osystem.h"
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
#include "odata.h"
#include "axis.h"
#include "nconfig.h"

#include "math/math_equation.h"

#define UPDATE_PROGRESS_LINE_NUM 0xfff

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
  MATH_CONST_STDEVPX,
  MATH_CONST_STDEVPY,
  MATH_CONST_STDEVX,
  MATH_CONST_STDEVY,
  TWOPASS_CONST_SIZE,
};

enum {
  MATH_CONST_MASK = TWOPASS_CONST_SIZE,
  MATH_CONST_MOVE,
  MATH_CONST_FIRST,
  MATH_CONST_COLX,
  MATH_CONST_COLY,
  MATH_CONST_AXISX,
  MATH_CONST_AXISX_MIN,
  MATH_CONST_AXISX_MAX,
  MATH_CONST_AXISX_LEN,
  MATH_CONST_AXISY,
  MATH_CONST_AXISY_MIN,
  MATH_CONST_AXISY_MAX,
  MATH_CONST_AXISY_LEN,
  MATH_CONST_HSKIP,
  MATH_CONST_RSTEP,
  MATH_CONST_FLINE,
  MATH_CONST_DATA_OBJ,
  MATH_CONST_FILE_OBJ,
  MATH_CONST_PATH_OBJ,
  MATH_CONST_RECT_OBJ,
  MATH_CONST_ARC_OBJ,
  MATH_CONST_MARK_OBJ,
  MATH_CONST_TEXT_OBJ,
  MATH_CONST_D,
  MATH_CONST_N,
  MATH_CONST_SIZE,
};

static char *FileConstant[MATH_CONST_SIZE] = {
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
  "STDEVPX",
  "STDEVPY",
  "STDEVX",
  "STDEVY",
  /* TWOPASS_CONST */



  "MASK",
  "MOVE",
  "FIRST",
  "COLX",
  "COLY",
  "AXISX",
  "AXISX_MIN",
  "AXISX_MAX",
  "AXISX_LEN",
  "AXISY",
  "AXISY_MIN",
  "AXISY_MAX",
  "AXISY_LEN",
  "HSKIP",
  "RSTEP",
  "FLINE",
  "DATA_OBJ",
  "FILE_OBJ",
  "PATH_OBJ",
  "RECT_OBJ",
  "ARC_OBJ",
  "MARK_OBJ",
  "TEXT_OBJ",
  "%D",
  "%N",
};

#define NAME		"data"
#define ALIAS		"file"
#define PARENT		"draw"
#define OVERSION	"1.00.00"
#define F2DCONF		"[data]"
#define COLUMN_ARRAY_NAME "COL"

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
#define ERR_SMALL_ARGS	122
#define ERR_INVALID_TYPE	123
#define ERR_INVALID_PARAM	124
#define ERRCONVERGE	125
#define ERR_INVALID_SOURCE	126
#define ERR_INVALID_OBJ	127
#define ERR_INVALID_RANGE	128

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
  "too small number of arguments.",
  "invalid type.",
  "invalid parameter.",
  "convergence error.",
  "invalid source.",
  "invalid object.",
  "invalid range.",
};

#define ERRNUM (sizeof(f2derrorlist) / sizeof(*f2derrorlist))

static char *data_type[]={
  N_("file"),
  N_("array"),
  N_("range"),
  NULL
};

static char *averaging_type_char[]={
  N_("simple"),
  N_("weighted"),
  N_("exponential"),
  NULL
};

static char *f2dtypechar[]={
  N_("mark"),
  N_("line"),
  N_("polygon"),
  N_("polygon_solid_fill"),
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

static int set_math_config(struct objlist *obj, N_VALUE *inst, char *field, char *str);

static struct obj_config FileConfig[] = {
  {"R",                OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"G",                OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"B",                OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"A",                OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"R2",               OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"G2",               OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"B2",               OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"A2",               OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"x",                OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"y",                OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"type",             OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"smooth_x",         OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"smooth_y",         OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
  {"averaging_type",   OBJ_CONFIG_TYPE_NUMERIC, NULL, NULL},
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
#define FIT_FIELD_PREFIX "fit_"

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

struct rgba {
  int r, g, b, a;
};

struct f2ddata_buf {
  double dx, dy, d2, d3;
  struct rgba col, col2;
  int marksize, marktype, line;
  int dxstat, dystat, d2stat, d3stat;
};

#define EQUATION_NUM 3

struct f2dlocal;

struct line_array {
  char *line;
  struct narray line_array;
};

struct f2ddata {
  struct objlist *obj;
  int id,src, GC;
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
  struct line_array line_array;
  double count;
  int eof;
  int dataclip;
  double axmin,axmax,aymin,aymax;
  double axmin2,axmax2,aymin2,aymax2;
  double axvx,axvy,ayvx,ayvy;
  int axisx, axisy;
  int axtype,aytype,axposx,axposy,ayposx,ayposy,axlen,aylen;
  double ratex,ratey;
  MathEquation *codex[EQUATION_NUM], *codey[EQUATION_NUM];
  MathValue minx, maxx, miny, maxy;
  int *const_id;
  struct rgba col, col2, color, color2, fg, bg;
  int fnumx, fnumy;
  int *needx, *needy;
  int dxstat,dystat,d2stat,d3stat;
  double dx,dy,d2,d3;
  int maxdim, use_column_array, use_column_string_array;
  int column_array_id_x, column_array_id_y, column_string_array_id_x, column_string_array_id_y;
  int need2pass;
  double sumx,sumy,sumxx,sumyy,sumxy;
  int num,datanum,prev_datanum;
  int marksize0,marksize;
  int marktype0,marktype;
  int ignore,negative;
  int msize,mtype;
  struct f2ddata_buf buf[DXBUFSIZE];
#if BUF_TYPE == USE_BUF_PTR
  struct f2ddata_buf *buf_ptr[DXBUFSIZE];
#elif BUF_TYPE == USE_RING_BUF
  int ringbuf_top;
#endif
  int bufnum,bufpo;
  int smooth,smoothx,smoothy;
  int averaging_type;
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
  struct array_prm array_data;
  double range_min, range_max;
  int range_div;
  struct f2dlocal *local;
  double text_align_h, text_align_v;
  int text_font, text_style, text_pt, text_space, text_script;
  MathExpression *end_expression;
};

struct f2dlocal {
  MathEquation *codex[EQUATION_NUM], *codey[EQUATION_NUM];
  MathValue minx, maxx, miny, maxy;
  int const_id[MATH_CONST_SIZE];
  int maxdimx,maxdimy, column_array_id_x, column_array_id_y, column_string_array_id_x, column_string_array_id_y;
  int need2passx,need2passy,total_line;
  struct f2ddata *data;
  int coord,idx,idy,id2,id3,icx,icy,ic2,ic3,isx,isy,is2,is3,iline;
  FILE *storefd;
  int endstore;
  double sumx, sumy, sumxx, sumyy, sumxy;
  double dminx, dmaxx, dminy, dmaxy, davx, davy, dstdevpx, dstdevpy, dstdevx, dstdevy;
  int num, rcode, use_drawing_func;
  time_t mtime, mtime_stat;
};

struct point_pos {
  double x, y, d;
};

struct line_position {
  int x0, y0, x1, y1;
};

static void draw_arrow(struct f2ddata *fp ,int GC, double x0, double y0, double x1, double y1, int msize, struct line_position *lp);
static int set_data_progress(struct f2ddata *fp);
static int getminmaxdata(struct f2ddata *fp, struct f2dlocal *local);
static int calc_fit_equation(struct objlist *obj, N_VALUE *inst, double x, double *y);
static void f2dtransf(double x,double y,int *gx,int *gy,void *local);
static int _f2dtransf(double x,double y,int *gx,int *gy,void *local);
static int f2drectclipf(double *x0,double *y0,double *x1,double *y1,void *local);
static int f2dlineclipf(double *x0,double *y0,double *x1,double *y1,void *local);
static int getposition(struct f2ddata *fp,double x,double y,int *gx,int *gy);
static int getposition2(struct f2ddata *fp,int axtype,int aytype,double *x,double *y);
static void set_column_array(MathEquation **code, int id, MathValue *gdata, int maxdim);
static void draw_errorbar(struct f2ddata *fp, int GC, int size, double dx0, double dy0, double  dx1, double dy1);
static void poly_add_point(struct narray *pos, double x, double y, struct f2ddata *fp);
static void poly_add_clip_point(struct narray *pos, double minx, double miny, double maxx, double maxy, double x, double y, struct f2ddata *fp);
static int poly_pos_sort_cb(const void *a, const void *b);
static void poly_set_pos(struct point_pos *p, int i, double x, double y, double x0, double y0);
static int poly_add_elements(struct narray *pos,
                             double minx, double miny, double maxx, double maxy,
                             double x0, double y0, double x1, double y1,
                             struct f2ddata *fp);
static void add_polygon_point(struct narray *pos, double x0, double y0, double x1, double y1, struct f2ddata *fp);
static void uniq_points(struct narray *pos);
static void draw_polygon(struct narray *pos, int GC, int fill);

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
      if (i > 0) {
	fp->ifs_buf[i] |= 1;
      }
    }
  }
  if (remark) {
    for (; *remark; remark++) {
      i = *remark;
      if (i > 0) {
	fp->ifs_buf[i] |= 2;
      }
    }
  }
}

#define CHECK_IFS(buf, ch) (buf[(unsigned char) ch] & 1)
#define CHECK_REMARK(remark, buf, ch) (remark && (buf[(unsigned char) ch] & 2))

#define FILE_OBJ_COLOR_COLOR 0
#define FILE_OBJ_COLOR_ALPHA 1

struct object_color_type {
  char *name;
  enum {
    COLOR_TYPE_MARK,
    COLOR_TYPE_PATH,
    COLOR_TYPE_TEXT,
  } color_type;
};

static int
line_number(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 0;
  }

  rval->val = fp->line;
  return 0;
}

static int
file_obj_color_alpha(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval, int type)
{
  struct f2ddata *fp;
  struct objlist *obj;
  int id, object_id, color_type;
  struct object_color_type obj_names[] = {
    {"data", COLOR_TYPE_MARK},
    {"path", COLOR_TYPE_PATH},
    {"rectangle", COLOR_TYPE_PATH},
    {"arc", COLOR_TYPE_PATH},
    {"mark", COLOR_TYPE_MARK},
    {"text", COLOR_TYPE_TEXT},
  };
  unsigned int i;

  *rval = exp->buf[0].val;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL ||
      exp->buf[1].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 0;
  }

  id = exp->buf[0].val.val;
  object_id = exp->buf[1].val.val;

  if (object_id == 0) {
    object_id = chkobjectid(fp->obj);
  }

  obj = NULL;
  color_type = COLOR_TYPE_TEXT;
  for (i = 0; i < sizeof(obj_names) / sizeof(*obj_names); i++) {
    obj = getobject(obj_names[i].name);
    if (obj && object_id == chkobjectid(obj)) {
      color_type = obj_names[i].color_type;
      break;
    }
    obj = NULL;
  }

  if (obj == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (chkobjinst(obj, id) == NULL) {
    return 0;
  }

  switch (color_type) {
  case COLOR_TYPE_MARK:
    if (type == FILE_OBJ_COLOR_COLOR) {
      getobj(obj, "R2", id, 0, NULL, &fp->color2.r);
      getobj(obj, "G2", id, 0, NULL, &fp->color2.g);
      getobj(obj, "B2", id, 0, NULL, &fp->color2.b);
    } else {
      getobj(obj, "A2", id, 0, NULL, &fp->color2.a);
    }
    /* fall through */
  case COLOR_TYPE_TEXT:
    if (type == FILE_OBJ_COLOR_COLOR) {
      getobj(obj, "R", id, 0, NULL, &fp->color.r);
      getobj(obj, "G", id, 0, NULL, &fp->color.g);
      getobj(obj, "B", id, 0, NULL, &fp->color.b);
    } else {
      getobj(obj, "A", id, 0, NULL, &fp->color.a);
    }
    break;
  case COLOR_TYPE_PATH:
    if (type == FILE_OBJ_COLOR_COLOR) {
      getobj(obj, "stroke_R", id, 0, NULL, &fp->color.r);
      getobj(obj, "stroke_G", id, 0, NULL, &fp->color.g);
      getobj(obj, "stroke_B", id, 0, NULL, &fp->color.b);
      getobj(obj, "fill_R", id, 0, NULL, &fp->color2.r);
      getobj(obj, "fill_G", id, 0, NULL, &fp->color2.g);
      getobj(obj, "fill_B", id, 0, NULL, &fp->color2.b);
    } else {
      getobj(obj, "stroke_A", id, 0, NULL, &fp->color.a);
      getobj(obj, "fill_A", id, 0, NULL, &fp->color2.a);
    }
    break;
  }

  fp->local->use_drawing_func = TRUE;
  return 0;
}

static int
file_objcolor(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  return file_obj_color_alpha(exp, eq, rval, FILE_OBJ_COLOR_COLOR);
}

static int
file_objalpha(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  return file_obj_color_alpha(exp, eq, rval, FILE_OBJ_COLOR_ALPHA);
}

static int
file_color(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;
  int color, val;

  *rval = exp->buf[1].val;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL ||
      exp->buf[1].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 0;
  }

  color = exp->buf[0].val.val;
  val = exp->buf[1].val.val;

#if 0
  if (val < 0 || val > 255) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
#else
  if (color < 8) {
    if (val < 0) {
      val = 0;
    } else if (val > 255) {
      val = 255;
    }
  }
#endif

  switch (color) {
  case 0:
    fp->color.r = val;
    break;
  case 1:
    fp->color.g = val;
    break;
  case 2:
    fp->color.b = val;
    break;
  case 3:
    fp->color.r = fp->color.g = fp->color.b = val;
    break;
  case 4:
    fp->color2.r = val;
    break;
  case 5:
    fp->color2.g = val;
    break;
  case 6:
    fp->color2.b = val;
    break;
  case 7:
    fp->color2.r = fp->color2.g = fp->color2.b = val;
    break;
  case 8:
    fp->color.r = ((val >> 16) & 0xff);
    fp->color.g = ((val >> 8) & 0xff);
    fp->color.b = (val & 0xff);
    break;
  case 9:
    fp->color2.r = ((val >> 16) & 0xff);
    fp->color2.g = ((val >> 8) & 0xff);
    fp->color2.b = (val & 0xff);
    break;
  default:
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  fp->local->use_drawing_func = TRUE;
  return 0;
}

static int
file_alpha(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;
  int alpha, color;

  *rval = exp->buf[0].val;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 0;
  }

  alpha = exp->buf[0].val.val;

#if 0
  if (alpha < 0 || alpha > 255) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
#else
  if (alpha < 0) {
    alpha = 0;
  } else if (alpha > 255) {
    alpha = 255;
  }
#endif

  color = exp->buf[1].val.val;
  switch (color) {
  case 1:
    fp->color.a = alpha;
    break;
  case 2:
    fp->color2.a = alpha;
    break;
  default:
    fp->color.a = alpha;
    fp->color2.a = alpha;
  }

  fp->local->use_drawing_func = TRUE;
  return 0;
}

static int
file_rgb_sub(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval, int color2)
{
  struct f2ddata *fp;
  int r, g, b;

  *rval = exp->buf[2].val;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL ||
      exp->buf[1].val.type != MATH_VALUE_NORMAL ||
      exp->buf[2].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 0;
  }

  r = exp->buf[0].val.val * 255;
  g = exp->buf[1].val.val * 255;
  b = exp->buf[2].val.val * 255;

#if 0
  if (r < 0 || r > 255 ||
      g < 0 || g > 255 ||
      b < 0 || b > 255) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
#else
  if (r < 0) {
    r = 0;
  } else if (r > 255) {
    r = 255;
  }

  if (g < 0) {
    g = 0;
  } else if (g > 255) {
    g = 255;
  }

  if (b < 0) {
    b = 0;
  } else if (b > 255) {
    b = 255;
  }
#endif

  if (color2) {
    fp->color2.r = r;
    fp->color2.g = g;
    fp->color2.b = b;
  } else {
    fp->color.r = r;
    fp->color.g = g;
    fp->color.b = b;
  }

  fp->local->use_drawing_func = TRUE;
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

  *rval = exp->buf[2].val;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL ||
      exp->buf[1].val.type != MATH_VALUE_NORMAL ||
      exp->buf[2].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 0;
  }

  h = exp->buf[0].val.val;
  s = exp->buf[1].val.val;
  b = exp->buf[2].val.val;

#if 0
  if (h < 0 || h > 1 ||
      s < 0 || s > 1 ||
      b < 0 || b > 1) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
#else
  if (h < 0) {
    h = 0;
  } else if (h > 1) {
    h = 1;
  }

  if (s < 0) {
    s = 0;
  } else if (s > 1) {
    s = 1;
  }

  if (b < 0) {
    b = 0;
  } else if (b > 1) {
    b = 1;
  }
#endif

  HSB2RGB(h, s, b, &r, &g, &bb);

  if (color2) {
    fp->color2.r = r;
    fp->color2.g = g;
    fp->color2.b = bb;
  } else {
    fp->color.r = r;
    fp->color.g = g;
    fp->color.b = bb;
  }

  fp->local->use_drawing_func = TRUE;
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

  *rval = exp->buf[0].val;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 0;
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

  *rval = exp->buf[0].val;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 0;
  }

  type = exp->buf[0].val.val;
  if (type < 0 || type >= MARK_TYPE_NUM) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  fp->marktype = type;

  return 0;
}

static int
file_fit_calc(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int file_id, r;
  double x, y;
  static struct objlist *file_obj = NULL;

  rval->val = 0;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL ||
      exp->buf[1].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  if (file_obj == NULL) {
    file_obj = getobject("data");
  }

  if (file_obj == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  file_id = exp->buf[0].val.val;
  x = exp->buf[1].val.val;

  r = ofile_calc_fit_equation(file_obj, file_id, x, &y);
  if (r) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = y;
  rval->type = MATH_VALUE_NORMAL;

  return 0;
}

static int
file_fit_prm(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int file_id, prm, r;
  char *argv[2], *ptr;
  struct savedstdio save;
  static struct objlist *file_obj = NULL;

  rval->val = 0;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL ||
      exp->buf[1].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  if (file_obj == NULL) {
    file_obj = getobject("data");
  }

  if (file_obj == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  file_id = exp->buf[0].val.val;
  prm = exp->buf[1].val.val;

  argv[0] = (char *) &prm;
  argv[1] = NULL;
  ignorestdio(&save);
  r = getobj(file_obj, "fit_prm", file_id, 1, argv, &ptr);
  restorestdio(&save);
  if (r < 0 || ptr == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  rval->val = atof(ptr);
  rval->type = MATH_VALUE_NORMAL;

  return 0;
}

#define ARC_INTERPOLATION 20
#define DRAW_ARC_ARG_NUM 10

static int
file_draw_arc(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;
  int i, num, stroke, fill, pie, close, cx, cy, ap[ARC_INTERPOLATION * 2], *pdata, px, py;
  double x, y, rx, ry, angle1, angle2;
  struct narray expand_points;

  rval->val = 0;
  for (i = 0; i < DRAW_ARC_ARG_NUM; i++) {
    if (exp->buf[i].val.type != MATH_VALUE_NORMAL) {
      return 0;
    }
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (fp->GC < 0) {
    return 0;
  }

  x      = exp->buf[0].val.val;
  y      = exp->buf[1].val.val;
  rx     = exp->buf[2].val.val;
  ry     = exp->buf[3].val.val;
  angle1 = exp->buf[4].val.val;
  angle2 = exp->buf[5].val.val;
  pie    = exp->buf[6].val.val;
  stroke = exp->buf[7].val.val;
  fill   = exp->buf[8].val.val;
  close  = exp->buf[9].val.val;

  if (getposition(fp, x, y, &cx, &cy)) {
    return 0;
  }

  angle2 = fmod(angle2, 360);
  if (angle2 == 0.0) {
    angle2 = 360;
    close = TRUE;
  }
  for (i = 0; i < ARC_INTERPOLATION; i++) {
    int r;
    double ax, ay, angle;
    angle = angle1 + angle2 / (ARC_INTERPOLATION - 1) * i;
    angle = MPI * angle / 180.0;
    ax = x + rx * cos(angle);
    ay = y + ry * sin(angle);
    r = getposition2(fp, fp->axtype, fp->aytype, &ax, &ay);
    if (r) {
      rval->type = MATH_VALUE_ERROR;
      return -1;
    }
    f2dtransf(ax, ay,
	      ap + i * 2,
	      ap + i * 2 + 1,
	      fp);
  }
  arrayinit(&expand_points, sizeof(int));
  if (curve_expand_points(ap, ARC_INTERPOLATION, INTERPOLATION_TYPE_SPLINE, &expand_points)) {
    arraydel(&expand_points);
    return 1;
  }
  if (pie) {
    arrayadd(&expand_points, &cx);
    arrayadd(&expand_points, &cy);
  }
  num = arraynum(&expand_points) / 2;
  pdata = arraydata(&expand_points);
  GRAcurrent_point(fp->GC, &px, &py);
  if (fill) {
    GRAcolor(fp->GC, fp->color2.r, fp->color2.g, fp->color2.b, fp->color2.a);
    GRAdrawpoly(fp->GC, num, pdata, GRA_FILL_MODE_EVEN_ODD);
  }
  if (stroke) {
    GRAcolor(fp->GC, fp->color.r, fp->color.g, fp->color.b, fp->color.a);
    if (close) {
      GRAdrawpoly(fp->GC, num, pdata, GRA_FILL_MODE_NONE);
    } else {
      int n;
      n = (pie) ? num - 1 : num;
      GRAmoveto(fp->GC, pdata[0], pdata[1]);
      for (i = 1; i < n; i++) {
	GRAlineto(fp->GC, pdata[i * 2], pdata[i * 2 + 1]);
      }
    }
  }
  GRAmoveto(fp->GC, px, py);
  arraydel(&expand_points);
  fp->local->use_drawing_func = TRUE;
  return 0;
}

static int
file_draw_rect(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;
  int i, stroke, fill, ap[8], px, py;
  double pos[4];

  rval->val = 0;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL ||
      exp->buf[1].val.type != MATH_VALUE_NORMAL ||
      exp->buf[2].val.type != MATH_VALUE_NORMAL ||
      exp->buf[3].val.type != MATH_VALUE_NORMAL ||
      exp->buf[4].val.type != MATH_VALUE_NORMAL ||
      exp->buf[5].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (fp->GC < 0) {
    return 0;
  }

  for (i = 0; i < 4; i++) {
    pos[i] = exp->buf[i].val.val;
  }
  pos[2] += pos[0];
  pos[3] += pos[1];
  stroke = exp->buf[4].val.val;
  fill = exp->buf[5].val.val;

  for (i = 0; i < 2; i++) {
    int r;
    r = getposition2(fp, fp->axtype, fp->aytype, pos + i * 2, pos + i * 2 + 1);
    if (r) {
      rval->type = MATH_VALUE_ERROR;
      return -1;
    }
  }
  if (f2drectclipf(pos, pos + 1, pos + 2, pos + 3, fp)) {
    return 0;
  }
  f2dtransf(pos[0], pos[1], ap + 0, ap + 1, fp);
  f2dtransf(pos[0], pos[3], ap + 2, ap + 3, fp);
  f2dtransf(pos[2], pos[3], ap + 4, ap + 5, fp);
  f2dtransf(pos[2], pos[1], ap + 6, ap + 7, fp);

  GRAcurrent_point(fp->GC, &px, &py);
  if (fill) {
    GRAcolor(fp->GC, fp->color2.r, fp->color2.g, fp->color2.b, fp->color2.a);
    GRAdrawpoly(fp->GC, 4, ap, GRA_FILL_MODE_EVEN_ODD);
  }
  if (stroke) {
    GRAcolor(fp->GC, fp->color.r, fp->color.g, fp->color.b, fp->color.a);
    GRAdrawpoly(fp->GC, 4, ap, GRA_FILL_MODE_NONE);
  }
  GRAmoveto(fp->GC, px, py);
  fp->local->use_drawing_func = TRUE;
  return 0;
}

static int
file_draw_line(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;
  int i, px, py, msize, arrow;
  double pos[4];
  struct line_position lp1, lp2;

  rval->val = 0;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL ||
      exp->buf[1].val.type != MATH_VALUE_NORMAL ||
      exp->buf[2].val.type != MATH_VALUE_NORMAL ||
      exp->buf[3].val.type != MATH_VALUE_NORMAL ||
      exp->buf[4].val.type != MATH_VALUE_NORMAL ||
      exp->buf[5].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (fp->GC < 0) {
    return 0;
  }

  for (i = 0; i < 4; i++) {
    pos[i] = exp->buf[i].val.val;
  }
  arrow = exp->buf[4].val.val;
  msize = exp->buf[5].val.val * 100;
  if (msize <= 0) {
    msize = fp->marksize;
  }

  GRAcurrent_point(fp->GC, &px, &py);
  GRAcolor(fp->GC, fp->color.r, fp->color.g, fp->color.b, fp->color.a);
  switch (arrow) {
  case ARROW_POSITION_END:
    draw_arrow(fp, fp->GC, pos[0], pos[1], pos[2], pos[3], msize, &lp1);
    GRAline(fp->GC, lp1.x0, lp1.y0, lp1.x1, lp1.y1);
    break;
  case ARROW_POSITION_BEGIN:
    draw_arrow(fp, fp->GC, pos[2], pos[3], pos[0], pos[1], msize, &lp1);
    GRAline(fp->GC, lp1.x1, lp1.y1, lp1.x0, lp1.y0);
    break;
  case ARROW_POSITION_BOTH:
    draw_arrow(fp, fp->GC, pos[0], pos[1], pos[2], pos[3], msize, &lp1);
    draw_arrow(fp, fp->GC, pos[2], pos[3], pos[0], pos[1], msize, &lp2);
    GRAline(fp->GC, lp2.x1, lp2.y1, lp1.x1, lp1.y1);
    break;
  default:
    draw_arrow(fp, fp->GC, pos[0], pos[1], pos[2], pos[3], 0, &lp1);
    GRAline(fp->GC, lp1.x0, lp1.y0, lp1.x1, lp1.y1);
    break;
  }
  GRAmoveto(fp->GC, px, py);
  fp->local->use_drawing_func = TRUE;

  return 0;
}

static int
file_draw_errorbar(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;
  int px, py;
  double x, y, erx, ery, size;

  rval->val = 0;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL ||
      exp->buf[1].val.type != MATH_VALUE_NORMAL ||
      exp->buf[2].val.type != MATH_VALUE_NORMAL ||
      exp->buf[3].val.type != MATH_VALUE_NORMAL ||
      exp->buf[4].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (fp->GC < 0) {
    return 0;
  }

  x = exp->buf[0].val.val;
  y = exp->buf[1].val.val;
  erx = exp->buf[2].val.val;
  ery = exp->buf[3].val.val;
  size = exp->buf[4].val.val * 100;
  if (size < 1) {
    size = fp->marksize;
  }

  GRAcurrent_point(fp->GC, &px, &py);
  GRAcolor(fp->GC, fp->color.r, fp->color.g, fp->color.b, fp->color.a);
  if (erx != 0) {
    draw_errorbar(fp, fp->GC, size / 2, x - erx, y, x + erx, y);
  }
  if (ery != 0) {
    draw_errorbar(fp, fp->GC, fp->marksize / 2, x, y - ery, x, y + ery);
  }
  GRAmoveto(fp->GC, px, py);
  fp->local->use_drawing_func = TRUE;

  return 0;
}

static int
file_draw_errorbar2(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;
  int px, py, size;
  double x0, y0, x1, y1;

  rval->val = 0;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL ||
      exp->buf[1].val.type != MATH_VALUE_NORMAL ||
      exp->buf[2].val.type != MATH_VALUE_NORMAL ||
      exp->buf[3].val.type != MATH_VALUE_NORMAL ||
      exp->buf[4].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (fp->GC < 0) {
    return 0;
  }

  x0 = exp->buf[0].val.val;
  y0 = exp->buf[1].val.val;
  x1 = exp->buf[2].val.val;
  y1 = exp->buf[3].val.val;
  size = exp->buf[4].val.val * 100;
  if (size < 1) {
    size = fp->marksize;
  }

  GRAcurrent_point(fp->GC, &px, &py);
  GRAcolor(fp->GC, fp->color.r, fp->color.g, fp->color.b, fp->color.a);
  draw_errorbar(fp, fp->GC, size / 2, x0, y0, x1, y1);
  GRAmoveto(fp->GC, px, py);
  fp->local->use_drawing_func = TRUE;

  return 0;
}

static int
file_draw_mark(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;
  int cx, cy, size;
  double x, y;

  rval->val = 0;
  if (exp->buf[0].val.type != MATH_VALUE_NORMAL ||
      exp->buf[1].val.type != MATH_VALUE_NORMAL ||
      exp->buf[2].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (fp->GC < 0) {
    return 0;
  }

  x    = exp->buf[0].val.val;
  y    = exp->buf[1].val.val;
  size = exp->buf[2].val.val * 100;

  if (size <= 0) {
    size = fp->marksize;
  }

  if (getposition(fp, x, y, &cx, &cy)) {
    return 0;
  }
  if (size > 0) {
    int px, py;
    GRAcurrent_point(fp->GC, &px, &py);
    GRAmark(fp->GC, fp->marktype, cx, cy, size,
            fp->color.r, fp->color.g, fp->color.b, fp->color.a,
            fp->color2.r, fp->color2.g, fp->color2.b, fp->color2.a);
    GRAmoveto(fp->GC, px, py);
    fp->local->use_drawing_func = TRUE;
  }
  return 0;
}

static void
draw_lines(struct narray *pos, int GC)
{
  int n, *ap;

  uniq_points(pos);
  ap = (int *) arraydata(pos);
  n = arraynum(pos);
  if (n > 3) {
    GRAlines(GC, n / 2, ap);
  }
}

struct LineStyleInfo {
  int num, *type, width, miter;
  enum GRA_LINE_CAP cap;
  enum GRA_LINE_JOIN join;
};

static void
save_line_style(int GC, struct LineStyleInfo *info)
{
  GRA_get_linestyle(GC, &info->num, &info->type, &info->width, &info->cap, &info->join, &info->miter);
}

static void
restore_line_style(int GC, struct LineStyleInfo *info)
{
  GRAlinestyle(GC, info->num, info->type, info->width, info->cap, info->join, info->miter);
  if (info->type) {
    g_free(info->type);
    info->type = NULL;
  }
}

static void
set_line_style(struct f2ddata *fp)
{
  int width, join, miter, n, *type;
  struct narray *style;

  getobj(fp->obj, "line_width", fp->id, 0, NULL, &width);
  getobj(fp->obj, "line_style", fp->id, 0, NULL, &style);
  getobj(fp->obj, "line_join", fp->id, 0, NULL, &join);
  getobj(fp->obj, "line_miter_limit", fp->id, 0, NULL, &miter);
  n = arraynum(style);
  type = arraydata(style);
  GRAlinestyle(fp->GC, n, type, width, GRA_LINE_CAP_BUTT, join, miter);
}

static int
file_draw_path(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval, int stroke, int fill, int close)
{
  struct f2ddata *fp;
  int i, id, n, first, px, py;
  double x0, y0, x1, y1, x2, y2;
  MathEquationArray *ax, *ay;
  struct narray pos;
  struct LineStyleInfo info;

  rval->val = 0;

  id = exp->buf[0].array.idx;
  ax = math_equation_get_array(eq, id);
  if (ax == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  n = ax->num;

  id = exp->buf[1].array.idx;
  ay = math_equation_get_array(eq, id);
  if (ay == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  if (ay->num < n) {
    n = ay->num;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (fp->GC < 0) {
    return 0;
  }

  save_line_style(fp->GC, &info);
  set_line_style(fp);
  arrayinit(&pos, sizeof(int));
  first = TRUE;
  for (i = 0; i < n; i++) {
    if (ax->data.val[i].type == MATH_VALUE_NORMAL && ay->data.val[i].type == MATH_VALUE_NORMAL) {
      if (first) {
        first = FALSE;
	x0 = ax->data.val[i].val;
	y0 = ay->data.val[i].val;
	x2 = ax->data.val[i].val;
	y2 = ay->data.val[i].val;
      } else {
	x1 = x2;
	y1 = y2;
	x2 = ax->data.val[i].val;
	y2 = ay->data.val[i].val;
	add_polygon_point(&pos, x1, y1, x2, y2, fp);
      }
    }
  }
  if (first) {
    goto End;
  }
  if (close) {
    add_polygon_point(&pos, x2, y2, x0, y0, fp);
  }

  if (fill < 0) {
    fill = GRA_FILL_MODE_NONE;
  } else if (fill > GRA_FILL_MODE_WINDING) {
    fill = GRA_FILL_MODE_WINDING;
  }

  GRAcurrent_point(fp->GC, &px, &py);
  GRAcolor(fp->GC, fp->color2.r, fp->color2.g, fp->color2.b, fp->color2.a);
  if (fill) {
    draw_polygon(&pos, fp->GC, fill);
  }
  GRAcolor(fp->GC, fp->color.r, fp->color.g, fp->color.b, fp->color.a);
  if (stroke) {
    if (close) {
      draw_polygon(&pos, fp->GC, GRA_FILL_MODE_NONE);
    } else {
      draw_lines(&pos, fp->GC);
    }
  }
  GRAmoveto(fp->GC, px, py);
  arraydel(&pos);
  fp->local->use_drawing_func = TRUE;

 End:
  restore_line_style(fp->GC, &info);
  return 0;
}

static int
file_draw_polyline(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  return file_draw_path(exp, eq, rval, TRUE, FALSE, FALSE);
}

static int
file_draw_polygon(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int stroke, fill;
  if (exp->buf[2].val.type != MATH_VALUE_NORMAL ||
      exp->buf[3].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }
  stroke = exp->buf[2].val.val;
  fill   = exp->buf[3].val.val;
  return file_draw_path(exp, eq, rval, stroke, fill, TRUE);
}

static int
file_text_align(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL ||
      exp->buf[1].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }
  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  fp->text_align_h = exp->buf[0].val.val;
  if (fp->text_align_h < 0) {
    fp->text_align_h = 0;
  } else if (fp->text_align_h > 1) {
    fp->text_align_h = 1;
  }
  fp->text_align_v = exp->buf[1].val.val;
  if (fp->text_align_v < 0) {
    fp->text_align_v = 0;
  } else if (fp->text_align_v > 1) {
    fp->text_align_v = 1;
  }
  return 0;
}

static int
file_text_font(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }
  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  fp->text_font   = exp->buf[0].val.val;
  if (fp->text_font <= 0) {
    fp->text_font = 0;
  }
  return 0;
}

static int
file_text_style(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }
  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  fp->text_style  = exp->buf[0].val.val;
  if (fp->text_style < 0) {
    fp->text_style = 0;
  } else if (fp->text_style > GRA_FONT_STYLE_MAX) {
    fp->text_style = GRA_FONT_STYLE_MAX;
  }
  return 0;
}

static int
file_text_size(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  struct f2ddata *fp;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL ||
      exp->buf[1].val.type != MATH_VALUE_NORMAL ||
      exp->buf[2].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }
  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  fp->text_pt     = exp->buf[0].val.val * 100;
  fp->text_space  = exp->buf[1].val.val * 100;
  fp->text_script = exp->buf[2].val.val * 100;
  if (fp->text_pt <= 0) {
    fp->text_pt = DEFAULT_FONT_PT;
  }
  if (fp->text_script <= 0) {
    fp->text_script = DEFAULT_SCRIPT_SIZE;
  } else if (fp->text_script < SCRIPT_SIZE_MIN) {
    fp->text_script = SCRIPT_SIZE_MIN;
  } else if (fp->text_script > SCRIPT_SIZE_MAX) {
    fp->text_script = SCRIPT_SIZE_MAX;
  }
  return 0;
}

static int
file_draw_text_sub(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval, int raw)
{
  double x, y, si, co, rdir, h_shift, v_shift;
  int font_id, pt, space, dir, script, style;
  int px, py, cx, cy;
  int w, h, bbox[4];
  struct f2ddata *fp;
  char *str, *font;

  if (exp->buf[1].val.type != MATH_VALUE_NORMAL ||
      exp->buf[2].val.type != MATH_VALUE_NORMAL ||
      exp->buf[3].val.type != MATH_VALUE_NORMAL ||
      exp->buf[4].val.type != MATH_VALUE_NORMAL ||
      exp->buf[5].val.type != MATH_VALUE_NORMAL ||
      exp->buf[6].val.type != MATH_VALUE_NORMAL ||
      exp->buf[7].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }
  if (! raw && exp->buf[8].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }
  x       = exp->buf[1].val.val;
  y       = exp->buf[2].val.val;
  dir     = exp->buf[3].val.val * 100;
  pt      = exp->buf[4].val.val * 100;
  font_id = exp->buf[5].val.val;
  style   = exp->buf[6].val.val;
  space   = exp->buf[7].val.val * 100;
  script  = 0;
  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  if (! raw) {
    script  = exp->buf[8].val.val * 100;
  }
  if (font_id <= 0) {
    font_id = fp->text_font;
  }
  if (pt <= 0) {
    pt = fp->text_pt;
  }
  if (script <= 0) {
    script = fp->text_script;
  } else if (script < SCRIPT_SIZE_MIN) {
    script = SCRIPT_SIZE_MIN;
  } else if (script > SCRIPT_SIZE_MAX) {
    script = SCRIPT_SIZE_MAX;
  }
  if (style <= 0) {
    style = fp->text_style;
  } else if (style > GRA_FONT_STYLE_MAX) {
    style = GRA_FONT_STYLE_MAX;
  }

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  str = (char *) math_expression_get_string_from_argument(exp, 0);
  if (str == NULL) {
    return 0;
  }

  if (fp->GC < 0) {
    return 0;
  }
  if (getposition(fp, x, y, &cx, &cy)) {
    return 0;
  }
  switch (font_id) {
  case 1:
    font = "Serif";
    break;
  case 2:
    font = "Monospace";
    break;
  default:
    font = "Sans-serif";
    break;
  }
  text_get_bbox(0, 0, str, font, style, pt, 0, space, script, raw, bbox);
  w = bbox[2] - bbox[0];
  h = bbox[3] - bbox[1];
  rdir = dir / 18000.0 * MPI;
  si = sin(rdir);
  co = cos(rdir);
  h_shift = bbox[0] + w * fp->text_align_h;
  v_shift = bbox[1] + h * (1 - fp->text_align_v);
  cx = cx - h_shift * co - v_shift * si;
  cy = cy + h_shift * si - v_shift * co;
  GRAcurrent_point(fp->GC, &px, &py);
  GRAcolor(fp->GC, fp->color.r, fp->color.g, fp->color.b, fp->color.a);
  GRAmoveto(fp->GC, cx, cy);
  if (raw) {
    GRAdrawtextraw(fp->GC, str, font, style, pt, space, dir);
  } else {
    GRAdrawtext(fp->GC, str, font, style, pt, space, dir, script);
  }
  GRAmoveto(fp->GC, px, py);
  fp->local->use_drawing_func = TRUE;
  return 0;
}

static int
file_draw_text(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  return file_draw_text_sub(exp, eq, rval, FALSE);
}

static int
file_draw_text_raw(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  return file_draw_text_sub(exp, eq, rval, TRUE);
}

static int
file_on_end(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  MathExpression *end_exp;
  struct f2ddata *fp;

  end_exp = exp->buf[0].exp;
  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }
  fp ->end_expression = end_exp;
  return 0;
}

static int
file_text_obj_set(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id;
  const char *str;
  char *tmp;
  struct objlist *text_obj;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }
  id = exp->buf[0].val.val;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  str = math_expression_get_string_from_argument(exp, 1);
  if (str == NULL) {
    return 0;
  }

  text_obj = getobject("text");
  if (text_obj == NULL) {
    return 0;
  }
  tmp = g_strdup(str);
  if (tmp == NULL) {
    return 0;
  }
  putobj(text_obj, "text", id, tmp);
  return 0;
}

static int
file_text_obj_get(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int id;
  char *str;
  struct objlist *text_obj;
  GString *gstr;

  if (exp->buf[0].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }
  id = exp->buf[0].val.val;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  gstr = math_expression_get_string_variable_from_argument(exp, 1);
  if (gstr == NULL) {
    return 0;
  }

  text_obj = getobject("text");
  if (text_obj == NULL) {
    return 0;
  }
  getobj(text_obj, "text", id, 0, NULL, &str);
  if (str == NULL) {
    return 0;
  }
  g_string_assign(gstr, str);
  return 0;
}

static int
file_string_column(MathFunctionCallExpression *exp, MathEquation *eq, MathValue *rval)
{
  int n, col;
  struct f2ddata *fp;
  GString *str;
  struct narray *array;

  if (exp->buf[1].val.type != MATH_VALUE_NORMAL) {
    return 0;
  }
  col = exp->buf[1].val.val;

  rval->val = 0;
  rval->type = MATH_VALUE_NORMAL;

  str = math_expression_get_string_variable_from_argument(exp, 0);
  if (str == NULL) {
    return 1;
  }

  fp = math_equation_get_user_data(eq);
  if (fp == NULL) {
    rval->type = MATH_VALUE_ERROR;
    return 1;
  }

  if (fp->line_array.line == NULL) {
    return 1;
  }
  if (col == 0) {
    g_string_assign(str, fp->line_array.line);
    return 0;
  }
  array = &(fp->line_array.line_array);
  n = arraynum(array);
  if (n < 1) {
    parse_data_line(array, fp->line_array.line, fp->ifs, fp->csv);
    n = arraynum(array);
  }
  if (col < 0) {
    col += n + 1;
  }
  if (n < 1 || col > n || col < 0) {
    return 0;
  }
  g_string_assign(str, arraynget_str(array, col - 1));
  return 0;
}

struct funcs {
  char *name;
  struct math_function_parameter prm;
};

static struct funcs BasicFunc[] = {
  {"LINE_NUMBER", {0, 0, 0, line_number, NULL, NULL, NULL, NULL}},
};

static struct funcs FitFunc[] = {
  {"FIT_CALC", {2, 0, 0, file_fit_calc,  NULL, NULL, NULL, NULL}},
  {"FIT_PRM",  {2, 0, 0, file_fit_prm,   NULL, NULL, NULL, NULL}},
};

static enum MATH_FUNCTION_ARG_TYPE draw_polyline_arg_type[] = {
  MATH_FUNCTION_ARG_TYPE_ARRAY,
  MATH_FUNCTION_ARG_TYPE_ARRAY,
};

static enum MATH_FUNCTION_ARG_TYPE draw_polygon_arg_type[] = {
  MATH_FUNCTION_ARG_TYPE_ARRAY,
  MATH_FUNCTION_ARG_TYPE_ARRAY,
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
};

static enum MATH_FUNCTION_ARG_TYPE draw_text_arg_type[] = {
  MATH_FUNCTION_ARG_TYPE_STRING,
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
};

static enum MATH_FUNCTION_ARG_TYPE text_obj_set_arg_type[] = {
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_STRING,
};

static enum MATH_FUNCTION_ARG_TYPE text_obj_get_arg_type[] = {
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
  MATH_FUNCTION_ARG_TYPE_STRING_VARIABLE,
};

static enum MATH_FUNCTION_ARG_TYPE string_column_arg_type[] = {
  MATH_FUNCTION_ARG_TYPE_STRING_VARIABLE,
  MATH_FUNCTION_ARG_TYPE_DOUBLE,
};

static enum MATH_FUNCTION_ARG_TYPE on_end_arg_type[] = {
  MATH_FUNCTION_ARG_TYPE_PROC,
};

static struct funcs FileFunc[] = {
  {"OBJ_ALPHA", {2, 1, MATH_FUNCTION_TYPE_NORMAL, file_objalpha, NULL, NULL, NULL, NULL}},
  {"OBJ_COLOR", {2, 1, MATH_FUNCTION_TYPE_NORMAL, file_objcolor, NULL, NULL, NULL, NULL}},
  {"COLOR",     {2, 1, MATH_FUNCTION_TYPE_NORMAL, file_color,    NULL, NULL, NULL, NULL}},
  {"ALPHA",     {2, 1, MATH_FUNCTION_TYPE_NORMAL, file_alpha,    NULL, NULL, NULL, NULL}},
  {"RGB",       {3, 1, MATH_FUNCTION_TYPE_NORMAL, file_rgb,      NULL, NULL, NULL, NULL}},
  {"RGB2",      {3, 1, MATH_FUNCTION_TYPE_NORMAL, file_rgb2,     NULL, NULL, NULL, NULL}},
  {"HSB",       {3, 1, MATH_FUNCTION_TYPE_NORMAL, file_hsb,      NULL, NULL, NULL, NULL}},
  {"HSB2",      {3, 1, MATH_FUNCTION_TYPE_NORMAL, file_hsb2,     NULL, NULL, NULL, NULL}},
  {"MARKSIZE",  {1, 1, MATH_FUNCTION_TYPE_NORMAL, file_marksize, NULL, NULL, NULL, NULL}},
  {"MARKTYPE",  {1, 1, MATH_FUNCTION_TYPE_NORMAL, file_marktype, NULL, NULL, NULL, NULL}},
  {"DRAW_RECT", {6, 1, MATH_FUNCTION_TYPE_NORMAL, file_draw_rect, NULL, NULL, NULL, NULL}},
  {"DRAW_LINE", {6, 1, MATH_FUNCTION_TYPE_NORMAL, file_draw_line, NULL, NULL, NULL, NULL}},
  {"DRAW_ARC",  {DRAW_ARC_ARG_NUM, 1, MATH_FUNCTION_TYPE_NORMAL, file_draw_arc, NULL, NULL, NULL, NULL}},
  {"DRAW_MARK",      {3, 1, MATH_FUNCTION_TYPE_NORMAL, file_draw_mark, NULL, NULL, NULL, NULL}},
  {"DRAW_ERRORBAR",  {5, 1, MATH_FUNCTION_TYPE_NORMAL, file_draw_errorbar, NULL, NULL, NULL, NULL}},
  {"DRAW_ERRORBAR2", {5, 1, MATH_FUNCTION_TYPE_NORMAL, file_draw_errorbar2, NULL, NULL, NULL, NULL}},
  {"DRAW_POLYLINE",  {2, 1, MATH_FUNCTION_TYPE_NORMAL, file_draw_polyline, draw_polyline_arg_type, NULL, NULL, NULL}},
  {"DRAW_POLYGON",   {4, 1, MATH_FUNCTION_TYPE_NORMAL, file_draw_polygon, draw_polygon_arg_type, NULL, NULL, NULL}},
  {"DRAW_TEXT",      {9, 1, MATH_FUNCTION_TYPE_NORMAL, file_draw_text, draw_text_arg_type, NULL, NULL, NULL}},
  {"DRAW_TEXT_RAW",  {8, 1, MATH_FUNCTION_TYPE_NORMAL, file_draw_text_raw, draw_text_arg_type, NULL, NULL, NULL}},
  {"TEXT_ALIGN",     {2, 1, MATH_FUNCTION_TYPE_NORMAL, file_text_align, NULL, NULL, NULL, NULL}},
  {"TEXT_FONT",      {1, 1, MATH_FUNCTION_TYPE_NORMAL, file_text_font, NULL, NULL, NULL, NULL}},
  {"TEXT_STYLE",     {1, 1, MATH_FUNCTION_TYPE_NORMAL, file_text_style, NULL, NULL, NULL, NULL}},
  {"TEXT_SIZE",      {3, 1, MATH_FUNCTION_TYPE_NORMAL, file_text_size, NULL, NULL, NULL, NULL}},
  {"TEXT_OBJ_SET",   {2, 1, MATH_FUNCTION_TYPE_NORMAL, file_text_obj_set, text_obj_set_arg_type, NULL, NULL, NULL}},
  {"TEXT_OBJ_GET",   {2, 1, MATH_FUNCTION_TYPE_NORMAL, file_text_obj_get, text_obj_get_arg_type, NULL, NULL, NULL}},
  {"STRING_COLUMN",  {2, 1, MATH_FUNCTION_TYPE_NORMAL, file_string_column, string_column_arg_type, NULL, NULL, NULL}},
  {"ON_END",         {1, 1, MATH_FUNCTION_TYPE_CALLBACK, file_on_end, on_end_arg_type, NULL, NULL, NULL}},
};

static int
add_func_sub(MathEquation *eq, struct funcs *funcs, unsigned int n)
{
  unsigned int i;

  for (i = 0; i < n; i++) {
    if (math_equation_add_func(eq, funcs[i].name, &funcs[i].prm) == NULL) {
      return 1;
    }
  }
  return 0;
}

static int
add_file_func(MathEquation *eq) {
  return add_func_sub(eq, FileFunc, sizeof(FileFunc) / sizeof(*FileFunc));
}

static int
add_fit_func(MathEquation *eq) {
  return add_func_sub(eq, FitFunc, sizeof(FitFunc) / sizeof(*FitFunc));
}

static int
add_basic_func(MathEquation *eq) {
  return add_func_sub(eq, BasicFunc, sizeof(BasicFunc) / sizeof(*BasicFunc));
}

int
get_axis_id(struct objlist *obj, N_VALUE *inst, struct objlist **aobj, int axis)
{
  char *field, *axis_str;
  struct narray iarray;
  int anum, id;

  switch (axis) {
  case AXIS_X:
    field = "axis_x";
    break;
  case AXIS_Y:
    field = "axis_y";
    break;
  case AXIS_REFERENCE:
    field = "reference";
    break;
  default:
    return - ERRNOAXIS;
  }

  _getobj(obj, field, inst, &axis_str);

  if (axis_str == NULL) {
    return - ERRNOAXIS;
  }

  arrayinit(&iarray, sizeof(int));
  if (getobjilist(axis_str, aobj, &iarray, FALSE, NULL)) {
    return - ERRNOAXIS;
  }

  anum = arraynum(&iarray);
  if (anum < 1) {
    arraydel(&iarray);
    return - ERRNOAXISINST;
  }

  id = arraylast_int(&iarray);
  arraydel(&iarray);

  return id;
}

struct axis_prm {
  int posx, posy;
  int len;
  double min, max, min2, max2;
  double vx, vy, rate;
  int type;
};

static int
get_axis_prm(struct objlist *obj, N_VALUE *inst, int axis, struct axis_prm *prm)
{
  int aid, dir;
  N_VALUE *inst1;
  struct objlist *aobj;
  double min, max, ddir;

  aid = get_axis_id(obj, inst, &aobj, axis);
  if (aid  < 0) {
    return aid;
  }

  inst1 = getobjinst(aobj, aid);
  if (inst1 == NULL) {
    return -1;
  }

  if (_getobj(aobj, "x", inst1, &prm->posx)) {
    return -1;
  }

  if (_getobj(aobj, "y", inst1, &prm->posy)) {
    return -1;
  }

  if (_getobj(aobj, "length", inst1, &prm->len)) {
    return -1;
  }

  if (_getobj(aobj, "direction", inst1, &dir)) {
    return -1;
  }

  if (_getobj(aobj, "min", inst1, &min)) {
    return -1;
  }

  if (_getobj(aobj, "max", inst1, &max)) {
    return -1;
  }

  if (_getobj(aobj, "type", inst1, &prm->type)) {
    return -1;
  }

  if (min == 0 && max == 0) {
    int id;
    id = get_axis_id(aobj, inst1, &aobj, AXIS_REFERENCE);
    if (id >= 0) {
      inst1 = getobjinst(aobj, id);
      if (inst1) {
	_getobj(aobj, "min", inst1, &min);
	_getobj(aobj, "max", inst1, &max);
	_getobj(aobj, "type", inst1, &prm->type);
      }
    }
  }

  prm->min2 = min;
  prm->max2 = max;

  ddir = dir / 18000.0 * MPI;
  prm->vx = cos(ddir);
  prm->vy = -sin(ddir);

  if (min == max) {
    return - ERRNOSETAXIS;
  }

  switch (prm->type) {
  case AXIS_TYPE_LOG:
    if (min <= 0 || max <= 0) {
      return - ERRMINMAX;
    }

    min = log10(min);
    max = log10(max);
    break;
  case AXIS_TYPE_INVERSE:
    if (min * max <= 0) {
      return - ERRMINMAX;
    }

    min = 1 / min;
    max = 1 / max;
    break;
  }

  prm->rate = prm->len / (max - min);
  prm->min = min;
  prm->max = max;

  return aid;
}

int
open_array(char *objstr, struct array_prm *ary)
{
  int i, n, dnum, id_max;
  struct narray iarray, *darray;
  struct objlist *dobj, *obj;

  dobj = getobject("darray");
  ary->obj = NULL;
  ary->col_num = 0;
  ary->data_num = 0;
  memset(ary->id, 0, sizeof(ary->id));
  memset(ary->ary, 0, sizeof(ary->ary));

  if (objstr == NULL) {
    return 1;
  }

  arrayinit(&iarray, sizeof(int));
  if (getobjilist(objstr, &obj, &iarray, FALSE, NULL)) {
    return 1;
  }

  if (obj != dobj) {
    arraydel(&iarray);
    return 1;
  }

  n = arraynum(&iarray);
  if (n < 1) {
    arraydel(&iarray);
    return 1;
  }

  if (n > FILE_OBJ_MAXCOL) {
    n = FILE_OBJ_MAXCOL;
  }

  id_max = chkobjlastinst(obj);
  for (i = 0; i < n; i++) {
    int id;
    id = arraynget_int(&iarray, i);
    if (id > id_max) {
      continue;
    }
    darray = NULL;
    getobj(dobj, "@", id, 0, NULL, &darray);
    ary->id[i] = id;
    ary->ary[i] = darray;
    dnum = arraynum(darray);
    if (dnum > ary->data_num) {
      ary->data_num = dnum;
    }
  }
  arraydel(&iarray);

  ary->obj = obj;
  ary->col_num = n;

  return 0;
}

static void
add_file_prm(struct f2ddata *fp, MathEquationParametar *prm)
{
  int i, j, id, num2, *data2;
  N_VALUE *inst1;

  if (prm == NULL) {
    return;
  }

  for (i = 0; i < prm->id_num; i++) {
    id = prm->id[i] / 1000;
    if (id == fp->id) {
      if (fp->maxdim < (prm->id[i] % 1000)) {
	fp->maxdim = prm->id[i] % 1000;
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
}

static struct f2ddata *
opendata(struct objlist *obj,N_VALUE *inst,
	 struct f2dlocal *f2dlocal,int axis,int raw)
{
  int fid;
  char *file;
  struct rgba fg, bg;
  int x,y,type,hskip,rstep,final,csv,src;
  char *remark,*ifs, *array;
  int smoothx,smoothy,averaging_type;
  struct narray *mask;
  struct narray *move,*movex,*movey;
  struct f2ddata *fp;
  int i;
  int axid,ayid;
  int marksize,marktype;
  int prev_datanum;
  int dataclip;
  struct stat stat_buf;
  struct axis_prm ax_prm, ay_prm;
  int div;
  double min, max;

  _getobj(obj, "source", inst, &src);

  /* for file source only */
  _getobj(obj,"file",inst,&file);
  _getobj(obj,"remark",inst,&remark);
  _getobj(obj,"ifs",inst,&ifs);
  _getobj(obj,"csv",inst,&csv);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);

  /* for array source only */
  _getobj(obj,"array",inst,&array);

  /* for range source only */
  _getobj(obj,"range_min",inst,&min);
  _getobj(obj,"range_max",inst,&max);
  _getobj(obj,"range_div",inst,&div);

  /* common */
  _getobj(obj,"id",inst,&fid);
  _getobj(obj,"type",inst,&type);
  _getobj(obj,"head_skip",inst,&hskip);
  _getobj(obj,"read_step",inst,&rstep);
  _getobj(obj,"final_line",inst,&final);
  _getobj(obj,"smooth_x",inst,&smoothx);
  _getobj(obj,"smooth_y",inst,&smoothy);
  _getobj(obj,"averaging_type",inst,&averaging_type);
  _getobj(obj,"mask",inst,&mask);
  _exeobj(obj,"move_data_adjust",inst,0,NULL);
  _getobj(obj,"move_data",inst,&move);
  _getobj(obj,"move_data_x",inst,&movex);
  _getobj(obj,"move_data_y",inst,&movey);
  _getobj(obj,"R",inst,&fg.r);
  _getobj(obj,"G",inst,&fg.g);
  _getobj(obj,"B",inst,&fg.b);
  _getobj(obj,"A",inst,&fg.a);
  _getobj(obj,"R2",inst,&bg.r);
  _getobj(obj,"G2",inst,&bg.g);
  _getobj(obj,"B2",inst,&bg.b);
  _getobj(obj,"A2",inst,&bg.a);
  _getobj(obj,"mark_size",inst,&marksize);
  _getobj(obj,"mark_type",inst,&marktype);
  _getobj(obj,"data_clip",inst,&dataclip);
  _getobj(obj,"data_num",inst,&prev_datanum);

  switch (src) {
  case DATA_SOURCE_FILE:
    if (file==NULL) {
      error(obj,ERRFILE);
      return NULL;
    }
    break;
  case DATA_SOURCE_ARRAY:
    break;
  case DATA_SOURCE_RANGE:
    if (min == max || div < 2) {
      error(obj,ERR_INVALID_RANGE);
      return NULL;
    }
    if (min > max) {
      double tmp;
      tmp = min;
      min = max;
      max = tmp;
    }
    break;
  }

  if (axis) {
    double ip1,ip2;
    axid = get_axis_prm(obj, inst, AXIS_X, &ax_prm);
    if (axid  < 0) {
      error(obj, - axid);
      return NULL;
    }

    ayid = get_axis_prm(obj, inst, AXIS_Y, &ay_prm);
    if (ayid  < 0) {
      error(obj, - ayid);
      return NULL;
    }

    ip1=-ax_prm.vy*ay_prm.vx+ax_prm.vx*ay_prm.vy;
    ip2=-ay_prm.vy*ax_prm.vx+ay_prm.vx*ax_prm.vy;

    if ((fabs(ip1)<=1e-15) || (fabs(ip2)<=1e-15)) {
      error(obj,ERRAXISDIR);
      return NULL;
    }
  } else {
    /* these initialization exist to avoid compile warnings. */
    axid =0;
    ayid =0;
    memset(&ax_prm, 0, sizeof(ax_prm));
    memset(&ay_prm, 0, sizeof(ay_prm));
    ax_prm.type = AXIS_TYPE_LINEAR;
    ay_prm.type = AXIS_TYPE_LINEAR;
  }
  if ((fp=g_malloc(sizeof(struct f2ddata)))==NULL) return NULL;

  fp->local = f2dlocal;
  fp->GC = -1;
  fp->src = src;
  switch (src) {
  case DATA_SOURCE_FILE:
    fp->file=file;
    if ((fp->fd=nfopen(file,"rt"))==NULL) {
      error2(obj,ERROPEN,file);
      g_free(fp);
      return NULL;
    }

    if (fstat(fileno(fp->fd), &stat_buf)) {
      error2(obj,ERROPEN,file);
      fclose(fp->fd);
      g_free(fp);
      return NULL;
    }
    fp->mtime = stat_buf.st_mtime;
    if (file==NULL) {
      error(obj,ERRFILE);
      return NULL;
    }
    break;
  case DATA_SOURCE_ARRAY:
    open_array(array, &fp->array_data);
    if (fp->array_data.data_num < 1) {
      error2(obj,ERROPEN,file);	/* to be fixed */
      g_free(fp);
      return NULL;
    }
    fp->file = NULL;
    fp->fd = NULL;
    fp->mtime = 1;
    break;
  case DATA_SOURCE_RANGE:
    fp->file = NULL;
    fp->fd = NULL;
    fp->range_min = min;
    fp->range_max = max;
    fp->range_div = div;
    fp->mtime = 1;
    break;
  }

  fp->obj=obj;
  fp->id=fid;
  fp->axisx = axid;
  fp->axisy = ayid;
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
  fp->averaging_type = averaging_type;

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

  fp->line_array.line = NULL;
  arrayinit(&(fp->line_array.line_array), sizeof(char *));

  fp->axmin=ax_prm.min;
  fp->axmax=ax_prm.max;
  fp->axmin2=ax_prm.min2;
  fp->axmax2=ax_prm.max2;
  fp->axvx=ax_prm.vx;
  fp->axvy=ax_prm.vy;
  fp->axtype=ax_prm.type;
  fp->axposx=ax_prm.posx;
  fp->axposy=ax_prm.posy;
  fp->axlen=ax_prm.len;
  fp->aymin=ay_prm.min;
  fp->aymax=ay_prm.max;
  fp->aymin2=ay_prm.min2;
  fp->aymax2=ay_prm.max2;
  fp->ayvx=ay_prm.vx;
  fp->ayvy=ay_prm.vy;
  fp->aytype=ay_prm.type;
  fp->ayposx=ay_prm.posx;
  fp->ayposy=ay_prm.posy;
  fp->aylen=ay_prm.len;
  fp->ratex=ax_prm.rate;
  fp->ratey=ay_prm.rate;
  fp->interrupt = FALSE;

  for (i = 0; i < (int) (sizeof(fp->codex) / sizeof(*fp->codex)); i++) {
    fp->codex[i] = f2dlocal->codex[i];
  }
  for (i = 0; i < (int) (sizeof(fp->codey) / sizeof(*fp->codey)); i++) {
    fp->codey[i] = f2dlocal->codey[i];
  }
  fp->const_id = f2dlocal->const_id;
  fp->column_array_id_x = -1;
  fp->column_array_id_y = -1;
  fp->column_string_array_id_x = -1;
  fp->column_string_array_id_y = -1;
  fp->use_column_array = FALSE;
  fp->use_column_string_array = FALSE;
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
  fp->bg=bg;
  fp->color2=bg;
  fp->fnumx = 0;
  fp->needx = NULL;
  fp->fnumy = 0;
  fp->needy = NULL;
  fp->dx=fp->dy=fp->d2=fp->d3=0;
  fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MATH_VALUE_UNDEF;
  fp->fg=fg;
  fp->color=fp->fg;
  fp->marksize0=marksize;
  fp->marksize=marksize;
  fp->marktype0=marktype;
  fp->marktype=marktype;
  fp->text_align_h = 0.0;
  fp->text_align_v = 0.0;
  fp->text_pt = DEFAULT_FONT_PT;
  fp->text_script = DEFAULT_SCRIPT_SIZE;
  fp->text_style = 0;
  fp->text_font = 0;
  fp->text_space = 0;
  fp->ignore=fp->negative=FALSE;
  arrayinit(&(fp->fileopen),sizeof(int));
  if (!raw) {
    MathEquationParametar *prm;

    if (fp->codex[0] && fp->codex[0]->exp) {
      prm = math_equation_get_parameter(fp->codex[0], 'F', NULL);
      if (prm) {
	fp->fnumx = prm->id_num;
	fp->needx = prm->id;
      }
      add_file_prm(fp, prm);
    }

    if (fp->codey[0] && fp->codey[0]->exp) {
      prm = math_equation_get_parameter(fp->codey[0], 'F', NULL);
      if (prm) {
	fp->fnumy = prm->id_num;
	fp->needy = prm->id;
      }
      add_file_prm(fp, prm);
    }

    fp->column_array_id_x = f2dlocal->column_array_id_x;
    fp->column_array_id_y = f2dlocal->column_array_id_y;
    fp->use_column_array = (f2dlocal->column_array_id_x >=0 || f2dlocal->column_array_id_y >= 0);
    fp->column_string_array_id_x = f2dlocal->column_string_array_id_x;
    fp->column_string_array_id_y = f2dlocal->column_string_array_id_y;
    fp->use_column_string_array = (f2dlocal->column_string_array_id_x >=0 || f2dlocal->column_string_array_id_y >= 0);
    fp->end_expression = NULL;
  }
  return fp;
}

static void
clear_line_array(struct f2ddata *fp)
{
  if (fp->line_array.line) {
    g_free(fp->line_array.line);
    fp->line_array.line = NULL;
  }
  arraydel2(&(fp->line_array.line_array));
}

static void
reopendata(struct f2ddata *fp)
{
  if (fp->fd) {
    fseek(fp->fd,0,SEEK_SET);
  }
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
  fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MATH_VALUE_UNDEF;
  fp->color2=fp->bg;
  fp->color=fp->fg;
  fp->marksize=fp->marksize0;
  fp->marktype=fp->marktype0;
  clear_line_array(fp);
}

static void
closedata(struct f2ddata *fp, struct f2dlocal *f2dlocal)
{
  int j,num2,*data2;
  N_VALUE *inst;
  MathValue val;

  if (fp==NULL) return;

  if (fp->end_expression) {
    math_expression_calculate(fp->end_expression, &val);
    fp->end_expression = NULL;
  }
  set_data_progress(fp);

  num2=arraynum(&(fp->fileopen));
  data2=arraydata(&(fp->fileopen));
  for (j=0;j<num2;j++) {
    N_VALUE *inst1;
    if ((inst1=chkobjinst(fp->obj,data2[j]))!=NULL) {
      _exeobj(fp->obj,"closedata_raw",inst1,0,NULL);
    }
  }
  arraydel(&(fp->fileopen));
  if (fp->fd) {
    fclose(fp->fd);
  }
  if ((inst=chkobjinst(fp->obj,fp->id))!=NULL)
    _putobj(fp->obj,"data_num",inst,&(fp->datanum));

  f2dlocal->num = fp->datanum;
  clear_line_array(fp);

  g_free(fp);
}

static char *
create_func_def_str(const char *name, const char *code)
{
  return g_strdup_printf("def %s(x,y,z) {%s;\n}", name, code);
}

static int
set_user_fnc(MathEquation **eq, const char *str, const char *fname, char **err_msg)
{
  int i;
  char default_func[] = "def f(x,y,z){}";
  //                     01234

  if (err_msg)
    *err_msg = NULL;

  for (i = 0; i < EQUATION_NUM; i++) {
    if (eq[i] == NULL) {
      return MATH_ERROR_MEMORY;
    }
  }

  default_func[4] = fname[0];

  if (str) {
    char *buf;
    buf = create_func_def_str(fname, str);
    if (buf == NULL)
      return MATH_ERROR_MEMORY;

    for (i = 0; i < EQUATION_NUM; i++) {
      int r;
      r = math_equation_parse(eq[i], buf);
      if (r) {
	if (err_msg) {
	  *err_msg = math_err_get_error_message(eq[i], buf, r);
	}
	g_free(buf);
	return r;
      }
    }

    g_free(buf);
  } else {
    for (i = 0; i < EQUATION_NUM; i++) {
      math_equation_parse(eq[i], default_func);
    }
  }

  return 0;
}

static int
set_equation(struct f2dlocal *f2dlocal, MathEquation **eq, enum EOEQ_ASSIGN_TYPE type, const char *f, const char *g, const char *h, const char *str, char **err_msg)
{
  int i, rcode, use_eoeq_assign;

  if (err_msg) {
    *err_msg = NULL;
  }

  for (i = 0; i < EQUATION_NUM; i++) {
    if (eq[i]) {
      math_equation_free(eq[i]);
    }
    eq[i] = NULL;
  }

  for (i = 0; i < EQUATION_NUM; i++) {
    eq[i] = ofile_create_math_equation(f2dlocal->const_id, type, 3, TRUE, TRUE, TRUE, TRUE, TRUE);
  }

  for (i = 0; i < EQUATION_NUM; i++) {
    if (eq[i] == NULL) {
      return MATH_ERROR_MEMORY;
    }
  }

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
    rcode = set_user_fnc(eq, h, "h", err_msg);
    if (rcode)
      return rcode;
  }
  use_eoeq_assign = eq[0]->use_eoeq_assign;
  if (str == NULL)
    return 0;

  for (i = 0; i < EQUATION_NUM; i++) {
    rcode = math_equation_parse(eq[i], str);
    if (rcode) {
      if (err_msg) {
	*err_msg = math_err_get_error_message(eq[i], str, rcode);
      }
      return rcode;
    }
    if (use_eoeq_assign) {
      eq[i]->use_eoeq_assign = TRUE;
    }
  }

  return rcode;
}

static int
put_func(struct objlist *obj, N_VALUE *inst, struct f2dlocal *f2dlocal, enum EOEQ_ASSIGN_TYPE assign_type, char *field, char *eq, int *use_eoeq_assign)
{
  int rcode, type;
  char *x, *y, *f, *g, *h, *err_msg;
  MathEquation *eqn;

  if (use_eoeq_assign ) {
    *use_eoeq_assign = FALSE;
  }
  type = field[5];

  _getobj(obj, "math_x", inst, &x);
  _getobj(obj, "math_y", inst, &y);
  _getobj(obj, "func_f", inst, &f);
  _getobj(obj, "func_g", inst, &g);
  _getobj(obj, "func_h", inst, &h);

  switch (type) {
  case 'x':
    f2dlocal->need2passx = FALSE;
    rcode = set_equation(f2dlocal, f2dlocal->codex, assign_type, f, g, h, eq, &err_msg);
    if (err_msg) {
      error22(obj, ERRUNKNOWN, field, err_msg);
      g_free(err_msg);
      set_equation(f2dlocal, f2dlocal->codex, assign_type, f, g, h, x, NULL);
    }
    f2dlocal->need2passx = math_equation_check_const(f2dlocal->codex[0],
						     f2dlocal->const_id,
						     TWOPASS_CONST_SIZE);
    eqn = f2dlocal->codex[0];
    break;
  case 'y':
    f2dlocal->need2passy = FALSE;
    rcode = set_equation(f2dlocal, f2dlocal->codey, assign_type, f, g, h, eq, &err_msg);
    if (err_msg) {
      error22(obj, ERRUNKNOWN, field, err_msg);
      g_free(err_msg);
      set_equation(f2dlocal, f2dlocal->codey, assign_type, f, g, h, y, NULL);
    }
    f2dlocal->need2passy = math_equation_check_const(f2dlocal->codey[0],
						     f2dlocal->const_id,
						     TWOPASS_CONST_SIZE);
    eqn = f2dlocal->codey[0];
    break;
  case 'f':
  case 'g':
  case 'h':
    switch (type) {
    case 'f':
      set_equation(f2dlocal, f2dlocal->codex, assign_type, eq, g, h, x, NULL);
      rcode = set_equation(f2dlocal, f2dlocal->codey, assign_type, eq, g, h, y, &err_msg);
      break;
    case 'g':
      set_equation(f2dlocal, f2dlocal->codex, assign_type, f, eq, h, x, NULL);
      rcode = set_equation(f2dlocal, f2dlocal->codey, assign_type, f, eq, h, y, &err_msg);
      break;
    case 'h':
      set_equation(f2dlocal, f2dlocal->codex, assign_type, f, g, eq, x, NULL);
      rcode = set_equation(f2dlocal, f2dlocal->codey, assign_type, f, g, eq, y, &err_msg);
      break;
    default:
      /* never reached */
      return 1;
    }
    if (err_msg) {
      error22(obj, ERRUNKNOWN, field, err_msg);
      g_free(err_msg);
    }

    if (rcode) {
      set_equation(f2dlocal, f2dlocal->codex, assign_type, f, g, h, x, NULL);
      set_equation(f2dlocal, f2dlocal->codey, assign_type, f, g, h, y, NULL);
    }

    f2dlocal->need2passx = math_equation_check_const(f2dlocal->codex[0],
						     f2dlocal->const_id,
						     TWOPASS_CONST_SIZE);
    f2dlocal->need2passy = math_equation_check_const(f2dlocal->codey[0],
						     f2dlocal->const_id,
						     TWOPASS_CONST_SIZE);
    eqn = f2dlocal->codex[0];
    break;
  default:
    /* never reached */
    return 1;
  }

  if (eqn && use_eoeq_assign ) {
    *use_eoeq_assign = eqn->use_eoeq_assign;
  }
  return rcode;
}

static int
f2dputmath(struct objlist *obj, N_VALUE *inst, enum EOEQ_ASSIGN_TYPE type, char *field, char *math, int *use_eoeq_assign)
{
  int rcode;
  struct f2dlocal *f2dlocal;

  _getobj(obj,"_local",inst,&f2dlocal);

  if (math) {
    g_strstrip(math);
    if (math[0] == '\0') {
      math = NULL;
    }
  }

  rcode = put_func(obj, inst, f2dlocal, type, field, math, use_eoeq_assign);
  if (rcode) {
    return 1;
  }

  if (strcmp(field,"math_x")==0) {
    f2dlocal->column_array_id_x = -1;
    f2dlocal->column_string_array_id_x = -1;
    f2dlocal->maxdimx = 0;
    if (f2dlocal->codex[0]) {
      MathEquationParametar *prm;
      int array_id;

      prm = math_equation_get_parameter(f2dlocal->codex[0], 0, NULL);
      if (prm == NULL) {
	return 1;
      }

      f2dlocal->maxdimx = prm->id_max;
      array_id = math_equation_check_array(f2dlocal->codex[0], COLUMN_ARRAY_NAME);
      f2dlocal->column_array_id_x = array_id;
      array_id = math_equation_check_string_array(f2dlocal->codex[0], "$" COLUMN_ARRAY_NAME);
      f2dlocal->column_string_array_id_x = array_id;
    }
  } else if (strcmp(field,"math_y")==0) {
    f2dlocal->column_array_id_y = -1;
    f2dlocal->maxdimy = 0;
    if (f2dlocal->codey[0]) {
      MathEquationParametar *prm;
      int array_id;

      prm = math_equation_get_parameter(f2dlocal->codey[0], 0, NULL);
      if (prm == NULL) {
	return 1;
      }
      f2dlocal->maxdimy = prm->id_max;
      array_id = math_equation_check_array(f2dlocal->codey[0], COLUMN_ARRAY_NAME);
      f2dlocal->column_array_id_y = array_id;
      array_id = math_equation_check_string_array(f2dlocal->codey[0], "$" COLUMN_ARRAY_NAME);
      f2dlocal->column_string_array_id_y = array_id;
    }
  }
  return 0;
}

static int
check_putmath(struct objlist *obj, N_VALUE *inst, char *field, char *math)
{
  int security, use_eoeq_assign;
  int rcode;

  security = get_security();
  if (security) {
    rcode = f2dputmath(obj, inst, EOEQ_ASSIGN_TYPE_BOTH, field, math, &use_eoeq_assign);
    if (rcode == 0 && use_eoeq_assign) {
      replace_eoeq_token(math);
    }
  } else {
    rcode = f2dputmath(obj, inst, EOEQ_ASSIGN_TYPE_ASSIGN, field, math, NULL);
  }
  return rcode;
}

static int
set_math_config(struct objlist *obj, N_VALUE *inst, char *field, char *str)
{
  char *f1;
  int len, use_eoeq_assign;

  f1 = getitok2(&str, &len, "");
  if (f2dputmath(obj, inst, EOEQ_ASSIGN_TYPE_BOTH, field, f1, &use_eoeq_assign) == 0) {
    if (use_eoeq_assign) {
      replace_eoeq_token(f1);
    }
    _putobj(obj, field, inst, f1);
  } else {
    g_free(f1);
  }
  return 0;
}

static int
f2dloadconfig(struct objlist *obj,N_VALUE *inst)
{
  return obj_load_config(obj, inst, F2DCONF, FileConfigHash);
}

static int
f2dsaveconfig(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  return obj_save_config(obj, inst, F2DCONF, FileConfig, sizeof(FileConfig) / sizeof(*FileConfig));
}

MathEquation *
ofile_create_math_equation(int *id, enum EOEQ_ASSIGN_TYPE type, int prm_digit, int use_fprm, int use_const, int usr_func, int use_fobj_func, int use_fit_func)
{
  MathEquation *code;
  struct math_const_parameter static_const[] = {
    {"FILL_RULE_NONE",     MATH_SCANNER_VAL_TYPE_NORMAL, {GRA_FILL_MODE_NONE,     MATH_VALUE_NORMAL}},
    {"FILL_RULE_EVEN_ODD", MATH_SCANNER_VAL_TYPE_NORMAL, {GRA_FILL_MODE_EVEN_ODD, MATH_VALUE_NORMAL}},
    {"FILL_RULE_WINDING",  MATH_SCANNER_VAL_TYPE_NORMAL, {GRA_FILL_MODE_WINDING,  MATH_VALUE_NORMAL}},
  };

  code = math_equation_basic_new();
  if (code == NULL)
    return NULL;

  math_equation_set_eoeq_assign_type(code, type);

  if (prm_digit > 0) {
    if (math_equation_add_parameter(code, 0, 1, prm_digit, MATH_EQUATION_PARAMETAR_USE_ID)) {
      math_equation_free(code);
      return NULL;
    }
  }

  if (use_fprm) {
    if (math_equation_add_parameter(code, 'F', 3, 5, MATH_EQUATION_PARAMETAR_USE_INDEX)) {
      math_equation_free(code);
      return NULL;
    }
  }

  if (math_equation_add_var(code, "X") != 0) {
    math_equation_free(code);
    return NULL;
  }

  if (math_equation_add_var(code, "Y") != 1) {
    math_equation_free(code);
    return NULL;
  }

  if (use_const) {
    unsigned int i;
    for (i = 0; i < MATH_CONST_SIZE; i++) {
      int f_id;
      f_id = math_equation_add_const(code, FileConstant[i], NULL);
      if (f_id < 0) {
	math_equation_free(code);
	return NULL;
      }
      if (id) {
	id[i] = f_id;
      }
    }
    for (i = 0; i < sizeof(static_const) / sizeof(*static_const); i++) {
      if (math_equation_add_const(code, static_const[i].str, &static_const[i].val) < 0) {
	math_equation_free(code);
	return NULL;
      }
    }
  }

  if (usr_func) {
    math_equation_parse(code, "def f(x,y,z){}");
    math_equation_parse(code, "def g(x,y,z){}");
    math_equation_parse(code, "def h(x,y,z){}");
  }

  if (use_fobj_func) {
    add_file_func(code);
  }

  if (use_fit_func) {
    add_fit_func(code);
  }

  add_basic_func(code);

  return code;
}

static int
f2dinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int x,y,rstep,final,msize,r2,g2,b2,a2,lwidth,miter,src,div;
  char *s1,*s2,*s3,*s4;
  struct f2dlocal *f2dlocal;
  int stat,minmaxstat,dataclip,num,ljoin;
  double min,max;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  src=DATA_SOURCE_FILE;
  x=1;
  y=2;
  rstep=1;
  final=-1;
  msize=DEFAULT_MARK_SIZE;
  lwidth=DEFAULT_LINE_WIDTH;
  r2=255;
  g2=255;
  b2=255;
  a2=255;
  miter=1000;
  num=0;
  stat=MATH_VALUE_MEOF;
  minmaxstat=MATH_VALUE_UNDEF;
  dataclip=TRUE;
  ljoin = JOIN_TYPE_BEVEL;
  div = 512;
  min = 1;
  max = 10;
  if (_putobj(obj,"source",inst,&src)) return 1;
  if (_putobj(obj,"x",inst,&x)) return 1;
  if (_putobj(obj,"y",inst,&y)) return 1;
  if (_putobj(obj,"read_step",inst,&rstep)) return 1;
  if (_putobj(obj,"final_line",inst,&final)) return 1;
  if (_putobj(obj,"mark_size",inst,&msize)) return 1;
  if (_putobj(obj,"R2",inst,&r2)) return 1;
  if (_putobj(obj,"G2",inst,&g2)) return 1;
  if (_putobj(obj,"B2",inst,&b2)) return 1;
  if (_putobj(obj,"A2",inst,&a2)) return 1;
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
  if (_putobj(obj,"range_min",inst,&min)) return 1;
  if (_putobj(obj,"range_max",inst,&max)) return 1;
  if (_putobj(obj,"range_div",inst,&div)) return 1;

  s2 = s3 = s4 = NULL;
  f2dlocal=NULL;

  s1 = g_strdup("#%'");
  if (s1 == NULL) goto errexit;
  if (_putobj(obj, "remark", inst, s1)) goto errexit;

  s2 = g_strdup(" ,\t()");
  if (s2 == NULL) goto errexit;
  if (_putobj(obj, "ifs", inst, s2)) goto errexit;

  s3 = g_strdup("axis:0");
  if (s3 == NULL) goto errexit;
  if (_putobj(obj, "axis_x", inst, s3)) goto errexit;

  s4 = g_strdup("axis:1");
  if (s4 == NULL) goto errexit;
  if (_putobj(obj, "axis_y", inst, s4)) goto errexit;

  f2dlocal=g_malloc(sizeof(struct f2dlocal));
  if (f2dlocal == NULL) goto errexit;
  memset(f2dlocal, 0, sizeof(struct f2dlocal));
  if (_putobj(obj,"_local",inst,f2dlocal)) goto errexit;

  f2dlocal->codex[0] = NULL;
  f2dlocal->codex[1] = NULL;
  f2dlocal->codex[2] = NULL;
  f2dlocal->codey[0] = NULL;
  f2dlocal->codey[1] = NULL;
  f2dlocal->codey[2] = NULL;
  f2dlocal->column_array_id_x = -1;
  f2dlocal->column_array_id_y = -1;
  f2dlocal->column_string_array_id_x = -1;
  f2dlocal->column_string_array_id_y = -1;
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
  f2dlocal->use_drawing_func = FALSE;

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
  f2dlocal->dstdevpx = HUGE_VAL;
  f2dlocal->dstdevpy = HUGE_VAL;
  f2dlocal->dstdevx  = HUGE_VAL;
  f2dlocal->dstdevy  = HUGE_VAL;
  f2dlocal->num = 0;
  f2dlocal->rcode = 0;
  f2dlocal->minx.val = 0;
  f2dlocal->maxx.val = 0;
  f2dlocal->miny.val = 0;
  f2dlocal->maxy.val = 0;
  f2dlocal->minx.type = MATH_VALUE_UNDEF;
  f2dlocal->maxx.type = MATH_VALUE_UNDEF;
  f2dlocal->miny.type = MATH_VALUE_UNDEF;
  f2dlocal->maxy.type = MATH_VALUE_UNDEF;
  f2dlocal->mtime = 0;
  f2dlocal->mtime_stat = 0;
  f2dlocal->total_line = 0;

  f2dloadconfig(obj,inst);
  return 0;

errexit:
  g_free(s1);
  g_free(s2);
  g_free(s3);
  g_free(s4);
  g_free(f2dlocal);
  return 1;
}

static int
f2ddone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"_local",inst,&f2dlocal);
  closedata(f2dlocal->data, f2dlocal);
  math_equation_free(f2dlocal->codex[0]);
  math_equation_free(f2dlocal->codex[1]);
  math_equation_free(f2dlocal->codex[2]);
  math_equation_free(f2dlocal->codey[0]);
  math_equation_free(f2dlocal->codey[1]);
  math_equation_free(f2dlocal->codey[2]);
  return 0;
}

static int
f2dfile(struct objlist *obj, N_VALUE *inst, N_VALUE *rval,
	int argc, char **argv)
{
  struct objlist *sys;
  int ignorepath;
  char *file, *file2;
  int num2;
  struct f2dlocal *f2dlocal;

  _getobj(obj, "_local", inst, &f2dlocal);
  f2dlocal->mtime = 0;
  f2dlocal->mtime_stat = 0;
  sys=getobject("system");
  getobj(sys, "ignore_path", 0, 0, NULL, &ignorepath);

  num2 = 0;
  _putobj(obj, "data_num", inst, &num2);

  if (argv[2] == NULL) {
    return 0;
  }

  file = get_utf8_filename(argv[2]);
  if (file == NULL) {
    return 1;
  }

  g_free(argv[2]);
  if (ignorepath) {
    file2 = getbasename(file);
    g_free(file);
    argv[2] = file2;
  } else {
    argv[2] = file;
  }

  return 0;
}

static int
f2dbasename(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                int argc,char **argv)
{
  char *file,*file2;

  g_free(rval->str);
  rval->str=NULL;
  _getobj(obj,"file",inst,&file);
  if (file==NULL) return 0;
  file2=getbasename(file);
  rval->str=file2;
  return 0;
}

static int
f2dput(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv)
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
    return check_putmath(obj, inst, field, math);
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

static double
get_value_from_str(char *po, char *po2, int *type)
{
  char *endptr;
  int st, ch;
  double val;

  if (po == po2) {
    *type = MATH_VALUE_NONUM;
    return 0;
  }

  ch = *po2;
  *po2 = '\0';
  val = strtod(po, &endptr);
  *po2 = ch;
  if (endptr >= po2) {
    if (check_infinite(val)) {
      st = MATH_VALUE_NAN;
    } else {
      st = MATH_VALUE_NORMAL;
    }
  } else if (endptr != po) {
    if (g_ascii_isspace(*endptr)) {
      if (check_infinite(val)) {
	st = MATH_VALUE_NAN;
      } else {
	st = MATH_VALUE_NORMAL;
      }
    } else {
      st = MATH_VALUE_NONUM;
    }
  } else {
    char *top, *bottom;
    top = po;
    bottom = po2;
    for (; top < bottom; top++) {
      if (! g_ascii_isspace(*top)) {
	break;
      }
    }
    for (; top < bottom; bottom--) {
      if (! g_ascii_isspace(*(bottom - 1))) {
	break;
      }
    }
    switch (bottom - top) {
    case 1:
      switch (*top) {
      case '|':
	st = MATH_VALUE_CONT;
	break;
      case '=':
	st = MATH_VALUE_BREAK;
	break;
      default:
	st = MATH_VALUE_NONUM;
      }
      break;
    case 3:
      if (strncmp(top, "NAN", 3) == 0) {
	st = MATH_VALUE_NAN;
      } else {
	st = MATH_VALUE_NONUM;
      }
      break;
    case 4:
      if (strncmp(top, "CONT", 4) == 0) {
	st = MATH_VALUE_CONT;
      } else {
	st = MATH_VALUE_NONUM;
      }
      break;
    case 5:
      if (strncmp(top, "UNDEF", 5) == 0) {
	st = MATH_VALUE_UNDEF;
      } else if (strncmp(top, "BREAK", 5) == 0) {
	st = MATH_VALUE_BREAK;
      } else {
	st = MATH_VALUE_NONUM;
      }
      break;
    default:
      st = MATH_VALUE_NONUM;
    }
  }

  *type = st;
  return val;
}

int
n_strtod(const char *str, MathValue *val)
{
  char *po, *po2;
  size_t l;
  int type;
  if (val == NULL || str == NULL) {
    return 1;
  }
  po = g_ascii_strup(str, -1);
  if (po == NULL) {
    return 1;
  }
  l = strlen(po);
  po2 = po + l;
  val->type = MATH_VALUE_NORMAL;
  val->val = get_value_from_str(po, po2, &type);
  g_free(po);
  val->type = type;
  return 0;
}

static void
column_array_clear(MathEquation **code, int id)
{
  int j;
  if (id < 0) {
    return;
  }

  for (j = 0; j < EQUATION_NUM; j++) {
    math_equation_clear_array(code[j], id);
  }
}

static void
column_array_push(MathEquation **code, int id, MathValue *val)
{
  int j;
  if (id < 0) {
    return;
  }

  for (j = 0; j < EQUATION_NUM; j++) {
    math_equation_push_array_val(code[j], id, val);
  }
}

static int
getdataarray(struct f2ddata *fp, char *buf, int maxdim, MathValue *data)
{
/*
   return: 0 no error
          -1 fatal error
           1 too small column
*/
  char *po, *po2;
  int st;
  double val;
  int i, r;
  int dim;
  char *ifs, csv;
  MathValue v;
  ifs = fp->ifs_buf;
  csv = fp->csv;
  fp->count++;
  r = 1;
  dim=0;
  data[dim].val = fp->count;
  data[dim].type = MATH_VALUE_NORMAL;
  po=buf;
  if (fp->use_column_array) {
    column_array_clear(fp->codex, fp->column_array_id_x);
    column_array_clear(fp->codey, fp->column_array_id_y);
    column_array_push(fp->codex, fp->column_array_id_x, data);
    column_array_push(fp->codey, fp->column_array_id_y, data);
  }
  while (*po!='\0') {
    int hex;
    hex = FALSE;
    if (! fp->use_column_array && dim >= maxdim) {
      r = 0;
      break;
    }
    if (csv) {
      for (;*po==' ';po++);
      if (*po=='\0') break;
      if (CHECK_IFS(ifs, *po)) {
        po2=po;
        po2++;
        val=0;
        st=MATH_VALUE_NONUM;
      } else {
#if 0
        for (po2=po;(*po2!='\0') && ! CHECK_IFS(ifs, *po2) && (*po2!=' ');po2++)
          *po2=toupper(*po2);
        for (i=0;i<po2-po;i++) if (*(po+i)=='D') *(po+i)='e';
#else
        for (po2=po;(*po2!='\0') && ! CHECK_IFS(ifs, *po2) && (*po2!=' ');po2++) {
	  *po2=toupper(*po2);
	  switch (*po2) {
	  case 'X':
	    hex = TRUE;
	    break;
	  case 'D':
	    if (! hex) {
	      *po2 = 'e';
	    }
	    break;
	  }
	}
#endif
	val = get_value_from_str(po, po2, &st);
        for (;*po2==' ';po2++);
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
	switch (*po2) {
	case 'X':
	  hex = TRUE;
	  break;
	case 'D':
	  if (! hex) {
	    *po2 = 'e';
	  }
	  break;
	}
      }
#endif
      val = get_value_from_str(po, po2, &st);
    }
    po=po2;
    dim++;
    v.val = val;
    v.type = st;
    if (dim <= maxdim) {
      data[dim] = v;
    }
    if (fp->use_column_array) {
      column_array_push(fp->codex, fp->column_array_id_x, &v);
      column_array_push(fp->codey, fp->column_array_id_y, &v);
    }
  }
  for (i=dim+1;i<=maxdim;i++) {
    data[i].val = 0;
    data[i].type = MATH_VALUE_NONUM;
  }
  return r;
}


static int
hskipdata(struct f2ddata *fp)
{
  int skip;
  char *buf;

  switch (fp->src) {
  case DATA_SOURCE_FILE:
    skip=0;
    while (skip<fp->hskip) {
      int rcode;
      if ((fp->line & UPDATE_PROGRESS_LINE_NUM) == 0 && set_data_progress(fp)) {
	return 0;
      }
      rcode=fgetline(fp->fd,&buf);
      if (rcode==-1) return -1;
      if (rcode==1) {
	fp->eof=TRUE;
	return 0;
      }
      g_free(buf);
      fp->line++;
      skip++;
    }
    break;
  case DATA_SOURCE_ARRAY:
    if (fp->hskip > fp->array_data.data_num) {
      fp->eof=TRUE;
      return 0;
    }
    fp->line = fp->hskip;
    break;
  case DATA_SOURCE_RANGE:
    if (fp->hskip > fp->range_div) {
      fp->eof = TRUE;
      return 0;
    }
    fp->line = fp->hskip;
    break;
  }
  return 0;
}

static int
set_data_progress(struct f2ddata *fp)
{
  char msgbuf[32];
  double frac;

  if (fp->final > 0) {
    frac = 1.0 * fp->line / fp->final;
  } else if (fp->prev_datanum > 0) {
    if (fp->datanum <= fp->prev_datanum) {
      if (fp->line < fp->hskip) {
	frac = 1.0 * fp->line / (fp->prev_datanum + fp->hskip);
      } else {
	frac = 1.0 * (fp->datanum + fp->bufnum + fp->hskip) / (fp->prev_datanum + fp->hskip);
      }
    } else {
      frac = -1;
    }
  } else {
    frac = -1;
  }

  if (frac > 1) {
    frac = 1.0;
  }

  snprintf(msgbuf, sizeof(msgbuf), "id:%d (%d)", fp->id, fp->line);
  set_progress(0, msgbuf, frac);
  if (ninterrupt() || fp->interrupt) {
    fp->eof=TRUE;
    fp->interrupt = TRUE;
    return TRUE;
  }
  return FALSE;
}

static void
getdata_get_other_files(struct f2ddata *fp, int fnumx, int fnumy, int *needx, int *needy,
			MathValue *datax, MathValue *datay, int filenum, int *openfile)
{
  char *argv[2];
  int i,j,k;
  double *ddata;
  int colnum;
  struct narray iarray;
  int col;
  struct narray *coldata;

  for (i = 0; i < filenum; i++) {
    int *idata, inum, argc, id;
    N_VALUE *inst1;
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
		datax[j].val = ddata[k];
		datax[j].type = nround(ddata[k + inum]);
	      }
	    }
	  }
	}
	for (j = 0; j < fnumy; j++) {
	  if (needy[j] / 1000 == id) {
	    n = needy[j] % 1000;
	    for (k = 0; k < inum; k++){
	      if (idata[k] == n) {
		datay[j].val = ddata[k];
		datay[j].type = nround(ddata[k + inum]);
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
getdata_skip_step(struct f2ddata *fp, int progress)
{
  char *buf;
  int i, step;

  switch (fp->src) {
  case DATA_SOURCE_FILE:
    step = 1;
    while (step < fp->rstep) {
      int rcode;
      if (progress && (fp->line & UPDATE_PROGRESS_LINE_NUM) == 0 && set_data_progress(fp)) {
	return 1;
      }
      rcode = fgetline(fp->fd, &buf);
      if (rcode == 1) {
	fp->eof = TRUE;
	g_free(buf);
	break;
      } else if (rcode == -1) {
	g_free(buf);
	return -1;
      }
      fp->line++;
      for (i = 0; buf[i] != '\0' && CHECK_IFS(fp->ifs_buf, buf[i]); i++);
      if (buf[i] != '\0' && (! CHECK_REMARK(fp->remark, fp->ifs_buf, buf[i]))) {
	step++;
      }
      g_free(buf);
    }
    break;
  case DATA_SOURCE_ARRAY:
    if (fp->line + fp->rstep - 1 > fp->array_data.data_num) {
      fp->eof = TRUE;
    } else {
      fp->line += fp->rstep - 1;
    }
    break;
  case DATA_SOURCE_RANGE:
    if (fp->line + fp->rstep - 1 > fp->range_div) {
      fp->eof = TRUE;
    } else {
      fp->line += fp->rstep - 1;
    }
    break;
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

static void
set_var(MathEquation *eq, const MathValue *x, const MathValue *y)
{
  math_equation_set_var(eq, 0, x);
  math_equation_set_var(eq, 1, y);
}

struct obj_name_const {
  char *obj_name;
  int const_id;
};

static int
set_const(MathEquation *eq, int *const_id, int need2pass, struct f2ddata *fp, int first)
{
  struct obj_name_const obj_names[] = {
    {"data",      MATH_CONST_DATA_OBJ},
    {"file",      MATH_CONST_FILE_OBJ},
    {"path",      MATH_CONST_PATH_OBJ},
    {"rectangle", MATH_CONST_RECT_OBJ},
    {"arc",       MATH_CONST_ARC_OBJ},
    {"mark",      MATH_CONST_MARK_OBJ},
    {"text",      MATH_CONST_TEXT_OBJ},
  };
  MathValue val;
  int i;

  if (eq == NULL || eq->exp == NULL)
    return 0;

  math_equation_clear(eq);

  if (need2pass) {
    math_equation_set_const(eq, const_id[MATH_CONST_MINX], &fp->minx);
    math_equation_set_const(eq, const_id[MATH_CONST_MINY], &fp->miny);

    math_equation_set_const(eq, const_id[MATH_CONST_MAXX], &fp->maxx);
    math_equation_set_const(eq, const_id[MATH_CONST_MAXY], &fp->maxy);

    val.val = fp->num;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_const(eq, const_id[MATH_CONST_NUM], &val);

    val.val = fp->sumx;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_const(eq, const_id[MATH_CONST_SUMX], &val);

    val.val = fp->sumy;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_const(eq, const_id[MATH_CONST_SUMY], &val);

    val.val = fp->sumxx;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_const(eq, const_id[MATH_CONST_SUMXX], &val);

    val.val = fp->sumyy;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_const(eq, const_id[MATH_CONST_SUMYY], &val);

    val.val = fp->sumxy;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_const(eq, const_id[MATH_CONST_SUMXY], &val);

    if (fp->num > 0) {
      double tmp;
      val.val = fp->sumx / fp->num;
      val.type = MATH_VALUE_NORMAL;
      math_equation_set_const(eq, const_id[MATH_CONST_AVX], &val);

      val.val = fp->sumy / fp->num;
      val.type = MATH_VALUE_NORMAL;
      math_equation_set_const(eq, const_id[MATH_CONST_AVY], &val);

      tmp = fp->sumxx / fp->num - (fp->sumx / fp->num) * (fp->sumx / fp->num);
      val.val = (tmp < 0) ? 0 : sqrt(tmp);
      val.type = MATH_VALUE_NORMAL;
      math_equation_set_const(eq, const_id[MATH_CONST_SGX], &val);
      math_equation_set_const(eq, const_id[MATH_CONST_STDEVPX], &val);

      tmp = fp->sumyy / fp->num - (fp->sumy / fp->num) * (fp->sumy / fp->num);
      val.val = (tmp < 0) ? 0 : sqrt(tmp);
      val.type = MATH_VALUE_NORMAL;
      math_equation_set_const(eq, const_id[MATH_CONST_SGY], &val);
      math_equation_set_const(eq, const_id[MATH_CONST_STDEVPY], &val);
    }
    if (fp->num > 1) {
      double n, tmp;

      n = fp->num;
      tmp = fp->sumxx / (n - 1) - (fp->sumx / (n - 1)) * (fp->sumx / n);
      val.val = (tmp < 0) ? 0 : sqrt(tmp);
      val.type = MATH_VALUE_NORMAL;
      math_equation_set_const(eq, const_id[MATH_CONST_STDEVX], &val);

      tmp = fp->sumyy / (n - 1) - (fp->sumy / (n - 1)) * (fp->sumy / n);
      val.val = (tmp < 0) ? 0 : sqrt(tmp);
      val.type = MATH_VALUE_NORMAL;
      math_equation_set_const(eq, const_id[MATH_CONST_STDEVY], &val);
    }
  }

  val.val = chkobjlastinst(fp->obj) + 1;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_N], &val);

  val.val = fp->id;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_D], &val);

  val.val = first;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_FIRST], &val);

  val.val = fp->x;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_COLX], &val);

  val.val = fp->y;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_COLY], &val);

  val.val = fp->axisx;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_AXISX], &val);

  val.val = fp->axmin2;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_AXISX_MIN], &val);

  val.val = fp->axmax2;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_AXISX_MAX], &val);

  val.val = fp->axlen;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_AXISX_LEN], &val);

  val.val = fp->axisy;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_AXISY], &val);

  val.val = fp->aymin2;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_AXISY_MIN], &val);

  val.val = fp->aymax2;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_AXISY_MAX], &val);

  val.val = fp->aylen;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_AXISY_LEN], &val);

  val.val = fp->masknum;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_MASK], &val);

  val.val = fp->movenum;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_MOVE], &val);

  val.val = fp->hskip;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_HSKIP], &val);

  val.val = fp->rstep;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_RSTEP], &val);

  getobj(fp->obj, "final_line", fp->id, 0, NULL, &i);
  val.val = i;
  val.type = MATH_VALUE_NORMAL;
  math_equation_set_const(eq, const_id[MATH_CONST_FLINE], &val);

  for (i = 0; i < (int) (sizeof(obj_names) / sizeof(*obj_names)); i++) {
    struct objlist *obj;

    obj = chkobject(obj_names[i].obj_name);
    if (obj == NULL) {
      continue;
    }

    val.val = chkobjectid(obj);
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_const(eq, const_id[obj_names[i].const_id], &val);
  }

  return math_equation_optimize(eq);
}

static int
set_const_all(struct f2ddata *fp)
{
  int first_x1, first_y1;

  if (set_const(fp->codex[0], fp->const_id, fp->need2pass, fp, TRUE))
    return 1;

  if (set_const(fp->codey[0], fp->const_id, fp->need2pass, fp, TRUE))
    return 1;

  switch (fp->type) {
  case TYPE_NORMAL:
    first_x1 = FALSE;
    first_y1 = FALSE;
    break;
  case TYPE_DIAGONAL:
    first_x1 = FALSE;
    first_y1 = FALSE;
    break;
  case TYPE_ERR_X:
    first_x1 = TRUE;
    first_y1 = FALSE;
    break;
  case TYPE_ERR_Y:
    first_x1 = FALSE;
    first_y1 = TRUE;
    break;
  default:
    first_x1 = FALSE;
    first_y1 = FALSE;
  }

  if (set_const(fp->codex[1], fp->const_id, fp->need2pass, fp, first_x1))
    return 1;

  if (set_const(fp->codex[2], fp->const_id, fp->need2pass, fp, FALSE))
    return 1;

  if (set_const(fp->codey[1], fp->const_id, fp->need2pass, fp, first_y1))
    return 1;

  if (set_const(fp->codey[2], fp->const_id, fp->need2pass, fp, FALSE))
    return 1;

  return 0;
}

static int
file_calculate(struct f2ddata *fp, MathEquation *eq, const MathValue *x, const MathValue *y, MathValue *prm, MathValue *fprm, MathValue *val)
{
  int r;
  if (eq == NULL || eq->exp == NULL)
    return 0;

  math_equation_set_parameter_data(eq, 0, prm);
  math_equation_set_parameter_data(eq, 'F', fprm);
  set_var(eq, x, y);
  math_equation_set_user_data(eq, fp);
  r = math_equation_calculate(eq, val);
  if (val->type == MATH_VALUE_INTERRUPT) {
    fp->interrupt = TRUE;
  }
  return r;
}

static int
getdata_sub2(struct f2ddata *fp, int fnumx, int fnumy, int *needx, int *needy, MathValue *datax, MathValue *datay,
	     MathValue *gdata, int filenum, int *openfile)
{
  int masked,moved,moven;
  struct f2ddata_buf *buf;
  MathValue dx, dy, dx2, dy2, dx3, dy3, d2, d3;

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
    int i, j;
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
	datax[i] = gdata[j];
      } else {
	datax[i].val = 0;
	datax[i].type = MATH_VALUE_NONUM;
      }
    }
    for (i = 0; i < fnumy; i++) {
      if (needy[i] / 1000 == fp->id) {
	j = needy[i] % 1000;
	datay[i] = gdata[j];
      } else {
	datay[i].val = 0;
	datay[i].type = MATH_VALUE_NONUM;
      }
    }
    if (filenum) {
      getdata_get_other_files(fp, fnumx, fnumy, needx, needy, datax, datay, filenum, openfile);
    }
  }

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

    dx2 = d2;
    dy2 = d3;

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

    if (fp->codey[1]) {
      file_calculate(fp, fp->codey[1], &dx2, &dy2, gdata, datax, &d2);
    }
    if (fp->codey[2]) {
      file_calculate(fp, fp->codey[2], &dx3, &dy3, gdata, datax, &d3);
    }
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

#if BUF_TYPE == USE_BUF_PTR
  buf = fp->buf_ptr[fp->bufnum];
#elif BUF_TYPE == USE_RING_BUF
  buf = &fp->buf[ring_buf_index(fp, fp->bufnum)];
#else
  buf = &fp->buf[fp->bufnum];
#endif
  buf->col = fp->color;
  buf->marksize = fp->marksize;
  buf->marktype = fp->marktype;
  buf->line = fp->line;
  buf->col2 = fp->color2;
  buf->dx = dx.val;
  buf->dy = dy.val;
  buf->d2 = d2.val;
  buf->d3 = d3.val;
  buf->dxstat = dx.type;
  buf->dystat = dy.type;
  buf->d2stat = d2.type;
  buf->d3stat = d3.type;
  fp->bufnum++;

  if (fp->rstep > 1) {
    return getdata_skip_step(fp, TRUE);
  }

  return 0;
}

static void
array_data(MathValue *gdata, struct narray *array, int i)
{
  if (array == NULL) {
    gdata->val = 0;
    gdata->type = MATH_VALUE_NONUM;
    return;
  }

  gdata->val = arraynget_double(array, i);
  gdata->type = MATH_VALUE_NORMAL;
}

static void
set_column_array(MathEquation **code, int id, MathValue *gdata, int maxdim)
{
  int i, j;
  if (id < 0) {
    return;
  }

  for (j = 0; j < EQUATION_NUM; j++) {
    math_equation_clear_array(code[j], id);
    for (i = 0; i <= maxdim; i++) {
      math_equation_set_array_val(code[j], id, i, gdata + i);
    }
  }
}

#define CHECK_TERMINATE(ch) ((ch) == '\0' || (ch) == '\n')
#define CHECK_CHR(ifs, ch) (ch && strchr(ifs, ch))

static void
check_add_str(struct narray *array, const char *str, int len)
{
  int valid;
  char *ptr;

  valid = g_utf8_validate(str, len, NULL);

  if (valid) {
    ptr = g_strndup(str, len);
    arrayadd(array, &ptr);
  } else {
    ptr = g_locale_to_utf8(str, len, NULL, NULL, NULL);
    if (ptr == NULL) {
      GString *s;
      int i;

      s = g_string_new("");
      if (s == NULL) {
	return;
      }
      for (i = 0; i < len; i++) {
	if (g_ascii_isprint(str[i]) || g_ascii_isspace(str[i])) {
	  g_string_append_c(s, str[i]);
	} else {
	  g_string_append(s, "〓");
	}
      }
      ptr = g_string_free(s, FALSE);
    }
    if (ptr) {
      arrayadd(array, &ptr);
    }
  }
}

const char *
parse_data_line(struct narray *array, const char *str, const char *ifs, int csv)
{
  const char *po;
  int len;

  if (str == NULL) {
    return NULL;
  }

  po = str;
  while (! CHECK_TERMINATE(*po)) {
    if (csv) {
      for (; *po == ' '; po++);
      if (CHECK_TERMINATE(*po)) break;
      if (CHECK_CHR(ifs, *po)) {
        po++;
	check_add_str(array, "", 0);
      } else {
	len = 0;
        for (; (! CHECK_TERMINATE(po[len])) && ! CHECK_CHR(ifs, po[len]) && (po[len] != ' '); len++) ;
	check_add_str(array, po, len);
	po += len;
	for (; (*po == ' '); po++);
	if (CHECK_CHR(ifs, *po)) po++;
      }
    } else {
      for (; (! CHECK_TERMINATE(*po)) && CHECK_CHR(ifs, *po); po++);
      len = 0;
      for (; (! CHECK_TERMINATE(po[len])) && ! CHECK_CHR(ifs, po[len]); len++) ;
      check_add_str(array, po, len);
      po += len;
      if (CHECK_TERMINATE(*po)) break;
    }
  }

  if (*po == '\n') {
    po++;
  }

  return (*po) ? po : NULL;
}

static void
set_column_string_array_equation(int id, MathEquation **code, const char *firts_line, const char **data, int n)
{
  int i, eqn;
  if (id < 0) {
    return;
  }
  for (eqn = 0; eqn < EQUATION_NUM; eqn++) {
    math_equation_clear_string_array(code[eqn], id);
    math_equation_set_array_str(code[eqn], id, 0, firts_line);
    for (i = 0; i < n; i++) {
      math_equation_set_array_str(code[eqn], id, i + 1, data[i]);
    }
  }
}

static void
set_column_string_array(struct f2ddata *fp)
{
  int n;
  struct narray *array;
  const char **data;
  const char *line;
  array = &(fp->line_array.line_array);
  line = fp->line_array.line;
  parse_data_line(array, line, fp->ifs, fp->csv);
  n = arraynum(array);
  data = arraydata(array);
  set_column_string_array_equation(fp->column_string_array_id_x, fp->codex, line, data, n);
  set_column_string_array_equation(fp->column_string_array_id_y, fp->codey, line, data, n);
}

static int
get_data_from_source(struct f2ddata *fp, int maxdim, MathValue *gdata)
{
  char *buf;
  int i, rcode, n;
  double x;
  MathValue nonum;

  nonum.val = 0;
  nonum.type = MATH_VALUE_NONUM;
  rcode = 0;
  switch (fp->src) {
  case DATA_SOURCE_FILE:
    rcode = fgetline(fp->fd, &buf);
    if (rcode == 1 || rcode == -1) {
      fp->eof = TRUE;
      return rcode;
    }

    fp->line++;

    clear_line_array(fp);
    fp->line_array.line = g_strdup(buf);
    if (fp->use_column_string_array) {
      set_column_string_array(fp);
    }
    for (i = 0; buf[i] && CHECK_IFS(fp->ifs_buf, buf[i]); i++);
    rcode = 2;
    if (buf[i] != '\0' && (! CHECK_REMARK(fp->remark, fp->ifs_buf, buf[i]))) {
      rcode = getdataarray(fp, buf, maxdim, gdata);
      if (rcode != -1) {
	rcode = 0;
      }
    }

    g_free(buf);
    break;
  case DATA_SOURCE_ARRAY:
    if (fp->line >= fp->array_data.data_num) {
      fp->eof = TRUE;
      return 1;
    }
    n = (fp->array_data.col_num > fp->maxdim) ? fp->maxdim : fp->array_data.col_num;
    fp->count++;
    gdata[0].val = fp->count;
    gdata[0].type = MATH_VALUE_NORMAL;
    for (i = 0; i < n; i++) {
      array_data(gdata + i + 1, fp->array_data.ary[i], fp->line);
    }
    for (i = n + 1; i <= fp->maxdim; i++) {
      gdata[i] = nonum;
    }

    set_column_array(fp->codex, fp->column_array_id_x, gdata, n);
    set_column_array(fp->codey, fp->column_array_id_y, gdata, n);
    fp->line++;
    break;
  case DATA_SOURCE_RANGE:
    if (fp->line > fp->range_div) {
      fp->eof = TRUE;
      return 1;
    }
    fp->count++;
    gdata[0].val = fp->count;
    gdata[0].type = MATH_VALUE_NORMAL;
    x = fp->range_min + (fp->range_max - fp->range_min) / fp->range_div * fp->line;
    gdata[1].val = x;
    gdata[1].type = MATH_VALUE_NORMAL;
    gdata[2].val = x;
    gdata[2].type = MATH_VALUE_NORMAL;
    for (i = 3; i <= fp->maxdim; i++) {
      gdata[i] = nonum;
    }

    fp->line++;
    set_column_array(fp->codex, fp->column_array_id_x, gdata, 2);
    set_column_array(fp->codey, fp->column_array_id_y, gdata, 2);
    break;
  }
  return rcode;
}

static int
getdata_sub1(struct f2ddata *fp, int fnumx, int fnumy, int *needx, int *needy,
	     MathValue *datax, MathValue *datay, MathValue *gdata, int filenum, int *openfile)
{
  while (! fp->eof && fp->bufnum < DXBUFSIZE) {
    int rcode;
    if (fp->final >= 0 && fp->line >= fp->final) {
      fp->eof=TRUE;
      break;
    }

    if ((fp->line & UPDATE_PROGRESS_LINE_NUM) == 0 && set_data_progress(fp)) {
      break;
    }

    rcode = get_data_from_source(fp, fp->maxdim, gdata);
    if (rcode == 0) {
      if (getdata_sub2(fp, fnumx, fnumy, needx, needy, datax, datay, gdata, filenum, openfile)) {
	break;
      }
    } else if (rcode == 1) {
      fp->eof=TRUE;
      break;
    } else if (rcode==-1) {
      return -1;
    }

    if (fp->interrupt) {
      return -1;
    }
    if ((fp->final>=0) && (fp->line>=fp->final)) fp->eof=TRUE;
  }
  return 0;
}

static void
calculate_average_simple(struct f2ddata *fp, int smx, int smy, int sm2, int sm3, int num)
{
  int i;
  struct f2ddata_buf *buf;
  int numx, numy, num2, num3;
  double sumx, sumy, sum2, sum3;

  sumx=sumy=sum2=sum3=0;
  numx=numy=num2=num3=0;
#if BUF_TYPE == USE_BUF_PTR
  for (i = 0; i <= num; i++) {
    buf = fp->buf_ptr[i];
    if (buf->dxstat == MATH_VALUE_NORMAL &&
	i >= fp->bufpo - smx &&
	i <= fp->bufpo + smx) {
      sumx += buf->dx;
      numx++;
    }
    if (buf->dystat == MATH_VALUE_NORMAL &&
	i >= fp->bufpo - smy &&
	i <= fp->bufpo + smy) {
      sumy += buf->dy;
      numy++;
    }
    if (buf->d2stat == MATH_VALUE_NORMAL &&
	i >= fp->bufpo - sm2 &&
	i <= fp->bufpo + sm2) {
      sum2 += buf->d2;
      num2++;
    }
    if (buf->d3stat == MATH_VALUE_NORMAL &&
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
  fp->col = buf->col;
  fp->col2 = buf->col2;
  fp->msize = buf->marksize;
  fp->mtype = buf->marktype;
#elif BUF_TYPE == USE_RING_BUF
  for (i=0;i<=num;i++) {
    n = ring_buf_index(fp, i);
    if ((fp->buf[n].dxstat==MATH_VALUE_NORMAL)
     && (i>=fp->bufpo-smx) && (i<=fp->bufpo+smx)) {
      sumx+=fp->buf[n].dx;
      numx++;
    }
    if ((fp->buf[n].dystat==MATH_VALUE_NORMAL)
     && (i>=fp->bufpo-smy) && (i<=fp->bufpo+smy)) {
      sumy+=fp->buf[n].dy;
      numy++;
    }
    if ((fp->buf[n].d2stat==MATH_VALUE_NORMAL)
     && (i>=fp->bufpo-sm2) && (i<=fp->bufpo+sm2)) {
      sum2+=fp->buf[n].d2;
      num2++;
    }
    if ((fp->buf[n].d3stat==MATH_VALUE_NORMAL)
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
  fp->col=fp->buf[n].col;
  fp->col2=fp->buf[n].col2;
  fp->msize=fp->buf[n].marksize;
  fp->mtype=fp->buf[n].marktype;
#else  /* BUF_TYPE */
  for (i=0;i<=num;i++) {
    if ((fp->buf[i].dxstat==MATH_VALUE_NORMAL)
     && (i>=fp->bufpo-smx) && (i<=fp->bufpo+smx)) {
      sumx+=fp->buf[i].dx;
      numx++;
    }
    if ((fp->buf[i].dystat==MATH_VALUE_NORMAL)
     && (i>=fp->bufpo-smy) && (i<=fp->bufpo+smy)) {
      sumy+=fp->buf[i].dy;
      numy++;
    }
    if ((fp->buf[i].d2stat==MATH_VALUE_NORMAL)
     && (i>=fp->bufpo-sm2) && (i<=fp->bufpo+sm2)) {
      sum2+=fp->buf[i].d2;
      num2++;
    }
    if ((fp->buf[i].d3stat==MATH_VALUE_NORMAL)
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
  fp->col=fp->buf[fp->bufpo].col;
  fp->col2=fp->buf[fp->bufpo].col2;
  fp->msize=fp->buf[fp->bufpo].marksize;
  fp->mtype=fp->buf[fp->bufpo].marktype;
#endif	/* BUF_TYPE */
}

static void
calculate_average_weughted(struct f2ddata *fp, int smx, int smy, int sm2, int sm3, int num)
{
  int i, weight;
  struct f2ddata_buf *buf;
  int numx, numy, num2, num3;
  double sumx, sumy, sum2, sum3, wx, wy, w2, w3;

  sumx = sumy = sum2 = sum3 = 0;
  numx = numy = num2 = num3 = 0;
  wx = wy = w2 = w3 = 0;
/*
           o
  i: 0 1 2 3 4 5 6 7 8 9 10
b-i: 3 2 1 0 1 2 3
  smx: 5
  bufpo: 3
  bufpo - smx: -2
  bufpo + smx: 8
  if (smx > bufpo) smx = bufpo

                 o
  i: 0 1 2 3 4 5 6 7 8 9 10 11
b-i: 6 5 4 3 2 1 0 1 2 3  4  5
  smx: 5
  bufpo: 6
  bufpo - smx: 1
  bufpo + smx: 11
  if (smx > bufpo) smx = bufpo
 */
#if 0
  if (smx > fp->bufpo) smx = fp->bufpo;
  if (smy > fp->bufpo) smy = fp->bufpo;
  if (sm2 > fp->bufpo) sm2 = fp->bufpo;
  if (sm3 > fp->bufpo) sm3 = fp->bufpo;
  if (smx > num - fp->bufpo) smx = num - fp->bufpo;
  if (smy > num - fp->bufpo) smy = num - fp->bufpo;
  if (sm2 > num - fp->bufpo) sm2 = num - fp->bufpo;
  if (sm3 > num - fp->bufpo) sm3 = num - fp->bufpo;
#endif
  for (i = 0; i <= num; i++) {
    buf = fp->buf_ptr[i];
    if (buf->dxstat == MATH_VALUE_NORMAL &&
	i >= fp->bufpo - smx &&
	i <= fp->bufpo + smx) {
      weight = smx + 1 - abs(fp->bufpo - i);
      wx += weight;
      sumx += buf->dx * weight;
      numx++;
    }
    if (buf->dystat == MATH_VALUE_NORMAL &&
	i >= fp->bufpo - smy &&
	i <= fp->bufpo + smy) {
      weight = smy + 1 - abs(fp->bufpo - i);
      wy += weight;
      sumy += buf->dy * weight;
      numy++;
    }
    if (buf->d2stat == MATH_VALUE_NORMAL &&
	i >= fp->bufpo - sm2 &&
	i <= fp->bufpo + sm2) {
      weight = sm2 + 1 - abs(fp->bufpo - i);
      w2 += weight;
      sum2 += buf->d2 * weight;
      num2++;
    }
    if (buf->d3stat == MATH_VALUE_NORMAL &&
	i >= fp->bufpo - sm3 &&
	i <= fp->bufpo + sm3) {
      weight = sm3 + 1 - abs(fp->bufpo - i);
      w3 += weight;
      sum3 += buf->d3 * weight;
      num3++;
    }
  }

  buf = fp->buf_ptr[fp->bufpo];

  if (numx != 0) {
    fp->dx = sumx / wx;
  }
  fp->dxstat = buf->dxstat;

  if (numy != 0) {
    fp->dy = sumy / wy;
  }
  fp->dystat = buf->dystat;

  if (num2 != 0) {
    fp->d2 = sum2 / w2;
  }
  fp->d2stat = buf->d2stat;

  if (num3 != 0) {
    fp->d3 = sum3 / w3;
  }
  fp->d3stat = buf->d3stat;
  fp->dline = buf->line;
  fp->col = buf->col;
  fp->col2 = buf->col2;
  fp->msize = buf->marksize;
  fp->mtype = buf->marktype;
}

#define HALFLIFE 2.8854

static void
calculate_average_exponential(struct f2ddata *fp, int smx, int smy, int sm2, int sm3, int num)
{
  int i;
  struct f2ddata_buf *buf;
  int numx, numy, num2, num3;
  double sumx, sumy, sum2, sum3, ax, ay, a2, a3, wx, wy, w2, w3, weight;

  sumx = sumy = sum2 = sum3 = 0;
  numx = numy = num2 = num3 = 0;
  wx = wy = w2 = w3 = 0;
#if 0
  if (smx > fp->bufpo) smx = fp->bufpo;
  if (smy > fp->bufpo) smy = fp->bufpo;
  if (sm2 > fp->bufpo) sm2 = fp->bufpo;
  if (smx > num - fp->bufpo) smx = num - fp->bufpo;
  if (smy > num - fp->bufpo) smy = num - fp->bufpo;
  if (sm2 > num - fp->bufpo) sm2 = num - fp->bufpo;
  if (sm3 > num - fp->bufpo) sm3 = num - fp->bufpo;
#endif
  ax = (1 - 2.0 / (smx / HALFLIFE + 2));
  ay = (1 - 2.0 / (smy / HALFLIFE + 2));
  a2 = (1 - 2.0 / (sm2 / HALFLIFE + 2));
  a3 = (1 - 2.0 / (sm3 / HALFLIFE + 2));
  for (i = 0; i <= num; i++) {
    buf = fp->buf_ptr[i];
    if (buf->dxstat == MATH_VALUE_NORMAL &&
	i >= fp->bufpo - smx &&
	i <= fp->bufpo + smx) {
      weight = pow(ax, abs(fp->bufpo - i));
      wx += weight;
      sumx += buf->dx * weight;
      numx++;
    }
    if (buf->dystat == MATH_VALUE_NORMAL &&
	i >= fp->bufpo - smy &&
	i <= fp->bufpo + smy) {
      weight = pow(ay, abs(fp->bufpo - i));
      wy += weight;
      sumy += buf->dy * weight;
      numy++;
    }
    if (buf->d2stat == MATH_VALUE_NORMAL &&
	i >= fp->bufpo - sm2 &&
	i <= fp->bufpo + sm2) {
      weight = pow(a2, abs(fp->bufpo - i));
      w2 += weight;
      sum2 += buf->d2 * weight;
      num2++;
    }
    if (buf->d3stat == MATH_VALUE_NORMAL &&
	i >= fp->bufpo - sm3 &&
	i <= fp->bufpo + sm3) {
      weight = pow(a3, abs(fp->bufpo - i));
      w3 += weight;
      sum3 += buf->d3 * weight;
      num3++;
    }
  }

  buf = fp->buf_ptr[fp->bufpo];
  if (numx != 0) {
    fp->dx = sumx / wx;
  }
  fp->dxstat = buf->dxstat;

  if (numy != 0) {
    fp->dy = sumy / wy;
  }
  fp->dystat = buf->dystat;

  if (num2 != 0) {
    fp->d2 = sum2 / w2;
  }
  fp->d2stat = buf->d2stat;

  if (num3 != 0) {
    fp->d3 = sum3 / w3;
  }
  fp->d3stat = buf->d3stat;
  fp->dline = buf->line;
  fp->col = buf->col;
  fp->col2 = buf->col2;
  fp->msize = buf->marksize;
  fp->mtype = buf->marktype;
}

static void
calculate_average(struct f2ddata *fp, int smx, int smy, int sm2, int sm3)
{
  int num;

  if (fp->bufpo + fp->smooth >= fp->bufnum) {
    num = fp->bufnum - 1;
  } else {
    num = fp->bufpo + fp->smooth;
  }

  switch (fp->averaging_type) {
  case MOVING_AVERAGE_SIMPLE:
    calculate_average_simple(fp, smx, smy, sm2, sm3, num);
    break;
  case MOVING_AVERAGE_WEIGHTED:
    calculate_average_weughted(fp, smx, smy, sm2, sm3, num);
    break;
  case MOVING_AVERAGE_EXPONENTIAL:
    calculate_average_exponential(fp, smx, smy, sm2, sm3, num);
    break;
  }
}

static int
getdata(struct f2ddata *fp)
/*
  return -1: fatal error
          0: no error
          1: EOF
*/
{
  int rcode;
  int smx,smy,sm2,sm3;
  int filenum,*openfile,*needx,*needy;
  struct narray filedatax,filedatay;
  unsigned int fnumx,fnumy,j;
#if BUF_TYPE == USE_RING_BUF
  int n;
#endif
  MathValue *datax,*datay;
  static MathValue gdata[FILE_OBJ_MAXCOL + 3];
  static MathValue math_value_zero = {0, 0};

  fp->dx=fp->dy=fp->d2=fp->d3=0;
  fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MATH_VALUE_UNDEF;
  filenum=arraynum(&(fp->fileopen));
  openfile=arraydata(&(fp->fileopen));

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


  if (rcode) {
    return rcode;
  }

  arraydel(&filedatax);
  arraydel(&filedatay);
  if ((fp->bufnum==0) || (fp->bufpo>=fp->bufnum)) {
    fp->dx=fp->dy=fp->d2=fp->d3=0;
    fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MATH_VALUE_MEOF;
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
  calculate_average(fp, smx, smy, sm2, sm3);

  switch (fp->type) {
  case TYPE_NORMAL:
    if (fp->dxstat==MATH_VALUE_NORMAL && fp->dystat==MATH_VALUE_NORMAL)
      fp->datanum++;
    break;
  case TYPE_DIAGONAL:
    if (fp->dxstat==MATH_VALUE_NORMAL && fp->dystat==MATH_VALUE_NORMAL && fp->d2stat==MATH_VALUE_NORMAL && fp->d3stat==MATH_VALUE_NORMAL)
      fp->datanum++;
    break;
  case TYPE_ERR_X:
    if (fp->dystat==MATH_VALUE_NORMAL && fp->d2stat==MATH_VALUE_NORMAL && fp->d3stat==MATH_VALUE_NORMAL)
      fp->datanum++;
    break;
  case TYPE_ERR_Y:
    if (fp->dxstat==MATH_VALUE_NORMAL && fp->d2stat==MATH_VALUE_NORMAL && fp->d3stat==MATH_VALUE_NORMAL)
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
  return 0;
}


static int
getdata2(struct f2ddata *fp, MathEquation *code, int maxdim, double *dd, int *ddstat)
/*
  return -1: fatal error
          0: no error
          1: EOF
*/
{
  MathValue val, gdata[FILE_OBJ_MAXCOL + 3], dx2, dy2;
  int masked;
  int find;

  *dd=0;
  *ddstat=MATH_VALUE_UNDEF;
  find=FALSE;
  while (!fp->eof && (!find)) {
    int rcode;
    if ((fp->final>=0) && (fp->line>=fp->final)) {
      fp->eof=TRUE;
      break;
    }
    rcode = get_data_from_source(fp, maxdim, gdata);
    if (rcode == 1) {
      fp->eof=TRUE;
      break;
    } else if (rcode==-1) {
      return -1;
    } else if (rcode != 0) {
      continue;
    }

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
    *dd=0;
    *ddstat=MATH_VALUE_UNDEF;
    dx2 = gdata[fp->x];
    dy2 = gdata[fp->y];
    if (code && code->exp) {
      file_calculate(fp, code, &dx2, &dy2, gdata, NULL, &val);
      *dd = val.val;
      *ddstat = val.type;
    }
    if (masked) *ddstat=MATH_VALUE_CONT;
    find=TRUE;
    fp->dline=fp->line;
    rcode = getdata_skip_step(fp, FALSE);
    if (rcode == -1) {
      return -1;
    }
    if ((fp->final>=0) && (fp->line>=fp->final)) fp->eof=TRUE;
  }
  if (!find) {
    *dd=0;
    *ddstat=MATH_VALUE_MEOF;
    return 1;
  }
  return 0;
}

static int
getdataraw(struct f2ddata *fp, int maxdim, MathValue *data)
/*
  return -1: fatal error
		  0: no error
		  1: EOF
*/
{
  int i;
  int masked;
  double dx,dy,d2,d3;
  char dxstat,dystat,d2stat,d3stat;
  int datanum;

  fp->dx=fp->dy=fp->d2=fp->d3=0;
  fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MATH_VALUE_UNDEF;
  datanum=0;
  while (!fp->eof && (datanum==0)) {
    int rcode;
    if ((fp->final>=0) && (fp->line>=fp->final)) {
      fp->eof=TRUE;
      break;
    }

    if ((fp->line & UPDATE_PROGRESS_LINE_NUM) == 0 && set_data_progress(fp)) {
      break;
    }

    rcode = get_data_from_source(fp, maxdim, data);
    if (rcode == 1) {
      fp->eof=TRUE;
      break;
    } else if (rcode==-1) {
      return -1;
    } else if (rcode != 0) {
      continue;
    }
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
    dx=dy=d2=d3=0;
    dystat=d2stat=d3stat=MATH_VALUE_UNDEF;
    dx = data[fp->x].val;
    dxstat = data[fp->x].type;

    switch (fp->type) {
    case TYPE_DIAGONAL:
      dy = data[fp->x + 1].val;
      dystat = data[fp->x + 1].type;
      break;
    default:
      dy = data[fp->y].val;
      dystat = data[fp->y].type;
    }

    switch (fp->type) {
    case TYPE_NORMAL:
      break;
    case TYPE_DIAGONAL:
      d2 = data[fp->y].val;
      d2stat = data[fp->y].type;
      d3 = data[fp->y + 1].val;
      d3stat = data[fp->y + 1].type;
      break;
    case TYPE_ERR_X:
      d2 = data[fp->x].val + data[fp->x + 1].val;
      if (data[fp->x].type < data[fp->x + 1].type) d2stat = data[fp->x].type;
      else d2stat = data[fp->x + 1].type;
      d3 = data[fp->x].val + data[fp->x + 2].val;
      if (data[fp->x].type < data[fp->x + 2].type) d3stat = data[fp->x].type;
      else d3stat = data[fp->x + 2].type;
      break;
    case TYPE_ERR_Y:
      d2 = data[fp->y].val + data[fp->y + 1].val;
      if (data[fp->y].type < data[fp->y + 1].type) d2stat = data[fp->y].type;
      else d2stat = data[fp->y + 1].type;
      d3 = data[fp->y].val + data[fp->y + 2].val;
      if (data[fp->y].type < data[fp->y + 2].type) d3stat = data[fp->y].type;
      else d3stat = data[fp->y + 2].type;
      break;
    }
    if (masked) {
      dxstat=dystat=d2stat=d3stat=MATH_VALUE_CONT;
      for (i=0;i<=maxdim;i++)
	data[i].type = MATH_VALUE_CONT;
    }
    fp->dx=dx;
    fp->dy=dy;
    fp->d2=d2;
    fp->d3=d3;
    fp->dxstat=dxstat;
    fp->dystat=dystat;
    fp->d2stat=d2stat;
    fp->d3stat=d3stat;
    fp->col=fp->color;
    fp->col2=fp->color2;
    datanum++;

    if (fp->rstep > 1 && getdata_skip_step(fp, TRUE))
      return -1;

    fp->datanum++;

    if ((fp->final>=0) && (fp->line>=fp->final)) fp->eof=TRUE;
  }
  if (datanum==0) {
    fp->dx=fp->dy=fp->d2=fp->d3=0;
    fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MATH_VALUE_MEOF;
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
  fp->dxstat = fp->dystat = fp->d2stat = fp->d3stat = MATH_VALUE_UNDEF;
  fp->dline = 0;
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
  MathValue gdata[FILE_OBJ_MAXCOL + 3];

  if (check_mtime(fp, local) == 0) {
    return 0;
  }

  if (hskipdata(fp)!=0) {
    closedata(fp, local);
    return 1;
  }

  fp->minx.type = MATH_VALUE_UNDEF;
  fp->maxx.type = MATH_VALUE_UNDEF;
  fp->miny.type = MATH_VALUE_UNDEF;
  fp->maxy.type = MATH_VALUE_UNDEF;
  fp->sumx=0;
  fp->sumy=0;
  fp->sumxx=0;
  fp->sumyy=0;
  fp->sumxy=0;
  fp->num=0;
  while ((rcode = getdataraw(fp, fp->maxdim, gdata)) == 0) {
    switch (fp->type) {
    case TYPE_NORMAL:
    case TYPE_DIAGONAL:
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)) {
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

      if (fp->type == TYPE_NORMAL)
	break;
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
	  fp->maxy.val = fp->d3;
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
      break;
    case TYPE_ERR_X:
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
      break;
    case TYPE_ERR_Y:
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
	  fp->maxy.val = fp->d3;
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
      break;
    }
  }
  fp->dx=fp->dy=fp->d2=fp->d3=0;
  fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MATH_VALUE_UNDEF;
  fp->dline=0;

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
getposition(struct f2ddata *fp, double x, double y, int *gx, int *gy)
/*
  return -1: unable to transform
          0: normal
          1: outside region
*/
{
  *gx = *gy = 0;
  if (getposition2(fp, fp->axtype, fp->aytype, &x, &y)) {
    return -1;
  }
  if (fp->dataclip &&
      (((fp->axmin > x || x > fp->axmax) && (fp->axmax > x || x > fp->axmin)) ||
       ((fp->aymin > y || y > fp->aymax) && (fp->aymax > y || y > fp->aymin)))) {
    /* fix-me: this condition will be simplified as (fp->dataclip && (fp->axmin>x || x>fp->axmax || fp->aymin>y || y>fp->aymax)) */
    return 1;
  }
  if (_f2dtransf(x, y, gx, gy, fp)) {
    fp->ignore = TRUE;
    return -1;
  }
  return 0;
}

static int
get_pos_sub(struct f2ddata *fp, double *val, int atype)
{
  switch (atype) {
  case AXIS_TYPE_LOG:
    if (*val == 0) {
      fp->ignore = TRUE;
      return -1;
    } else if (*val < 0) {
      fp->negative = TRUE;
      *val = fabs(*val);
    }
    *val = log10(*val);
    break;
  case AXIS_TYPE_INVERSE:
    if (*val == 0) {
      fp->ignore = TRUE;
      return -1;
    }
    *val = 1 / *val;
    break;
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
  int r;

  r = get_pos_sub(fp, x, axtype);
  if (r) {
    return -1;
  }

  r = get_pos_sub(fp, y, aytype);
  if (r) {
    return -1;
  }

  return 0;
}

static int
_f2dtransf(double x,double y,int *gx,int *gy,void *local)
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
    return 1;
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

static void
f2dtransf(double x,double y,int *gx,int *gy,void *local)
{
  _f2dtransf(x, y, gx, gy, local);
}

static int
f2dlineclipf(double *x0,double *y0,double *x1,double *y1,void *local)
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
    double a;
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

  switch (fp->src) {
  case DATA_SOURCE_FILE:
    snprintf(buf, sizeof(buf), "#%d: %s (%d:%s)",fp->id,fp->file,fp->dline,s);
    break;
  case DATA_SOURCE_ARRAY:
    snprintf(buf, sizeof(buf), "#%d: Array (%s)",fp->id, s);
    break;
 case DATA_SOURCE_RANGE:
   snprintf(buf, sizeof(buf), "#%d: Range (%s)",fp->id, s);
    break;
  }
  error2(obj,code,buf);
}

static void
errordisp(struct objlist *obj,
	  struct f2ddata *fp,
	  int *emerr,int *emnonum,int *emig,int *emng)
{
  int x,y;
  char *s;

  if (!*emerr) {
    x=FALSE;
    y=FALSE;
    if ((fp->dxstat==MATH_VALUE_ERROR) || (fp->dxstat==MATH_VALUE_NAN)) x=TRUE;
    if ((fp->dystat==MATH_VALUE_ERROR) || (fp->dystat==MATH_VALUE_NAN)) y=TRUE;

    switch (fp->type) {
    case TYPE_NORMAL:
      break;
    case TYPE_DIAGONAL:
      if ((fp->d2stat==MATH_VALUE_ERROR) || (fp->d2stat==MATH_VALUE_NAN)) x=TRUE;
      if ((fp->d3stat==MATH_VALUE_ERROR) || (fp->d3stat==MATH_VALUE_NAN)) y=TRUE;
      break;
    case TYPE_ERR_X:
      if ((fp->d2stat==MATH_VALUE_ERROR) || (fp->d2stat==MATH_VALUE_NAN)) x=TRUE;
      if ((fp->d3stat==MATH_VALUE_ERROR) || (fp->d3stat==MATH_VALUE_NAN)) x=TRUE;
      break;
    case TYPE_ERR_Y:
      if ((fp->d2stat==MATH_VALUE_ERROR) || (fp->d2stat==MATH_VALUE_NAN)) y=TRUE;
      if ((fp->d3stat==MATH_VALUE_ERROR) || (fp->d3stat==MATH_VALUE_NAN)) y=TRUE;
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
  if (!*emnonum) {
    x=FALSE;
    y=FALSE;
    if (fp->dxstat==MATH_VALUE_NONUM) x=TRUE;
    if (fp->dystat==MATH_VALUE_NONUM) y=TRUE;
    switch (fp->type) {
    case TYPE_NORMAL:
      break;
    case TYPE_DIAGONAL:
      if (fp->d2stat==MATH_VALUE_NONUM) x=TRUE;
      if (fp->d3stat==MATH_VALUE_NONUM) y=TRUE;
      break;
    case TYPE_ERR_X:
      if (fp->d2stat==MATH_VALUE_NONUM) x=TRUE;
      if (fp->d3stat==MATH_VALUE_NONUM) x=TRUE;
      break;
    case TYPE_ERR_Y:
      if (fp->d2stat==MATH_VALUE_NONUM) y=TRUE;
      if (fp->d3stat==MATH_VALUE_NONUM) y=TRUE;
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
	   int *emerr,int *emnonum,int *emig,int *emng,
	   char ddstat,char *s)
{
  if (!*emerr && (ddstat==MATH_VALUE_ERROR)) {
    f2derror(obj,fp,ERRMERR,s);
    *emerr=TRUE;
  }
  if (!*emerr && (ddstat==MATH_VALUE_NAN)) {
    f2derror(obj,fp,ERRMERR,s);
    *emerr=TRUE;
  }
  if (!*emnonum && (ddstat==MATH_VALUE_NONUM)) {
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
	int fr,int fg,int fb,int fa, int *size,
	double **x,double **y,double **z,
	int **r,int **g,int **b, int **a,
	double **c1,double **c2,double **c3,
	double **c4,double **c5,double **c6)
{
  double *xb,*yb,*zb,*c1b,*c2b,*c3b,*c4b,*c5b,*c6b;
  int *rb,*gb,*bb,*ab;
  int bz;

  if (*size==0) {
    if (((*x=g_malloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*y=g_malloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*z=g_malloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*r=g_malloc(sizeof(int)*SPBUFFERSZ))==NULL)
     || ((*g=g_malloc(sizeof(int)*SPBUFFERSZ))==NULL)
     || ((*b=g_malloc(sizeof(int)*SPBUFFERSZ))==NULL)
     || ((*a=g_malloc(sizeof(int)*SPBUFFERSZ))==NULL)
     || ((*c1=g_malloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*c2=g_malloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*c3=g_malloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*c4=g_malloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*c5=g_malloc(sizeof(double)*SPBUFFERSZ))==NULL)
     || ((*c6=g_malloc(sizeof(double)*SPBUFFERSZ))==NULL)) {
      g_free(*x);  g_free(*y);  g_free(*z);
      g_free(*r);  g_free(*g);  g_free(*b);  g_free(*a);
      g_free(*c1); g_free(*c2); g_free(*c3);
      g_free(*c4); g_free(*c5); g_free(*c6);
      return NULL;
    }
  } else if ((*size%SPBUFFERSZ)==0) {
    bz=*size/SPBUFFERSZ+1;
    if (((xb=g_realloc(*x,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((yb=g_realloc(*y,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((zb=g_realloc(*z,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((rb=g_realloc(*r,sizeof(int)*SPBUFFERSZ*bz))==NULL)
     || ((gb=g_realloc(*g,sizeof(int)*SPBUFFERSZ*bz))==NULL)
     || ((bb=g_realloc(*b,sizeof(int)*SPBUFFERSZ*bz))==NULL)
     || ((ab=g_realloc(*b,sizeof(int)*SPBUFFERSZ*bz))==NULL)
     || ((c1b=g_realloc(*c1,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((c2b=g_realloc(*c2,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((c3b=g_realloc(*c3,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((c4b=g_realloc(*c4,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((c5b=g_realloc(*c5,sizeof(double)*SPBUFFERSZ*bz))==NULL)
     || ((c6b=g_realloc(*c6,sizeof(double)*SPBUFFERSZ*bz))==NULL)) {
      g_free(*x);  g_free(*y);  g_free(*z);
      g_free(*r);  g_free(*g);  g_free(*b);  g_free(*a);
      g_free(*c1); g_free(*c2); g_free(*c3);
      g_free(*c4); g_free(*c5); g_free(*c6);
      return NULL;
    } else {
      *x=xb;   *y=yb;   *z=zb;
      *r=rb;   *g=gb;   *b=bb;   *a=ab;
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
  (*a)[*size]=fa;
  (*size)++;
  return *x;
}

static int
markout(struct objlist *obj,struct f2ddata *fp,int GC, int width,int snum,int *style)
{
  int emerr,emnonum,emig,emng;
  int gx,gy;

  emerr=emnonum=emig=emng=FALSE;
  GRAlinestyle(GC,snum,style,width,GRA_LINE_CAP_BUTT,GRA_LINE_JOIN_MITER,1000);
  while (getdata(fp)==0) {
    if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL) &&
	(getposition(fp,fp->dx,fp->dy,&gx,&gy)==0)) {
      if (fp->msize>0)
        GRAmark(GC,fp->mtype, gx, gy, fp->msize,
		fp->col.r, fp->col.g, fp->col.b, fp->col.a,
		fp->col2.r, fp->col2.g, fp->col2.b, fp->col2.a);
    } else errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
  }
  errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
  return 0;
}

static int
lineout(struct objlist *obj,struct f2ddata *fp,int GC,
	int width,int snum,int *style,
	int join,int miter,int close)
{
  int emerr,emnonum,emig,emng;
  int first;
  double x0,y0;

  emerr=emnonum=emig=emng=FALSE;
#if EXPAND_DOTTED_LINE
  GRAlinestyle(GC,0,NULL,width,GRA_LINE_CAP_BUTT,join,miter);
#else
  GRAlinestyle(GC, snum, style, width, GRA_LINE_CAP_BUTT, join, miter);
#endif
  first=TRUE;
  while (getdata(fp)==0) {
    GRAcolor(GC,fp->col.r,fp->col.g,fp->col.b, fp->col.a);
    if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
    && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
      if (first) {
        GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,NULL,NULL,fp,
                      fp->dx,fp->dy);
        first=FALSE;
        x0=fp->dx;
        y0=fp->dy;
      } else {
	GRAdashlinetod(GC,fp->dx,fp->dy);
      }
    } else {
      if ((fp->dxstat!=MATH_VALUE_CONT) && (fp->dystat!=MATH_VALUE_CONT)) {
        if (! first && close) {
	  GRAdashlinetod(GC,x0,y0);
	}
        first=TRUE;
      }
      errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
    }
  }
  if (!first && close) GRAdashlinetod(GC,x0,y0);
  errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
  return 0;
}

static void
poly_add_point(struct narray *pos, double x, double y, struct f2ddata *fp)
{
  int gx, gy;

  f2dtransf(x, y, &gx, &gy, fp);
  arrayadd(pos, &gx);
  arrayadd(pos, &gy);
}

static void
poly_add_clip_point(struct narray *pos, double minx, double miny, double maxx, double maxy, double x, double y, struct f2ddata *fp)
{
  if (x < minx) {
    x = minx;
  } else if (x > maxx) {
    x = maxx;
  }

  if (y < miny) {
    y = miny;
  } else if (y > maxy) {
    y = maxy;
  }

  poly_add_point(pos, x, y, fp);
}

static int
poly_pos_sort_cb(const void *a, const void *b)
{
  const struct point_pos *p1, *p2;
  double d;
  int r;

  p1 = a;
  p2 = b;

  d = p1->d - p2->d;

  if (d < 0) {
    r = -1;
  } else if (d > 0) {
    r = 1;
  } else {
    r = 0;
  }

  return r;
}

static void
poly_set_pos(struct point_pos *p, int i, double x, double y, double x0, double y0)
{
  p[i].x = x;
  p[i].y = y;
  x -= x0;
  y -= y0;
  p[i].d = x * x + y * y;
}

static int
poly_add_elements(struct narray *pos,
	       double minx, double miny, double maxx, double maxy,
	       double x0, double y0, double x1, double y1,
	       struct f2ddata *fp)
{
  double x, y, v0, v1, a, b, ba;
  struct point_pos cpos[4];
  int i;

  if (x0 == x1 && y0 == y1) {
    return 1;
  }

  if (x0 >= minx && x0 <= maxx && y0 >= miny && y0 <= maxy) {
    poly_add_clip_point(pos, minx, miny, maxx, maxy, x0, y0, fp);
  }

  if (x0 == x1) {
    if ((y0 < miny && y1 < miny) || (y0 > maxy && y1 > maxy)) {
      return 1;
    }

    if (y0 > y1) {
      if (y0 > maxy) {
	poly_add_clip_point(pos, minx, miny, maxx, maxy, x0, maxy, fp);
      }

      if (y1 < miny) {
	poly_add_clip_point(pos, minx, miny, maxx, maxy, x0, miny, fp);
      }
    } else {
      if (y0 < miny) {
	poly_add_clip_point(pos, minx, miny, maxx, maxy, x0, miny, fp);
      }

      if (y1 > maxy) {
	poly_add_clip_point(pos, minx, miny, maxx, maxy, x0, maxy, fp);
      }
    }

    goto End;
  } else if (y0 == y1) {
    if ((x0 < minx && x1 < minx) || (x0 > maxx && x1 > maxx)){
      return 1;
    }

    if (x0 > x1) {
      if (x0 > maxx) {
	poly_add_clip_point(pos, minx, miny, maxx, maxy, maxx, y0, fp);
      }

      if (x1 < minx) {
	poly_add_clip_point(pos, minx, miny, maxx, maxy, minx, y0, fp);
      }
    } else {
      if (x0 < minx) {
	poly_add_clip_point(pos, minx, miny, maxx, maxy, minx, y0, fp);
      }

      if (x1 > maxx) {
	poly_add_clip_point(pos, minx, miny, maxx, maxy, maxx, y0, fp);
      }
    }

    goto End;
  }

  a = (y1 - y0) / (x1 - x0);
  b = (x1 * y0 - x0 * y1) / (x1 - x0);
  ba = (x1 * y0 - x0 * y1) / (y1 - y0);

  cpos[0].d = -1;
  cpos[1].d = -1;
  cpos[2].d = -1;
  cpos[3].d = -1;

  v0 = a * x0 + b;
  v1 = a * x1 + b;
  x = maxx;
  y = a * maxx + b;
  if (((x >= x0 && x <= x1) || (x >= x1 && x <= x0)) &&
      ((y >= v0 && y <= v1) || (y >= v1 && y <= v0))) {
    poly_set_pos(cpos, 0, x, y, x0, y0);
  }

  x = minx;
  y = a * minx + b;
  if (((x >= x0 && x <= x1) || (x >= x1 && x <= x0)) &&
      ((y >= v0 && y <= v1) || (y >= v1 && y <= v0))) {
    poly_set_pos(cpos, 1, x, y, x0, y0);
  }

  v0 = y0 / a - ba;
  v1 = y1 / a - ba;
  x = maxy / a -  ba;
  y = maxy;
  if (((x >= v0 && x <= v1) || (x >= v1 && x <= v0)) &&
      ((y >= y0 && y <= y1) || (y >= y1 && y <= y0))) {
    poly_set_pos(cpos, 2, x, y, x0, y0);
  }

  x = miny / a -  ba;
  y = miny;
  if (((x >= v0 && x <= v1) || (x >= v1 && x <= v0)) &&
      ((y >= y0 && y <= y1) || (y >= y1 && y <= y0))) {
    poly_set_pos(cpos, 3, x, y, x0, y0);
  }

  qsort(cpos, 4, sizeof(*cpos), poly_pos_sort_cb);
  for (i = 0; i < 4; i++) {
    if (cpos[i].d >= 0) {
      poly_add_clip_point(pos, minx, miny, maxx, maxy, cpos[i].x, cpos[i].y, fp);
    }
  }

 End:
  if (x1 >= minx && x1 <= maxx && y1 >= miny && y1 <= maxy) {
    poly_add_clip_point(pos, minx, miny, maxx, maxy, x1, y1, fp);
  }

  return 0;
}

static void
add_polygon_point(struct narray *pos, double x0, double y0, double x1, double y1, struct f2ddata *fp)
{
  double minx, miny, maxx, maxy;

  if (! fp->dataclip) {
    poly_add_point(pos, x0, y0, fp);
    poly_add_point(pos, x1, y1, fp);

    return;
  }

  if (fp->axmin > fp->axmax) {
    minx = fp->axmax;
    maxx = fp->axmin;
  } else {
    minx = fp->axmin;
    maxx = fp->axmax;
  }
  if (fp->aymax > fp->aymin) {
    miny = fp->aymin;
    maxy = fp->aymax;
  } else {
    miny = fp->aymax;
    maxy = fp->aymin;
  }

  poly_add_elements(pos, minx, miny, maxx, maxy, x0, y0, x1, y1, fp);
}

static void
uniq_points(struct narray *pos)
{
  int n, i, x0, y0, x1, y1;

  n = arraynum(pos) / 2 - 1;
  if (n < 2) {
    return;
  }

  for (i = n; i > 0; i--) {
    x0 = arraynget_int(pos, i * 2 - 2);
    y0 = arraynget_int(pos, i * 2 - 1);
    x1 = arraynget_int(pos, i * 2);
    y1 = arraynget_int(pos, i * 2 + 1);
    if (x0 == x1 && y0 == y1) {
      arrayndel(pos, i * 2 + 1);
      arrayndel(pos, i * 2);
    }
  }

  n = arraynum(pos) / 2 - 1;
  if (n < 2) {
    return;
  }

  x0 = arraynget_int(pos, 0);
  y0 = arraynget_int(pos, 1);
  x1 = arraynget_int(pos, n * 2);
  y1 = arraynget_int(pos, n * 2 + 1);
  if (x0 == x1 && y0 == y1) {
    arrayndel(pos, n * 2 + 1);
    arrayndel(pos, n * 2);
  }

}

static void
draw_polygon(struct narray *pos, int GC, int fill)
{
  int n, *ap;

  uniq_points(pos);
  ap = (int *) arraydata(pos);
  n = arraynum(pos);
  if (n > 3) {
    GRAdrawpoly(GC, n / 2, ap, fill);
  }

#if 0
  /* for debug */
  int i;
  for (i = 0; i < n / 2; i++) {
    char buf[256];
    GRAmark(GC, 0, ap[i * 2], ap[i * 2 + 1], 200,
	    0, 0, 0, 255,
	    0, 0, 0, 255);
    GRAcolor(GC, 0, 0, 0, 255);
    sprintf(buf, "%d/%d", i + 1, n / 2);
    GRAmoveto(GC, ap[i * 2], ap[i * 2 + 1] - i * 500);
    GRAdrawtext(GC, buf, "Serif", 0, 2000, 0, 0, 7000);
  }
#endif
}

static int
polyout(struct objlist *obj, struct f2ddata *fp, int GC)
{
  int emerr, emnonum, emig, emng;
  int first;
  struct narray pos;
  double x0, y0, x1, y1, x2, y2;

  arrayinit(&pos, sizeof(int));
  emerr = emnonum = emig = emng = FALSE;

  first = TRUE;
  while (getdata(fp) == 0) {
    GRAcolor(GC, fp->col.r, fp->col.g, fp->col.b, fp->col.a);
    if (fp->dxstat == MATH_VALUE_NORMAL && fp->dystat == MATH_VALUE_NORMAL) {
      if (first) {
        first = FALSE;
	x0 = fp->dx;
	y0 = fp->dy;
	x2 = fp->dx;
	y2 = fp->dy;
      } else {
	x1 = x2;
	y1 = y2;
	x2 = fp->dx;
	y2 = fp->dy;
	add_polygon_point(&pos, x1, y1, x2, y2, fp);
      }
    } else {
      if (fp->dxstat != MATH_VALUE_CONT && fp->dystat != MATH_VALUE_CONT) {
	if (! first) {
	  add_polygon_point(&pos, x2, y2, x0, y0, fp);
	}
	draw_polygon(&pos, GC, GRA_FILL_MODE_WINDING);
        arraydel(&pos);
        first = TRUE;
      }
      errordisp(obj, fp, &emerr, &emnonum, &emig, &emng);
    }
  }

  if (! first) {
    add_polygon_point(&pos, x2, y2, x0, y0, fp);
  }
  draw_polygon(&pos, GC, GRA_FILL_MODE_WINDING);
  arraydel(&pos);

  errordisp(obj, fp, &emerr, &emnonum, &emig, &emng);
  return 0;
}

#define FREE_INTP_BUF()				\
  g_free(x);  g_free(y);  g_free(z);		\
  g_free(r);  g_free(g);  g_free(b); g_free(a);	\
  g_free(c1); g_free(c2); g_free(c3);		\
  g_free(c4); g_free(c5); g_free(c6);

static int
curveout(struct objlist *obj,struct f2ddata *fp,int GC,
	 int width,int snum,int *style,
	 int join,int miter,int intp)
{
  int emerr,emnonum,emig,emng;
  int j,num;
  int first;
  double *x,*y,*z,*c1,*c2,*c3,*c4,*c5,*c6,count;
  int *r,*g,*b,*a;
  double c[8];
  double bs1[7],bs2[7],bs3[4],bs4[4];
  int bsr[7],bsg[7],bsb[7],bsa[7],bsr2[4],bsg2[4],bsb2[4],bsa2[4];
  int spcond;

  emerr=emnonum=emig=emng=FALSE;
#if EXPAND_DOTTED_LINE
  GRAlinestyle(GC,0,NULL,width,GRA_LINE_CAP_BUTT,join,miter);
#else
  GRAlinestyle(GC, snum, style, width, GRA_LINE_CAP_BUTT, join, miter);
#endif
  switch (intp) {
  case INTERPOLATION_TYPE_SPLINE:
  case INTERPOLATION_TYPE_SPLINE_CLOSE:
    num=0;
    count=0;
    x=y=z=c1=c2=c3=c4=c5=c6=NULL;
    r=g=b=a=NULL;
    while (getdata(fp)==0) {
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
      && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
        if (dataadd(fp->dx,fp->dy,count,fp->col.r,fp->col.g,fp->col.b,fp->col.a,&num,
		    &x,&y,&z,&r,&g,&b,&a,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) return -1;
        count++;
      } else {
        if ((fp->dxstat!=MATH_VALUE_CONT) && (fp->dystat!=MATH_VALUE_CONT)) {
          if (num>=2) {
            if (intp==INTERPOLATION_TYPE_SPLINE) {
	      spcond=SPLCND2NDDIF;
	    } else {
              spcond=SPLCNDPERIODIC;
              if ((x[num-1]!=x[0]) || (y[num-1]!=y[0])) {
                if (dataadd(x[0],y[0],count,r[0],g[0],b[0],a[0],&num,
			    &x,&y,&z,&r,&g,&b,&a,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) return -1;
              }
            }
            if (spline(z,x,c1,c2,c3,num,spcond,spcond,0,0)
             || spline(z,y,c4,c5,c6,num,spcond,spcond,0,0)) {
	      FREE_INTP_BUF();
              error(obj,ERRSPL);
              return -1;
            }
            GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,
                          f2dsplinedif,splineint,fp,x[0],y[0]);
            for (j=0;j<num-1;j++) {
              c[0]=c1[j]; c[1]=c2[j]; c[2]=c3[j];
              c[3]=c4[j]; c[4]=c5[j]; c[5]=c6[j];
              GRAcolor(GC,r[j],g[j],b[j], a[j]);
              if (!GRAcurve(GC,c,x[j],y[j])) break;
            }
          }
	  FREE_INTP_BUF();
          num=0;
          count=0;
          x=y=z=c1=c2=c3=c4=c5=c6=NULL;
          r=g=b=a=NULL;
        }
        errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
      }
    }
    if (num!=0) {
      if (intp==INTERPOLATION_TYPE_SPLINE) {
	spcond=SPLCND2NDDIF;
      } else {
        spcond=SPLCNDPERIODIC;
        if ((x[num-1]!=x[0]) || (y[num-1]!=y[0])) {
          if (dataadd(x[0],y[0],count,r[0],g[0],b[0],a[0],&num,
		      &x,&y,&z,&r,&g,&b,&a,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) return -1;
        }
      }
      if (spline(z,x,c1,c2,c3,num,spcond,spcond,0,0)
       || spline(z,y,c4,c5,c6,num,spcond,spcond,0,0)) {
	FREE_INTP_BUF();
        error(obj,ERRSPL);
        return -1;
      }
      GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,
                    f2dsplinedif,splineint,fp,x[0],y[0]);
      for (j=0;j<num-1;j++) {
        c[0]=c1[j]; c[1]=c2[j]; c[2]=c3[j];
        c[3]=c4[j]; c[4]=c5[j]; c[5]=c6[j];
        GRAcolor(GC,r[j],g[j],b[j], a[j]);
        if (!GRAcurve(GC,c,x[j],y[j])) break;
      }
    }
    FREE_INTP_BUF();
    break;
  case INTERPOLATION_TYPE_BSPLINE:
    first=TRUE;
    num=0;
    while (getdata(fp)==0) {
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
      && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
        if (first) {
          bs1[num]=fp->dx;
          bs2[num]=fp->dy;
          bsr[num]=fp->col.r;
          bsg[num]=fp->col.g;
          bsb[num]=fp->col.b;
          bsa[num]=fp->col.a;
          num++;
          if (num>=7) {
            for (j=0;j<2;j++) {
              bspline(j+1,bs1+j,c);
              bspline(j+1,bs2+j,c+4);
              if (j==0) {
                GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,
                              f2dbsplinedif,bsplineint,fp,c[0],c[4]);
              }
              GRAcolor(GC,bsr[j],bsg[j],bsb[j],bsa[j]);
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
            bsa[j-1]=bsa[j];
          }
          bs1[6]=fp->dx;
          bs2[6]=fp->dy;
          bsr[6]=fp->col.r;
          bsg[6]=fp->col.g;
          bsb[6]=fp->col.b;
          bsa[6]=fp->col.a;
          num++;
          bspline(0,bs1+1,c);
          bspline(0,bs2+1,c+4);
          GRAcolor(GC,bsr[1],bsg[1],bsb[1],bsa[1]);
          if (!GRAcurve(GC,c,c[0],c[4])) return -1;
        }
      } else {
        if ((fp->dxstat!=MATH_VALUE_CONT) && (fp->dystat!=MATH_VALUE_CONT)) {
          if (!first) {
            for (j=0;j<2;j++) {
              bspline(j+3,bs1+j+2,c);
              bspline(j+3,bs2+j+2,c+4);
              GRAcolor(GC,bsr[j+2],bsg[j+2],bsb[j+2],bsa[j+2]);
              if (!GRAcurve(GC,c,c[0],c[4])) return -1;
            }
          }
          first=TRUE;
          num=0;
        }
        errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
      }
    }
    if (!first) {
      for (j=0;j<2;j++) {
        bspline(j+3,bs1+j+2,c);
        bspline(j+3,bs2+j+2,c+4);
        GRAcolor(GC,bsr[j+2],bsg[j+2],bsb[j+2],bsa[j+2]);
        if (!GRAcurve(GC,c,c[0],c[4])) return -1;
      }
    }
    break;
  case INTERPOLATION_TYPE_BSPLINE_CLOSE:
    first=TRUE;
    num=0;
    while (getdata(fp)==0) {
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
      && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
        if (first) {
          bs1[num]=fp->dx;
          bs3[num]=fp->dx;
          bs2[num]=fp->dy;
          bs4[num]=fp->dy;
          bsr[num]=fp->col.r;
          bsg[num]=fp->col.g;
          bsb[num]=fp->col.b;
          bsa[num]=fp->col.a;
          bsr2[num]=fp->col.r;
          bsg2[num]=fp->col.g;
          bsb2[num]=fp->col.b;
          bsa2[num]=fp->col.a;
          num++;
          if (num>=4) {
            bspline(0,bs1,c);
            bspline(0,bs2,c+4);
            GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,
                          f2dbsplinedif,bsplineint,fp,c[0],c[4]);
            GRAcolor(GC,bsr[0],bsg[0],bsb[0],bsa[0]);
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
            bsa[j-1]=bsa[j];
          }
          bs1[3]=fp->dx;
          bs2[3]=fp->dy;
          bsr[3]=fp->col.r;
          bsg[3]=fp->col.g;
          bsb[3]=fp->col.b;
          bsa[3]=fp->col.a;
          num++;
          bspline(0,bs1,c);
          bspline(0,bs2,c+4);
          GRAcolor(GC,bsr[0],bsg[0],bsb[0],bsa[0]);
          if (!GRAcurve(GC,c,c[0],c[4])) return -1;
        }
      } else {
        if ((fp->dxstat!=MATH_VALUE_CONT) && (fp->dystat!=MATH_VALUE_CONT)) {
          if (!first) {
            for (j=0;j<3;j++) {
              bs1[4+j]=bs3[j];
              bs2[4+j]=bs4[j];
              bsr[4+j]=bsr2[j];
              bsg[4+j]=bsg2[j];
              bsb[4+j]=bsb2[j];
              bsa[4+j]=bsa2[j];
              bspline(0,bs1+j+1,c);
              bspline(0,bs2+j+1,c+4);
              GRAcolor(GC,bsr[j+1],bsg[j+1],bsb[j+1],bsa[j+1]);
              if (!GRAcurve(GC,c,c[0],c[4])) return -1;
            }
          }
          first=TRUE;
          num=0;
        }
        errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
      }
    }
    if (!first) {
      for (j=0;j<3;j++) {
        bs1[4+j]=bs3[j];
        bs2[4+j]=bs4[j];
        bsr[4+j]=bsr2[j];
        bsg[4+j]=bsg2[j];
        bsb[4+j]=bsb2[j];
        bsa[4+j]=bsa2[j];
        bspline(0,bs1+j+1,c);
        bspline(0,bs2+j+1,c+4);
        GRAcolor(GC,bsr[j+1],bsg[j+1],bsb[j+1],bsa[j+1]);
        if (!GRAcurve(GC,c,c[0],c[4])) return -1;
      }
    }
    break;
  }
  errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
  return 0;
}

static void
draw_arrow(struct f2ddata *fp ,int GC, double x0, double y0, double x1, double y1, int msize, struct line_position *lp)
{
  int gx0, gy0, gx1, gy1;
  double d2, d3, headlen, headwidth;

  headlen = 72426;
  headwidth = 60000;

  d2 = x1;
  d3 = y1;
  if (f2dlineclipf(&x0, &y0, &x1, &y1, fp)) {
    return;
  }
  f2dtransf(x0, y0, &gx0, &gy0, fp);
  f2dtransf(x1, y1, &gx1, &gy1, fp);
  if ((x1 == d2) && (y1 == d3) && (msize > 0)) {
    double dx, dy, len, alen, awidth;
    alen = msize;
    awidth = alen * headwidth / headlen / 2.0;
    dx = gx1-gx0;
    dy = gy1-gy0;
    len = sqrt(dx*dx+dy*dy);
    if (len > 0) {
      int ax0, ay0, ap[8];
      ax0 = nround(gx1 - dx * alen / len);
      ay0 = nround(gy1 - dy * alen / len);
      ap[0] = nround(ax0 - dy / len * awidth);
      ap[1] = nround(ay0 + dx / len * awidth);
      ap[2] = gx1;
      ap[3] = gy1;
      ap[4] = nround(ax0 + dy / len * awidth);
      ap[5] = nround(ay0 - dx / len * awidth);
      GRAdrawpoly(GC, 3, ap, GRA_FILL_MODE_EVEN_ODD);
      lp->x0 = gx0;
      lp->y0 = gy0;
      lp->x1 = ax0;
      lp->y1 = ay0;
    }
  } else {
    lp->x0 = gx0;
    lp->y0 = gy0;
    lp->x1 = gx1;
    lp->y1 = gy1;
  }
}

static int
rectout(struct objlist *obj,struct f2ddata *fp,int GC,
	int width,int snum,int *style,int type)
{
  int emerr,emnonum,emig,emng;
  double x0,y0,x1,y1;
  int gx0,gy0,gx1,gy1;
  int ap[8];

  emerr=emnonum=emig=emng=FALSE;

  if (type == PLOT_TYPE_DIAGONAL) GRAlinestyle(GC,snum,style,width,GRA_LINE_CAP_BUTT,GRA_LINE_JOIN_MITER,1000);
  else GRAlinestyle(GC,snum,style,width,GRA_LINE_CAP_PROJECTING,GRA_LINE_JOIN_MITER,1000);
  while (getdata(fp)==0) {
    GRAcolor(GC,fp->col.r,fp->col.g,fp->col.b, fp->col.a);
    if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
     && (fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL)
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
	struct line_position lp;
	draw_arrow(fp, GC, fp->dx, fp->dy, fp->d2, fp->d3, fp->msize, &lp);
	GRAline(GC, lp.x0, lp.y0, lp.x1, lp.y1);
      }
      if (type == PLOT_TYPE_RECTANGLE_FILL || type == PLOT_TYPE_RECTANGLE_SOLID_FILL) {
        if (type == PLOT_TYPE_RECTANGLE_FILL) {
	  GRAcolor(GC, fp->col2.r, fp->col2.g, fp->col2.b, fp->col2.a);
	}
        x0=fp->dx;
        y0=fp->dy;
        x1=fp->d2;
        y1=fp->d3;
        if (f2drectclipf(&x0,&y0,&x1,&y1,fp)==0) {
	  f2dtransf(x0, y0, ap + 0, ap + 1, fp);
	  f2dtransf(x0, y1, ap + 2, ap + 3, fp);
	  f2dtransf(x1, y1, ap + 4, ap + 5, fp);
	  f2dtransf(x1, y0, ap + 6, ap + 7, fp);
	  GRAdrawpoly(GC, 4, ap, GRA_FILL_MODE_EVEN_ODD);
        }
        if (type == PLOT_TYPE_RECTANGLE_FILL) {
	  GRAcolor(GC,fp->col.r,fp->col.g,fp->col.b, fp->col.a);
	}
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
    } else errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
  }
  errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
  return 0;
}

static void
draw_errorbar(struct f2ddata *fp, int GC, int size, double dx0, double dy0, double  dx1, double dy1)
{
  double x0, y0, x1, y1, x, y, r, dirx, diry;
  int gx0, gy0, gx1, gy1;

  x0 = dx0;
  y0 = dy0;
  x1 = dx1;
  y1 = dy1;
  if (f2dlineclipf(&x0, &y0, &x1, &y1, fp)) {
    return;
  }

  f2dtransf(x0, y0, &gx0, &gy0, fp);
  f2dtransf(x1, y1, &gx1, &gy1, fp);
  x = gx1 - gx0;
  y = gy1 - gy0;
  r = sqrt(x * x + y * y);
  if (r == 0) {
    return;

  }
  dirx = x / r;
  diry = y / r;

  GRAline(GC, gx0, gy0, gx1, gy1);
  if (dx0 == x0 && dy0 == y0) {
    GRAline(GC,
            gx0 + nround(size * diry),
            gy0 - nround(size * dirx),
            gx0 - nround(size * diry),
            gy0 + nround(size * dirx));
  }
  if (dx1 == x1 && dy1 == y1) {
      GRAline(GC,
              gx1 + nround(size * diry),
              gy1 - nround(size * dirx),
              gx1 - nround(size * diry),
              gy1 + nround(size * dirx));
  }
}

static int
errorbarout(struct objlist *obj,struct f2ddata *fp,int GC,
	    int width,int snum,int *style,int type)
{
  int emerr,emnonum,emig,emng;

  emerr=emnonum=emig=emng=FALSE;
  GRAlinestyle(GC,snum,style,width,GRA_LINE_CAP_BUTT,GRA_LINE_JOIN_MITER,1000);
  while (getdata(fp)==0) {
    int size;
    size=fp->marksize0/2;
    GRAcolor(GC,fp->col.r,fp->col.g,fp->col.b, fp->col.a);
    if (type == PLOT_TYPE_ERRORBAR_X) {
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
       && (fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL)
       && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)
       && (getposition2(fp,fp->axtype,fp->axtype,&(fp->d2),&(fp->d3))==0)) {
        draw_errorbar(fp, GC, size, fp->d2, fp->dy, fp->d3, fp->dy);
      } else errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
    } else if (type == PLOT_TYPE_ERRORBAR_Y) {
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
       && (fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL)
       && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)
       && (getposition2(fp,fp->aytype,fp->aytype,&(fp->d2),&(fp->d3))==0)) {
        draw_errorbar(fp, GC, size, fp->dx, fp->d2, fp->dx, fp->d3);
      } else errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
    }
  }
  errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
  return 0;
}

static int
stairout(struct objlist *obj,struct f2ddata *fp,int GC,
	 int width,int snum,int *style,
	 int join,int miter,int type)
{
  int emerr,emnonum,emig,emng;
  int num;
  double x0,y0,x1,y1,x,y,dx,dy;

  emerr=emnonum=emig=emng=FALSE;
#if EXPAND_DOTTED_LINE
  GRAlinestyle(GC,0,NULL,width,GRA_LINE_CAP_BUTT,join,miter);
#else
  GRAlinestyle(GC, snum, style, width, GRA_LINE_CAP_BUTT, join, miter);
#endif
  num=0;
  while (getdata(fp)==0) {
    GRAcolor(GC,fp->col.r,fp->col.g,fp->col.b, fp->col.a);
    if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
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
      if ((fp->dxstat!=MATH_VALUE_CONT) && (fp->dystat!=MATH_VALUE_CONT)) {
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
      errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
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
  errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
  return 0;
}

static void
draw_rect(int GC, int *ap, double v0, double v1, double v2, double v3)
{
  GRAline(GC, ap[0], ap[1], ap[2] ,ap[3]);
  if (v0 == v1) {
    GRAline(GC, ap[2], ap[3], ap[4] ,ap[5]);
  }
  GRAline(GC, ap[4], ap[5], ap[6] ,ap[7]);
  if (v2 == v3) {
    GRAline(GC, ap[6], ap[7], ap[0] ,ap[1]);
  }
}

static int
barout(struct objlist *obj,struct f2ddata *fp,int GC,
       int width,int snum,int *style,int type)
{
  int emerr,emnonum,emig,emng;
  double x0,y0,x1,y1;
  int gx0,gy0,gx1,gy1;
  int ap[8];

  emerr=emnonum=emig=emng=FALSE;
  if (type <= PLOT_TYPE_BAR_FILL_Y) GRAlinestyle(GC,snum,style,width,GRA_LINE_CAP_PROJECTING,GRA_LINE_JOIN_MITER,1000);
  while (getdata(fp)==0) {
    int size;
    size=fp->marksize0/2;
    if (fp->dxstat != MATH_VALUE_NORMAL ||
	fp->dystat != MATH_VALUE_NORMAL ||
	getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))) {
      errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
      continue;
    }
    switch (type) {
    case PLOT_TYPE_BAR_FILL_X:
    case PLOT_TYPE_BAR_SOLID_FILL_X:
    case PLOT_TYPE_BAR_X:
      x0=0;
      y0=fp->dy;
      x1=fp->dx;
      y1=fp->dy;
      if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)) {
	break;
      }
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
      switch (type) {
      case PLOT_TYPE_BAR_X:
	GRAcolor(GC,fp->col.r,fp->col.g,fp->col.b, fp->col.a);
	draw_rect(GC, ap, x1, fp->dx, x0, 0);
	break;
      case PLOT_TYPE_BAR_SOLID_FILL_X:
	GRAcolor(GC,fp->col.r,fp->col.g,fp->col.b, fp->col.a);
	GRAdrawpoly(GC,4,ap,GRA_FILL_MODE_EVEN_ODD);
	break;
      case PLOT_TYPE_BAR_FILL_X:
	GRAcolor(GC, fp->col2.r, fp->col2.g, fp->col2.b, fp->col2.a);
	GRAdrawpoly(GC,4,ap,GRA_FILL_MODE_EVEN_ODD);
	GRAcolor(GC,fp->col.r,fp->col.g,fp->col.b, fp->col.a);
	draw_rect(GC, ap, x1, fp->dx, x0, 0);
	break;
      }
      break;
    case PLOT_TYPE_BAR_FILL_Y:
    case PLOT_TYPE_BAR_SOLID_FILL_Y:
    case PLOT_TYPE_BAR_Y:
      x0=fp->dx;
      y0=0;
      x1=fp->dx;
      y1=fp->dy;
      if (f2dlineclipf(&x0,&y0,&x1,&y1,fp)) {
	break;
      }
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
      switch (type) {
      case PLOT_TYPE_BAR_Y:
	GRAcolor(GC,fp->col.r,fp->col.g,fp->col.b, fp->col.a);
	draw_rect(GC, ap, y1, fp->dy, y0, 0);
	break;
      case PLOT_TYPE_BAR_SOLID_FILL_Y:
	GRAcolor(GC,fp->col.r,fp->col.g,fp->col.b, fp->col.a);
	GRAdrawpoly(GC,4,ap,GRA_FILL_MODE_EVEN_ODD);
	break;
      case PLOT_TYPE_BAR_FILL_Y:
	GRAcolor(GC, fp->col2.r, fp->col2.g, fp->col2.b, fp->col2.a);
	GRAdrawpoly(GC,4,ap,GRA_FILL_MODE_EVEN_ODD);
	GRAcolor(GC,fp->col.r,fp->col.g,fp->col.b, fp->col.a);
	draw_rect(GC, ap, y1, fp->dy, y0, 0);
	break;
      }
      break;
    }
  }
  errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
  return 0;
}

static int
calc_weight(struct objlist *obj, struct f2dlocal *f2dlocal, struct f2ddata *fp, const char *weight, struct narray *data, struct narray *index)
{
  MathEquation *code;
  MathEquationParametar *prm;
  double dd;
  int emerr, emserr, emnonum, emig, emng, two_pass, maxdim, ddstat, rcode, datanum2, i, j;
  int const_id[MATH_CONST_SIZE];

  code = ofile_create_math_equation(const_id, EOEQ_ASSIGN_TYPE_ASSIGN, 3, FALSE, TRUE, FALSE, FALSE, TRUE);
  if (code == NULL) {
    return 1;
  }

  rcode = math_equation_parse(code, weight);
  if (rcode) {
    math_equation_free(code);
    return 1;
  }

  prm = math_equation_get_parameter(code, 0, NULL);
  maxdim = (prm) ? prm->id_max : 0;

  two_pass = math_equation_check_const(code, const_id, TWOPASS_CONST_SIZE);
  if (two_pass) {
    reopendata(fp);
    if (getminmaxdata(fp, f2dlocal) == -1) {
      math_equation_free(code);
      return 1;
    }
  }

  if (maxdim < fp->x) maxdim = fp->x;
  if (maxdim < fp->y) maxdim = fp->y;
  datanum2 = fp->datanum;
  reopendata(fp);
  if (hskipdata(fp) != 0) {
    math_equation_free(code);
    return 1;
  }
  fp->datanum = datanum2;

  if (set_const(code, const_id, two_pass, fp, TRUE)) {
    math_equation_free(code);
    return 1;
  }

  emerr = emserr = emnonum = emig = emng = FALSE;
  for (i = j = 0; getdata2(fp, code, maxdim, &dd, &ddstat) == 0; i++) {
    int *line;
    line = (int *) arraynget(index, j);

    if (line == NULL) {
      break;
    } else if (*line != i) {
      continue;
    }

    j++;
    if (ddstat == MATH_VALUE_NORMAL) {
      if (arrayadd(data, &dd) == NULL) {
	return -1;
      }
    } else {
      errordisp2(obj, fp, &emerr, &emnonum, &emig, &emng, ddstat, "weight");
    }
  }

  math_equation_free(code);

  if (arraynum(data) == 0) {
    return -1;
  }

  return 0;
}

static int
calc_fit(struct objlist *obj, struct f2dlocal *f2dlocal, struct f2ddata *fp, struct objlist *fitobj, int fit_id)
{
  int emerr, emnonum, emig, emng, i;
  struct narray data, index;
  char *weight, *argv[2];
  double dnum;

  arrayinit(&data,sizeof(double));
  arrayinit(&index,sizeof(int));
  emerr = emnonum = emig = emng = FALSE;

  for (i = 0; getdata(fp)==0; i++) {
    if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)) {
      if (arrayadd(&data, &fp->dx) == NULL ||
	  arrayadd(&data, &fp->dy) == NULL ||
	  arrayadd(&index, &i) == NULL) {
	arraydel(&index);
	arraydel(&data);
	return -1;
      }
    } else {
      errordisp(obj, fp, &emerr, &emnonum, &emig, &emng);
    }
  }

  errordisp(obj, fp, &emerr, &emnonum, &emig, &emng);
  if ((dnum=(double )arraynum(&data))==0) {
    arraydel(&index);
    arraydel(&data);
    return -1;
  }

  dnum /= 2;
  if (arrayins(&data, &dnum, 0) == NULL) {
    arraydel(&index);
    arraydel(&data);
    return -1;
  }

  if (getobj(fitobj, "weight_func", fit_id, 0, NULL, &weight) == -1) {
    arraydel(&index);
    arraydel(&data);
    return -1;
  }

  if (weight) {
    int rcode;
    rcode = calc_weight(obj, f2dlocal, fp, weight, &data, &index);
    if (rcode) {
      arraydel(&index);
      arraydel(&data);
      return rcode;
    }
  }
  arraydel(&index);

  argv[0] = (void *) (&data);
  argv[1] = NULL;
  if (exeobj(fitobj, "fit", fit_id, 1, argv)) {
    arraydel(&data);
    return -1;
  }
  arraydel(&data);

  return 0;
}

#define MATH_EQUATION_FREE(eq) math_equation_free(eq)

static int
draw_interpolation(struct f2ddata *fp, int GC, int num, int snum, int *style,
		   double *c, double *x, double *y, double *z,
		   double *c1, double *c2, double *c3,
		   double *c4, double *c5, double *c6)
{
  int j, spcond;

  spcond = SPLCND2NDDIF;
  if (spline(z, x, c1, c2, c3, num, spcond, spcond, 0, 0) ||
      spline(z, y, c4, c5, c6, num, spcond, spcond, 0, 0)) {
    return -1;
  }
  GRAcurvefirst(GC, snum, style, f2dlineclipf, f2dtransf,
		f2dsplinedif, splineint, fp, x[0], y[0]);
  for (j = 0; j < num - 1; j++) {
    c[0] = c1[j];
    c[1] = c2[j];
    c[2] = c3[j];
    c[3] = c4[j];
    c[4] = c5[j];
    c[5] = c6[j];

    if (GRAcurve(GC, c, x[j], y[j]) == 0) {
      break;
    }
  }

  return 0;
}

static int
draw_fit(struct objlist *obj, struct f2ddata *fp,
	 int GC, struct objlist *fitobj, N_VALUE *fit_inst,
	 int width, int snum, int *style, int join, int miter)
{
  char *equation;
  double min, max, dx, dy;
  int i, div, interpolation, first, rcode, num, emerr;
  int *r, *g, *b, *a;
  double c[8], *x, *y, *z, *c1, *c2, *c3, *c4, *c5, *c6, count;
  MathEquation *code;
  MathValue val;

  if (_getobj(fitobj, "equation", fit_inst, &equation)) return -1;
  if (_getobj(fitobj, "min", fit_inst, &min)) return -1;
  if (_getobj(fitobj, "max", fit_inst, &max)) return -1;
  if (_getobj(fitobj, "div", fit_inst, &div)) return -1;
  if (_getobj(fitobj, "interpolation", fit_inst, &interpolation)) return -1;
  if (equation==NULL) return -1;
  if ((min==0) && (max==0)) {
    min=fp->axmin2;
    max=fp->axmax2;
  } else if (min==max) return 0;
  code = ofile_create_math_equation(NULL, EOEQ_ASSIGN_TYPE_ASSIGN, 0, FALSE, FALSE, FALSE, FALSE, TRUE);
  if (code == NULL) {
    return 1;
  }

  rcode = math_equation_parse(code, equation);
  if (rcode) {
    math_equation_free(code);
    return 1;
  }

  rcode = math_equation_optimize(code);
  if (rcode) {
    math_equation_free(code);
    return 1;
  }

  if (code->exp == NULL) {
    math_equation_free(code);
    return 1;
  }

  GRAcolor(GC,fp->color.r,fp->color.g,fp->color.b, fp->color.a);
#if EXPAND_DOTTED_LINE
  GRAlinestyle(GC,0,NULL,width,GRA_LINE_CAP_BUTT,join,miter);
#else
  GRAlinestyle(GC, snum, style, width, GRA_LINE_CAP_BUTT, join, miter);
#endif
  num=0;
  count=0;
  emerr=FALSE;
  x=y=z=c1=c2=c3=c4=c5=c6=NULL;
  r=g=b=a=NULL;
  first=TRUE;
  for (i=0;i<=div;i++) {
    dx=min+(max-min)/div*i;
    val.val = dx;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_var(code, 0, &val);
    math_equation_calculate(code, &val);
    dy = val.val;
    if (interpolation) {
      if ((val.type==MATH_VALUE_NORMAL)
      && (getposition2(fp,fp->axtype,fp->aytype,&dx,&dy)==0)) {
        if (dataadd(dx,dy,count,0,0,0,255,&num,
                    &x,&y,&z,&r,&g,&b,&a,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) {
	  MATH_EQUATION_FREE(code);
          return -1;
        }
        count++;
      } else {
        if (num >= 2 && draw_interpolation(fp, GC, num, snum, style, c, x, y, z, c1, c2, c3, c4, c5, c6)) {
	  FREE_INTP_BUF();
	  MATH_EQUATION_FREE(code);
	  error(obj,ERRSPL);
	  return -1;
        }
	FREE_INTP_BUF();
        num=0;
        count=0;
        x=y=z=c1=c2=c3=c4=c5=c6=NULL;
        r=g=b=a=NULL;
      }
    } else {
      if ((val.type==MATH_VALUE_NORMAL) && (getposition2(fp,fp->axtype,fp->aytype,&dx,&dy)==0)) {
        if (first) {
          GRAcurvefirst(GC,snum,style,f2dlineclipf,f2dtransf,NULL,NULL,fp,dx,dy);
          first=FALSE;
        } else {
	  GRAdashlinetod(GC,dx,dy);
	}
      } else {
	first=TRUE;
      }
    }
    if ((!emerr) && (val.type!=MATH_VALUE_NORMAL) && (val.type!=MATH_VALUE_UNDEF)) {
      error(obj,ERRMERR);
      emerr=TRUE;
    }
  }
  MATH_EQUATION_FREE(code);

  if (interpolation) {
    if (num > 0 && draw_interpolation(fp, GC, num, snum, style, c, x, y, z, c1, c2, c3, c4, c5, c6)) {
      FREE_INTP_BUF();
      error(obj,ERRSPL);
      return -1;
    }
    FREE_INTP_BUF();
  }

  return 0;
}

static int
get_fit_obj_id(char *fit, struct objlist **fitobj, N_VALUE **inst)
{
  struct narray iarray;
  int anum, id;

  if (fit == NULL) {
    return -1;
  }

  arrayinit(&iarray, sizeof(int));
  if (getobjilist(fit, fitobj, &iarray, FALSE, NULL)) {
    return -1 ;
  }
  anum = arraynum(&iarray);
  if (anum < 1) {
    arraydel(&iarray);
    return -1 ;
  }

  id = arraylast_int(&iarray);
  arraydel(&iarray);

  *inst = getobjinst(*fitobj, id);
  if (*inst == NULL) {
    return -1 ;
  }

  return id;
}

static void
dummyout(struct f2ddata *fp, int GC, int width, int snum, int *style, int join, int miter)
{
  GRAlinestyle(GC, snum, style, width, GRA_LINE_CAP_BUTT, join, miter);
  while (! getdata(fp));
}

static int
fitout(struct objlist *obj,struct f2dlocal *f2dlocal,
       struct f2ddata *fp,int GC,
       int width,int snum,int *style,
       int join,int miter,char *fit,int redraw)
{
  struct objlist *fitobj;
  int id;
  N_VALUE *inst;
  char *equation;

  if (fit == NULL) {
    error(obj, ERRNOFIT);
    return -1;
  }

  id = get_fit_obj_id(fit, &fitobj, &inst);
  if (id < 0) {
    error2(obj, ERRNOFITINST, fit);
    return -1;
  }

  if (_getobj(fitobj, "equation", inst, &equation)) {
    return -1;
  }

  if (equation == NULL && redraw == 0) {
    int rcode;
    f2dlocal->use_drawing_func = FALSE;
    rcode = calc_fit(obj, f2dlocal, fp, fitobj, id);
    if (rcode) {
      return rcode;
    }
  } else if (f2dlocal->use_drawing_func) {
    dummyout(fp, GC, width, snum, style, join, miter);
  }

  return draw_fit(obj, fp, GC, fitobj, inst, width, snum, style, join, miter);
}

static int
f2ddraw(struct objlist *obj, N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  int GC;
  int type, src;
  int mtype;
  int lwidth, ljoin, lmiter, intp;
  struct narray *lstyle;
  int snum, *style;
  struct f2ddata *fp;
  int rcode;
  int w, h, clip, zoom;
  char *fit, *field, *array;
  char *file;
  struct array_prm ary;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  _getobj(obj, "_local", inst, &f2dlocal);
  _getobj(obj, "source", inst, &src);
  _getobj(obj, "GC", inst, &GC);
  if (GC<0)
    return 0;

  switch (src) {
  case DATA_SOURCE_FILE:
   _getobj(obj, "file", inst, &file);
   if (file == NULL){
     return 0;
   }
   break;
  case DATA_SOURCE_ARRAY:
    _getobj(obj,"array", inst, &array);
    open_array(array, &ary);
    if (ary.data_num < 1) {
      return 0;
    }
    break;
  }

  _getobj(obj, "type", inst, &type);
  _getobj(obj, "mark_type", inst, &mtype);
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
  if (fp == NULL) {
    return 1;
  }

  fp->GC = GC;
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

  if (set_const_all(fp))
    return 1;

  GRAregion(GC, &w, &h, &zoom);
  GRAview(GC, 0, 0, w*10000.0/zoom, h*10000.0/zoom, clip);
  switch (type) {
  case PLOT_TYPE_MARK:
    rcode = markout(obj, fp, GC, lwidth, snum, style);
    break;
  case PLOT_TYPE_LINE:
    rcode = lineout(obj, fp, GC, lwidth, snum, style, ljoin, lmiter, FALSE);
    break;
  case PLOT_TYPE_POLYGON:
    rcode = lineout(obj, fp, GC, lwidth, snum, style, ljoin, lmiter, TRUE);
    break;
  case PLOT_TYPE_POLYGON_SOLID_FILL:
    rcode = polyout(obj, fp, GC);
    break;
  case PLOT_TYPE_CURVE:
    rcode = curveout(obj, fp, GC, lwidth, snum, style, ljoin, lmiter, intp);
    break;
  case PLOT_TYPE_DIAGONAL:
  case PLOT_TYPE_ARROW:
  case PLOT_TYPE_RECTANGLE:
  case PLOT_TYPE_RECTANGLE_FILL:
  case PLOT_TYPE_RECTANGLE_SOLID_FILL:
    rcode = rectout(obj, fp, GC, lwidth, snum, style, type);
    break;
  case PLOT_TYPE_ERRORBAR_X:
    rcode = errorbarout(obj, fp, GC, lwidth, snum, style, type);
    break;
  case PLOT_TYPE_ERRORBAR_Y:
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
    rcode = barout(obj, fp, GC, lwidth, snum, style, type);
    break;
  case PLOT_TYPE_FIT:
    field = (char *)argv[1];
    rcode = fitout(obj, f2dlocal, fp, GC, lwidth, snum, style, ljoin, lmiter, fit, field[0] == 'r');
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

  return 0;
}

static int
f2dgetcoord(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  double x,y;
  int gx,gy;
  int id;
  double ip1,ip2;
  int dataclip;
  double minx,maxx,miny,maxy;
  double v1x,v1y,v2x,v2y,vx,vy;
  double a,b,c,d;
  struct narray *array;
  struct axis_prm ax_prm, ay_prm;

  x = arg_to_double(argv, 2);
  y = arg_to_double(argv, 3);
  _getobj(obj,"data_clip",inst,&dataclip);

  id = get_axis_prm(obj, inst, AXIS_X, &ax_prm);
  if (id < 0) {
    return 1;
  }

  id = get_axis_prm(obj, inst, AXIS_Y, &ay_prm);
  if (id < 0) {
    return 1;
  }

  ip1=-ax_prm.vy*ay_prm.vx+ax_prm.vx*ay_prm.vy;
  ip2=-ay_prm.vy*ax_prm.vx+ay_prm.vx*ax_prm.vy;
  if ((fabs(ip1)<=1e-15) || (fabs(ip2)<=1e-15)) return 1;

  gx=gy=0;
  minx=ax_prm.min;
  maxx=ax_prm.max;
  miny=ay_prm.min;
  maxy=ay_prm.max;

  if (ax_prm.type == AXIS_TYPE_LOG) {
    if (x == 0) {
      return -1;
    } else if (x < 0) {
      x = fabs(x);
    }
    x = log10(x);
  } else if (ax_prm.type == AXIS_TYPE_INVERSE) {
    if (x == 0) {
      return -1;
    }
    x = 1 / x;
  }
  if (ay_prm.type == AXIS_TYPE_LOG) {
    if (y == 0) {
      return -1;
    } else if (y < 0) {
      y = fabs(y);
    }
    y = log10(y);
  } else if (ay_prm.type == AXIS_TYPE_INVERSE) {
    if (y == 0) {
      return -1;
    }
    y = 1 / y;
  }

  if (dataclip &&
  ((((minx>x) || (x>maxx)) && ((maxx>x) || (x>minx)))
   || (((miny>y) || (y>maxy)) && ((maxy>y) || (y>miny))))) return 1;
  v1x=ax_prm.rate*(x-minx)*ax_prm.vx;
  v1y=ax_prm.rate*(x-minx)*ax_prm.vy;
  v2x=ay_prm.rate*(y-miny)*ay_prm.vx;
  v2y=ay_prm.rate*(y-miny)*ay_prm.vy;
  vx=ay_prm.posx-ax_prm.posx+v2x-v1x;
  vy=ay_prm.posy-ax_prm.posy+v2y-v1y;
  a=ay_prm.vy*ax_prm.vx-ay_prm.vx*ax_prm.vy;
  c=-ay_prm.vy*vx+ay_prm.vx*vy;
  b=ax_prm.vy*ay_prm.vx-ax_prm.vx*ay_prm.vy;
  d=ax_prm.vy*vx-ax_prm.vx*vy;
  if ((fabs(a)<=1e-16) && (fabs(b)<=1e-16)) {
    return -1;
  } else if (fabs(b)<=1e-16) {
    a=c/a;
    gx=ay_prm.posx+nround(v2x+a*ax_prm.vx);
    gy=ay_prm.posy+nround(v2y+a*ax_prm.vy);
  } else {
    b=d/b;
    gx=ax_prm.posx+nround(v1x+b*ay_prm.vx);
    gy=ax_prm.posy+nround(v1y+b*ay_prm.vy);
  }

  array=rval->array;
  if (arraynum(array) > 0) {
    arraydel(array);
  }
  if ((array==NULL) && ((array=arraynew(sizeof(double)))==NULL)) return 1;
  arrayins(array,&gy,0);
  arrayins(array,&gx,0);
  if (arraynum(array)==0) {
    arrayfree(array);
    rval->array = NULL;
    return 1;
  }
  rval->array=array;
  return 0;
}

static int
f2devaluate(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;
  char *str;
  struct narray *array;
  int gx0,gy0,gx1,gy1,num,limit;
  int minx,miny,maxx,maxy,err;
  double line,dx,dy,d2,d3;
  int src;

  array=rval->array;
  arrayfree(array);
  array=NULL;
  rval->array=array;

  _getobj(obj,"source", inst, &src);
  switch (src) {
  case DATA_SOURCE_FILE:
    _getobj(obj, "file", inst, &str);
    if (str == NULL) {
      return 0;
    }
    break;
  case DATA_SOURCE_ARRAY:
    _getobj(obj, "array", inst, &str);
    if (str == NULL) {
      return 0;
    }
    break;
  case DATA_SOURCE_RANGE:
    break;
  }

  _getobj(obj,"_local",inst,&f2dlocal);
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

  if (set_const_all(fp))
    return 1;

  while (getdata(fp)==0) {
    switch (fp->type) {
    case TYPE_NORMAL:
      dx=fp->dx;
      dy=fp->dy;
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
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
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
      && (fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL)
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
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
      && (fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL)
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
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
       && (fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL)
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
    if ((int) arraynum(array) >= (limit * 3)) break;
  }
  closedata(fp, f2dlocal);
  num=arraynum(array);
  if (num/3>0) rval->array=array;
  else arrayfree(array);
  return 0;
}

static int
f2dredraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int redrawf, num, dmax, type, source, hidden, r;
  int GC;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"hidden",inst,&hidden);
  _getobj(obj,"source",inst,&source);
  _getobj(obj,"redraw_flag",inst,&redrawf);
  _getobj(obj,"data_num",inst,&num);
  _getobj(obj,"redraw_num",inst,&dmax);
  _getobj(obj, "type", inst, &type);

  r = 0;
  if (num > 0 && (dmax == 0 || num <= dmax) && redrawf) {
    r = f2ddraw(obj,inst,rval,argc,argv);
  } else if (source == DATA_SOURCE_RANGE && redrawf) {
    r = f2ddraw(obj,inst,rval,argc,argv);
  } else {
    if (! hidden) {
      system_draw_notify();
    }
    _getobj(obj,"GC",inst,&GC);
    if (GC<0) return 0;
  }
  if (r) {
    system_draw_notify();
  }
  return 0;
}

static int
f2dcolumn_file(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *file,*ifs;
  int csv;
  int cline,ccol;
  int line,col;
  FILE *fd;
  char *buf,*buf2;
  char *po,*po2;

  g_free(rval->str);
  rval->str=NULL;
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
    int rcode;
    if ((rcode=fgetline(fd,&buf))!=0) {
      fclose(fd);
      return 1;
    }
    cline++;
    if (cline==line) {
      if (col==0) rval->str=buf;
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
                if ((buf2=g_malloc(po2-po+1))==NULL) {
                  fclose(fd);
                  g_free(buf);
                  return 1;
                }
                strncpy(buf2,po,po2-po);
                buf2[po2-po]='\0';
                rval->str=buf2;
                break;
              } else {
                for (;*po2==' ';po2++);
                if (strchr(ifs,*po2)!=NULL) po2++;
                po=po2;
              }
            }
          } else {
            for (;(*po!='\0') && (strchr(ifs,*po)!=NULL);po++);
            if (*po=='\0') break;
            for (po2=po;(*po2!='\0') && (strchr(ifs,*po2)==NULL);po2++);
            if (ccol==col) {
              if ((buf2=g_malloc(po2-po+1))==NULL) {
                fclose(fd);
                g_free(buf);
                return 1;
              }
              strncpy(buf2,po,po2-po);
              buf2[po2-po]='\0';
              rval->str=buf2;
              break;
            } else po=po2;
          }
        }
        g_free(buf);
      }
      break;
    } else g_free(buf);
  }
  fclose(fd);
  return 0;
}

static int
f2dcolumn_array(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct array_prm ary;
  int line, col, n;
  char *array;
  double val;

  g_free(rval->str);
  rval->str = NULL;

  line=*(int *)argv[2];
  col=*(int *)argv[3];

  _getobj(obj,"array", inst, &array);
  open_array(array, &ary);

  if (col >= ary.col_num) {
    return 0;
  }

  n = arraynum(ary.ary[col - 1]);

  if (line < 1 || line >= n) {
    return 0;
  }

  val = arraynget_double(ary.ary[col - 1], line - 1);

  rval->str = g_strdup_printf(DOUBLE_STR_FORMAT, val);

  return 0;
}

static int
f2dcolumn_range(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int line, col, div;
  double val, min, max;

  g_free(rval->str);
  rval->str = NULL;

  line=*(int *)argv[2];
  col=*(int *)argv[3];

  _getobj(obj, "range_min", inst, &min);
  _getobj(obj, "range_max", inst, &max);
  _getobj(obj, "range_div", inst, &div);

  if (col >= 2) {
    return 0;
  }

  if (line < 1 || line >= div) {
    return 0;
  }

  val = min + (max - min) / div * line;

  rval->str = g_strdup_printf(DOUBLE_STR_FORMAT, val);

  return 0;
}

static int
f2dcolumn(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int r, src;

  _getobj(obj,"source", inst, &src);
  r = 1;
  switch (src) {
  case DATA_SOURCE_FILE:
    r = f2dcolumn_file(obj, inst, rval, argc, argv);
    break;
  case DATA_SOURCE_ARRAY:
    r = f2dcolumn_array(obj, inst, rval, argc, argv);
    break;
  case DATA_SOURCE_RANGE:
    r = f2dcolumn_range(obj, inst, rval, argc, argv);
    break;
  }

  return r;
}

static int
f2dhead_file(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int cline, line;
  char *file, *ptr;
  GString *s;
  FILE *fd;

  g_free(rval->str);

  rval->str = NULL;

  _getobj(obj, "file", inst, &file);

  line = *(int *) argv[2];

  if (line <= 0)
    return 0;

  if (file == NULL)
    return 0;

  fd = nfopen(file, "rt");
  if (fd == NULL)
    return 0;

  s = g_string_sized_new(256);
  if (s == NULL) {
    fclose(fd);
    return 0;
  }

  for (cline = 0; cline < line; cline++) {
    if (fgetline(fd, &ptr)) {
      break;
    }

    if (cline) {
      s = g_string_append_c(s, '\n');
    }

    g_string_append_printf(s, "%s", ptr);
    g_free(ptr);
  }

  fclose(fd);

  rval->str = g_string_free(s, FALSE);

  return 0;
}

static int
f2dhead(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int r, src;

  _getobj(obj,"source", inst, &src);
  r = 1;
  switch (src) {
  case DATA_SOURCE_FILE:
    r = f2dhead_file(obj, inst, rval, argc, argv);
    break;
  case DATA_SOURCE_ARRAY:
  case DATA_SOURCE_RANGE:
    break;
  }

  return r;
}

static int
f2dsettings_file(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
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
  struct narray *iarray;
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
  g_free(rem);

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
          if (strncmp(f2dtypechar[i], po + 2, strlen(f2dtypechar[i])) == 0) {
	    break;
	  }
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
	    aid = get_axis_id(obj, inst, &aobj, (x) ? AXIS_X : AXIS_Y);
	    if (aid >= 0) {
	      putobj(aobj,"min",aid,&f1);
	      putobj(aobj,"max",aid,&f2);
	      putobj(aobj,"inc",aid,&f3);
            }
          }
        } else err=TRUE;
        break;
      case 'e':
        if ((po[2]=='x') || (po[2]=='y')) {
          for (i=0;i<3;i++)
            if (strncmp(axistypechar[i],po+3,strlen(axistypechar[i]))==0) break;
          if (i!=3) {
	    aid = get_axis_id(obj, inst, &aobj, (po[2]=='x') ? AXIS_X : AXIS_Y);
	    if (aid >= 0) {
	      putobj(aobj,"type",aid,&i);
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
	  if ((s=g_malloc(i-2))!=NULL) {
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
	err=TRUE;
        break;
      }
    } else {
      err=TRUE;
    }
    if (err) {
      error2(obj,ERRILOPTION,buf);
      g_free(buf);
      return 1;
    }
  }
  g_free(buf);
  return 0;
}

static int
f2dsettings(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int r, src;

  _getobj(obj,"source", inst, &src);
  r = 1;
  switch (src) {
  case DATA_SOURCE_FILE:
    r = f2dsettings_file(obj, inst, rval, argc, argv);
    break;
  case DATA_SOURCE_ARRAY:
  case DATA_SOURCE_RANGE:
    break;
  }

  return r;
}

static int
f2dtime(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *file;
  GStatBuf buf;
  int style, src;

  g_free(rval->str);
  rval->str=NULL;

  _getobj(obj,"source", inst, &src);
  if (src != DATA_SOURCE_FILE) {
    error(obj, ERR_INVALID_SOURCE);
    return -1;
  }

  _getobj(obj,"file",inst,&file);
  if (file==NULL) return 0;
  if (nstat(file,&buf)!=0) return 1;
  style=*(int *)(argv[2]);
  rval->str=ntime((time_t *)&(buf.st_mtime),style);
  return 0;
}

static int
f2ddate(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *file;
  GStatBuf buf;
  int style, src;

  g_free(rval->str);
  rval->str=NULL;

  _getobj(obj,"source", inst, &src);
  if (src != DATA_SOURCE_FILE) {
    error(obj, ERR_INVALID_SOURCE);
    return -1;
  }

  _getobj(obj,"file",inst,&file);
  if (file==NULL) return 0;
  if (nstat(file,&buf)!=0) return 1;
  style=*(int *)(argv[2]);
  rval->str=ndate((time_t *)&(buf.st_mtime),style);
  return 0;
}

static void
f2dsettbl(N_VALUE *inst,struct f2dlocal *f2dlocal,struct f2ddata *fp)
{
  int gx,gy,g2,g3;

  switch (fp->type) {
  case TYPE_NORMAL:
    inst[f2dlocal->idx].d=fp->dx;
    inst[f2dlocal->idy].d=fp->dy;
    if (f2dlocal->coord) {
      getposition(fp,fp->dx,fp->dy,&gx,&gy);
      inst[f2dlocal->icx].i=gx;
      inst[f2dlocal->icy].i=gy;
    }
    break;
  case TYPE_DIAGONAL:
    inst[f2dlocal->idx].d=fp->dx;
    inst[f2dlocal->idy].d=fp->dy;
    inst[f2dlocal->id2].d=fp->d2;
    inst[f2dlocal->id3].d=fp->d3;
    if (f2dlocal->coord) {
      getposition(fp,fp->dx,fp->dy,&gx,&gy);
      getposition(fp,fp->d2,fp->d3,&g2,&g3);
      inst[f2dlocal->icx].i=gx;
      inst[f2dlocal->icy].i=gy;
      inst[f2dlocal->ic2].i=g2;
      inst[f2dlocal->ic3].i=g3;
    }
    break;
  case TYPE_ERR_X:
    inst[f2dlocal->idx].d=fp->dx;
    inst[f2dlocal->idy].d=fp->dy;
    inst[f2dlocal->id2].d=fp->d2;
    inst[f2dlocal->id3].d=fp->d3;
    if (f2dlocal->coord) {
      getposition(fp,fp->dx,fp->dy,&gx,&gy);
      getposition(fp,fp->d2,fp->dy,&g2,&gy);
      getposition(fp,fp->d3,fp->dy,&g3,&gy);
      inst[f2dlocal->icx].i=gx;
      inst[f2dlocal->icy].i=gy;
      inst[f2dlocal->ic2].i=g2;
      inst[f2dlocal->ic3].i=g3;
    }
    break;
  case TYPE_ERR_Y:
    inst[f2dlocal->idx].d=fp->dx;
    inst[f2dlocal->idy].d=fp->dy;
    inst[f2dlocal->id2].d=fp->d2;
    inst[f2dlocal->id3].d=fp->d3;
    if (f2dlocal->coord) {
      getposition(fp,fp->dx,fp->dy,&gx,&gy);
      getposition(fp,fp->dx,fp->d2,&gx,&g2);
      getposition(fp,fp->dx,fp->d3,&gx,&g3);
      inst[f2dlocal->icx].i=gx;
      inst[f2dlocal->icy].i=gy;
      inst[f2dlocal->ic2].i=g2;
      inst[f2dlocal->ic3].i=g3;
    }
    break;
  }
  inst[f2dlocal->isx].i=fp->dxstat;
  inst[f2dlocal->isy].i=fp->dystat;
  inst[f2dlocal->is2].i=fp->d2stat;
  inst[f2dlocal->is3].i=fp->d3stat;
  inst[f2dlocal->iline].i=fp->dline;
}

static int
f2dopendata(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
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
f2dgetdata(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
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
f2dclosedata(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                 int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;

  _getobj(obj,"_local",inst,&f2dlocal);
  fp=f2dlocal->data;
  if (fp==NULL) return 0;
  fp->dx=fp->dy=fp->d2=fp->d3=0;
  fp->dxstat=fp->dystat=fp->d2stat=fp->d3stat=MATH_VALUE_MEOF;
  f2dsettbl(inst,f2dlocal,fp);
  closedata(fp, f2dlocal);
  f2dlocal->data=NULL;
  return 0;
}

static int
f2dopendataraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
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
f2dgetdataraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;
  int rcode;
  struct narray *iarray;
  struct narray *darray;
  int i,num,*data,maxdim;
  double d;
  MathValue gdata[FILE_OBJ_MAXCOL + 3];

  _getobj(obj,"_local",inst,&f2dlocal);
  fp=f2dlocal->data;
  if (fp==NULL) {
    return 0;
  }
  darray=rval->array;
  arrayfree(darray);
  rval->array=NULL;
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
    return 0;
  }
  if (maxdim>FILE_OBJ_MAXCOL) maxdim=FILE_OBJ_MAXCOL;
  rcode = getdataraw(fp, maxdim, gdata);
  if (rcode!=0) {
    closedata(fp, f2dlocal);
    f2dlocal->data=NULL;
    return 1;
  }
  darray=arraynew(sizeof(double));
  for (i=0;i<num;i++) {
    if (data[i]>FILE_OBJ_MAXCOL) {
      d=0;
    } else {
      d = gdata[data[i]].val;
    }
    arrayadd(darray,&d);
  }
  for (i=0;i<num;i++) {
    if (data[i]>FILE_OBJ_MAXCOL) {
      d=MATH_VALUE_NONUM;
    } else {
      d = gdata[data[i]].type;
    }
    arrayadd(darray,&d);
  }
  rval->array=darray;
  return 0;
}

static int
f2dclosedataraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
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
get_mtime(struct objlist *obj, N_VALUE *inst, time_t *mtime)
{
  GStatBuf buf;
  int r, src;
  char *file;

  _getobj(obj, "source", inst, &src);
  switch (src) {
  case DATA_SOURCE_FILE:
    break;
  case DATA_SOURCE_ARRAY:
  case DATA_SOURCE_RANGE:
    *mtime = 1;
    return 0;
  }

  _getobj(obj, "file", inst, &file);

  if (file == NULL)
    return 1;

  r = nstat(file, &buf);
  if (r)
    return 1;

  *mtime = buf.st_mtime;
  return 0;
}

struct data_stat {
  double min, max, ave, stdevp, stdev;
};

static int
f2dstat_sub(struct objlist *obj,N_VALUE *inst, const char *field, struct f2dlocal *f2dlocal, struct data_stat *stat_x,
	    struct data_stat *stat_y, int *num)
{
  int rcode, interrupt;
  int dnum,minxstat,maxxstat,minystat,maxystat;
  double minx,maxx,miny,maxy;
  double sumx,sumxx,sumy,sumyy, tmp;
  time_t mtime;
  struct f2ddata *fp;

  if (get_mtime(obj, inst, &mtime))
    return 1;

  /* evaluation of mtime must be done before call opendata(). */

  if (f2dlocal->mtime_stat == mtime) {
    stat_x->min = f2dlocal->dminx;
    stat_x->max = f2dlocal->dmaxx;
    stat_y->min = f2dlocal->dminy;
    stat_y->max = f2dlocal->dmaxy;
    stat_x->ave = f2dlocal->davx;
    stat_y->ave = f2dlocal->davy;
    stat_x->stdevp = f2dlocal->dstdevpx;
    stat_y->stdevp = f2dlocal->dstdevpy;
    stat_x->stdev  = f2dlocal->dstdevx;
    stat_y->stdev  = f2dlocal->dstdevy;
    *num = f2dlocal->num;

    if (f2dlocal->dminx != HUGE_VAL || strcmp(field, "dnum") == 0) {
      return 0;
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
  minxstat=MATH_VALUE_UNDEF;
  maxxstat=MATH_VALUE_UNDEF;
  minystat=MATH_VALUE_UNDEF;
  maxystat=MATH_VALUE_UNDEF;
  dnum=0;
  minx=maxx=miny=maxy=0;
  sumx=sumxx=sumy=sumyy=0;

  if (set_const_all(fp))
    return 1;

  while ((rcode=getdata(fp))==0) {
    switch (fp->type) {
    case TYPE_NORMAL:
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)) {
        if ((minxstat==MATH_VALUE_UNDEF) || (minx>fp->dx)) minx=fp->dx;
        if ((maxxstat==MATH_VALUE_UNDEF) || (maxx<fp->dx)) maxx=fp->dx;
        minxstat=MATH_VALUE_NORMAL;
        maxxstat=MATH_VALUE_NORMAL;
        if ((minystat==MATH_VALUE_UNDEF) || (miny>fp->dy)) miny=fp->dy;
        if ((maxystat==MATH_VALUE_UNDEF) || (maxy<fp->dy)) maxy=fp->dy;
        minystat=MATH_VALUE_NORMAL;
        maxystat=MATH_VALUE_NORMAL;
        sumx=sumx+fp->dx;
        sumxx=sumxx+(fp->dx)*(fp->dx);
        sumy=sumy+fp->dy;
        sumyy=sumyy+(fp->dy)*(fp->dy);
        dnum++;
      }
      break;
    case TYPE_DIAGONAL:
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
       && (fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL)) {
        if ((minxstat==MATH_VALUE_UNDEF) || (minx>fp->dx)) minx=fp->dx;
        if ((maxxstat==MATH_VALUE_UNDEF) || (maxx<fp->dx)) maxx=fp->dx;
        minxstat=MATH_VALUE_NORMAL;
        maxxstat=MATH_VALUE_NORMAL;
        if ((minystat==MATH_VALUE_UNDEF) || (miny>fp->dy)) miny=fp->dy;
        if ((maxystat==MATH_VALUE_UNDEF) || (maxy<fp->dy)) maxy=fp->dy;
        minystat=MATH_VALUE_NORMAL;
        maxystat=MATH_VALUE_NORMAL;
        if ((minxstat==MATH_VALUE_UNDEF) || (minx>fp->d2)) minx=fp->d2;
        if ((maxxstat==MATH_VALUE_UNDEF) || (maxx<fp->d2)) maxx=fp->d2;
        minxstat=MATH_VALUE_NORMAL;
        maxxstat=MATH_VALUE_NORMAL;
        if ((minystat==MATH_VALUE_UNDEF) || (miny>fp->d3)) miny=fp->d3;
        if ((maxystat==MATH_VALUE_UNDEF) || (maxy<fp->d3)) maxy=fp->d3;
        minystat=MATH_VALUE_NORMAL;
        maxystat=MATH_VALUE_NORMAL;
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
      if ((fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL)
       && (fp->dystat==MATH_VALUE_NORMAL)) {
        if ((minystat==MATH_VALUE_UNDEF) || (miny>fp->dy)) miny=fp->dy;
        if ((maxystat==MATH_VALUE_UNDEF) || (maxy<fp->dy)) maxy=fp->dy;
        minystat=MATH_VALUE_NORMAL;
        maxystat=MATH_VALUE_NORMAL;
        if ((minxstat==MATH_VALUE_UNDEF) || (minx>fp->d2)) minx=fp->d2;
        if ((maxxstat==MATH_VALUE_UNDEF) || (maxx<fp->d2)) maxx=fp->d2;
        minxstat=MATH_VALUE_NORMAL;
        maxxstat=MATH_VALUE_NORMAL;
        if ((minxstat==MATH_VALUE_UNDEF) || (minx>fp->d3)) minx=fp->d3;
        if ((maxxstat==MATH_VALUE_UNDEF) || (maxx<fp->d3)) maxx=fp->d3;
        minxstat=MATH_VALUE_NORMAL;
        maxxstat=MATH_VALUE_NORMAL;
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
      if ((fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL)
       && (fp->dxstat==MATH_VALUE_NORMAL)) {
        if ((minxstat==MATH_VALUE_UNDEF) || (minx>fp->dx)) minx=fp->dx;
        if ((maxxstat==MATH_VALUE_UNDEF) || (maxx<fp->dx)) maxx=fp->dx;
        minxstat=MATH_VALUE_NORMAL;
        maxxstat=MATH_VALUE_NORMAL;
        if ((minystat==MATH_VALUE_UNDEF) || (miny>fp->d2)) miny=fp->d2;
        if ((maxystat==MATH_VALUE_UNDEF) || (maxy<fp->d2)) maxy=fp->d2;
        minystat=MATH_VALUE_NORMAL;
        maxystat=MATH_VALUE_NORMAL;
        if ((minystat==MATH_VALUE_UNDEF) || (miny>fp->d3)) miny=fp->d3;
        if ((maxystat==MATH_VALUE_UNDEF) || (maxy<fp->d3)) maxy=fp->d3;
        minystat=MATH_VALUE_NORMAL;
        maxystat=MATH_VALUE_NORMAL;
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
    tmp = sumxx / dnum - sumx * sumx;
    sumxx = (tmp < 0) ? 0 : sqrt(tmp);
    tmp = sumyy / dnum - sumy * sumy;
    sumyy = (tmp < 0) ? 0 : sqrt(tmp);
  } else {
    sumx=sumy=sumxx=sumyy=0;
  }
  if (minxstat!=MATH_VALUE_NORMAL) minx=0;
  if (minystat!=MATH_VALUE_NORMAL) miny=0;
  if (maxxstat!=MATH_VALUE_NORMAL) maxx=0;
  if (maxystat!=MATH_VALUE_NORMAL) maxy=0;

  if (interrupt == FALSE)
    f2dlocal->mtime_stat = mtime;

  stat_x->min = minx;
  stat_x->max = maxx;
  stat_y->min = miny;
  stat_y->max = maxy;
  stat_x->ave = sumx;
  stat_y->ave = sumy;
  stat_x->stdevp = sumxx;
  stat_y->stdevp = sumyy;
  if (dnum > 1) {
    tmp = sqrt(dnum / (dnum - 1.0));
    stat_x->stdev = sumxx * tmp;
    stat_y->stdev = sumyy * tmp;
  } else {
    stat_x->stdev = HUGE_VAL;
    stat_y->stdev = HUGE_VAL;
  }
  *num = dnum;

  return 0;
}

static int
f2dstat(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
	int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  int dnum;
  char *field;
  char *ptr;
  int src, r;
  struct data_stat stat_x, stat_y;

  g_free(rval->str);
  rval->str=NULL;
  field=argv[1];
  _getobj(obj,"_local",inst,&f2dlocal);
  _getobj(obj,"source", inst, &src);

  memset(&stat_x, 0, sizeof(stat_x));
  memset(&stat_y, 0, sizeof(stat_y));

  r = f2dstat_sub(obj, inst, field, f2dlocal, &stat_x, &stat_y, &dnum);
  if (r) {
    return r;
  }

  f2dlocal->dminx = stat_x.min;
  f2dlocal->dmaxx = stat_x.max;
  f2dlocal->dminy = stat_y.min;
  f2dlocal->dmaxy = stat_y.max;
  f2dlocal->davx = stat_x.ave;
  f2dlocal->davy = stat_y.ave;
  f2dlocal->dstdevpx = stat_x.stdevp;
  f2dlocal->dstdevpy = stat_y.stdevp;
  f2dlocal->dstdevx  = stat_x.stdev;
  f2dlocal->dstdevy  = stat_y.stdev;

  ptr = NULL;
  if (strcmp(field, "dnum") == 0) {
    ptr = g_strdup_printf("%d", dnum);
  } else if (strcmp(field,"dminx")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, stat_x.min);
  } else if (strcmp(field,"dmaxx")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, stat_x.max);
  } else if (strcmp(field,"dminy")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, stat_y.min);
  } else if (strcmp(field,"dmaxy")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, stat_y.max);
  } else if (strcmp(field,"davx")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, stat_x.ave);
  } else if (strcmp(field,"davy")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, stat_y.ave);
  } else if (strcmp(field,"dsigx")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, stat_x.stdevp);
  } else if (strcmp(field,"dsigy")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, stat_y.stdevp);
  } else if (strcmp(field,"dstdevpx")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, stat_x.stdevp);
  } else if (strcmp(field,"dstdevpy")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, stat_y.stdevp);
  } else if (strcmp(field,"dstdevx")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, stat_x.stdev);
  } else if (strcmp(field,"dstdevy")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, stat_y.stdev);
  }

  if (ptr == NULL) return -1;

  rval->str = ptr;

  return 0;
}

static int
f2dstat2(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
	 int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;
  int rcode;
  int line;
  double dx,dy,d2,d3;
  int find;
  char *field;
  char *ptr;

  g_free(rval->str);
  rval->str=NULL;
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

  if (set_const_all(fp))
    return 1;

  while ((rcode=getdata(fp))==0) {
    if (fp->dline==line) {
      switch (fp->type) {
      case TYPE_NORMAL:
        if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)) {
          dx=fp->dx;
          dy=fp->dy;
          find=TRUE;
        }
	break;
      default:
        if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
         && (fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL)) {
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

  ptr = NULL;
  if (strcmp(field,"dx")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, dx);
  } else if (strcmp(field,"dy")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, dy);
  } else if (strcmp(field,"d2")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, d2);
  } else if (strcmp(field,"d3")==0) {
    ptr = g_strdup_printf(DOUBLE_STR_FORMAT, d3);
  }

  if (ptr == NULL) {
    return -1;
  }

  rval->str = ptr;
  return 0;
}

static int
set_bounding_info(struct objlist *obj, N_VALUE *inst, double minx, double maxx, double miny, double maxy, int minxstat, int maxxstat, int minystat, int maxystat)
{
  if (_putobj(obj,"minx",inst,&minx)) return 1;
  if (_putobj(obj,"maxx",inst,&maxx)) return 1;
  if (_putobj(obj,"miny",inst,&miny)) return 1;
  if (_putobj(obj,"maxy",inst,&maxy)) return 1;
  if (_putobj(obj,"stat_minx",inst,&minxstat)) return 1;
  if (_putobj(obj,"stat_maxx",inst,&maxxstat)) return 1;
  if (_putobj(obj,"stat_miny",inst,&minystat)) return 1;
  if (_putobj(obj,"stat_maxy",inst,&maxystat)) return 1;
  return 0;
}

static int
f2dboundings_file(struct f2dlocal *f2dlocal, struct f2ddata *fp, struct objlist *obj, N_VALUE *inst, int abs)
{
  int rcode;
  int minxstat,maxxstat,minystat,maxystat,type;
  double minx,maxx,miny,maxy;

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
  minxstat=MATH_VALUE_UNDEF;
  maxxstat=MATH_VALUE_UNDEF;
  minystat=MATH_VALUE_UNDEF;
  maxystat=MATH_VALUE_UNDEF;
  minx=maxx=miny=maxy=0;

  if (set_const_all(fp))
    return 1;

  while ((rcode=getdata(fp))==0) {
    switch (fp->type) {
    case TYPE_NORMAL:
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)) {
        if ((!abs) || (fp->dx>0)) {
          if ((minxstat==MATH_VALUE_UNDEF) || (minx>fp->dx)) minx=fp->dx;
          if ((maxxstat==MATH_VALUE_UNDEF) || (maxx<fp->dx)) maxx=fp->dx;
          minxstat=MATH_VALUE_NORMAL;
          maxxstat=MATH_VALUE_NORMAL;
        }
        if ((!abs) || (fp->dy>0)) {
          if ((minystat==MATH_VALUE_UNDEF) || (miny>fp->dy)) miny=fp->dy;
          if ((maxystat==MATH_VALUE_UNDEF) || (maxy<fp->dy)) maxy=fp->dy;
          minystat=MATH_VALUE_NORMAL;
          maxystat=MATH_VALUE_NORMAL;
        }
	if (_getobj(obj, "type", inst, &type)) return 1;
	switch (type) {
	case PLOT_TYPE_BAR_X:
	case PLOT_TYPE_BAR_FILL_X:
	case PLOT_TYPE_BAR_SOLID_FILL_X:
	  if (minxstat != MATH_VALUE_NORMAL) {
	    break;
	  }
	if (minx > 0 && maxx > 0) {
	  minx = 0;
	} else if (minx < 0 && maxx < 0) {
	  maxx = 0;
	}
	break;
	case PLOT_TYPE_BAR_Y:
	case PLOT_TYPE_BAR_FILL_Y:
	case PLOT_TYPE_BAR_SOLID_FILL_Y:
	  if (minystat != MATH_VALUE_NORMAL) {
	    break;
	  }
	  if (miny > 0 && maxy > 0) {
	    miny = 0;
	  } else if (miny < 0 && maxy < 0) {
	    maxy = 0;
	  }
	  break;
	}
      }
      break;
    case TYPE_DIAGONAL:
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
       && (fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL)) {
        if ((!abs) || (fp->dx>0)) {
          if ((minxstat==MATH_VALUE_UNDEF) || (minx>fp->dx)) minx=fp->dx;
          if ((maxxstat==MATH_VALUE_UNDEF) || (maxx<fp->dx)) maxx=fp->dx;
          minxstat=MATH_VALUE_NORMAL;
          maxxstat=MATH_VALUE_NORMAL;
        }
        if ((!abs) || (fp->dy>0)) {
          if ((minystat==MATH_VALUE_UNDEF) || (miny>fp->dy)) miny=fp->dy;
          if ((maxystat==MATH_VALUE_UNDEF) || (maxy<fp->dy)) maxy=fp->dy;
          minystat=MATH_VALUE_NORMAL;
          maxystat=MATH_VALUE_NORMAL;
        }
        if ((!abs) || (fp->d2>0)) {
          if ((minxstat==MATH_VALUE_UNDEF) || (minx>fp->d2)) minx=fp->d2;
          if ((maxxstat==MATH_VALUE_UNDEF) || (maxx<fp->d2)) maxx=fp->d2;
          minxstat=MATH_VALUE_NORMAL;
          maxxstat=MATH_VALUE_NORMAL;
        }
        if ((!abs) || (fp->d3>0)) {
          if ((minystat==MATH_VALUE_UNDEF) || (miny>fp->d3)) miny=fp->d3;
          if ((maxystat==MATH_VALUE_UNDEF) || (maxy<fp->d3)) maxy=fp->d3;
          minystat=MATH_VALUE_NORMAL;
          maxystat=MATH_VALUE_NORMAL;
        }
      }
      break;
    case TYPE_ERR_X:
      if ((fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL)
       && (fp->dystat==MATH_VALUE_NORMAL)) {
        if ((!abs) || (fp->dy>0)) {
          if ((minystat==MATH_VALUE_UNDEF) || (miny>fp->dy)) miny=fp->dy;
          if ((maxystat==MATH_VALUE_UNDEF) || (maxy<fp->dy)) maxy=fp->dy;
          minystat=MATH_VALUE_NORMAL;
          maxystat=MATH_VALUE_NORMAL;
        }
        if ((!abs) || (fp->d2>0)) {
          if ((minxstat==MATH_VALUE_UNDEF) || (minx>fp->d2)) minx=fp->d2;
          if ((maxxstat==MATH_VALUE_UNDEF) || (maxx<fp->d2)) maxx=fp->d2;
          minxstat=MATH_VALUE_NORMAL;
          maxxstat=MATH_VALUE_NORMAL;
        }
        if ((!abs) || (fp->d3>0)) {
          if ((minxstat==MATH_VALUE_UNDEF) || (minx>fp->d3)) minx=fp->d3;
          if ((maxxstat==MATH_VALUE_UNDEF) || (maxx<fp->d3)) maxx=fp->d3;
          minxstat=MATH_VALUE_NORMAL;
          maxxstat=MATH_VALUE_NORMAL;
        }
      }
      break;
    case TYPE_ERR_Y:
      if ((fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL)
       && (fp->dxstat==MATH_VALUE_NORMAL)) {
        if ((!abs) || (fp->dx>0)) {
          if ((minxstat==MATH_VALUE_UNDEF) || (minx>fp->dx)) minx=fp->dx;
          if ((maxxstat==MATH_VALUE_UNDEF) || (maxx<fp->dx)) maxx=fp->dx;
          minxstat=MATH_VALUE_NORMAL;
          maxxstat=MATH_VALUE_NORMAL;
        }
        if ((!abs) || (fp->d2>0)) {
          if ((minystat==MATH_VALUE_UNDEF) || (miny>fp->d2)) miny=fp->d2;
          if ((maxystat==MATH_VALUE_UNDEF) || (maxy<fp->d2)) maxy=fp->d2;
          minystat=MATH_VALUE_NORMAL;
          maxystat=MATH_VALUE_NORMAL;
        }
        if ((!abs) || (fp->d3>0)) {
          if ((minystat==MATH_VALUE_UNDEF) || (miny>fp->d3)) miny=fp->d3;
          if ((maxystat==MATH_VALUE_UNDEF) || (maxy<fp->d3)) maxy=fp->d3;
          minystat=MATH_VALUE_NORMAL;
          maxystat=MATH_VALUE_NORMAL;
        }
      }
      break;
    }
  }

  if (rcode != -1) {
    set_bounding_info(obj, inst, minx, maxx, miny, maxy, minxstat, maxxstat, minystat, maxystat);
  }

  return rcode;
}

static int
f2dboundings(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;
  int rcode;
  int abs;

  abs=*(int *)argv[2];
  rcode = set_bounding_info(obj, inst, 0, 0, 0, 0, MATH_VALUE_UNDEF, MATH_VALUE_UNDEF, MATH_VALUE_UNDEF, MATH_VALUE_UNDEF);
  if (rcode) {
    return 1;
  }
  _getobj(obj,"_local",inst,&f2dlocal);

  fp = opendata(obj,inst,f2dlocal,FALSE,FALSE);
  if (fp == NULL)
    return 1;

  rcode = f2dboundings_file(f2dlocal, fp, obj, inst, abs);
  closedata(fp, f2dlocal);
  if (rcode==-1) {
    return -1;
  }

  return 0;
}

static int
f2dbounding(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
	    int argc,char **argv)
{
  struct narray *minmax;
  char *axiss;
  struct narray iarray;
  struct objlist *aobj;
  int i,anum,id,r = 1;
  int *adata;
  double min,max;
  int minstat,maxstat;
  int hidden;
  int type,abs;
  char *argv2[2];

  minmax=rval->array;
  if (minmax!=NULL) arrayfree(minmax);
  rval->array=NULL;
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

  id = get_axis_id(obj, inst, &aobj, AXIS_X);
  if (id >= 0) {
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
      if ((minstat==MATH_VALUE_NORMAL) && (maxstat==MATH_VALUE_NORMAL)) {
	minmax=arraynew(sizeof(double));
	arrayadd(minmax,&min);
	arrayadd(minmax,&max);
	rval->array=minmax;
      }
      r = 0;
      goto exit;
    }
  }
  id = get_axis_id(obj, inst, &aobj, AXIS_Y);
  if (id >= 0) {
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
      if ((minstat==MATH_VALUE_NORMAL) && (maxstat==MATH_VALUE_NORMAL)) {
	minmax=arraynew(sizeof(double));
	arrayadd(minmax,&min);
	arrayadd(minmax,&max);
	rval->array=minmax;
      }
    }
  }
  r = 0;
exit:
  arraydel(&iarray);
  return r;
}

static int
save_fit(GString *s, char *fit, struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray iarray;
  struct objlist *fitobj;
  char *fitsave;
  int id, idnum;

  arrayinit(&iarray, sizeof(int));
  if (getobjilist(fit, &fitobj, &iarray, FALSE, NULL)) {
    return 1;
  }

  idnum = arraynum(&iarray);
  if (idnum < 1) {
    arraydel(&iarray);
    return 1;
  }

  id = arraylast_int(&iarray);
  arraydel(&iarray);
  if (getobj(fitobj, "save", id, 0, NULL, &fitsave) == -1) {
    return 1;
  }

  g_string_append(s, fitsave);
  return 0;
}

static int
f2dsave(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array, *array2;
  int anum;
  char **adata;
  int i, j, r;
  char *argv2[4];
  char *s2, *fit;
  GString *s;

  array = (struct narray *) argv[2];
  anum = arraynum(array);
  adata = arraydata(array);

  for (j = 0; j < anum; j++) {
    if (strcmp("fit", adata[j]) == 0) {
      return pathsave(obj, inst, rval, argc, argv);
    }
  }

  _getobj(obj, "fit", inst, &fit);
  if (fit == NULL) {
    return pathsave(obj, inst, rval, argc, argv);
  }

  array2 = arraynew(sizeof(char *));
  for (i = 0; i < anum; i++) {
    arrayadd(array2, &(adata[i]));
  }

  s2 = "fit";
  arrayadd(array2, &s2);
  argv2[0] = argv[0];
  argv2[1] = argv[1];
  argv2[2] = (char *) array2;
  argv2[3] = NULL;
  if (pathsave(obj, inst, rval, 3, argv2)) {
    arrayfree(array2);
    return 1;
  }

  s = g_string_sized_new(1024);
  if (s == NULL) {
    return 1;
  }

  r = save_fit(s, fit, obj, inst, rval, argc, argv);
  if (r) {
    g_string_free(s, TRUE);
    return pathsave(obj, inst, rval, argc, argv);
  }

  arrayfree(array2);
  g_string_append(s, rval->str);
  g_string_append(s, "\tfile::fit='fit:^'${fit::oid}\n");
  g_free(rval->str);
  rval->str = g_string_free(s, FALSE);
  return 0;
}

static int
f2dstore_file(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  char *file,*date,*time;
  int style;
  char *buf;

  g_free(rval->str);
  rval->str=NULL;
  _getobj(obj,"_local",inst,&f2dlocal);
  if (f2dlocal->endstore) {
    f2dlocal->endstore=FALSE;
    return 1;
  }

  if (f2dlocal->storefd == NULL) {
    char *base, *argv2[2];
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
    if(date == NULL) {
      date = "1-1-1970";
    }
    _getobj(obj,"time",inst,&time);
    if(time == NULL) {
      time = "00:00:00";
    }
    if ((base=getbasename(file))==NULL) return 1;
    if ((f2dlocal->storefd=nfopen(file,"rt"))==NULL) {
      g_free(base);
      return 1;
    }
    buf = g_strdup_printf("file::load_data '%s' '%s %s' <<'[EOF]'", base, date, time);
    g_free(base);
    if (buf == NULL) {
      fclose(f2dlocal->storefd);
      f2dlocal->storefd=NULL;
      return 1;
    }
    rval->str=buf;
  } else {
    int r;
    r = fgetline(f2dlocal->storefd, &buf);
    if (r) {
      fclose(f2dlocal->storefd);
      f2dlocal->storefd=NULL;
      buf = g_strdup("[EOF]\n");
      if (buf == NULL) return 1;
      f2dlocal->endstore=TRUE;
      rval->str=buf;
    } else {
      rval->str=buf;
    }
  }

  return 0;
}

static int
f2dstore(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int r, src;

  _getobj(obj,"source", inst, &src);
  r = 1;
  switch (src) {
  case DATA_SOURCE_FILE:
    r = f2dstore_file(obj, inst, rval, argc, argv);
    break;
  case DATA_SOURCE_ARRAY:
  case DATA_SOURCE_RANGE:
    break;
  }

  return r;
}

static int
nstrrchr(const char *str, int chr, int index)
{
  int i;
  for (i = index; i >= 0; i--) {
    if (str[i] == chr) {
      break;
    }
  }

  return i;
}

static int
get_filename_last_index(const char *s)
{
  int i;

  i = strlen(s);
  i = nstrrchr(s, ' ', i);
  if (i < 0) {
    return i;
  }
  while (s[i] == ' ') {
    i--;
  }

  i = nstrrchr(s, ' ', i);
  if (i < 0) {
    return i;
  }

  return i;
}

static int
f2dload_sub(struct objlist *obj, N_VALUE *inst, char **s, int *expand, char **fullname)
{
  struct objlist *sys;
  char *exdir, *file2, *file, *oldfile, *fname;
  int len;

  if (s == NULL)
    return 1;

  sys = getobject("system");
  if (expand) {
    getobj(sys, "expand_file", 0, 0, NULL, expand);
  }

  len = get_filename_last_index(*s);
  if (len < 0) {
    return 1;
  }

  file = g_strndup(*s, len);
  if (file == NULL) {
    return 1;
  }
  *s = *s + len;

  getobj(sys, "expand_dir", 0, 0, NULL, &exdir);

  file2 = getfilename(CHK_STR(exdir), "/", file);
  g_free(file);

  fname = getfullpath(file2);
  if (fname == NULL) {
    g_free(file2);
    return 1;
  }
  if (fullname)
    *fullname = fname;

  g_free(file2);

  _getobj(obj, "file", inst, &oldfile);
  g_free(oldfile);

  _putobj(obj, "file", inst, fname);

  return 0;
}

static int
f2dload_file(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int expand;
  char *s, *fullname;
  int mkdata;
  time_t ftime;

  s = argv[2];

  if (f2dload_sub(obj, inst, &s, &expand, &fullname))
    return 1;

  if (! expand)
    return 0;

  if (gettimeval(s, &ftime))
    return 1;

  if (naccess(fullname, R_OK) != 0) {
    mkdata = TRUE;
  } else {
    char *mes;

    mes = g_strdup_printf("`%s' Overwrite existing file?", fullname);
    if (mes == NULL)
      return 1;

    mkdata = inputyn(mes);
    g_free(mes);
  }

  if (mkdata) {
    int len;
    struct utimbuf tm;
    FILE *fp;
    int fd;
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
f2dload(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int r, src;

  _getobj(obj,"source", inst, &src);
  r = 1;
  switch (src) {
  case DATA_SOURCE_FILE:
    r = f2dload_file(obj, inst, rval, argc, argv);
    break;
  case DATA_SOURCE_ARRAY:
  case DATA_SOURCE_RANGE:
    break;
  }

  return r;
}

static int
f2dstoredum(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  char *file,*base,*date,*time;
  int style;
  char *buf;
  char *argv2[2];

  g_free(rval->str);
  rval->str=NULL;
  _getobj(obj,"_local",inst,&f2dlocal);
  if (f2dlocal->endstore) {
    f2dlocal->endstore=FALSE;
    return 1;
  }
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
  if(date == NULL) {
    date = "1-1-1970";
  }
  _getobj(obj,"time",inst,&time);
  if(time == NULL) {
    time = "00:00:00";
  }
  if ((base=getbasename(file))==NULL) return 1;
  buf = g_strdup_printf("file::load_dummy '%s' '%s %s'\n", base, date, time);
  g_free(base);
  if (buf == NULL) {
    return 1;
  }
  rval->str=buf;
  f2dlocal->endstore=TRUE;
  return 0;
}

static int
f2dloaddum(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *s = argv[2];
  return f2dload_sub(obj, inst, &s, NULL, NULL);
}

static int
f2dtight(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv)
{
  obj_do_tighten(obj, inst, "axis_x");
  obj_do_tighten(obj, inst, "axis_y");
  obj_do_tighten(obj, inst, "fit");
  obj_do_tighten_all(obj, inst, "array");

  return 0;
}

static int
curveoutfile(struct objlist *obj,struct f2ddata *fp,FILE *fp2,
                 int intp,int div)
{
  int emerr,emnonum,emig,emng;
  int j,k,num;
  int first;
  double *x,*y,*z,*c1,*c2,*c3,*c4,*c5,*c6,count,dd,dx,dy;
  int *r,*g,*b,*a;
  double c[8];
  double bs1[7],bs2[7],bs3[4],bs4[4];
  int spcond;

  emerr=emnonum=emig=emng=FALSE;
  switch (intp) {
  case INTERPOLATION_TYPE_SPLINE:
  case INTERPOLATION_TYPE_SPLINE_CLOSE:
    num=0;
    count=0;
    x=y=z=c1=c2=c3=c4=c5=c6=NULL;
    r=g=b=a=NULL;
    while (getdata(fp)==0) {
      if (fp->dxstat==MATH_VALUE_NORMAL && fp->dystat==MATH_VALUE_NORMAL) {
        if (dataadd(fp->dx,fp->dy,count,fp->col.r,fp->col.g,fp->col.b,fp->col.a,&num,
		    &x,&y,&z,&r,&g,&b,&a,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) return -1;
        count++;
      } else {
        if ((fp->dxstat!=MATH_VALUE_CONT) && (fp->dystat!=MATH_VALUE_CONT)) {
          if (num>=2) {
            if (intp==INTERPOLATION_TYPE_SPLINE) {
	      spcond=SPLCND2NDDIF;
	    } else {
              spcond=SPLCNDPERIODIC;
              if ((x[num-1]!=x[0]) || (y[num-1]!=y[0])) {
                if (dataadd(x[0],y[0],count,r[0],g[0],b[0],a[0],&num,
			    &x,&y,&z,&r,&g,&b,&a,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) return -1;
              }
            }
            if (spline(z,x,c1,c2,c3,num,spcond,spcond,0,0)
             || spline(z,y,c4,c5,c6,num,spcond,spcond,0,0)) {
	      FREE_INTP_BUF();
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
	  FREE_INTP_BUF();
          num=0;
          count=0;
          x=y=z=c1=c2=c3=c4=c5=c6=NULL;
          r=g=b=a=NULL;
        }
        errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
      }
    }
    if (num!=0) {
      if (intp==INTERPOLATION_TYPE_SPLINE) {
	spcond=SPLCND2NDDIF;
      } else {
        spcond=SPLCNDPERIODIC;
        if ((x[num-1]!=x[0]) || (y[num-1]!=y[0])) {
          if (dataadd(x[0],y[0],count,r[0],g[0],b[0],a[0],&num,
		      &x,&y,&z,&r,&g,&b,&a,&c1,&c2,&c3,&c4,&c5,&c6)==NULL) return -1;
        }
      }
      if (spline(z,x,c1,c2,c3,num,spcond,spcond,0,0)
       || spline(z,y,c4,c5,c6,num,spcond,spcond,0,0)) {
	FREE_INTP_BUF();
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
    FREE_INTP_BUF();
    break;
  case INTERPOLATION_TYPE_BSPLINE:
    first=TRUE;
    num=0;
    while (getdata(fp)==0) {
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
      && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
        if (first) {
          bs1[num]=fp->dx;
          bs2[num]=fp->dy;
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
          }
          bs1[6]=fp->dx;
          bs2[6]=fp->dy;
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
        if ((fp->dxstat!=MATH_VALUE_CONT) && (fp->dystat!=MATH_VALUE_CONT)) {
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
        errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
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
      if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
      && (getposition2(fp,fp->axtype,fp->aytype,&(fp->dx),&(fp->dy))==0)) {
        if (first) {
          bs1[num]=fp->dx;
          bs3[num]=fp->dx;
          bs2[num]=fp->dy;
          bs4[num]=fp->dy;
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
          }
          bs1[3]=fp->dx;
          bs2[3]=fp->dy;
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
        if ((fp->dxstat!=MATH_VALUE_CONT) && (fp->dystat!=MATH_VALUE_CONT)) {
          if (!first) {
            for (j=0;j<3;j++) {
              bs1[4+j]=bs3[j];
              bs2[4+j]=bs4[j];
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
        errordisp(obj,fp,&emerr,&emnonum,&emig,&emng);
      }
    }
    if (!first) {
      for (j=0;j<3;j++) {
        bs1[4+j]=bs3[j];
        bs2[4+j]=bs4[j];
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
save_data_file(struct objlist *obj, N_VALUE *inst, struct f2dlocal *f2dlocal,
	       struct f2ddata *fp, const char *data_file, const char *file,
	       int div, int append)
{
  int type, intp, r;
  FILE *fp2;

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
    error22(obj, ERRUNKNOWN, g_strerror(errno), file);
    closedata(fp, f2dlocal);
    return 1;
  }

  if (set_const_all(fp))
    return 1;

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
    int rcode;
    while ((rcode=getdata(fp))==0) {
      switch (fp->type) {
      case TYPE_NORMAL:
        if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL))
          fprintf(fp2,"%.15e %.15e\n",fp->dx,fp->dy);
	break;
      case TYPE_DIAGONAL:
        if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
         && (fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL))
          fprintf(fp2,"%.15e %.15e %.15e %.15e\n",fp->dx,fp->dy,fp->d2,fp->d3);
	break;
      case TYPE_ERR_X:
        if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
         && (fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL))
          fprintf(fp2,"%.15e %.15e %.15e %.15e\n",
                     fp->dx,fp->d2-fp->dx,fp->d3-fp->dx,fp->dy);
	break;
      case TYPE_ERR_Y:
        if ((fp->dxstat==MATH_VALUE_NORMAL) && (fp->dystat==MATH_VALUE_NORMAL)
         && (fp->d2stat==MATH_VALUE_NORMAL) && (fp->d3stat==MATH_VALUE_NORMAL))
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
  fclose(fp2);

  return r;
}

static int
f2doutputfile(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                  int argc,char **argv)
{
  struct f2dlocal *f2dlocal;
  struct f2ddata *fp;
  char *file, *data_file;
  int div,append,r;

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

  r = save_data_file(obj, inst, f2dlocal, fp, data_file, file, div, append);

  closedata(fp, f2dlocal);
  return r;
}

static int
update_field(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct f2dlocal *f2dlocal;

  _getobj(obj, "_local", inst, &f2dlocal);
  f2dlocal->mtime = 0;
  f2dlocal->mtime_stat = 0;

  return 0;
}

static int
accept_ascii_only(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *str;
  int i, j, n;

  update_field(obj, inst, rval, argc, argv);

  if (argv[2] == NULL) {
    return 0;
  }

  str = argv[2];
  n = strlen(str);
  for (i = j = 0; i < n; i++) {
    if (str[i] > 0 && (g_ascii_isprint(str[i]) || g_ascii_isspace(str[i]))) {
      str[j] = str[i];
      j++;
    }
  }
  str[j] = '\0';

  return 0;
}

static int
update_mask(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray *array;

  update_field(obj, inst, rval, argc, argv);

  array = (struct narray *) argv[2];
  arraysort_int(array);
  arrayuniq_int(array);
  return 0;
}

static int
foputabs(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  update_field(obj, inst, rval, argc, argv);
  return oputabs(obj, inst, rval, argc, argv);
}

static int
foputge1(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  update_field(obj, inst, rval, argc, argv);
  return oputge1(obj, inst, rval, argc, argv);
}

static int
adjust_move_num(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
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

#define MAX_ITERATION 10000

static int
bisection(MathEquation *eq, double a, double b, double y, double tolerance, double *x)
{
  int r, i;
  double c, fa, fb;
  MathValue val, rval;

  if (tolerance < 0) {
    tolerance = 0;
  }

  if (b < a) {
    c = a;
    a = b;
    b = c;
  }

  val.type = MATH_VALUE_NORMAL;

  val.val = a;
  math_equation_set_var(eq, 0, &val);
  r = math_equation_calculate(eq, &rval);
  if (r || rval.type != MATH_VALUE_NORMAL) {
    return 1;
  }
  fa = rval.val - y;

  val.val = b;
  math_equation_set_var(eq, 0, &val);
  r = math_equation_calculate(eq, &rval);
  if (r || rval.type != MATH_VALUE_NORMAL) {
    return 1;
  }
  fb = rval.val - y;

  if (compare_double(fa, 0)) {
    *x = a;
    return 0;
  }

  if (compare_double(fb, 0)) {
    *x = b;
    return 0;
  }

  if (fa * fb > 0) {
    return 1;
  }

  i = 0;
  while (1) {
    double fc;
    c = (a + b) / 2;
    if (c - a <= tolerance || b - c <= tolerance) {
      break;
    }

    val.val = c;
    math_equation_set_var(eq, 0, &val);
    r = math_equation_calculate(eq, &rval);
    if (r || rval.type != MATH_VALUE_NORMAL) {
      return 1;
    }

    fc = rval.val - y;
    if (compare_double(fc, 0)) {
      break;
    }

    if (fc * fa > 0) {
      a = c;
      fa = fc;
    } else {
      b = c;
      fb = fc;
    }

    if (tolerance == 0 && i > MAX_ITERATION) {
      break;
    }
    i++;
  }

  *x = c;

  return i >= MAX_ITERATION;
}

static int
newton(MathEquation *eq, double *xx, double y)
{
  int r, n;
  double x, fx;
  MathValue val, rval;

  val.type = MATH_VALUE_NORMAL;

  x = *xx;

  val.val = x;
  math_equation_set_var(eq, 0, &val);
  r = math_equation_calculate(eq, &rval);
  if (r || rval.type != MATH_VALUE_NORMAL) {
    return 1;
  }
  fx = rval.val - y;

  n = 0;
  while (! compare_double(fx, 0)) {
    double dx, df, h, x_prev, fx_prev;
    int i;

    dx = (x == 0) ? 1E-6 : x * 1E-6;
    val.val = x + dx;
    math_equation_set_var(eq, 0, &val);
    r = math_equation_calculate(eq, &rval);
    if (r || rval.type != MATH_VALUE_NORMAL) {
      return 1;
    }
    df = (rval.val - y - fx) / dx;
    h = fx / df;

    x_prev = x;
    fx_prev = fx;
    i = 0;
    do {
      x = x_prev - h;
      val.val = x;
      math_equation_set_var(eq, 0, &val);
      r = math_equation_calculate(eq, &rval);
      if (r || rval.type != MATH_VALUE_NORMAL) {
	return 1;
      }
      fx = rval.val - y;
      h /= 2;

      i++;
      if (i > MAX_ITERATION) {
	return 1;
      }
    } while (fabs(fx) > fabs(fx_prev));

    n++;
    if (n > MAX_ITERATION || (x == x_prev)) {
      break;
    }
  }
  *xx = x;

  return n >= MAX_ITERATION;
}

static int
solve_equation(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  MathEquation *eq = NULL;
  int r, n, type, fit_id;
  char *equation, *fit, prefix[] = FIT_FIELD_PREFIX;
  N_VALUE *fit_inst;
  double x, y, *data;
  struct narray *darray;
  struct objlist *fit_obj;

  if (_exeparent(obj, argv[1], inst, rval, argc, argv)) return 1;

  g_free(rval->str);
  rval->str = NULL;

  _getobj(obj, "type", inst, &type);
  _getobj(obj, "fit", inst, &fit);

  if (type != PLOT_TYPE_FIT) {
    error(obj, ERR_INVALID_TYPE);
    return 1;
  }

  fit_id = get_fit_obj_id(fit, &fit_obj, &fit_inst);
  if (fit_id < 0) {
    error2(obj, ERRNOFITINST, fit);
    return -1;
  }

  if (_getobj(fit_obj, "equation", fit_inst, &equation)) {
    return -1;
  }

  darray = (struct narray *) (argv[2]);
  n = arraynum(darray);
  data = arraydata(darray);

  eq = ofile_create_math_equation(NULL, EOEQ_ASSIGN_TYPE_ASSIGN, 0, FALSE, FALSE, FALSE, FALSE, TRUE);
  if (eq == NULL) {
    return 1;
  }

  r = math_equation_parse(eq, equation);
  if (r) {
    math_equation_free(eq);
    return 1;
  }

  r = math_equation_optimize(eq);
  if (r) {
    math_equation_free(eq);
    return 1;
  }

  if (eq->exp == NULL) {
    math_equation_free(eq);
    return 1;
  }

  if (argv[1][sizeof(prefix) - 1] == 'b') {
    double a, b, tolerance;
    if (n < 2) {
      error(obj, ERR_SMALL_ARGS);
      math_equation_free(eq);
      return 1;
    }

    a = data[0];
    b = data[1];
    y = (n > 2) ? data[2] : 0;
    x = 0;
    tolerance = (n > 3) ? data[3] : 0;

    r = bisection(eq, a, b, y, tolerance, &x);
  } else {
    x = (n > 0) ? data[0] : 0;
    y = (n > 1) ? data[1] : 0;

    r = newton(eq, &x, y);
  }

  math_equation_free(eq);

  if (r) {
    error(obj, ERRCONVERGE);
    return 1;
  }
  rval->str = g_strdup_printf(DOUBLE_STR_FORMAT, x);

  return 0;
}

int
ofile_calc_fit_equation(struct objlist *obj, int id, double x, double *y)
{
  N_VALUE *inst;

  inst = chkobjinst(obj, id);
  if (inst == NULL) {
    return ERRNOFITINST;
  }

  return calc_fit_equation(obj, inst, x, y);
}

static int
calc_fit_equation(struct objlist *obj, N_VALUE *inst, double x, double *y)
{
  int type, fit_id;
  char *fit;
  N_VALUE *fit_inst;
  struct objlist *fit_obj;
  char *fit_argv[2];

  *y = 0;

  _getobj(obj, "type", inst, &type);
  _getobj(obj, "fit", inst, &fit);

  if (type != PLOT_TYPE_FIT) {
    return ERR_INVALID_TYPE;
  }

  fit_id = get_fit_obj_id(fit, &fit_obj, &fit_inst);
  if (fit_id < 0) {
    return ERRNOFITINST;
  }

  fit_argv[0] = (char *) &x;
  fit_argv[1] = NULL;
  getobj(fit_obj, "calc", fit_id, 1, fit_argv, y);

  return 0;
}

static int
calc_equation(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int r;
  double x, y;

  if (_exeparent(obj, argv[1], inst, rval, argc, argv)) return 1;

  g_free(rval->str);
  rval->str = NULL;

  x = arg_to_double(argv, 2);
  r = calc_fit_equation(obj, inst, x, &y);

  switch (r) {
  case 0:
    break;
  case ERR_INVALID_TYPE:
  case ERRNOFITINST:
    error(obj, ERR_INVALID_TYPE);
    return 1;
  case ERR_INVALID_SOURCE:
    error(obj, ERR_INVALID_SOURCE);
    return 1;
  default:
    break;
  }

  rval->str = g_strdup_printf(DOUBLE_STR_FORMAT, y);

  return 0;
}

static int
get_fit_parameter(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int type, fit_id, i;
  char *fit;
  N_VALUE *fit_inst;
  struct objlist *fit_obj;
  char prm_str[] = "%00";
  double val;

  if (_exeparent(obj, argv[1], inst, rval, argc, argv)) return 1;

  g_free(rval->str);
  rval->str = NULL;

  _getobj(obj, "type", inst, &type);
  _getobj(obj, "fit", inst, &fit);

  if (type != PLOT_TYPE_FIT) {
    error(obj, ERR_INVALID_TYPE);
    return 1;
  }

  fit_id = get_fit_obj_id(fit, &fit_obj, &fit_inst);
  if (fit_id < 0) {
    error2(obj, ERRNOFITINST, fit);
    return -1;
  }

  i = * (int *) argv[2];
  if (i < 0 || i > 9) {
    error(obj, ERR_INVALID_PARAM);
    return 1;
  }
  prm_str[2] += i;

  if (_getobj(fit_obj, prm_str, fit_inst, &val)) {
    return -1;
  }

  rval->str = g_strdup_printf(DOUBLE_STR_FORMAT, val);

  return 0;
}

static int
add_keyword(struct nhash *hash, void *data)
{
  GString *str;

  str = data;
  g_string_append(str, hash->key);
  g_string_append_c(str, '\n');
  return 0;
}

static gchar *
create_keyword_list(NHASH hash)
{
  GString *str;
  gchar *text;

  str = g_string_new("");
  nhash_each(hash, add_keyword, str);
  text = g_string_free(str, FALSE);
  return g_strstrip(text);
}

char *
odata_get_functions(void)
{
  MathEquation *eq;
  gchar *text;

  eq = ofile_create_math_equation(NULL, EOEQ_ASSIGN_TYPE_ASSIGN, 3, TRUE, TRUE, TRUE, TRUE, TRUE);
  if (eq == NULL) {
    return NULL;
  }

  text = create_keyword_list(eq->function);

  math_equation_free(eq);
  return text;
}

char *
odata_get_constants(void)
{
  MathEquation *eq;
  gchar *text;

  eq = ofile_create_math_equation(NULL, EOEQ_ASSIGN_TYPE_ASSIGN, 3, TRUE, TRUE, TRUE, TRUE, TRUE);
  if (eq == NULL) {
    return NULL;
  }

  text = create_keyword_list(eq->constant);

  math_equation_free(eq);
  return text;
}

static int
get_functions(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj, argv[1], inst, rval, argc, argv)) return 1;

  g_free(rval->str);
  rval->str = NULL;

  rval->str = odata_get_functions();

  return 0;
}

static int
get_constants(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj, argv[1], inst, rval, argc, argv)) return 1;

  g_free(rval->str);
  rval->str = NULL;

  rval->str = odata_get_constants();

  return 0;
}

static int
set_array(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct array_prm ary;

  if (argv[2] && open_array(argv[2], &ary)) {
    error2(obj, ERR_INVALID_OBJ, argv[2]);
    return 1;
  }

  return 0;
}

static int
data_inst_dup(struct objlist *obj, N_VALUE *src, N_VALUE *dest)
{
  int i, pos;
  struct f2dlocal *local_new, *local_src;
  char *math;

  pos = obj_get_field_pos(obj, "_local");
  if (pos < 0) {
    return 1;
  }
  local_src = src[pos].ptr;
  local_new = g_memdup(local_src, sizeof(struct f2dlocal));
  if (local_new == NULL) {
    return 1;
  }
  dest[pos].ptr = local_new;

  if (local_src->data) {
    closedata(local_src->data, local_src);
    local_src->data = NULL;
    local_new->data = NULL;
  }
  if (local_src->codex[0]) {
    _getobj(obj, "math_x", src, &math);
    for (i = 0; i < EQUATION_NUM; i++) {
      local_new->codex[i] = NULL;
    }
    put_func(obj, dest, local_new, EOEQ_ASSIGN_TYPE_ASSIGN, "math_x", math, NULL);
  }
  if (local_src->codey[0]) {
    _getobj(obj, "math_y", src, &math);
    for (i = 0; i < EQUATION_NUM; i++) {
      local_new->codey[i] = NULL;
    }
    put_func(obj, dest, local_new, EOEQ_ASSIGN_TYPE_ASSIGN, "math_y", math, NULL);
  }

  return 0;
}

static int
data_inst_free(struct objlist *obj, N_VALUE *inst)
{
  int i, pos;
  struct f2dlocal *local;

  pos = obj_get_field_pos(obj, "_local");
  if (pos < 0) {
    return 1;
  }
  local = inst[pos].ptr;
  if (local->data) {
    closedata(local->data, local);
  }
  for (i = 0; i < EQUATION_NUM; i++) {
    if (local->codex[i]) {
      math_equation_free(local->codex[i]);
    }
    if (local->codey[i]) {
      math_equation_free(local->codey[i]);
    }
  }
  g_free(local);
  return 0;
}

static struct objtable file2d[] = {
  {"init",NVFUNC,NEXEC,f2dinit,NULL,0},
  {"done",NVFUNC,NEXEC,f2ddone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"source",NENUM,NREAD|NWRITE,NULL,data_type,0},

  /* for file */
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
  {"averaging_type",NENUM,NREAD|NWRITE,NULL,averaging_type_char,0},

  {"mark_type",NINT,NREAD|NWRITE,oputmarktype,NULL,0},
  {"mark_size",NINT,NREAD|NWRITE,oputabs,NULL,0},

  {"line_width",NINT,NREAD|NWRITE,oputge1,NULL,0},
  {"line_style",NIARRAY,NREAD|NWRITE,oputstyle,NULL,0},
  {"line_join",NENUM,NREAD|NWRITE,NULL,joinchar,0},
  {"line_miter_limit",NINT,NREAD|NWRITE,oputge1,NULL,0},
  {"R2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"G2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"B2",NINT,NREAD|NWRITE,oputcolor,NULL,0},
  {"A2",NINT,NREAD|NWRITE,oputcolor,NULL,0},

  {"remark",NSTR,NREAD|NWRITE,accept_ascii_only,NULL,0},
  {"ifs",NSTR,NREAD|NWRITE,accept_ascii_only,NULL,0},
  {"csv",NBOOL,NREAD|NWRITE,update_field,NULL,0},
  {"head_skip",NINT,NREAD|NWRITE,foputabs,NULL,0},
  {"read_step",NINT,NREAD|NWRITE,foputge1,NULL,0},
  {"final_line",NINT,NREAD|NWRITE,f2dput,NULL,0},
  {"mask",NIARRAY,NREAD|NWRITE,update_mask,NULL,0},
  {"move_data",NIARRAY,NREAD|NWRITE,update_field,NULL,0},
  {"move_data_x",NDARRAY,NREAD|NWRITE,update_field,NULL,0},
  {"move_data_y",NDARRAY,NREAD|NWRITE,update_field,NULL,0},
  {"move_data_adjust",NVFUNC,NREAD|NEXEC,adjust_move_num,"",0},

  {"axis_x",NOBJ,NREAD|NWRITE,NULL,NULL,0},
  {"axis_y",NOBJ,NREAD|NWRITE,NULL,NULL,0},
  {"data_clip",NBOOL,NREAD|NWRITE,NULL,NULL,0},

  {"dnum",NSFUNC,NREAD|NEXEC,f2dstat,"",0},
  {"dminx",NSFUNC,NREAD|NEXEC,f2dstat,"",0},
  {"dmaxx",NSFUNC,NREAD|NEXEC,f2dstat,"",0},
  {"davx",NSFUNC,NREAD|NEXEC,f2dstat,"",0},
  {"dsigx",NSFUNC,NREAD|NEXEC,f2dstat,"",0},
  {"dstdevpx",NSFUNC,NREAD|NEXEC,f2dstat,"",0},
  {"dstdevx",NSFUNC,NREAD|NEXEC,f2dstat,"",0},
  {"dminy",NSFUNC,NREAD|NEXEC,f2dstat,"",0},
  {"dmaxy",NSFUNC,NREAD|NEXEC,f2dstat,"",0},
  {"davy",NSFUNC,NREAD|NEXEC,f2dstat,"",0},
  {"dsigy",NSFUNC,NREAD|NEXEC,f2dstat,"",0},
  {"dstdevpy",NSFUNC,NREAD|NEXEC,f2dstat,"",0},
  {"dstdevy",NSFUNC,NREAD|NEXEC,f2dstat,"",0},
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
  {"opendata",NVFUNC,NREAD|NEXEC,f2dopendata,"",0},
  {"opendatac",NVFUNC,NREAD|NEXEC,f2dopendata,"",0},
  {"getdata",NVFUNC,NREAD|NEXEC,f2dgetdata,"",0},
  {"closedata",NVFUNC,NREAD|NEXEC,f2dclosedata,"",0},
  {"opendata_raw",NVFUNC,NEXEC,f2dopendataraw,"",0},
  {"getdata_raw",NDAFUNC,NEXEC,f2dgetdataraw,"ia",0},
  {"closedata_raw",NVFUNC,NEXEC,f2dclosedataraw,"",0},
  {"column",NSFUNC,NREAD|NEXEC,f2dcolumn,"ii",0},
  {"basename",NSFUNC,NREAD|NEXEC,f2dbasename,"",0},
  {"head_lines",NSFUNC,NREAD|NEXEC,f2dhead,"i",0},
  {"boundings",NVFUNC,NREAD|NEXEC,f2dboundings,"b",0},
  {"bounding",NDAFUNC,NREAD|NEXEC,f2dbounding,"o",0},
  {"load_settings",NVFUNC,NREAD|NEXEC,f2dsettings,"",0},
  {"time",NSFUNC,NREAD|NEXEC,f2dtime,"i",0},
  {"date",NSFUNC,NREAD|NEXEC,f2ddate,"i",0},
  {"save",NSFUNC,NREAD|NEXEC,f2dsave,"sa",0},
  {"evaluate",NDAFUNC,NREAD|NEXEC,f2devaluate,"iiiiii",0},
  {"store_data",NSFUNC,NREAD|NEXEC,f2dstore,"",0},
  {"load_data",NVFUNC,NREAD|NEXEC,f2dload,"s",0},
  {"store_dummy",NSFUNC,NREAD|NEXEC,f2dstoredum,"",0},
  {"load_dummy",NVFUNC,NREAD|NEXEC,f2dloaddum,"s",0},
  {"tight",NVFUNC,NREAD|NEXEC,f2dtight,"",0},
  {"save_config",NVFUNC,NREAD|NEXEC,f2dsaveconfig,"",0},
  {"output_file",NVFUNC,NREAD|NEXEC,f2doutputfile,"sib",0},
  {"modified",NVFUNC,NEXEC,update_field,"",0},
  {"hsb",NVFUNC,NREAD|NEXEC,put_hsb,"ddd",0},
  {"hsb2",NVFUNC,NREAD|NEXEC,put_hsb2,"ddd",0},
  {FIT_FIELD_PREFIX "newton",NSFUNC,NREAD|NEXEC,solve_equation,"da",0},
  {FIT_FIELD_PREFIX "bisection",NSFUNC,NREAD|NEXEC,solve_equation,"da",0},
  {FIT_FIELD_PREFIX "calc",NSFUNC,NREAD|NEXEC,calc_equation,"d",0},
  {FIT_FIELD_PREFIX "prm",NSFUNC,NREAD|NEXEC,get_fit_parameter,"i",0},
  {"math_functions",NSFUNC,NREAD|NEXEC,get_functions,"",0},
  {"math_constants",NSFUNC,NREAD|NEXEC,get_constants,"",0},
  {"_local",NPOINTER,0,NULL,NULL,0},

  /* for range */

  {"range_min", NDOUBLE,NREAD|NWRITE,NULL,NULL,0},
  {"range_max", NDOUBLE,NREAD|NWRITE,NULL,NULL,0},
  {"range_div", NINT,NREAD|NWRITE,oputge1,NULL,0},

  /* for array */

  {"array",NOBJ,NREAD|NWRITE,set_array,NULL,0},
};

#define TBLNUM (sizeof(file2d) / sizeof(*file2d))

void *
addfile(void)
/* addfile() returns NULL on error */
{
  struct objlist *data_obj;

  if (FileConfigHash == NULL) {
    unsigned int i;
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

  data_obj = addobject(NAME,ALIAS,PARENT,OVERSION,TBLNUM,file2d,ERRNUM,f2derrorlist,NULL,NULL);
  obj_set_undo_func(data_obj, data_inst_dup, data_inst_free);
  return data_obj;
}

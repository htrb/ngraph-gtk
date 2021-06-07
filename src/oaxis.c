/* --*-coding:utf-8-*-- */

#include "common.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>

#include "nhash.h"
#include "ntime.h"
#include "ioutil.h"
#include "object.h"
#include "mathfn.h"
#include "spline.h"
#include "gra.h"
#include "oroot.h"
#include "odraw.h"
#include "oaxis.h"
#include "olegend.h"
#include "axis.h"
#include "nstring.h"
#include "nconfig.h"
#include "shell.h"

#include "math/math_equation.h"

#define NAME "axis"
#define PARENT "draw"
#define OVERSION  "1.00.00"

#define AXIS_HISTORY_NUM 30

#define INST_ARRAY_NUM 4

#define ERRAXISTYPE 100
#define ERRAXISHEAD 101
#define ERRAXISGAUGE 102
#define ERRAXISSPL 103
#define ERRMINMAX 104
#define ERRFORMAT 105
#define ERRGROUPING 106
#define ERRNUMMATH 107

static char *axiserrorlist[]={
  "illegal axis type.",
  "illegal arrow/wave type.",
  "illegal gauge type.",
  "error: spline interpolation.",
  "illegal value of min/max/inc.",
  "illegal format.",
  "illegal grouping type.",
  "error in math:",
};

#define ERRNUM (sizeof(axiserrorlist) / sizeof(*axiserrorlist))

enum AXIS_TYPE {
  AXIS_TYPE_SINGLE,
  AXIS_TYPE_FRAME,
  AXIS_TYPE_SECTION,
  AXIS_TYPE_CROSS,
};

char *axistypechar[]={
  N_("linear"),
  N_("log"),
  N_("inverse"),
  N_("MJD"),
  NULL
};

static char *axisgaugechar[]={
  N_("none"),
  N_("both"),
  N_("left"),
  N_("right"),
  NULL
};

enum AXIS_GAUGE {
  AXIS_GAUGE_NONE,
  AXIS_GAUGE_BOTH,
  AXIS_GAUGE_LEFT,
  AXIS_GAUGE_RIGHT,
};

static char *axisnumchar[]={
  N_("none"),
  N_("left"),
  N_("right"),
  NULL
};

enum AXIS_NUM_POS {
  AXIS_NUM_POS_NONE,
  AXIS_NUM_POS_LEFT,
  AXIS_NUM_POS_RIGHT,
};

static char *anumalignchar[]={
  N_("center"),
  N_("left"),
  N_("right"),
  N_("point"),
  NULL
};

enum AXIS_NUM_NO_ZERO {
  AXIS_NUM_NO_ZERO_REGULAR,
  AXIS_NUM_NO_ZERO_NO_ZERO,
  AXIS_NUM_NO_ZERO_NO_FLOATING_POINT,
  AXIS_NUM_NO_ZERO_FALSE,
  AXIS_NUM_NO_ZERO_TRUE,
};

static char *anumnozero[]={
  N_("regular"),
  N_("no_zero"),
  N_("no_floating_point"),
  "\0false",			/* for backward compatibility */
  "\0true",			/* for backward compatibility */
  NULL,
};

enum AXIS_NUM_ALIGN {
  AXIS_NUM_ALIGN_CENTER,
  AXIS_NUM_ALIGN_LEFT,
  AXIS_NUM_ALIGN_RIGHT,
  AXIS_NUM_ALIGN_POINT,
};

static char *anumdirchar[]={
  N_("horizontal"),
  N_("parallel1"),
  N_("parallel2"),
  N_("normal1"),
  N_("normal2"),
  N_("oblique1"),
  N_("oblique2"),
  "\0normal",			/* for backward compatibility */
  "\0parallel",		/* for backward compatibility */
  NULL
};

static struct obj_config AxisConfig[] = {
  {"R", OBJ_CONFIG_TYPE_NUMERIC},
  {"G", OBJ_CONFIG_TYPE_NUMERIC},
  {"B", OBJ_CONFIG_TYPE_NUMERIC},
  {"A", OBJ_CONFIG_TYPE_NUMERIC},
  {"type", OBJ_CONFIG_TYPE_NUMERIC},
  {"direction", OBJ_CONFIG_TYPE_NUMERIC},
  {"baseline", OBJ_CONFIG_TYPE_NUMERIC},
  {"width", OBJ_CONFIG_TYPE_NUMERIC},
  {"arrow", OBJ_CONFIG_TYPE_NUMERIC},
  {"arrow_length", OBJ_CONFIG_TYPE_NUMERIC},
  {"wave", OBJ_CONFIG_TYPE_NUMERIC},
  {"wave_length", OBJ_CONFIG_TYPE_NUMERIC},
  {"wave_width", OBJ_CONFIG_TYPE_NUMERIC},
  {"gauge", OBJ_CONFIG_TYPE_NUMERIC},
  {"gauge_length1", OBJ_CONFIG_TYPE_NUMERIC},
  {"gauge_width1", OBJ_CONFIG_TYPE_NUMERIC},
  {"gauge_length2", OBJ_CONFIG_TYPE_NUMERIC},
  {"gauge_width2", OBJ_CONFIG_TYPE_NUMERIC},
  {"gauge_length3", OBJ_CONFIG_TYPE_NUMERIC},
  {"gauge_width3", OBJ_CONFIG_TYPE_NUMERIC},
  {"gauge_R", OBJ_CONFIG_TYPE_NUMERIC},
  {"gauge_G", OBJ_CONFIG_TYPE_NUMERIC},
  {"gauge_B", OBJ_CONFIG_TYPE_NUMERIC},
  {"gauge_A", OBJ_CONFIG_TYPE_NUMERIC},
  {"num", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_auto_norm", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_log_pow", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_pt", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_space", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_script_size", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_align", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_no_zero", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_direction", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_shift_p", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_shift_n", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_R", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_G", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_B", OBJ_CONFIG_TYPE_NUMERIC},
  {"num_A", OBJ_CONFIG_TYPE_NUMERIC},

  {"num_head", OBJ_CONFIG_TYPE_STRING},
  {"num_format", OBJ_CONFIG_TYPE_STRING},
  {"num_tail", OBJ_CONFIG_TYPE_STRING},
  {"num_font", OBJ_CONFIG_TYPE_STRING},
  {"num_font_style", OBJ_CONFIG_TYPE_NUMERIC},

  {"style", OBJ_CONFIG_TYPE_STYLE},
  {"gauge_style", OBJ_CONFIG_TYPE_STYLE},
};

static NHASH AxisConfigHash = NULL;

static int get_axis_group_type(struct objlist *obj, N_VALUE *inst, N_VALUE **inst_array, int check_all);

static N_VALUE *
check_group(struct objlist *obj, char type, N_VALUE *inst, int num)
{
  int n;
  char *group, *endptr;

  while (inst) {
    _getobj(obj, "group", inst, &group);
    if (group && group[0] == type) {
      n = strtol(group + 2, &endptr, 10);
      if (num == n)
	break;
    }
    inst = inst[obj->nextp].inst;
  }
  return inst;
}

static int
axisuniqgroup(struct objlist *obj,char type)
{
  int num;
  N_VALUE *inst;

  num = 0;
  do {
    num++;
    inst = check_group(obj, type, obj->root, num);
    if (inst == NULL) {
      inst = check_group(obj, type, obj->root2, num);
    }
  } while (inst);
  return num;
}

static int
axisloadconfig(struct objlist *obj,N_VALUE *inst,char *conf)
{
  return obj_load_config(obj, inst, conf, AxisConfigHash);
}

static int
axisinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int width;
  int alen,awid,wlen,wwid,alpha;
  int bline;
  int len1,wid1,len2,wid2,len3,wid3;
  int pt,sx,sy,logpow,scriptsize;
  int autonorm,num,gnum,margin;
  char *font,*format,*group,*name;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  width=DEFAULT_LINE_WIDTH;
  alen=72426;
  awid=60000;
  wlen=300;
  wwid=DEFAULT_LINE_WIDTH;
  len1=100;
  wid1=DEFAULT_LINE_WIDTH;
  len2=200;
  wid2=DEFAULT_LINE_WIDTH;
  len3=300;
  wid3=DEFAULT_LINE_WIDTH;
  bline=TRUE;
  pt=DEFAULT_FONT_PT;
  sx=0;
  sy=100;
  autonorm=5;
  logpow=TRUE;
  scriptsize=DEFAULT_SCRIPT_SIZE;
  num=-1;
  alpha=255;
  margin=500;
  if (_putobj(obj,"baseline",inst,&bline)) return 1;
  if (_putobj(obj,"width",inst,&width)) return 1;
  if (_putobj(obj,"auto_scale_margin",inst,&margin)) return 1;
  if (_putobj(obj,"arrow_length",inst,&alen)) return 1;
  if (_putobj(obj,"arrow_width",inst,&awid)) return 1;
  if (_putobj(obj,"wave_length",inst,&wlen)) return 1;
  if (_putobj(obj,"wave_width",inst,&wwid)) return 1;
  if (_putobj(obj,"gauge_length1",inst,&len1)) return 1;
  if (_putobj(obj,"gauge_width1",inst,&wid1)) return 1;
  if (_putobj(obj,"gauge_length2",inst,&len2)) return 1;
  if (_putobj(obj,"gauge_width2",inst,&wid2)) return 1;
  if (_putobj(obj,"gauge_length3",inst,&len3)) return 1;
  if (_putobj(obj,"gauge_width3",inst,&wid3)) return 1;
  if (_putobj(obj,"gauge_A",inst,&alpha)) return 1;
  if (_putobj(obj,"num_pt",inst,&pt)) return 1;
  if (_putobj(obj,"num_script_size",inst,&scriptsize)) return 1;
  if (_putobj(obj,"num_auto_norm",inst,&autonorm)) return 1;
  if (_putobj(obj,"num_shift_p",inst,&sx)) return 1;
  if (_putobj(obj,"num_shift_n",inst,&sy)) return 1;
  if (_putobj(obj,"num_log_pow",inst,&logpow)) return 1;
  if (_putobj(obj,"num_num",inst,&num)) return 1;
  if (_putobj(obj,"num_A",inst,&alpha)) return 1;

  font = group = name = NULL;

  format = g_strdup("%g");
  if (format == NULL) goto errexit;
  if (_putobj(obj,"num_format",inst,format)) goto errexit;

  font = g_strdup(fontchar[0]);
  if (font == NULL) goto errexit;
  if (_putobj(obj,"num_font",inst,font)) goto errexit;

  gnum = axisuniqgroup(obj,'a');
  group = g_strdup_printf("a_%d", gnum);
  if (group == NULL) goto errexit;
  if (_putobj(obj,"group",inst,group)) goto errexit;

  name = g_strdup_printf("a_%d",gnum);
  if (name == NULL) goto errexit;
  if (_putobj(obj,"name",inst,name)) goto errexit;

  axisloadconfig(obj,inst,"[axis]");
  return 0;

errexit:
  g_free(format);
  g_free(font);
  g_free(group);
  g_free(name);
  return 1;
}

static int
axisdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  N_VALUE *inst_array[INST_ARRAY_NUM];
  int i;

  get_axis_group_type(obj, inst, inst_array, TRUE);
  for (i = 0; i < INST_ARRAY_NUM; i++) {
    if (inst_array[i] && inst_array[i] != inst) {
      char *group, *group2;
      int gnum;

      gnum = axisuniqgroup(obj,'a');
      group = g_strdup_printf("a_%d", gnum);
      if (group == NULL)
	break;

      _getobj(obj, "group", inst_array[i], &group2);
      g_free(group2);
      if (_putobj(obj, "group", inst_array[i], group))
	break;
    }
  }

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  return 0;
}

static int
axisput(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
            int argc,char **argv)
{
  char *field;
  char *format;
  int sharp,minus,plus;
  int i,j;

  field=argv[1];
  if (strcmp(field,"arrow_length")==0) {
    if (*(int *)(argv[2])<10000) *(int *)(argv[2])=10000;
    else if (*(int *)(argv[2])>200000) *(int *)(argv[2])=200000;
  } else if (strcmp(field,"arrow_width")==0) {
    if (*(int *)(argv[2])<10000) *(int *)(argv[2])=10000;
    else if (*(int *)(argv[2])>200000) *(int *)(argv[2])=200000;
  } else if ((strcmp(field,"wave_length")==0)
          || (strcmp(field,"wave_width"))==0) {
    if (*(int *)(argv[2])<1) *(int *)(argv[2])=1;
  } else if (strcmp(field,"num_pt")==0) {
    if (*(int *)(argv[2])<TEXT_SIZE_MIN) *(int *)(argv[2])=TEXT_SIZE_MIN;
  } else if (strcmp(field,"num_script_size")==0) {
    if (*(int *)(argv[2])<1000) *(int *)(argv[2])=1000;
	else if (*(int *)(argv[2])>100000) *(int *)(argv[2])=100000;
  } else if (strcmp(field,"num_format")==0) {
    format=(char *)(argv[2]);
    if (format==NULL) {
      error(obj,ERRFORMAT);
      return 1;
    }
    if (format[0]!='%') {
      error(obj,ERRFORMAT);
      return 1;
    }
    sharp=minus=plus=FALSE;
    for (i=1;(format[i]!='\0') && (strchr("#-+",format[i])!=NULL);i++) {
      if (format[i]=='#') {
        if (sharp) {
          error(obj,ERRFORMAT);
          return 1;
        } else sharp=TRUE;
      }
      if (format[i]=='-') {
        if (minus) {
          error(obj,ERRFORMAT);
          return 1;
        } else minus=TRUE;
      }
      if (format[i]=='+') {
        if (plus) {
          error(obj,ERRFORMAT);
          return 1;
        } else plus=TRUE;
      }
    }
    if (format[i]=='0') i++;
    for (j=i;isdigit(format[i]);i++) ;
    if (j-i>2) {
      error(obj,ERRFORMAT);
      return 1;
    }
    if (format[i]=='.') {
      i++;
      for (j=i;isdigit(format[i]);i++) ;
      if ((j-i>2) || (j==i)) {
        error(obj,ERRFORMAT);
        return 1;
      }
    }
    if (format[i]=='\0') {
      error(obj,ERRFORMAT);
      return 1;
    }
    if (strchr("efgEG",format[i])==NULL) {
      error(obj,ERRFORMAT);
      return 1;
    }
    if (format[i+1]!='\0') {
      error(obj,ERRFORMAT);
      return 1;
    }
  } else if (strcmp(field,"num_num")==0) {
    if (*(int *)(argv[2])<-1) *(int *)(argv[2])=-1;
  }
  return 0;
}

static int
axisgeometry(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  N_VALUE *inst_array[INST_ARRAY_NUM];
  int i;

  get_axis_group_type(obj, inst, inst_array, TRUE);

  for (i = 0; i < INST_ARRAY_NUM; i++) {
    if (inst_array[i]) {
      if (clear_bbox(obj, inst_array[i])) {
	return 1;
      }
    }
  }

  return 0;
}

static int
axisdirection(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int dir;

  dir = * (int *) argv[2];

  dir %= 36000;
  if (dir < 0)
    dir += 36000;

  * (int *) argv[2] = dir;

  return axisgeometry(obj, inst, rval, argc, argv);
}

static void
axis_get_box(struct objlist *obj,N_VALUE *inst, int *pos)
{
  int minx, miny, maxx, maxy;
  int x0, y0, x1, y1, length, direction;
  double dir;

  _getobj(obj, "x", inst, &x0);
  _getobj(obj, "y", inst, &y0);
  _getobj(obj, "length", inst, &length);
  _getobj(obj, "direction", inst, &direction);

  dir = direction / 18000.0 * MPI;
  x1 = x0 + nround(length * cos(dir));
  y1 = y0 - nround(length * sin(dir));
  maxx = minx = x0;
  maxy = miny = y0;

  if (x1 < minx) minx = x1;
  if (x1 > maxx) maxx = x1;
  if (y1 < miny) miny = y1;
  if (y1 > maxy) maxy = y1;

  pos[0] = minx;
  pos[1] = miny;
  pos[2] = maxx;
  pos[3] = maxy;
  pos[4] = x0;
  pos[5] = y0;
  pos[6] = x1;
  pos[7] = y1;
#define POS_ARRAY_SIZE 8
}


static int
axisbbox2(struct objlist *obj, N_VALUE *inst, struct narray **rval)
{
  int i, pos[POS_ARRAY_SIZE];
  struct narray *array;

  if (inst == NULL)
    return 1;

  array = *rval;

  if (arraynum(array) != 0)
    return 0;

  if (array == NULL && (array = arraynew(sizeof(int))) == NULL)
    return 1;

  axis_get_box(obj, inst, pos);

  for (i = 0; i < POS_ARRAY_SIZE; i++) {
    arrayadd(array, pos + i);
  }

  if (arraynum(array) == 0) {
    arrayfree(array);
    return 1;
  }

  *rval = array;

  return 0;
}

static int
check_direction(struct objlist *obj, int type, N_VALUE **inst_array)
{
  int i, n, direction, normal_dir[] = {0, 9000, 0, 9000};

  switch (type) {
  case 'f':
  case 's':
    for (i = 0; i < 2; i++) {
      int p1, p2, len1, len2;
      char *field;

      _getobj(obj, "length", inst_array[0 + i], &len1);
      _getobj(obj, "length", inst_array[2 + i], &len2);

      if ((len1 > 0 && len2 < 0) || (len1 < 0 && len2 > 0))
	return 1;

      field = (i == 0) ? "x" : "y";

      _getobj(obj, field, inst_array[1 - i], &p1);
      _getobj(obj, field, inst_array[3 - i], &p2);

      switch (i) {
      case 0:
	if ((len1 < 0 && p2 > p1) || (len1 > 0 && p2 < p1))
	  return 1;
	break;
      case 1:
	if ((len1 < 0 && p2 < p1) || (len1 > 0 && p2 > p1))
	  return 1;
	break;
      }
    }

    n = 4;
    break;
  case 'c':
    n = 2;
    break;
  case 'a':
  default:
    return 1;
  }

  for (i = 0; i < n; i++) {
    _getobj(obj, "direction", inst_array[i], &direction);
    if (direction != normal_dir[i]) {
      return 1;
    }
  }

  return 0;
}

#define FIND_X 1
#define FIND_Y 2
#define FIND_U 4
#define FIND_R 8
#define FIND_FRAME (FIND_X | FIND_Y | FIND_U | FIND_R)
#define FIND_CROSS (FIND_X | FIND_Y)
#define CHECK_FRAME(a) ((a & FIND_FRAME) == FIND_FRAME)
#define CHECK_CROSS(a) ((a & FIND_CROSS) == FIND_CROSS)

static int
get_axis_group_type(struct objlist *obj, N_VALUE *inst, N_VALUE **inst_array, int check_all)
{
  char *group, *group2;
  N_VALUE *inst2;
  char type;
  int find_axis, len, id, i;

  inst_array[0] = inst;

  for (i = 1; i < INST_ARRAY_NUM; i++) {
    inst_array[i] = NULL;
  }

  _getobj(obj, "group", inst, &group);

  if (group == NULL || group[0] == 'a')
    return 'a';

  len = strlen(group);
  if (len < 3)
    return 'a';

  if (check_all) {
    id = chkobjlastinst(obj);
  } else {
    _getobj(obj, "id", inst, &id);
  }

  find_axis = 0;

  type = group[0];

  for (i = 0; i <= id; i++) {
    inst2 = chkobjinst(obj, i);
    _getobj(obj, "group", inst2, &group2);

    if (group2 == NULL || group2[0] != type)
      continue;

    len = strlen(group2);
    if (len < 3)
      continue;

    if (strcmp(group + 2, group2 + 2))
      continue;

    switch (group2[1]) {
    case 'X':
      find_axis |= FIND_X;
      inst_array[0] = inst2;
      break;
    case 'Y':
      find_axis |= FIND_Y;
      inst_array[1] = inst2;
      break;
    case 'U':
      find_axis |= FIND_U;
      inst_array[2] = inst2;
      break;
    case 'R':
      find_axis |= FIND_R;
      inst_array[3] = inst2;
      break;
    }

    switch (type) {
    case 'f':
    case 's':
      if (CHECK_FRAME(find_axis)) {
	return type;
      }
      break;
    case 'c':
      if (CHECK_CROSS(find_axis)) {
	return type;
      }
      break;
    default:
      return 'a';
    }
  }

  return -1;
}

static int
get_axis_box(struct objlist *obj, N_VALUE *inst, int *minx, int *miny, int *maxx, int *maxy)
{
  struct narray *rval2;

  rval2 = NULL;

  if (axisbbox2(obj, inst, &rval2))
    return 1;

  *minx = arraynget_int(rval2, 0);
  *miny = arraynget_int(rval2, 1);
  *maxx = arraynget_int(rval2, 2);
  *maxy = arraynget_int(rval2, 3);
  arrayfree(rval2);

  return 0;
}

static struct narray *
set_axis_box(struct narray *array, int minx, int miny, int maxx, int maxy, int add_point)
{
  if (array == NULL && ((array = arraynew(sizeof(int))) == NULL))
    return NULL;

  arrayadd(array,&minx);
  arrayadd(array,&miny);
  arrayadd(array,&maxx);
  arrayadd(array,&maxy);

  if (add_point) {
    arrayadd(array,&minx);
    arrayadd(array,&miny);

    arrayadd(array,&maxx);
    arrayadd(array,&miny);

    arrayadd(array,&maxx);
    arrayadd(array,&maxy);

    arrayadd(array,&minx);
    arrayadd(array,&maxy);
  }

  if (arraynum(array)==0) {
    arrayfree(array);
    return NULL;
  }

  return array;
}

static int
get_axis_group_box(struct objlist *obj, N_VALUE **inst_array, int type, int *minx, int *miny, int *maxx, int *maxy)
{
  int x0, y0, x1, y1, i;

  switch (type) {
  case 'a':
    if (get_axis_box(obj, inst_array[0], minx, miny, maxx, maxy))
      return 1;
    break;
  case 'f':
  case 's':
    if (get_axis_box(obj, inst_array[0], minx, miny, maxx, maxy))
      return 1;

    for (i = 1; i < 4; i++) {
      if (get_axis_box(obj, inst_array[i], &x0, &y0, &x1, &y1))
	return 1;

      if (x0 < *minx) *minx = x0;
      if (y0 < *miny) *miny = y0;
      if (x1 > *maxx) *maxx = x1;
      if (y1 > *maxy) *maxy = y1;
    }
    break;
  case 'c':
    if (get_axis_box(obj, inst_array[0], minx, miny, maxx, maxy))
      return 1;

    if (get_axis_box(obj, inst_array[1], &x0, &y0, &x1, &y1))
      return 1;

    if (x0 < *minx) *minx = x0;
    if (y0 < *miny) *miny = y0;
    if (x1 > *maxx) *maxx = x1;
    if (y1 > *maxy) *maxy = y1;
    break;
  }

  return 0;
}

static int
axisbbox(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int type, dir;
  N_VALUE *inst_array[INST_ARRAY_NUM];
  struct narray *array;
  int minx,miny,maxx,maxy;

  array = rval->array;
  if (arraynum(array) != 0)
    return 0;

  type = get_axis_group_type(obj, inst, inst_array, FALSE);

  switch (type) {
  case 'a':
    return axisbbox2(obj, inst, &rval->array);
    break;
  case 'f':
  case 's':
  case 'c':
    if (get_axis_group_box(obj, inst_array, type, &minx, &miny, &maxx, &maxy))
      return 1;

    dir = check_direction(obj, type, inst_array);
    array = set_axis_box(array, minx, miny, maxx, maxy, ! dir);
    if (array == NULL) {
      rval->array = NULL;
      return 1;
    }

    rval->array = array;
    break;
  }

  return 0;
}

static int
axismatch2(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy,err;
  int *data;
  struct narray *array;

  rval->i=FALSE;
  array=NULL;
  axisbbox2(obj,inst,&array);
  if (array==NULL) return 0;
  minx=*(int *)argv[2];
  miny=*(int *)argv[3];
  maxx=*(int *)argv[4];
  maxy=*(int *)argv[5];
  err=*(int *)argv[6];
  if ((minx==maxx) && (miny==maxy)) {
    int i, num;
    num=arraynum(array)-4;
    data=arraydata(array);
    for (i=0;i<num-2;i+=2) {
      double x1,y1,x2,y2;
      double r,r2,r3;
      x1=data[4+i];
      y1=data[5+i];
      x2=data[6+i];
      y2=data[7+i];
      r2=sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
      r=sqrt((minx-x1)*(minx-x1)+(miny-y1)*(miny-y1));
      r3=sqrt((minx-x2)*(minx-x2)+(miny-y2)*(miny-y2));
      if ((r<=err) || (r3<err)) {
        rval->i=TRUE;
        break;
      }
      if (r2!=0) {
        double ip;
        ip=((x2-x1)*(minx-x1)+(y2-y1)*(miny-y1))/r2;
        if ((0<=ip) && (ip<=r2)) {
          x2=x1+(x2-x1)*ip/r2;
          y2=y1+(y2-y1)*ip/r2;
          r=sqrt((minx-x2)*(minx-x2)+(miny-y2)*(miny-y2));
          if (r<err) {
            rval->i=TRUE;
            break;
          }
        }
      }
    }
  } else {
    int bminx,bminy,bmaxx,bmaxy;
    if (arraynum(array)<4) {
      arrayfree(array);
      return 1;
    }
    bminx=arraynget_int(array,0);
    bminy=arraynget_int(array,1);
    bmaxx=arraynget_int(array,2);
    bmaxy=arraynget_int(array,3);
    if ((minx<=bminx) && (bminx<=maxx)
     && (minx<=bmaxx) && (bmaxx<=maxx)
     && (miny<=bminy) && (bminy<=maxy)
     && (miny<=bmaxy) && (bmaxy<=maxy)) rval->i=TRUE;
  }
  arrayfree(array);
  return 0;
}

static int
axismatch(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int n, type;
  N_VALUE *inst_array[INST_ARRAY_NUM];
  int rval2;

  rval->i = FALSE;

  type = get_axis_group_type(obj, inst, inst_array, FALSE);

  n = 0;
  switch (type) {
  case 'a':
    return axismatch2(obj, inst, rval, argc, argv);
    break;
  case 'f':
  case 's':
    n = 4;
    break;
  case 'c':
    n = 2;
    break;
  }

  if (n) {
    int i, match;
    match = FALSE;

    for (i = 0; i < n; i++) {
      axismatch2(obj, inst_array[i], (void *)&rval2, argc, argv);
      match = match || rval2;
    }

    rval->i = match;
  }

  return 0;
}

static int
axismove2(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int x,y;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  x+=*(int *)argv[2];
  y+=*(int *)argv[3];
  if (_putobj(obj,"x",inst,&x)) return 1;
  if (_putobj(obj,"y",inst,&y)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
axismove(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int i;
  N_VALUE *inst_array[INST_ARRAY_NUM];

  if (* (int *) argv[2] == 0 && * (int *) argv[3] == 0)
    return 0;

  if (get_axis_group_type(obj, inst, inst_array, FALSE) < 0)
    return 1;

  for (i = 0; i < INST_ARRAY_NUM; i++) {
    if (inst_array[i]) {
      axismove2(obj, inst_array[i], rval, argc, argv);
    }
  }

  return 0;
}

static int
axisrotate2(struct objlist *obj, N_VALUE *inst, int px, int py, int angle)
{
  int x, y, dir;

  if (inst == NULL)
    return 1;

  _getobj(obj, "x", inst, &x);
  _getobj(obj, "y", inst, &y);
  _getobj(obj, "direction", inst, &dir);

  rotate(px, py, angle, &x, &y);
  dir += angle;
  dir %= 36000;

  if (_putobj(obj, "x", inst, &x)) return 1;
  if (_putobj(obj, "y", inst, &y)) return 1;
  if (_putobj(obj, "direction", inst, &dir)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
axisrotate(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int i, n, type, angle, use_pivot, px, py, minx, miny, maxx, maxy;
  N_VALUE *inst_array[INST_ARRAY_NUM];

  angle = *(int *) argv[2];
  use_pivot = * (int *) argv[3];
  px = *(int *) argv[4];
  py = *(int *) argv[5];

  angle %= 36000;
  if (angle < 0)
    angle += 36000;

  type = get_axis_group_type(obj, inst, inst_array, FALSE);

  switch (type) {
  case 'a':
    n = 1;
    break;
  case 'f':
  case 's':
    n = 4;
    break;
  case 'c':
    n = 2;
    break;
  default:
    return 0;
  }

  if (! use_pivot) {
    if (get_axis_group_box(obj, inst_array, type, &minx, &miny, &maxx, &maxy))
      return 1;

    px = (minx + maxx) / 2;
    py = (miny + maxy) / 2;
  }

  for (i = 0; i < n; i++) {
    axisrotate2(obj, inst_array[i], px, py, angle);
  }

  return 0;
}

static int
axisflip2(struct objlist *obj, N_VALUE *inst, int px, int py, enum FLIP_DIRECTION dir)
{
  int x, y, a, p, g_dir, n_dir, n_align;

  if (inst == NULL)
    return 1;

  _getobj(obj, "x", inst, &x);
  _getobj(obj, "y", inst, &y);
  _getobj(obj, "direction", inst, &a);
  _getobj(obj, "gauge", inst, &g_dir);
  _getobj(obj, "num", inst, &n_dir);
  _getobj(obj, "num_align", inst, &n_align);

  switch (dir) {
  case FLIP_DIRECTION_HORIZONTAL:
    a = 18000 - a;
    p = px;
    switch (n_align) {
    case AXIS_NUM_ALIGN_LEFT:
      n_align = AXIS_NUM_ALIGN_RIGHT;
      break;
    case AXIS_NUM_ALIGN_RIGHT:
      n_align= AXIS_NUM_ALIGN_LEFT;
      break;
    }
    break;
  case FLIP_DIRECTION_VERTICAL:
    a = -a;
    p = py;
    break;
  default:
    p = 0;
  }

  switch (g_dir) {
  case AXIS_GAUGE_LEFT:
    g_dir = AXIS_GAUGE_RIGHT;
    break;
  case AXIS_GAUGE_RIGHT:
    g_dir = AXIS_GAUGE_LEFT;
    break;
  };

  switch (n_dir) {
  case AXIS_NUM_POS_LEFT:
    n_dir = AXIS_NUM_POS_RIGHT;
    break;
  case AXIS_NUM_POS_RIGHT:
    n_dir = AXIS_NUM_POS_LEFT;
    break;
  }

  a %= 36000;
  a += (a < 0) ? 36000 : 0;

  flip(p, dir, &x, &y);

  if (_putobj(obj, "x", inst, &x)) return 1;
  if (_putobj(obj, "y", inst, &y)) return 1;
  if (_putobj(obj, "direction", inst, &a)) return 1;
  if (_putobj(obj, "gauge", inst, &g_dir)) return 1;
  if (_putobj(obj, "num", inst, &n_dir)) return 1;
  if (_putobj(obj, "num_align", inst, &n_align)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
axisflip(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int i, n, type, use_pivot, px, py, minx, miny, maxx, maxy;
  N_VALUE *inst_array[INST_ARRAY_NUM];
  enum FLIP_DIRECTION dir;

  dir = (* (int *) argv[2] == FLIP_DIRECTION_HORIZONTAL) ? FLIP_DIRECTION_HORIZONTAL : FLIP_DIRECTION_VERTICAL;
  use_pivot = * (int *) argv[3];
  px = py = *(int *) argv[4];

  type = get_axis_group_type(obj, inst, inst_array, FALSE);

  switch (type) {
  case 'a':
    n = 1;
    break;
  case 'f':
  case 's':
    n = 4;
    break;
  case 'c':
    n = 2;
    break;
  default:
    return 0;
  }

  if (! use_pivot) {
    if (get_axis_group_box(obj, inst_array, type, &minx, &miny, &maxx, &maxy))
      return 1;

    px = (minx + maxx) / 2;
    py = (miny + maxy) / 2;
  }

  for (i = 0; i < n; i++) {
    axisflip2(obj, inst_array[i], px, py, dir);
  }

  return 0;
}

static int
axischange2(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int len,dir,x,y;
  double x2,y2;
  int point,x0,y0;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"length",inst,&len);
  _getobj(obj,"direction",inst,&dir);
  x2=x+len*cos(dir*MPI/18000.0);
  y2=y-len*sin(dir*MPI/18000.0);
  point=*(int *)argv[2];
  x0=*(int *)argv[3];
  y0=*(int *)argv[4];
  if (point==0) {
    x+=x0;
    y+=y0;
  } else if (point==1) {
    x2+=x0;
    y2+=y0;
  }
  len=sqrt((x2-x)*(x2-x)+(y2-y)*(y2-y));
  if ((x2-x)==0) {
    if ((y2-y)==0) dir=0;
    else if ((y2-y)>0) dir=27000;
    else dir=9000;
  } else {
    dir=atan(-(y2-y)/(x2-x))/MPI*18000.0;
    if ((x2-x)<0) dir+=18000;
    if (dir<0) dir+=36000;
  }
  if (_putobj(obj,"x",inst,&x)) return 1;
  if (_putobj(obj,"y",inst,&y)) return 1;
  if (_putobj(obj,"length",inst,&len)) return 1;
  if (_putobj(obj,"direction",inst,&dir)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static void
axis_change_point0(struct objlist *obj, int type, N_VALUE **inst_array, int x0, int y0)
{
  int len, x, y;

  switch (type) {
  case 'f':
  case 's':
    _getobj(obj, "x", inst_array[2], &x);
    _getobj(obj, "y", inst_array[2], &y);
    _getobj(obj, "length", inst_array[2], &len);
    x += x0;
    y += y0;
    len -= x0;
    _putobj(obj, "x", inst_array[2], &x);
    _putobj(obj, "y", inst_array[2], &y);

    _putobj(obj, "length", inst_array[2], &len);
    _getobj(obj, "length", inst_array[3], &len);
    len -= y0;
    _putobj(obj, "length", inst_array[3], &len);
    /* FALLTHROUGH */
  case 'c':
    _getobj(obj, "x", inst_array[0], &x);
    _getobj(obj, "length", inst_array[0], &len);
    x += x0;
    len -= x0;
    _putobj(obj, "x", inst_array[0], &x);
    _putobj(obj, "length", inst_array[0], &len);

    _getobj(obj, "x", inst_array[1], &x);
    _getobj(obj, "length", inst_array[1], &len);
    x += x0;
    len -= y0;
    _putobj(obj, "x", inst_array[1], &x);
    _putobj(obj, "length", inst_array[1], &len);
    break;
  }
}

static void
axis_change_point1(struct objlist *obj, int type, N_VALUE **inst_array, int x0, int y0)
{
  int len, x, y;

  switch (type) {
  case 'f':
  case 's':
    _getobj(obj, "y", inst_array[2], &y);
    _getobj(obj, "length", inst_array[2], &len);
    y += y0;
    len += x0;
    _putobj(obj, "y", inst_array[2], &y);
    _putobj(obj, "length", inst_array[2], &len);

    _getobj(obj, "x", inst_array[3], &x);
    _getobj(obj, "length", inst_array[3], &len);
    x += x0;
    len -= y0;
    _putobj(obj, "x", inst_array[3], &x);
    _putobj(obj, "length", inst_array[3], &len);
    /* FALLTHROUGH */
  case 'c':
    _getobj(obj, "length", inst_array[0], &len);
    len += x0;
    _putobj(obj, "length", inst_array[0], &len);

    _getobj(obj, "length", inst_array[1], &len);
    len -= y0;
    _putobj(obj, "length", inst_array[1], &len);
    break;
  }
}

static void
axis_change_point2(struct objlist *obj, int type, N_VALUE **inst_array, int x0, int y0)
{
  int len, x, y;

  switch (type) {
  case 'f':
  case 's':
    _getobj(obj, "length", inst_array[2], &len);
    len += x0;
    _putobj(obj, "length", inst_array[2], &len);

    _getobj(obj, "x", inst_array[3], &x);
    _getobj(obj, "y", inst_array[3], &y);
    _getobj(obj, "length", inst_array[3], &len);
    x += x0;
    y += y0;
    len += y0;
    _putobj(obj, "x", inst_array[3], &x);
    _putobj(obj, "y", inst_array[3], &y);
    _putobj(obj, "length", inst_array[3], &len);
    /* FALLTHROUGH */
  case 'c':
    _getobj(obj, "y", inst_array[0], &y);
    _getobj(obj, "length", inst_array[0], &len);
    y += y0;
    len += x0;
    _putobj(obj, "y", inst_array[0], &y);
    _putobj(obj, "length", inst_array[0], &len);

    _getobj(obj, "y", inst_array[1], &y);
    _getobj(obj, "length", inst_array[1], &len);
    y += y0;
    len += y0;
    _putobj(obj, "y", inst_array[1], &y);
    _putobj(obj, "length", inst_array[1], &len);
    break;
  }
}

static void
axis_change_point3(struct objlist *obj, int type, N_VALUE **inst_array, int x0, int y0)
{
  int len, x, y;

  switch (type) {
  case 'f':
  case 's':
    _getobj(obj, "x", inst_array[2], &x);
    _getobj(obj, "length", inst_array[2], &len);
    x += x0;
    len -= x0;
    _putobj(obj, "x", inst_array[2], &x);
    _putobj(obj, "length", inst_array[2], &len);

    _getobj(obj, "y", inst_array[3], &y);
    _getobj(obj, "length", inst_array[3], &len);
    y += y0;
    len += y0;
    _putobj(obj, "y", inst_array[3], &y);
    _putobj(obj, "length", inst_array[3], &len);
    /* FALLTHROUGH */
  case 'c':
    _getobj(obj, "x", inst_array[0], &x);
    _getobj(obj, "y", inst_array[0], &y);
    _getobj(obj, "length", inst_array[0], &len);
    x += x0;
    y += y0;
    len -= x0;
    _putobj(obj, "x", inst_array[0], &x);
    _putobj(obj, "y", inst_array[0], &y);
    _putobj(obj, "length", inst_array[0], &len);

    _getobj(obj, "x", inst_array[1], &x);
    _getobj(obj, "y", inst_array[1], &y);
    _getobj(obj, "length", inst_array[1], &len);
    x += x0;
    y += y0;
    len += y0;
    _putobj(obj, "x", inst_array[1], &x);
    _putobj(obj, "y", inst_array[1], &y);
    _putobj(obj, "length", inst_array[1], &len);
    break;
  }
}

static int
axischange(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  N_VALUE *inst_array[INST_ARRAY_NUM];
  int type, point, x0, y0, len;

  type = get_axis_group_type(obj, inst, inst_array, FALSE);

  point= * (int *) argv[2];
  x0 = * (int *) argv[3];
  y0 = * (int *) argv[4];

  switch (type) {
  case 'a':
    return axischange2(obj, inst, rval, argc, argv);
    break;
  case 'f':
  case 's':
  case 'c':
    if (check_direction(obj, type, inst_array))
      return 0;

    _getobj(obj, "length", inst_array[0], &len);
    if (len < 0) {
      switch (point) {
      case 0: point = 1; break;
      case 1: point = 0; break;
      case 2: point = 3; break;
      case 3: point = 2; break;
      }
    }

    _getobj(obj, "length", inst_array[1], &len);
    if (len < 0) {
      switch (point) {
      case 0: point = 3; break;
      case 1: point = 2; break;
      case 2: point = 1; break;
      case 3: point = 0; break;
      }
    }

    switch (point) {
    case 0:
      axis_change_point0(obj, type, inst_array, x0, y0);
      break;
    case 1:
      axis_change_point1(obj, type, inst_array, x0, y0);
      break;
    case 2:
      axis_change_point2(obj, type, inst_array, x0, y0);
      break;
    case 3:
      axis_change_point3(obj, type, inst_array, x0, y0);
      break;
    }

    if (clear_bbox(obj, inst))
      return 1;

    break;
  }
  return 0;
}

static int
axiszoom2(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int x,y,len,refx,refy,preserve_width;
  double zoom_x, zoom_y, zoom_n, zoom_p, zoom;
  int pt,space,wid1,wid2,wid3,len1,len2,len3,wid,wlen,wwid,direction;
  struct narray *style,*gstyle;
  int snum,*sdata,gsnum,*gsdata;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  zoom_x = (*(int *) argv[2]) / 10000.0;
  zoom_y = (*(int *) argv[3]) / 10000.0;
  zoom = MIN(zoom_x, zoom_y);
  refx = (*(int *)argv[4]);
  refy = (*(int *)argv[5]);
  preserve_width = (*(int *)argv[6]);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"length",inst,&len);
  _getobj(obj,"width",inst,&wid);
  _getobj(obj,"num_pt",inst,&pt);
  _getobj(obj,"num_space",inst,&space);
  _getobj(obj,"wave_length",inst,&wlen);
  _getobj(obj,"wave_width",inst,&wwid);
  _getobj(obj,"gauge_length1",inst,&len1);
  _getobj(obj,"gauge_width1",inst,&wid1);
  _getobj(obj,"gauge_length2",inst,&len2);
  _getobj(obj,"gauge_width2",inst,&wid2);
  _getobj(obj,"gauge_length3",inst,&len3);
  _getobj(obj,"gauge_width3",inst,&wid3);
  _getobj(obj,"gauge_style",inst,&gstyle);
  _getobj(obj,"style",inst,&style);
  _getobj(obj,"direction",inst,&direction);
  snum=arraynum(style);
  sdata=arraydata(style);
  gsnum=arraynum(gstyle);
  gsdata=arraydata(gstyle);
  direction = calc_zoom_direction(direction, zoom_x, zoom_y, &zoom_p, &zoom_n);
  len*=zoom_p;
  wlen*=zoom_n;
  len1*=zoom_n;
  len2*=zoom_n;
  len3*=zoom_n;
  x=(x-refx)*zoom_x+refx;
  y=(y-refy)*zoom_y+refy;
  pt*=zoom;
  space*=zoom;
  if (! preserve_width) {
    int i;
    wid*=zoom;
    wid1*=zoom;
    wid2*=zoom;
    wid3*=zoom;
    wwid*=zoom;
    for (i=0;i<snum;i++) sdata[i]=sdata[i]*zoom;
    for (i=0;i<gsnum;i++) gsdata[i]=gsdata[i]*zoom;
  }
  if (_putobj(obj,"x",inst,&x)) return 1;
  if (_putobj(obj,"y",inst,&y)) return 1;
  if (_putobj(obj,"length",inst,&len)) return 1;
  if (_putobj(obj,"width",inst,&wid)) return 1;
  if (_putobj(obj,"num_pt",inst,&pt)) return 1;
  if (_putobj(obj,"num_space",inst,&space)) return 1;
  if (_putobj(obj,"wave_length",inst,&wlen)) return 1;
  if (_putobj(obj,"wave_width",inst,&wwid)) return 1;
  if (_putobj(obj,"gauge_length1",inst,&len1)) return 1;
  if (_putobj(obj,"gauge_width1",inst,&wid1)) return 1;
  if (_putobj(obj,"gauge_length2",inst,&len2)) return 1;
  if (_putobj(obj,"gauge_width2",inst,&wid2)) return 1;
  if (_putobj(obj,"gauge_length3",inst,&len3)) return 1;
  if (_putobj(obj,"gauge_width3",inst,&wid3)) return 1;
  if (_putobj(obj,"direction",inst,&direction)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
axiszoom(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int i;
  N_VALUE *inst_array[INST_ARRAY_NUM];

  if (get_axis_group_type(obj, inst, inst_array, FALSE) < 0)
    return 1;

  for (i = 0; i < INST_ARRAY_NUM; i++) {
    if (inst_array[i]) {
      axiszoom2(obj, inst_array[i], rval, argc, argv);
    }
  }
  return 0;
}

static void
get_num_pos_horizontal(int align, int plen, int fx0, int fy0, int fx1, int fy1, int *x, int *y)
{
  switch (align) {
  case AXIS_NUM_ALIGN_CENTER:
    *x = (fx0 + fx1) / 2;
    break;
  case AXIS_NUM_ALIGN_LEFT:
    *x = fx0;
    break;
  case AXIS_NUM_ALIGN_RIGHT:
    *x = fx1;
    break;
  case AXIS_NUM_ALIGN_POINT:
    *x = fx1 + plen / 2;
    break;
  default:
    /* never reached */
    *x = 0;
  }
  *y = (fy0 + fy1) / 2;
}

static void
get_num_pos(int align, int plen, double nndir, int fx0, int fy0, int fx1, int fy1, int *x, int *y)
{
  int px0, py0;

  switch (align) {
  case AXIS_NUM_ALIGN_CENTER:
    px0 = (fx0 + fx1) / 2;
    break;
  case AXIS_NUM_ALIGN_LEFT:
    px0 = fx0;
    break;
  case AXIS_NUM_ALIGN_RIGHT:
    px0 = fx1;
    break;
  case AXIS_NUM_ALIGN_POINT:
    px0 = fx1 + plen / 2;
    break;
  default:
    /* never reached */
    px0 = 0;
  }
  py0 = (fy0 + fy1) / 2;

  *x =  cos(nndir) * px0 + sin(nndir) * py0;
  *y = -sin(nndir) * px0 + cos(nndir) * py0;
}

static void
get_num_pos_oblique(int align, int plen, double nndir, int fx0, int fy0, int fx1, int fy1, int *x, int *y)
{
  int px0, py0;

  switch (align) {
  case AXIS_NUM_ALIGN_LEFT:
    px0 = fx0;
    break;
  case AXIS_NUM_ALIGN_RIGHT:
    px0 = fx1;
    break;
  default:
    /* never reached */
    px0 = fx0;
  }
  py0 = fy0;

  *x =  cos(nndir) * px0 + sin(nndir) * py0;
  *y = -sin(nndir) * px0 + cos(nndir) * py0;
}

struct font_config {
  char *font;
  int style, pt, space, scriptsize;
};

struct axis_config {
  int type;
  double min, max, inc;
  int div;
  int x0, y0, x1, y1;		/* start and atop position of baseline */
  int length, width;
  int direction;		/* direction in degree multiplied 100 */
  double dir;			/* direction in radian */
  MathEquation *code;
};


static void
get_num_ofst_horizontal(const struct axis_config *aconf, int align, int side,
			int ilenmax, int plen,
			int hx0, int hy0, int hx1, int hy1,
			int *x1, int *y1, int *x2, int *y2, int *len)
{
  int fx0,fy0,px0,px1,py0,py1;
  double t;

  if (side==AXIS_NUM_POS_LEFT) {
    if (aconf->direction<9000) {
      px0=hx1;
      py0=hy1;
    } else if (aconf->direction<18000) {
      px0=hx1;
      py0=hy0;
    } else if (aconf->direction<27000) {
      px0=hx0;
      py0=hy0;
    } else {
      px0=hx0;
      py0=hy1;
    }
  } else {
    if (aconf->direction<9000) {
      px0=hx0;
      py0=hy0;
    } else if (aconf->direction<18000) {
      px0=hx0;
      py0=hy1;
    } else if (aconf->direction<27000) {
      px0=hx1;
      py0=hy1;
    } else {
      px0=hx1;
      py0=hy0;
    }
  }

  switch (align) {
  case AXIS_NUM_ALIGN_CENTER:
    px1=(hx0+hx1)/2;
    break;
  case AXIS_NUM_ALIGN_LEFT:
    px1=hx0;
    break;
  case AXIS_NUM_ALIGN_RIGHT:
    px1=hx1;
    break;
  case AXIS_NUM_ALIGN_POINT:
    px1=ilenmax+plen/2;
    break;
  default:
    /* never reached */
    px1 = 0;
  }
  py1=(hy0+hy1)/2;
  fx0=px1-px0;
  fy0=py1-py0;
  t=cos(aconf->dir)*fx0-sin(aconf->dir)*fy0;
  *x1=fx0-t*cos(aconf->dir);
  *y1=fy0+t*sin(aconf->dir);
  if (side==AXIS_NUM_POS_LEFT) {
    if (aconf->direction<9000) px1=hx1;
    else if (aconf->direction<18000) px1=hx1;
    else if (aconf->direction<27000) px1=hx0;
    else px1=hx0;
  } else {
    if (aconf->direction<9000) px1=hx0;
    else if (aconf->direction<18000) px1=hx0;
    else if (aconf->direction<27000) px1=hx1;
    else px1=hx1;
  }
  py1=(hy0+hy1)/2;
  fx0=px1-px0;
  fy0=py1-py0;
  t=cos(aconf->dir)*fx0-sin(aconf->dir)*fy0;
  *x2=fx0-t*cos(aconf->dir);
  *y2=fy0+t*sin(aconf->dir);
  *len=abs(hx1-hx0);
}

static void
get_num_ofst_parallel(const struct axis_config *aconf, int side,
		      int hx0, int hy0, int hx1, int hy1,
		      int *x1, int *y1, int *x2, int *y2, int *len)
{
  int py1;

  py1=(hy0+hy1)/2;
  if (side==AXIS_NUM_POS_LEFT) py1*=-1;
  *x1=-py1*sin(aconf->dir);
  *y1=-py1*cos(aconf->dir);
  *x2=-py1*sin(aconf->dir);
  *y2=-py1*cos(aconf->dir);
  *len=abs(hx1-hx0);
}

static void
get_num_ofst_normal1(const struct axis_config *aconf, int align, int side,
		     int ilenmax, int plen,
		     int hx0, int hy0, int hx1, int hy1,
		     int *x1, int *y1, int *x2, int *y2, int *len)
{
  int py1;

  switch (side) {
  case AXIS_NUM_POS_LEFT:
    switch (align) {
    case AXIS_NUM_ALIGN_CENTER:
      py1 = (hx1 - hx0) / 2;
      break;
    case AXIS_NUM_ALIGN_LEFT:
      py1 = hx1 - hx0;
      break;
    case AXIS_NUM_ALIGN_RIGHT:
      py1 = 0;
      break;
    case AXIS_NUM_ALIGN_POINT:
      py1 = plen / 2;
      break;
    default:
      /* never reached */
      py1 = 0;
    }
    break;
  case AXIS_NUM_POS_RIGHT:
    switch (align) {
    case AXIS_NUM_ALIGN_CENTER:
      py1 = (hx0 - hx1) / 2;
      break;
    case AXIS_NUM_ALIGN_LEFT:
      py1 = 0;
      break;
    case AXIS_NUM_ALIGN_RIGHT:
      py1 = hx0 - hx1;
      break;
    case AXIS_NUM_ALIGN_POINT:
      py1 = hx0 - hx1 + plen / 2;
      break;
    default:
      /* never reached */
      py1 = 0;
    }
    break;
  default:
    /* never reached */
    py1 = 0;
  }
  *x1 = -py1 * sin(aconf->dir);
  *y1 = -py1 * cos(aconf->dir);
  *x2 = *y2 = 0;
  *len = abs(hx1 - hx0);
}

static void
get_num_ofst_normal2(const struct axis_config *aconf, int align, int side,
		     int ilenmax, int plen,
		     int hx0, int hy0, int hx1, int hy1,
		     int *x1, int *y1, int *x2, int *y2, int *len)
{
  int py1;

  switch (side) {
  case AXIS_NUM_POS_LEFT:
    switch (align) {
    case AXIS_NUM_ALIGN_CENTER:
      py1 = (hx1 - hx0) / 2;
      break;
    case AXIS_NUM_ALIGN_LEFT:
      py1 = hx1 - hx0;
      break;
    case AXIS_NUM_ALIGN_RIGHT:
      py1 = 0;
      break;
    case AXIS_NUM_ALIGN_POINT:
      py1 = hx1 - ilenmax - plen / 2;
      break;
    default:
      /* never reached */
      py1 = 0;
    }
    break;
  case AXIS_NUM_POS_RIGHT:
    switch (align) {
    case AXIS_NUM_ALIGN_CENTER:
      py1 = (hx0 - hx1) / 2;
      break;
    case AXIS_NUM_ALIGN_LEFT:
      py1 = 0;
      break;
    case AXIS_NUM_ALIGN_RIGHT:
      py1 = hx0 - hx1;
      break;
    case AXIS_NUM_ALIGN_POINT:
      py1 = hx0 - ilenmax - plen / 2;
      break;
    default:
      /* never reached */
      py1 = 0;
    }
    break;
  default:
    /* never reached */
    py1 = 0;
  }
  *x1 = -py1 * sin(aconf->dir);
  *y1 = -py1 * cos(aconf->dir);
  *x2 = *y2 = 0;
  *len = abs(hx1 - hx0);
}

static void
get_num_ofst_oblique1(const struct axis_config *aconf, int align, int side,
		   int ilenmax, int plen,
		   int hx0, int hy0, int hx1, int hy1,
		   int *x1, int *y1, int *x2, int *y2, int *len)
{
  int w, h;
  double sin_t, cos_t;

  switch (side) {
  case AXIS_NUM_POS_LEFT:
    w = abs(hx1 - hx0);
    h = abs(hy1 - hy0);

    cos_t = cos(aconf->dir + MPI / 4);
    sin_t = sin(aconf->dir + MPI / 4);

    *x1 =   w * cos_t - h * sin_t;
    *y1 = - w * sin_t - h * cos_t;
    break;
  case AXIS_NUM_POS_RIGHT:
    *x1 = *y1 = 0;
    break;
  default:
    /* never reached */
    *x1 = 0;
    *y1 = 0;
  }

  *x2 = *y2 = 0;
  *len = abs(hx1 - hx0);
}

static void
get_num_ofst_oblique2(const struct axis_config *aconf, int align, int side,
		   int ilenmax, int plen,
		   int hx0, int hy0, int hx1, int hy1,
		   int *x1, int *y1, int *x2, int *y2, int *len)
{
  int w, h;
  double sin_t, cos_t;

  switch (side) {
  case AXIS_NUM_POS_LEFT:
    w = abs(hx1 - hx0);
    h = abs(hy1 - hy0);

    cos_t = cos(aconf->dir - MPI / 4);
    sin_t = sin(aconf->dir - MPI / 4);

    *x1 = - w * cos_t - h * sin_t;
    *y1 =   w * sin_t - h * cos_t;
    break;
  case AXIS_NUM_POS_RIGHT:
    *x1 = *y1 = 0;
    break;
  default:
    /* never reached */
    *x1 = 0;
    *y1 = 0;
  }

  *x2 = *y2 = 0;
  *len = abs(hx1 - hx0);
}

static char *
mjd_to_date_str(const struct axis_config *aconf, double mjd, const gchar *date_format)
{
  struct tm tm;
  const gchar
    *fmt_y = "%Y",
    *fmt_ym = "%Y-%m",
    *fmt_ymd = "%Y-%m-%d",
    *fmt_ymdhms = "%Y-%m-%d\\&\\n%H:%M:%S\\&",
    *fmt_ymdhm = "%Y-%m-%d\\&\\n%H:%M\\&",
    *fmt_hms = "%H:%M:%S",
    *fmt_hm = "%H:%M",
    *fmt;

  mjd2gd(mjd, &tm);
  if (tm.tm_year < MJD2GD_YEAR_MIN) {
    return NULL;
  }

  if (date_format && date_format[0]) {
    fmt = date_format;
  } else {
    if (fabs(aconf->max - aconf->min) < 1) {
      if (tm.tm_sec == 0) {
	fmt = fmt_hm;
      } else {
	fmt = fmt_hms;
      }
    } else if (fabs(aconf->max - aconf->min) > 365) {
      if (tm.tm_sec == 0) {
	if (tm.tm_hour == 0 && tm.tm_min == 0) {
	  if (tm.tm_mday == 1) {
	    if (tm.tm_mon == 0) {
	      fmt = fmt_y;
	    } else {
	      fmt = fmt_ym;
	    }
	  } else {
	    fmt = fmt_ymd;
	  }
	} else {
	  fmt = fmt_ymdhm;
	}
      } else {
	fmt = fmt_ymdhms;
      }
    } else {
      if (tm.tm_sec == 0) {
	if (tm.tm_hour == 0 && tm.tm_min == 0) {
	  fmt = fmt_ymd;
	} else {
	  fmt = fmt_ymdhm;
	}
      } else {
	fmt = fmt_ymdhms;
      }
    }
  }

  return nstrftime(fmt, mjd);
}

static char *
get_axis_gauge_num_str(const char *format, double a, int no_flt_zero)
{
  int i, j, len;
  char *s;
  char pm[] = "±";
  GString *num;

  if (no_flt_zero && a == 0.0) {
    return g_strdup("0");
  }

  num = g_string_sized_new(16);
  if (num == NULL) {
    return NULL;
  }

  s = strchr(format, '+');
  if (a == 0 && s) {
    char format2[256];
    len = strlen(format);
    for (j = 0; j < (int) sizeof(pm) - 1; j++) {
      format2[j] = pm[j];
    }
    for (i = 0; i < len && j < (int) sizeof(format2) - 1; i++) {
      format2[j] = format[i];
      if (format[i] != '+') {
	j++;
      }
    }
    format2[j] = '\0';
    n_gstr_printf_double(num, format2, a);
  } else {
    n_gstr_printf_double(num, format, a);
  }

  len = num->len;
  for (i = 0; i < len; i++) {
    if (num->str[i] == 'E' || num->str[i] == 'e') {
      switch (num->str[i + 1]) {
      case '+':
	j = 2;
	if (i + 2 < len && num->str[i + 2] == '0') {
	  j++;
	}
	g_string_erase(num, i, j);
	break;
      case '-':
	if (i + 2 < len && num->str[i + 2] == '0') {
	  g_string_erase(num, i + 2, 1);
	}
	g_string_erase(num, i, 1);
	break;
      case '0':
	/* never reached */
	g_string_erase(num, i, 2);
	break;
      default:
	/* never reached */
	g_string_erase(num, i, 1);
	break;
      }

      g_string_insert(num, i, "×10^");
      g_string_append_c(num, '@');
      break;
    }
  }

  return g_string_free(num, FALSE);
}

static double
calc_numbering_value(const struct axis_config *aconf, double po)
{
  MathValue val;

  if (aconf->code) {
    val.val = po;
    val.type = MATH_VALUE_NORMAL;
    math_equation_set_var(aconf->code, 0, &val);
    math_equation_calculate(aconf->code, &val);
    if (val.type == MATH_VALUE_NORMAL) {
      po = val.val;
    } else {
      po = 0;
    }
  }

  return po;
}

static double
numformat(char **text, int *nlen, const char *format,
	  const struct axis_config *aconf,
	  const struct axislocal *alocal,
	  int logpow, double po, double norm,
	  const char *head, const char *tail, const char *date_format, int nozero)
{
  int lpow;
  char *num;
  double a;

  *text = NULL;
  *nlen = 0;

  if ((! logpow && (alocal->atype == AXISLOGBIG || alocal->atype == AXISLOGNORM)) ||
      (alocal->atype == AXISLOGSMALL)) {
    po = calc_numbering_value(aconf, po);
    a = pow(10.0, po);
  } else if (alocal->atype == AXISINVERSE) {
    a = calc_numbering_value(aconf, 1.0 / po);
  } else {
    po = calc_numbering_value(aconf, po);
    a = po / norm;
  }

  if (aconf->type == AXIS_TYPE_MJD) {
    num = mjd_to_date_str(aconf, po, date_format);
    if (num) {
      logpow = 0;
    } else {
      num = get_axis_gauge_num_str(format, a, nozero == AXIS_NUM_NO_ZERO_NO_FLOATING_POINT);
    }
  } else {
    num = get_axis_gauge_num_str(format, a, nozero == AXIS_NUM_NO_ZERO_NO_FLOATING_POINT);
  }

  lpow = (logpow && (alocal->atype == AXISLOGBIG || alocal->atype == AXISLOGNORM));
  *text = g_strdup_printf("%s%s%s%s%s",
			  CHK_STR(head),
			  (lpow) ? "10^" : "",
			  CHK_STR(num),
			  (lpow) ? "@" : "",
			  CHK_STR(tail));

  if (num) {
    *nlen = strlen(num);
    g_free(num);
  } else {
    *nlen = 0;
  }

  return a;
}

static int
draw_numbering_normalize(int GC, int side, const struct axis_config *aconf,
			 const struct font_config *font, double norm,
			 int maxlen, int sx, int sy, int dlx2, int dly2,
			 int ndir, double nndir, int ndirection)
{
  int fx0, fy0, fx1, fy1, px0, px1, py0, py1, gx0, gy0;
  char num[256];

  if (norm/pow(10.0,cutdown(log10(norm)))==1) {
    //          sprintf(num,"[%%F{Symbol}%c%%F{%s}10^%+d@]", (char )0xb4,font,(int )cutdown(log10(norm)));
    snprintf(num, sizeof(num), "[×10^%+d@]", (int )cutdown(log10(norm)));
  } else {
    //          sprintf(num,"[%g%%F{Symbol}%c%%F{%s}10^%+d@]", norm/pow(10.0,cutdown(log10(norm))), (char )0xb4,font,(int )cutdown(log10(norm)));
    snprintf(num, sizeof(num), "[%g×10^%+d@]", norm/pow(10.0,cutdown(log10(norm))), (int )cutdown(log10(norm)));
  }
  GRAtextextent(num,font->font, font->style, font->pt, font->space, font->scriptsize,
		&fx0,&fy0,&fx1,&fy1,FALSE);

  if (abs(fy1-fy0)>maxlen)
    maxlen=abs(fy1-fy0);

  gx0=aconf->x0+(aconf->length+maxlen*1.2)*cos(aconf->dir);
  gy0=aconf->y0-(aconf->length+maxlen*1.2)*sin(aconf->dir);
  gx0=gx0-sy*sin(aconf->dir)+sx*cos(aconf->dir)+dlx2;
  gy0=gy0-sy*cos(aconf->dir)-sx*sin(aconf->dir)+dly2;
  switch (ndir) {
  case AXIS_NUM_POS_HORIZONTAL:
    if (side==AXIS_NUM_POS_LEFT) {
      if ((aconf->direction>4500) && (aconf->direction<=22500)) px1=fx1;
      else px1=fx0;
    } else {
      if ((aconf->direction>13500) && (aconf->direction<=31500)) px1=fx1;
      else px1=fx0;
    }
    py1=(fy0+fy1)/2;
    break;
  case AXIS_NUM_POS_PARALLEL1:
  case AXIS_NUM_POS_PARALLEL2:
    if (ndir==AXIS_NUM_POS_PARALLEL1) {
      px0=fx0;
    } else {
      px0=fx1;
    }
    py0=(fy0+fy1)/2;
    px1=cos(nndir)*px0+sin(nndir)*py0;
    py1=-sin(nndir)*px0+cos(nndir)*py0;
    break;
  case AXIS_NUM_POS_NORMAL1:
  case AXIS_NUM_POS_NORMAL2:
    if (side==AXIS_NUM_POS_LEFT) {
      if (ndir==AXIS_NUM_POS_NORMAL1) {
	px0=fx0;
      } else {
	px0=fx1;
      }
    } else {
      if (ndir==AXIS_NUM_POS_NORMAL1) {
	px0=fx1;
      } else {
	px0=fx0;
      }
    }
    py0=(fy0+fy1)/2;
    px1=cos(nndir)*px0+sin(nndir)*py0;
    py1=-sin(nndir)*px0+cos(nndir)*py0;
    break;
  case AXIS_NUM_POS_OBLIQUE1:
  case AXIS_NUM_POS_OBLIQUE2:
  default:
    px1 = py1 = 0;
    break;
  }
  GRAmoveto(GC,gx0-px1,gy0-py1);
  GRAdrawtext(GC,num,font->font,font->style,font->pt,font->space,ndirection,font->scriptsize);

  return 0;
}

static int
draw_numbering(struct objlist *obj, N_VALUE *inst, struct axislocal *alocal,
	       int GC, int side, int align, int ndir, int ilenmax, int plen,
	       struct axis_config *aconf, const struct font_config *font, int step,
	       int nnum, int numcount, int begin, int autonorm, int nozero,
	       int logpow, const char *format, double norm,
	       const char *head, int headlen, const char *tail, const char *date_format,
	       int hx0, int hy0, int hx1, int hy1, struct narray *array)
{
  int fx0,fy0,fx1,fy1,px0,px1,py0,py1;
  int dlx,dly,dlx2,dly2,maxlen;
  int rcode;
  int gx0,gy0;
  double nndir, po, min1, max1, value;
  int numlen,i;
  char *text, ch;
  int sx, sy, ndirection, cstep;

  _getobj(obj,"num_shift_p",inst,&sx);
  _getobj(obj,"num_shift_n",inst,&sy);

  switch (aconf->type) {
  case AXIS_TYPE_LOG:
    min1=log10(aconf->min);
    max1=log10(aconf->max);
    break;
  case AXIS_TYPE_INVERSE:
    min1=1/aconf->min;
    max1=1/aconf->max;
    break;
  default:
    min1=aconf->min;
    max1=aconf->max;
  }

  switch (ndir) {
  case AXIS_NUM_POS_HORIZONTAL:
    ndirection = 0;
    break;
  case AXIS_NUM_POS_NORMAL1:
    ndirection = aconf->direction + 9000;
    break;
  case AXIS_NUM_POS_NORMAL2:
    ndirection = aconf->direction + 27000;
    break;
  case AXIS_NUM_POS_OBLIQUE1:
    ndirection = aconf->direction + 4500;
    break;
  case AXIS_NUM_POS_OBLIQUE2:
    ndirection = aconf->direction + 31500;
    break;
  case AXIS_NUM_POS_PARALLEL1:
    ndirection = aconf->direction;
    break;
  case AXIS_NUM_POS_PARALLEL2:
    ndirection = aconf->direction + 18000;
    break;
  default:
    /* never reached */
    ndirection = 0;
  }
  if (ndirection > 36000) {
    ndirection -= 36000;
  }
  nndir = ndirection / 18000.0 * MPI;

  if (side==AXIS_NUM_POS_RIGHT) sy*=-1;

  switch (ndir) {
  case AXIS_NUM_POS_HORIZONTAL:
    get_num_ofst_horizontal(aconf, align, side, ilenmax, plen, hx0, hy0, hx1, hy1, &dlx, &dly, &dlx2, &dly2, &maxlen);
    break;
  case AXIS_NUM_POS_PARALLEL1:
  case AXIS_NUM_POS_PARALLEL2:
    get_num_ofst_parallel(aconf, side, hx0, hy0, hx1, hy1, &dlx, &dly, &dlx2, &dly2, &maxlen);
    break;
  case AXIS_NUM_POS_NORMAL1:
    get_num_ofst_normal1(aconf, align, side, ilenmax, plen, hx0, hy0, hx1, hy1, &dlx, &dly, &dlx2, &dly2, &maxlen);
    break;
  case AXIS_NUM_POS_NORMAL2:
    get_num_ofst_normal2(aconf, align, side, ilenmax, plen, hx0, hy0, hx1, hy1, &dlx, &dly, &dlx2, &dly2, &maxlen);
    break;
  case AXIS_NUM_POS_OBLIQUE1:
    get_num_ofst_oblique1(aconf, align, side, ilenmax, plen, hx0, hy0, hx1, hy1, &dlx, &dly, &dlx2, &dly2, &maxlen);
    break;
  case AXIS_NUM_POS_OBLIQUE2:
    get_num_ofst_oblique2(aconf, align, side, ilenmax, plen, hx0, hy0, hx1, hy1, &dlx, &dly, &dlx2, &dly2, &maxlen);
    break;
  default:
    /* never reached */
    dlx = 0;
    dly = 0;
    dlx2 = 0;
    dly2 = 0;
    maxlen = 0;
  }

  if (getaxispositionini(alocal,aconf->type,aconf->min,aconf->max,aconf->inc,aconf->div,FALSE)!=0)
    return 1;

  if (begin<=0)
    begin=1;

  cstep = step - begin + 1;
  numcount = 0;

  while ((rcode=getaxisposition(alocal,&po))!=-2) {
    if (rcode>=2) {
      gx0=aconf->x0+(po-min1)*aconf->length/(max1-min1)*cos(aconf->dir);
      gy0=aconf->y0-(po-min1)*aconf->length/(max1-min1)*sin(aconf->dir);
      gx0=gx0-sy*sin(aconf->dir)+sx*cos(aconf->dir)+dlx;
      gy0=gy0-sy*cos(aconf->dir)-sx*sin(aconf->dir)+dly;
      if ((cstep==step) || ((alocal->atype==AXISLOGSMALL) && (rcode==3))) {
	numcount++;
	if (((numcount<=nnum) || (nnum==-1)) && ((po!=0) || (nozero != AXIS_NUM_NO_ZERO_NO_ZERO))) {
	  value = numformat(&text, &numlen, format, aconf, alocal, logpow, po, norm, head, tail, date_format, nozero);
	  if (text == NULL) {
	    return 1;
	  }
	  if (align == AXIS_NUM_ALIGN_POINT) {
	    for (i = headlen; i < headlen + numlen; i++) {
	      if (text[i]=='.') {
		break;
	      }
	    }
	    ch=text[i];
	    text[i]='\0';
	    GRAtextextent(text,font->font, font->style, font->pt, font->space, font->scriptsize,
			  &fx0,&fy0,&fx1,&fy1,FALSE);
	    if (abs(fx1-fx0)>ilenmax) ilenmax=abs(fx1-fx0);
	    text[i]=ch;
	    GRAtextextent(text,font->font, font->style, font->pt, font->space, font->scriptsize,
			  &px0,&py0,&px1,&py1,FALSE);
	    if (py0<fy0) fy0=py0;
	    if (py1>fy1) fy1=py1;
	  } else {
	    GRAtextextent(text,font->font, font->style, font->pt, font->space, font->scriptsize,
			  &fx0,&fy0,&fx1,&fy1,FALSE);
	  }
	  switch (ndir) {
	  case AXIS_NUM_POS_HORIZONTAL:
	    get_num_pos_horizontal(align, plen, fx0, fy0, fx1, fy1, &px1, &py1);
	    break;
	  case AXIS_NUM_POS_PARALLEL1:
	  case AXIS_NUM_POS_PARALLEL2:
	    get_num_pos(align, plen, nndir, fx0, fy0, fx1, fy1, &px1, &py1);
	    break;
	  case AXIS_NUM_POS_NORMAL1:
	    get_num_pos(align, plen, nndir, fx0, fy0, fx1, fy1, &px1, &py1);
	    break;
	  case AXIS_NUM_POS_NORMAL2:
	    get_num_pos(align, plen, nndir, fx1, fy0, fx0, fy1, &px1, &py1);
	    break;
	  case AXIS_NUM_POS_OBLIQUE1:
	    get_num_pos_oblique(AXIS_NUM_ALIGN_LEFT, plen, nndir, fx1, fy0, fx0, fy1, &px1, &py1);
	    break;
	  case AXIS_NUM_POS_OBLIQUE2:
	    get_num_pos_oblique(AXIS_NUM_ALIGN_RIGHT, plen, nndir, fx1, fy0, fx0, fy1, &px1, &py1);
	    break;
	  default:
	    px1 = 0;
	    py1 = 0;
	  }
	  if (array == NULL) {
	    GRAmoveto(GC,gx0-px1,gy0-py1);
	    GRAdrawtext(GC,text,font->font,font->style,font->pt,font->space,ndirection,font->scriptsize);
	  } else {
	    char *s;
	    s = g_strdup_printf("%5d %5d %5d %.14E %.14E", gx0 - px1, gy0 - py1, ndirection, value, po);
	    arrayadd(array, &s);
	  }
	  g_free(text);
	}

	if ((alocal->atype==AXISLOGSMALL) && (rcode==3)) {
	  cstep=step-begin;
	} else {
	  cstep=0;
	}
      }
      cstep++;
    }
  }

  if (norm != 1 && array == NULL) {
    draw_numbering_normalize(GC, side, aconf, font, norm, maxlen, sx, sy, dlx2, dly2, ndir, nndir, ndirection);
  }

  return 0;
}

static int
get_step(struct axislocal *alocal, int step, int *begin)
{
  int n, count, rcode;
  double po;

  count = 0;
  while ((rcode = getaxisposition(alocal, &po)) != -2) {
    if (rcode >= 2) {
      count++;
    }
  }

  switch (alocal->atype) {
  case AXISINVERSE:
    if (step == 0) {
      if (count == 0) {
	n = 1;
      } else {
	n = nround(pow(10.0, (double) (int) (log10((double) count))));
      }

      if (n != 1) {
	n--;
      }

      if (count >= 18 * n) {
	step = 9 * n;
      } else if (count >= 6 * n) {
	step = 3 * n;
      } else {
	step = n;
      }
    }

    if (*begin == 0) {
      *begin = 1;
    }
    break;
  case AXISLOGSMALL:
    if (step == 0) {
      step = 1;
    }

    if (*begin == 0) {
      *begin = 1;
    }
    break;
  default:
    if (step == 0) {
      if (count == 0) {
	n = 1;
      } else {
	n = nround(pow(10.0,(double )(int )(log10(count * 0.5))));
      }
      if (count >= 10 * n) {
	step = 5 * n;
      } else if (count >= 5 * n) {
	step = 2 * n;
      } else {
	step = n;
      }
    }
    if (*begin == 0) {
      double min1, min2;

      min1 = roundmin(alocal->posst, alocal->dposl);
      min2 = roundmin(alocal->posst, alocal->dposm);
      *begin = nround(fabs((min1 - min2) / alocal->dposm)) % step + 1;
    }
  }

  return step;
}

static int
numbering(struct objlist *obj, N_VALUE *inst, int GC, struct axis_config *aconf, struct narray *array)
{
  int fr,fg,fb,fa;
  int side, begin,step,nnum,numcount,cstep;
  int autonorm,align,nozero;
  char *format,*head,*tail,*text,*date_format;
  int headlen,numlen;
  int logpow;
  double po;
  double norm;
  int rcode, i;
  int hx0,hy0,hx1,hy1,fx0,fy0,fx1,fy1,ndir;
  int ilenmax,flenmax,plen;
  struct font_config font;
  struct axislocal alocal;

  _getobj(obj, "num", inst, &side);
  if (side == AXIS_NUM_POS_NONE)
    return 0;

  _getobj(obj, "num_R", inst, &fr);
  _getobj(obj, "num_G", inst, &fg);
  _getobj(obj, "num_B", inst, &fb);
  _getobj(obj, "num_A", inst, &fa);
  _getobj(obj, "num_pt", inst, &font.pt);
  _getobj(obj, "num_space", inst, &font.space);
  _getobj(obj, "num_script_size", inst, &font.scriptsize);
  _getobj(obj, "num_begin", inst, &begin);
  _getobj(obj, "num_step", inst, &step);
  _getobj(obj, "num_num", inst, &nnum);
  _getobj(obj, "num_auto_norm", inst, &autonorm);
  _getobj(obj, "num_head", inst, &head);
  _getobj(obj, "num_format", inst, &format);
  _getobj(obj, "num_tail", inst, &tail);
  _getobj(obj, "num_date_format", inst, &date_format);
  _getobj(obj, "num_log_pow", inst, &logpow);
  _getobj(obj, "num_align", inst, &align);
  _getobj(obj, "num_no_zero", inst, &nozero);
  _getobj(obj, "num_font", inst, &font.font);
  _getobj(obj, "num_font_style", inst, &font.style);
  _getobj(obj, "num_direction",inst, &ndir);

  GRAcolor(GC, fr, fg, fb, fa);

  headlen = (head) ? strlen(head) : 0;

  if (getaxispositionini(&alocal, aconf->type, aconf->min, aconf->max, aconf->inc, aconf->div, FALSE)!=0) {
    error(obj, ERRMINMAX);
    return 1;
  }

  step = get_step(&alocal, step, &begin);

  norm = 1;
  if (alocal.atype == AXISNORMAL && aconf->code == NULL) {
    double abs_pos;
    abs_pos = fabs(alocal.dposm);
    if (abs_pos >= pow(10.0, (double) autonorm) ||
	abs_pos <= pow(10.0, (double) - autonorm)) {
      norm = abs_pos;
    }
  }

  if (aconf->type == AXIS_TYPE_MJD) {
    norm = 1;
    if (align == AXIS_NUM_ALIGN_POINT) {
      align = AXIS_NUM_ALIGN_CENTER;
    }
  }

  if (ndir == AXIS_NUM_POS_OBLIQUE1 || ndir == AXIS_NUM_POS_OBLIQUE2) {
    align = AXIS_NUM_ALIGN_CENTER;
  }

  if (getaxispositionini(&alocal, aconf->type, aconf->min, aconf->max, aconf->inc, aconf->div, FALSE)) {
    return 1;
  }

  GRAtextextent(".", font.font, font.style, font.pt, font.space, font.scriptsize,
		&fx0, &fy0, &fx1, &fy1, FALSE);
  plen = abs(fx1 - fx0);
  hx0 = hy0 = hx1 = hy1 = 0;
  flenmax = ilenmax = 0;
  if (begin <= 0) {
    begin = 1;
  }
  cstep = step - begin + 1;
  numcount = 0;
  while ((rcode = getaxisposition(&alocal, &po)) != -2) {
    if (rcode < 2) {
      continue;
    }

    if (cstep == step ||
	(alocal.atype == AXISLOGSMALL && rcode == 3)) {
      numcount++;

      if ((numcount <= nnum || nnum == -1) &&
	  (po != 0 || (nozero != AXIS_NUM_NO_ZERO_NO_ZERO))) {

	numformat(&text, &numlen, format, aconf, &alocal, logpow, po, norm, head, tail, date_format, nozero);
	if (text == NULL) {
	  return 1;
	}
	if (align == AXIS_NUM_ALIGN_POINT) {
	  for (i = headlen; i < headlen + numlen; i++) {
	    if (text[i] == '.') {
	      break;
	    }
	  }
	  if (text[i] == '.') {
	    GRAtextextent(text + i + 1, font.font, font.style, font.pt, font.space, font.scriptsize,
			  &fx0, &fy0, &fx1, &fy1, FALSE);
	    hy0 = MIN(hy0, fy0);
	    hy1 = MAX(hy1, fy1);
	    if (abs(fx1-fx0) > flenmax) {
	      flenmax = abs(fx1 - fx0);
	    }
	  }
	  text[i] = '\0';
	  GRAtextextent(text, font.font, font.style, font.pt, font.space, font.scriptsize,
			&fx0, &fy0, &fx1, &fy1, FALSE);
	  if (abs(fx1 - fx0) > ilenmax) {
	    ilenmax = abs(fx1 - fx0);
	  }
	  hy0 = MIN(hy0, fy0);
	  hy1 = MAX(hy1, fy1);
	} else {
	  GRAtextextent(text, font.font, font.style, font.pt, font.space, font.scriptsize,
			&fx0, &fy0, &fx1, &fy1, FALSE);
	  hx0 = MIN(hx0, fx0);
	  hx1 = MAX(hx1, fx1);
	  hy0 = MIN(hy0, fy0);
	  hy1 = MAX(hy1, fy1);
	}
	g_free(text);
      }
      if ((alocal.atype == AXISLOGSMALL) && (rcode == 3)) {
	cstep = step - begin;
      } else {
	cstep = 0;
      }
    }
    cstep++;
  }

  if (align == AXIS_NUM_ALIGN_POINT) {
    hx0 = 0;
    hx1 = flenmax + ilenmax + plen / 2;
  }

  if (hx0 != hx1 && hy0 != hy1) {
    draw_numbering(obj, inst, &alocal, GC,
		   side, align, ndir, ilenmax, plen, aconf, &font, step, nnum,
		   numcount, begin, autonorm, nozero, logpow, format,
		   norm, head, headlen, tail, date_format,
		   hx0, hy0, hx1, hy1, array);
  }

  return 0;
}

static int
draw_gauge(struct objlist *obj,N_VALUE *inst, int GC, struct axis_config *aconf)
{
  int fr,fg,fb,fa;
  struct narray *style;
  int snum,*sdata;
  struct axislocal alocal;
  double po,gmin,gmax,min1,max1,min2,max2;
  int len1,wid1,len2,wid2,len3,wid3,len,wid;
  int limit;
  int rcode;
  int gx0,gy0,gx1,gy1;
  int gauge;

  _getobj(obj,"gauge",inst,&gauge);
  if (gauge == AXIS_GAUGE_NONE)
    return 0;

  _getobj(obj,"gauge_R",inst,&fr);
  _getobj(obj,"gauge_G",inst,&fg);
  _getobj(obj,"gauge_B",inst,&fb);
  _getobj(obj,"gauge_A",inst,&fa);
  _getobj(obj,"gauge_min",inst,&gmin);
  _getobj(obj,"gauge_max",inst,&gmax);
  _getobj(obj,"gauge_style",inst,&style);
  _getobj(obj,"gauge_length1",inst,&len1);
  _getobj(obj,"gauge_width1",inst,&wid1);
  _getobj(obj,"gauge_length2",inst,&len2);
  _getobj(obj,"gauge_width2",inst,&wid2);
  _getobj(obj,"gauge_length3",inst,&len3);
  _getobj(obj,"gauge_width3",inst,&wid3);

  snum=arraynum(style);
  sdata=arraydata(style);

  GRAcolor(GC,fr,fg,fb, fa);

  if (getaxispositionini(&alocal,aconf->type,aconf->min,aconf->max,aconf->inc,aconf->div,FALSE)!=0) {
    error(obj,ERRMINMAX);
    return 1;
  }

  if ((gmin!=0) || (gmax!=0)) limit=TRUE;
  else limit=FALSE;

  switch (aconf->type) {
  case AXIS_TYPE_LOG:
    min1=log10(aconf->min);
    max1=log10(aconf->max);
    if (limit && (gmin>0) && (gmax>0)) {
      min2=log10(gmin);
      max2=log10(gmax);
    } else limit=FALSE;
    break;
  case AXIS_TYPE_INVERSE:
    min1=1/aconf->min;
    max1=1/aconf->max;
    if (limit && (gmin*gmax>0)) {
      min2=1/gmin;
      max2=1/gmax;
    } else limit=FALSE;
    break;
  default:
    min1=aconf->min;
    max1=aconf->max;
    if (limit) {
      min2=gmin;
      max2=gmax;
    }
  }

  while ((rcode=getaxisposition(&alocal,&po))!=-2) {
    if ((rcode>=0) && (!limit || ((min2-po)*(max2-po)<=0))) {
      gx0=aconf->x0+(po-min1)*aconf->length/(max1-min1)*cos(aconf->dir);
      gy0=aconf->y0-(po-min1)*aconf->length/(max1-min1)*sin(aconf->dir);
      if (rcode==1) {
	len=len1;
	wid=wid1;
      } else if (rcode==2) {
	len=len2;
	wid=wid2;
      } else {
	len=len3;
	wid=wid3;
      }
      GRAlinestyle(GC,snum,sdata,wid,GRA_LINE_CAP_BUTT,GRA_LINE_JOIN_MITER,1000);
      if ((gauge==1) || (gauge==2)) {
	gx1=gx0-len*sin(aconf->dir);
	gy1=gy0-len*cos(aconf->dir);
	GRAline(GC,gx0,gy0,gx1,gy1);
      }
      if ((gauge==1) || (gauge==3)) {
	gx1=gx0+len*sin(aconf->dir);
	gy1=gy0+len*cos(aconf->dir);
	GRAline(GC,gx0,gy0,gx1,gy1);
      }
    }
  }

  return 0;
}

static int
get_axis_parameter(struct objlist *obj, N_VALUE *inst, struct axis_config *aconf)
{
  _getobj(obj, "min",  inst, &aconf->min);
  _getobj(obj, "max",  inst, &aconf->max);
  _getobj(obj, "inc",  inst, &aconf->inc);
  _getobj(obj, "div",  inst, &aconf->div);
  _getobj(obj, "type", inst, &aconf->type);
  return 0;
}

static MathEquation *
get_axis_math(struct objlist *obj, char *math)
{
  MathEquation *code;
  int rcode, security;

  code = math_equation_basic_new();
  if (code == NULL) {
    return NULL;
  }
  security = get_security();
  math_equation_set_eoeq_assign_type(code, (security) ? EOEQ_ASSIGN_TYPE_BOTH : EOEQ_ASSIGN_TYPE_ASSIGN);

  if (math_equation_add_var(code, "X") != 0) {
    math_equation_free(code);
    return NULL;
  }

  rcode = math_equation_parse(code, math);
  if (rcode == 0 && code->use_eoeq_assign) {
    replace_eoeq_token(math);
  } else if (rcode) {
    char *err_msg;;

    err_msg = math_err_get_error_message(code, math, rcode);
    error2(obj, ERRNUMMATH, err_msg);
    g_free(err_msg);
    math_equation_free(code);
    return NULL;
  }

  return code;
}

static int
alloc_axis_math(struct objlist *obj, N_VALUE *inst, struct axis_config *aconf)
{
  char *math;

  aconf->code = NULL;
  _getobj(obj, "num_math", inst, &math);
  if (math == NULL || math[0] == '\0') {
    return 0;
  }
  aconf->code = get_axis_math(obj, math);
  return 0;
}

static int
free_axis_math(struct axis_config *aconf)
{
  if (aconf->code) {
    math_equation_free(aconf->code);
  }
  aconf->code = NULL;
  return 0;
}

static int
get_reference_parameter(struct objlist *obj, N_VALUE *inst,  struct axis_config *aconf, struct narray *ids)
{
  char *axis;
  struct objlist *aobj;
  int anum;
  struct narray iarray;
  int type, myid;

  _getobj(obj,"reference",inst, &axis);
  if (axis == NULL)
    return 1;

  _getobj(obj, "id", inst, &myid);
  arrayadd(ids, &myid);

  arrayinit(&iarray,sizeof(int));
  if (getobjilist(axis,&aobj,&iarray,FALSE,NULL))
    return 1;

  type = aconf->type;

  anum=arraynum(&iarray);
  if (anum>0) {
    N_VALUE *inst1;
    int id;
    id=arraylast_int(&iarray);
    arraydel(&iarray);
    if (array_find_int(ids, id) >= 0) {
      return 1;
    }
    inst1 = getobjinst(aobj,id);
    if (inst1) {
      get_axis_parameter(aobj, inst1, aconf);
      if (aconf->min == 0 && aconf->max == 0 && aconf->inc == 0) {
	get_reference_parameter(obj, inst1, aconf, ids);
      }
    }
  }

  if ((aconf->type == AXIS_TYPE_LINEAR ||
       aconf->type == AXIS_TYPE_MJD) &&
      (type == AXIS_TYPE_LINEAR ||
       type == AXIS_TYPE_MJD)) {
    aconf->type = type;
  }	/* AXIS_TYPE_LINEAR and AXIS_TYPE_MJD are compatible each other */

  return 0;
}

static int
draw_wave(struct objlist *obj, N_VALUE *inst, struct axis_config *aconf, int GC)
{
  int wave, wwidth, wlength, i;
  double wx[5],wxc1[5],wxc2[5],wxc3[5];
  double wy[5],wyc1[5],wyc2[5],wyc3[5];
  double ww[5],c[6];
  double dx, dy;

  _getobj(obj,"wave",inst,&wave);
  _getobj(obj,"wave_length",inst,&wlength);
  _getobj(obj,"wave_width",inst,&wwidth);

  for (i=0;i<5;i++) ww[i]=i;
  if ((wave==ARROW_POSITION_BEGIN) || (wave==ARROW_POSITION_BOTH)) {
    dx=cos(aconf->dir);
    dy=sin(aconf->dir);
    wx[0]=nround(aconf->x0-dy*wlength);
    wx[1]=nround(aconf->x0-dy*0.5*wlength-dx*0.25*wlength);
    wx[2]=aconf->x0;
    wx[3]=nround(aconf->x0+dy*0.5*wlength+dx*0.25*wlength);
    wx[4]=nround(aconf->x0+dy*wlength);
    if (spline(ww,wx,wxc1,wxc2,wxc3,5,SPLCND2NDDIF,SPLCND2NDDIF,0,0)) {
      error(obj,ERRAXISSPL);
      return 1;
    }
    wy[0]=nround(aconf->y0-dx*wlength);
    wy[1]=nround(aconf->y0-dx*0.5*wlength+dy*0.25*wlength);
    wy[2]=aconf->y0;
    wy[3]=nround(aconf->y0+dx*0.5*wlength-dy*0.25*wlength);
    wy[4]=nround(aconf->y0+dx*wlength);
    if (spline(ww,wy,wyc1,wyc2,wyc3,5,SPLCND2NDDIF,SPLCND2NDDIF,0,0)) {
      error(obj,ERRAXISSPL);
      return 1;
    }
    GRAlinestyle(GC,0,NULL,wwidth,GRA_LINE_CAP_BUTT,GRA_LINE_JOIN_MITER,1000);
    GRAcurvefirst(GC,0,NULL,NULL,NULL,splinedif,splineint,NULL,wx[0],wy[0]);
    for (i=0;i<4;i++) {
      c[0]=wxc1[i];
      c[1]=wxc2[i];
      c[2]=wxc3[i];
      c[3]=wyc1[i];
      c[4]=wyc2[i];
      c[5]=wyc3[i];
      if (!GRAcurve(GC,c,wx[i],wy[i])) break;
    }
  }

  if ((wave==ARROW_POSITION_END) || (wave==ARROW_POSITION_BOTH)) {
    dx=cos(aconf->dir);
    dy=sin(aconf->dir);
    wx[0]=nround(aconf->x1-dy*wlength);
    wx[1]=nround(aconf->x1-dy*0.5*wlength-dx*0.25*wlength);
    wx[2]=aconf->x1;
    wx[3]=nround(aconf->x1+dy*0.5*wlength+dx*0.25*wlength);
    wx[4]=nround(aconf->x1+dy*wlength);
    if (spline(ww,wx,wxc1,wxc2,wxc3,5,SPLCND2NDDIF,SPLCND2NDDIF,0,0)) {
      error(obj,ERRAXISSPL);
      return 1;
    }
    wy[0]=nround(aconf->y1-dx*wlength);
    wy[1]=nround(aconf->y1-dx*0.5*wlength+dy*0.25*wlength);
    wy[2]=aconf->y1;
    wy[3]=nround(aconf->y1+dx*0.5*wlength-dy*0.25*wlength);
    wy[4]=nround(aconf->y1+dx*wlength);
    if (spline(ww,wy,wyc1,wyc2,wyc3,5,SPLCND2NDDIF,SPLCND2NDDIF,0,0)) {
      error(obj,ERRAXISSPL);
      return 1;
    }
    GRAlinestyle(GC,0,NULL,wwidth,GRA_LINE_CAP_BUTT,GRA_LINE_JOIN_MITER,1000);
    GRAcurvefirst(GC,0,NULL,NULL,NULL,splinedif,splineint,NULL,wx[0],wy[0]);
    for (i=0;i<4;i++) {
      c[0]=wxc1[i];
      c[1]=wxc2[i];
      c[2]=wxc3[i];
      c[3]=wyc1[i];
      c[4]=wyc2[i];
      c[5]=wyc3[i];
      if (!GRAcurve(GC,c,wx[i],wy[i])) break;
    }
  }

  return 0;
}

static int
draw_arrow(struct objlist *obj, N_VALUE *inst, struct axis_config *aconf, int GC)
{
  int arrow,alength,awidth;
  double alen,awid,dx,dy;
  int ap[6];

  _getobj(obj,"arrow",inst,&arrow);
  _getobj(obj,"arrow_length",inst,&alength);
  _getobj(obj,"arrow_width",inst,&awidth);

  alen=aconf->width*(double )alength/10000;
  awid=aconf->width*(double )awidth/20000;

  if ((arrow==ARROW_POSITION_BEGIN) || (arrow==ARROW_POSITION_BOTH)) {
    dx=-cos(aconf->dir);
    dy=sin(aconf->dir);
    ap[0]=nround(aconf->x0-dy*awid);
    ap[1]=nround(aconf->y0+dx*awid);
    ap[2]=nround(aconf->x0+dx*alen);
    ap[3]=nround(aconf->y0+dy*alen);
    ap[4]=nround(aconf->x0+dy*awid);
    ap[5]=nround(aconf->y0-dx*awid);
    GRAlinestyle(GC,0,NULL,1,GRA_LINE_CAP_BUTT,GRA_LINE_JOIN_MITER,1000);
    GRAdrawpoly(GC,3,ap,GRA_FILL_MODE_EVEN_ODD);
  }

  if ((arrow==ARROW_POSITION_END) || (arrow==ARROW_POSITION_BOTH)) {
    dx=cos(aconf->dir);
    dy=-sin(aconf->dir);
    ap[0]=nround(aconf->x1-dy*awid);
    ap[1]=nround(aconf->y1+dx*awid);
    ap[2]=nround(aconf->x1+dx*alen);
    ap[3]=nround(aconf->y1+dy*alen);
    ap[4]=nround(aconf->x1+dy*awid);
    ap[5]=nround(aconf->y1-dx*awid);
    GRAlinestyle(GC,0,NULL,1,GRA_LINE_CAP_BUTT,GRA_LINE_JOIN_MITER,1000);
    GRAdrawpoly(GC,3,ap,GRA_FILL_MODE_EVEN_ODD);
  }

  return 0;
}

static int
axisdraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC;
  int fr, fg, fb, fa, w, h, bline;
  struct narray *style;
  int snum, *sdata;
  int clip, zoom;
  int hidden, hidden2;
  struct axis_config aconf;

  aconf.code = NULL;

  _getobj(obj,"hidden",inst,&hidden);
  hidden2=FALSE;
  _putobj(obj,"hidden",inst,&hidden2);
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv))
    return 1;
  _putobj(obj,"hidden",inst,&hidden);

  _getobj(obj,"GC",inst,&GC);
  if (GC<0)
    return 0;

  if (hidden)
    goto exit;

  _getobj(obj,"R",inst,&fr);
  _getobj(obj,"G",inst,&fg);
  _getobj(obj,"B",inst,&fb);
  _getobj(obj,"A",inst,&fa);
  _getobj(obj,"x",inst,&aconf.x0);
  _getobj(obj,"y",inst,&aconf.y0);
  _getobj(obj,"direction",inst,&aconf.direction);
  _getobj(obj,"baseline",inst,&bline);
  _getobj(obj,"length",inst,&aconf.length);
  _getobj(obj,"width",inst,&aconf.width);
  _getobj(obj,"style",inst,&style);
  _getobj(obj,"clip",inst,&clip);

  snum=arraynum(style);
  sdata=arraydata(style);

  aconf.dir=aconf.direction/18000.0*MPI;
  aconf.x1=aconf.x0+nround(aconf.length*cos(aconf.dir));
  aconf.y1=aconf.y0-nround(aconf.length*sin(aconf.dir));
  alloc_axis_math(obj, inst, &aconf);

  GRAregion(GC,&w,&h,&zoom);
  GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);
  GRAcolor(GC,fr,fg,fb, fa);

  if (bline) {
    GRAlinestyle(GC,snum,sdata,aconf.width,GRA_LINE_CAP_PROJECTING,GRA_LINE_JOIN_MITER,1000);
    GRAline(GC,aconf.x0,aconf.y0,aconf.x1,aconf.y1);
  }

  draw_arrow(obj, inst, &aconf, GC);

  if (draw_wave(obj, inst, &aconf, GC))
    goto exit;

  get_axis_parameter(obj, inst, &aconf);
  if (aconf.min == 0 && aconf.max == 0 && aconf.inc == 0) {
    struct narray *ids;
    ids = arraynew(sizeof(int));
    if (ids) {
      get_reference_parameter(obj, inst, &aconf, ids);
      arrayfree(ids);
    }
  }

  if (aconf.min != aconf.max && aconf.inc != 0 &&
      draw_gauge(obj, inst, GC, &aconf)) {
    goto exit;
  }

  if (aconf.code == NULL) {
    get_axis_parameter(obj, inst, &aconf);
  }
  if (aconf.min != aconf.max && aconf.inc != 0) {
    numbering(obj, inst, GC, &aconf, NULL);
  }

exit:
  free_axis_math(&aconf);
  return 0;
}

static int
axis_get_numbering(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int GC;
  int hidden;
  struct axis_config aconf;
  struct narray *array;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  arrayfree2(rval->array);
  rval->array = NULL;

  _getobj(obj, "hidden", inst, &hidden);
  if (hidden)
    return 0;

  _getobj(obj, "GC", inst, &GC);
  if (GC < 0)
    return 0;

  _getobj(obj, "x", inst, &aconf.x0);
  _getobj(obj, "y", inst, &aconf.y0);
  _getobj(obj, "direction", inst, &aconf.direction);
  _getobj(obj, "length", inst, &aconf.length);
  _getobj(obj, "width", inst, &aconf.width);

  aconf.dir = aconf.direction / 18000.0 * MPI;
  aconf.x1 = aconf.x0+nround(aconf.length * cos(aconf.dir));
  aconf.y1 = aconf.y0-nround(aconf.length * sin(aconf.dir));
  alloc_axis_math(obj, inst, &aconf);

  get_axis_parameter(obj, inst, &aconf);
  if (aconf.min != aconf.max && aconf.inc != 0) {
    array = arraynew(sizeof(char *));
    if (array == NULL) {
      free_axis_math(&aconf);
      return 1;
    }
    numbering(obj, inst, GC, &aconf, array);
    rval->array = array;
  }
  free_axis_math(&aconf);

  return 0;
}


static int
axisclear(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  double min,max,inc;

  min=max=inc=0;
  if (_putobj(obj,"min",inst,&min)) return 1;
  if (_putobj(obj,"max",inst,&max)) return 1;
  if (_putobj(obj,"inc",inst,&inc)) return 1;
  return 0;
}

static int
axisadjust(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *axis;
  int ad;
  struct objlist *aobj;
  int anum,id;
  struct narray iarray;
  N_VALUE *inst1;
  double min,max,inc,dir,po,dir1,x;
  int type,posx,posy,len,idir,posx1,posy1,div;
  struct axislocal alocal;
  int rcode;
  int first;
  int gx,gy,gx0,gy0,count;

  _getobj(obj,"x",inst,&posx1);
  _getobj(obj,"y",inst,&posy1);
  _getobj(obj,"direction",inst,&idir);
  _getobj(obj,"adjust_axis",inst,&axis);
  _getobj(obj,"adjust_position",inst,&ad);
  dir1=idir*MPI/18000;
  if (axis==NULL) return 0;
  arrayinit(&iarray,sizeof(int));
  if (getobjilist(axis,&aobj,&iarray,FALSE,NULL)) return 1;
  anum=arraynum(&iarray);
  if (anum<1) {
     arraydel(&iarray);
     return 1;
  }
  id=arraylast_int(&iarray);
  arraydel(&iarray);
  if ((inst1=getobjinst(aobj,id))==NULL) return 1;
  if (_getobj(aobj,"min",inst1,&min)) return 1;
  if (_getobj(aobj,"max",inst1,&max)) return 1;
  if (_getobj(aobj,"inc",inst1,&inc)) return 1;
  if (_getobj(aobj,"div",inst1,&div)) return 1;
  if (_getobj(aobj,"type",inst1,&type)) return 1;
  if (_getobj(aobj,"x",inst1,&posx)) return 1;
  if (_getobj(aobj,"y",inst1,&posy)) return 1;
  if (_getobj(aobj,"length",inst1,&len)) return 1;
  if (_getobj(aobj,"direction",inst1,&idir)) return 1;
  dir=idir*MPI/18000;
  if (min==max) return 0;
  if (dir==dir1) return 0;
  if (getaxispositionini(&alocal,type,min,max,inc,div,FALSE)!=0) return 0;
  if (type==AXIS_TYPE_LOG) {
    min=log10(min);
    max=log10(max);
  } else if (type==AXIS_TYPE_INVERSE) {
    min=1/min;
    max=1/max;
  }
  gx0 = gy0 = 0;	/* this initialization is added to avoid compile warnings. */
  first=TRUE;
  count=0;
  while ((rcode=getaxisposition(&alocal,&po))!=-2) {
    if (rcode>=2) {
      count++;
      gx=posx+(po-min)*len/(max-min)*cos(dir);
      gy=posy-(po-min)*len/(max-min)*sin(dir);
      if (first) {
        gx0=gx;
        gy0=gy;
        first=FALSE;
      }
      if (((ad==0) && (po==0)) || (count==ad)) {
        gx0=gx;
        gy0=gy;
      }
    }
  }
  if (first) return 0;
  x=-sin(dir1)*(gx0-posx1)-cos(dir1)*(gy0-posy1);
  x=x/(-cos(dir)*sin(dir1)+sin(dir)*cos(dir1));
  posx1=nround(posx1+x*cos(dir));
  posy1=nround(posy1-x*sin(dir));
  if (_putobj(obj,"x",inst,&posx1)) return 1;
  if (_putobj(obj,"y",inst,&posy1)) return 1;

  if (clear_bbox(obj, inst))
    return 1;

  return 0;
}

static int
axischangescale(struct objlist *obj,N_VALUE *inst,
                    double *rmin,double *rmax,double *rinc,int room)
{
  int type;
  double min,max,inc,ming,maxg,order;

  _getobj(obj,"type",inst,&type);
  ming=*rmin;
  maxg=*rmax;
  if (ming>maxg) {
    double a;
    a=ming;
    ming=maxg;
    maxg=a;
  }
  if (type==AXIS_TYPE_LOG) {
    if (ming<=0) return 1;
    if (maxg<=0) return 1;
    ming=log10(ming);
    maxg=log10(maxg);
  } else if (type==AXIS_TYPE_INVERSE) {
    if (ming*maxg<=0) return 1;
  }
  order=(fabs(ming)+fabs(maxg))*0.5;
  if (order==0) {
    maxg=1;
    ming=-1;
  } else if (fabs(maxg-ming)/order<1e-6) {
    maxg=maxg+order*0.5;
    ming=ming-order*0.5;
  }
  inc=scale(maxg-ming);
  if (room > 0) {
    max=maxg+(maxg-ming) * (room * 0.0001);
    max=nraise(max/inc*10)*inc/10;
    min=ming-(maxg-ming) * (room * 0.0001);
    min=cutdown(min/inc*10)*inc/10;
    if (type==AXIS_TYPE_LOG) {
      max=pow(10.0,max);
      min=pow(10.0,min);
      max=log10(nraise(max/scale(max))*scale(max));
      min=log10(cutdown(min/scale(min)+1e-15)*scale(min));
    } else if (type==AXIS_TYPE_INVERSE) {
      if (ming*min<=0) min=ming;
      if (maxg*max<=0) max=maxg;
    }
  } else {
    max=maxg;
    min=ming;
  }
  if (min==max) max=min+1;
  if (type!=AXIS_TYPE_INVERSE) {
    double mmin;
    inc=scale(max-min);
    if (max<min) inc*=-1;
    mmin=roundmin(min,inc)+inc;
    if ((mmin-min)*(mmin-max)>0) inc/=10;
  } else {
    inc=scale(max-min);
  }
  if ((type!=AXIS_TYPE_LOG) && (inc==0)) inc=1;
  if (type==AXIS_TYPE_LOG) inc=nround(inc);
  inc=fabs(inc);
  if (type==AXIS_TYPE_LOG) {
    min=pow(10.0,min);
    max=pow(10.0,max);
    inc=pow(10.0,inc);
  }
  *rmin=min;
  *rmax=max;
  *rinc=inc;
  return 0;
}

static int
axisscale(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int type,room;
  double min,max,inc;

  _getobj(obj,"type",inst,&type);
  min = arg_to_double(argv, 2);
  max = arg_to_double(argv, 3);
  room=*(int *)argv[4];
  axischangescale(obj,inst,&min,&max,&inc,room);
  if (_putobj(obj,"min",inst,&min)) return 1;
  if (_putobj(obj,"max",inst,&max)) return 1;
  if (_putobj(obj,"inc",inst,&inc)) return 1;
  return 0;
}

static int
axiscoordinate(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int x,y,dx,dy,type,dir,len;
  double min,max,c,t,val;

  _getobj(obj,"type",inst,&type);
  _getobj(obj,"min",inst,&min);
  _getobj(obj,"max",inst,&max);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"length",inst,&len);
  _getobj(obj,"direction",inst,&dir);
  if (min==max) return 1;
  if (len==0) return 1;
  dx=*(int *)argv[2];
  dy=*(int *)argv[3];
  c=dir/18000.0*MPI;
  t=-(x-dx)*cos(c)+(y-dy)*sin(c);
  if (type==AXIS_TYPE_LOG) {
    if (min<=0) return 1;
    if (max<=0) return 1;
    min=log10(min);
    max=log10(max);
  } else if (type==AXIS_TYPE_INVERSE) {
    if (min*max<=0) return 1;
    min=1.0/min;
    max=1.0/max;
  }
  val=min+(max-min)*t/len;
  if (type==AXIS_TYPE_LOG) {
    val=pow(10.0,val);
  } else if (type==AXIS_TYPE_INVERSE) {
    if (val==0) return 1;
    val=1.0/val;
  }
  rval->d=val;
  return 0;
}

static int
axisautoscalefile(struct objlist *obj,N_VALUE *inst,char *fileobj,double *rmin,double *rmax)
{
  struct objlist *fobj;
  int fnum;
  int *fdata;
  struct narray iarray;
  double min,max,min1,max1;
  int i,id,set;
  char buf[20], msgbuf[64], *group;
  char *argv2[4];
  struct narray *minmax;

  arrayinit(&iarray,sizeof(int));
  if (getobjilist(fileobj,&fobj,&iarray,FALSE,NULL)) return 1;
  fnum=arraynum(&iarray);
  fdata=arraydata(&iarray);
  _getobj(obj,"id",inst,&id);
  snprintf(buf, sizeof(buf), "axis:%d",id);
  _getobj(obj,"group",inst,&group);
  argv2[0]=(void *)buf;
  argv2[1]=NULL;
  min = max = 0;	/* this initialization is added to avoid compile warnings. */
  set=FALSE;
  for (i=0;i<fnum;i++) {
    double frac;
    frac = 1.0 * i / fnum;
    snprintf(msgbuf, sizeof(msgbuf), "%s (%.1f%%)", (group) ? group : buf, frac * 100);
    set_progress(1, msgbuf, frac);
    minmax=NULL;
    getobj(fobj,"bounding",fdata[i],1,argv2,&minmax);

    if (arraynum(minmax)>=2) {
      min1=arraynget_double(minmax,0);
      max1=arraynget_double(minmax,1);
      if (!set) {
        min=min1;
        max=max1;
	set=TRUE;
      } else {
        if (min1<min) min=min1;
        if (max1>max) max=max1;
      }
    }
  }
  arraydel(&iarray);
  if (!set) return 1;
  *rmin=min;
  *rmax=max;
  return 0;
}

static int
axisautoscale(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                  int argc,char **argv)
{
  char *fileobj;
  int room;
  double omin,omax,oinc;
  double min,max,inc;

  fileobj=(char *)argv[2];
  _getobj(obj,"auto_scale_margin",inst,&room);

  if (axisautoscalefile(obj,inst,fileobj,&min,&max))
    return 0;

  axischangescale(obj,inst,&min,&max,&inc,room);
  _getobj(obj,"min",inst,&omin);
  _getobj(obj,"max",inst,&omax);
  _getobj(obj,"inc",inst,&oinc);
  if (omin==omax) {
    if (_putobj(obj,"min",inst,&min)) return 1;
    if (_putobj(obj,"max",inst,&max)) return 1;
  }
  if (oinc==0) {
    if (_putobj(obj,"inc",inst,&inc)) return 1;
  }
  return 0;
}

static int
axisgetautoscale(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                  int argc,char **argv)
{
  char *fileobj;
  int room;
  double min,max,inc;
  struct narray *result;

  result=rval->array;
  arrayfree(result);
  rval->array=NULL;
  fileobj=(char *)argv[2];
  _getobj(obj,"auto_scale_margin",inst,&room);
  if (axisautoscalefile(obj,inst,fileobj,&min,&max)==0) {
    axischangescale(obj,inst,&min,&max,&inc,room);
    result=arraynew(sizeof(double));
    arrayadd(result,&min);
    arrayadd(result,&max);
    arrayadd(result,&inc);
    rval->array=result;
  }
  return 0;
}

static int
axisautoscale_margin(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
		     int argc,char **argv)
{
  int room;
  room = * (int *) argv[2];
  if (room < 0) {
    * (int *) argv[2] = 0;
  } else if (room > 100000) {
    * (int *) argv[2] = 100000;
  }

  return 0;
}

static int
axistight(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv)
{
  obj_do_tighten(obj, inst, "reference");
  obj_do_tighten(obj, inst, "adjust_axis");

  return 0;
}

static void
set_group(struct objlist *obj, int gnum, int id, char axis, char type)
{
  char *group, *group2;
  N_VALUE *inst2;

  inst2 = chkobjinst(obj, id);
  if (inst2 == NULL) {
    return;
  }

  group = g_strdup_printf("%c%c%d", type, axis, gnum);
  if (group) {
    _getobj(obj, "group", inst2, &group2);
    g_free(group2);
    _putobj(obj, "group", inst2, group);
  }

  group = g_strdup_printf("%c%c%d", type, axis, gnum);
  if (group) {
    _getobj(obj, "name", inst2, &group2);
    g_free(group2);
    _putobj(obj, "name", inst2, group);
  }
}

static int
axisgrouping(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                 int argc,char **argv)
{
  struct narray *iarray;
  int *data;
  int num,gnum;
  char type;

  iarray = (struct narray *)argv[2];
  num = arraynum(iarray);

  if (num < 1)
    return 1;

  data = arraydata(iarray);

  switch (data[0]) {
  case AXIS_TYPE_FRAME:
    type='f';
    break;
  case AXIS_TYPE_SECTION:
    type='s';
    break;
  case AXIS_TYPE_CROSS:
    type='c';
    break;
  default:
    error(obj, ERRGROUPING);
    return 1;
  }

  gnum = axisuniqgroup(obj, type);

  switch (data[0]) {
  case AXIS_TYPE_FRAME:
  case AXIS_TYPE_SECTION:
    if (num < 5)
      return 1;

    set_group(obj, gnum, data[1], 'X', type);
    set_group(obj, gnum, data[2], 'Y', type);
    set_group(obj, gnum, data[3], 'U', type);
    set_group(obj, gnum, data[4], 'R', type);
    break;
  case AXIS_TYPE_CROSS:
    if (num<3)
      return 1;

    set_group(obj, gnum, data[1], 'X', type);
    set_group(obj, gnum, data[2], 'Y', type);
  }
  return 0;
}

static void
set_group_pos(struct objlist *obj, int id, int x, int y, int len, int dir)
{
  N_VALUE *inst2;

  inst2 = chkobjinst(obj, id);
  if (inst2 == NULL)
    return;

  _putobj(obj, "direction", inst2, &dir);
  _putobj(obj, "x", inst2, &x);
  _putobj(obj, "y", inst2, &y);
  _putobj(obj, "length", inst2, &len);

  if (clear_bbox(obj, inst2))
    return;
}

static int
axisgrouppos(struct objlist *obj, N_VALUE *inst, N_VALUE *rval,
	     int argc, char **argv)
{
  int x, y, lx, ly;
  struct narray *iarray;
  int *data;
  int anum;

  iarray = (struct narray *)argv[2];

  anum = arraynum(iarray);

  if (anum < 1)
    return 1;

  data = arraydata(iarray);

  switch (data[0]) {
  case AXIS_TYPE_FRAME:
  case AXIS_TYPE_SECTION:
    if (anum < 9)
      return 1;

    x = data[5];
    y = data[6];
    lx = data[7];
    ly = data[8];

    set_group_pos(obj, data[1], x,      y,      lx,    0);
    set_group_pos(obj, data[2], x,      y,      ly, 9000);
    set_group_pos(obj, data[3], x,      y - ly, lx,    0);
    set_group_pos(obj, data[4], x + lx, y,      ly, 9000);

    break;
  case AXIS_TYPE_CROSS:
    if (anum < 7)
      return 1;

    x = data[3];
    y = data[4];
    lx = data[5];
    ly = data[6];

    set_group_pos(obj, data[1], x, y, lx, 0);
    set_group_pos(obj, data[2], x, y, ly, 9000);
    break;
  }
  return 0;
}

static void
axis_default(struct objlist *obj, int id, int *oid, int dir,
	     enum AXIS_GAUGE gauge, enum AXIS_NUM_POS num,
	     enum AXIS_NUM_ALIGN align, char *conf)
{
  N_VALUE *inst2;

  inst2 = chkobjinst(obj, id);
  if (inst2 == NULL)
    return;

  _putobj(obj, "gauge", inst2, &gauge);
  _putobj(obj, "num", inst2, &num);
  _putobj(obj, "num_align", inst2, &align);
  _putobj(obj, "direction", inst2, &dir);

  if (oid)
    _getobj(obj, "oid", inst2, oid);

  if (conf)
    axisloadconfig(obj, inst2, conf);
}

static void
axis_default_set(struct objlist *obj, int id, int oid, char *field, char *conf)
{
  N_VALUE *inst2;
  char *ref, *ref2;

  inst2 = chkobjinst(obj, id);
  if (inst2 == NULL)
    return;

  ref = g_strdup_printf("axis:^%d", oid);
  if (ref == NULL)
    return;

  _getobj(obj, field, inst2, &ref2);
  g_free(ref2);
  _putobj(obj, field, inst2, ref);

  axisloadconfig(obj, inst2, conf);
}


static void
axis_default_set_ref(struct objlist *obj, int id, int oid, char *conf)
{
  axis_default_set(obj, id, oid, "reference", conf);
}

static void
axis_default_set_adj(struct objlist *obj, int id, int oid, char *conf)
{
  axis_default_set(obj, id, oid, "adjust_axis", conf);
}

static int
axisdefgrouping(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                 int argc,char **argv)
{
  int oidx, oidy;
  struct narray *iarray;
  int *data;
  int anum;

  if (axisgrouping(obj, inst, rval, argc, argv))
    return 1;

  iarray = (struct narray *) argv[2];
  anum = arraynum(iarray);
  if (anum < 1)
    return 1;

  data = arraydata(iarray);

  switch (data[0]) {
  case AXIS_TYPE_FRAME:
    if (anum < 5)
      return 1;

    axis_default(obj, data[1], &oidx,    0, AXIS_GAUGE_LEFT,  AXIS_NUM_POS_RIGHT, AXIS_NUM_ALIGN_CENTER, "[axis_fX]");
    axis_default(obj, data[2], &oidy, 9000, AXIS_GAUGE_RIGHT, AXIS_NUM_POS_LEFT,  AXIS_NUM_ALIGN_RIGHT,  "[axis_fY]");
    axis_default(obj, data[3], NULL,     0, AXIS_GAUGE_RIGHT, AXIS_NUM_POS_LEFT,  AXIS_NUM_ALIGN_CENTER, NULL);
    axis_default(obj, data[4], NULL,  9000, AXIS_GAUGE_LEFT,  AXIS_NUM_POS_RIGHT, AXIS_NUM_ALIGN_LEFT,   NULL);

    axis_default_set_ref(obj, data[3], oidx, "[axis_fU]");
    axis_default_set_ref(obj, data[4], oidy, "[axis_fR]");

    if (anum < 9)
      return 0;

    break;
  case AXIS_TYPE_SECTION:
    if (anum < 5)
      return 1;

    axis_default(obj, data[1], &oidx,    0, AXIS_GAUGE_NONE, AXIS_NUM_POS_RIGHT, AXIS_NUM_ALIGN_CENTER, "[axis_sX]");
    axis_default(obj, data[2], &oidy, 9000, AXIS_GAUGE_NONE, AXIS_NUM_POS_LEFT,  AXIS_NUM_ALIGN_RIGHT,  "[axis_sY]");
    axis_default(obj, data[3], NULL,     0, AXIS_GAUGE_NONE, AXIS_NUM_POS_LEFT,  AXIS_NUM_ALIGN_CENTER, NULL);
    axis_default(obj, data[4], NULL,  9000, AXIS_GAUGE_NONE, AXIS_NUM_POS_RIGHT, AXIS_NUM_ALIGN_LEFT,   NULL);

    axis_default_set_ref(obj, data[3], oidx, "[axis_sU]");
    axis_default_set_ref(obj, data[4], oidy, "[axis_sR]");

    if (anum < 9)
      return 0;

    break;
  case AXIS_TYPE_CROSS:
    if (anum < 3)
      return 1;

    axis_default(obj, data[1], &oidx,    0, AXIS_GAUGE_BOTH, AXIS_NUM_POS_RIGHT, AXIS_NUM_ALIGN_CENTER, NULL);
    axis_default(obj, data[2], &oidy, 9000, AXIS_GAUGE_BOTH, AXIS_NUM_POS_LEFT,  AXIS_NUM_ALIGN_RIGHT, NULL);
    axis_default_set_adj(obj, data[1], oidy, "[axis_cX]");
    axis_default_set_adj(obj, data[2], oidx, "[axis_cY]");

    if (anum < 7)
      return 0;

    break;
  default:
    return 1;
  }

  if (axisgrouppos(obj, inst, rval, argc, argv))
    return 1;

  return 0;
}

static int
axis_save_group(struct objlist *obj, int type, N_VALUE **inst_array, N_VALUE *rval)
{
  char *str;
  int i, n, id;
  GString *s;

  switch (type) {
  case 'f':
    str = "\naxis::grouping 1 ";
    n = 4;
    break;
  case 's':
    str = "\naxis::grouping 2 ";
    n = 4;
    break;
  case 'c':
    str = "\naxis::grouping 3 ";
    n = 2;
    break;
  default:
    /* never reached */
    return 1;
  }

  s = g_string_sized_new(64);
  if (s == NULL)
    return 1;

  if (rval->str) {
    g_string_append(s, rval->str);
  }
  g_string_append(s, str);

  for (i = 0; i < n; i++) {
    _getobj(obj, "id", inst_array[i], &id);
    g_string_append_printf(s, "%d%c", id, (i == n - 1) ? '\n' : ' ');
  }

  g_free(rval->str);
  rval->str = g_string_free(s, FALSE);

  return 0;
}

static int
axissave(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int i, r, anum, type;
  struct narray *array;
  char **adata;
  N_VALUE *inst_array[INST_ARRAY_NUM];

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;

  array = (struct narray *) argv[2];
  anum = arraynum(array);
  adata = arraydata(array);
  for (i = 0; i < anum; i++) {
    if (strcmp("grouping", adata[i]) == 0)
      return 0;
  }

  type = get_axis_group_type(obj, inst, inst_array, FALSE);

  r = 0;
  switch (type) {
  case 'a':
    break;
  case 'f':
  case 's':
  case 'c':
    r = axis_save_group(obj, type, inst_array, rval);
    break;
  }
  return r;
}

int
axis_get_group(struct objlist *obj, N_VALUE *inst,  struct AxisGroupInfo *info)
{
  int i, lastinst, n;
  char *group, *group2;
  N_VALUE *inst2;
  char group3[20];

  if (_getobj(obj, "group", inst, &group)) {
    return 1;
  }

  if (group == NULL || group[0] == 'a') {
    int id;
    _getobj(obj, "id", inst, &id);
    info->type = 'a';
    info->num = 1;
    info->inst[0] = inst;
    info->id[0] = id;
    return 0;
  }

  lastinst = chkobjlastinst(obj);
  info->type = group[0];
  strncpy(group3, group, sizeof(group3) - 1);
  group3[sizeof(group3) - 1] = '\0';

  n = 0;
  for (i = lastinst; i >= 0; i--) {
    inst2 = chkobjinst(obj, i);
    _getobj(obj, "group", inst2, &group2);
    if (group2 &&
        group2[0] == info->type &&
        strcmp(group3 + 2, group2 + 2) == 0) {
      info->id[n] = i;
      info->inst[n] = inst2;
      n++;
      if (n >= AXIS_GROUPE_NUM_MAX) {
        break;
      }
    }
  }
  info->num = n;
  return 0;
}

static int
axismanager(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int i,id,lastinst;
  char *group,*group2;
  char type;

  _getobj(obj,"id",inst,&id);
  _getobj(obj,"group",inst,&group);
  if ((group==NULL) || (group[0]=='a')) {
    rval->i=id;
    return 0;
  }
  lastinst=chkobjlastinst(obj);
  id=-1;
  type=group[0];
  for (i=0;i<=lastinst;i++) {
    N_VALUE *inst2;
    inst2=chkobjinst(obj,i);
    _getobj(obj,"group",inst2,&group2);
    if ((group2!=NULL) && (group2[0]==type) && (strcmp(group+2,group2+2)==0))
      id=i;
  }
  rval->i=id;
  return 0;
}

static int
axisscalepush(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,
                  char **argv)
{
  struct narray *array;
  int num;
  double min,max,inc,*data;

  _getobj(obj,"min",inst,&min);
  _getobj(obj,"max",inst,&max);
  _getobj(obj,"inc",inst,&inc);
  if ((min==0) && (max==0) && (inc==0)) return 0;
  _getobj(obj,"scale_history",inst,&array);
  if (array==NULL) {
    if ((array=arraynew(sizeof(double)))==NULL) return 1;
    if (_putobj(obj,"scale_history",inst,array)) {
      arrayfree(array);
      return 1;
    }
  }
  num=arraynum(array);
  data=arraydata(array);
  if ((num>=3) && (data[0]==min) && (data[1]==max) && (data[2]==inc)) return 0;
  if (num > AXIS_HISTORY_NUM) {
    arrayndel(array, AXIS_HISTORY_NUM - 1);
    arrayndel(array, AXIS_HISTORY_NUM - 2);
    arrayndel(array, AXIS_HISTORY_NUM - 3);
  }
  arrayins(array,&inc,0);
  arrayins(array,&max,0);
  arrayins(array,&min,0);
  return 0;
}

static int
axisscalepop(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,
                  char **argv)
{
  struct narray *array;
  int num;
  double *data;

  _getobj(obj,"scale_history",inst,&array);
  if (array==NULL) return 0;
  num=arraynum(array);
  data=arraydata(array);
  if (num>=3) {
    _putobj(obj,"min",inst,&(data[0]));
    _putobj(obj,"max",inst,&(data[1]));
    _putobj(obj,"inc",inst,&(data[2]));
    arrayndel(array,2);
    arrayndel(array,1);
    arrayndel(array,0);
  }
  if (arraynum(array)==0) {
    arrayfree(array);
    _putobj(obj,"scale_history",inst,NULL);
  }
  return 0;
}

static int
anumnozeroput(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int type;

  type = * (int *) argv[2];
  switch (type) {
  case AXIS_NUM_NO_ZERO_REGULAR:
  case AXIS_NUM_NO_ZERO_NO_ZERO:
  case AXIS_NUM_NO_ZERO_NO_FLOATING_POINT:
    break;
  case AXIS_NUM_NO_ZERO_TRUE:
    * (int *) argv[2] = AXIS_NUM_NO_ZERO_NO_ZERO;
    break;
  case AXIS_NUM_NO_ZERO_FALSE:
    * (int *) argv[2] = AXIS_NUM_NO_ZERO_REGULAR;
    break;
  }

  return 0;
}

static int
anumdirput(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv)
{
  int type;

  type = * (int *) argv[2];
  switch (type) {
  case AXIS_NUM_POS_HORIZONTAL:
  case AXIS_NUM_POS_PARALLEL1:
  case AXIS_NUM_POS_PARALLEL2:
  case AXIS_NUM_POS_NORMAL1:
  case AXIS_NUM_POS_NORMAL2:
  case AXIS_NUM_POS_OBLIQUE1:
  case AXIS_NUM_POS_OBLIQUE2:
    break;
  case AXIS_NUM_POS_NORMAL:
    * (int *) argv[2] = AXIS_NUM_POS_HORIZONTAL;
    break;
  case AXIS_NUM_POS_PARALLEL:
    * (int *) argv[2] = AXIS_NUM_POS_PARALLEL1;
    break;
  default:
    * (int *) argv[2] = AXIS_NUM_POS_HORIZONTAL;
    break;
  }

  return 0;
}

static int
num_put_math(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  MathEquation *code;
  char *math;

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

  code = get_axis_math(obj, math);
  if (code) {
    math_equation_free(code);
    return 0;
  }

  return 1;
}

static int
put_gauge_hsb(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  return put_hsb_color(obj, inst, argc, argv, "gauge_%c");
}

static int
put_num_hsb(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  return put_hsb_color(obj, inst, argc, argv, "num_%c");
}

static struct objtable axis_obj[] = {
  {"init",NVFUNC,NEXEC,axisinit,NULL,0},
  {"done",NVFUNC,NEXEC,axisdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"group",NSTR,NREAD,NULL,NULL,0},
  {"min",NDOUBLE,NREAD|NWRITE,NULL,NULL,0},
  {"max",NDOUBLE,NREAD|NWRITE,NULL,NULL,0},
  {"inc",NDOUBLE,NREAD|NWRITE,NULL,NULL,0},
  {"div",NINT,NREAD|NWRITE,oputabs,NULL,0},
  {"type",NENUM,NREAD|NWRITE,NULL,axistypechar,0},
  {"x",NINT,NREAD|NWRITE,axisgeometry,NULL,0},
  {"y",NINT,NREAD|NWRITE,axisgeometry,NULL,0},
  {"direction",NINT,NREAD|NWRITE,axisdirection,NULL,0},
  {"baseline",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"length",NINT,NREAD|NWRITE,axisgeometry,NULL,0},
  {"width",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"style",NIARRAY,NREAD|NWRITE,NULL,NULL,0},
  {"auto_scale_margin",NINT,NREAD|NWRITE,axisautoscale_margin,NULL,0},
  {"adjust_axis",NOBJ,NREAD|NWRITE,NULL,NULL,0},
  {"adjust_position",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"arrow",NENUM,NREAD|NWRITE,NULL,arrowchar,0},
  {"arrow_length",NINT,NREAD|NWRITE,axisput,NULL,0},
  {"arrow_width",NINT,NREAD|NWRITE,axisput,NULL,0},
  {"wave",NENUM,NREAD|NWRITE,NULL,arrowchar,0},
  {"wave_length",NINT,NREAD|NWRITE,axisput,NULL,0},
  {"wave_width",NINT,NREAD|NWRITE,axisput,NULL,0},
  {"reference",NOBJ,NREAD|NWRITE,NULL,NULL,0},
  {"gauge",NENUM,NREAD|NWRITE,NULL,axisgaugechar,0},
  {"gauge_min",NDOUBLE,NREAD|NWRITE,NULL,NULL,0},
  {"gauge_max",NDOUBLE,NREAD|NWRITE,NULL,NULL,0},
  {"gauge_style",NIARRAY,NREAD|NWRITE,NULL,NULL,0},
  {"gauge_length1",NINT,NREAD|NWRITE,oputabs,NULL,0},
  {"gauge_width1",NINT,NREAD|NWRITE,oputabs,NULL,0},
  {"gauge_length2",NINT,NREAD|NWRITE,oputabs,NULL,0},
  {"gauge_width2",NINT,NREAD|NWRITE,oputabs,NULL,0},
  {"gauge_length3",NINT,NREAD|NWRITE,oputabs,NULL,0},
  {"gauge_width3",NINT,NREAD|NWRITE,oputabs,NULL,0},
  {"gauge_R",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"gauge_G",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"gauge_B",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"gauge_A",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"num",NENUM,NREAD|NWRITE,NULL,axisnumchar,0},
  {"num_begin",NINT,NREAD|NWRITE,oputabs,NULL,0},
  {"num_step",NINT,NREAD|NWRITE,oputabs,NULL,0},
  {"num_num",NINT,NREAD|NWRITE,axisput,NULL,0},
  {"num_auto_norm",NINT,NREAD|NWRITE,oputabs,NULL,0},
  {"num_head",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"num_format",NSTR,NREAD|NWRITE,axisput,NULL,0},
  {"num_tail",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"num_log_pow",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"num_pt",NINT,NREAD|NWRITE,axisput,NULL,0},
  {"num_space",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"num_font",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"num_font_style",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"num_script_size",NINT,NREAD|NWRITE,axisput,NULL,0},
  {"num_align",NENUM,NREAD|NWRITE,NULL,anumalignchar,0},
  {"num_no_zero",NENUM,NREAD|NWRITE,anumnozeroput,anumnozero,0},
  {"num_direction",NENUM,NREAD|NWRITE,anumdirput,anumdirchar,0},
  {"num_shift_p",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"num_shift_n",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"num_R",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"num_G",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"num_B",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"num_A",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"num_date_format",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"num_math",NSTR,NREAD|NWRITE,num_put_math,NULL,0},
  {"scale_push",NVFUNC,NREAD|NEXEC,axisscalepush,"",0},
  {"scale_pop",NVFUNC,NREAD|NEXEC,axisscalepop,"",0},
  {"scale_history",NDARRAY,NREAD,NULL,NULL,0},
  {"scale",NVFUNC,NREAD|NEXEC,axisscale,"ddi",0},
  {"auto_scale",NVFUNC,NREAD|NEXEC,axisautoscale,"o",0},
  {"get_auto_scale",NDAFUNC,NREAD|NEXEC,axisgetautoscale,"o",0},
  {"clear",NVFUNC,NREAD|NEXEC,axisclear,"",0},
  {"adjust",NVFUNC,NREAD|NEXEC,axisadjust,"",0},
  {"draw",NVFUNC,NREAD|NEXEC,axisdraw,"i",0},
  {"bbox",NIAFUNC,NREAD|NEXEC,axisbbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,axismove,"ii",0},
  {"rotate",NVFUNC,NREAD|NEXEC,axisrotate,"iiii",0},
  {"flip",NVFUNC,NREAD|NEXEC,axisflip,"iii",0},
  {"change",NVFUNC,NREAD|NEXEC,axischange,"iii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,axiszoom,"iiiii",0},
  {"match",NBFUNC,NREAD|NEXEC,axismatch,"iiiii",0},
  {"coordinate",NDFUNC,NREAD|NEXEC,axiscoordinate,"ii",0},
  {"tight",NVFUNC,NREAD|NEXEC,axistight,"",0},
  {"grouping",NVFUNC,NREAD|NEXEC,axisgrouping,"ia",0},
  {"default_grouping",NVFUNC,NREAD|NEXEC,axisdefgrouping,"ia",0},
  {"group_position",NVFUNC,NREAD|NEXEC,axisgrouppos,"ia",0},
  {"group_manager",NIFUNC,NREAD|NEXEC,axismanager,"",0},
  {"get_numbering",NSAFUNC,NREAD|NEXEC,axis_get_numbering,"",0},
  {"save",NSFUNC,NREAD|NEXEC,axissave,"sa",0},

  {"hsb", NVFUNC, NREAD|NEXEC, put_hsb,"ddd",0},
  {"gauge_hsb", NVFUNC, NREAD|NEXEC, put_gauge_hsb,"ddd",0},
  {"num_hsb", NVFUNC, NREAD|NEXEC, put_num_hsb,"ddd",0},

  /* following fields exist for backward compatibility */
  {"num_jfont",NSTR,NWRITE,NULL,NULL,0},
};

#define TBLNUM (sizeof(axis_obj) / sizeof(*axis_obj))

void *
addaxis(void)
/* addaxis() returns NULL on error */
{

  if (AxisConfigHash == NULL) {
    unsigned int i;
    AxisConfigHash = nhash_new();
    if (AxisConfigHash == NULL)
      return NULL;

    for (i = 0; i < sizeof(AxisConfig) / sizeof(*AxisConfig); i++) {
      if (nhash_set_ptr(AxisConfigHash, AxisConfig[i].name, (void *) &AxisConfig[i])) {
	nhash_free(AxisConfigHash);
	return NULL;
      }
    }
  }

  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, axis_obj, ERRNUM, axiserrorlist, NULL, NULL);
}

/* 
 * $Id: oaxis.c,v 1.27 2009/03/09 10:21:48 hito Exp $
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
#include <stdarg.h>
#include <math.h>
#include <string.h>
#include <ctype.h>

#include "common.h"

#include "ngraph.h"
#include "nhash.h"
#include "ioutil.h"
#include "object.h"
#include "mathfn.h"
#include "spline.h"
#include "gra.h"
#include "oroot.h"
#include "odraw.h"
#include "olegend.h"
#include "axis.h"
#include "nstring.h"
#include "nconfig.h"

#define NAME "axis"
#define PARENT "draw"
#define OVERSION  "1.00.00"
#define TRUE  1
#define FALSE 0

#define ERRAXISTYPE 100
#define ERRAXISHEAD 101
#define ERRAXISGAUGE 102
#define ERRAXISSPL 103
#define ERRMINMAX 104
#define ERRFORMAT 105
#define ERRGROUPING 106

static char *axiserrorlist[]={
  "illegal axis type.",
  "illegal arrow/wave type.",
  "illegal gauge type.",
  "error: spline interpolation.",
  "illegal value of min/max/inc.",
  "illegal format.",
  "illegal grouping type.",
};

#define ERRNUM (sizeof(axiserrorlist) / sizeof(*axiserrorlist))

enum AXIS_TYPE {
  AXIS_TYPE_SINGLE,
  AXIS_TYPE_FRAME,
  AXIS_TYPE_SECTION,
  AXIS_TYPE_CROSS,
};

char *axistypechar[4]={
  N_("linear"),
  N_("log"),
  N_("inverse"),
  NULL
};

char *axisgaugechar[]={
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

char *axisnumchar[]={
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

char *anumalignchar[]={
  N_("center"),
  N_("left"),
  N_("right"),
  N_("point"),
  NULL
};

enum AXIS_NUM_ALIGN {
  AXIS_NUM_ALIGN_CENTER,
  AXIS_NUM_ALIGN_LEFT,
  AXIS_NUM_ALIGN_RIGHT,
  AXIS_NUM_ALIGN_POINT,
};

char *anumdirchar[]={
  N_("normal"),
  N_("parallel"),
  N_("parallel2"),
  NULL
};

enum AXIS_NUM_DIR {
  AXIS_NUM_POS_NORMAL,
  AXIS_NUM_POS_PARALLEL1,
  AXIS_NUM_POS_PARALLEL2,
};

static struct obj_config AxisConfig[] = {
  {"R", OBJ_CONFIG_TYPE_NUMERIC},
  {"G", OBJ_CONFIG_TYPE_NUMERIC},
  {"B", OBJ_CONFIG_TYPE_NUMERIC},
  {"type", OBJ_CONFIG_TYPE_NUMERIC},
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

  {"num_head", OBJ_CONFIG_TYPE_STRING},
  {"num_format", OBJ_CONFIG_TYPE_STRING},
  {"num_tail", OBJ_CONFIG_TYPE_STRING},
  {"num_font", OBJ_CONFIG_TYPE_STRING},
  {"num_jfont", OBJ_CONFIG_TYPE_STRING},

  {"style", OBJ_CONFIG_TYPE_STYLE},
  {"gauge_style", OBJ_CONFIG_TYPE_STYLE},
};

static NHASH AxisConfigHash = NULL;

static int 
axisuniqgroup(struct objlist *obj,char type)
{
  int num;
  char *inst,*group,*endptr;
  int nextp;

  nextp=obj->nextp;
  num=0;
  do {
    num++;
    inst=obj->root;
    while (inst!=NULL) {
      _getobj(obj,"group",inst,&group);
      if ((group!=NULL) && (group[0]==type)) {
        if (num==strtol(group+2,&endptr,10)) break;
      }
      inst=*(char **)(inst+nextp);
    }
    if (inst==NULL) {
      inst=obj->root2;
      while (inst!=NULL) {
        _getobj(obj,"group",inst,&group);
        if ((group!=NULL) && (group[0]==type)) {
          if (num==strtol(group+2,&endptr,10)) break;
        }
        inst=*(char **)(inst+nextp);
      }
    }
  } while (inst!=NULL);
  return num;
}

static int 
axisloadconfig(struct objlist *obj,char *inst,char *conf)
{
  return obj_load_config(obj, inst, conf, AxisConfigHash);
}

static int 
axisinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int width;
  int alen,awid,wlen,wwid;
  int bline;
  int len1,wid1,len2,wid2,len3,wid3;
  int pt,sx,sy,logpow,scriptsize;
  int autonorm,num,gnum;
  char *font,*jfont,*format,*group,*name, buf[256];

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  width=40;
  alen=72426;
  awid=60000;
  wlen=300;
  wwid=40;
  len1=100;
  wid1=40;
  len2=200;
  wid2=40;
  len3=300;
  wid3=40;
  bline=TRUE;
  pt=2000;
  sx=0;
  sy=100;
  autonorm=5;
  logpow=TRUE;
  scriptsize=7000;
  num=-1;
  if (_putobj(obj,"baseline",inst,&bline)) return 1;
  if (_putobj(obj,"width",inst,&width)) return 1;
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
  if (_putobj(obj,"num_pt",inst,&pt)) return 1;
  if (_putobj(obj,"num_script_size",inst,&scriptsize)) return 1;
  if (_putobj(obj,"num_auto_norm",inst,&autonorm)) return 1;
  if (_putobj(obj,"num_shift_p",inst,&sx)) return 1;
  if (_putobj(obj,"num_shift_n",inst,&sy)) return 1;
  if (_putobj(obj,"num_log_pow",inst,&logpow)) return 1;
  if (_putobj(obj,"num_num",inst,&num)) return 1;

  format = font = jfont = group = name = NULL;

  format = nstrdup("%g");
  if (format == NULL) goto errexit;
  if (_putobj(obj,"num_format",inst,format)) goto errexit;

  font = nstrdup(fontchar[4]);
  if (font == NULL) goto errexit;
  if (_putobj(obj,"num_font",inst,font)) goto errexit;

  jfont = nstrdup(jfontchar[1]);
  if (jfont == NULL) goto errexit;
  if (_putobj(obj,"num_jfont",inst,jfont)) goto errexit;

  gnum = axisuniqgroup(obj,'a');
  snprintf(buf, sizeof(buf), "a_%d",gnum);
  group = nstrdup(buf);
  if (group == NULL) goto errexit;
  if (_putobj(obj,"group",inst,group)) goto errexit;

  snprintf(buf, sizeof(buf), "a_%d",gnum);
  name = nstrdup(buf);
  if (name == NULL) goto errexit;
  if (_putobj(obj,"name",inst,name)) goto errexit;

  axisloadconfig(obj,inst,"[axis]");
  return 0;

errexit:
  memfree(format);
  memfree(font);
  memfree(jfont);
  memfree(group);
  memfree(name);
  return 1;
}

static int 
axisdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
axisput(struct objlist *obj,char *inst,char *rval,
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
axisgeometry(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;

  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static int 
axisdirection(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int dir;

  dir = * (int *) argv[2];

  dir %= 36000;
  if (dir < 0)
    dir += 36000;

  * (int *) argv[2] = dir;

  return axisgeometry(obj, inst, rval, argc, argv);
}


static int 
axisbbox2(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy;
  int x0,y0,x1,y1,length,direction;
  double dir;
  struct narray *array;

  array=*(struct narray **)rval;
  if (arraynum(array)!=0) return 0;
  _getobj(obj,"x",inst,&x0);
  _getobj(obj,"y",inst,&y0);
  _getobj(obj,"length",inst,&length);
  _getobj(obj,"direction",inst,&direction);
  if ((array==NULL) && ((array=arraynew(sizeof(int)))==NULL)) return 1;
  dir=direction/18000.0*MPI;
  x1=x0+nround(length*cos(dir));
  y1=y0-nround(length*sin(dir));
  maxx=minx=x0;
  maxy=miny=y0;
  arrayadd(array,&x0);
  arrayadd(array,&y0);
  arrayadd(array,&x1);
  arrayadd(array,&y1);
  if (x1<minx) minx=x1;
  if (x1>maxx) maxx=x1;
  if (y1<miny) miny=y1;
  if (y1>maxy) maxy=y1;
  arrayins(array,&(maxy),0);
  arrayins(array,&(maxx),0);
  arrayins(array,&(miny),0);
  arrayins(array,&(minx),0);
  if (arraynum(array)==0) {
    arrayfree(array);
    return 1;
  }
  *(struct narray **)rval=array;
  return 0;
}

static int
check_direction(struct objlist *obj, int type, char **inst_array)
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
get_axis_group_type(struct objlist *obj, char *inst, char **inst_array)
{
  char *group, *group2, *inst2;
  char type;
  int find_axis, len, id, i;

  _getobj(obj, "group", inst, &group);

  if (group  ==  NULL || group[0]  ==  'a')
    return 'a';

  len = strlen(group);
  if (len < 3)
    return 'a';

  _getobj(obj, "id", inst, &id);

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
  }

  switch (type) {
  case 'f':
  case 's':
    if (! CHECK_FRAME(find_axis)) {
      type = -1;
    }
    break;
  case 'c':
    if (! CHECK_CROSS(find_axis)) {
      type = -1;
    }
    break;
  default:
    type = -1;
  }

  return type;
}

static void
get_axis_box(struct objlist *obj, char *inst, int argc, char **argv, int *minx, int *miny, int *maxx, int *maxy)
{
  struct narray *rval2;

  rval2 = NULL;
  axisbbox2(obj, inst, (void *) &rval2, argc, argv);
  *minx = * (int *) arraynget(rval2, 0);
  *miny = * (int *) arraynget(rval2, 1);
  *maxx = * (int *) arraynget(rval2, 2);
  *maxy = * (int *) arraynget(rval2, 3);
  arrayfree(rval2);
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
axisbbox(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i, type, dir;
  char *inst_array[4];
  struct narray *array;
  int minx,miny,maxx,maxy;
  int x0,y0,x1,y1;

  array = * (struct narray **) rval;
  if (arraynum(array) != 0)
    return 0;

  type = get_axis_group_type(obj, inst, inst_array);

  switch (type) {
  case 'a':
    return axisbbox2(obj, inst, rval, argc, argv);
    break;
  case 'f':
  case 's':
    get_axis_box(obj, inst_array[0], argc, argv, &minx, &miny, &maxx, &maxy);

    for (i = 1; i < 4; i++) {
      get_axis_box(obj, inst_array[i], argc, argv, &x0, &y0, &x1, &y1);
      if (x0 < minx) minx = x0;
      if (y0 < miny) miny = y0;
      if (x1 > maxx) maxx = x1;
      if (y1 > maxy) maxy = y1;
    }

    dir = check_direction(obj, type, inst_array);
    array = set_axis_box(array, minx, miny, maxx, maxy, ! dir);
    if (array == NULL) {
      * (struct narray **) rval = NULL;
      return 1;
    }

    * (struct narray **) rval = array;
    break;
  case 'c':
    get_axis_box(obj, inst_array[0], argc, argv, &minx, &miny, &maxx, &maxy);
    get_axis_box(obj, inst_array[1], argc, argv, &x0, &y0, &x1, &y1);

    if (x0 < minx) minx = x0;
    if (y0 < miny) miny = y0;
    if (x1 > maxx) maxx = x1;
    if (y1 > maxy) maxy = y1;

    dir = check_direction(obj, type, inst_array);
    array = set_axis_box(array, minx, miny, maxx, maxy, ! dir);
    if (array == NULL) {
      * (struct narray **) rval = NULL;
      return 1;
    }

    * (struct narray **) rval = array;
    break;
  }

  return 0;
}

static int 
axismatch2(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy,err;
  int bminx,bminy,bmaxx,bmaxy;
  int i,num,*data;
  double x1,y1,x2,y2;
  double r,r2,r3,ip;
  struct narray *array;

  *(int *)rval=FALSE;
  array=NULL;
  axisbbox2(obj,inst,(void *)&array,argc,argv);
  if (array==NULL) return 0;
  minx=*(int *)argv[2];
  miny=*(int *)argv[3];
  maxx=*(int *)argv[4];
  maxy=*(int *)argv[5];
  err=*(int *)argv[6];
  if ((minx==maxx) && (miny==maxy)) {
    num=arraynum(array)-4;
    data=arraydata(array);
    for (i=0;i<num-2;i+=2) {
      x1=data[4+i];
      y1=data[5+i];
      x2=data[6+i];
      y2=data[7+i];
      r2=sqrt((x2-x1)*(x2-x1)+(y2-y1)*(y2-y1));
      r=sqrt((minx-x1)*(minx-x1)+(miny-y1)*(miny-y1));
      r3=sqrt((minx-x2)*(minx-x2)+(miny-y2)*(miny-y2));
      if ((r<=err) || (r3<err)) {
        *(int *)rval=TRUE;
        break;
      }
      if (r2!=0) {
        ip=((x2-x1)*(minx-x1)+(y2-y1)*(miny-y1))/r2;
        if ((0<=ip) && (ip<=r2)) {
          x2=x1+(x2-x1)*ip/r2;
          y2=y1+(y2-y1)*ip/r2;
          r=sqrt((minx-x2)*(minx-x2)+(miny-y2)*(miny-y2));
          if (r<err) {
            *(int *)rval=TRUE;
            break;
          }
        }
      }
    }
  } else {
    if (arraynum(array)<4) {
      arrayfree(array);
      return 1;
    }
    bminx=*(int *)arraynget(array,0);
    bminy=*(int *)arraynget(array,1);
    bmaxx=*(int *)arraynget(array,2);
    bmaxy=*(int *)arraynget(array,3);
    if ((minx<=bminx) && (bminx<=maxx)
     && (minx<=bmaxx) && (bmaxx<=maxx)
     && (miny<=bminy) && (bminy<=maxy)
     && (miny<=bmaxy) && (bmaxy<=maxy)) *(int *)rval=TRUE;
  }
  arrayfree(array);
  return 0;
}

static int 
axismatch(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int i, n, type;
  char *inst_array[4];
  int rval2, match;

  *(int *)rval = FALSE;

  type = get_axis_group_type(obj, inst, inst_array);

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
    match = FALSE;

    for (i = 0; i < n; i++) {
      axismatch2(obj, inst_array[i], (void *)&rval2, argc, argv);
      match = match || rval2;
    }

    * (int *) rval = match;
  }

  return 0;
}

static int 
axismove2(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int x,y;
  struct narray *array;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  x+=*(int *)argv[2];
  y+=*(int *)argv[3];
  if (_putobj(obj,"x",inst,&x)) return 1;
  if (_putobj(obj,"y",inst,&y)) return 1;
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static int 
axismove(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i, type;
  char *inst_array[4];

  type = get_axis_group_type(obj, inst, inst_array);

  switch (type) {
  case 'a':
    return axismove2(obj, inst, rval, argc, argv);
    break;
  case 'f':
  case 's':
    for (i = 0; i < 4; i++) {
      axismove2(obj, inst_array[i], rval, argc, argv);
    }
    break;
  case 'c':
    axismove2(obj, inst_array[0], rval, argc, argv);
    axismove2(obj, inst_array[1], rval, argc, argv);
    break;
  }
  return 0;
}

static int 
axischange2(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int len,dir,x,y;
  double x2,y2;
  int point,x0,y0;
  struct narray *array;

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
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static void
axis_change_point0(struct objlist *obj, int type, char **inst_array, int x0, int y0)
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
axis_change_point1(struct objlist *obj, int type, char **inst_array, int x0, int y0)
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
axis_change_point2(struct objlist *obj, int type, char **inst_array, int x0, int y0)
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
axis_change_point3(struct objlist *obj, int type, char **inst_array, int x0, int y0)
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
axischange(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *inst_array[4];
  int type, point, x0, y0, len;
  struct narray *array;

  type = get_axis_group_type(obj, inst, inst_array);

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

    _getobj(obj, "bbox", inst, &array);
    arrayfree(array);

    if (_putobj(obj, "bbox", inst, NULL))
      return 1;

    break;
  }
  return 0;
}

static int 
axiszoom2(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int x,y,len,refx,refy,preserve_width;
  double zoom;
  struct narray *array;
  int pt,space,wid1,wid2,wid3,len1,len2,len3,wid,wlen,wwid;
  struct narray *style,*gstyle;
  int i,snum,*sdata,gsnum,*gsdata;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  zoom=(*(int *)argv[2])/10000.0;
  refx=(*(int *)argv[3]);
  refy=(*(int *)argv[4]);
  preserve_width = (*(int *)argv[5]);
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
  snum=arraynum(style);
  sdata=arraydata(style);
  gsnum=arraynum(gstyle);
  gsdata=arraydata(gstyle);
  wlen*=zoom;
  len1*=zoom;
  len2*=zoom;
  len3*=zoom;
  x=(x-refx)*zoom+refx;
  y=(y-refy)*zoom+refy;
  len*=zoom;
  pt*=zoom;
  space*=zoom;
  if (! preserve_width) {
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
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static int 
axiszoom(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i, type;
  char *inst_array[4];

  type = get_axis_group_type(obj, inst, inst_array);

  switch (type) {
  case 'a':
    return axiszoom2(obj, inst, rval, argc, argv);
    break;
  case 'f':
  case 's':
    for (i = 0; i < 4; i++) {
      axiszoom2(obj, inst_array[i], rval, argc, argv);
    }
    break;
  case 'c':
    axiszoom2(obj, inst_array[0], rval, argc, argv);
    axiszoom2(obj, inst_array[1], rval, argc, argv);
    break;
  }
  return 0;
}

static void 
numformat(char *num,char *format,double a)
{
  int i,j,len,ret;
  char *s;
  char format2[256];

  s=strchr(format,'+');
  if ((a==0) && (s!=NULL)) {
    len=strlen(format);
    for (i=j=0;i<=len;i++) {
      format2[j]=format[i];
      if (format[i]!='+') j++;
    }
    ret=sprintf(num,"\\xb1");
    ret+=sprintf(num+ret,format2,a);
  } else {
    ret=sprintf(num,format,a);
  }
}

static int 
axisdraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int GC;
  int fr,fg,fb,lm,tm,w,h;
  int arrow,alength,awidth;
  int wave,wlength,wwidth;
  int x0,y0,x1,y1,direction,length,width;
  int bline;
  double min,max,inc;
  struct narray *style;
  int snum,*sdata;
  int div,type;
  double alen,awid,dx,dy,dir;
  int ap[6];
  double wx[5],wxc1[5],wxc2[5],wxc3[5];
  double wy[5],wyc1[5],wyc2[5],wyc3[5];
  double ww[5],c[6];
  int i;
  int clip,zoom;
  char *axis;
  struct axislocal alocal;
  double po,gmin,gmax,min1,max1,min2,max2;
  int gauge,len1,wid1,len2,wid2,len3,wid3,len,wid;
  struct narray iarray;
  struct objlist *aobj;
  int anum,id;
  char *inst1;
  int limit;
  int rcode;
  int gx0,gy0,gx1,gy1;
  int logpow,scriptsize;
  int side,pt,space,begin,step,nnum,numcount,cstep,ndir;
  int autonorm,align,sx,sy,nozero;
  char *format,*head,*tail,*font,*jfont,num[256],*text;
  int headlen,taillen,numlen;
  int dlx,dly,dlx2,dly2,maxlen,ndirection;
  int n,count;
  double norm;
  double t,nndir;
  int hx0,hy0,hx1,hy1,fx0,fy0,fx1,fy1,px0,px1,py0,py1;
  int ilenmax,flenmax,plen;
  char ch;
  int hidden,hidden2;

  _getobj(obj,"hidden",inst,&hidden);
  hidden2=FALSE;
  _putobj(obj,"hidden",inst,&hidden2);
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _putobj(obj,"hidden",inst,&hidden);
  _getobj(obj,"GC",inst,&GC);
  if (GC<0) return 0;
  if (hidden) goto exit;
  _getobj(obj,"R",inst,&fr);
  _getobj(obj,"G",inst,&fg);
  _getobj(obj,"B",inst,&fb);
  _getobj(obj,"x",inst,&x0);
  _getobj(obj,"y",inst,&y0);
  _getobj(obj,"direction",inst,&direction);
  _getobj(obj,"baseline",inst,&bline);
  _getobj(obj,"length",inst,&length);
  _getobj(obj,"width",inst,&width);
  _getobj(obj,"style",inst,&style);
  _getobj(obj,"arrow",inst,&arrow);
  _getobj(obj,"arrow_length",inst,&alength);
  _getobj(obj,"arrow_width",inst,&awidth);
  _getobj(obj,"wave",inst,&wave);
  _getobj(obj,"wave_length",inst,&wlength);
  _getobj(obj,"wave_width",inst,&wwidth);
  _getobj(obj,"clip",inst,&clip);
  snum=arraynum(style);
  sdata=arraydata(style);

  dir=direction/18000.0*MPI;
  x1=x0+nround(length*cos(dir));
  y1=y0-nround(length*sin(dir));

  GRAregion(GC,&lm,&tm,&w,&h,&zoom);
  GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);
  GRAcolor(GC,fr,fg,fb);

  if (bline) {
    GRAlinestyle(GC,snum,sdata,width,2,0,1000);
    GRAline(GC,x0,y0,x1,y1);
  }

  alen=width*(double )alength/10000;
  awid=width*(double )awidth/20000;
  if ((arrow==2) || (arrow==3)) {
    dx=-cos(dir);
    dy=sin(dir);
    ap[0]=nround(x0-dy*awid);
    ap[1]=nround(y0+dx*awid);
    ap[2]=nround(x0+dx*alen);
    ap[3]=nround(y0+dy*alen);
    ap[4]=nround(x0+dy*awid);
    ap[5]=nround(y0-dx*awid);
    GRAlinestyle(GC,0,NULL,1,0,0,1000);
    GRAdrawpoly(GC,3,ap,1);
  }
  if ((arrow==1) || (arrow==3)) {
    dx=cos(dir);
    dy=-sin(dir);
    ap[0]=nround(x1-dy*awid);
    ap[1]=nround(y1+dx*awid);
    ap[2]=nround(x1+dx*alen);
    ap[3]=nround(y1+dy*alen);
    ap[4]=nround(x1+dy*awid);
    ap[5]=nround(y1-dx*awid);
    GRAlinestyle(GC,0,NULL,1,0,0,1000);
    GRAdrawpoly(GC,3,ap,1);
  }
  for (i=0;i<5;i++) ww[i]=i;
  if ((wave==2) || (wave==3)) {
    dx=cos(dir);
    dy=sin(dir);
    wx[0]=nround(x0-dy*wlength);
    wx[1]=nround(x0-dy*0.5*wlength-dx*0.25*wlength);
    wx[2]=x0;
    wx[3]=nround(x0+dy*0.5*wlength+dx*0.25*wlength);
    wx[4]=nround(x0+dy*wlength);
    if (spline(ww,wx,wxc1,wxc2,wxc3,5,SPLCND2NDDIF,SPLCND2NDDIF,0,0)) {
      error(obj,ERRAXISSPL);
      goto exit;
    }
    wy[0]=nround(y0-dx*wlength);
    wy[1]=nround(y0-dx*0.5*wlength+dy*0.25*wlength);
    wy[2]=y0;
    wy[3]=nround(y0+dx*0.5*wlength-dy*0.25*wlength);
    wy[4]=nround(y0+dx*wlength);
    if (spline(ww,wy,wyc1,wyc2,wyc3,5,SPLCND2NDDIF,SPLCND2NDDIF,0,0)) {
      error(obj,ERRAXISSPL);
      goto exit;
    }
    GRAlinestyle(GC,0,NULL,wwidth,0,0,1000);
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
  if ((wave==1) || (wave==3)) {
    dx=cos(dir);
    dy=sin(dir);
    wx[0]=nround(x1-dy*wlength);
    wx[1]=nround(x1-dy*0.5*wlength-dx*0.25*wlength);
    wx[2]=x1;
    wx[3]=nround(x1+dy*0.5*wlength+dx*0.25*wlength);
    wx[4]=nround(x1+dy*wlength);
    if (spline(ww,wx,wxc1,wxc2,wxc3,5,SPLCND2NDDIF,SPLCND2NDDIF,0,0)) {
      error(obj,ERRAXISSPL);
      goto exit;
    }
    wy[0]=nround(y1-dx*wlength);
    wy[1]=nround(y1-dx*0.5*wlength+dy*0.25*wlength);
    wy[2]=y1;
    wy[3]=nround(y1+dx*0.5*wlength-dy*0.25*wlength);
    wy[4]=nround(y1+dx*wlength);
    if (spline(ww,wy,wyc1,wyc2,wyc3,5,SPLCND2NDDIF,SPLCND2NDDIF,0,0)) {
      error(obj,ERRAXISSPL);
      goto exit;
    }
    GRAlinestyle(GC,0,NULL,wwidth,0,0,1000);
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

  _getobj(obj,"min",inst,&min);
  _getobj(obj,"max",inst,&max);
  _getobj(obj,"inc",inst,&inc);
  _getobj(obj,"div",inst,&div);
  _getobj(obj,"type",inst,&type);

  if ((min==0) && (max==0) && (inc==0)) {
    _getobj(obj,"reference",inst,&axis);
    if (axis!=NULL) {
      arrayinit(&iarray,sizeof(int));
      if (getobjilist(axis,&aobj,&iarray,FALSE,NULL)) goto numbering;
      anum=arraynum(&iarray);
      if (anum>0) {
        id=*(int *)arraylast(&iarray);
        arraydel(&iarray);
        if ((anum>0) && ((inst1=getobjinst(aobj,id))!=NULL)) {
           _getobj(aobj,"min",inst1,&min);
           _getobj(aobj,"max",inst1,&max);
           _getobj(aobj,"inc",inst1,&inc);
           _getobj(aobj,"div",inst1,&div);
           _getobj(aobj,"type",inst1,&type);
        }
      }
    }
  }

  if ((min==max) || (inc==0)) goto numbering;

  dir=direction/18000.0*MPI;

  _getobj(obj,"gauge",inst,&gauge);

  if (gauge!=0) {
    _getobj(obj,"gauge_R",inst,&fr);
    _getobj(obj,"gauge_G",inst,&fg);
    _getobj(obj,"gauge_B",inst,&fb);
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

    GRAcolor(GC,fr,fg,fb);

    if (getaxispositionini(&alocal,type,min,max,inc,div,FALSE)!=0) {
      error(obj,ERRMINMAX);
      goto exit;
    }

    if ((gmin!=0) || (gmax!=0)) limit=TRUE;
    else limit=FALSE;

    if (type==1) {
      min1=log10(min);
      max1=log10(max);
      if (limit && (gmin>0) && (gmax>0)) {
        min2=log10(gmin);
        max2=log10(gmax);
      } else limit=FALSE;
    } else if (type==2) {
      min1=1/min;
      max1=1/max;
      if (limit && (gmin*gmax>0)) {
        min2=1/gmin;
        max2=1/gmax;
      } else limit=FALSE;
    } else {
      min1=min;
      max1=max;
      if (limit) {
        min2=gmin;
        max2=gmax;
      }
    }

    while ((rcode=getaxisposition(&alocal,&po))!=-2) {
      if ((rcode>=0) && (!limit || ((min2-po)*(max2-po)<=0))) {
        gx0=x0+(po-min1)*length/(max1-min1)*cos(dir);
        gy0=y0-(po-min1)*length/(max1-min1)*sin(dir);
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
        GRAlinestyle(GC,snum,sdata,wid,0,0,1000);
        if ((gauge==1) || (gauge==2)) {
          gx1=gx0-len*sin(dir);
          gy1=gy0-len*cos(dir);
          GRAline(GC,gx0,gy0,gx1,gy1);
        }
        if ((gauge==1) || (gauge==3)) {
          gx1=gx0+len*sin(dir);
          gy1=gy0+len*cos(dir);
          GRAline(GC,gx0,gy0,gx1,gy1);
        }
      }
    }
  }

numbering:

  _getobj(obj,"min",inst,&min);
  _getobj(obj,"max",inst,&max);
  _getobj(obj,"inc",inst,&inc);
  _getobj(obj,"div",inst,&div);
  _getobj(obj,"type",inst,&type);

  if ((min==max) || (inc==0)) goto exit;

  _getobj(obj,"num",inst,&side);

  if (side!=0) {
    _getobj(obj,"num_R",inst,&fr);
    _getobj(obj,"num_G",inst,&fg);
    _getobj(obj,"num_B",inst,&fb);
    _getobj(obj,"num_pt",inst,&pt);
    _getobj(obj,"num_space",inst,&space);
    _getobj(obj,"num_script_size",inst,&scriptsize);
    _getobj(obj,"num_begin",inst,&begin);
    _getobj(obj,"num_step",inst,&step);
    _getobj(obj,"num_num",inst,&nnum);
    _getobj(obj,"num_auto_norm",inst,&autonorm);
    _getobj(obj,"num_head",inst,&head);
    _getobj(obj,"num_format",inst,&format);
    _getobj(obj,"num_tail",inst,&tail);
    _getobj(obj,"num_log_pow",inst,&logpow);
    _getobj(obj,"num_align",inst,&align);
    _getobj(obj,"num_shift_p",inst,&sx);
    _getobj(obj,"num_shift_n",inst,&sy);
    _getobj(obj,"num_direction",inst,&ndir);
    _getobj(obj,"num_no_zero",inst,&nozero);
    _getobj(obj,"num_font",inst,&font);
    _getobj(obj,"num_jfont",inst,&jfont);

    GRAcolor(GC,fr,fg,fb);

    if (side==2) sy*=-1;

    if (head!=NULL) headlen=strlen(head);
    else headlen=0;
    if (tail!=NULL) taillen=strlen(tail);
    else taillen=0;

    switch (ndir) {
    case 0:
      ndirection = 0;
      break;
    case 1:
      ndirection = direction;
      break;
    case 2:
      ndirection = direction + 18000;
      if (ndirection > 36000)
	ndirection -= 36000;
      break;
    default:
      /* never reached */
      ndirection = 0;
    }
    nndir=ndirection/18000.0*MPI;

    if (type==1) {
      min1=log10(min);
      max1=log10(max);
    } else if (type==2) {
      min1=1/min;
      max1=1/max;
    } else {
      min1=min;
      max1=max;
    }

    if (getaxispositionini(&alocal,type,min,max,inc,div,FALSE)!=0) {
      error(obj,ERRMINMAX);
      goto exit;
    }

    count=0;
    while ((rcode=getaxisposition(&alocal,&po))!=-2) {
      if (rcode>=2) count++;
    }

    if (alocal.atype==AXISINVERSE) {
      if (step==0) {
        if (count==0) n=1;
        else n=nround(pow(10.0,(double )(int )(log10((double )count))));
        if (n!=1) n--;
        if (count>=18*n) step=9*n;
        else if (count>=6*n) step=3*n;
        else step=n;
      }
      if (begin==0) begin=1;
    } else if (alocal.atype==AXISLOGSMALL) {
      if (step==0) step=1;
      if (begin==0) begin=1;
    } else {
      if (step==0) {
        if (count==0) n=1;
        else n=nround(pow(10.0,(double )(int )(log10(count*0.5))));
        if (count>=10*n) step=5*n;
        else if (count>=5*n) step=2*n;
        else step=n;
      }
      if (begin==0)
        begin=nround(fabs((roundmin(alocal.posst,alocal.dposl)
                        -roundmin(alocal.posst,alocal.dposm))/alocal.dposm))
             % step+1;
    }

    norm=1;
    if (alocal.atype==AXISNORMAL) {
      if ((fabs(alocal.dposm)>=pow(10.0,(double )autonorm))
       || (fabs(alocal.dposm)<=pow(10.0,(double )-autonorm)))
        norm=fabs(alocal.dposm);
    }

    if (getaxispositionini(&alocal,type,min,max,inc,div,FALSE)!=0)
      goto exit;
    GRAtextextent(".",font,jfont,pt,space,scriptsize,&fx0,&fy0,&fx1,&fy1,FALSE);
    plen=abs(fx1-fx0);
    hx0=hy0=hx1=hy1=0;
    flenmax=ilenmax=0;
    if (begin<=0) begin=1;
    cstep=step-begin+1;
    numcount=0;
    while ((rcode=getaxisposition(&alocal,&po))!=-2) {
      if (rcode>=2) {
        if ((cstep==step) || ((alocal.atype==AXISLOGSMALL) && (rcode==3))) {
          numcount++;
          if (((numcount<=nnum) || (nnum==-1)) && ((po!=0) || !nozero)) {
            if ((!logpow
            && ((alocal.atype==AXISLOGBIG) || (alocal.atype==AXISLOGNORM)))
            || (alocal.atype==AXISLOGSMALL))
              numformat(num,format,pow(10.0,po));
            else if (alocal.atype==AXISINVERSE)
              numformat(num,format,1.0/po);
            else
              numformat(num,format,po/norm);
            numlen=strlen(num);
            if ((text=memalloc(numlen+headlen+taillen+5))==NULL) goto exit;
            text[0]='\0';
            if (headlen!=0) strcpy(text,head);
            if (logpow
            && ((alocal.atype==AXISLOGBIG) || (alocal.atype==AXISLOGNORM)))
              strcat(text,"10^");
            if (numlen!=0) strcat(text,num);
            if (logpow
            && ((alocal.atype==AXISLOGBIG) || (alocal.atype==AXISLOGNORM)))
              strcat(text,"@");
            if (taillen!=0) strcat(text,tail);
            if (align==3) {
              for (i=headlen;i<headlen+numlen;i++) if (text[i]=='.') break;
              if (text[i]=='.') {
                GRAtextextent(text+i+1,font,jfont,pt,space,scriptsize,
                              &fx0,&fy0,&fx1,&fy1,FALSE);
                if (fy0<hy0) hy0=fy0;
                if (fy1>hy1) hy1=fy1;
                if (abs(fx1-fx0)>flenmax) flenmax=abs(fx1-fx0);
              }
              text[i]='\0';
              GRAtextextent(text,font,jfont,pt,space,scriptsize,
                                &fx0,&fy0,&fx1,&fy1,FALSE);
              if (abs(fx1-fx0)>ilenmax) ilenmax=abs(fx1-fx0);
              if (fy0<hy0) hy0=fy0;
              if (fy1>hy1) hy1=fy1;
            } else {
              GRAtextextent(text,font,jfont,pt,space,scriptsize,
                            &fx0,&fy0,&fx1,&fy1,FALSE);
              if (fx0<hx0) hx0=fx0;
              if (fx1>hx1) hx1=fx1;
              if (fy0<hy0) hy0=fy0;
              if (fy1>hy1) hy1=fy1;
            }
            memfree(text);
          }
          if ((alocal.atype==AXISLOGSMALL) && (rcode==3)) cstep=step-begin;
          else cstep=0;
	}
        cstep++;
      }
    }
    if (align==3) {
      hx0=0;
      hx1=flenmax+ilenmax+plen/2;
    }

    if ((abs(hx0-hx1)!=0) && (abs(hy0-hy1)!=0)) {

      if (ndir==0) {
        if (side==1) {
          if (direction<9000) {
            px0=hx1;
            py0=hy1;
          } else if (direction<18000) {
            px0=hx1;
            py0=hy0;
          } else if (direction<27000) {
            px0=hx0;
            py0=hy0;
          } else {
            px0=hx0;
            py0=hy1;
          }
        } else {
          if (direction<9000) {
            px0=hx0;
            py0=hy0;
          } else if (direction<18000) {
            px0=hx0;
            py0=hy1;
          } else if (direction<27000) {
            px0=hx1;
            py0=hy1;
          } else {
            px0=hx1;
            py0=hy0;
          }
        }
        if (align==0) px1=(hx0+hx1)/2;
        else if (align==1) px1=hx0;
        else if (align==2) px1=hx1;
        else if (align==3) px1=ilenmax+plen/2;
        py1=(hy0+hy1)/2;
        fx0=px1-px0;
        fy0=py1-py0;
        t=cos(dir)*fx0-sin(dir)*fy0;
        dlx=fx0-t*cos(dir);
        dly=fy0+t*sin(dir);
        if (side==1) {
          if (direction<9000) px1=hx1;
          else if (direction<18000) px1=hx1;
          else if (direction<27000) px1=hx0;
          else px1=hx0;
        } else {
          if (direction<9000) px1=hx0;
          else if (direction<18000) px1=hx0;
          else if (direction<27000) px1=hx1;
          else px1=hx1;
        }
        py1=(hy0+hy1)/2;
        fx0=px1-px0;
        fy0=py1-py0;
        t=cos(dir)*fx0-sin(dir)*fy0;
        dlx2=fx0-t*cos(dir);
        dly2=fy0+t*sin(dir);
        maxlen=abs(hx1-hx0);
      } else if ((ndir==1) || (ndir==2)) {
        py1=(hy0+hy1)/2;
        if (side==1) py1*=-1;
        dlx=-py1*sin(dir);
        dly=-py1*cos(dir);
        dlx2=-py1*sin(dir);
        dly2=-py1*cos(dir);
        maxlen=abs(hx1-hx0);
      } else {
	/* never reached */
        dlx = 0;
        dly = 0;
        dlx2 = 0;
        dly2 = 0;
        maxlen = 0;
      }
      if (getaxispositionini(&alocal,type,min,max,inc,div,FALSE)!=0)
        goto exit;
      if (begin<=0) begin=1;
      cstep=step-begin+1;
      numcount=0;
      while ((rcode=getaxisposition(&alocal,&po))!=-2) {
        if (rcode>=2) {
          gx0=x0+(po-min1)*length/(max1-min1)*cos(dir);
          gy0=y0-(po-min1)*length/(max1-min1)*sin(dir);
          gx0=gx0-sy*sin(dir)+sx*cos(dir)+dlx;
          gy0=gy0-sy*cos(dir)-sx*sin(dir)+dly;
          if ((cstep==step) || ((alocal.atype==AXISLOGSMALL) && (rcode==3))) {
            numcount++;
            if (((numcount<=nnum) || (nnum==-1)) && ((po!=0) || !nozero)) {
              if ((!logpow
              && ((alocal.atype==AXISLOGBIG) || (alocal.atype==AXISLOGNORM)))
              || (alocal.atype==AXISLOGSMALL))
                numformat(num,format,pow(10.0,po));
              else if (alocal.atype==AXISINVERSE)
                numformat(num,format,1.0/po);
              else
                numformat(num,format,po/norm);
              numlen=strlen(num);
              if ((text=memalloc(numlen+headlen+taillen+5))==NULL)
                goto exit;
              text[0]='\0';
              if (headlen!=0) strcpy(text,head);
              if (logpow
              && ((alocal.atype==AXISLOGBIG) || (alocal.atype==AXISLOGNORM)))
                strcat(text,"10^");
              if (numlen!=0) strcat(text,num);
              if (logpow
              && ((alocal.atype==AXISLOGBIG) || (alocal.atype==AXISLOGNORM)))
                strcat(text,"@");
              if (taillen!=0) strcat(text,tail);
              if (align==3) {
                for (i=headlen;i<headlen+numlen;i++) if (text[i]=='.') break;
                ch=text[i];
                text[i]='\0';
                GRAtextextent(text,font,jfont,pt,space,scriptsize,
                              &fx0,&fy0,&fx1,&fy1,FALSE);
                if (abs(fx1-fx0)>ilenmax) ilenmax=abs(fx1-fx0);
                text[i]=ch;
                GRAtextextent(text,font,jfont,pt,space,scriptsize,
                              &px0,&py0,&px1,&py1,FALSE);
                if (py0<fy0) fy0=py0;
                if (py1>fy1) fy1=py1;
              } else {
                GRAtextextent(text,font,jfont,pt,space,scriptsize,
                              &fx0,&fy0,&fx1,&fy1,FALSE);
              }
              if (ndir==0) {
                if (align==0) px1=(fx0+fx1)/2;
				else if (align==1) px1=fx0;
                else if (align==2) px1=fx1;
                else if (align==3) px1=fx1+plen/2;
                py1=(fy0+fy1)/2;
              } else if ((ndir==1) || (ndir==2)) {
                if (align==0) px0=(fx0+fx1)/2;
                else if (align==1) px0=fx0;
				else if (align==2) px0=fx1;
                else if (align==3) px0=fx1+plen/2;
                py0=(fy0+fy1)/2;
                px1=cos(nndir)*px0+sin(nndir)*py0;
				py1=-sin(nndir)*px0+cos(nndir)*py0;
              }
              GRAmoveto(GC,gx0-px1,gy0-py1);
              GRAdrawtext(GC,text,font,jfont,pt,space,ndirection,scriptsize);
			  memfree(text);
            }
            if ((alocal.atype==AXISLOGSMALL) && (rcode==3)) cstep=step-begin;
            else cstep=0;
          }
          cstep++;
        }
      }

      if (norm!=1) {
        if (norm/pow(10.0,cutdown(log10(norm)))==1) {
	  //          sprintf(num,"[%%F{Symbol}%c%%F{%s}10^%+d@]", (char )0xb4,font,(int )cutdown(log10(norm)));
          sprintf(num,"[\\xd710^%+d@]", (int )cutdown(log10(norm)));
        } else {
	  //          sprintf(num,"[%g%%F{Symbol}%c%%F{%s}10^%+d@]", norm/pow(10.0,cutdown(log10(norm))), (char )0xb4,font,(int )cutdown(log10(norm)));
          sprintf(num,"[%g\\xd710^%+d@]", norm/pow(10.0,cutdown(log10(norm))), (int )cutdown(log10(norm)));
        }
        GRAtextextent(num,font,jfont,pt,space,scriptsize,
                      &fx0,&fy0,&fx1,&fy1,FALSE);
        if (abs(fy1-fy0)>maxlen) maxlen=abs(fy1-fy0);
        gx0=x0+(length+maxlen*1.2)*cos(dir);
        gy0=y0-(length+maxlen*1.2)*sin(dir);
        gx0=gx0-sy*sin(dir)+sx*cos(dir)+dlx2;
        gy0=gy0-sy*cos(dir)-sx*sin(dir)+dly2;
        if (ndir==0) {
          if (side==1) {
            if ((direction>4500) && (direction<=22500)) px1=fx1;
            else px1=fx0;
          } else {
            if ((direction>13500) && (direction<=31500)) px1=fx1;
            else px1=fx0;
          }
          py1=(fy0+fy1)/2;
        } else {
          if (ndir==1) px0=fx0;
          else px0=fx1;
          py0=(fy0+fy1)/2;
          px1=cos(nndir)*px0+sin(nndir)*py0;
          py1=-sin(nndir)*px0+cos(nndir)*py0;
        }
        GRAmoveto(GC,gx0-px1,gy0-py1);
        GRAdrawtext(GC,num,font,jfont,pt,space,ndirection,scriptsize);
      }

    }

  }

exit:
  GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  return 0;
}

static int 
axisclear(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  double min,max,inc;

  min=max=inc=0;
  if (_putobj(obj,"min",inst,&min)) return 1;
  if (_putobj(obj,"max",inst,&max)) return 1;
  if (_putobj(obj,"inc",inst,&inc)) return 1;
  return 0;
}

static int 
axisadjust(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *axis;
  int ad;
  struct objlist *aobj;
  int anum,id;
  struct narray iarray,*array;
  char *inst1;
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
  id=*(int *)arraylast(&iarray);
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
  if (type==1) {
    min=log10(min);
    max=log10(max);
  } else if (type==2) {
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
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static int 
axischangescale(struct objlist *obj,char *inst,
                    double *rmin,double *rmax,double *rinc,int room)
{
  int type;
  double min,max,inc,ming,maxg,order,mmin;
  double a;

  _getobj(obj,"type",inst,&type);
  ming=*rmin;
  maxg=*rmax;
  if (ming>maxg) {
    a=ming;
    ming=maxg;
    maxg=a;
  }
  if (type==1) {
    if (ming<=0) return 1;
    if (maxg<=0) return 1;
    ming=log10(ming);
    maxg=log10(maxg);
  } else if (type==2) {
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
  if (room==0) {
    max=maxg+(maxg-ming)*0.05;
    max=nraise(max/inc*10)*inc/10;
    min=ming-(maxg-ming)*0.05;
    min=cutdown(min/inc*10)*inc/10;
    if (type==1) {
      max=pow(10.0,max);
      min=pow(10.0,min);
      max=log10(nraise(max/scale(max))*scale(max));
      min=log10(cutdown(min/scale(min)+1e-15)*scale(min));
    } else if (type==2) {
      if (ming*min<=0) min=ming;
      if (maxg*max<=0) max=maxg;
    }
  } else {
    max=maxg;
    min=ming;
  }
  if (min==max) max=min+1;
  if (type!=2) {
    inc=scale(max-min);
    if (max<min) inc*=-1;
    mmin=roundmin(min,inc)+inc;
    if ((mmin-min)*(mmin-max)>0) inc/=10;
  } else {
    inc=scale(max-min);
  }
  if ((type!=1) && (inc==0)) inc=1;
  if (type==1) inc=nround(inc);
  inc=fabs(inc);
  if (type==1) {
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
axisscale(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int type,room;
  double min,max,inc;

  _getobj(obj,"type",inst,&type);
  min=*(double *)argv[2];
  max=*(double *)argv[3];
  room=*(int *)argv[4];
  axischangescale(obj,inst,&min,&max,&inc,room);
  if (_putobj(obj,"min",inst,&min)) return 1;
  if (_putobj(obj,"max",inst,&max)) return 1;
  if (_putobj(obj,"inc",inst,&inc)) return 1;
  return 0;
}

static int 
axiscoordinate(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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
  if (type==1) {
    if (min<=0) return 1;
    if (max<=0) return 1;
    min=log10(min);
    max=log10(max);
  } else if (type==2) {
    if (min*max<=0) return 1;
    min=1.0/min;
    max=1.0/max;
  }
  val=min+(max-min)*t/len;
  if (type==1) {
    val=pow(10.0,val);
  } else if (type==2) {
    if (val==0) return 1;
    val=1.0/val;
  }
  *(double *)rval=val;
  return 0;
}

static int 
axisautoscalefile(struct objlist *obj,char *inst,char *fileobj,double *rmin,double *rmax)
{
  struct objlist *fobj;
  int fnum;
  int *fdata;
  struct narray iarray;
  double min,max,min1,max1, frac;
  int i,id,set;
  char buf[20], msgbuf[64], *group;
  char *argv2[4];
  struct narray *minmax;

  arrayinit(&iarray,sizeof(int));
  if (getobjilist(fileobj,&fobj,&iarray,FALSE,NULL)) return 1;
  fnum=arraynum(&iarray);
  fdata=arraydata(&iarray);
  _getobj(obj,"id",inst,&id);
  sprintf(buf,"axis:%d",id);
  _getobj(obj,"group",inst,&group);
  argv2[0]=(void *)buf;
  argv2[1]=NULL;
  min = max = 0;	/* this initialization is added to avoid compile warnings. */
  set=FALSE;
  for (i=0;i<fnum;i++) {
    frac = 1.0 * i / fnum;
    snprintf(msgbuf, sizeof(msgbuf), "%s (%.1f%%)", (group) ? group : buf, frac * 100);
    set_progress(1, msgbuf, frac);
    minmax=NULL;
    getobj(fobj,"bounding",fdata[i],1,argv2,&minmax);

    if (arraynum(minmax)>=2) {
      min1=*(double *)arraynget(minmax,0);
      max1=*(double *)arraynget(minmax,1);
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
axisautoscale(struct objlist *obj,char *inst,char *rval,
                  int argc,char **argv)
{
  char *fileobj;
  int room;
  double omin,omax,oinc;
  double min,max,inc;

  fileobj=(char *)argv[2];
  room=*(int *)argv[3];

  if (axisautoscalefile(obj,inst,fileobj,&min,&max))
    return 1;

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
axisgetautoscale(struct objlist *obj,char *inst,char *rval,
                  int argc,char **argv)
{
  char *fileobj;
  int room;
  double min,max,inc;
  struct narray *result;

  result=*(struct narray **)rval;
  arrayfree(result);
  *(struct narray **)rval=NULL;
  fileobj=(char *)argv[2];
  room=*(int *)argv[3];
  if (axisautoscalefile(obj,inst,fileobj,&min,&max)==0) {
    axischangescale(obj,inst,&min,&max,&inc,room);
    result=arraynew(sizeof(double));
    arrayadd(result,&min);
    arrayadd(result,&max);
    arrayadd(result,&inc);
    *(struct narray **)rval=result;
  }
  return 0;
}

static int 
axistight(struct objlist *obj,char *inst,char *rval, int argc,char **argv)
{
  obj_do_tighten(obj, inst, "reference");
  obj_do_tighten(obj, inst, "adjust_axis");

  return 0;
}

#define BUF_SIZE 16
static void
set_group(struct objlist *obj, int gnum, int id, char axis, char type)
{
  char *group,*group2;
  char *inst2;

  inst2 = chkobjinst(obj, id);
  if (inst2 == NULL) {
    return;
  }

  group = memalloc(BUF_SIZE);
  if (group) {
    _getobj(obj, "group", inst2, &group2);
    memfree(group2);
    snprintf(group, BUF_SIZE, "%c%c%d", type, axis, gnum);
    _putobj(obj, "group", inst2, group);
  }

  group = memalloc(BUF_SIZE);
  if (group) {
    _getobj(obj, "name", inst2, &group2);
    memfree(group2);
    snprintf(group, BUF_SIZE, "%c%c%d", type, axis, gnum);
    _putobj(obj, "name", inst2, group);
  }
}

static int 
axisgrouping(struct objlist *obj,char *inst,char *rval,
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

  data = (int *)arraydata(iarray);

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
  struct narray *array;
  char *inst2;

  inst2 = chkobjinst(obj, id);
  if (inst2 == NULL)
    return;

  _putobj(obj, "direction", inst2, &dir);
  _putobj(obj, "x", inst2, &x);
  _putobj(obj, "y", inst2, &y);
  _putobj(obj, "length", inst2, &len);
  _getobj(obj, "bbox", inst2, &array);
  arrayfree(array);
  _putobj(obj, "bbox", inst2, NULL);
}

static int 
axisgrouppos(struct objlist *obj, char *inst, char *rval, 
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

  data = (int *)arraydata(iarray);

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
  char *inst2;

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
  char *inst2, *ref, *ref2;

  inst2 = chkobjinst(obj, id);
  if (inst2 == NULL)
    return;

  ref = memalloc(BUF_SIZE);
  if (ref == NULL)
    return;

  _getobj(obj, field, inst2, &ref2);
  memfree(ref2);
  snprintf(ref, BUF_SIZE, "axis:^%d", oid);
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
axisdefgrouping(struct objlist *obj,char *inst,char *rval,
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

  data = (int *)arraydata(iarray);

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
axis_save_groupe(struct objlist *obj, int type, char **inst_array, char **rval)
{
  char *s, *str, buf[64];
  int i, n, id;

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

  s = nstrnew();
  if (s == NULL)
    return 1;

  s = nstrcat(s, *rval);
  if (s == NULL)
    return 1;

  s = nstrcat(s, str);
  if (s == NULL)
    return 1;

  for (i = 0; i < n; i++) {
    _getobj(obj, "id", inst_array[i], &id);
    snprintf(buf, sizeof(buf), "%d%c", id, (i == n - 1) ? '\n' : ' ');
    s = nstrcat(s, buf);
    if (s == NULL)
      return 1;
  }

  memfree(*rval);
  *rval = s;

  return 0;
}

static int 
axissave(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i, r, anum, type;
  struct narray *array;
  char **adata, *inst_array[4];

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;

  array = (struct narray *) argv[2];
  anum = arraynum(array);
  adata = arraydata(array);
  for (i = 0; i < anum; i++) {
    if (strcmp("grouping", adata[i]) == 0)
      return 0;
  }

  type = get_axis_group_type(obj, inst, inst_array);

  r = 0;
  switch (type) {
  case 'a':
    break;
  case 'f':
  case 's':
  case 'c':
    r = axis_save_groupe(obj, type, inst_array, (char **) rval);
    break;
  }
  return r;
}

static int 
axismanager(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i,id,lastinst;
  char *group,*group2;
  char type;
  char *inst2;

  _getobj(obj,"id",inst,&id);
  _getobj(obj,"group",inst,&group);
  if ((group==NULL) || (group[0]=='a')) {
    *(int *)rval=id;
    return 0;
  }
  lastinst=chkobjlastinst(obj);
  id=-1;
  type=group[0];
  for (i=0;i<=lastinst;i++) {
    inst2=chkobjinst(obj,i);
    _getobj(obj,"group",inst2,&group2);
    if ((group2!=NULL) && (group2[0]==type) && (strcmp(group+2,group2+2)==0))
      id=i;
  }
  *(int *)rval=id;
  return 0;
}

static int 
axisscalepush(struct objlist *obj,char *inst,char *rval,int argc,
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
  if (num>30) {
    arrayndel(array,29);
    arrayndel(array,28);
    arrayndel(array,27);
  }
  arrayins(array,&inc,0);
  arrayins(array,&max,0);
  arrayins(array,&min,0);
  return 0;
}

static int 
axisscalepop(struct objlist *obj,char *inst,char *rval,int argc,
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

static struct objtable axis[] = {
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
  {"num_jfont",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"num_script_size",NINT,NREAD|NWRITE,axisput,NULL,0},
  {"num_align",NENUM,NREAD|NWRITE,NULL,anumalignchar,0},
  {"num_no_zero",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"num_direction",NENUM,NREAD|NWRITE,NULL,anumdirchar,0},
  {"num_shift_p",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"num_shift_n",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"num_R",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"num_G",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"num_B",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"scale_push",NVFUNC,NREAD|NEXEC,axisscalepush,NULL,0},
  {"scale_pop",NVFUNC,NREAD|NEXEC,axisscalepop,NULL,0},
  {"scale_history",NDARRAY,NREAD,NULL,NULL,0},
  {"scale",NVFUNC,NREAD|NEXEC,axisscale,"ddi",0},
  {"auto_scale",NVFUNC,NREAD|NEXEC,axisautoscale,"oi",0},
  {"get_auto_scale",NDAFUNC,NREAD|NEXEC,axisgetautoscale,"oi",0},
  {"clear",NVFUNC,NREAD|NEXEC,axisclear,NULL,0},
  {"adjust",NVFUNC,NREAD|NEXEC,axisadjust,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,axisdraw,"i",0},
  {"bbox",NIAFUNC,NREAD|NEXEC,axisbbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,axismove,"ii",0},
  {"change",NVFUNC,NREAD|NEXEC,axischange,"iii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,axiszoom,"iiii",0},
  {"match",NBFUNC,NREAD|NEXEC,axismatch,"iiiii",0},
  {"coordinate",NDFUNC,NREAD|NEXEC,axiscoordinate,"ii",0},
  {"tight",NVFUNC,NREAD|NEXEC,axistight,NULL,0},
  {"grouping",NVFUNC,NREAD|NEXEC,axisgrouping,"ia",0},
  {"default_grouping",NVFUNC,NREAD|NEXEC,axisdefgrouping,"ia",0},
  {"group_position",NVFUNC,NREAD|NEXEC,axisgrouppos,"ia",0},
  {"group_manager",NIFUNC,NREAD|NEXEC,axismanager,NULL,0},
  {"save",NSFUNC,NREAD|NEXEC,axissave,"sa",0},
};

#define TBLNUM (sizeof(axis) / sizeof(*axis))

void *addaxis(void)
/* addaxis() returns NULL on error */
{
  unsigned int i;

  if (AxisConfigHash == NULL) {
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

  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, axis, ERRNUM, axiserrorlist, NULL, NULL);
}

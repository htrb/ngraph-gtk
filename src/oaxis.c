/* 
 * $Id: oaxis.c,v 1.2 2008/06/03 07:18:28 hito Exp $
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ngraph.h"
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

#define ERRNUM 6

#define ERRAXISTYPE 100
#define ERRAXISHEAD 101
#define ERRAXISGAUGE 102
#define ERRAXISSPL 103
#define ERRMINMAX 104
#define ERRFORMAT 105

char *axiserrorlist[ERRNUM]={
  "illegal axis type.",
  "illegal arrow/wave type.",
  "illegal gauge type.",
  "error: spline interpolation.",
  "illegal value of min/max/inc.",
  "illegal format.",
};

char *axistypechar[4]={
  "linear",
  "log",
  "inverse",
  NULL
};

char *axisgaugechar[5]={
  "none",
  "both",
  "left",
  "right",
  NULL
};

char *axisnumchar[4]={
  "none",
  "left",
  "right",
  NULL
};

char *anumalignchar[5]={
  "center",
  "left",
  "right",
  "point",
  NULL
};

char *anumdirchar[4]={
  "normal",
  "parallel",
  "parallel2",
  NULL
};

int axisuniqgroup(struct objlist *obj,char type)
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

int axisloadconfig(struct objlist *obj,char *inst,char *conf)
{
  FILE *fp;
  char *tok,*str,*s2;
  char *f1,*f2;
  int val;
  char *endptr;
  int len;
  struct narray *iarray;

  if ((fp=openconfig(conf))==NULL) return 0;
  while ((tok=getconfig(fp,&str))!=NULL) {
    s2=str;
    if (strcmp(tok,"R")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"R",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"G")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"G",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"B")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"B",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"type")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"type",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"direction")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"direction",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"baseline")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"baseline",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"width")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"width",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"arrow")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"arrow",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"arrow_length")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"arrow_length",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"wave")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"wave",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"wave_length")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"wave_length",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"wave_width")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"wave_width",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"gauge")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"gauge",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"gauge_length1")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"gauge_length1",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"gauge_width1")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"gauge_width1",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"gauge_length2")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"gauge_length2",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"gauge_width2")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"gauge_width2",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"gauge_length3")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"gauge_length3",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"gauge_width3")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"gauge_width3",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"gauge_R")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"gauge_R",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"gauge_G")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"gauge_G",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"gauge_B")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"gauge_B",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_auto_norm")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num_auto_norm",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_log_pow")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num_log_pow",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_pt")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num_pt",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_space")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num_space",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_script_size")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num_script_size",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_align")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num_align",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_no_zero")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num_no_zero",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_direction")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num_direction",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_shift_p")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num_shift_p",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_shift_n")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num_shift_n",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_R")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num_R",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_G")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num_G",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_B")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') _putobj(obj,"num_B",inst,&val);
      memfree(f1);
    } else if (strcmp(tok,"num_head")==0) {
      f1=getitok2(&s2,&len,"");
      _getobj(obj,"num_head",inst,&f2);
      memfree(f2);
      _putobj(obj,"num_head",inst,f1);
    } else if (strcmp(tok,"num_format")==0) {
      f1=getitok2(&s2,&len,"");
      _getobj(obj,"num_format",inst,&f2);
      memfree(f2);
      _putobj(obj,"num_format",inst,f1);
    } else if (strcmp(tok,"num_tail")==0) {
      f1=getitok2(&s2,&len,"");
      _getobj(obj,"num_tail",inst,&f2);
      memfree(f2);
      _putobj(obj,"num_tail",inst,f1);
    } else if (strcmp(tok,"num_font")==0) {
      f1=getitok2(&s2,&len,"");
      _getobj(obj,"num_font",inst,&f2);
      memfree(f2);
      _putobj(obj,"num_font",inst,f1);
    } else if (strcmp(tok,"num_jfont")==0) {
      f1=getitok2(&s2,&len,"");
      _getobj(obj,"num_jfont",inst,&f2);
      memfree(f2);
      _putobj(obj,"num_jfont",inst,f1);
    } else if (strcmp(tok,"style")==0) {
      if ((iarray=arraynew(sizeof(int)))!=NULL) {
        while ((f1=getitok2(&s2,&len," \t,"))!=NULL) {
          val=strtol(f1,&endptr,10);
          if (endptr[0]=='\0') arrayadd(iarray,&val);
          memfree(f1);
        }
        _putobj(obj,"style",inst,iarray);
      }
    } else if (strcmp(tok,"gauge_style")==0) {
      if ((iarray=arraynew(sizeof(int)))!=NULL) {
        while ((f1=getitok2(&s2,&len," \t,"))!=NULL) {
          val=strtol(f1,&endptr,10);
          if (endptr[0]=='\0') arrayadd(iarray,&val);
          memfree(f1);
        }
        _putobj(obj,"gauge_style",inst,iarray);
      }
    }
    memfree(tok);
    memfree(str);
  }
  closeconfig(fp);
  return 0;
}

int axisinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int width;
  int alen,awid,wlen,wwid;
  int bline;
  int len1,wid1,len2,wid2,len3,wid3;
  int pt,sx,sy,logpow,scriptsize;
  int autonorm,num,gnum;
  char *font,*jfont,*format,*group,*name;

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
  format=font=jfont=group=name=NULL;
  if ((format=memalloc(3))==NULL) goto errexit;
  strcpy(format,"%g");
  if (_putobj(obj,"num_format",inst,format)) goto errexit;
  if ((font=memalloc(strlen(fontchar[4])+1))==NULL) goto errexit;
  strcpy(font,fontchar[4]);
  if (_putobj(obj,"num_font",inst,font)) goto errexit;
  if ((jfont=memalloc(strlen(jfontchar[1])+1))==NULL) goto errexit;
  strcpy(jfont,jfontchar[1]);
  if (_putobj(obj,"num_jfont",inst,jfont)) goto errexit;
  if ((group=memalloc(13))==NULL) goto errexit;
  gnum=axisuniqgroup(obj,'a');
  sprintf(group,"a_%d",gnum);
  if (_putobj(obj,"group",inst,group)) goto errexit;
  if ((name=memalloc(13))==NULL) goto errexit;
  sprintf(name,"a_%d",gnum);
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

int axisdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

int axisput(struct objlist *obj,char *inst,char *rval,
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
    if (*(int *)(argv[2])<500) *(int *)(argv[2])=500;
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

int axisgeometry(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;

  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}


int axisbbox2(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

int axisbbox(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i,id;
  char *group,*group2;
  char type;
  int findX,findY,findU,findR;
  char *inst2;
  char *instX,*instY,*instU,*instR;
  struct narray *rval2,*array;
  int minx,miny,maxx,maxy;
  int x0,y0,x1,y1;

  array=*(struct narray **)rval;
  if (arraynum(array)!=0) return 0;
  _getobj(obj,"group",inst,&group);
  if ((group==NULL) || (group[0]=='a'))
    return axisbbox2(obj,inst,rval,argc,argv);
  _getobj(obj,"id",inst,&id);
  findX=findY=findU=findR=FALSE;
  type=group[0];
  for (i=0;i<=id;i++) {
    inst2=chkobjinst(obj,i);
    _getobj(obj,"group",inst2,&group2);
    if ((group2!=NULL) && (group2[0]==type)) {
      if (strcmp(group+2,group2+2)==0) {
        if (group2[1]=='X') {
          findX=TRUE;
          instX=inst2;
        } else if (group2[1]=='Y') {
          findY=TRUE;
          instY=inst2;
        } else if (group2[1]=='U') {
          findU=TRUE;
          instU=inst2;
        } else if (group2[1]=='R') {
          findR=TRUE;
          instR=inst2;
        }
      }
    }
  }
  if (((type=='f') || (type=='s')) && findX && findY && findU && findR) {
    rval2=NULL;
    axisbbox2(obj,instX,(void *)&rval2,argc,argv);
    minx=*(int *)arraynget(rval2,0);
    miny=*(int *)arraynget(rval2,1);
    maxx=*(int *)arraynget(rval2,2);
    maxy=*(int *)arraynget(rval2,3);
    arrayfree(rval2);
    rval2=NULL;
    axisbbox2(obj,instY,(void *)&rval2,argc,argv);
    x0=*(int *)arraynget(rval2,0);
    y0=*(int *)arraynget(rval2,1);
    x1=*(int *)arraynget(rval2,2);
    y1=*(int *)arraynget(rval2,3);
    if (x0<minx) minx=x0;
    if (y0<miny) miny=y0;
    if (x1>maxx) maxx=x1;
    if (y1>maxy) maxy=y1;
    arrayfree(rval2);
    rval2=NULL;
    axisbbox2(obj,instU,(void *)&rval2,argc,argv);
    x0=*(int *)arraynget(rval2,0);
    y0=*(int *)arraynget(rval2,1);
    x1=*(int *)arraynget(rval2,2);
    y1=*(int *)arraynget(rval2,3);
    if (x0<minx) minx=x0;
    if (y0<miny) miny=y0;
    if (x1>maxx) maxx=x1;
    if (y1>maxy) maxy=y1;
    arrayfree(rval2);
    rval2=NULL;
    axisbbox2(obj,instR,(void *)&rval2,argc,argv);
    x0=*(int *)arraynget(rval2,0);
    y0=*(int *)arraynget(rval2,1);
    x1=*(int *)arraynget(rval2,2);
    y1=*(int *)arraynget(rval2,3);
    if (x0<minx) minx=x0;
    if (y0<miny) miny=y0;
    if (x1>maxx) maxx=x1;
    if (y1>maxy) maxy=y1;
    arrayfree(rval2);
    if ((array==NULL) && ((array=arraynew(sizeof(int)))==NULL)) return 1;
    arrayadd(array,&minx);
    arrayadd(array,&miny);
    arrayadd(array,&maxx);
    arrayadd(array,&maxy);
    arrayadd(array,&minx);
    arrayadd(array,&miny);
    arrayadd(array,&maxx);
    arrayadd(array,&miny);
    arrayadd(array,&maxx);
    arrayadd(array,&maxy);
    arrayadd(array,&minx);
    arrayadd(array,&maxy);
    if (arraynum(array)==0) {
      arrayfree(array);
      return 1;
    }
    *(struct narray **)rval=array;
  } else if ((type=='c') && findX && findY) {
    rval2=NULL;
    axisbbox2(obj,instX,(void *)&rval2,argc,argv);
    minx=*(int *)arraynget(rval2,0);
    miny=*(int *)arraynget(rval2,1);
    maxx=*(int *)arraynget(rval2,2);
    maxy=*(int *)arraynget(rval2,3);
    arrayfree(rval2);
    rval2=NULL;
    axisbbox2(obj,instY,(void *)&rval2,argc,argv);
    x0=*(int *)arraynget(rval2,0);
    y0=*(int *)arraynget(rval2,1);
    x1=*(int *)arraynget(rval2,2);
    y1=*(int *)arraynget(rval2,3);
    if (x0<minx) minx=x0;
    if (y0<miny) miny=y0;
    if (x1>maxx) maxx=x1;
    if (y1>maxy) maxy=y1;
    arrayfree(rval2);
    if ((array==NULL) && ((array=arraynew(sizeof(int)))==NULL)) return 1;
    arrayadd(array,&minx);
    arrayadd(array,&miny);
    arrayadd(array,&maxx);
    arrayadd(array,&maxy);
    arrayadd(array,&minx);
    arrayadd(array,&miny);
    arrayadd(array,&maxx);
    arrayadd(array,&miny);
    arrayadd(array,&maxx);
    arrayadd(array,&maxy);
    arrayadd(array,&minx);
    arrayadd(array,&maxy);
    if (arraynum(array)==0) {
      arrayfree(array);
      return 1;
    }
    *(struct narray **)rval=array;
  }
  return 0;
}

int axismatch2(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

int axismatch(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i,id;
  char *group,*group2;
  char type;
  int findX,findY,findU,findR;
  char *inst2;
  char *instX,*instY,*instU,*instR;
  int rval2,match;

  *(int *)rval=FALSE;
  _getobj(obj,"group",inst,&group);
  if ((group==NULL) || (group[0]=='a'))
    return axismatch2(obj,inst,rval,argc,argv);
  _getobj(obj,"id",inst,&id);
  findX=findY=findU=findR=FALSE;
  type=group[0];
  for (i=0;i<=id;i++) {
    inst2=chkobjinst(obj,i);
    _getobj(obj,"group",inst2,&group2);
    if ((group2!=NULL) && (group2[0]==type)) {
      if (strcmp(group+2,group2+2)==0) {
        if (group2[1]=='X') {
          findX=TRUE;
          instX=inst2;
        } else if (group2[1]=='Y') {
          findY=TRUE;
          instY=inst2;
        } else if (group2[1]=='U') {
          findU=TRUE;
          instU=inst2;
        } else if (group2[1]=='R') {
          findR=TRUE;
          instR=inst2;
        }
      }
    }
  }
  if (((type=='f') || (type=='s')) && findX && findY && findU && findR) {
    match=FALSE;
    axismatch2(obj,instX,(void *)&rval2,argc,argv);
    match=match || rval2;
    axismatch2(obj,instY,(void *)&rval2,argc,argv);
    match=match || rval2;
    axismatch2(obj,instU,(void *)&rval2,argc,argv);
    match=match || rval2;
    axismatch2(obj,instR,(void *)&rval2,argc,argv);
    match=match || rval2;
    *(int *)rval=match;
  } else if ((type=='c') && findX && findY) {
    match=FALSE;
    axismatch2(obj,instX,(void *)&rval2,argc,argv);
    match=match || rval2;
    axismatch2(obj,instY,(void *)&rval2,argc,argv);
    match=match || rval2;
    *(int *)rval=match;
  }
  return 0;
}

int axismove2(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

int axismove(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i,id;
  char *group,*group2;
  char type;
  int findX,findY,findU,findR;
  char *inst2;
  char *instX,*instY,*instU,*instR;

  _getobj(obj,"group",inst,&group);
  if ((group==NULL) || (group[0]=='a'))
    return axismove2(obj,inst,rval,argc,argv);
  _getobj(obj,"id",inst,&id);
  findX=findY=findU=findR=FALSE;
  type=group[0];
  for (i=0;i<=id;i++) {
    inst2=chkobjinst(obj,i);
    _getobj(obj,"group",inst2,&group2);
    if ((group2!=NULL) && (group2[0]==type)) {
      if (strcmp(group+2,group2+2)==0) {
        if (group2[1]=='X') {
          findX=TRUE;
          instX=inst2;
        } else if (group2[1]=='Y') {
          findY=TRUE;
          instY=inst2;
        } else if (group2[1]=='U') {
          findU=TRUE;
          instU=inst2;
        } else if (group2[1]=='R') {
          findR=TRUE;
          instR=inst2;
        }
      }
    }
  }
  if (((type=='f') || (type=='s')) && findX && findY && findU && findR) {
    axismove2(obj,instX,rval,argc,argv);
    axismove2(obj,instY,rval,argc,argv);
    axismove2(obj,instU,rval,argc,argv);
    axismove2(obj,instR,rval,argc,argv);
  } else if ((type=='c') && findX && findY) {
    axismove2(obj,instX,rval,argc,argv);
    axismove2(obj,instY,rval,argc,argv);
  }
  return 0;
}

int axischange2(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

int axischange(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i,id;
  char *group,*group2;
  char type;
  int findX,findY,findU,findR;
  char *inst2;
  char *instX,*instY,*instU,*instR;
  int point,x0,y0;
  int x,y,len,x1,x2,y1,y2;
  struct narray *array;

  _getobj(obj,"group",inst,&group);
  if ((group==NULL) || (group[0]=='a'))
    return axischange2(obj,inst,rval,argc,argv);
  _getobj(obj,"id",inst,&id);
  findX=findY=findU=findR=FALSE;
  type=group[0];
  for (i=0;i<=id;i++) {
    inst2=chkobjinst(obj,i);
    _getobj(obj,"group",inst2,&group2);
    if ((group2!=NULL) && (group2[0]==type)) {
      if (strcmp(group+2,group2+2)==0) {
        if (group2[1]=='X') {
          findX=TRUE;
          instX=inst2;
        } else if (group2[1]=='Y') {
          findY=TRUE;
          instY=inst2;
        } else if (group2[1]=='U') {
          findU=TRUE;
          instU=inst2;
        } else if (group2[1]=='R') {
          findR=TRUE;
          instR=inst2;
        }
      }
    }
  }
  point=*(int *)argv[2];
  x0=*(int *)argv[3];
  y0=*(int *)argv[4];
  if (((type=='f') || (type=='s')) && findX && findY && findU && findR) {
    _getobj(obj,"y",instX,&y1);
    _getobj(obj,"y",instU,&y2);
    if  (y1<y2) {
      if (point==0) point=3;
      else if (point==1) point=2;
      else if (point==2) point=1;
      else if (point==3) point=0;
    }
    _getobj(obj,"x",instY,&x1);
    _getobj(obj,"x",instR,&x2);
    if  (x1>x2) {
      if (point==0) point=1;
      else if (point==1) point=0;
      else if (point==2) point=3;
      else if (point==3) point=2;
    }
    if (point==0) {
      _getobj(obj,"x",instX,&x);
      _getobj(obj,"length",instX,&len);
      x+=x0;
      len-=x0;
      _putobj(obj,"x",instX,&x);
      _putobj(obj,"length",instX,&len);
      _getobj(obj,"x",instY,&x);
      _getobj(obj,"length",instY,&len);
      x+=x0;
      len-=y0;
      _putobj(obj,"x",instY,&x);
      _putobj(obj,"length",instY,&len);
      _getobj(obj,"x",instU,&x);
      _getobj(obj,"y",instU,&y);
      _getobj(obj,"length",instU,&len);
      x+=x0;
      y+=y0;
      len-=x0;
      _putobj(obj,"x",instU,&x);
      _putobj(obj,"y",instU,&y);
      _putobj(obj,"length",instU,&len);
      _getobj(obj,"length",instR,&len);
      len-=y0;
      _putobj(obj,"length",instR,&len);
    } else if (point==1) {
      _getobj(obj,"length",instX,&len);
      len+=x0;
      _putobj(obj,"length",instX,&len);
      _getobj(obj,"length",instY,&len);
      len-=y0;
      _putobj(obj,"length",instY,&len);
      _getobj(obj,"y",instU,&y);
      _getobj(obj,"length",instU,&len);
      y+=y0;
      len+=x0;
      _putobj(obj,"y",instU,&y);
      _putobj(obj,"length",instU,&len);
      _getobj(obj,"x",instR,&x);
      _getobj(obj,"length",instR,&len);
      x+=x0;
      len-=y0;
      _putobj(obj,"x",instR,&x);
      _putobj(obj,"length",instR,&len);
    } else if (point==2) {
      _getobj(obj,"y",instX,&y);
      _getobj(obj,"length",instX,&len);
      y+=y0;
      len+=x0;
      _putobj(obj,"y",instX,&y);
      _putobj(obj,"length",instX,&len);
      _getobj(obj,"y",instY,&y);
      _getobj(obj,"length",instY,&len);
      y+=y0;
      len+=y0;
      _putobj(obj,"y",instY,&y);
      _putobj(obj,"length",instY,&len);
      _getobj(obj,"length",instU,&len);
      len+=x0;
      _putobj(obj,"length",instU,&len);
      _getobj(obj,"x",instR,&x);
      _getobj(obj,"y",instR,&y);
      _getobj(obj,"length",instR,&len);
      x+=x0;
      y+=y0;
      len+=y0;
      _putobj(obj,"x",instR,&x);
      _putobj(obj,"y",instR,&y);
      _putobj(obj,"length",instR,&len);
    } else if (point==3) {
      _getobj(obj,"x",instX,&x);
      _getobj(obj,"y",instX,&y);
      _getobj(obj,"length",instX,&len);
      x+=x0;
      y+=y0;
      len-=x0;
      _putobj(obj,"x",instX,&x);
      _putobj(obj,"y",instX,&y);
      _putobj(obj,"length",instX,&len);
      _getobj(obj,"x",instY,&x);
      _getobj(obj,"y",instY,&y);
      _getobj(obj,"length",instY,&len);
      x+=x0;
      y+=y0;
      len+=y0;
      _putobj(obj,"x",instY,&x);
      _putobj(obj,"y",instY,&y);
      _putobj(obj,"length",instY,&len);
      _getobj(obj,"x",instU,&x);
      _getobj(obj,"length",instU,&len);
      x+=x0;
      len-=x0;
      _putobj(obj,"x",instU,&x);
      _putobj(obj,"length",instU,&len);
      _getobj(obj,"y",instR,&y);
      _getobj(obj,"length",instR,&len);
      y+=y0;
      len+=y0;
      _putobj(obj,"y",instR,&y);
      _putobj(obj,"length",instR,&len);
    }
    _getobj(obj,"bbox",inst,&array);
    arrayfree(array);
    if (_putobj(obj,"bbox",inst,NULL)) return 1;
  } else if ((type=='c') && findX && findY) {
    if (point==0) {
      _getobj(obj,"x",instX,&x);
      _getobj(obj,"length",instX,&len);
      x+=x0;
      len-=x0;
      _putobj(obj,"x",instX,&x);
      _putobj(obj,"length",instX,&len);
      _getobj(obj,"x",instY,&x);
      _getobj(obj,"length",instY,&len);
      x+=x0;
      len-=y0;
      _putobj(obj,"x",instY,&x);
      _putobj(obj,"length",instY,&len);
    } else if (point==1) {
      _getobj(obj,"length",instX,&len);
      len+=x0;
      _putobj(obj,"length",instX,&len);
      _getobj(obj,"length",instY,&len);
      len-=y0;
      _putobj(obj,"length",instY,&len);
    } else if (point==2) {
      _getobj(obj,"y",instX,&y);
      _getobj(obj,"length",instX,&len);
      y+=y0;
      len+=x0;
      _putobj(obj,"y",instX,&y);
      _putobj(obj,"length",instX,&len);
      _getobj(obj,"y",instY,&y);
      _getobj(obj,"length",instY,&len);
      y+=y0;
      len+=y0;
      _putobj(obj,"y",instY,&y);
      _putobj(obj,"length",instY,&len);
    } else if (point==3) {
      _getobj(obj,"x",instX,&x);
      _getobj(obj,"y",instX,&y);
      _getobj(obj,"length",instX,&len);
      x+=x0;
      y+=y0;
      len-=x0;
      _putobj(obj,"x",instX,&x);
      _putobj(obj,"y",instX,&y);
      _putobj(obj,"length",instX,&len);
      _getobj(obj,"x",instY,&x);
      _getobj(obj,"y",instY,&y);
      _getobj(obj,"length",instY,&len);
      x+=x0;
      y+=y0;
      len+=y0;
      _putobj(obj,"x",instY,&x);
      _putobj(obj,"y",instY,&y);
      _putobj(obj,"length",instY,&len);
    }
    _getobj(obj,"bbox",inst,&array);
    arrayfree(array);
    if (_putobj(obj,"bbox",inst,NULL)) return 1;
  }
  return 0;
}

int axiszoom2(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

int axiszoom(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i,id;
  char *group,*group2;
  char type;
  int findX,findY,findU,findR;
  char *inst2;
  char *instX,*instY,*instU,*instR;

  _getobj(obj,"group",inst,&group);
  if ((group==NULL) || (group[0]=='a'))
    return axiszoom2(obj,inst,rval,argc,argv);
  _getobj(obj,"id",inst,&id);
  findX=findY=findU=findR=FALSE;
  type=group[0];
  for (i=0;i<=id;i++) {
    inst2=chkobjinst(obj,i);
    _getobj(obj,"group",inst2,&group2);
    if ((group2!=NULL) && (group2[0]==type)) {
      if (strcmp(group+2,group2+2)==0) {
        if (group2[1]=='X') {
          findX=TRUE;
          instX=inst2;
        } else if (group2[1]=='Y') {
          findY=TRUE;
          instY=inst2;
        } else if (group2[1]=='U') {
          findU=TRUE;
          instU=inst2;
        } else if (group2[1]=='R') {
          findR=TRUE;
          instR=inst2;
        }
      }
    }
  }
  if (((type=='f') || (type=='s')) && findX && findY && findU && findR) {
    axiszoom2(obj,instX,rval,argc,argv);
    axiszoom2(obj,instY,rval,argc,argv);
    axiszoom2(obj,instU,rval,argc,argv);
    axiszoom2(obj,instR,rval,argc,argv);
  } else if ((type=='c') && findX && findY) {
    axiszoom2(obj,instX,rval,argc,argv);
    axiszoom2(obj,instY,rval,argc,argv);
  }
  return 0;
}

void numformat(char *num,char *format,double a)
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

int axisdraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

    if (ndir==0) ndirection=0;
    else if (ndir==1) ndirection=direction;
    else if (ndir==2) {
      ndirection=direction+18000;
      if (ndirection>36000) ndirection-=36000;
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

int axisclear(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  double min,max,inc;

  min=max=inc=0;
  if (_putobj(obj,"min",inst,&min)) return 1;
  if (_putobj(obj,"max",inst,&max)) return 1;
  if (_putobj(obj,"inc",inst,&inc)) return 1;
  return 0;
}

int axisadjust(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

int axischangescale(struct objlist *obj,char *inst,
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

int axisscale(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

int axiscoordinate(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

int axisautoscalefile(struct objlist *obj,char *inst,char *fileobj,double *rmin,double *rmax)
{
  struct objlist *fobj;
  int fnum;
  int *fdata;
  struct narray iarray;
  double min,max,min1,max1;
  int i,id,set;
  char buf[20];
  char *argv2[4];
  struct narray *minmax;

  arrayinit(&iarray,sizeof(int));
  if (getobjilist(fileobj,&fobj,&iarray,FALSE,NULL)) return 1;
  fnum=arraynum(&iarray);
  fdata=arraydata(&iarray);
  _getobj(obj,"id",inst,&id);
  sprintf(buf,"axis:%d",id);
  argv2[0]=(void *)buf;
  argv2[1]=NULL;
  set=FALSE;
  for (i=0;i<fnum;i++) {
    minmax=NULL;
    getobj(fobj,"bounding",fdata[i],1,argv2,&minmax);
    if (arraynum(minmax)>=2) {
      min1=*(double *)arraynget(minmax,0);
      max1=*(double *)arraynget(minmax,1);
      if (!set) {
        min=min1;
        max=max1;
      } else {
        if (min1<min) min=min1;
        if (max1>max) max=max1;
      }
      set=TRUE;
    }
  }
  arraydel(&iarray);
  if (!set) return 1;
  *rmin=min;
  *rmax=max;
  return 0;
}

int axisautoscale(struct objlist *obj,char *inst,char *rval,
                  int argc,char **argv)
{
  char *fileobj;
  int room;
  double omin,omax,oinc;
  double min,max,inc;

  fileobj=(char *)argv[2];
  room=*(int *)argv[3];
  if (axisautoscalefile(obj,inst,fileobj,&min,&max)==0) {
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
  }
  return 0;
}

int axisgetautoscale(struct objlist *obj,char *inst,char *rval,
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

int axistight(struct objlist *obj,char *inst,char *rval,
              int argc,char **argv)
{
  char *axis,*axis2;
  struct narray iarray;
  int anum,id,oid;
  struct objlist *aobj;

  if ((!_getobj(obj,"reference",inst,&axis)) && (axis!=NULL)) {
    arrayinit(&iarray,sizeof(int));
    if (!getobjilist(axis,&aobj,&iarray,FALSE,NULL)) {
      anum=arraynum(&iarray);
      if (anum>0) {
        id=*(int *)arraylast(&iarray);
        if (getobj(aobj,"oid",id,0,NULL,&oid)!=-1) {
          if ((axis2=(char *)memalloc(strlen(chkobjectname(aobj))+10))!=NULL) {
            sprintf(axis2,"%s:^%d",chkobjectname(aobj),oid);
            _putobj(obj,"reference",inst,axis2);
            memfree(axis);
          }
        }
      }
    }
    arraydel(&iarray);
  }
  if ((!_getobj(obj,"adjust_axis",inst,&axis)) && (axis!=NULL)) {
    arrayinit(&iarray,sizeof(int));
    if (!getobjilist(axis,&aobj,&iarray,FALSE,NULL)) {
      anum=arraynum(&iarray);
      if (anum>0) {
        id=*(int *)arraylast(&iarray);
        if (getobj(aobj,"oid",id,0,NULL,&oid)!=-1) {
          if ((axis2=(char *)memalloc(strlen(chkobjectname(aobj))+10))!=NULL) {
            sprintf(axis2,"%s:^%d",chkobjectname(aobj),oid);
            _putobj(obj,"adjust_axis",inst,axis2);
            memfree(axis);
          }
        }
      }
    }
    arraydel(&iarray);
  }
  return 0;
}

int axisgrouping(struct objlist *obj,char *inst,char *rval,
                 int argc,char **argv)
{
  struct narray *iarray;
  int *data;
  int num,gnum;
  char *group,*group2;
  char *inst2,type;

  iarray=(struct narray *)argv[2];
  num=arraynum(iarray);
  if (num<1) return 1;
  data=(int *)arraydata(iarray);
  if (data[0]==1) type='f';
  else if (data[0]==2) type='s';
  else if (data[0]==3) type='c';
  gnum=axisuniqgroup(obj,type);
  if ((data[0]==1) || (data[0]==2)) {
    if (num<5) return 1;
    if ((inst2=chkobjinst(obj,data[1]))!=NULL) {
      if ((group=memalloc(13))!=NULL) {
        _getobj(obj,"group",inst2,&group2);
        memfree(group2);
        sprintf(group,"%cX%d",type,gnum);
        _putobj(obj,"group",inst2,group);
      }
      if ((group=memalloc(13))!=NULL) {
        _getobj(obj,"name",inst2,&group2);
        memfree(group2);
        sprintf(group,"%cX%d",type,gnum);
        _putobj(obj,"name",inst2,group);
      }
    }
    if ((inst2=chkobjinst(obj,data[2]))!=NULL) {
      if ((group=memalloc(13))!=NULL) {
        _getobj(obj,"group",inst2,&group2);
        memfree(group2);
        sprintf(group,"%cY%d",type,gnum);
        _putobj(obj,"group",inst2,group);
      }
      if ((group=memalloc(13))!=NULL) {
        _getobj(obj,"name",inst2,&group2);
        memfree(group2);
        sprintf(group,"%cY%d",type,gnum);
        _putobj(obj,"name",inst2,group);
      }
    }
    if ((inst2=chkobjinst(obj,data[3]))!=NULL) {
      if ((group=memalloc(13))!=NULL) {
        _getobj(obj,"group",inst2,&group2);
        memfree(group2);
        sprintf(group,"%cU%d",type,gnum);
        _putobj(obj,"group",inst2,group);
      }
      if ((group=memalloc(13))!=NULL) {
        _getobj(obj,"name",inst2,&group2);
        memfree(group2);
        sprintf(group,"%cU%d",type,gnum);
        _putobj(obj,"name",inst2,group);
      }
    }
    if ((inst2=chkobjinst(obj,data[4]))!=NULL) {
      if ((group=memalloc(13))!=NULL) {
        _getobj(obj,"group",inst2,&group2);
        memfree(group2);
        sprintf(group,"%cR%d",type,gnum);
        _putobj(obj,"group",inst2,group);
      }
      if ((group=memalloc(13))!=NULL) {
        _getobj(obj,"name",inst2,&group2);
        memfree(group2);
        sprintf(group,"%cR%d",type,gnum);
        _putobj(obj,"name",inst2,group);
      }
    }
  } else if (data[0]==3) {
    if (num<3) return 1;
    if ((inst2=chkobjinst(obj,data[1]))!=NULL) {
      if ((group=memalloc(13))!=NULL) {
        _getobj(obj,"group",inst2,&group2);
        memfree(group2);
        sprintf(group,"%cX%d",type,gnum);
        _putobj(obj,"group",inst2,group);
      }
      if ((group=memalloc(13))!=NULL) {
        _getobj(obj,"name",inst2,&group2);
        memfree(group2);
        sprintf(group,"%cX%d",type,gnum);
        _putobj(obj,"name",inst2,group);
      }
    }
    if ((inst2=chkobjinst(obj,data[2]))!=NULL) {
      if ((group=memalloc(13))!=NULL) {
        _getobj(obj,"group",inst2,&group2);
        memfree(group2);
        sprintf(group,"%cY%d",type,gnum);
        _putobj(obj,"group",inst2,group);
      }
      if ((group=memalloc(13))!=NULL) {
        _getobj(obj,"name",inst2,&group2);
        memfree(group2);
        sprintf(group,"%cY%d",type,gnum);
        _putobj(obj,"name",inst2,group);
      }
    }
  }
  return 0;
}

int axisgrouppos(struct objlist *obj,char *inst,char *rval,
                 int argc,char **argv)
{
  int x0,y0,x,y,lenx,leny,len,dir;
  struct narray *iarray;
  struct narray *array;
  int *data;
  int anum;
  char *inst2;

  iarray=(struct narray *)argv[2];
  anum=arraynum(iarray);
  if (anum<1) return 1;
  data=(int *)arraydata(iarray);
  if ((data[0]==1) || (data[0]==2)) {
    if (anum<9) return 1;
    x0=data[5];
    y0=data[6];
    lenx=data[7];
    leny=data[8];
    if ((inst2=chkobjinst(obj,data[1]))!=NULL) {
      x=x0;
      y=y0;
      len=lenx;
      dir=0;
      _putobj(obj,"direction",inst2,&dir);
      _putobj(obj,"x",inst2,&x);
      _putobj(obj,"y",inst2,&y);
      _putobj(obj,"length",inst2,&len);
      _getobj(obj,"bbox",inst2,&array);
      arrayfree(array);
      _putobj(obj,"bbox",inst2,NULL);
    }
    if ((inst2=chkobjinst(obj,data[2]))!=NULL) {
      x=x0;
      y=y0;
      len=leny;
      dir=9000;
      _putobj(obj,"direction",inst2,&dir);
      _putobj(obj,"x",inst2,&x);
      _putobj(obj,"y",inst2,&y);
      _putobj(obj,"length",inst2,&len);
      _getobj(obj,"bbox",inst2,&array);
      arrayfree(array);
      _putobj(obj,"bbox",inst2,NULL);
    }
    if ((inst2=chkobjinst(obj,data[3]))!=NULL) {
      x=x0;
      y=y0-leny;
      len=lenx;
      dir=0;
      _putobj(obj,"direction",inst2,&dir);
      _putobj(obj,"x",inst2,&x);
      _putobj(obj,"y",inst2,&y);
      _putobj(obj,"length",inst2,&len);
      _getobj(obj,"bbox",inst2,&array);
      arrayfree(array);
      _putobj(obj,"bbox",inst2,NULL);
    }
    if ((inst2=chkobjinst(obj,data[4]))!=NULL) {
      x=x0+lenx;
      y=y0;
      len=leny;
      dir=9000;
      _putobj(obj,"direction",inst2,&dir);
      _putobj(obj,"x",inst2,&x);
      _putobj(obj,"y",inst2,&y);
      _putobj(obj,"length",inst2,&len);
      _getobj(obj,"bbox",inst2,&array);
      arrayfree(array);
      _putobj(obj,"bbox",inst2,NULL);
    }
  } else if (data[0]==3) {
    if (anum<7) return 1;
    x0=data[3];
    y0=data[4];
    lenx=data[5];
    leny=data[6];
    if ((inst2=chkobjinst(obj,data[1]))!=NULL) {
      x=x0;
      y=y0;
      len=lenx;
      dir=0;
      _putobj(obj,"direction",inst2,&dir);
      _putobj(obj,"x",inst2,&x);
      _putobj(obj,"y",inst2,&y);
      _putobj(obj,"length",inst2,&len);
      _getobj(obj,"bbox",inst2,&array);
      arrayfree(array);
      _putobj(obj,"bbox",inst2,NULL);
    }
    if ((inst2=chkobjinst(obj,data[2]))!=NULL) {
      x=x0;
      y=y0;
      len=leny;
      dir=9000;
      _putobj(obj,"direction",inst2,&dir);
      _putobj(obj,"x",inst2,&x);
      _putobj(obj,"y",inst2,&y);
      _putobj(obj,"length",inst2,&len);
      _getobj(obj,"bbox",inst2,&array);
      arrayfree(array);
      _putobj(obj,"bbox",inst2,NULL);
    }
  }
  return 0;
}

int axisdefgrouping(struct objlist *obj,char *inst,char *rval,
                 int argc,char **argv)
{
  int dir,gauge,num,align,oidx,oidy;
  struct narray *iarray;
  int *data;
  int anum;
  char *ref,*ref2;
  char *inst2;

  if (axisgrouping(obj,inst,rval,argc,argv)) return 1;
  iarray=(struct narray *)argv[2];
  anum=arraynum(iarray);
  if (anum<1) return 1;
  data=(int *)arraydata(iarray);
  oidx=oidy=0;
  if ((data[0]==1) || (data[0]==2)) {
    if (anum<5) return 1;
    if ((inst2=chkobjinst(obj,data[1]))!=NULL) {
      dir=0;
      if (data[0]==2) gauge=0;
      else gauge=2;
      num=2;
      align=0;
      _putobj(obj,"gauge",inst2,&gauge);
      _putobj(obj,"num",inst2,&num);
      _putobj(obj,"num_align",inst2,&align);
      _putobj(obj,"direction",inst2,&dir);
      _getobj(obj,"oid",inst2,&oidx);
      if (data[0]==1) axisloadconfig(obj,inst2,"[axis_fX]");
      else axisloadconfig(obj,inst2,"[axis_sX]");
    }
    if ((inst2=chkobjinst(obj,data[2]))!=NULL) {
      dir=9000;
      if (data[0]==2) gauge=0;
      else gauge=3;
      num=1;
      align=2;
      _putobj(obj,"gauge",inst2,&gauge);
      _putobj(obj,"num",inst2,&num);
      _putobj(obj,"num_align",inst2,&align);
      _putobj(obj,"direction",inst2,&dir);
      _getobj(obj,"oid",inst2,&oidy);
      if (data[0]==1) axisloadconfig(obj,inst2,"[axis_fY]");
      else axisloadconfig(obj,inst2,"[axis_sY]");
    }
    if ((inst2=chkobjinst(obj,data[3]))!=NULL) {
      dir=0;
      if (data[0]==2) gauge=0;
      else gauge=3;
      num=1;
      align=0;
      _putobj(obj,"gauge",inst2,&gauge);
      _putobj(obj,"num",inst2,&num);
      _putobj(obj,"num_align",inst2,&align);
      _putobj(obj,"direction",inst2,&dir);
      if ((ref=memalloc(15))!=NULL) {
        _getobj(obj,"reference",inst2,&ref2);
        memfree(ref2);
        sprintf(ref,"axis:^%d",oidx);
        _putobj(obj,"reference",inst2,ref);
      }
      if (data[0]==1) axisloadconfig(obj,inst2,"[axis_fU]");
      else axisloadconfig(obj,inst2,"[axis_sU]");
    }
    if ((inst2=chkobjinst(obj,data[4]))!=NULL) {
      dir=9000;
      if (data[0]==2) gauge=0;
      else gauge=2;
      num=2;
      align=1;
      _putobj(obj,"gauge",inst2,&gauge);
      _putobj(obj,"num",inst2,&num);
      _putobj(obj,"num_align",inst2,&align);
      _putobj(obj,"direction",inst2,&dir);
      if ((ref=memalloc(15))!=NULL) {
        _getobj(obj,"reference",inst2,&ref2);
        memfree(ref2);
        sprintf(ref,"axis:^%d",oidy);
        _putobj(obj,"reference",inst2,ref);
      }
      if (data[0]==1) axisloadconfig(obj,inst2,"[axis_fR]");
      else axisloadconfig(obj,inst2,"[axis_sR]");
    }
    if (anum<9) return 0;
    if (axisgrouppos(obj,inst,rval,argc,argv)) return 1;
  } else if (data[0]==3) {
    if (anum<3) return 1;
    if ((inst2=chkobjinst(obj,data[1]))!=NULL) {
      dir=0;
      gauge=1;
      num=2;
      align=0;
      _putobj(obj,"gauge",inst2,&gauge);
      _putobj(obj,"num",inst2,&num);
      _putobj(obj,"num_align",inst2,&align);
      _putobj(obj,"direction",inst2,&dir);
      _getobj(obj,"oid",inst2,&oidx);
    }
    if ((inst2=chkobjinst(obj,data[2]))!=NULL) {
      dir=9000;
      gauge=1;
      num=1;
      align=2;
      _putobj(obj,"gauge",inst2,&gauge);
      _putobj(obj,"num",inst2,&num);
      _putobj(obj,"num_align",inst2,&align);
      _putobj(obj,"direction",inst2,&dir);
      _getobj(obj,"oid",inst2,&oidy);
    }
    if ((inst2=chkobjinst(obj,data[1]))!=NULL) {
      if ((ref=memalloc(15))!=NULL) {
        _getobj(obj,"adjust_axis",inst2,&ref2);
        memfree(ref2);
        sprintf(ref,"axis:^%d",oidy);
        _putobj(obj,"adjust_axis",inst2,ref);
      }
      axisloadconfig(obj,inst2,"[axis_cX]");
    }
    if ((inst2=chkobjinst(obj,data[2]))!=NULL) {
      if ((ref=memalloc(15))!=NULL) {
        _getobj(obj,"adjust_axis",inst2,&ref2);
        memfree(ref2);
        sprintf(ref,"axis:^%d",oidx);
        _putobj(obj,"adjust_axis",inst2,ref);
      }
      axisloadconfig(obj,inst2,"[axis_cY]");
    }
    if (anum<7) return 0;
    if (axisgrouppos(obj,inst,rval,argc,argv)) return 1;
  }
  return 0;
}

int axissave(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int i,j,id;
  char *group,*group2;
  struct narray *array;
  int anum;
  char **adata;
  char type;
  int idx,idy,idu,idr;
  int findX,findY,findU,findR;
  char *inst2;
  char buf[12];
  char *s;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"group",inst,&group);
  if ((group==NULL) || (group[0]=='a')) return 0;
  array=(struct narray *)argv[2];
  anum=arraynum(array);
  adata=arraydata(array);
  for (j=0;j<anum;j++)
    if (strcmp("grouping",adata[j])==0) return 0;
  _getobj(obj,"id",inst,&id);
  findX=findY=findU=findR=FALSE;
  type=group[0];
  for (i=0;i<=id;i++) {
    inst2=chkobjinst(obj,i);
    _getobj(obj,"group",inst2,&group2);
    if ((group2!=NULL) && (group2[0]==type)) {
      if (strcmp(group+2,group2+2)==0) {
        if (group2[1]=='X') {
          findX=TRUE;
          idx=i;
        } else if (group2[1]=='Y') {
          findY=TRUE;
          idy=i;
        } else if (group2[1]=='U') {
          findU=TRUE;
          idu=i;
        } else if (group2[1]=='R') {
          findR=TRUE;
          idr=i;
        }
      }
    }
  }
  if ((type=='f') && findX && findY && findU && findR) {
    if ((s=nstrnew())==NULL) return 1;
    if ((s=nstrcat(s,*(char **)rval))==NULL) return 1;
    if ((s=nstrcat(s,"\naxis::grouping 1"))==NULL) return 1;
    sprintf(buf," %d",idx);
    if ((s=nstrcat(s,buf))==NULL) return 1;
    sprintf(buf," %d",idy);
    if ((s=nstrcat(s,buf))==NULL) return 1;
    sprintf(buf," %d",idu);
    if ((s=nstrcat(s,buf))==NULL) return 1;
    sprintf(buf," %d\n",idr);
    if ((s=nstrcat(s,buf))==NULL) return 1;
    memfree(*(char **)rval);
    *(char **)rval=s;
  } else if ((type=='s') && findX && findY && findU && findR) {
    if ((s=nstrnew())==NULL) return 1;
    if ((s=nstrcat(s,*(char **)rval))==NULL) return 1;
    if ((s=nstrcat(s,"\naxis::grouping 2"))==NULL) return 1;
    sprintf(buf," %d",idx);
    if ((s=nstrcat(s,buf))==NULL) return 1;
    sprintf(buf," %d",idy);
    if ((s=nstrcat(s,buf))==NULL) return 1;
    sprintf(buf," %d",idu);
    if ((s=nstrcat(s,buf))==NULL) return 1;
    sprintf(buf," %d\n",idr);
    if ((s=nstrcat(s,buf))==NULL) return 1;
    memfree(*(char **)rval);
    *(char **)rval=s;
  } else if ((type=='c') && findX && findY) {
    if ((s=nstrnew())==NULL) return 1;
    if ((s=nstrcat(s,*(char **)rval))==NULL) return 1;
    if ((s=nstrcat(s,"\naxis::grouping 3"))==NULL) return 1;
    sprintf(buf," %d",idx);
    if ((s=nstrcat(s,buf))==NULL) return 1;
    sprintf(buf," %d\n",idy);
    if ((s=nstrcat(s,buf))==NULL) return 1;
    memfree(*(char **)rval);
    *(char **)rval=s;
  }
  return 0;
}

int axismanager(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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

int axisscalepush(struct objlist *obj,char *inst,char *rval,int argc,
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

int axisscalepop(struct objlist *obj,char *inst,char *rval,int argc,
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

#define TBLNUM 81

struct objtable axis[TBLNUM] = {
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
  {"direction",NINT,NREAD|NWRITE,axisgeometry,NULL,0},
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

void *addaxis()
/* addaxis() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,axis,ERRNUM,axiserrorlist,NULL,NULL);
}

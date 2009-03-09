/* 
 * $Id: otext.c,v 1.13 2009/03/09 10:21:48 hito Exp $
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
#include <math.h>
#include <ctype.h>

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include "ngraph.h"
#include "object.h"
#include "gra.h"
#include "oroot.h"
#include "odraw.h"
#include "otext.h"
#include "olegend.h"
#include "mathfn.h"
#include "nstring.h"
#include "nconfig.h"

#define NAME "text"
#define PARENT "legend"
#define OVERSION  "1.00.00"
#define TEXTCONF "[text]"
#define TRUE  1
#define FALSE 0

static char *texterrorlist[]={
  ""
};

#define ERRNUM (sizeof(texterrorlist) / sizeof(*texterrorlist))

static struct obj_config TextConfig[] = {
  {"R", OBJ_CONFIG_TYPE_NUMERIC},
  {"G", OBJ_CONFIG_TYPE_NUMERIC},
  {"B", OBJ_CONFIG_TYPE_NUMERIC},
  {"pt", OBJ_CONFIG_TYPE_NUMERIC},
  {"space", OBJ_CONFIG_TYPE_NUMERIC},
  {"direction", OBJ_CONFIG_TYPE_NUMERIC},
  {"script_size", OBJ_CONFIG_TYPE_NUMERIC},
  {"raw", OBJ_CONFIG_TYPE_NUMERIC},
  {"font", OBJ_CONFIG_TYPE_STRING},
  {"jfont", OBJ_CONFIG_TYPE_STRING},
};

static NHASH TextConfigHash = NULL;

static int 
textloadconfig(struct objlist *obj,char *inst)
{
  return obj_load_config(obj, inst, TEXTCONF, TextConfigHash);
}

static int 
textsaveconfig(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  return obj_save_config(obj, inst, TEXTCONF, TextConfig, sizeof(TextConfig) / sizeof(*TextConfig));
}

static int 
textinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int pt,scriptsize;
  char *font,*jfont;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  pt=2000;
  scriptsize=7000;
  if (_putobj(obj,"pt",inst,&pt)) return 1;
  if (_putobj(obj,"script_size",inst,&scriptsize)) return 1;

  font = memalloc(strlen(fontchar[4]) + 1);
  jfont = memalloc(strlen(jfontchar[1]) + 1);
  if (font == NULL || jfont == NULL) {
    memfree(font);
    memfree(jfont);
    return 1;
  }
  strcpy(font,fontchar[4]);
  strcpy(jfont,jfontchar[1]);
  if (_putobj(obj,"font",inst,font) || _putobj(obj,"jfont",inst,jfont)) {
    memfree(font);
    memfree(jfont);
    return 1;
  }
  textloadconfig(obj,inst);
  return 0;
}

static int 
textdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int 
textgeometry(struct objlist *obj,char *inst,char *rval,
                 int argc,char **argv)
{
  char *field;
  struct narray *array; 

  field=(char *)(argv[1]);
  if (strcmp(field,"pt") == 0) {
    if (*(int *)(argv[2])<TEXT_SIZE_MIN) {
      *(int *)(argv[2])=TEXT_SIZE_MIN;
    }
  } else if (strcmp(field,"script_size")==0) {
    if (*(int *)(argv[2]) < TEXT_OBJ_SCRIPT_SIZE_MIN) {
      *(int *)(argv[2])=TEXT_OBJ_SCRIPT_SIZE_MIN;
    } else if (*(int *)(argv[2])>TEXT_OBJ_SCRIPT_SIZE_MAX) {
      *(int *)(argv[2])=TEXT_OBJ_SCRIPT_SIZE_MAX;
    }
  }
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static int 
textdraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int GC;
  int x,y,pt,space,dir,fr,fg,fb,tm,lm,w,h,scriptsize,raw;
  char *font,*jfont;
  char *text;
  int clip,zoom;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"GC",inst,&GC);
  if (GC<0) return 0;
  _getobj(obj,"R",inst,&fr);
  _getobj(obj,"G",inst,&fg);
  _getobj(obj,"B",inst,&fb);
  _getobj(obj,"text",inst,&text);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"pt",inst,&pt);
  _getobj(obj,"space",inst,&space);
  _getobj(obj,"direction",inst,&dir);
  _getobj(obj,"script_size",inst,&scriptsize);
  _getobj(obj,"raw",inst,&raw);
  _getobj(obj,"font",inst,&font);
  _getobj(obj,"jfont",inst,&jfont);
  _getobj(obj,"clip",inst,&clip);
  GRAregion(GC,&lm,&tm,&w,&h,&zoom);
  GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);
  GRAcolor(GC,fr,fg,fb);
  GRAmoveto(GC,x,y);
  if (raw) GRAdrawtextraw(GC,text,font,jfont,pt,space,dir);
  else GRAdrawtext(GC,text,font,jfont,pt,space,dir,scriptsize);
  GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  return 0;
}

static int 
textprintf(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct narray *array;
  char **argv2;
  int argc2;
  char *format,*format2,*s;
  int po,arg,i,j,quote;
  char *ret;
  char *buf;
  int len1,len2,len;
  int vi,err;
  long long int vll;
  double vd;
  char *endptr;

  memfree(*(char **)rval);
  *(char **)rval=NULL;
  array=(struct narray *)argv[2];
  argv2=arraydata(array);
  argc2=arraynum(array);
  if (argc2<1) return 0;
  format=argv2[0];
  if ((ret=nstrnew())==NULL) return 1;
  po=0;
  arg=1;
  while (format[po]!='\0') {
    quote=FALSE;
    for (i=po;(quote || (format[i]!='%')) && (format[i]!='\0');i++) {
      if (quote) quote=FALSE;
      else if (format[i]=='\\') quote=TRUE;
    }
    if (i>po) {
      if ((ret=nstrncat(ret,format+po,i-po))==NULL) return 1;
    }
    po=i;
    if (format[po]=='%') {
      for (i = 1; format[po+i] != '\0' && strchr("diouxXeEfgGcs%",format[po+i]) == NULL; i++) {
	if (strchr("$*qjzZtLh", format[i])) {
          memfree(ret);
          return 1;
	}
      }
      if (format[po+i]!='\0') {
        if ((format2=memalloc(i+2))==NULL) {
          memfree(ret);
          return 1;
        }
        strncpy(format2,format+po,i+1);
        format2[i+1]='\0';
        len1=len2=0;
        err=FALSE;
        s=format2;
        for (j=0;(s[j]!='\0') && (!isdigit(s[j]));j++);
        if (isdigit(s[j])) {
          len1=strtol(s+j,&endptr,10);
          if (len1<0) len1=0;
          if (endptr[0]=='.') {
            s=endptr;
            for (j=0;(s[j]!='\0') && (strchr(".*",s[j])!=NULL);j++);
            if (isdigit(s[j])) {
              len2=strtol(s+j,&endptr,10);
              if (len2<0) len2=0;
            } else {
	      err = TRUE;
	    }
          }
        }
        if (!err) {
          len=len1+len2+256;
          switch (format[po+i]) {
          case 'd': case 'i': case 'o': case 'u': case 'x': case 'X':
	    if (i > 2 && strncmp(format2 + i - 2, "ll", 2) == 0) {
	      vll=0;
	      if ((arg<argc2) && (argv2[arg]!=NULL)) {
		vll=strtoll(argv2[arg],&endptr,10);
	      }
	      arg++;
	      if ((buf=memalloc(len))!=NULL) {
		sprintf(buf,format2,vll);
		ret=nstrcat(ret,buf);
		memfree(buf);
	      }
	    }else {
	      vi=0;
	      if ((arg<argc2) && (argv2[arg]!=NULL)) {
		vi=strtol(argv2[arg],&endptr,10);
	      }
	      arg++;
	      if ((buf=memalloc(len))!=NULL) {
		sprintf(buf,format2,vi);
		ret=nstrcat(ret,buf);
		memfree(buf);
	      }
	    }
            break;
          case 'e': case 'E': case 'f': case 'g': case 'G':
	    if (i > 2 && strncmp(format2 + i - 2, "ll", 2) == 0) {
	      break;
	    }
            vd=0.0;
            if ((arg<argc2) && (argv2[arg]!=NULL)) {
              vd=strtod(argv2[arg],&endptr);
            }
            arg++;
            if ((buf=memalloc(len))!=NULL) {
              sprintf(buf,format2,vd);
              ret=nstrcat(ret,buf);
              memfree(buf);
            }
            break;
          case 's':
	    if (i > 1 && format2[i - 1] == 'l') {
	      break;
	    }
            if ((arg<argc2) && (argv2[arg]!=NULL)) {
              if ((buf=memalloc(len+strlen(argv2[arg])))!=NULL) {
                sprintf(buf,format2,argv2[arg]);
                ret=nstrcat(ret,buf);
                memfree(buf);
              }
            }
            arg++;
            break;
          case 'c':
	    if (i > 1 && format2[i - 1] == 'l') {
	      break;
	    }
            if ((arg<argc2) && (argv2[arg]!=NULL)) {
              if ((buf=memalloc(len+strlen(argv2[arg])))!=NULL) {
                sprintf(buf,format2,argv2[arg][0]);
                ret=nstrcat(ret,buf);
                memfree(buf);
              }
            }
            arg++;
            break;
          }
        }
        memfree(format2);
        if (ret==NULL) return 1;
        po+=i+1;
      } else po++;
    }
  }
  *(char **)rval=ret;
  return 0;
}

static int 
textbbox(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy;
  struct narray *array;
  int x,y,pt,space,dir,scriptsize,raw;
  char *font,*jfont;
  char *text;
  int gx0,gy0,gx1,gy1;
  int i,ggx[4],ggy[4];
  double si,co;

  array=*(struct narray **)rval;
  if (arraynum(array)!=0) return 0;
  _getobj(obj,"text",inst,&text);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"pt",inst,&pt);
  _getobj(obj,"space",inst,&space);
  _getobj(obj,"direction",inst,&dir);
  _getobj(obj,"script_size",inst,&scriptsize);
  _getobj(obj,"raw",inst,&raw);
  _getobj(obj,"font",inst,&font);
  _getobj(obj,"jfont",inst,&jfont);
  if (raw) GRAtextextentraw(text,font,jfont,pt,space,&gx0,&gy0,&gx1,&gy1);
  else GRAtextextent(text,font,jfont,pt,space,scriptsize,&gx0,&gy0,&gx1,&gy1,FALSE);
  si=-sin(dir/18000.0*MPI);
  co=cos(dir/18000.0*MPI);
  ggx[0]=x+gx0*co-gy0*si;
  ggy[0]=y+gx0*si+gy0*co;
  ggx[1]=x+gx1*co-gy0*si;
  ggy[1]=y+gx1*si+gy0*co;
  ggx[2]=x+gx0*co-gy1*si;
  ggy[2]=y+gx0*si+gy1*co;
  ggx[3]=x+gx1*co-gy1*si;
  ggy[3]=y+gx1*si+gy1*co;
  minx=ggx[0];
  maxx=ggx[0];
  miny=ggy[0];
  maxy=ggy[0];
  for (i=1;i<4;i++) {
    if (ggx[i]<minx) minx=ggx[i];
    if (ggx[i]>maxx) maxx=ggx[i];
    if (ggy[i]<miny) miny=ggy[i];
    if (ggy[i]>maxy) maxy=ggy[i];
  }
  if ((array==NULL) && ((array=arraynew(sizeof(int)))==NULL)) return 1;
  arrayins(array,&(maxy),0);
  arrayins(array,&(maxx),0);
  arrayins(array,&(miny),0);
  arrayins(array,&(minx),0);
  if (arraynum(array)==0) {
    arrayfree(array);
    *(struct narray **) rval = NULL;
    return 1;
  }
  *(struct narray **)rval=array;
  return 0;
}

static int 
textmove(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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
textzoom(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int x,y,pt,space,refx,refy;
  double zoom;
  struct narray *array;

  zoom=(*(int *)argv[2])/10000.0;
  refx=(*(int *)argv[3]);
  refy=(*(int *)argv[4]);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"pt",inst,&pt);
  _getobj(obj,"space",inst,&space);
  x=(x-refx)*zoom+refx;
  y=(y-refy)*zoom+refy;
  pt=pt*zoom;
  space=space*zoom;

  if (pt < 1)
    pt = 1;

  if (_putobj(obj,"x",inst,&x)) return 1;
  if (_putobj(obj,"y",inst,&y)) return 1;
  if (_putobj(obj,"pt",inst,&pt)) return 1;
  if (_putobj(obj,"space",inst,&space)) return 1;
  _getobj(obj,"bbox",inst,&array);
  arrayfree(array);
  if (_putobj(obj,"bbox",inst,NULL)) return 1;
  return 0;
}

static int 
textmatch(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int minx,miny,maxx,maxy,err;
  int bminx,bminy,bmaxx,bmaxy;
  struct narray *array;
  int gx0,gy0,gx1,gy1;
  int px,py,px2,py2;
  double si,co;
  int x,y,pt,space,dir,scriptsize,raw;
  char *font,*jfont;
  char *text;

  *(int *)rval=FALSE;
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"text",inst,&text);
  _getobj(obj,"x",inst,&x);
  _getobj(obj,"y",inst,&y);
  _getobj(obj,"pt",inst,&pt);
  _getobj(obj,"space",inst,&space);
  _getobj(obj,"direction",inst,&dir);
  _getobj(obj,"script_size",inst,&scriptsize);
  _getobj(obj,"raw",inst,&raw);
  _getobj(obj,"font",inst,&font);
  _getobj(obj,"jfont",inst,&jfont);
  minx=*(int *)(argv[2]);
  miny=*(int *)(argv[3]);
  maxx=*(int *)(argv[4]);
  maxy=*(int *)(argv[5]);
  err=*(int *)(argv[6]);
  if ((minx==maxx) && (miny==maxy)) {
    if (raw) GRAtextextentraw(text,font,jfont,pt,space,&gx0,&gy0,&gx1,&gy1);
    else GRAtextextent(text,font,jfont,pt,space,scriptsize,&gx0,&gy0,&gx1,&gy1,FALSE);
    si=sin(dir/18000.0*MPI);
    co=cos(dir/18000.0*MPI);
    px=minx-x;
    py=miny-y;
    px2=px*co-py*si;
    py2=px*si+py*co;
    if ((gx0<=px2) && (px2<=gx1)
     && (gy0<=py2) && (py2<=gy1)) *(int *)rval=TRUE;
  } else {
    if (_exeobj(obj,"bbox",inst,0,NULL)) return 1;
    _getobj(obj,"bbox",inst,&array);
    if (arraynum(array)<4) return 1;
    bminx=*(int *)arraynget(array,0);
    bminy=*(int *)arraynget(array,1);
    bmaxx=*(int *)arraynget(array,2);
    bmaxy=*(int *)arraynget(array,3);
    if ((minx<=bminx) && (bminx<=maxx)
     && (minx<=bmaxx) && (bmaxx<=maxx)
     && (miny<=bminy) && (bminy<=maxy)
     && (miny<=bmaxy) && (bmaxy<=maxy)) *(int *)rval=TRUE;
  }
  return 0;
}

static struct objtable text[] = {
  {"init",NVFUNC,NEXEC,textinit,NULL,0},
  {"done",NVFUNC,NEXEC,textdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"text",NSTR,NREAD|NWRITE,textgeometry,NULL,0},
  {"x",NINT,NREAD|NWRITE,textgeometry,NULL,0},
  {"y",NINT,NREAD|NWRITE,textgeometry,NULL,0},
  {"pt",NINT,NREAD|NWRITE,textgeometry,NULL,0},
  {"font",NSTR,NREAD|NWRITE,textgeometry,NULL,0},
  {"jfont",NSTR,NREAD|NWRITE,textgeometry,NULL,0},
  {"space",NINT,NREAD|NWRITE,textgeometry,NULL,0},
  {"direction",NINT,NREAD|NWRITE,textgeometry,NULL,0},
  {"script_size",NINT,NREAD|NWRITE,textgeometry,NULL,0},
  {"raw",NBOOL,NREAD|NWRITE,textgeometry,NULL,0},

  {"draw",NVFUNC,NREAD|NEXEC,textdraw,"i",0},

  {"printf",NSFUNC,NREAD|NEXEC,textprintf,"sa",0},
  {"bbox",NIAFUNC,NREAD|NEXEC,textbbox,"",0},
  {"move",NVFUNC,NREAD|NEXEC,textmove,"ii",0},
  {"zooming",NVFUNC,NREAD|NEXEC,textzoom,"iiii",0},
  {"match",NBFUNC,NREAD|NEXEC,textmatch,"iiiii",0},
  {"save_config",NVFUNC,NREAD|NEXEC,textsaveconfig,NULL,0},
};

#define TBLNUM (sizeof(text) / sizeof(*text))

void *addtext()
/* addtext() returns NULL on error */
{
  unsigned int i;

  if (TextConfigHash == NULL) {
    TextConfigHash = nhash_new();
    if (TextConfigHash == NULL)
      return NULL;

    for (i = 0; i < sizeof(TextConfig) / sizeof(*TextConfig); i++) {
      if (nhash_set_ptr(TextConfigHash, TextConfig[i].name, (void *) &TextConfig[i])) {
	nhash_free(TextConfigHash);
	return NULL;
      }
    }
  }

  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,text,ERRNUM,texterrorlist,NULL,NULL);
}

/* 
 * $Id: oagrid.c,v 1.1 2008/05/29 09:37:33 hito Exp $
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <math.h>
#include <string.h>
#include <stdio.h>
#include "ngraph.h"
#include "object.h"
#include "mathfn.h"
#include "gra.h"
#include "axis.h"
#include "oroot.h"
#include "odraw.h"

#define NAME "axisgrid"
#define PARENT "draw"
#define OVERSION  "1.00.00"
#define TRUE  1
#define FALSE 0

#define ERRNUM 3

#define ERRNOAXISINST 100
#define ERRMINMAX 101
#define ERRAXISDIR 102

char *agriderrorlist[ERRNUM]={
  "no instance for axis",
  "illegal axis min/max.",
  "illegal axis direction.", 
};

int agridinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int wid1,wid2,wid3,dot;
  int r,g,b,br,bg,bb;
  struct narray *style1;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  wid1=5;
  wid2=10;
  wid3=20;
  r=0;
  g=255;
  b=255;
  br=255;
  bg=255;
  bb=255;
  style1=arraynew(sizeof(int));
  dot=150;
  arrayadd(style1,&dot);
  arrayadd(style1,&dot);
  if (_putobj(obj,"width1",inst,&wid1)) return 1;
  if (_putobj(obj,"width2",inst,&wid2)) return 1;
  if (_putobj(obj,"width3",inst,&wid3)) return 1;
  if (_putobj(obj,"style1",inst,style1)) return 1;
  if (_putobj(obj,"R",inst,&r)) return 1;
  if (_putobj(obj,"G",inst,&g)) return 1;
  if (_putobj(obj,"B",inst,&b)) return 1;
  if (_putobj(obj,"BR",inst,&br)) return 1;
  if (_putobj(obj,"BG",inst,&bg)) return 1;
  if (_putobj(obj,"BB",inst,&bb)) return 1;
  return 0;
}


int agriddone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

int agriddraw(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int GC;
  int fr,fg,fb,br,bg,bb,lm,tm,w,h;
  char *axisx,*axisy;
  int wid1,wid2,wid3,wid;
  struct narray *st1,*st2,*st3;
  int snum,*sdata,snum1,snum2,snum3,*sdata1,*sdata2,*sdata3;
  struct narray iarray;
  struct objlist *aobj;
  int anum,id;
  char *inst1;
  int axposx,axposy,ayposx,ayposy,axdir,aydir,dirx,diry,axlen,aylen;
  double axmin,axmax,aymin,aymax,axinc,ayinc,dir;
  int axdiv,aydiv,axtype,aytype;
  struct axislocal alocal;
  int rcode,gx0,gy0,gx1,gy1,x0,y0,x1,y1;
  double po,minx,miny,maxx,maxy;
  int clip,zoom,back;
  char *raxis;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"GC",inst,&GC);
  if (GC<0) return 0;
  _getobj(obj,"R",inst,&fr);
  _getobj(obj,"G",inst,&fg);
  _getobj(obj,"B",inst,&fb);
  _getobj(obj,"axis_x",inst,&axisx);
  _getobj(obj,"axis_y",inst,&axisy);
  _getobj(obj,"width1",inst,&wid1);
  _getobj(obj,"style1",inst,&st1);
  _getobj(obj,"width2",inst,&wid2);
  _getobj(obj,"style2",inst,&st2);
  _getobj(obj,"width3",inst,&wid3);
  _getobj(obj,"style3",inst,&st3);
  _getobj(obj,"clip",inst,&clip);
  _getobj(obj,"background",inst,&back);
  _getobj(obj,"BR",inst,&br);
  _getobj(obj,"BG",inst,&bg);
  _getobj(obj,"BB",inst,&bb);
  snum1=arraynum(st1);
  sdata1=arraydata(st1);
  snum2=arraynum(st2);
  sdata2=arraydata(st2);
  snum3=arraynum(st3);
  sdata3=arraydata(st3);

  if (axisx==NULL) {
    return 0;
  } else {
    arrayinit(&iarray,sizeof(int));
    if (getobjilist(axisx,&aobj,&iarray,FALSE,NULL)) return 1;
    anum=arraynum(&iarray);
    if (anum<1) {
      arraydel(&iarray);
      error2(obj,ERRNOAXISINST,axisx);
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
    if (_getobj(aobj,"inc",inst1,&axinc)) return 1;
    if (_getobj(aobj,"div",inst1,&axdiv)) return 1;
    if (_getobj(aobj,"type",inst1,&axtype)) return 1;
    if ((axmin==0) && (axmax==0) && (axinc==0)) {
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
              _getobj(aobj,"inc",inst1,&axinc);
              _getobj(aobj,"div",inst1,&axdiv);
              _getobj(aobj,"type",inst1,&axtype);
            }
          }
        }
      }
    }
    if ((dirx%9000)!=0) {
      error(obj,ERRAXISDIR);
      return 1;
    }
    axdir=dirx/9000;
    if (axmin!=axmax) {
      if (axtype==1) {
        minx=log10(axmin);
        maxx=log10(axmax);
      } else if (axtype==2) {
        minx=1/axmin;
        maxx=1/axmax;
      } else {
        minx=axmin;
        maxx=axmax;
      }
    }
  }
  if (axisy==NULL) {
    return 0;
  } else {
    arrayinit(&iarray,sizeof(int));
    if (getobjilist(axisy,&aobj,&iarray,FALSE,NULL)) return 1;
    anum=arraynum(&iarray);
    if (anum<1) {
      arraydel(&iarray);
      error2(obj,ERRNOAXISINST,axisy);
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
    if (_getobj(aobj,"inc",inst1,&ayinc)) return 1;
    if (_getobj(aobj,"div",inst1,&aydiv)) return 1;
    if (_getobj(aobj,"type",inst1,&aytype)) return 1;
    if ((aymin==0) && (aymax==0) && (ayinc==0)) {
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
              _getobj(aobj,"inc",inst1,&ayinc);
              _getobj(aobj,"div",inst1,&aydiv);
              _getobj(aobj,"type",inst1,&aytype);
            }
          }
        }
      }
    }
    if ((diry%9000)!=0) {
      error(obj,ERRAXISDIR);
      return 1;
    }
    aydir=diry/9000;
    if (aymin!=aymax) {
      if (aytype==1) {
        miny=log10(aymin);
        maxy=log10(aymax);
      } else if (aytype==2) {
        miny=1/aymin;
        maxy=1/aymax;
      } else {
        miny=aymin;
        maxy=aymax;
      }
    }
  }
  if (((axdir+aydir)%2)==0) {
    error(obj,ERRAXISDIR);
    return 1;
  }

  GRAregion(GC,&lm,&tm,&w,&h,&zoom);
  GRAview(GC,0,0,w*10000.0/zoom,h*10000.0/zoom,clip);
  if (back) {
    GRAcolor(GC,br,bg,bb);
    dir=dirx/18000.0*MPI;
    gx0=axposx;
    gx1=axposx;
    x1=axposx+nround(axlen*cos(dir));
    if (x1<gx0) gx0=x1;
    if (x1>gx1) gx1=x1;
    gy0=axposy;
    gy1=axposy;
    y1=axposy-nround(axlen*sin(dir));
    if (y1<gy0) gy0=y1;
    if (y1>gy1) gy1=y1;
    dir=diry/18000.0*MPI;
    x1=ayposx;
    if (x1<gx0) gx0=x1;
    if (x1>gx1) gx1=x1;
    x1=ayposx+nround(aylen*cos(dir));
    if (x1<gx0) gx0=x1;
    if (x1>gx1) gx1=x1;
    y1=ayposy;
    if (y1<gy0) gy0=y1;
    if (y1>gy1) gy1=y1;
    y1=ayposy-nround(aylen*sin(dir));
    if (y1<gy0) gy0=y1;
    if (y1>gy1) gy1=y1;
    GRArectangle(GC,gx0,gy0,gx1,gy1,1);
  }
  if ((axmin==axmax) || (aymin==aymax)) goto exit;
  GRAcolor(GC,fr,fg,fb);

  if (getaxispositionini(&alocal,axtype,axmin,axmax,axinc,axdiv,TRUE)!=0) {
    error(obj,ERRMINMAX);
    goto exit;
  }
  while ((rcode=getaxisposition(&alocal,&po))!=-2) {
    if (rcode>=1) {
      if (rcode==1) {
        snum=snum1;
        sdata=sdata1;
        wid=wid1;
      } else if (rcode==2) {
        snum=snum2;
        sdata=sdata2;
        wid=wid2;
      } else {
        snum=snum3;
        sdata=sdata3;
        wid=wid3;
      }
      if (wid!=0) {
        GRAlinestyle(GC,snum,sdata,wid,0,0,1000);
        if (axdir==0) gx0=axposx+(po-minx)*axlen/(maxx-minx);
        else if (axdir==1) gy0=axposy-(po-minx)*axlen/(maxx-minx);
        else if (axdir==2) gx0=axposx-(po-minx)*axlen/(maxx-minx);
        else gy0=axposy+(po-minx)*axlen/(maxx-minx);
        if (aydir==0) {
          x0=ayposx;
          y0=gy0;
          x1=ayposx+aylen;
          y1=gy0;
        } else if (aydir==1) {
          x0=gx0;
          y0=ayposy;
          x1=gx0;
          y1=ayposy-aylen;
        } else if (aydir==2) {
          x0=ayposx;
          y0=gy0;
          x1=ayposx-aylen;
          y1=gy0;
        } else {
          x0=ayposx;
          y0=gy0;
          x1=ayposx-aylen;
          y1=gy0;
        }
        GRAline(GC,x0,y0,x1,y1);
      }
    }
  }
  if (getaxispositionini(&alocal,aytype,aymin,aymax,ayinc,aydiv,TRUE)!=0) {
    error(obj,ERRMINMAX);
    goto exit;
  }
  while ((rcode=getaxisposition(&alocal,&po))!=-2) {
    if (rcode>=1) {
      if (rcode==1) {
        snum=snum1;
        sdata=sdata1;
        wid=wid1;
      } else if (rcode==2) {
        snum=snum2;
        sdata=sdata2;
        wid=wid2;
      } else {
        snum=snum3;
        sdata=sdata3;
        wid=wid3;
      }
      if (wid!=0) {
        GRAlinestyle(GC,snum,sdata,wid,0,0,1000);
        if (aydir==0) gx0=ayposx+(po-miny)*aylen/(maxy-miny);
        else if (aydir==1) gy0=ayposy-(po-miny)*aylen/(maxy-miny);
        else if (aydir==2) gx0=ayposx-(po-miny)*aylen/(maxy-miny);
        else gy0=ayposy+(po-miny)*aylen/(maxy-miny);
        if (axdir==0) {
          x0=axposx;
          y0=gy0;
          x1=axposx+axlen;
          y1=gy0;
        } else if (axdir==1) {
          x0=gx0;
          y0=axposy;
          x1=gx0;
          y1=axposy-axlen;
        } else if (axdir==2) {
          x0=axposx;
          y0=gy0;
          x1=axposx-axlen;
          y1=gy0;
        } else {
          x0=axposx;
          y0=gy0;
          x1=axposx-axlen;
          y1=gy0;
        }
        GRAline(GC,x0,y0,x1,y1);
      }
    }
  }
exit:
  GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  return 0;
}

int agridtight(struct objlist *obj,char *inst,char *rval,
               int argc,char **argv)
{
  char *axis,*axis2;
  struct narray iarray;
  int anum,id,oid;
  struct objlist *aobj;

  if ((!_getobj(obj,"axis_x",inst,&axis)) && (axis!=NULL)) {
    arrayinit(&iarray,sizeof(int));
    if (!getobjilist(axis,&aobj,&iarray,FALSE,NULL)) {
      anum=arraynum(&iarray);
      if (anum>0) {
        id=*(int *)arraylast(&iarray);
        if (getobj(aobj,"oid",id,0,NULL,&oid)!=-1) {
          if ((axis2=(char *)memalloc(strlen(chkobjectname(aobj))+10))!=NULL) {
            sprintf(axis2,"%s:^%d",chkobjectname(aobj),oid);
            _putobj(obj,"axis_x",inst,axis2);
            memfree(axis);
          }
        }
      }
    }
    arraydel(&iarray);
  }
  if ((!_getobj(obj,"axis_y",inst,&axis)) && (axis!=NULL)) {
    arrayinit(&iarray,sizeof(int));
    if (!getobjilist(axis,&aobj,&iarray,FALSE,NULL)) {
      anum=arraynum(&iarray);
      if (anum>0) {
        id=*(int *)arraylast(&iarray);
        if (getobj(aobj,"oid",id,0,NULL,&oid)!=-1) {
          if ((axis2=(char *)memalloc(strlen(chkobjectname(aobj))+10))!=NULL) {
            sprintf(axis2,"%s:^%d",chkobjectname(aobj),oid);
            _putobj(obj,"axis_y",inst,axis2);
            memfree(axis);
          }
        }
      }
    }
    arraydel(&iarray);
  }
  return 0;
}

#define TBLNUM 17

struct objtable agrid[TBLNUM] = {
  {"init",NVFUNC,NEXEC,agridinit,NULL,0},
  {"done",NVFUNC,NEXEC,agriddone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"axis_x",NOBJ,NREAD|NWRITE,NULL,NULL,0},
  {"axis_y",NOBJ,NREAD|NWRITE,NULL,NULL,0},
  {"width1",NINT,NREAD|NWRITE,oputabs,NULL,0},
  {"style1",NIARRAY,NREAD|NWRITE,NULL,NULL,0},
  {"width2",NINT,NREAD|NWRITE,oputabs,NULL,0},
  {"style2",NIARRAY,NREAD|NWRITE,NULL,NULL,0},
  {"width3",NINT,NREAD|NWRITE,oputabs,NULL,0},
  {"style3",NIARRAY,NREAD|NWRITE,NULL,NULL,0},
  {"background",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"BR",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"BG",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"BB",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,agriddraw,"i",0},
  {"tight",NVFUNC,NREAD|NEXEC,agridtight,NULL,0},
};

void *addagrid()
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,agrid,ERRNUM,agriderrorlist,NULL,NULL);
}

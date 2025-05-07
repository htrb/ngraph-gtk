/*
 * $Id: ogra.c,v 1.11 2009-11-16 09:13:04 hito Exp $
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

#include <limits.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <glib.h>

#include "object.h"
#include "ioutil.h"
#include "ogra.h"
#include "ogra_error.h"
#include "gra.h"
#include "nstring.h"

#define NAME "gra"
#define PARENT "object"
#define OVERSION  "1.00.00"

static char *GRAerrorlist[]={
  "unable to open device",
  "device is busy",
  "device is already opened",
  "no instance for output device",
  "illegal graphics context",
  "gra is now opened.",
  "gra is closed.",
};

#define ERRNUM (sizeof(GRAerrorlist) / sizeof(*GRAerrorlist))

char *gra_decimalsign_char[]={
  N_("period"),
  N_("comma"),
  NULL
};

static enum GRA_DECIMALSIGN_TYPE DefaultDecimalsign = GRA_DECIMALSIGN_TYPE_PERIOD;

static void set_progress_val(int i, int n, const char *name);
static int oGRAclose(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);

void
gra_set_default_decimalsign(enum GRA_DECIMALSIGN_TYPE decimalsign)
{
  DefaultDecimalsign = decimalsign;
}

static int
oGRAinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC,width,height,zoom, decimalsign;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  GC=-1;
  width=21000;
  height=29700;
  zoom=10000;
  decimalsign = DefaultDecimalsign;
  if (_putobj(obj,"open",inst,&GC)) return 1;
  if (_putobj(obj,"GC",inst,&GC)) return 1;
  if (_putobj(obj,"zoom",inst,&zoom)) return 1;
  if (_putobj(obj,"paper_width",inst,&width)) return 1;
  if (_putobj(obj,"paper_height",inst,&height)) return 1;
  if (_putobj(obj, "decimalsign", inst, &decimalsign)) return 1;
  return 0;
}

static int
oGRAdisconnect(struct objlist *obj,void *inst,int clear)
{
  struct objlist *dobj;
  struct narray *sarray;
  N_VALUE *dinst;
  char *device,*dfield,*gfield;
  int oid,did,gid;

  _getobj(obj,"oid",inst,&oid);
  _getobj(obj,"_device",inst,&device);
  _putobj(obj,"_device",inst,NULL);
  if (device == NULL) {
    return 0;
  }
  if (((dobj=getobjlist(device,&did,&dfield,NULL))!=NULL)
      && ((dinst=chkobjinstoid(dobj,did))!=NULL)) {
    if ((!chkobjfield(dobj,"_list"))
        && (!_getobj(dobj,"_list",dinst,&sarray)) && (sarray!=NULL)) {
      const struct objlist *gobj;
      char *list;
      list=arraynget_str(sarray,0);
      if (((gobj=getobjlist(list,&gid,&gfield,NULL))!=NULL)
          && (gobj==obj) && (gid==oid) && (strcmp(gfield,"open")==0)) {
        arrayfree2(sarray);
        _putobj(dobj,"_list",dinst,NULL);
        _exeobj(dobj,"disconnect",dinst,0,NULL);
        if (clear) _exeobj(dobj,"disconnect",dinst,0,NULL);
      }
    }
  }
  g_free(device);
  return 0;
}

static int
oGRAdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC;

  _getobj(obj,"GC",inst,&GC);
  GRAclose(GC);
  if (oGRAdisconnect(obj,inst,FALSE)) return 1;
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int
oGRAputdevice(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,
                  int argc,char **argv)
{
  int GC;

  _getobj(obj,"GC",inst,&GC);
  if (GC!=-1) {
    error(obj,ERRGRABUSY);
    return 1;
  }
  if (oGRAdisconnect(obj,inst,FALSE)) return 1;
  return 0;
}

static int
close_gc(struct objlist *obj, N_VALUE *inst, int GC)
{
  GRAclose(GC);
  GC = -1;

  if (_putobj(obj, "GC", inst, &GC)) {
    return 1;
  }

  return 0;
}

static int
oGRAopen(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct narray iarray;
  int GC, r;
  char *device,*dev,*gfield;
  struct objlist *dobj,*robj;
  N_VALUE *dinst;
  void *local;
  int oid,gid;
  int topm,leftm,width,height,zoom;

  _getobj(obj,"device",inst,&device);
  _getobj(obj,"GC",inst,&GC);
  if (GC!=-1) {
    error2(obj,ERRALOPEN,device);
    return 1;
  }
  _getobj(obj,"left_margin",inst,&leftm);
  _getobj(obj,"top_margin",inst,&topm);
  _getobj(obj,"zoom",inst,&zoom);
  _getobj(obj,"paper_width",inst,&width);
  _getobj(obj,"paper_height",inst,&height);
  if (device==NULL) {
    GC=GRAopen(NULL,NULL,NULL,NULL,-1,-1,-1,-1,NULL,NULL);
    if (GC<0) {
      error2(obj,ERROPEN,device);
      return 1;
    }
  } else {
    int anum,id;
    struct narray **list;
    int output, charheight, chardescent, strwidth;
    arrayinit(&iarray,sizeof(int));
    if (getobjilist(device,&dobj,&iarray,FALSE,NULL)) return 1;
    anum=arraynum(&iarray);
    if (anum<1) {
      arraydel(&iarray);
      error2(obj,ERRNODEVICE,device);
      return 1;
    }
    id=arraylast_int(&iarray);
    arraydel(&iarray);

    /* check target device */
    dinst = getobjinst(dobj, id);
    if (dinst == NULL) {
      return 1;
    }

    robj = NULL;

    if (!chkobjfield(dobj,"_output")) {
      if ((output=getobjtblpos(dobj,"_output",&robj))==-1) return 1;
    } else output=-1;

    if (!chkobjfield(dobj,"_strwidth")) {
      if ((strwidth=getobjtblpos(dobj,"_strwidth",&robj))==-1) return 1;
    } else strwidth=-1;

    if (!chkobjfield(dobj,"_charascent")) {
      if ((charheight=getobjtblpos(dobj,"_charascent",&robj))==-1) return 1;
    } else charheight=-1;

    if (!chkobjfield(dobj,"_chardescent")) {
      if ((chardescent=getobjtblpos(dobj,"_chardescent",&robj))==-1) return 1;
    } else chardescent=-1;

    if (robj == NULL) {
      error2(obj, ERROPEN, device);
      return -1;
    }

    if (!chkobjfield(dobj,"_list")) {
      int  offset;

      offset = getobjoffset(dobj, "_list");
      if (offset == -1) {
	return 1;
      }
      list = &dinst[offset].array;
    } else {
      list = NULL;
    }

    if (!chkobjfield(dobj,"_local")) {
      if (_getobj(dobj,"_local",dinst,&local)) return 1;
    } else local=NULL;

    GC=GRAopen(chkobjectname(dobj),"_output",
               robj,dinst,output,strwidth,charheight,chardescent,list,local);

    if (GC==-2) {
      error2(obj,ERRBUSY,device);
      return 1;
    } else if (GC<0) {
      error2(obj,ERROPEN,device);
      return 1;
    }

    /* clear gra connected to target device */
    if ((list!=NULL) && (*list!=NULL) && (arraynum(*list)!=0)) {
      struct objlist *gobj;
      N_VALUE *ginst;
      if (((gobj=getobjlist(arraynget_str(*list,0),&gid,&gfield,NULL))!=NULL)
      && ((ginst=chkobjinstoid(gobj,gid))!=NULL)
      && (!_getobj(gobj,"_device",ginst,&dev))) {
        if (oGRAdisconnect(gobj,ginst,TRUE)) return 1;
      }
    }

    if (oGRAdisconnect(obj,inst,FALSE)) return 1;
    if (!_getobj(dobj,"oid",dinst,&oid)) {
      if ((dev=mkobjlist(dobj,NULL,oid,"_output",TRUE))!=NULL)
      if (_putobj(obj,"_device",inst,dev)) {
        g_free(dev);
        return 1;
      }
    }
  }
  rval->i=GC;
  if (_putobj(obj,"GC",inst,&GC)) return 1;
  r = GRAinit(GC,leftm,topm,width,height,zoom);
  if (r) {
    error2(obj,ERROPEN,device);
    close_gc(obj, inst, GC);
    return r;
  }

  GRAaddlist(GC,obj,inst,(char *)argv[0],(char *)argv[1]);
  return 0;
}

static int
oGRAclose(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC;

  _getobj(obj,"GC",inst,&GC);
  GRAend(GC);
  return close_gc(obj, inst, GC);
}

static void
set_gra_decimalsign(struct objlist *obj, N_VALUE *inst)
{
  enum GRA_DECIMALSIGN_TYPE decimalsign;

  _getobj(obj, "decimalsign", inst, &decimalsign);
  switch (decimalsign) {
  case GRA_DECIMALSIGN_TYPE_PERIOD:
    set_decimalsign(DECIMALSIGN_TYPE_PERIOD);
    break;
  case GRA_DECIMALSIGN_TYPE_COMMA:
    set_decimalsign(DECIMALSIGN_TYPE_COMMA);
    break;
  }
}

static int
oGRAredraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct objlist *dobj;
  N_VALUE *dinst;
  char *device,*dfield,*field;
  int oid,did;

  field=(char *)(argv[1]);
  _getobj(obj,"oid",inst,&oid);
  _getobj(obj,"_device",inst,&device);
  if (device == NULL) {
    return 0;
  }
  set_gra_decimalsign(obj, inst);
  if (((dobj=getobjlist(device,&did,&dfield,NULL))!=NULL)
      && ((dinst=chkobjinstoid(dobj,did))!=NULL)) {
    if (chkobjfield(dobj,field)==0) _exeobj(dobj,field,dinst,0,NULL);
  }
  return 0;
}

static int
oGRAclear(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC;
  struct objlist *dobj;
  N_VALUE *dinst;
  char *device,*dfield,*field;
  int oid,did;

  field=(char *)(argv[1]);
  _getobj(obj,"GC",inst,&GC);
  if (GC!=-1) GRAreopen(GC);
  _getobj(obj,"oid",inst,&oid);
  _getobj(obj,"_device",inst,&device);
  if (device == NULL) {
    return 0;
  }
  if (((dobj=getobjlist(device,&did,&dfield,NULL))!=NULL)
      && ((dinst=chkobjinstoid(dobj,did))!=NULL)) {
    if (chkobjfield(dobj,field)==0) _exeobj(dobj,field,dinst,0,NULL);
  }
  return 0;
}

static int
oGRAputtopm(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC;
  const char *arg;

  _getobj(obj,"GC",inst,&GC);
  if (GC!=-1) {
    error(obj,ERRGRABUSY);
    return 1;
  }
  arg=argv[1];
  if (arg[0]=='p') {
    if ((*(int *)(argv[2]))<=0) *(int *)(argv[2])=1;
  } else if (arg[0]=='z') {
    if ((*(int *)(argv[2]))<=0) *(int *)(argv[2])=1;
  }
  if (oGRAdisconnect(obj,inst,FALSE)) return 1;
  return 0;
}

static int
oGRAdrawparent(const struct objlist *parent, char **oGRAargv, int layer)
{
  struct objlist *ocur;
  int i,instnum;
  const char *objname;

  ocur=chkobjroot();
  while (ocur!=NULL) {
    if (chkobjparent(ocur)==parent) {
      instnum = chkobjlastinst(ocur);
      if (instnum != -1) {
	objname = chkobjectname(ocur);
	if (layer) {
	  GRAlayer(*((int *) oGRAargv[0]), objname);
	}
        for (i=0;i<=instnum;i++) {
	  set_progress_val(i, instnum, objname);

          if (ninterrupt()) return FALSE;
	  exeobj(ocur,"draw",i,1,oGRAargv);
        }
      }
      if (!oGRAdrawparent(ocur, oGRAargv, layer)) return FALSE;
    }
    ocur=ocur->next;
  }
  return TRUE;
}

static int
oGRAdraw(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int GC, layer;
  struct objlist *draw;
  struct narray *array;
  char *oGRAargv[2];

  _getobj(obj,"GC",inst,&GC);
  if (GC==-1) {
    error(obj,ERRGRACLOSE);
    return 1;
  }

  set_gra_decimalsign(obj, inst);
  layer = GRAlayer_support(GC);
  _getobj(obj,"draw_obj",inst,&array);
  oGRAargv[0]=(char *)&GC;
  oGRAargv[1]=NULL;
  if (array==NULL) {
    if ((draw=getobject("draw"))==NULL) return 1;
    oGRAdrawparent(draw, oGRAargv, layer);
  } else {
    char **drawrable;
    const char *objname;
    int j,i,anum,instnum;
    anum=arraynum(array);
    drawrable=arraydata(array);
    for (j=0;j<anum;j++) {
      draw=getobject(drawrable[j]);
      if (draw == NULL)
	continue;

      instnum = chkobjlastinst(draw);
      if (instnum < 0)
	continue;

      objname = chkobjectname(draw);
      if (layer) {
	GRAlayer(GC, objname);
      }
      for (i=0;i<=instnum;i++) {
	set_progress_val(i, instnum, objname);

	if (ninterrupt()) return 0;
	exeobj(draw,"draw",i,1,oGRAargv);
      }
    }
  }
  return 0;
}

static void
set_progress_val(int i, int n, const char *name)
{
  double frac;
  char msgbuf[1024];

  if (i == 0) {
    set_progress(0, "", 0);
  }

  frac = 1.0 * i / (n + 1);
  snprintf(msgbuf, sizeof(msgbuf), _("drawing %s (%.1f%%)"), name, frac * 100);
  set_progress(1, msgbuf, frac);
}

static struct objtable GRA[] = {
  {"init",NVFUNC,NEXEC,oGRAinit,NULL,0},
  {"done",NVFUNC,NEXEC,oGRAdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"device",NOBJ,NREAD|NWRITE,oGRAputdevice,NULL,0},
  {"open",NIFUNC,NEXEC|NREAD,oGRAopen,"",0},
  {"close",NVFUNC,NREAD|NEXEC,oGRAclose,"",0},
  {"left_margin",NINT,NREAD|NWRITE,oGRAputtopm,NULL,0},
  {"top_margin",NINT,NREAD|NWRITE,oGRAputtopm,NULL,0},
  {"zoom",NINT,NREAD|NWRITE,oGRAputtopm,NULL,0},
  {"paper_width",NINT,NREAD|NWRITE,oGRAputtopm,NULL,0},
  {"paper_height",NINT,NREAD|NWRITE,oGRAputtopm,NULL,0},
  {"decimalsign",NENUM,NREAD|NWRITE,NULL,gra_decimalsign_char,0},
  {"draw_obj",NSARRAY,NREAD|NWRITE,NULL,NULL,0},
  {"GC",NINT,NREAD,NULL,NULL,0},
  {"redraw",NVFUNC,NREAD|NEXEC,oGRAredraw,"",0},
  {"flush",NVFUNC,NREAD|NEXEC,oGRAredraw,"",0},
  {"clear",NVFUNC,NREAD|NEXEC,oGRAclear,"",0},
  {"_device",NSTR,NREAD,NULL,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,oGRAdraw,"",0},
};

#define TBLNUM (sizeof(GRA) / sizeof(*GRA))

void *addgra()
/* addgra() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,GRA,ERRNUM,GRAerrorlist,NULL,NULL);
}

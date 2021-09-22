/*
 * $Id: oagrid.c,v 1.20 2010-03-04 08:30:16 hito Exp $
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

#include <math.h>
#include <string.h>
#include <stdio.h>
#include "object.h"
#include "mathfn.h"
#include "gra.h"
#include "axis.h"
#include "oroot.h"
#include "odraw.h"

#define NAME N_("axisgrid")
#define PARENT "draw"
#define OVERSION  "1.00.00"

#define ERRNOAXISINST 100
#define ERRMINMAX 101
#define ERRAXISDIR 102

static char *agriderrorlist[]={
  "no instance for axis",
  "illegal axis min/max.",
  "illegal axis direction.",
};

#define ERRNUM (sizeof(agriderrorlist) / sizeof(*agriderrorlist))

static struct obj_config GridConfig[] = {
  {"R",          OBJ_CONFIG_TYPE_NUMERIC},
  {"G",          OBJ_CONFIG_TYPE_NUMERIC},
  {"B",          OBJ_CONFIG_TYPE_NUMERIC},
  {"A",          OBJ_CONFIG_TYPE_NUMERIC},
  {"style1",     OBJ_CONFIG_TYPE_STYLE},
  {"style2",     OBJ_CONFIG_TYPE_STYLE},
  {"style3",     OBJ_CONFIG_TYPE_STYLE},
  {"background", OBJ_CONFIG_TYPE_NUMERIC},
  {"BR",         OBJ_CONFIG_TYPE_NUMERIC},
  {"BG",         OBJ_CONFIG_TYPE_NUMERIC},
  {"BB",         OBJ_CONFIG_TYPE_NUMERIC},
};

#define GRID_CONFIG_TITLE "[axisgrid]"

static NHASH GridConfigHash = NULL;

static int
agridinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int wid1,wid2,wid3,dot,grid;
  int r,g,b,br,bg,bb,ba;
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
  ba=255;
  style1=arraynew(sizeof(int));
  dot=150;
  grid=TRUE;
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
  if (_putobj(obj,"BA",inst,&ba)) return 1;
  if (_putobj(obj,"grid_x",inst,&grid)) return 1;
  if (_putobj(obj,"grid_y",inst,&grid)) return 1;
  return 0;
}


static int
agriddone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

struct axis_prm {
  int type, div;
  double amin, amax, inc;
  double min, max;
};

struct axis_pos {
  int dir, len, x, y;
  double len_x, len_y;
};

#define GRID_PRM_NUM 3

struct grid_prm {
  int wid[GRID_PRM_NUM];
  int snum[GRID_PRM_NUM];
  int *sdata[GRID_PRM_NUM];
};

static int
calc_intersection(int x1, int y1, int dir1, int x2, int y2, int dir2, int *ix, int *iy)
{
  double x, y, a, b;

  if (dir1 == dir2 || dir1 - dir2 == 18000 || dir1 - dir2 == -18000)
    return 1;

  if (x1 == x2 && y1 == y2) {
    *ix = x1;
    *iy = y1;
    return 0;
  }

  dir1 = -dir1;
  dir2 = -dir2;

  if (dir1 == -9000 || dir1 == -27000) {
    x = x1;
    b = tan(dir2 / 18000.0 * MPI);
    y = y2 + b * (x1 - x2);
  } else if (dir2 == -9000 || dir2 == -27000) {
    x = x2;
    a = tan(dir1 / 18000.0 * MPI);
    y = y1 + a * (x2 - x1);
  } else {
    a = tan(dir1 / 18000.0 * MPI);
    b = tan(dir2 / 18000.0 * MPI);
    x = (y2 - y1 + a * x1 - b * x2) / (a - b);
    y = y1 + a * (x - x1);
  }

  if (dir1 == 0 || dir1 == -18000) {
    y = y1;
  } else if (dir2 == 0 || dir2 == -18000) {
    y = y2;
  }

  *ix = nround(x);
  *iy = nround(y);

  return 0;
}

static int
get_grid_prm(struct objlist *obj, char *axisy,
	     struct axis_prm *prm, struct axis_pos *pos)
{
  struct narray iarray;
  int anum, id;
  N_VALUE *inst1;
  char *raxis;
  struct objlist *aobj;

  if (axisy == NULL) {
    return -1;
  }

  arrayinit(&iarray, sizeof(int));

  if (getobjilist(axisy, &aobj, &iarray, FALSE, NULL))
    return 1;

  anum = arraynum(&iarray);
  if (anum < 1) {
    arraydel(&iarray);
    error2(obj, ERRNOAXISINST, axisy);
    return 1;
  }

  id = arraylast_int(&iarray);
  arraydel(&iarray);

  if ((inst1 = getobjinst(aobj, id)) == NULL) return 1;

  if (_getobj(aobj, "x", inst1, &pos->x)) return 1;
  if (_getobj(aobj, "y", inst1, &pos->y)) return 1;
  if (_getobj(aobj, "length", inst1, &pos->len)) return 1;
  if (_getobj(aobj, "direction", inst1, &pos->dir)) return 1;
  if (_getobj(aobj, "min", inst1, &prm->amin)) return 1;
  if (_getobj(aobj, "max", inst1, &prm->amax)) return 1;
  if (_getobj(aobj, "inc", inst1, &prm->inc)) return 1;
  if (_getobj(aobj, "div", inst1, &prm->div)) return 1;
  if (_getobj(aobj, "type", inst1, &prm->type)) return 1;

  if ((prm->amin == 0) && (prm->amax == 0) && (prm->inc == 0)) {
    if (_getobj(aobj, "reference", inst1, &raxis)) return 1;
    if (raxis) {
      arrayinit(&iarray, sizeof(int));
      if (!getobjilist(raxis, &aobj, &iarray, FALSE, NULL)) {
	anum = arraynum(&iarray);
	if (anum > 0) {
	  id = arraylast_int(&iarray);
	  arraydel(&iarray);
	  if (anum > 0 && (inst1 = getobjinst(aobj, id))) {
	    _getobj(aobj, "min", inst1, &prm->amin);
	    _getobj(aobj, "max", inst1, &prm->amax);
	    _getobj(aobj, "inc", inst1, &prm->inc);
	    _getobj(aobj, "div", inst1, &prm->div);
	    _getobj(aobj, "type", inst1, &prm->type);
	  }
	}
      }
    }
  }

  pos->dir %= 36000;
  if (pos->dir < 0)
    pos->dir += 36000;

  pos->len_x = pos->len * cos(pos->dir / 18000.0 * MPI);
  pos->len_y = pos->len * sin(pos->dir / 18000.0 * MPI);

  if (prm->amin == prm->amax)
    return 0;

  switch (prm->type) {
  case 1:
    prm->min = log10(prm->amin);
    prm->max = log10(prm->amax);
    break;
  case 2:
    prm->min = 1 / prm->amin;
    prm->max = 1 / prm->amax;
    break;
  default:
    prm->min = prm->amin;
    prm->max = prm->amax;
  }

  return 0;
}

static int
draw_grid_line(struct objlist *obj, int GC,
	       struct axis_prm *a1_prm, struct axis_pos *a1_pos,
	       struct axis_pos *a2_pos, struct grid_prm *gprm)
{
  int r, rcode, snum, wid, *sdata;
  double po;
  int x0, y0, x, y;
  struct axislocal alocal;

  if (getaxispositionini(&alocal, a1_prm->type, a1_prm->amin, a1_prm->amax, a1_prm->inc, a1_prm->div, TRUE)) {
    error(obj, ERRMINMAX);
    return 1;
  }

  r = calc_intersection(a1_pos->x, a1_pos->y, a2_pos->dir,
			a2_pos->x, a2_pos->y, a1_pos->dir,
			&x, &y);

  if (r) {
    error(obj, ERRAXISDIR);
    return 1;
  }

  while ((rcode = getaxisposition(&alocal, &po)) != -2) {
    switch (rcode) {
    case 1:
    case 2:
    case 3:
      snum = gprm->snum[rcode - 1];
      sdata = gprm->sdata[rcode - 1];
      wid = gprm->wid[rcode - 1];
      break;
    default:
      wid = 0;
    }

    if (wid == 0)
      continue;

    GRAlinestyle(GC, snum, sdata, wid, GRA_LINE_CAP_BUTT, GRA_LINE_JOIN_MITER, 1000);

    x0 = x + (po - a1_prm->min) * a1_pos->len_x / (a1_prm->max - a1_prm->min);
    y0 = y - (po - a1_prm->min) * a1_pos->len_y / (a1_prm->max - a1_prm->min);

    GRAline(GC, x0, y0, x0 + a2_pos->len_x, y0 - a2_pos->len_y);
  }
  return 0;
}

static int
draw_background(struct objlist *obj, N_VALUE *inst, int GC, struct axis_pos *ax, struct axis_pos *ay)
{
  int r, br, bg, bb, ba, pos[8];

  _getobj(obj, "BR", inst, &br);
  _getobj(obj, "BG", inst, &bg);
  _getobj(obj, "BB", inst, &bb);
  _getobj(obj, "BA", inst, &ba);
  GRAcolor(GC, br, bg, bb, ba);

  r = calc_intersection(ax->x, ax->y, ay->dir,
			ay->x, ay->y, ax->dir,
			pos + 0, pos + 1);
  if (r)
    return 1;

  pos[2] = pos[0] + ax->len_x;
  pos[3] = pos[1] - ax->len_y;

  pos[4] = pos[2] + ay->len_x;
  pos[5] = pos[3] - ay->len_y;

  pos[6] = pos[0] + ay->len_x;
  pos[7] = pos[1] - ay->len_y;

  GRAdrawpoly(GC, 4, pos, GRA_FILL_MODE_EVEN_ODD);

  return 0;
}

static int
agriddraw(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int GC, clip, zoom, back;
  int i, fr, fg, fb, fa, w, h, r, grid_x, grid_y;
  char *axisx, *axisy;
  struct axis_prm ax_prm,  ay_prm;
  struct axis_pos ax_pos,  ay_pos;
  struct grid_prm gprm;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  _getobj(obj, "GC", inst, &GC);
  if (GC < 0) return 0;

  _getobj(obj, "R", inst, &fr);
  _getobj(obj, "G", inst, &fg);
  _getobj(obj, "B", inst, &fb);
  _getobj(obj, "A", inst, &fa);
  _getobj(obj, "axis_x", inst, &axisx);
  _getobj(obj, "axis_y", inst, &axisy);
  _getobj(obj, "clip", inst, &clip);
  _getobj(obj, "background", inst, &back);
  _getobj(obj, "grid_x", inst, &grid_x);
  _getobj(obj, "grid_y", inst, &grid_y);

  for (i = 0; i < GRID_PRM_NUM; i++) {
    char buf[32];
    struct narray *st;

    snprintf(buf, sizeof(buf), "width%d", i + 1);
    _getobj(obj, buf, inst, &gprm.wid[i]);

    snprintf(buf, sizeof(buf), "style%d", i + 1);
    _getobj(obj, buf, inst, &st);

    gprm.snum[i] = arraynum(st);
    gprm.sdata[i] = arraydata(st);
  }

  r = get_grid_prm(obj, axisx, &ax_prm, &ax_pos);
  if (r) {
    return (r > 0) ? 1 : 0;
  }

  r = get_grid_prm(obj, axisy, &ay_prm, &ay_pos);
  if (r) {
    return (r > 0) ? 1 : 0;
  }

  GRAregion(GC, &w, &h, &zoom);
  GRAview(GC, 0, 0, w * 10000.0 / zoom, h * 10000.0 / zoom, clip);

  if (back && draw_background(obj, inst, GC, &ax_pos, &ay_pos)) {
    error(obj, ERRAXISDIR);
    return 1;
  }

  if (ax_prm.amin != ax_prm.amax && ay_prm.amin != ay_prm.amax) {
    GRAcolor(GC, fr, fg, fb, fa);
    if (grid_x && draw_grid_line(obj, GC, &ax_prm, &ax_pos, &ay_pos, &gprm)) {
      goto error_exit;
    }
    if (grid_y && draw_grid_line(obj, GC, &ay_prm, &ay_pos, &ax_pos, &gprm)) {
      goto error_exit;
    }
  }

 error_exit:
  return 0;
}

static int
agridtight(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  obj_do_tighten(obj, inst, "axis_x");
  obj_do_tighten(obj, inst, "axis_y");

  return 0;
}

static struct objtable agrid[] = {
  {"init",NVFUNC,NEXEC,agridinit,NULL,0},
  {"done",NVFUNC,NEXEC,agriddone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"axis_x",NOBJ,NREAD|NWRITE,NULL,NULL,0},
  {"axis_y",NOBJ,NREAD|NWRITE,NULL,NULL,0},
  {"grid_x",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"grid_y",NBOOL,NREAD|NWRITE,NULL,NULL,0},
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
  {"BA",NINT,NREAD|NWRITE,NULL,NULL,0},
  {"draw",NVFUNC,NREAD|NEXEC,agriddraw,"i",0},
  {"tight",NVFUNC,NREAD|NEXEC,agridtight,"",0},
  {"hsb", NVFUNC, NREAD|NEXEC, put_hsb,"ddd",0},
};

#define TBLNUM (sizeof(agrid) / sizeof(*agrid))

void *
addagrid(void)
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,agrid,ERRNUM,agriderrorlist,NULL,NULL);
}

/* 
 * $Id: oagrid.c,v 1.11 2009/04/17 08:23:05 hito Exp $
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

#define ERRNOAXISINST 100
#define ERRMINMAX 101
#define ERRAXISDIR 102

static char *agriderrorlist[]={
  "no instance for axis",
  "illegal axis min/max.",
  "illegal axis direction.", 
};

#define ERRNUM (sizeof(agriderrorlist) / sizeof(*agriderrorlist))

static int 
agridinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
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


static int 
agriddone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

struct axis_prm {
  int type, div;
  double amin, amax, inc;
  double min, max;
};

#define ANGLE_ROTATION 1

struct axis_pos {
  int dir, len, x, y;
#if ANGLE_ROTATION
#else
  int dir2;
#endif
};

struct grid_prm {
  int wid1, wid2, wid3;
  int snum1, snum2, snum3;
  int *sdata1, *sdata2, *sdata3;
};

#if ANGLE_ROTATION
static int
calc_intersection(double x1, double y1, int dir1, double x2, double y2, int dir2, int *ix, int *iy)
{
  double x, y, a, b;;

  if (dir1 == dir2 || dir1 - dir2 == 18000 || dir1 - dir2 == -18000)
    return 1;

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
#endif

static int
get_grid_prm(struct objlist *obj, char *axisy,
	     struct axis_prm *prm, struct axis_pos *pos)
{
  struct narray iarray;
  int anum, id;
  char *inst1, *raxis;
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

  id = * (int *) arraylast(&iarray);
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
	  id = * (int *) arraylast(&iarray);
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

#if ANGLE_ROTATION
#else
  if (pos->dir % 9000 != 0) {
    error(obj,  ERRAXISDIR);
    return 1;
  }

  pos->dir2 = pos->dir / 9000;
#endif

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


#if ANGLE_ROTATION

static int
draw_grid_line(struct objlist *obj, int GC,
	       struct axis_prm *a1_prm, struct axis_pos *a1_pos,
	       struct axis_pos *a2_pos, struct grid_prm *gprm)
{
  int r, rcode, snum, wid, *sdata;
  double po, dir1, dir2, cos1, sin1, cos2, sin2;;
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

  dir1 = a1_pos->dir / 18000.0 * MPI;
  dir2 = a2_pos->dir / 18000.0 * MPI;

  cos1 = a1_pos->len * cos(dir1);
  sin1 = a1_pos->len * sin(dir1);
  cos2 = a2_pos->len * cos(dir2);
  sin2 = a2_pos->len * sin(dir2);

  while ((rcode = getaxisposition(&alocal, &po)) != -2) {
    if (rcode < 1) {
      continue;
    }

    if (rcode == 1) {
      snum = gprm->snum1;
      sdata = gprm->sdata1;
      wid = gprm->wid1;
    } else if (rcode == 2) {
      snum = gprm->snum2;
      sdata = gprm->sdata2;
      wid = gprm->wid2;
    } else {
      snum = gprm->snum3;
      sdata = gprm->sdata3;
      wid = gprm->wid3;
    }

    if (wid == 0)
      continue;

    GRAlinestyle(GC, snum, sdata, wid, 0, 0, 1000);

    x0 = x + (po - a1_prm->min) * cos1 / (a1_prm->max - a1_prm->min);
    y0 = y - (po - a1_prm->min) * sin1 / (a1_prm->max - a1_prm->min);

    GRAline(GC, x0, y0, x0 + cos2, y0 - sin2);
  }
  return 0;
}

#else

static int
draw_grid_line(struct objlist *obj, int GC,
	       struct axis_prm *a1_prm, struct axis_pos *a1_pos,
	       struct axis_pos *a2_pos, struct grid_prm *gprm)
{
  int g, rcode, snum, wid, *sdata;
  double po;
  int x0, y0, x1, y1;
  struct axislocal alocal;

  if (getaxispositionini(&alocal, a1_prm->type, a1_prm->amin, a1_prm->amax, a1_prm->inc, a1_prm->div, TRUE)) {
    error(obj, ERRMINMAX);
    return 1;
  }

  while ((rcode = getaxisposition(&alocal, &po)) != -2) {
    if (rcode < 1) {
      continue;
    }

    if (rcode == 1) {
      snum = gprm->snum1;
      sdata = gprm->sdata1;
      wid = gprm->wid1;
    } else if (rcode == 2) {
      snum = gprm->snum2;
      sdata = gprm->sdata2;
      wid = gprm->wid2;
    } else {
      snum = gprm->snum3;
      sdata = gprm->sdata3;
      wid = gprm->wid3;
    }

    if (wid == 0)
      continue;

    GRAlinestyle(GC, snum, sdata, wid, 0, 0, 1000);

    switch (a1_pos->dir2) {
    case 0:
      g = a1_pos->x + (po - a1_prm->min) * a1_pos->len / (a1_prm->max - a1_prm->min);
      break;
    case 1:
      g = a1_pos->y - (po - a1_prm->min) * a1_pos->len / (a1_prm->max - a1_prm->min);
      break;
    case 2:
      g = a1_pos->x - (po - a1_prm->min) * a1_pos->len / (a1_prm->max - a1_prm->min);
      break;
    case 3:
      g = a1_pos->y + (po - a1_prm->min) * a1_pos->len / (a1_prm->max - a1_prm->min);
      break;
    default:
      /* never reached */
      g = 0;
    }

    switch (a2_pos->dir2) {
    case 0:
      x0 = a2_pos->x;
      y0 = g;
      x1 = a2_pos->x + a2_pos->len;
      y1 = g;
      break;
    case 1:
      x0 = g;
      y0 = a2_pos->y;
      x1 = g;
      y1 = a2_pos->y - a2_pos->len;
      break;
    case 2:
      x0 = a2_pos->x;
      y0 = g;
      x1 = a2_pos->x - a2_pos->len;
      y1 = g;
      break;
    case 3:
      x0 = g;
      y0 = a2_pos->y;
      x1 = g;
      y1 = a2_pos->y + a2_pos->len;
      break;
    default:
      /* never reached */
      x0 = x1 = y0 = y1 = 0;
    }
    GRAline(GC, x0, y0, x1, y1);
  }
  return 0;
}

#endif

#if ANGLE_ROTATION
static void
draw_background(struct objlist *obj, char *inst, int GC, struct axis_pos *ax, struct axis_pos *ay)
{
  int r, br, bg, bb, pos[8];
  double dirx, diry, cosx, sinx, cosy, siny;

  _getobj(obj, "BR", inst, &br);
  _getobj(obj, "BG", inst, &bg);
  _getobj(obj, "BB", inst, &bb);
  GRAcolor(GC, br, bg, bb);

  dirx = ax->dir / 18000.0 * MPI;
  diry = ay->dir / 18000.0 * MPI;

  cosx = ax->len * cos(dirx);
  sinx = ax->len * sin(dirx);

  cosy = ay->len * cos(diry);
  siny = ay->len * sin(diry);

  r = calc_intersection(ax->x, ax->y, ay->dir,
			ay->x, ay->y, ax->dir,
			pos + 0, pos + 1);
  if (r)
    return;

  pos[2] = pos[0] + cosx;
  pos[3] = pos[1] - sinx;

  pos[4] = pos[2] + cosy;
  pos[5] = pos[3] - siny;

  pos[6] = pos[0] + cosy;
  pos[7] = pos[1] - siny;

  GRAdrawpoly(GC, 4, pos, 1);
}

#else

static void
draw_background(struct objlist *obj, char *inst, int GC, struct axis_pos *ax, struct axis_pos *ay)
{
  int gx0, gy0, gx1, gy1, x1, y1, br, bg, bb, pos[8];
  double dir;

  _getobj(obj, "BR", inst, &br);
  _getobj(obj, "BG", inst, &bg);
  _getobj(obj, "BB", inst, &bb);
  GRAcolor(GC, br, bg, bb);

  dir = ax->dir / 18000.0 * MPI;

  gx0 = ax->x;
  gx1 = ax->x;
  x1 = ax->x + nround(ax->len * cos(dir));
  if (x1 < gx0) gx0 = x1;
  if (x1 > gx1) gx1 = x1;

  gy0 = ax->y;
  gy1 = ax->y;
  y1 = ax->y - nround(ax->len * sin(dir));
  if (y1 < gy0) gy0 = y1;
  if (y1 > gy1) gy1 = y1;

  dir = ay->dir / 18000.0 * MPI;

  x1 = ay->x;
  if (x1 < gx0) gx0 = x1;
  if (x1 > gx1) gx1 = x1;

  x1 = ay->x + nround(ay->len * cos(dir));
  if (x1 < gx0) gx0 = x1;
  if (x1 > gx1) gx1 = x1;

  y1 = ay->y;
  if (y1 < gy0) gy0 = y1;
  if (y1 > gy1) gy1 = y1;

  y1 = ay->y - nround(ay->len * sin(dir));
  if (y1 < gy0) gy0 = y1;
  if (y1 > gy1) gy1 = y1;

  GRArectangle(GC, gx0, gy0, gx1, gy1, 1);
}
#endif

static int 
agriddraw(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int GC, clip, zoom, back;
  int fr, fg, fb, lm, tm, w, h, r;
  char *axisx, *axisy;
  struct narray *st1, *st2, *st3;
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
  _getobj(obj, "axis_x", inst, &axisx);
  _getobj(obj, "axis_y", inst, &axisy);
  _getobj(obj, "width1", inst, &gprm.wid1);
  _getobj(obj, "style1", inst, &st1);
  _getobj(obj, "width2", inst, &gprm.wid2);
  _getobj(obj, "style2", inst, &st2);
  _getobj(obj, "width3", inst, &gprm.wid3);
  _getobj(obj, "style3", inst, &st3);
  _getobj(obj, "clip", inst, &clip);
  _getobj(obj, "background", inst, &back);

  gprm.snum1 = arraynum(st1);
  gprm.sdata1 = arraydata(st1);
  gprm.snum2 = arraynum(st2);
  gprm.sdata2 = arraydata(st2);
  gprm.snum3 = arraynum(st3);
  gprm.sdata3 = arraydata(st3);

  r = get_grid_prm(obj, axisx, &ax_prm, &ax_pos);
  if (r) {
    return (r > 0) ? 1 : 0;
  }

  r = get_grid_prm(obj, axisy, &ay_prm, &ay_pos);
  if (r) {
    return (r > 0) ? 1 : 0;
  }

  GRAregion(GC, &lm, &tm, &w, &h, &zoom);
  GRAview(GC, 0, 0, w * 10000.0 / zoom, h * 10000.0 / zoom, clip);
  if (back) {
    draw_background(obj, inst, GC, &ax_pos, &ay_pos);
  }

#if ANGLE_ROTATION
#else
  if (((ax_pos.dir2 + ay_pos.dir2) % 2) == 0) {
    error(obj, ERRAXISDIR);
    return 1;
  }
#endif

  if (ax_prm.amin == ax_prm.amax || ay_prm.amin == ay_prm.amax)
    goto exit;

  GRAcolor(GC, fr, fg, fb);

  if (draw_grid_line(obj, GC, &ax_prm, &ax_pos, &ay_pos, &gprm))
    goto exit;

  draw_grid_line(obj, GC, &ay_prm, &ay_pos, &ax_pos, &gprm);

 exit:
  GRAaddlist(GC, obj, inst, (char *) argv[0], (char *) argv[1]);
  return 0;
}

static int 
agridtight(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
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

#define TBLNUM (sizeof(agrid) / sizeof(*agrid))

void *
addagrid(void)
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,agrid,ERRNUM,agriderrorlist,NULL,NULL);
}

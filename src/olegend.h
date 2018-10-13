/*
 * $Id: olegend.h,v 1.6 2009-05-01 09:15:58 hito Exp $
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

#ifndef OLEGEND_HEADER
#define OLEGEND_HEADER

#define ROTATE_MARK 0

enum FLIP_DIRECTION {
  FLIP_DIRECTION_HORIZONTAL,
  FLIP_DIRECTION_VERTICAL,
};

int legendgeometry(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv);
int legendbbox(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv);
int legendmove(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv);
int legendrotate(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);
int legendchange(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv);
int legendzoom(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv);
int legendmatch(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);
void rotate(int px, int py, int angle, int *x, int *y);
void flip(int pivot, enum FLIP_DIRECTION dir, int *x, int *y);
int legendflip(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);
int put_color_for_backward_compatibility(struct objlist *obj, N_VALUE *inst, N_VALUE *rval,  int argc, char **argv);
void draw_marker_mark(struct objlist *obj, N_VALUE *inst, int GC, int width, int headlen, int headwidth, int x0, int y0, double dx, double dy, int r, int g, int b, int a, int type);
void draw_marker_bar(struct objlist *obj, N_VALUE *inst, int GC, int width, int headlen, int headwidth, int x0, int y0, double dx, double dy);
void draw_marker_wave(struct objlist *obj, N_VALUE *inst, int GC, int width, int headlen, int headwidth, int x0, int y0, double dx, double dy, int errspl);
int mark_flip(int dir, int type);
int mark_rotate(int angle, int type);

#endif

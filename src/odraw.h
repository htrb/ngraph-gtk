/*
 * $Id: odraw.h,v 1.6 2009-08-07 02:52:40 hito Exp $
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

#ifndef ODRAW_HEADER
#define ODRAW_HEADER

enum INTERPOLATION_TYPE {
  INTERPOLATION_TYPE_SPLINE,
  INTERPOLATION_TYPE_SPLINE_CLOSE,
  INTERPOLATION_TYPE_BSPLINE,
  INTERPOLATION_TYPE_BSPLINE_CLOSE,
};

enum ARROW_POSITION_TYPE {
  ARROW_POSITION_NONE,
  ARROW_POSITION_END,
  ARROW_POSITION_BEGIN,
  ARROW_POSITION_BOTH,
};

enum SAVE_PATH_TYPE {
  SAVE_PATH_UNCHANGE,
  SAVE_PATH_FULL,
  SAVE_PATH_RELATIVE,
  SAVE_PATH_BASE,
};

enum JOIN_TYPE {
  JOIN_TYPE_MITER,
  JOIN_TYPE_ROUND,
  JOIN_TYPE_BEVEL,
};

enum FONT_TYPE {
  FONT_TYPE_SANS_SERIF,
  FONT_TYPE_SERIF,
  FONT_TYPE_MONOSPACE,
};

enum FONT_STYLE {
  FONT_STYLE_NORMAL = 0,
  FONT_STYLE_BOLD   = 1,
  FONT_STYLE_ITALIC = 2,
};

#define INTERPOLATION_TYPE_NUM (INTERPOLATION_TYPE_BSPLINE_CLOSE + 1)

extern char *pathchar[];
extern char *joinchar[];
extern char *fontchar[];
extern char *intpchar[];
extern char *arrowchar[];

int pathsave(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv);
int clear_bbox(struct objlist *obj, N_VALUE *inst);
int put_hsb_color(struct objlist *obj, N_VALUE *inst, int argc, char **argv, char *format);
int put_fill_hsb(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int put_stroke_hsb(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int put_hsb(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int put_hsb2(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);

#endif

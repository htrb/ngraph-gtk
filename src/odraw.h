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

#define ARROW_SIZE_MIN 10000
#define ARROW_SIZE_MAX 200000

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

#define ARROW_POSITION_TYPE_NUM (ARROW_POSITION_BOTH + 1)

enum MARKER_TYPE {
  MARKER_TYPE_NONE,
  MARKER_TYPE_ARROW,
  MARKER_TYPE_WAVE,
  MARKER_TYPE_MARK,
  MARKER_TYPE_BAR,
};

#define MARKER_TYPE_NUM (MARKER_TYPE_BAR + 1)

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

#define JOIN_TYPE_NUM (JOIN_TYPE_BEVEL + 1)

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
#define DEFAULT_LINE_WIDTH   40
#define DEFAULT_MARK_SIZE   200
#define DEFAULT_FONT_PT    2000
#define DEFAULT_SCRIPT_SIZE 7000

#define SCRIPT_SIZE_MIN 1000
#define SCRIPT_SIZE_MAX 100000

extern char *pathchar[];
extern char *joinchar[];
extern char *fontchar[];
extern char *intpchar[];
extern char *arrowchar[];
extern char *marker_type_char[];

int pathsave(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv);
int clear_bbox(struct objlist *obj, N_VALUE *inst);
int put_hsb_color(struct objlist *obj, N_VALUE *inst, int argc, char **argv, char *format);
int put_fill_hsb(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int put_stroke_hsb(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int put_hsb(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int put_hsb2(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int curve_expand_points(int *pdata, int num, int intp, struct narray *expand_points);
void text_get_bbox(int x, int y, char *text, char *font, int style, int pt, int dir, int space, int scriptsize, int raw, int *bbox);

#endif

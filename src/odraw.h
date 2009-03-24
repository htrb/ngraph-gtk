/* 
 * $Id: odraw.h,v 1.3 2009/03/24 08:33:31 hito Exp $
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

#define INTERPOLATION_TYPE_NUM (INTERPOLATION_TYPE_BSPLINE_CLOSE + 1)

extern char *pathchar[];
extern char *capchar[];
extern char *joinchar[];
extern char *fillchar[];
extern char *fontchar[];
extern char *jfontchar[];
extern char *intpchar[];
extern char *arrowchar[];

int pathsave(struct objlist *obj,char *inst,char *rval,
             int argc,char **argv);

#endif

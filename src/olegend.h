/* 
 * $Id: olegend.h,v 1.5 2009/04/19 06:46:13 hito Exp $
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

int legendgeometry(struct objlist *obj,char *inst,char *rval, int argc,char **argv);
int legendbbox(struct objlist *obj,char *inst,char *rval, int argc,char **argv);
int legendmove(struct objlist *obj,char *inst,char *rval, int argc,char **argv);
int legendrotate(struct objlist *obj,char *inst,char *rval,int argc,char **argv);
int legendchange(struct objlist *obj,char *inst,char *rval, int argc,char **argv);
int legendzoom(struct objlist *obj,char *inst,char *rval, int argc,char **argv);
int legendmatch(struct objlist *obj,char *inst,char *rval,int argc,char **argv);
void rotate(int px, int py, int angle, int *x, int *y);


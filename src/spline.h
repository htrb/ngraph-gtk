/*
 * $Id: spline.h,v 1.1.1.1 2008-05-29 09:37:33 hito Exp $
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

#define SPLCNDAUTO     0
#define SPLCND1STDIF   1
#define SPLCND2NDDIF   2
#define SPLCNDPERIODIC 3

int spline(double x[],double y[],double c1[],double c2[],double c3[],
           int num,int bc0,int bc1,double df0,double df1);
void bspline(int edge,double x[],double c[]);
void splinedif(double d,double c[],
               double *dx,double *dy,double *ddx,double *ddy,void *local);
void bsplinedif(double d,double c[],
                double *dx,double *dy,double *ddx,double *ddy,void *local);
void splineint(double d,double c[],double x0,double y0,double *x,double *y,
               void *local);
void bsplineint(double d,double c[],double x0,double y0,double *x,double *y,
               void *local);

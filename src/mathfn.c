/*
 * $Id: mathfn.c,v 1.9 2010-03-04 08:30:16 hito Exp $
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

#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include <float.h>
#include <glib.h>
#include "mathfn.h"

double
distance(double x1, double y1)
{
  return sqrt(x1 * x1 + y1 * y1);
}

int
compare_double(double x, double y)
{
  if (x) {
    return fabs(x - y) <= fabs(DBL_EPSILON * x);
  } else {
    return fabs(x - y) <= fabs(DBL_EPSILON * y);
  }
}

double
cutdown(double x)
{
  if (x>=0) return (int )x;
  else if ((x-(int )x)==0.0) return (int )x;
  else return (int )(x-1);
}

double
nraise(double x)
{
  if (x<0) return (int ) (x - N_EPSILON);
  else if ((x-(int )x)==0.0) return (int )x;
  else return (int )(x+1);
}

int
nround(double x)
{
  if (x>G_MAXINT) return G_MAXINT;
  else if (x<G_MININT) return G_MININT;
  else {
    int ix;
    double dx;
    ix=(int )x;
    dx=x-ix;
    if (dx>=0.5) return ix+1;
    else if (dx<=-0.5) return ix-1;
    else return ix;
  }
}

static int
matinv(int dim,matrix m,matrix mi)
{
  int i,j,k;
  int pivot;
  vector vec;

  for (i=0;i<dim;i++)
    for (j=0;j<dim;j++)
      if (i==j) mi[i][j]=1;
      else mi[i][j]=0;
  for (i=0;i<dim;i++) {
    double max;
    max=0;
    for (j=i;j<dim;j++) {
      if (fabs(m[j][i])>max) {
        pivot=j;
        max=fabs(m[j][i]);
      }
    }
    if (max<=1E-300) return -1;
    for (k=0;k<dim;k++) {
      vec[k]=m[pivot][k];
      m[pivot][k]=m[i][k];
      m[i][k]=vec[k];
      vec[k]=mi[pivot][k];
      mi[pivot][k]=mi[i][k];
      mi[i][k]=vec[k];
    }
    for (j=dim-1;j>=0;j--) mi[i][j]=mi[i][j]/m[i][i];
    for (j=dim-1;j>=i;j--) m[i][j]=m[i][j]/m[i][i];
    for (j=0;j<dim;j++)
     if (j!=i)
       for (k=dim-1;k>=0;k--) mi[j][k]=mi[j][k]-m[j][i]*mi[i][k];
    for (j=0;j<dim;j++)
     if (j!=i)
       for (k=dim-1;k>=0;k--) m[j][k]=m[j][k]-m[j][i]*m[i][k];
  }
  return 0;
}

int
matsolv(int dim,matrix a,vector b,vector x)
{
  matrix ai;
  int i,j;

  if (matinv(dim,a,ai)) return -1;
  for (i=0;i<dim;i++) {
    double d;
    d=0;
    for (j=0;j<dim;j++) d+=ai[i][j]*b[j];
    x[i]=d;
  }
  return 0;
}


void
HSB2RGB(double h, double s, double b, int *R, int *G, int *B)
{
  double th, s3;

  if (b == 0) {
    *R = *G = *B = 0;
    return;
  }
  s = 1 - s;
  s3 = sqrt(3.0);
  if (1.0 / 6.0 >= h) {
    th = tan(2 * MPI * h);
    *R = b * 255;
    *B = b * s * 255;
    *G = b * (2 * th - s * (th - s3)) / (th + s3) * 255;
  } else if (2.0 / 6.0 >= h) {
    th = tan(2 * MPI * (h - 1.0 / 3.0));
    *G = b * 255;
    *B = b * s * 255;
    *R = b * (2 * th - s * (th + s3)) / (th - s3) * 255;
  } else if (3.0 / 6.0 >= h) {
    th = tan(2 * MPI * (h - 1.0 / 3.0));
    *G = b * 255;
    *R = b * s * 255;
    *B = b * (2 * th - s * (th - s3)) / (th + s3) * 255;
  } else if (4.0 / 6.0 >= h) {
    th = tan(2 * MPI * (h - 2.0 / 3.0));
    *B = b * 255;
    *R = b * s * 255;
    *G = b * (2 * th - s * (th + s3)) / (th - s3) * 255;
  } else if (5.0 / 6.0 >= h) {
    th = tan(2 * MPI * (h - 2.0 / 3.0));
    *B = b * 255;
    *G = b * s * 255;
    *R = b * (2 * th - s * (th - s3)) / (th + s3) * 255;
  } else {
    th = tan(2 * MPI * h);
    *R = b * 255;
    *G = b * s * 255;
    *B = b * (2 * th - s * (th + s3)) / (th - s3) * 255;
  }
}

int
bsearch_int(const int *ary, int n, int val, int *idx)
{
  int min, max;

  if (n == 0 || ary == NULL) {
    if (idx) {
      *idx = 0;
    }
    return FALSE;
  }

  min = 0;
  max = n - 1;

  while (1) {
    int i;
    i = (min + max) / 2;

    if (ary[i] == val) {
      if (idx) {
	*idx = i;
      }
      return TRUE;
    }

    if (min >= max) {
      if (idx) {
	if (ary[i] < val) {
	  i++;
	}
	*idx = i;
      }
      return FALSE;
    }

    if (ary[i] < val) {
      min = i + 1;
    } else {
      max = i - 1;
    }
  }

  return FALSE;
}

/*
 * $Id: mathfn.h,v 1.6 2009-12-04 16:11:20 hito Exp $
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

#include "ofit.h"

#define MPI 3.1415926535897932385
#define MEXP1 2.71828182845905
#define MEULER 0.57721566490153286
#define N_EPSILON 1E-12

typedef double vector[FIT_DIMENSION_MAX + 1];
typedef vector matrix[FIT_DIMENSION_MAX + 1];

double cutdown(double x);
double nraise(double x);
double frand(double a);
int nround(double x);
int matsolv(int dim,matrix a,vector b,vector x);
void HSB2RGB(double h,double s,double b,int *R,int *G,int *B);
int bsearch_int(const int *ary, int n, int val, int *idx);
int compare_double(double x, double y);
double distance(double x1, double y1);

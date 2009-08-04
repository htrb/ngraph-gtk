/* 
 * $Id: mathfn.h,v 1.4 2009/08/04 10:41:47 hito Exp $
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

#define MPI 3.1415926535897932385
#define MEXP1 2.71828182845905
#define MEULER 0.57721566490153286

typedef double vector[11];
typedef vector matrix[11];

double cutdown(double x);
double nraise(double x);
double frand(double a);
int nround(double x);
int matinv(int dim,matrix m,matrix mi);
int matsolv(int dim,matrix a,vector b,vector x);
int gamma2(double x,double *val);
int exp1(double x,double *val);
int icgamma(double mu,double x,double *val);
int erfc1(double x,double *val);
int qinv1(double x,double *val);
int beta(double p,double q,double *val);
int jbessel(int n,double x,double *val);
int ybessel(int n,double x,double *val);
int legendre(int n,double x,double *val);
int laguer(int n,double alp,double x,double *val);
int hermite(int n,double x,double *val);
int chebyshev(int n,double x,double *val);
int mjd(int year,int month,int day);
void HSB2RGB(double h,double s,double b,int *R,int *G,int *B);
int bsearch_int(int *ary, int n, int val, int *idx);
int compare_double(double x, double y);


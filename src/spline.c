/*
 * $Id: spline.c,v 1.4 2010-03-04 08:30:16 hito Exp $
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

#include "mathfn.h"
#include "spline.h"

static int
splinecheck(const double d[], const double mu[], const double ram[], const double x[], const double y[], int num)
{
  double h1;
  int i;

  h1=x[1]-x[0];
  if (h1==0) return -1;
  for (i=1;i<num-1;i++) {
    double h2;
    h2=x[i+1]-x[i];
    if ((h2==0) || (h1==0) || (h1+h2==0)) return -1;
    h1=h2;
  }
  return 0;
}

static void
splinesetval(double d[], double mu[], double ram[], const double x[], const double y[], int num)
{
  double h1,y1;
  int i;

  h1=x[1]-x[0];
  y1=y[1]-y[0];
  for (i=1;i<num-1;i++) {
    double h2, y2;
    h2=x[i+1]-x[i];
    y2=y[i+1]-y[i];
    ram[i]=h2/(h1+h2);
    mu[i]=1-ram[i];
    d[i]=6/(h1+h2)*(y2/h2-y1/h1);
    h1=h2;
    y1=y2;
  }
  mu[0]=0;
  ram[num-1]=0;
}

static int
splinesolv(double d[], double mu[], const double ram[], double x[], double y[], int num, double *xx)
{
  int i;

  *xx=-mu[0]*0.5;
  d[0]=d[0]*0.5;
  mu[0]=ram[0]*0.5;
  for (i=1;i<num;i++) {
    double h;
    h=2-mu[i]*mu[i-1];
    if (h==0) return -1;
    *xx=-mu[i]*(*xx)/h;
    d[i]=(d[i]-mu[i]*d[i-1])/h;
    mu[i]=ram[i]/h;
  }
  *xx-=mu[num-1];
  return 0;
}

static int
splinesolv2(double d[], const double mu[], double ram[], double x[], double y[], int num, double *xx)
{
  int i;

  *xx=-ram[num-1]*0.5;
  d[num-1]=d[num-1]*0.5;
  ram[num-1]=mu[num-1]*0.5;
  for (i=num-2;i>=0;i--) {
    double h;
    h=2-ram[i]*ram[i+1];
    if (h==0) return -1;
    *xx=-ram[i]*(*xx)/h;
    d[i]=(d[i]-ram[i]*d[i+1])/h;
    ram[i]=mu[i]/h;
  }
  *xx-=ram[0];
  return 0;
}

int
splineperiod(double d[],double mu[],double ram[],double x[],double y[],
	     int num,double *df0)
{
  double h,xx,a,b,c,e;

  splinesetval(d,mu,ram,x,y,num);
  h=x[1]-x[0];
  d[0]=6/h/h*(y[1]-y[0]);
  ram[0]=1;
  mu[0]=6/h;
  h=x[num-1]-x[num-2];
  d[num-1]=-6/h/h*(y[num-1]-y[num-2]);
  mu[num-1]=1;
  ram[num-1]=-6/h;
  if (splinesolv(d,mu,ram,x,y,num,&xx)!=0) return -1;
  a=d[num-1];
  b=xx;
  splinesetval(d,mu,ram,x,y,num);
  h=x[1]-x[0];
  d[0]=6/h/h*(y[1]-y[0]);
  ram[0]=1;
  mu[0]=6/h;
  h=x[num-1]-x[num-2];
  d[num-1]=-6/h/h*(y[num-1]-y[num-2]);
  mu[num-1]=1;
  ram[num-1]=-6/h;
  if (splinesolv2(d,mu,ram,x,y,num,&xx)) return -1;
  c=d[0];
  e=xx;
  h=e-b;
  if (h==0) return -1;
  *df0=(a*e-b*c)/h;
  return 0;
}

static int
splineboundary(double d[],double mu[],double ram[],double x[],double y[],
	       int num,int bc0,int bc1,double df0,double df1)
{
  vector b,coe;
  matrix m;
  double h;
  int i,j;

  if (bc0==SPLCNDAUTO) {
    for (i=0;i<4;i++) {
      m[i][3]=1;
      b[i]=y[i];
      for (j=2;j>=0;j--) m[i][j]=m[i][j+1]*x[i];
    }
    if (matsolv(4,m,b,coe)!=0) return -1;
    df0=3*coe[0]*m[0][1]+2*coe[1]*m[0][2]+coe[2];
  }
  if (bc1==SPLCNDAUTO) {
    for (i=0;i<4;i++) {
      m[i][3]=1;
      b[i]=y[num+i-4];
      for (j=2;j>=0;j--) m[i][j]=m[i][j+1]*x[num+i-4];
    }
    if (matsolv(4,m,b,coe)!=0) return -1;
    df1=3*coe[0]*m[3][1]+2*coe[1]*m[3][2]+coe[2];
  }
  if (bc0==SPLCNDPERIODIC) {
    if (splineperiod(d,mu,ram,x,y,num,&df0)!=0) return -1;
    bc0=SPLCND2NDDIF;
    bc1=SPLCND2NDDIF;
    df1=df0;
  }
  if (bc0==SPLCND2NDDIF) {
    d[0]=2*df0;
    ram[0]=0;
  } else if ((bc0==SPLCNDAUTO) || (bc0==SPLCND1STDIF)) {
    h=x[1]-x[0];
    d[0]=6/h*((y[1]-y[0])/h-df0);
    ram[0]=1;
  } else return -1;
  if (bc1==2) {
    d[num-1]=2*df1;
    mu[num-1]=0;
  } else if ((bc1==0) || (bc1==1)) {
    h=x[num-1]-x[num-2];
    d[num-1]=6/h*(df1-(y[num-1]-y[num-2])/h);
    mu[num-1]=1;
  } else return -1;
  return 0;
}

int
spline(double x[],double y[],double c1[],double c2[],double c3[],
           int num,int bc0,int bc1,double df0,double df1)
{
  int i;
  double xx;

  if (splinecheck(c1,c2,c3,x,y,num)!=0) return -1;
  if (splineboundary(c1,c2,c3,x,y,num,bc0,bc1,df0,df1)!=0) return -1;
  splinesetval(c1,c2,c3,x,y,num);
  if (splinesolv(c1,c2,c3,x,y,num,&xx)!=0) return -1;

  c3[num-1]=c1[num-1];
  for (i=num-2;i>=0;i--) c3[i]=c1[i]-c2[i]*c3[i+1];
  for (i=0;i<num-1;i++) {
    double h, m;
     h=x[i+1]-x[i];
     m=c3[i];
     c3[i]=(c3[i+1]-m)/6/h;
     c2[i]=m*0.5;
     c1[i]=(y[i+1]-y[i])/h-h*(c2[i]+h*c3[i]);
  }
  return 0;
}

void
bspline(int edge, const double x[], double c[])
{
  if (edge==0) {
      c[0]=(x[0]+4*x[1]+x[2])/6;
      c[1]=(-3*x[0]+3*x[2])/6;
      c[2]=(3*x[0]-6*x[1]+3*x[2])/6;
      c[3]=(-x[0]+3*x[1]-3*x[2]+x[3])/6;
  } else if (edge==1) {
      c[0]=(12*x[0])/12;
      c[1]=(-36*x[0]+36*x[1])/12;
      c[2]=(36*x[0]-54*x[1]+18*x[2])/12;
      c[3]=(-12*x[0]+21*x[1]-11*x[2]+2*x[3])/12;
  } else if (edge==2) {
      c[0]=(3*x[0]+7*x[1]+2*x[2])/12;
      c[1]=(-9*x[0]+3*x[1]+6*x[2])/12;
      c[2]=(9*x[0]-15*x[1]+6*x[2])/12;
      c[3]=(-3*x[0]+7*x[1]-6*x[2]+2*x[3])/12;
  } else if (edge==3) {
      c[0]=(2*x[0]+8*x[1]+2*x[2])/12;
      c[1]=(-6*x[0]+6*x[2])/12;
      c[2]=(6*x[0]-12*x[1]+6*x[2])/12;
      c[3]=(-2*x[0]+6*x[1]-7*x[2]+3*x[3])/12;
  } else if (edge==4) {
      c[0]=(2*x[0]+7*x[1]+3*x[2])/12;
      c[1]=(-6*x[0]-3*x[1]+9*x[2])/12;
      c[2]=(6*x[0]-15*x[1]+9*x[2])/12;
      c[3]=(-2*x[0]+11*x[1]-21*x[2]+12*x[3])/12;
  }
}

void
splinedif(double d, const double c[],
               double *dx,double *dy,double *ddx,double *ddy,void *local)
{
  *dx=2*c[1]+6*d*c[2];
  *dy=2*c[4]+6*d*c[5];
  *ddx=6*c[2];
  *ddy=6*c[5];
}

#ifdef COMPILE_UNUSED_FUNCTIONS
void
splinedifxy(double d, const cdouble c[],
                 double *dx,double *dy,double *ddx,double *ddy,void *local)
{
  *dx=0;
  *dy=(2*c[1]+6*c[3]*d*c[2])*c[3]*c[3];
  *ddx=0;
  *ddy=6*c[3]*c[3]*c[3]*c[2];
}
#endif  /* COMPILE_UNUSED_FUNCTIONS */

void
bsplinedif(double d, const double c[],
                double *dx,double *dy,double *ddx,double *ddy,void *local)
{
  *dx=2*c[2]+6*d*c[3];
  *dy=2*c[6]+6*d*c[7];
  *ddx=6*c[3];
  *ddy=6*c[7];
}

void
splineint(double d, const double c[], double x0, double y0, double *x, double *y,
               void *local)
{
  *x=x0+d*(c[0]+d*(c[1]+d*c[2]));
  *y=y0+d*(c[3]+d*(c[4]+d*c[5]));
}

#ifdef COMPILE_UNUSED_FUNCTIONS
void
splineintxy(double d,const double c[],double x0,double y0,double *x,double *y,
                 void *local)
{
  double dd;

  dd=c[3]*d;
  *x=x0+dd;
  *y=y0+dd*(c[0]+dd*(c[1]+dd*c[2]));
}
#endif  /* COMPILE_UNUSED_FUNCTIONS */

void
bsplineint(double d, const double c[], double x0, double y0, double *x, double *y,
                void *local)
{
  *x=c[0]+d*(c[1]+d*(c[2]+d*c[3]));
  *y=c[4]+d*(c[5]+d*(c[6]+d*c[7]));
}

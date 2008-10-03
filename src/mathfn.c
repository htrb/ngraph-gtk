/* 
 * $Id: mathfn.c,v 1.2 2008/10/03 03:53:53 hito Exp $
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

#ifdef HAVE_CONFIG_H
#include "config.h"
#endif

#include <stdlib.h>
#include <limits.h>
#include <math.h>
#include "mathfn.h"

#define TRUE 1
#define FALSE 0

int randinit=FALSE;
/*
int randm[521];
int randj;
*/

double cutdown(double x)
{
  if (x>=0) return (int )x;
  else if ((x-(int )x)==0.0) return (int )x;
  else return (int )(x-1);
}

double nraise(double x)
{
  if (x<0) return (int )x;
  else if ((x-(int )x)==0.0) return (int )x;
  else return (int )(x+1);
}

int mjd(int year,int month,int day)
{
  int d,d1,d2,d3,d4;

  d=(14-month)/12;
  d1=(year-d)*365.25;
  d2=(month+d*12-2)*30.59;
  d3=(year-d)/100;
  d4=(year-d)/400;
  return d1+d2-d3+d4+day+1721088-2400000;
}

int nround(double x)
{
  int ix;
  double dx;

  if (x>LONG_MAX) return LONG_MAX;
  else if (x<LONG_MIN) return LONG_MIN;
  else {
    ix=(int )x;
    dx=x-ix;
    if (dx>=0.5) return ix+1;
    else if (dx<=-0.5) return ix-1;
    else return ix;
  }
}

/*
void randomize(int seed)
{
  int i,ih,ii,ij,mj;
  int ia[521];
  int j;

  if (seed==0) seed=1;
  for (i=0;i<521;i++) {
    seed*=69069;
    ia[i]=(seed>=0) ? 1:-1;
  }
  for (j=0;j<521;j++) {
    ih=(j*32)%521;
    mj=0;
    for (i=0;i<31;i++) {
      ii=(ih+i)%521;
      mj=2*mj-(ia[ii]-1)/2;
      ij=(ii+489)%521;
      ia[ii]*=ia[ij];
    }
    randm[j]=mj;
    ii=(ih+31)%521;
    ij=(ii+489)%521;
    ia[ii]*=ia[ij];
  }
  randj=0;
}*/

double frand(double a)
{
  if (!randinit) {
    randinit=TRUE;
    srand(1);
  }
  return (rand()/((double )RAND_MAX+1))*a;
/*
  if (!randinit) {
    randinit=TRUE;
    randomize(1);
  }
  randj++;
  if (randj==521) randj=0;
  k=randj-32;
  if (k<0) k+=521;
  randm[randj]=randm[randj]^randm[k];
  return *((float *)(randm+randj))*0.4656613E-9*a;
*/
}

int matinv(int dim,matrix m,matrix mi)
{
  int i,j,k;
  double max;
  int pivot;
  vector vec;

  for (i=0;i<dim;i++)
    for (j=0;j<dim;j++)
      if (i==j) mi[i][j]=1;
      else mi[i][j]=0;
  for (i=0;i<dim;i++) {
    max=0;
    for (j=i;j<dim;j++) 
      if (fabs(m[j][i])>max) {
        pivot=j;
        max=fabs(m[j][i]);
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

int matsolv(int dim,matrix a,vector b,vector x)
{
  matrix ai;
  int i,j;
  double d;

  if (matinv(dim,a,ai)) return -1;
  for (i=0;i<dim;i++) {
    d=0;
    for (j=0;j<dim;j++) d+=ai[i][j]*b[j];
    x[i]=d;
  }
  return 0;
}

int gamma2(double x,double *val)
{
  double m,s,x1,xx;
  int i;
  static double gammacoe[19]=
               {-0.42278433509846714,
                -0.23309373642178674,
                 0.19109110138769151,
                -0.24552490005400017E-1,
                -0.17645244550144320E-1,
                 0.80232730222673465E-2,
                -0.80432977560424699E-3,
                -0.3608378162548E-3,
                 0.1455961421399E-3,
                -0.175458597517E-4,
                -0.25889950224E-5,
                 0.13385015466E-5,
                -0.2054743152E-6,
                -0.1595268E-9,
                 0.62756218E-8,
                -0.12736143E-8,
                 0.923397E-10,
                 0.120028E-10,
                -0.42202E-11};

  if (((x<=0) && ((x-(int )x)==0)) || (x>57)) return -1;
  m=1;
  if (1.5<x)
    while (1.5<x) {
      x-=1;
      m*=x;
    }
  else if (x<0.5)
    while (x<0.5) {
      if (x<=1E-300) return -1;
      m/=x;
      x++;
    }
  s=1;
  xx=1;
  x1=x-1;
  for (i=0;i<19;i++) {
    xx*=x1;
    s+=xx*gammacoe[i];
  }
  s*=x;
  if (s==0) return -1;
  *val=m/s;
  return 0;
}

int exp1(double x,double *val)
{
  double x2,xx,xx2,qexp1,qexp2;
  int i,n;
  static double xo[18]=
          {3.3,4.0,4.8,5.8,7.1,8.6,10.4,12.5,15.2,18.4,22.2,
           26.9,32.5,39.4,47.6,57.6,69.7,83.0};
  static double eipt[18]=
          {44.85377247567375, 35.95520078636207,
           28.55548567959244, 22.28304130069319,
           17.15083762120765, 13.52650411391756,
           10.81282973538746,  8.781902021178544,
            7.084719123528884, 5.769115302038587,
            4.728746710237675, 3.867300609441438,
            3.178040354772841, 2.606037129493851,
            2.146957943513479, 1.767357146269297,
            1.455922099180754, 1.219698244176420};
  static double eimt[18]=
          {24.23610327385172,  20.63456499010558,
           17.65538999222755,  14.96789725060241,
           12.50401823871386,  10.51365383249826,
            8.830924437781962,  7.443527518977505,
            6.194083826443568,  5.167187241655524,
            4.317774043309917,  3.588549395492134,
            2.987594411636582,  2.476696474779806,
            2.058451487317135,  1.706965828524471,
            1.414702600059552,  1.190641095161813};

  x2=fabs(x);
  if ((x==0) || (x2>174)) return -1;
  if (x2>=90) {
    n=1;
    xx=1;
    qexp2=1;
    do {
      xx*=n/x;
      n++;
      qexp1=qexp2;
      qexp2=qexp1+xx;
    } while (qexp1!=qexp2);
    qexp1/=x;
    if (x<0) qexp1=-qexp1*exp(-x2);
    else qexp1*=exp(x2);
  } else if (x2>=3) {
    i=0;
    while ((i<18) && (x2>=0.5*(xo[i]+xo[i+1]))) i++;
    if (x>0) xx=eipt[i]/100;
    else xx=eimt[i]/100;
    xx2=1/(-xo[i]);
    qexp2=xx;
    n=1;
    do {
      qexp1=qexp2;
      xx=(xo[i]-x2)/n*(xx+xx2);
      if (x<0) xx=-xx;
      qexp2=qexp1+xx;
      xx2*=(x2-xo[i])/(-xo[i]);
      n++;
    } while (qexp2!=qexp1);
    if (x<0) qexp1=-qexp1*exp(-x2);
    else qexp1*=exp(x2);
  } else {
    n=1;
    xx=1;
    qexp2=0;
    do {
      xx*=x/n;
      qexp1=qexp2;
      qexp2=qexp1+xx/n;
      n++;
    } while (qexp1!=qexp2);
    qexp1+=MEULER+log(x2);
  }
  *val=qexp1;
  return 0;
}

int icgamma(double mu,double x,double *val)
{
  double a,u,p0,p1,q0,q1;
  int i,i2,i3;

  if ((x<0) || (mu<0)) return -1;
  if (mu==0) {
    if (exp1(-x,val)) return -1;
    else *val=-*val;
  } else if (x==0) {
    if (gamma2(mu,val)) return -1;
  } else {
    i2=0;
    while (mu>1) {
      i=0;
      do {
        if (i==56) return -1;
        i++;
        mu--;
      } while (mu>1);
      i2=i;
    }
    u=exp(-x+mu*log(x));
    if (x<=1) {
      if (gamma2(mu,&p1)) return -1;
      p0=1;
      q0=1;
      i=0;
      do {
        i++;
        q0*=x/(mu+i);
        p0+=q0;
      } while ((q0>1E-17) && (i!=20));
      *val=p1-u/mu*p0;
    } else {
      i3=(int )(120/x+5);
      p0=0;
      if (mu!=1) p1=1E-78/(1-mu);
      else p1=1E-78;
      q0=p1;
      q1=x*q0;
      for (i=1;i<=i3;i++) {
        a=i-mu;
        p0=p1+a*p0;
        q0=q1+a*q0;
        p1=x*p0+i*p1;
        q1=x*q0+i*q1;
        if (i>=50) {
          q0/=i;
          p0/=i;
          q1/=i;
          p1/=i;
        }
      }
      *val=u/q1*p1;
    }
    for (i=1;i<i2;i++) {
      *val=u+mu*(*val);
      u*=x;
      mu++;
    }
  }
  return 0;
}

int erfc1(double x,double *val)
{
  int i,i2,sg;
  double x2,x3,sum,h,h2;

  x2=fabs(x);
  if (x2<=0.1) {
    sum=0;
    sg=1;
    x3=x2;
    for (i=0;i<=5;i++) {
      if (i==0) i2=1;
      else i2*=i;
      sum+=sg*x3/i2/(2*i+1);
      sg*=-1;
      x3*=x2*x2;
    }
    sum*=2/sqrt(MPI);
    *val=1-sum;
  } else if (x2<=100) {
    h=0.5;
    h2=h*h;
    sum=0;
    for (i=1;i<=13;i++) {
      i2=i*i;
      sum+=exp(-i2*h2)/(i2*h2+x2*x2);
    }
    if (x2<6) {
      sum+=0.5/(x2*x2);
      sum*=2*x2/MPI*exp(-x2*x2)*h;
      sum-=2/(exp(2*MPI*x2/h)-1);
    } else {
      sum+=0.5/(x2*x2);
      sum*=2*x2/MPI*exp(-x2*x2)*h;
    }
    *val=sum;
  } else {
    sum=1;
    sg=-1;
    x3=2*x2*x2;
    i2=1;
    for (i=1;i<=5;i++) {
      i2*=(2*i-1);
      sum+=sg*i2/x3;
      sg*=-1;
      x3*=2*x2*x2;
    }
    sum*=exp(-x2*x2)/sqrt(MPI)/x2;
    *val=sum;
  }
  if (x<0) *val=2-(*val);
  return 0;
}

double qinv3(double y)
{
  double n,sum,y2,y3,i;

  sum=0;
  y2=y;
  y3=y*y;
  i=1;
  n=0;
  do {
    i*=(2*n+1);
    sum+=y2/i;
    n=n+1;
    y2*=y3;
  } while (fabs(y2/i)>1e-16);
  return sum;
}

double qinv2(double y)
{
  int n,i;
  double a;

  if (y<7.5) n=(int )(110/(y-1));
  else if (y<12.5) n=(int )(-1.6*y+30);
  else n=10;
  a=0;
  for (i=n;i>0;i--) a=i/(y+a);
  return 1/(y+a);
}

int qinv1(double x,double *val)
{
  double x2,m2,c0,c1,y0,y1;

  if (x<=0) return -1;
  if (x>=1) return -1;
  if (x>0.5) x2=1-x;
  else x2=x;
  m2=sqrt(2*MPI);
  if (x2<=0.01) {
    c0=sqrt(-2*log(m2*3*x2));
    c1=c0+1/c0;
    y1=sqrt(-2*log(m2*c1*x2));
    do {
     y0=y1;
     y1=y0+(qinv2(y0)-m2*x2*exp(0.5*y0*y0));
    } while (fabs(y0-y1)>=1e-14);
  } else if (x2<=0.5) {
    y1=m2*(0.5-x2)*1.5;
    do {
     y0=y1;
     y1=y0-(qinv3(y0)-m2*(0.5-x2)*exp(0.5*y0*y0));
    } while (fabs(y0-y1)>=1e-14);
  }
  if (x>0.5) *val=-y1;
  else *val=y1;
  return 0;
}

int beta(double p,double q,double *val)
{
  double a,b,c;

  if (gamma2(p,&a)) return -1;
  if (gamma2(q,&b)) return -1;
  if (gamma2(p+q,&c)) return -1;
  if (fabs(c)<1E-300) return -1;
  *val=a*b/c;
  return 0;
}

int jbessel(int n,double x,double *val)
{
  int n2,m,l,i;
  double x2,t1,t2,t3,s,j;

  x2=fabs(x);
  n2=abs(n);
  if ((n2>1000) || (x2>1000)) return -1;
  if (x2<=2E-5) {
    t1=x2*0.5;
    if (n2==0) j=1-t1*t1;
    else if (x2<=1E-77) j=0;
    else {
      t2=1;
      t3=1;
      i=1;
      while ((i<=n2) && (fabs(t3)>fabs(t2/t1)*1E-77)) {
        t3*=t1/t2;
        t2++;
        i++;
      }
      if (i<=n2) j=0;
      else j=t3*(1-t1*t1/t2);
    }
  } else {
    if (n2>x2) m=n2;
    else m=(int )x2;
    if (x2>=100) l=(int )(0.073*x2+47);
    else if (x2>=10) l=(int )(0.27*x2+27);
    else if (x2>1) l=(int )(1.4*x2+14);
    else l=14;
    m+=l;
    t3=0;
    t2=1E-75;
    s=0;
    for (i=m-1;i>=0;i--) {
      t1=2*(i+1)/x2*t2-t3;
      if (i==n2) j=t1;
      if ((i%2==0) && (i!=0)) {
        s+=t1;
        if (fabs(s)>=1E55) {
          t1*=1E-55;
          t2*=1E-55;
          s*=1E-55;
          j*=1E-55;
        }
      }
      t3=t2;
      t2=t1;
    }
    s=2*s+t1;
    j/=s;
  }
  if ((n<0) && (n2%2==1)) j=-j;
  if ((x<0) && (n2%2==1)) j=-j;
  *val=j;
  return 0;
}

int ybessel(int n,double x,double *val)
{
  int n2,m,i,l;
  double x2,t1,t2,t3,s,ss,w,j0,j1,y0,y1,y2;

  x2=fabs(x);
  n2=abs(n);
  if ((n2>1000) || (x2>1000) || (x2<=1E-300)) return -1;
  if (x2<=2E-5) {
    t1=x2*0.5;
    j0=1-t1*t1;
    y0=2/MPI*((MEULER+log(t1))*j0+t1*t1);
    if (n2!=0) {
      j1=t1-0.5*t1*t1*t1;
      y1=(j1*y0-2/MPI/x2)/j0;
      if (n2>=-65/log10(t1)) return -1;
      for (i=2;i<=n2;i++) {
        y2=2*(i-1)/x2*y1-y0;
        y0=y1;
        y1=y2;
      }
    }
  } else {
    if (x2>=100) m=(int )(1.073*x2+47);
    else if (x2>=10) m=(int )(1.27*x2+28);
    else if (x2>=1) m=(int )(2.4*x2+15);
    else if (x2>=0.1) m=17;
    else m=10;
    t3=0;
    t2=1E-75;
    s=0;
    ss=0;
    for (i=m-1;i>=0;i--) {
      t1=2*(i+1)/x2*t2-t3;
      if ((i%2==0) && (i!=0)) {
        s+=t1;
        l=i/2;
        if (l%2==0) ss-=t1/l;
        else ss+=t1/l;
        if (fabs(s)>=1E55) {
          t1*=1E-55;
          t2*=1E-55;
          s*=1E-55;
          ss*=1E-55;
        }
      }
      t3=t2;
      t2=t1;
    }
    s=2*s+t1;
    j0=t1/s;
    j1=t3/s;
    ss/=s;
    y0=2/MPI*((MEULER+log(x2*0.5))*j0+2*ss);
    if (n2!=0) {
      y1=(j1*y0-2/MPI/x2)/j0;
      for (i=2;i<=n2;i++) {
        if (y1<=-1E65) {
          w=y1*1E-10;
          y2=2*(i-1)/x2*w-y0*1E-10;
          if (fabs(y2)>=1E65) return -1;
          y2*=1E10;
        } else y2=2*(i-1)/x2*y1-y0;
        y0=y1;
        y1=y2;
      }
    }
  }
  if (n==0) *val=y0;
  else {
    if ((n<0) && (n2%2==1)) y1=-y1;
    if ((x<0) && (n2%2==1)) y1=-y1;
    *val=y1;
  }
  return 0;
}

int legendre(int n,double x,double *val)
{
  double l1,l2,tmp1,tmp2;
  int i;

  if (n<0) return -1;
  l1=1;
  if (n==0) *val=l1;
  else {
    l2=x;
    if (n==1) *val=l2;
    else {
      for (i=2;i<=n;i++) {
        tmp1=x*l2;
        tmp2=tmp1-l1;
        *val=tmp2+tmp1-tmp2/i;
        l1=l2;
        l2=*val;
      }
    }
  }
  return 0;
}

int laguer(int n,double alp,double x,double *val)
{
  int i;
  double l1,l2,tmp1,tmp2;

  if (n<0) return -1;
  l1=1;
  if (n==0) *val=l1;
  else {
    l2=1-x+alp;
    if (n==1) *val=l2;
    else {
      tmp1=1+x-alp;
      tmp2=1-alp;
      for (i=2;i<=n;i++) {
        *val=(l2-l1)+l2-(tmp1*l2-tmp2*l1)/i;
        l1=l2;
        l2=*val;
      }
    }
  }
  return 0;
}

int hermite(int n,double x,double *val)
{
  double l1,l2;
  int i;

  if (n<0) return -1;
  l1=1;
  if (n==0) *val=l1;
  else {
    l2=2*x;
    if (n==1) *val=l2;
    else {
      for (i=2;i<=n;i++) {
        *val=x*l2-(i-1)*l1;
        *val+=*val;
        l1=l2;
        l2=*val;
      }
    }
  }
  return 0;
}

int chebyshev(int n,double x,double *val)
{
  double l1,l2,tmp1;
  int i;

  if (n<0) return -1;
  l1=1;
  if (n==0) *val=l1;
  else {
    l2=x;
    if (n==1) *val=l2;
    else {
      tmp1=x+x;
      for (i=2;i<=n;i++) {
        *val=tmp1*l2-l1;
        l1=l2;
        l2=*val;
      }
    }
  }
  return 0;
}

void HSB2RGB(double h,double s,double b,int *R,int *G,int *B)
{
  double th,s3;

  if (b==0) {
    *R=*G=*B=0;
    return;
  }
  s=1-s;
  s3=sqrt(3.0);
  if (1.0/6.0>=h) {
    th=tan(2*MPI*h);
    *R=b*255;
    *B=b*s*255;
    *G=b*(2*th-s*(th-s3))/(th+s3)*255;
  } else if (2.0/6.0>=h) {
    th=tan(2*MPI*(h-1.0/3.0));
    *G=b*255;
    *B=b*s*255;
    *R=b*(2*th-s*(th+s3))/(th-s3)*255;
  } else if (3.0/6.0>=h) {
    th=tan(2*MPI*(h-1.0/3.0));
    *G=b*255;
    *R=b*s*255;
    *B=b*(2*th-s*(th-s3))/(th+s3)*255;
  } else if (4.0/6.0>=h) {
    th=tan(2*MPI*(h-2.0/3.0));
    *B=b*255;
    *R=b*s*255;
    *G=b*(2*th-s*(th+s3))/(th-s3)*255;
  } else if (5.0/6.0>=h) {
    th=tan(2*MPI*(h-2.0/3.0));
    *B=b*255;
    *G=b*s*255;
    *R=b*(2*th-s*(th-s3))/(th+s3)*255;
  } else {
    th=tan(2*MPI*h);
    *R=b*255;
    *G=b*s*255;
    *B=b*(2*th-s*(th+s3))/(th-s3)*255;
  }
}

int
bsearch_int(int *ary, int n, int val)
{
  int min, max, i;

  if (n == 0 || ary == NULL)
    return -1;

  i = (n - 1) / 2;
  min = 0;
  max = n - 1;

  while (1) {
    if (val < ary[min] || val > ary[max]) {
      i = -1;
      break;
    }

    if (ary[min] == val) {
      i = min;
      break;
    } else if (ary[max] == val) {
      i = max;
      break;
    } else if (ary[i] == val) {
      break;
    }

    if (max - min < 2) {
      i = -1;
      break;
    }

    if (ary[i] < val) {
      min = i;
      i = (i + max) / 2;
    } else if (ary[i] > val) {
      max = i;
      i = (i + min ) / 2;
    }
  }

  return i;
}

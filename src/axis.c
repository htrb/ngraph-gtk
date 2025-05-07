/*
 * $Id: axis.c,v 1.7 2010-03-04 08:30:16 hito Exp $
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

#include <math.h>
#include <string.h>
#include "mathfn.h"
#include "object.h"
#include "axis.h"

#ifdef HAVE_LIBGSL
#include <gsl/gsl_math.h>
#endif

double
scale(double x)
{
  return pow(10.0,cutdown(log10(fabs(x))));
}

double
roundmin(double min,double sc)
{
  return nraise(min/sc)*sc;
}

static void
getaxispositionfirst(struct axislocal *alocal)
{
  int numlim1,numlim2,numlim3;

  if (alocal->tighten) {
    numlim1=6;
    numlim2=11;
    numlim3=21;
  } else {
    numlim1=2;
    numlim2=3;
    numlim3=6;
  }

  if ((alocal->atype==AXISNORMAL) || (alocal->atype==AXISLOGBIG)) {
    if (alocal->div<=0) {
      if (alocal->num<=numlim1) alocal->countsend=10;
      else if (alocal->num<=numlim2) alocal->countsend=5;
      else if (alocal->num<=numlim3) alocal->countsend=2;
      else alocal->countsend=1;
    } else alocal->countsend=alocal->div;
    alocal->dposl=alocal->inc*10;
    alocal->posl=roundmin(alocal->posst,alocal->dposl)-alocal->dposl;
    alocal->dposm=alocal->inc;
    alocal->posm=roundmin(alocal->posst,alocal->dposm)-alocal->dposm;
    alocal->countmend=10;
    alocal->countm=
     nround((roundmin(alocal->posst,alocal->dposm)-alocal->posl)/alocal->dposm);
    alocal->dposs=alocal->dposm/alocal->countsend;
    alocal->counts=1;
  } else if (alocal->atype==AXISINVERSE) {
    double min, max, inc;
    if (alocal->div<=0) {
      if (alocal->num<=numlim1) alocal->countsend=10;
      else if (alocal->num<=numlim2) alocal->countsend=5;
      else if (alocal->num<=numlim3) alocal->countsend=2;
      else alocal->countsend=1;
    } else alocal->countsend=alocal->div;
    if (((alocal->max>alocal->min) && (alocal->min>0))
     || ((alocal->max<alocal->min) && (alocal->min<0))) {
      max=alocal->min;
      min=alocal->max;
      inc=alocal->inc*(-1);
    } else {
      min=alocal->min;
      max=alocal->max;
      inc=alocal->inc;
    }
    alocal->dposm=inc;
    alocal->posm=roundmin(min,alocal->dposm)-alocal->dposm;
    alocal->countmend=(max-alocal->posm)/alocal->dposm;
    alocal->countm=1;
    alocal->dposl=alocal->dposm*alocal->countmend;
    alocal->posl=alocal->posm;
    alocal->dposs=alocal->dposm/alocal->countsend;
    alocal->counts=1;
  } else if (alocal->atype==AXISLOGNORM) {
    if (alocal->div==1) {
      alocal->countsend=1;
      if (alocal->inc>0) alocal->dposs=9;
      else alocal->dposs=-0.9;
    } else if (alocal->div==2) {
      alocal->countsend=2;
      if (alocal->inc>0) alocal->dposs=4;
      else alocal->dposs=-0.5;
    } else {
      alocal->countsend=9;
      if (alocal->inc>0) alocal->dposs=1;
      else alocal->dposs=-0.1;
    }
    alocal->dposl=alocal->inc*10;
    alocal->posl=roundmin(alocal->posst,alocal->dposl)-alocal->dposl;
    alocal->dposm=alocal->inc;
    alocal->posm=roundmin(alocal->posst,alocal->dposm)-alocal->dposm;
    alocal->countmend=10;
    alocal->countm=
      nround((roundmin(alocal->posst,alocal->dposm)-alocal->posl)/alocal->dposm);
    alocal->counts=1;
  } else if (alocal->atype==AXISLOGSMALL) {
    if (alocal->div<=0) {
      if (alocal->num<=numlim1) alocal->countsend=10;
      else if (alocal->num<=numlim2) alocal->countsend=5;
      else if (alocal->num<=numlim3) alocal->countsend=2;
      else alocal->countsend=1;
    } else alocal->countsend=alocal->div;
    alocal->dposl=alocal->inc;
    alocal->posl=roundmin(alocal->posst,alocal->dposl)-alocal->dposl;
    if (alocal->dposl>0) alocal->dposm=1;
    else alocal->dposm=-0.1;
    alocal->posm=alocal->posl;
    alocal->countmend=9;
    alocal->countm=1;
    alocal->dposs=1.0/alocal->countsend;
    alocal->counts=1;
  }
  alocal->count=0;
}

int
getaxisposition(struct axislocal *alocal, /*@out@*/ double *po)
{
  int rcode;

  if (alocal->counts>=alocal->countsend) {
    if (alocal->atype==AXISLOGSMALL)
      *po=alocal->posl+log10(1.0+alocal->countm*alocal->dposm);
    else *po=alocal->posm+alocal->dposm;
    alocal->posm=*po;
    alocal->counts=1;
    rcode=2;
    if (alocal->countm==alocal->countmend) {
      *po=alocal->posl+alocal->dposl;
      alocal->posl=*po;
      alocal->countm=1;
      rcode=3;
      if (alocal->atype==AXISINVERSE) {
        double dd;
        if (((alocal->dposm>=0) && (alocal->min>=0))
         || ((alocal->dposm<0) && (alocal->min<0))) dd=10;
        else dd=0.1;
        alocal->dposl*=dd;
        alocal->dposm*=dd;
        alocal->dposs*=dd;
        alocal->countmend=-1;
      }
    } else alocal->countm++;
  } else {
    rcode=1;
    if (alocal->atype==AXISLOGNORM)
      *po=alocal->posm+log10(1.0+alocal->counts*alocal->dposs);
    else if (alocal->atype==AXISLOGSMALL)
      *po=log10(pow(10.0, alocal->posm)
		+ alocal->counts * alocal->dposs * pow(10.0, alocal->posl) * alocal->dposm);
    else *po=alocal->posm+alocal->counts*alocal->dposs;
    alocal->counts++;
  }
  if (fabs(*po/alocal->dposs)<1E-14) *po=0;
  if ((*po-alocal->posst)*(*po-alocal->posed)>0
#ifdef HAVE_LIBGSL
      && gsl_fcmp(*po, alocal->posst, N_EPSILON)
      && gsl_fcmp(*po, alocal->posed, N_EPSILON)
#endif
      ) {
    if (alocal->dposm>=0) {
      if (((alocal->posst<=alocal->posed) && (*po<alocal->posst))
       || ((alocal->posst>=alocal->posed) && (*po<alocal->posed))) rcode=-1;
      else rcode=-2;
    } else {
      if (((alocal->posst<=alocal->posed) && (*po>alocal->posed))
       || ((alocal->posst>=alocal->posed) && (*po>alocal->posst))) rcode=-1;
      else rcode=-2;
    }
  }
  if ((alocal->atype==AXISINVERSE) && (rcode>=0)) *po=1 / *po;
  alocal->count++;
  if (alocal->count>=10000) rcode=-2;
  return rcode;
}

int
getaxispositionini(struct axislocal *alocal, int type,
		   double min,double max,double inc,int div,int tighten)
{
  double po;
  int rcode;
  int num;

  if (compare_double(min, max)) return -1;
  if (compare_double(inc, 0)) return -1;
  if (type==AXIS_TYPE_LOG) {
    if ((min<=0) || (max<=0)) return -1;
    if (compare_double(fabs(inc), 10)) {
      alocal->atype=AXISLOGNORM;
    } else if (compare_double(fabs(inc), 1)) {
      alocal->atype=AXISLOGSMALL;
    } else {
      alocal->atype=AXISLOGBIG;
    }
    alocal->min=log10(min);
    alocal->max=log10(max);
    alocal->inc=log10(fabs(inc));
    if (alocal->atype==AXISLOGSMALL) alocal->inc=1;
  } else if (type==AXIS_TYPE_INVERSE) {
    if (min*max<=0) return -1;
    alocal->atype=AXISINVERSE;
    alocal->min=min;
    alocal->max=max;
    alocal->inc=inc;
  } else {
    alocal->atype=AXISNORMAL;
    alocal->min=min;
    alocal->max=max;
    alocal->inc=inc;
  }
  if (alocal->max<alocal->min) alocal->inc=-fabs(alocal->inc);
  else alocal->inc=fabs(alocal->inc);
  alocal->div=div;
  alocal->posst=alocal->min;
  alocal->posed=alocal->max;
  alocal->num=100;
  alocal->tighten=tighten;
  getaxispositionfirst(alocal);
  num=0;
  while ((rcode=getaxisposition(alocal,&po))!=-2) {
    if (rcode>=2) num++;
  }
  alocal->num=num;
  if (alocal->count>1000) return -1;
  getaxispositionfirst(alocal);
  return 0;
}

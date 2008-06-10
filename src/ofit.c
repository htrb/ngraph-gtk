/* 
 * $Id: ofit.c,v 1.5 2008/06/10 04:21:33 hito Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "ngraph.h"
#include "object.h"
#include "mathcode.h"
#include "mathfn.h"
#include "oroot.h"

#define NAME "fit"
#define PARENT "object"
#define OVERSION  "1.00.01"
#define TRUE  1
#define FALSE 0

#define ERRNUM 14

#define ERRSYNTAX 100
#define ERRILLEGAL 101
#define ERRNEST   102
#define ERRMANYPARAM 103
#define ERRLN 104
#define ERRSMLDATA 105
#define ERRPOINT 106
#define ERRMATH 107
#define ERRSINGULAR 108
#define ERRNOEQS 109
#define ERRTHROUGH 110
#define ERRRANGE 111
#define ERRNEGATIVEWEIGHT 112
#define ERRCONVERGE 113

char *fiterrorlist[ERRNUM]={
  "syntax error.",
  "not allowd function.",
  "sum() or dif(): deep nest.",
  "too many parameters.",
  "illegal data -> ignored.",
  "too small number of data.",
  "illegal point",
  "math error -> ignored.",
  "singular matrix.",
  "no fitting equation.",
  "`through_point' for type `user' is not supported.",
  "math range check.",
  "negative or zero weight -> ignored.",
  "convergence error.",
};

char *fittypechar[6]={
  N_("poly"),
  N_("pow"),
  N_("exp"),
  N_("log"),
  N_("user"),
  NULL
};

struct fitlocal {
  int id, oid;
  char *codef;
  struct narray *needdata;
  char *codedf[10];
  int dim;
  double coe[11];
  int num;
  double derror,correlation;
  char *equation;
};

int fitinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  int div,dimension;
  double converge;
  struct fitlocal *fitlocal;
  int i,disp, oid;

  if (_getobj(obj, "oid", inst, &oid) == -1) return 1;
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  div=500;
  dimension=1;
  converge=1;
  disp=TRUE;
  if (_putobj(obj,"div",inst,&div)) return 1;
  if (_putobj(obj,"poly_dimension",inst,&dimension)) return 1;
  if (_putobj(obj,"converge",inst,&converge)) return 1;
  if (_putobj(obj,"display",inst,&disp)) return 1;


  if ((fitlocal=memalloc(sizeof(struct fitlocal)))==NULL) return 1;
  fitlocal->codef=NULL;
  for (i=0;i<10;i++) fitlocal->codedf[i]=NULL;
  fitlocal->needdata=NULL;
  fitlocal->equation=NULL;
  fitlocal->oid = oid;
  if (_putobj(obj,"_local",inst,fitlocal)) {
    memfree(fitlocal);
    return 1;
  }
  return 0;
}

int fitdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct fitlocal *fitlocal;
  int i;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"_local",inst,&fitlocal);
  memfree(fitlocal->codef);
  memfree(fitlocal->equation);
  arrayfree(fitlocal->needdata);
  for (i=0;i<10;i++) memfree(fitlocal->codedf[i]);
  return 0;
}

int fitput(struct objlist *obj,char *inst,char *rval,
           int argc,char **argv)
{
  char *field;
  int rcode,ecode,maxdim,need2pass;
  struct narray *needdata;
  char *math,*code;
  struct fitlocal *fitlocal;
  char *equation;

  _getobj(obj,"_local",inst,&fitlocal);
  field=argv[1];
  if (strcmp(field,"poly_dimension")==0) {
    if (*(int *)(argv[2])<1) *(int *)(argv[2])=1;
    else if (*(int *)(argv[2])>9) *(int *)(argv[2])=9;
  } else if ((strcmp(field,"user_func")==0)
          || ((strncmp(field,"derivative",10)==0) && (field[10]!='\0'))) {
    math=(char *)(argv[2]);
    if (math!=NULL) {
      needdata=arraynew(sizeof(int));
      rcode=mathcode(math,&code,needdata,NULL,&maxdim,&need2pass,
                     TRUE,FALSE,FALSE,TRUE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE,FALSE);
      if (field[0]!='u') {
        arrayfree(needdata);
      }
      if (rcode!=MCNOERR) {
        if (rcode==MCSYNTAX) ecode=ERRSYNTAX;
        else if (rcode==MCILLEGAL) ecode=ERRILLEGAL;
        else if (rcode==MCNEST) ecode=ERRNEST;
        error(obj,ecode);
        return 1;
      }
      if (maxdim>9) {
        error(obj,ERRMANYPARAM);
        return 1;
      }
    } else {
      code=NULL;
      needdata=NULL;
    }
    if (field[0]=='u') {
      memfree(fitlocal->codef);
      fitlocal->codef=code;
      arrayfree(fitlocal->needdata);
      fitlocal->needdata=needdata;
    } else {
      memfree(fitlocal->codedf[field[10]-'0']);
      fitlocal->codedf[field[10]-'0']=code;
    }
  }
  _getobj(obj,"equation",inst,&equation);
  if (_putobj(obj,"equation",inst,NULL)) return 1;
  memfree(equation);
  return 0;
}

int fitpoly(struct fitlocal *fitlocal,
            int type,int dimension,int through,double x0,double y0,
            double *data,int num,int disp,int weight,double *wdata)
/*
  return
         -5: range check
         -2: singular matrix
         -1: too small data
          0: no error
          1: fatail error
*/
{
  int i,j,k,dim;
  double yy,y1,y2,derror,sy,correlation,wt,sum;
  vector b,x1,x2,coe;
  matrix m;
  char *equation;
  char buf[1024];

  if (type==0) dim=dimension+1;
  else dim=2;
  if (!through && (num<dim)) return -1;
  if (through && (num<dim+1)) return -1;
  yy=0;
  for (i=0;i<dim+1;i++) {
    b[i]=0;
    for (j=0;j<dim+1;j++) m[i][j]=0;
  }
  sum=0;
  for (k=0;k<num;k++) {
    if (weight) wt=wdata[k];
    else wt=1;
    sum+=wt;
    y1=data[k*2+1];
    x1[0]=1;
    for (i=1;i<dim;i++) {
      if ((fabs(x1[i-1])>1e100) || (fabs(data[k*2])>1e100)) return -5;
      x1[i]=x1[i-1]*data[k*2];
      if (fabs(x1[i])>1e100) return -5;
    }
    yy+=wt*y1*y1;
    if (fabs(yy)>1e100) return -5;
    for (i=0;i<dim;i++) {
      if (fabs(y1)>1e100) return -5;
      b[i]+=wt*y1*x1[i];
      if (fabs(b[i])>1e100) return -5;
    }
    for (i=0;i<dim;i++)
      for (j=0;j<dim;j++) {
        m[i][j]=m[i][j]+wt*x1[i]*x1[j];
        if (fabs(m[i][j])>1e100) return -5;
      }
  }
  if (through) {
    y2=y0;
    x2[0]=1;
    for (i=1;i<dim;i++) x2[i]=x2[i-1]*x0;
    for (i=0;i<dim;i++) {
      m[i][dim]=x2[i];
      m[dim][i]=x2[i];
    }
    b[dim]=y2;
    dim++;
  }
  if (matsolv(dim,m,b,coe)) return -2;
  derror=yy;
  for (i=0;i<dim;i++) derror-=coe[i]*b[i];
  derror=fabs(derror)/sum;
  sy=yy/sum-(b[0]/sum)*(b[0]/sum);
  if ((sy==0) || (1-derror/sy<0)) correlation=-1;
  else correlation=sqrt(1-derror/sy);
  derror=sqrt(derror);
  if (through) dim--;
  for (i=0;i<dim;i++) fitlocal->coe[i]=coe[i];
  for (;i<10;i++) fitlocal->coe[i]=0;
  fitlocal->dim=dim;
  fitlocal->derror=derror;
  fitlocal->correlation=correlation;
  fitlocal->num=num;
  if ((equation=memalloc(512))==NULL) return 1;
  equation[0]='\0';
  j=0;
  j += sprintf(equation+j, "--------\nfit:%d (^%d)\n", fitlocal->id, fitlocal->oid);
  if (type==0) {
    for (i=dim-1;i>1;i--) j+=sprintf(equation+j,"%.15e*X^%d+",coe[i],i);
    j+=sprintf(equation+j,"%.15e*X+%.15e",coe[1],coe[0]);
  } else if (type==1) sprintf(equation,"exp(%.15e)*X^%.15e",coe[0],coe[1]);
  else if (type==2) sprintf(equation,"exp(%.15e*X+%.15e)",coe[1],coe[0]);
  else if (type==3) sprintf(equation,"%.15e*Ln(X)+%.15e",coe[1],coe[0]);
  fitlocal->equation=equation;

  if (disp) {
    i=0;
    if (type==0) i+=sprintf(buf+i,"Eq: %%0i*X^i (i=0-%d)\n\n",dim-1);
    else if (type==1) i+=sprintf(buf+i,"Eq: exp(%%00)*X^%%01\n\n");
    else if (type==2) i+=sprintf(buf+i,"Eq: exp(%%01*X+%%00)\n\n");
    else if (type==3) i+=sprintf(buf+i,"Eq: %%01*Ln(X)+%%00\n\n");
    for (j=0;j<dim;j++)
      i+=sprintf(buf+i,"       %%0%d = %+.7e\n",j,coe[j]);
    i+=sprintf(buf+i,"\n");
    i+=sprintf(buf+i,"    points = %d\n",num);
    i+=sprintf(buf+i,"    <DY^2> = %+.7e\n",derror);
    if (correlation>=0)
      i+=sprintf(buf+i,"|r| or |R| = %+.7e\n",correlation);
    else
      i+=sprintf(buf+i,"|r| or |R| = -------------\n");
    ndisplaydialog(buf);
  }

  return 0;
}

int fituser(struct objlist *obj,struct fitlocal *fitlocal,char *func,
            int deriv,double converge,double *data,int num,int disp,
            int weight,double *wdata)
/*
  return
         -7: interrupt
         -6: convergence
         -5: range check
         -4: syntax error
         -3: eqaution is not specified
         -2: singular matrix
         -1: too small data
          0: no error
          1: fatal error

*/
{
  int ecode;
  int *needdata;
  int tbl[10],dim,n,count,rcode,err,err2,err3;
  double yy,y,y1,y2,y3,sy,spx,spy,dxx,dxxc,xx,derror,correlation;
  double b[10],x2[10],par[10],par2[10],parerr[10];
  char parstat[10];
  matrix m;
  int i,j,k,pnum;
  char *equation;
  char buf[1024];
  double wt,sum;
/*
  matrix m2;
  int count2;
  double sum2;
  double lambda,s0;
*/
  if (num<1) return -1;
  if (fitlocal->codef==NULL) return -3;
  if (fitlocal->needdata==NULL) return -3;
  dim=arraynum(fitlocal->needdata);
  needdata=arraydata(fitlocal->needdata);
  if (deriv) {
    for (i=0;i<dim;i++) {
      if (fitlocal->codedf[needdata[i]]==NULL) return -3;
    }
  }
  for (i=0;i<dim;i++) tbl[i]=needdata[i];
  for (i=0;i<10;i++) {
    parstat[i]=MNOERR;
    par[i]=fitlocal->coe[i];
  }

  ecode=0;
/*
  err2=FALSE;
  n=0;
  sum=0;
  yy=0;
  lambda=0.001;
  for (k=0;k<num;k++) {
    if (weight) wt=wdata[k];
    else wt=1;
    sum+=wt;
    spx=data[k*2];
    spy=data[k*2+1];
    err=FALSE;
    rcode=calculate(fitlocal->codef,1,spx,MNOERR,0,0,0,0,
                    0,0,0,0,0,0,0,0,0,0,0,0,
                    par,parstat,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                    NULL,NULL,NULL,0,NULL,NULL,NULL,0,&y1);
    if (rcode==MSERR) return -4;
    else if (rcode!=MNOERR) err=TRUE;
    if (!err) {
      if (fabs(yy)>1e100) return -5;
      y2=spy-y1;
      if ((fabs(y2)>1e50) || (fabs(spy)>1e50)) err=TRUE;
    }
    if (!err) {
      n++;
      yy+=wt*y2*y2;
    } else err2=TRUE;
  }
  if (err2) error(obj,ERRMATH);
  if (n<1) {
    ecode=-1;
    goto errexit;
  }
  s0=yy/sum;
*/

  count=0;
  err3=FALSE;
  do {
    yy=0;
    y=0;
    y3=0;
    for (i=0;i<dim;i++) {
      b[i]=0;
      for (j=0;j<dim;j++) m[j][i]=0;
    }
    err2=FALSE;
    n=0;
    sum=0;
    for (k=0;k<num;k++) {
      if (weight) wt=wdata[k];
      else wt=1;
      sum+=wt;
      spx=data[k*2];
      spy=data[k*2+1];
      err=FALSE;
      rcode=calculate(fitlocal->codef,1,spx,MNOERR,0,0,0,0,
                      0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                      par,parstat,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                      NULL,NULL,NULL,0,NULL,NULL,NULL,0,&y1);
      if (rcode==MSERR) return -4;
      else if (rcode!=MNOERR) err=TRUE;
      if (deriv) {
        for (j=0;j<dim;j++) {
          rcode=calculate(fitlocal->codedf[tbl[j]],1,spx,MNOERR,0,0,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                      par,parstat,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                          NULL,NULL,NULL,0,NULL,NULL,NULL,0,&(x2[j]));
          if (rcode==MSERR) return -4;
          else if (rcode!=MNOERR) err=TRUE;
        }
      } else {
        for (j=0;j<dim;j++) {
          for (i=0;i<10;i++) par2[i]=par[i];
          dxx=par2[j]*converge*1e-6;
          if (dxx==0) dxx=1e-6;
          par2[tbl[j]]+=dxx;
          rcode=calculate(fitlocal->codef,1,spx,MNOERR,0,0,0,0,
                          0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                     par2,parstat,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                          NULL,NULL,NULL,0,NULL,NULL,NULL,0,&(x2[j]));
          if (rcode==MSERR) return -4;
          else if (rcode!=MNOERR) err=TRUE;
          x2[j]=(x2[j]-y1)/dxx;
        }
      }
      if (!err) {
        if ((fabs(yy)>1e100) || (fabs(y3)>1e100)) return -5;
        y2=spy-y1;
        if ((fabs(y2)>1e50) || (fabs(spy)>1e50)) err=TRUE;
      }
      if (!err) {
        n++;
        yy+=wt*y2*y2;
        y+=wt*spy;
        y3+=wt*spy*spy;
        for (j=0;j<dim;j++) b[j]+=wt*x2[j]*y2;
        for (j=0;j<dim;j++)
          for (i=0;i<dim;i++) m[j][i]=m[j][i]+wt*x2[j]*x2[i];
      } else err2=TRUE;
    }
    if (!err3 && err2) {
      error(obj,ERRMATH);
      err3=TRUE;
    }
    if (n<1) {
      ecode=-1;
      goto errexit;
    }

    count++;

    derror=fabs(yy)/sum;
    sy=y3/sum-(y/sum)*(y/sum);
    if ((sy==0) || (1-derror/sy<0)) correlation=-1;
    else correlation=sqrt(1-derror/sy);
    if (correlation>1) correlation=-1;
    derror=sqrt(derror);
    /*
    if (disp) {
      i=0;
      i += sprintf(buf + i, "fit:^%d\n", fitlocal->oid);
      i+=sprintf(buf+i,"Eq: User defined\n\n");
      for (j=0;j<dim;j++)
        i+=sprintf(buf+i,"       %%0%d = %+.7e\n",tbl[j],par[tbl[j]]);
      i+=sprintf(buf+i,"\n");
      i+=sprintf(buf+i,"    points = %d\n",n);
      if (count==1) i+=sprintf(buf+i,"     delta = \n");
      else i+=sprintf(buf+i,"     delta = %+.7e\n",dxxc);
      i+=sprintf(buf+i,"    <DY^2> = %+.7e\n",derror);
      if (correlation>=0)
        i+=sprintf(buf+i,"|r| or |R| = %+.7e\n",correlation);
      else
        i+=sprintf(buf+i,"|r| or |R| = -------------\n");
      i+=sprintf(buf+i," Iteration = %d\n",count);
      ndisplaydialog(buf);
    }
    */
    if (count & 0x3f) {
      sprintf(buf,"fit:^%d Iteration = %d", fitlocal->oid, count);
      set_progress(0, buf, -1);
    }
    if (ninterrupt()) {
      ecode=-7;
      goto errexit;
    }

/*
    count2=0;
    sum2=sum;
    while (TRUE) {
      count2++;
      sprintf(buf,"Iteration = %d:%d",count,count2);
      ndisplaystatus(buf);
      for (j=0;j<dim;j++)
        for (i=0;i<dim;i++) m2[j][i]=m[j][i];
      for (i=0;i<dim;i++) m2[i][i]=m[i][i]+sum2*lambda;
      if (matsolv(dim,m2,b,parerr)) goto repeat;
      for (i=0;i<dim;i++) par2[tbl[i]]=par[tbl[i]]+parerr[i];
      n=0;
      err2=FALSE;
      sum=0;
      yy=0;
      for (k=0;k<num;k++) {
        if (weight) wt=wdata[k];
        else wt=1;
        sum+=wt;
        spx=data[k*2];
        spy=data[k*2+1];
        err=FALSE;
        rcode=calculate(fitlocal->codef,1,spx,MNOERR,0,0,0,0,
                        0,0,0,0,0,0,0,0,0,0,0,0,
                     par2,parstat,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,NULL,
                        NULL,NULL,NULL,0,NULL,NULL,NULL,0,&y1);
        if (rcode==MSERR) return -4;
        else if (rcode!=MNOERR) err=TRUE;
        if (!err) {
          if (fabs(yy)>1e100) goto repeat;
          y2=spy-y1;
          if ((fabs(y2)>1e50) || (fabs(spy)>1e50)) err=TRUE;
        }
        if (!err) {
          n++;
          yy+=wt*y2*y2;
        }
      }
      if (n<1) goto repeat;
      if (yy/sum<=s0) {
        s0=yy/sum;
        break;
      }
repeat:
      lambda*=10;
      if (lambda>1e100) return -6;
    }

    lambda/=10;
*/
    if (matsolv(dim,m,b,parerr)) return -2;

    dxxc=0;
    xx=0;
    for (i=0;i<dim;i++) {
      dxxc+=parerr[i]*parerr[i];
      par[tbl[i]]+=parerr[i];
      xx+=par[tbl[i]]*par[tbl[i]];
    }
    dxxc=sqrt(dxxc);
    xx=sqrt(xx);


  } while ((dxxc>xx*converge/100) && ((xx>1e-6) || (dxxc>1e-6*converge/100)));

  if (disp) {
    i=0;
    i += sprintf(buf + i, "--------\nfit:%d (^%d)\n", fitlocal->id, fitlocal->oid);
    i+=sprintf(buf+i,"Eq: User defined\n");
    i+=sprintf(buf+i,"    %s\n\n", func);
    for (j=0;j<dim;j++)
      i+=sprintf(buf+i,"       %%0%d = %+.7e\n",tbl[j],par[tbl[j]]);
    i+=sprintf(buf+i,"\n");
    i+=sprintf(buf+i,"    points = %d\n",n);
    i+=sprintf(buf+i,"     delta = %+.7e\n",dxxc);
    i+=sprintf(buf+i,"    <DY^2> = %+.7e\n",derror);
    if (correlation>=0)
      i+=sprintf(buf+i,"|r| or |R| = %+.7e\n",correlation);
    else
      i+=sprintf(buf+i,"|r| or |R| = -------------\n");
    ndisplaydialog(buf);
  }

errexit:
  if ((ecode==0) || (ecode==-5)) {
    for (i=0;i<10;i++) fitlocal->coe[i]=par[i];
    fitlocal->dim=dim;
    fitlocal->derror=derror;
    fitlocal->correlation=correlation;
    fitlocal->num=n;
    pnum=0;
    for (i=0;func[i]!='\0';i++)
      if (func[i]=='%') {
        pnum++;
        i+=2;
      }
    if ((equation=memalloc(strlen(func)+25*pnum+1))==NULL) return 1;
    j=0;
    for (i=0;func[i]!='\0';i++)
      if (func[i]!='%') {
        equation[j]=func[i];
        j++;
      } else {
        if (isdigit(func[i+1]) && isdigit(func[i+2]) && isdigit(func[i+3])) {
          pnum=(func[i+1]-'0')*100+(func[i+2]-'0')*10+(func[i+3]-'0');
          i+=3;
        } else if (isdigit(func[i+1]) && isdigit(func[i+2])) {
          pnum=(func[i+1]-'0')*10+(func[i+2]-'0');
          i+=2;
        } else {
          pnum=(func[i+1]-'0');
          i+=1;
        }
        j+=sprintf(equation+j,"%.15e",par[pnum]);
      }
    equation[j]='\0';
    fitlocal->equation=equation;
  }
  return ecode;
}

int fitfit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  struct fitlocal *fitlocal;
  int i,type,through,dimension,deriv,disp;
  double x,y,x0,y0,converge,wt;
  struct narray *darray;
  double *data,*wdata;
  char *equation,*func;
  int dnum,num,err,err2,err3,rcode;
  double derror,correlation,pp;
  int ecode;
  int weight,anum;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"equation",inst,&equation);
  memfree(equation);
  if (_putobj(obj,"equation",inst,NULL)) return 1;
  if ((equation=memalloc(6))==NULL) return 1;
  strcpy(equation,"undef");
  if (_putobj(obj,"equation",inst,equation)) {
    memfree(equation);
    return 1;
  }
  num=0;
  derror=0;
  correlation=0;
  pp=0;
  if (_putobj(obj,"number",inst,&num)) return 1;
  if (_putobj(obj,"error",inst,&derror)) return 1;
  if (_putobj(obj,"correlation",inst,&correlation)) return 1;
  if (_putobj(obj,"%00",inst,&pp)) return 1;
  if (_putobj(obj,"%01",inst,&pp)) return 1;
  if (_putobj(obj,"%02",inst,&pp)) return 1;
  if (_putobj(obj,"%03",inst,&pp)) return 1;
  if (_putobj(obj,"%04",inst,&pp)) return 1;
  if (_putobj(obj,"%05",inst,&pp)) return 1;
  if (_putobj(obj,"%06",inst,&pp)) return 1;
  if (_putobj(obj,"%07",inst,&pp)) return 1;
  if (_putobj(obj,"%08",inst,&pp)) return 1;
  if (_putobj(obj,"%09",inst,&pp)) return 1;

  _getobj(obj,"_local",inst,&fitlocal);
  _getobj(obj,"type",inst,&type);
  _getobj(obj,"through_point",inst,&through);
  _getobj(obj,"point_x",inst,&x0);
  _getobj(obj,"point_y",inst,&y0);
  _getobj(obj,"poly_dimension",inst,&dimension);

  _getobj(obj,"user_func",inst,&func);
  _getobj(obj,"derivative",inst,&deriv);
  _getobj(obj,"converge",inst,&converge);
  _getobj(obj, "id", inst, &(fitlocal->id));
  _getobj(obj,"parameter0",inst,&(fitlocal->coe[0]));
  _getobj(obj,"parameter1",inst,&(fitlocal->coe[1]));
  _getobj(obj,"parameter2",inst,&(fitlocal->coe[2]));
  _getobj(obj,"parameter3",inst,&(fitlocal->coe[3]));
  _getobj(obj,"parameter4",inst,&(fitlocal->coe[4]));
  _getobj(obj,"parameter5",inst,&(fitlocal->coe[5]));
  _getobj(obj,"parameter6",inst,&(fitlocal->coe[6]));
  _getobj(obj,"parameter7",inst,&(fitlocal->coe[7]));
  _getobj(obj,"parameter8",inst,&(fitlocal->coe[8]));
  _getobj(obj,"parameter9",inst,&(fitlocal->coe[9]));
  _getobj(obj,"display",inst,&disp);

  if (through && (type==4)) {
    error(obj,ERRTHROUGH);
    return 1;
  }
  darray=(struct narray *)(argv[2]);
  if (arraynum(darray)<1) return -1;
  data=arraydata(darray);
  anum=arraynum(darray)-1;
  dnum=nround(data[0]);
  data+=1;
  if (dnum==(anum/2)) weight=FALSE;
  else if (dnum==(anum/3)) {
    weight=TRUE;
    wdata=data+2*dnum;
  } else return 1;
  num=0;
  err2=err3=FALSE;
  for (i=0;i<dnum;i++) {
    x=data[i*2];
    y=data[i*2+1];
    if (weight) wt=wdata[i];
    err=FALSE;
    if (type==1) {
      if (y<=0) err=TRUE;
      else y=log(y);
      if (x<=0) err=TRUE;
      else x=log(x);
    } else if (type==2) {
      if (y<=0) err=TRUE;
      else y=log(y);
    } else if (type==3) {
      if (x<=0) err=TRUE;
      else x=log(x);
    }
    if (err) err2=TRUE;
    else if (weight && (wt<=0)) {
      err=TRUE;
      err3=TRUE;
    }
    if (!err) {
      data[num*2]=x;
      data[num*2+1]=y;
      if (weight) wdata[num]=wt;
      num++;
    }
  }
  if (err2) error(obj,ERRLN);
  if (err3) error(obj,ERRNEGATIVEWEIGHT);
  if (through) {
    err=FALSE;
    if (type==1) {
      if (y0<=0) err=TRUE;
      else y0=log(y0);
      if (x0<=0) err=TRUE;
      else x0=log(x0);
    } else if (type==2) {
      if (y0<=0) err=TRUE;
      else y0=log(y0);
    } else if (type==3) {
      if (x0<=0) err=TRUE;
      else x0=log(x0);
    }
    if (err) {
      error(obj,ERRPOINT);
      return 1;
    }
  }
  if (type!=4)
    rcode=fitpoly(fitlocal,type,dimension,through,x0,y0,data,num,disp,weight,wdata);
  else
    rcode=fituser(obj,fitlocal,func,deriv,converge,data,num,disp,weight,wdata);
  if (rcode==1) return 1;
  else if (rcode==-1) ecode=ERRSMLDATA;
  else if (rcode==-2) ecode=ERRSINGULAR;
  else if (rcode==-3) ecode=ERRNOEQS;
  else if (rcode==-4) ecode=ERRSYNTAX;
  else if (rcode==-5) ecode=ERRRANGE;
  else if (rcode==-6) ecode=ERRCONVERGE;
  else ecode=0;
  if (ecode!=0) {
    error(obj,ecode);
    return 1;
  }
  if (_putobj(obj,"number",inst,&(fitlocal->num))) return 1;
  if (_putobj(obj,"error",inst,&(fitlocal->derror))) return 1;
  if (_putobj(obj,"correlation",inst,&(fitlocal->correlation))) return 1;
  if (_putobj(obj,"%00",inst,&(fitlocal->coe[0]))) return 1;
  if (_putobj(obj,"%01",inst,&(fitlocal->coe[1]))) return 1;
  if (_putobj(obj,"%02",inst,&(fitlocal->coe[2]))) return 1;
  if (_putobj(obj,"%03",inst,&(fitlocal->coe[3]))) return 1;
  if (_putobj(obj,"%04",inst,&(fitlocal->coe[4]))) return 1;
  if (_putobj(obj,"%05",inst,&(fitlocal->coe[5]))) return 1;
  if (_putobj(obj,"%06",inst,&(fitlocal->coe[6]))) return 1;
  if (_putobj(obj,"%07",inst,&(fitlocal->coe[7]))) return 1;
  if (_putobj(obj,"%08",inst,&(fitlocal->coe[8]))) return 1;
  if (_putobj(obj,"%09",inst,&(fitlocal->coe[9]))) return 1;
  _getobj(obj,"equation",inst,&equation);
  if (_putobj(obj,"equation",inst,fitlocal->equation)) return 1;
  memfree(equation);
  fitlocal->equation=NULL;
  return 0;
}



#define TBLNUM 54

struct objtable fit[TBLNUM] = {
  {"init",NVFUNC,NEXEC,fitinit,NULL,0},
  {"done",NVFUNC,NEXEC,fitdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},

  {"profile",NSTR,NREAD|NWRITE,NULL,NULL,0},

  {"type",NENUM,NREAD|NWRITE,fitput,fittypechar,0},
  {"min",NDOUBLE,NREAD|NWRITE,NULL,NULL,0},
  {"max",NDOUBLE,NREAD|NWRITE,NULL,NULL,0},
  {"div",NINT,NREAD|NWRITE,oputge1,NULL,0},
  {"interpolation",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"through_point",NBOOL,NREAD|NWRITE,fitput,NULL,0},
  {"point_x",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"point_y",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"equation",NSTR,NREAD|NWRITE,NULL,NULL,0},

  {"poly_dimension",NINT,NREAD|NWRITE,fitput,NULL,0},

  {"weight_func",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"user_func",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative",NBOOL,NREAD|NWRITE,fitput,NULL,0},
  {"derivative0",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative1",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative2",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative3",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative4",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative5",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative6",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative7",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative8",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"derivative9",NSTR,NREAD|NWRITE,fitput,NULL,0},
  {"converge",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter0",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter1",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter2",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter3",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter4",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter5",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter6",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter7",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter8",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"parameter9",NDOUBLE,NREAD|NWRITE,fitput,NULL,0},
  {"%00",NDOUBLE,NREAD,NULL,NULL,0},
  {"%01",NDOUBLE,NREAD,NULL,NULL,0},
  {"%02",NDOUBLE,NREAD,NULL,NULL,0},
  {"%03",NDOUBLE,NREAD,NULL,NULL,0},
  {"%04",NDOUBLE,NREAD,NULL,NULL,0},
  {"%05",NDOUBLE,NREAD,NULL,NULL,0},
  {"%06",NDOUBLE,NREAD,NULL,NULL,0},
  {"%07",NDOUBLE,NREAD,NULL,NULL,0},
  {"%08",NDOUBLE,NREAD,NULL,NULL,0},
  {"%09",NDOUBLE,NREAD,NULL,NULL,0},
  {"number",NINT,NREAD,NULL,NULL,0},
  {"error",NDOUBLE,NREAD,NULL,NULL,0},
  {"correlation",NDOUBLE,NREAD,NULL,NULL,0},
  {"display",NBOOL,NREAD|NWRITE,NULL,NULL,0},

  {"fit",NVFUNC,NREAD|NEXEC,fitfit,"da",0},
  {"_local",NPOINTER,0,NULL,NULL,0},
};

void *addfit()
/* addfit() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,fit,ERRNUM,fiterrorlist,NULL,NULL);
}

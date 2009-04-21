/* 
 * $Id: mathcode.c,v 1.8 2009/04/21 14:17:58 hito Exp $
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
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "object.h"
#include "nstring.h"
#include "mathfn.h"
#include "mathcode.h"

#define TRUE 1
#define FALSE 0

#define OPENUM 65

char *matherrorchar[]={
 "noerr",
 "err",
 "nan",
 "undef",
 "syntax",
 "cont",
 "break",
 "nonum",
 "eof",
 NULL
};

static struct {
  char *op;
  char code;
  char arg;
} opetable[OPENUM]={
  {"ABS(",  'A',1},
  {"SIGN(", 'B',1},
  {"INT(",  'C',1},
  {"FRAC(", 'D',1},
  {"ROUND(",'E',1},
  {"SQR(",  'F',1},
  {"SQRT(", 'G',1},
  {"EXP(",  'H',1},
  {"LN(",   'I',1},
  {"LOG(",  'J',1},
  {"SIN(",  'K',1},
  {"COS(",  'L',1},
  {"TAN(",  'M',1},
  {"ASIN(", 'N',1},
  {"ACOS(", 'O',1},
  {"ATAN(", 'P',1},
  {"SINH(", 'Q',1},
  {"COSH(", 'R',1},
  {"TANH(", 'S',1},
  {"ASINH(",'T',1},
  {"ACOSH(",'U',1},
  {"ATANH(",'V',1},
  {"THETA(",'W',1},
  {"DELTA(",'a',1},
  {"GAUSS(",'b',1},
  {"GAMMA(",'c',1},
  {"ICGAM(",'d',2},
  {"ERFC(", 'e',1},
  {"EI(",   'f',1},
  {"BETA(", 'g',2},
  {"JN(",   'h',2},
  {"YN(",   'i',2},
  {"MIN(",  'j',2},
  {"MAX(",  'k',2},
  {"EQ(",   'l',2},
  {"NEQ(",  'm',2},
  {"GE(",   'n',2},
  {"GT(",   'o',2},
  {"LE(",   'p',2},
  {"LT(",   'q',2},
  {"NOT(",  'r',1},
  {"OR(",   's',2},
  {"AND(",  't',2},
  {"XOR(",  'u',2},
  {"RAND(", 'v',1},
  {"PN(",   'w',2},
  {"LGN(",  'x',3},
  {"HN(",   'y',2},
  {"TN(",   'z',2},
  {"M(",    '{',2},
  {"RM(",   '}',1},
  {"SUM(",  '[',1},
  {"DIF(",  ']',1},
  {"MJD(",  '<',3},
  {"QINV(", '>',1},
  {"F(",(char )0xa0,3},
  {"G(",(char )0xa1,3},
  {"H(",(char )0xa2,3},
  {"IF(",'?',3},
  {"FOR(",':',5},
  {"COLOR(",'"',2},
  {"RGB(",(char )0xc0,3},
  {"HSB(",(char )0xc1,3},
  {"MARKSIZE(",'\'',1},
  {"MARKTYPE(",'$',1},
};

/*
used character code:

=XYZ#|*+-/\^(){}[]!,"@&%$~?_`
8x
9x
b0 b1 b2 b3
d0 d1 d2 d3 d4 d5 d6 d7 d8 d9 da db dc dd

*/

int
mathcode_error(struct objlist *obj, enum MATH_CODE_ERROR_NO rcode, int *ecode) {
  switch (rcode) {
  case MCNOERR:
    return 0;
  case MCSYNTAX:
  case MCILLEGAL:
  case MCNEST:
    error(obj, ecode[rcode]);
    return 1;
  }
  /* never reached */
  return 1;
}

enum MATH_CODE_ERROR_NO 
mathcode(char *str,char **code,
	 struct narray *needdata,struct narray *needfile,
	 int *maxdim,int *twopass,
	 int datax,int datay,int dataz,
	 int column,int multi,int minmax,int memory,int userfn,
	 int color,int marksize,int file)
{
  enum MATH_CODE_ERROR_NO rcode;
  int i,po,integpo,difpo,dim,id,*ndata,nn,maxdim2;
  unsigned int j;
  size_t len;
  double x;
  unsigned char ch;
  char *s=NULL,*endptr;
  struct narray *ncode=NULL;

  rcode=MCSYNTAX;
  integpo=0;
  difpo=0;
  if (minmax) *twopass=FALSE;
  if (column) *maxdim=0;
  if ((ncode=arraynew(1))==NULL) goto errexit;
  if ((s=memalloc(strlen(str)+2))==NULL) goto errexit;
  strcpy(s,str);

  j=0;
  for (i=0;s[i]!='\0';i++)
    if (s[i]!=' ') s[j++]=toupper(s[i]);
  if (j==0) return rcode;
  if (s[j-1]!='=') s[j++]='=';
  s[j]='\0';

  po=0;
  while (s[po]!='\0') {
    if (s[po]=='=') {
      if (arrayadd(ncode,s+po)==NULL) goto errexit;
      po++;
    } else if (isdigit(s[po]) || (s[po]=='.')) {
      if ((s[po]=='0') && (s[po+1]=='X')) x=strtol(s+po,&endptr,16);
      else x=strtod(s+po,&endptr);
      len=endptr-(s+po);
      ch='#';
      if (arrayadd(ncode,&ch)==NULL) goto errexit;
      for (j=0;j<sizeof(x);j++)
        if (arrayadd(ncode,((char *)&x)+j)==NULL) goto errexit;
      po+=len;
    } else if ((s[po]=='%') && isdigit(s[po+1])) {
      if (!column) {
        rcode=MCILLEGAL;
        goto errexit;
      }
      if (isdigit(s[po+2]) && isdigit(s[po+3])) {
        if (arrayadd(ncode,s+po+1)==NULL) goto errexit;
        if (arrayadd(ncode,s+po+2)==NULL) goto errexit;
        if (arrayadd(ncode,s+po+3)==NULL) goto errexit;
        dim=(s[po+1]-'0')*100+(s[po+2]-'0')*10+(s[po+3]-'0');
        po+=4;
      } else if (isdigit(s[po+2])) {
        ch='0';
        if (arrayadd(ncode,&ch)==NULL) goto errexit;
        if (arrayadd(ncode,s+po+1)==NULL) goto errexit;
        if (arrayadd(ncode,s+po+2)==NULL) goto errexit;
        dim=(s[po+1]-'0')*10+(s[po+2]-'0');
        po+=3;
      } else {
        ch='0';
        if (arrayadd(ncode,&ch)==NULL) goto errexit;
        if (arrayadd(ncode,&ch)==NULL) goto errexit;
        if (arrayadd(ncode,s+po+1)==NULL) goto errexit;
        dim=(s[po+1]-'0');
        po+=2;
      }
      if (*maxdim<dim) *maxdim=dim;
      if (needdata!=NULL) {
        ndata=arraydata(needdata);
        nn=arraynum(needdata);
        for (i=0;i<nn;i++) if (ndata[i]==dim) break;
        if (i==nn) arrayadd(needdata,&dim);
      }
    } else if ((s[po]=='%') && (s[po+1]=='D')) {
      if (!file) {
        rcode=MCILLEGAL;
        goto errexit;
      }
      ch=(char )0xb3;
      if (arrayadd(ncode,&ch)==NULL) goto errexit;
      po+=2;
    } else if ((s[po]=='%') && (s[po+1]=='F')
      && isdigit(s[po+2]) && isdigit(s[po+3]) && isdigit(s[po+4])) {
      if (!file || !column) {
        rcode=MCILLEGAL;
        goto errexit;
      }
      ch='|';
      if (arrayadd(ncode,&ch)==NULL) goto errexit;
      if (arrayadd(ncode,s+po+2)==NULL) goto errexit;
      if (arrayadd(ncode,s+po+3)==NULL) goto errexit;
      id=(s[po+2]-'0')*10+(s[po+3]-'0');
      if (isdigit(s[po+5]) && isdigit(s[po+6])) {
        if (arrayadd(ncode,s+po+4)==NULL) goto errexit;
        if (arrayadd(ncode,s+po+5)==NULL) goto errexit;
        if (arrayadd(ncode,s+po+6)==NULL) goto errexit;
        dim=(s[po+4]-'0')*100+(s[po+5]-'0')*10+(s[po+6]-'0');
        po+=7;
      } else if (isdigit(s[po+5])) {
        ch='0';
        if (arrayadd(ncode,&ch)==NULL) goto errexit;
        if (arrayadd(ncode,s+po+4)==NULL) goto errexit;
        if (arrayadd(ncode,s+po+5)==NULL) goto errexit;
        dim=(s[po+4]-'0')*10+(s[po+5]-'0');
        po+=6;
      } else {
        ch='0';
        if (arrayadd(ncode,&ch)==NULL) goto errexit;
        if (arrayadd(ncode,&ch)==NULL) goto errexit;
        if (arrayadd(ncode,s+po+4)==NULL) goto errexit;
        dim=(s[po+4]-'0');
        po+=5;
      }
      maxdim2=id*1000+dim;
      if (needfile!=NULL) {
        ndata=arraydata(needfile);
        nn=arraynum(needfile);
        for (i=0;i<nn;i++) {
          if (ndata[i]==maxdim2) break;
          else if (ndata[i]>maxdim2) {
            arrayins(needfile,&maxdim2,i);
            break;
          }
        }
        if (i==nn) arrayadd(needfile,&maxdim2);
      }
    } else if (strchr("+-*/\\^()!,",s[po])!=NULL) {
      if (arrayadd(ncode,s+po)==NULL) goto errexit;
      po++;
    } else {
      for (i=0;i<OPENUM &&
         (strncmp(s+po,opetable[i].op,strlen(opetable[i].op))!=0);i++);
      if (i==OPENUM) {
        if (s[po]=='X') {
          if (!datax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          if (arrayadd(ncode,s+po)==NULL) goto errexit;
          po++;
        } else if (s[po]=='Y') {
          if (!datay) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          if (arrayadd(ncode,s+po)==NULL) goto errexit;
          po++;
        } else if (s[po]=='Z') {
          if (!dataz) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          if (arrayadd(ncode,s+po)==NULL) goto errexit;
          po++;
        } else if (strncmp(s+po,"FIRST",5)==0) {
          if (!datax && !datay && !dataz) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xb2;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=5;
        } else if (strncmp(s+po,"NUM",3)==0) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xd0;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=3;
          *twopass=TRUE;
        } else if (strncmp(s+po,"MINX",4)==0) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xd1;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=4;
          *twopass=TRUE;
        } else if (strncmp(s+po,"MINY",4)==0) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xd2;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=4;
          *twopass=TRUE;
        } else if (strncmp(s+po,"MAXX",4)==0) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xd3;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=4;
          *twopass=TRUE;
        } else if (strncmp(s+po,"MAXY",4)==0) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xd4;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=4;
          *twopass=TRUE;
        } else if (strncmp(s+po,"SUMXX",5)==0) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xd7;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=5;
          *twopass=TRUE;
        } else if (strncmp(s+po,"SUMYY",5)==0) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xd8;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=5;
          *twopass=TRUE;
        } else if ((strncmp(s+po,"SUMXY",5)==0)
                || (strncmp(s+po,"SUMYX",5)==0)) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xd9;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=5;
          *twopass=TRUE;
        } else if (strncmp(s+po,"SUMX",4)==0) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xd5;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=4;
          *twopass=TRUE;
        } else if (strncmp(s+po,"SUMY",4)==0) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xd6;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=4;
          *twopass=TRUE;
        } else if (strncmp(s+po,"AVX",3)==0) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xda;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=3;
          *twopass=TRUE;
        } else if (strncmp(s+po,"AVY",3)==0) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xdb;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=3;
          *twopass=TRUE;
        } else if (strncmp(s+po,"SGX",3)==0) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xdc;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=3;
          *twopass=TRUE;
        } else if (strncmp(s+po,"SGY",3)==0) {
          if (!minmax) {
            rcode=MCILLEGAL;
            goto errexit;
          }
          ch=(char )0xdd;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=3;
          *twopass=TRUE;
        } else if ((strncmp(s+po,"EULER",5)==0) || (s[po]=='E')
                || (strncmp(s+po,"PI",2)==0)) {
          ch='#';
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          if (strncmp(s+po,"EULER",5)==0) {
            x=MEULER;
            po+=5;
          } else if (s[po]=='E') {
            x=MEXP1;
            po++;
          } else {
            x=MPI;
            po+=2;
          }
          for (j=0;j<sizeof(x);j++)
            if (arrayadd(ncode,(char *)&x+j)==NULL) goto errexit;
        } else if (strncmp(s+po,"NAN",3)==0) {
          ch='~';
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=3;
        } else if (strncmp(s+po,"UNDEF",5)==0) {
          ch='@';
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=5;
        } else if (strncmp(s+po,"CONT",4)==0) {
          ch=(char )0xb0;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=4;
        } else if (strncmp(s+po,"BREAK",5)==0) {
          ch=(char )0xb1;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          po+=5;
        } else goto errexit;
      } else {
        if ((!multi) && (strchr("[]",opetable[i].code)!=NULL)) {
          rcode=MCILLEGAL;
          goto errexit;
        }
        if ((!memory) && (strchr("{}",opetable[i].code)!=NULL)) {
          rcode=MCILLEGAL;
          goto errexit;
        }
        if ((!color) 
        && ((opetable[i].code=='"')
        || (opetable[i].code==(char )0xc0) || (opetable[i].code==(char )0xc1))) {
          rcode=MCILLEGAL;
          goto errexit;
        }
        if ((!marksize) &&
            ((opetable[i].code=='\'') || (opetable[i].code=='$'))) {
          rcode=MCILLEGAL;
          goto errexit;
        }
        if ((!userfn) &&
       (   (opetable[i].code==(char )0xa0)
        || (opetable[i].code==(char )0xa1)
        || (opetable[i].code==(char )0xa2)  )) {
          rcode=MCILLEGAL;
          goto errexit;
        }
        if (opetable[i].code=='[') {
          if (integpo>=10) {
            rcode=MCNEST;
            goto errexit;
          }
          ch=(unsigned char)0x80+(unsigned char)integpo;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          integpo++;
        } else if (opetable[i].code==']') {
          if (difpo>=10) {
            rcode=MCNEST;
            goto errexit;
          }
          ch=(unsigned char)0x90+(unsigned char)difpo;
          if (arrayadd(ncode,&ch)==NULL) goto errexit;
          difpo++;
	    } else {
          if (arrayadd(ncode,&(opetable[i].code))==NULL) goto errexit;
        }
        ch='0'+(unsigned char)opetable[i].arg;
        if (arrayadd(ncode,&ch)==NULL) goto errexit;
        ch='(';
        if (arrayadd(ncode,&ch)==NULL) goto errexit;
        po+=strlen(opetable[i].op);
      }
    }
  }
  ch='_';
  if (arrayadd(ncode,&ch)==NULL) goto errexit;
  len=arraynum(ncode);
  *code=arraydata(ncode);
  memfree(ncode);
  memfree(s);
  return MCNOERR;

errexit:
  arrayfree(ncode);
  memfree(s);
  arraydel(needdata);
  arraydel(needfile);
  *code=NULL;
  return rcode;
}

static void 
mathexpand(int pre,int *oppo,int *numpo,int *ifpo,int *forpo,
     double numbuf[],char numstat[],
     char opbuf[],char prebuf[],char argbuf[],char noexpand[],char fnoexpand[],
     double memory[],char memorystat[],
     double sumdata[],char sumstat[],
     double difdata[],char difstat[],
     int color[],int *marksize,int *marktype,
     char *ufcodef,char *ufcodeg,char *ufcodeh)
{
  double x;
  int i,status,argnum;
  char *ufcode;
  double umemory[MEMORYNUM];
  char umemorystat[MEMORYNUM];

  while ((*oppo>=0) && (pre<=prebuf[*oppo])) {
    if (strchr("+-*/\\^",opbuf[*oppo])!=NULL) argnum=2;
    else argnum=argbuf[*oppo];
    if (opbuf[*oppo]=='?') {
      if ((numstat[*numpo-2]==MNOERR) && (numbuf[*numpo-2]!=0)) {
        numbuf[*numpo-2]=numbuf[*numpo-1];
        numstat[*numpo-2]=numstat[*numpo-1];
      } else {
        numbuf[*numpo-2]=numbuf[*numpo];
        numstat[*numpo-2]=numstat[*numpo];
      }
      (*ifpo)--;
    } else if (opbuf[*oppo]==':') {
      numbuf[*numpo-4]=numbuf[*numpo];
      (*forpo)--;
    } else if ((!noexpand[*ifpo]) && (!fnoexpand[*forpo])) {
      status=MNOERR;
      for (i=1;i<=argnum;i++)
        if (numstat[*numpo-i+1]>status) status=numstat[*numpo-i+1];
      if ((opbuf[*oppo]!='{') && (status!=0)) numstat[*numpo-argnum+1]=status;
      else {
          switch (opbuf[*oppo]) {
          case '!': 
            if ((numbuf[*numpo]-(int )numbuf[*numpo]!=0)
            || (numbuf[*numpo]>69) || (numbuf[*numpo]<0)) 
              numstat[*numpo]=MERR;
            else {
              x=1;
              for (i=2;i<=(int )numbuf[*numpo];i++) x*=i;
              numbuf[*numpo]=x;
            }
            break;
          case '^': 
            if ((numbuf[*numpo-1]==0) && (numbuf[*numpo]==0))
              numbuf[*numpo-1]=1;
            else if (numbuf[*numpo-1]==0) numbuf[*numpo-1]=0;
            else if (numbuf[*numpo]-(int )numbuf[*numpo]==0) {
              x=numbuf[*numpo]*log(fabs(numbuf[*numpo-1]));
              if (x>230) numstat[*numpo-1]=MERR;
              else if (x<-230) numbuf[*numpo-1]=0;
              else {
                if (numbuf[*numpo]/2-(int )(numbuf[*numpo]/2)==0) 
                  numbuf[*numpo-1]=exp(x);
                else if (numbuf[*numpo-1]>=0) numbuf[*numpo-1]=exp(x);
                else numbuf[*numpo-1]=-exp(x);
              }
            } else if (numbuf[*numpo-1]<0) numstat[*numpo-1]=MERR;
            else {
              x=numbuf[*numpo]*log(numbuf[*numpo-1]);
              if (x>230) numstat[*numpo-1]=MERR;
              else if (x<-230) numbuf[*numpo-1]=0;
              else numbuf[*numpo-1]=exp(x);
            }
            break;
          case '`':
            numbuf[*numpo]=numbuf[*numpo];
            break;
          case '+': 
            numbuf[*numpo-1]=numbuf[*numpo-1]+numbuf[*numpo];
            break;
          case '&':
            numbuf[*numpo]=-numbuf[*numpo];
            break;
          case '-':
            numbuf[*numpo-1]=numbuf[*numpo-1]-numbuf[*numpo];
            break;
          case '/':
            if (fabs(numbuf[*numpo])<=1E-100) numstat[*numpo-1]=MERR;
            else numbuf[*numpo-1]=numbuf[*numpo-1]/numbuf[*numpo];
            break;
          case '\\': 
            if (fabs(numbuf[*numpo])<=1E-100) numstat[*numpo-1]=MERR;
            else numbuf[*numpo-1]=numbuf[*numpo-1]
                 -((int )(numbuf[*numpo-1]/numbuf[*numpo]))*numbuf[*numpo];
            break;
          case '*':
            numbuf[*numpo-1]=numbuf[*numpo-1]*numbuf[*numpo];
            break;
          case 'A':
            numbuf[*numpo]=fabs(numbuf[*numpo]);
            break;
          case 'B':
            if (numbuf[*numpo]>=0) numbuf[*numpo]=1;
            else numbuf[*numpo]=-1;
            break;
          case 'C':
            numbuf[*numpo]=(int )numbuf[*numpo];
            break;
          case 'D':
            numbuf[*numpo]=numbuf[*numpo]-(int )numbuf[*numpo];
            break;
          case 'E': 
            if (fabs(numbuf[*numpo]-(int )numbuf[*numpo])>=0.5) {
              if (numbuf[*numpo]>=0) 
                numbuf[*numpo]=numbuf[*numpo]
                             -(numbuf[*numpo]-(int )numbuf[*numpo])+1;
              else numbuf[*numpo]=numbuf[*numpo]
                             -(numbuf[*numpo]-(int )numbuf[*numpo])-1;
            } else numbuf[*numpo]=numbuf[*numpo]
                             -(numbuf[*numpo]-(int )numbuf[*numpo]);
            break;
          case 'F': 
            if (fabs(numbuf[*numpo])>=1E50) numstat[*numpo]=MERR;
            else numbuf[*numpo]=numbuf[*numpo]*numbuf[*numpo];
            break;
          case 'G':
            if (numbuf[*numpo]<0) numstat[*numpo]=MERR;
            else numbuf[*numpo]=sqrt(numbuf[*numpo]);
            break;
          case 'H':
            if (numbuf[*numpo]>230) numstat[*numpo]=MERR;
            else if (numbuf[*numpo]<-230) numbuf[*numpo]=0;
            else numbuf[*numpo]=exp(numbuf[*numpo]);
            break;
          case 'I': 
            if (numbuf[*numpo]<=0) numstat[*numpo]=MERR;
            else numbuf[*numpo]=log(numbuf[*numpo]);
            break;
          case 'J': 
            if (numbuf[*numpo]<=0) numstat[*numpo]=MERR;
            else numbuf[*numpo]=log10(numbuf[*numpo]);
            break;
          case 'K': 
            numbuf[*numpo]=sin(numbuf[*numpo]);
            break;
          case 'L': 
            numbuf[*numpo]=cos(numbuf[*numpo]);
            break;
          case 'M':
            x=cos(numbuf[*numpo]);
            if (fabs(x)<=1E-100) numstat[*numpo]=MERR;
            else numbuf[*numpo]=sin(numbuf[*numpo])/x;
            break;
          case 'N': 
            if (fabs(numbuf[*numpo])>1) numstat[*numpo]=MERR;
            else if (numbuf[*numpo]==-1) numbuf[*numpo]=-MPI*0.5;
            else if (numbuf[*numpo]==1) numbuf[*numpo]=MPI*0.5;
            else numbuf[*numpo]=
                 atan(numbuf[*numpo]/sqrt(1-numbuf[*numpo]*numbuf[*numpo]));
            break;
          case 'O': 
            if (fabs(numbuf[*numpo])>1) numstat[*numpo]=MERR;
            else if (numbuf[*numpo]==0) numbuf[*numpo]=MPI*0.5;
            else if (numbuf[*numpo]>0) 
              numbuf[*numpo]=atan(sqrt(1-numbuf[*numpo]*numbuf[*numpo])
                                 /numbuf[*numpo]);
            else if (numbuf[*numpo]<0) 
              numbuf[*numpo]=atan(sqrt(1-numbuf[*numpo]*numbuf[*numpo])
                                 /numbuf[*numpo])+MPI;
            break;
          case 'P': 
            numbuf[*numpo]=atan(numbuf[*numpo]);
            break;
          case 'Q': 
            if (fabs(numbuf[*numpo])>230) numstat[*numpo]=MERR;
            else numbuf[*numpo]=(exp(numbuf[*numpo])-exp(-numbuf[*numpo]))*0.5;
            break;
          case 'R': 
            if (fabs(numbuf[*numpo])>230) numstat[*numpo]=MERR;
            else numbuf[*numpo]=(exp(numbuf[*numpo])+exp(-numbuf[*numpo]))*0.5;
            break;
          case 'S': 
            if (fabs(numbuf[*numpo])>115) numstat[*numpo]=MERR;
            else numbuf[*numpo]=
                 (exp(2*numbuf[*numpo])-1)/(exp(2*numbuf[*numpo])+1);
            break;
          case 'T':
            if (fabs(numbuf[*numpo])>=1E50) numstat[*numpo]=MERR;
            else {
              x=numbuf[*numpo]+sqrt(numbuf[*numpo]*numbuf[*numpo]+1);
              if (x<=0) numstat[*numpo]=MERR;
              else numbuf[*numpo]=log(x);
            }
            break;
          case 'U':
            if ((fabs(numbuf[*numpo])>1E50) || (fabs(numbuf[*numpo])<1))
              numstat[*numpo]=MERR;
            else {
              x=numbuf[*numpo]+sqrt(numbuf[*numpo]*numbuf[*numpo]-1);
              if (x<=0) numstat[*numpo]=MERR;
              else numbuf[*numpo]=log(x);
            }
            break;
          case 'V':
            if (numbuf[*numpo]==1) numstat[*numpo]=MERR;
            else {
              x=(1+numbuf[*numpo])/(1-numbuf[*numpo]);
              if (x<=0) numstat[*numpo]=MERR;
              else numbuf[*numpo]=log(x)*0.5;
            }
            break;
          case 'W':
            if (numbuf[*numpo]>=0) numbuf[*numpo]=1;
            else numbuf[*numpo]=0;
            break;
          case 'a':
            if (numbuf[*numpo]==0) numbuf[*numpo]=1;
            else numbuf[*numpo]=0;
            break;
          case 'b':
            if (numbuf[*numpo]>=0) numbuf[*numpo]=(int )(numbuf[*numpo]);
            else if (numbuf[*numpo]-(int )numbuf[*numpo]==0)
              numbuf[*numpo]=(int )numbuf[*numpo];
            else numbuf[*numpo]=(int )numbuf[*numpo]-1;
            break;
          case 'c':
            if (gamma2(numbuf[*numpo],&(numbuf[*numpo]))) numstat[*numpo]=MERR;
            break;
          case 'd':
            if (icgamma(numbuf[*numpo-1],numbuf[*numpo],&(numbuf[*numpo-1])))
              numstat[*numpo-1]=MERR;
            break;
          case 'e':
            if (erfc1(numbuf[*numpo],&(numbuf[*numpo]))) numstat[*numpo]=MERR;
            break;
          case 'f':
            if (exp1(numbuf[*numpo],&(numbuf[*numpo]))) numstat[*numpo]=MERR;
            break;
          case 'g':
            if (beta(numbuf[*numpo-1],numbuf[*numpo],&(numbuf[*numpo-1])))
              numstat[*numpo-1]=MERR;
            break;
          case 'h':
            if ((numbuf[*numpo-1]-(int )numbuf[*numpo-1]!=0)
            || (fabs(numbuf[*numpo-1])>1000)) numstat[*numpo-1]=MERR;
            else if (jbessel((int )numbuf[*numpo-1],
                   numbuf[*numpo],&(numbuf[*numpo-1]))) numstat[*numpo-1]=MERR;
            break;
          case 'i':
            if ((numbuf[*numpo-1]-(int )numbuf[*numpo-1]!=0)
            || (fabs(numbuf[*numpo-1])>1000)) numstat[*numpo-1]=MERR;
            else if (ybessel((int )numbuf[*numpo-1],
                   numbuf[*numpo],&(numbuf[*numpo-1]))) numstat[*numpo-1]=MERR;
            break;
          case 'j':
            if (numbuf[*numpo-1]>numbuf[*numpo])
              numbuf[*numpo-1]=numbuf[*numpo];
            break;
          case 'k':
            if (numbuf[*numpo-1]<numbuf[*numpo])
              numbuf[*numpo-1]=numbuf[*numpo];
            break;
          case 'l':
            if (numbuf[*numpo-1]==numbuf[*numpo]) numbuf[*numpo-1]=1;
            else numbuf[*numpo-1]=0;
            break;
          case 'm':
            if (numbuf[*numpo-1]!=numbuf[*numpo]) numbuf[*numpo-1]=1;
            else numbuf[*numpo-1]=0;
            break;
          case 'n':
            if (numbuf[*numpo-1]>=numbuf[*numpo]) numbuf[*numpo-1]=1;
            else numbuf[*numpo-1]=0;
            break;
          case 'o':
            if (numbuf[*numpo-1]>numbuf[*numpo]) numbuf[*numpo-1]=1;
            else numbuf[*numpo-1]=0;
            break;
          case 'p':
            if (numbuf[*numpo-1]<=numbuf[*numpo]) numbuf[*numpo-1]=1;
            else numbuf[*numpo-1]=0;
            break;
          case 'q':
            if (numbuf[*numpo-1]<numbuf[*numpo]) numbuf[*numpo-1]=1;
            else numbuf[*numpo-1]=0;
            break;
          case 'r':
            if (numbuf[*numpo]==0) numbuf[*numpo]=1;
            else numbuf[*numpo]=0;
            break;
          case 's':
            if ((numbuf[*numpo-1]!=0) | (numbuf[*numpo]!=0))
              numbuf[*numpo-1]=1;
            else numbuf[*numpo-1]=0;
            break;
          case 't':
            if ((numbuf[*numpo-1]!=0) & (numbuf[*numpo]!=0))
              numbuf[*numpo-1]=1;
            else numbuf[*numpo-1]=0;
            break;
          case 'u':
            if ((numbuf[*numpo-1]!=0) ^ (numbuf[*numpo]!=0))
              numbuf[*numpo-1]=1;
            else numbuf[*numpo-1]=0;
            break;
          case 'v':
            numbuf[*numpo]=frand(numbuf[*numpo]);
            break;
          case 'w':
            if ((numbuf[*numpo-1]-(int )numbuf[*numpo-1]!=0)
            || (fabs(numbuf[*numpo-1])>1000)) numstat[*numpo-1]=MERR;
            else if (legendre((int )numbuf[*numpo-1],numbuf[*numpo],
                      &(numbuf[*numpo-1]))) numstat[*numpo-1]=MERR;
            break;
          case 'x':
            if ((numbuf[*numpo-2]-(int )numbuf[*numpo-2]!=0)
            || (fabs(numbuf[*numpo-2])>1000)) numstat[*numpo-2]=MERR;
            else if (laguer((int )numbuf[*numpo-2],numbuf[*numpo-1],
                 numbuf[*numpo],&(numbuf[*numpo-2]))) numstat[*numpo-2]=MERR;
            break;
          case 'y':
            if ((numbuf[*numpo-1]-(int )numbuf[*numpo-1]!=0)
            || (fabs(numbuf[*numpo-1])>1000)) numstat[*numpo-1]=MERR;
            else if (hermite((int )numbuf[*numpo-1],numbuf[*numpo],
                      &(numbuf[*numpo-1]))) numstat[*numpo-1]=MERR;
            break;
          case 'z':
            if ((numbuf[*numpo-1]-(int )numbuf[*numpo-1]!=0)
            || (fabs(numbuf[*numpo-1])>1000)) numstat[*numpo-1]=MERR;
            else if (chebyshev((int )numbuf[*numpo-1],numbuf[*numpo],
                      &(numbuf[*numpo-1]))) numstat[*numpo-1]=MERR;
            break;
          case '{':
            if ((numbuf[*numpo-1]-(int )numbuf[*numpo-1]!=0)
            || (numbuf[*numpo-1]<0) || (numbuf[*numpo-1]>(MEMORYNUM-1)))
              numstat[*numpo-1]=MERR;
            else {
              memory[(int )numbuf[*numpo-1]]=numbuf[*numpo];
              memorystat[(int )numbuf[*numpo-1]]=numstat[*numpo];
              numbuf[*numpo-1]=numbuf[*numpo];
              numstat[*numpo-1]=numstat[*numpo];
            }
            break;
          case '}':
            if ((numbuf[*numpo]-(int )numbuf[*numpo]!=0)
            || (numbuf[*numpo]<0) || (numbuf[*numpo]>(MEMORYNUM-1)))
              numstat[*numpo]=MERR;
            else {
              numstat[*numpo]=memorystat[(int )numbuf[*numpo]];
              numbuf[*numpo]=memory[(int )numbuf[*numpo]];
            }
            break;
          case '"':
            if ((numbuf[*numpo-1]-(int )numbuf[*numpo-1]!=0)
            || (numbuf[*numpo-1]<0) || (numbuf[*numpo-1]>3))
              numstat[*numpo-1]=MERR;
            else {
              if (numbuf[*numpo]>255) numbuf[*numpo]=255;
              else if (numbuf[*numpo]<0) numbuf[*numpo]=0;
              if (((int )numbuf[*numpo-1])!=3)
                color[(int )numbuf[*numpo-1]]=nround(numbuf[*numpo]);
              else {
                color[0]=nround(numbuf[*numpo]);
                color[1]=nround(numbuf[*numpo]);
                color[2]=nround(numbuf[*numpo]);
              }
            }
            numbuf[*numpo-1]=numbuf[*numpo];
            break;
          case (char )0xc0:
            while (numbuf[*numpo-2]<0) numbuf[*numpo-2]+=1;
            while (numbuf[*numpo-1]<0) numbuf[*numpo-1]+=1;
            while (numbuf[*numpo]<0) numbuf[*numpo]+=1;
            while (numbuf[*numpo-2]>1) numbuf[*numpo-2]-=1;
            while (numbuf[*numpo-1]>1) numbuf[*numpo-1]-=1;
            while (numbuf[*numpo]>1) numbuf[*numpo]-=1;
            color[0]=nround(numbuf[*numpo-2]*255);
            color[1]=nround(numbuf[*numpo-1]*255);
            color[2]=nround(numbuf[*numpo]*255);
            break;
          case (char )0xc1:
            while (numbuf[*numpo-2]<0) numbuf[*numpo-2]+=1;
            while (numbuf[*numpo-1]<0) numbuf[*numpo-1]+=1;
            while (numbuf[*numpo]<0) numbuf[*numpo]+=1;
            while (numbuf[*numpo-2]>1) numbuf[*numpo-2]-=1;
            while (numbuf[*numpo-1]>1) numbuf[*numpo-1]-=1;
            while (numbuf[*numpo]>1) numbuf[*numpo]-=1;
            HSB2RGB(numbuf[*numpo-2],numbuf[*numpo-1],numbuf[*numpo],
                    &(color[0]),&(color[1]),&(color[2]));
            break;
          case '\'':
            if (numbuf[*numpo]<0) numstat[*numpo]=MERR;
            else *marksize=nround(numbuf[*numpo]);
            break;
          case '$':
            if (numbuf[*numpo]<0) numstat[*numpo]=MERR;
            else *marktype=nround(numbuf[*numpo]);
            break;
          case '<':
            numbuf[*numpo-2]
              =mjd(numbuf[*numpo-2],numbuf[*numpo-1],numbuf[*numpo]);
            break;
          case '>':
            if (qinv1(numbuf[*numpo],&(numbuf[*numpo]))) numstat[*numpo]=MERR;
            break;
          case (char )0x80: case (char )0x81:
          case (char )0x82: case (char )0x83:
          case (char )0x84: case (char )0x85:
          case (char )0x86: case (char )0x87:
          case (char )0x88: case (char )0x89:
            i=opbuf[*oppo]-(char )0x80;
            sumdata[i]=sumdata[i]+numbuf[*numpo];
            numbuf[*numpo]=sumdata[i];
            if (sumstat[i]==MNOERR) sumstat[i]=numstat[*numpo];
            numstat[*numpo]=sumstat[i];
            break;
          case (char )0x90: case (char )0x91:
          case (char )0x92: case (char )0x93:
          case (char )0x94: case (char )0x95:
          case (char )0x96: case (char )0x97:
          case (char )0x98: case (char )0x99:
            i=opbuf[*oppo]-(char )0x90;
            if (difstat[i]==MUNDEF) {
              difdata[i]=numbuf[*numpo];
              difstat[i]=numstat[*numpo];
              numstat[*numpo]=MUNDEF;
            } else {
              x=numbuf[*numpo]-difdata[i];
              difdata[i]=numbuf[*numpo];
              numbuf[*numpo]=x;
              difstat[i]=numstat[*numpo];
              if (difstat[i]>numstat[*numpo]) numstat[*numpo]=difstat[i];
            }
            break;
          case (char )0xa0: case (char )0xa1: case (char )0xa2:
            i=opbuf[*oppo]-(char )0xa0;
            if (i==0) ufcode=ufcodef;
            else if (i==1) ufcode=ufcodeg;
            else ufcode=ufcodeh;
            for (i=0;i<MEMORYNUM;i++) {
              umemory[i]=0;
              umemorystat[i]=MNOERR;
            }
            numstat[*numpo-2]=calculate(ufcode,1,
                            numbuf[*numpo-2],numstat[*numpo-2],
                            numbuf[*numpo-1],numstat[*numpo-1],
                            numbuf[*numpo],numstat[*numpo],
                            0,0,0,0,0,0,0,0,0,0,0,0,0,0,
                            NULL,NULL,
                            umemory,umemorystat,
                            NULL,NULL,
                            NULL,NULL,
                            NULL,NULL,NULL,
                            ufcodef,ufcodeg,ufcodeh,
                            0,NULL,NULL,NULL,0,
                            &(numbuf[*numpo-2]));
            break;
          }
      }
    }
    (*numpo)+=-argnum+1;
    (*oppo)--;
  }
}

int 
calculate(char *code,
              int first,
              double x,char xstat,
              double y,char ystat,
              double z,char zstat,
              double minx,char minxstat,
              double maxx,char maxxstat,
              double miny,char minystat,
              double maxy,char maxystat,
              int num,
              double sumx,double sumy,double sumxx,double sumyy,double sumxy,
              double *data,char *datastat,
              double *memory,char *memorystat,
              double *sumdata,char *sumstat,
              double *difdata,char *difstat,
              int *color,int *marksize,int *marktype,
              char *ufcodef,char *ufcodeg,char *ufcodeh,
              int fnum,int *needfile,double *fdata,char *fdatastat,int file,
              double *value)
{
    double numbuf[20];
    char numstat[20];
    char opbuf[20];
    char prebuf[20];
    char argbuf[20];
    char noexpand[21];
    char fnoexpand[21];
    int po,numpo,oppo,ifpo,forpo,i,dim,id,maxdim2;
    char numeric;
    char argnum;
    struct {
      int mem;
      double end;
      double step;
      int po;
    } forstat[21];

    *value=0;
    if (code==NULL) return MSERR;
    numpo=-1;
    oppo=-1;
    forpo=0;
    ifpo=0;
    noexpand[0]=FALSE;
    fnoexpand[0]=FALSE;
    numeric=FALSE;
    po=0;
    while (code[po]!='_') {
        if ((code[po]=='-') && !numeric) code[po]='&';
	if ((code[po]=='+') && !numeric) code[po]='`';
        switch (code[po]) {
        case '#':
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          memcpy(numbuf+numpo,code+po+1,sizeof(double));
          numstat[numpo]=MNOERR;
          po+=sizeof(double)+1;
          numeric=TRUE;
          break;
        case '~':
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=0;
          numstat[numpo]=MNAN;
          po++;
          numeric=TRUE;
          break;
        case '@':
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=0;
          numstat[numpo]=MUNDEF;
          po++;
          numeric=TRUE;
          break;
        case (char )0xb0:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=0;
          numstat[numpo]=MSCONT;
          po++;
          numeric=TRUE;
          break;
        case (char )0xb1:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=0;
          numstat[numpo]=MSBREAK;
          po++;
          numeric=TRUE;
          break;
        case 'X':
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=x;
          numstat[numpo]=xstat;
          po++;
          numeric=TRUE;
          break;
        case 'Y':
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=y;
          numstat[numpo]=ystat;
          po++;
          numeric=TRUE;
          break;
        case 'Z':
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=z;
          numstat[numpo]=zstat;
          po++;
          numeric=TRUE;
          break;
        case (char )0xb2:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=first;
          numstat[numpo]=MNOERR;
          po++;
          numeric=TRUE;
          break;
        case (char )0xb3:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=file;
          numstat[numpo]=MNOERR;
          po++;
          numeric=TRUE;
          break;
        case '0': case '1': case '2': case '3': case '4':
        case '5': case '6': case '7': case '8': case '9':
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          dim=(code[po]-'0')*100+(code[po+1]-'0')*10+(code[po+2]-'0');
          numbuf[numpo]=data[dim];
          numstat[numpo]=datastat[dim];
          po+=3;
          numeric=TRUE;
          break;
        case '|':
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          id=(code[po+1]-'0')*10+(code[po+2]-'0');
          dim=(code[po+3]-'0')*100+(code[po+4]-'0')*10+(code[po+5]-'0');
          maxdim2=id*1000+dim;
          for (i=0;i<fnum;i++)
            if (needfile[i]==maxdim2) break;
          if (i==fnum) {
            numbuf[numpo]=0;
            numstat[numpo]=MERR;
          } else {
            numbuf[numpo]=fdata[i];
            numstat[numpo]=fdatastat[i];
          }
          po+=6;
          numeric=TRUE;
          break;
        case (char )0xd0:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=num;
          numstat[numpo]=MNOERR;
          po++;
          numeric=TRUE;
          break;
        case (char )0xd1:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=minx;
          numstat[numpo]=minxstat;
          po++;
          numeric=TRUE;
          break;
        case (char )0xd2:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=miny;
          numstat[numpo]=minystat;
          po++;
          numeric=TRUE;
          break;
        case (char )0xd3:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=maxx;
          numstat[numpo]=maxxstat;
          po++;
          numeric=TRUE;
          break;
        case (char )0xd4:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=maxy;
          numstat[numpo]=maxystat;
          po++;
          numeric=TRUE;
          break;
        case (char )0xd5:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=sumx;
          numstat[numpo]=MNOERR;
          po++;
          numeric=TRUE;
          break;
        case (char )0xd6:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=sumy;
          numstat[numpo]=MNOERR;
          po++;
          numeric=TRUE;
          break;
        case (char )0xd7:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=sumxx;
          numstat[numpo]=MNOERR;
          po++;
          numeric=TRUE;
          break;
        case (char )0xd8:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=sumyy;
          numstat[numpo]=MNOERR;
          po++;
          numeric=TRUE;
          break;
        case (char )0xd9:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          numbuf[numpo]=sumxy;
          numstat[numpo]=MNOERR;
          po++;
          numeric=TRUE;
          break;
        case (char )0xda:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          if (num==0) numstat[numpo]=MERR;
          else {
            numbuf[numpo]=sumx/num;
            numstat[numpo]=MNOERR;
          }
          po++;
          numeric=TRUE;
          break;
        case (char )0xdb:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          if (num==0) numstat[numpo]=MERR;
          else {
            numbuf[numpo]=sumy/num;
            numstat[numpo]=MNOERR;
          }
          po++;
          numeric=TRUE;
          break;
        case (char )0xdc:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          if (num==0) numstat[numpo]=MERR;
          else {
            numbuf[numpo]=sqrt(sumxx/num-(sumx/num)*(sumx/num));
            numstat[numpo]=MNOERR;
          }
          po++;
          numeric=TRUE;
          break;
        case (char )0xdd:
          if (numeric) return MSERR;
          numpo++;
          if (numpo>=20) return MSERR;
          if (num==0) numstat[numpo]=MERR;
          else {
            numbuf[numpo]=sqrt(sumyy/num-(sumy/num)*(sumy/num));
            numstat[numpo]=MNOERR;
          }
          po++;
          numeric=TRUE;
          break;
        case '`':
        case '&':
          if (numeric) return MSERR;
          oppo++;
          if (oppo>=20) return MSERR;
          opbuf[oppo]=code[po];
          prebuf[oppo]=5;
          argbuf[oppo]=1;
          po++;
          break;
        case 'A': case 'B': case 'C': case 'D': case 'E': case 'F': case 'G':
        case 'H': case 'I': case 'J': case 'K': case 'L': case 'M': case 'N':
        case 'O': case 'P': case 'Q': case 'R': case 'S': case 'T': case 'U':
        case 'V': case 'W':
        case 'a': case 'b': case 'c': case 'd': case 'e': case 'f': case 'g':
        case 'h': case 'i': case 'j': case 'k': case 'l': case 'm': case 'n':
        case 'o': case 'p': case 'q': case 'r': case 's': case 't': case 'u':
        case 'v': case 'w': case 'x': case 'y': case 'z':
        case '{': case '}': case '"': case '\'': case '$': case '?': case ':':
        case '<': case '>':
        case (char )0x80: case (char )0x81: 
        case (char )0x82: case (char )0x83: 
        case (char )0x84: case (char )0x85:
        case (char )0x86: case (char )0x87:
        case (char )0x88: case (char )0x89:
        case (char )0x90: case (char )0x91: 
        case (char )0x92: case (char )0x93: 
        case (char )0x94: case (char )0x95:
        case (char )0x96: case (char )0x97:
        case (char )0x98: case (char )0x99:
        case (char )0xa0: case (char )0xa1: case (char )0xa2:
        case (char )0xc0: case (char )0xc1:
          if (numeric) return MSERR;
          oppo++;
          if (oppo>=20) return MSERR;
          opbuf[oppo]=code[po];
          prebuf[oppo]=5;
          argbuf[oppo]=code[po+1]-'0';
          po+=2;
          break;
        case '!':
          if (!numeric) return MSERR;
          mathexpand(5,&oppo,&numpo,&ifpo,&forpo,numbuf,numstat,
                     opbuf,prebuf,argbuf,noexpand,fnoexpand,
                     memory,memorystat,sumdata,sumstat,difdata,difstat,
                     color,marksize,marktype,
                     ufcodef,ufcodeg,ufcodeh);
          oppo++;
          if (oppo>=20) return MSERR;
          opbuf[oppo]=code[po];
          prebuf[oppo]=5;
          argbuf[oppo]=1;
          po++;
          numeric=TRUE;
          break;
        case '^':
          if (!numeric) return MSERR;
          mathexpand(4,&oppo,&numpo,&ifpo,&forpo,numbuf,numstat,
                     opbuf,prebuf,argbuf,noexpand,fnoexpand,
                     memory,memorystat,sumdata,sumstat,difdata,difstat,
                     color,marksize,marktype,
                     ufcodef,ufcodeg,ufcodeh);
          oppo++;
          if (oppo>=20) return MSERR;
          opbuf[oppo]=code[po];
          prebuf[oppo]=4;
          argbuf[oppo]=1;
          po++;
          numeric=FALSE;
          break;
        case '*': case '/': case '\\':
          if (!numeric) return MSERR;
          mathexpand(3,&oppo,&numpo,&ifpo,&forpo,numbuf,numstat,
                     opbuf,prebuf,argbuf,noexpand,fnoexpand,
                     memory,memorystat,sumdata,sumstat,difdata,difstat,
                     color,marksize,marktype,
                     ufcodef,ufcodeg,ufcodeh);
          oppo++;
          if (oppo>=20) return MSERR;
          opbuf[oppo]=code[po];
          prebuf[oppo]=3;
          argbuf[oppo]=1;
          po++;
          numeric=FALSE;
          break;
        case '+': case '-':
          if (!numeric) return MSERR;
          mathexpand(2,&oppo,&numpo,&ifpo,&forpo,numbuf,numstat,
                     opbuf,prebuf,argbuf,noexpand,fnoexpand,
                     memory,memorystat,sumdata,sumstat,difdata,difstat,
                     color,marksize,marktype,
                     ufcodef,ufcodeg,ufcodeh);
          oppo++;
          if (oppo>=20) return MSERR;
          opbuf[oppo]=code[po];
          prebuf[oppo]=2;
          argbuf[oppo]=1;
          po++;
          numeric=FALSE;
          break;
        case ')':
          if (!numeric) return MSERR;
          mathexpand(1,&oppo,&numpo,&ifpo,&forpo,numbuf,numstat,
                     opbuf,prebuf,argbuf,noexpand,fnoexpand,
                     memory,memorystat,sumdata,sumstat,difdata,difstat,
                     color,marksize,marktype,
                     ufcodef,ufcodeg,ufcodeh);
          if (oppo==-1) return MSERR;
          if (argbuf[oppo]!=1) return MSERR;
          if ((oppo>4) && (opbuf[oppo]==',') && (opbuf[oppo-1]==',') 
          && (opbuf[oppo-2]==',') && (opbuf[oppo-3]==',') 
          && (opbuf[oppo-4]=='(') && (opbuf[oppo-5]==':')) {
            if (memorystat[forstat[forpo].mem]!=MNOERR) return MSERR;
            memory[forstat[forpo].mem]+=forstat[forpo].step;
            if (forstat[forpo].step==0) fnoexpand[forpo]=FALSE;
            else if (forstat[forpo].step>0) {
              if (memory[forstat[forpo].mem]<=forstat[forpo].end)
                fnoexpand[forpo]=FALSE;
              else fnoexpand[forpo]=TRUE;
            } else if (forstat[forpo].step<0) {
              if (memory[forstat[forpo].mem]>=forstat[forpo].end)
                fnoexpand[forpo]=FALSE;
              else fnoexpand[forpo]=TRUE;
            }
            if ((!fnoexpand[forpo]) && (!ninterrupt())) {
              po=forstat[forpo].po+1;
              numpo--;
              numeric=FALSE;
              break;
            }
          }
          while (opbuf[oppo]==',') oppo--;
          if (opbuf[oppo]=='(') oppo--;
          po++;
          numeric=TRUE;
          break;
        case ',':
          if (!numeric) return MSERR;
          mathexpand(1,&oppo,&numpo,&ifpo,&forpo,numbuf,numstat,
                     opbuf,prebuf,argbuf,noexpand,fnoexpand,
                     memory,memorystat,sumdata,sumstat,difdata,difstat,
                     color,marksize,marktype,
                     ufcodef,ufcodeg,ufcodeh);
          if (oppo==-1) return MSERR;
          if (argbuf[oppo]<1) return MSERR;

          if ((oppo>0) && (opbuf[oppo]=='(') && (opbuf[oppo-1]=='?')) {
            ifpo++;
            if (ifpo>20) return MSERR;
            if ((numpo>-1) && (numstat[numpo]==MNOERR)
            && (numbuf[numpo]!=0) && (!noexpand[ifpo-1])) 
              noexpand[ifpo]=FALSE;
            else noexpand[ifpo]=TRUE;
          }
          if ((oppo>1) && (opbuf[oppo]==',') 
          && (opbuf[oppo-1]=='(') && (opbuf[oppo-2]=='?')) {
            if ((numpo>0) && (numstat[numpo-1]==MNOERR) 
            && (numbuf[numpo-1]==0) && (!noexpand[ifpo-1])) 
              noexpand[ifpo]=FALSE;
            else noexpand[ifpo]=TRUE;
          }

          if ((oppo>0) && (opbuf[oppo]=='(') && (opbuf[oppo-1]==':')) {
            forpo++;
            if (forpo>20) return MSERR;
            forstat[forpo].mem=(int )numbuf[numpo];
            if ((forstat[forpo].mem<0) || (forstat[forpo].mem>(MEMORYNUM-1))) return MSERR;
          }
          if ((oppo>1) && (opbuf[oppo]==',') 
          && (opbuf[oppo-1]=='(') && (opbuf[oppo-2]==':')) {
            if (numstat[numpo]!=MNOERR) return MSERR;
            memory[forstat[forpo].mem]=numbuf[numpo];
            memorystat[forstat[forpo].mem]=MNOERR;
          }
          if ((oppo>2) && (opbuf[oppo]==',') && (opbuf[oppo-1]==',') 
          && (opbuf[oppo-2]=='(') && (opbuf[oppo-3]==':')) {
            if (numstat[numpo]!=MNOERR) return MSERR;
            forstat[forpo].end=numbuf[numpo];
          }
          if ((oppo>3) && (opbuf[oppo]==',') && (opbuf[oppo-1]==',') 
          && (opbuf[oppo-2]==',') 
          && (opbuf[oppo-3]=='(') && (opbuf[oppo-4]==':')) {
            if (numstat[numpo]!=MNOERR) return MSERR;
            forstat[forpo].step=numbuf[numpo];
            forstat[forpo].po=po;
            if (memorystat[forstat[forpo].mem]!=MNOERR) return MSERR;
            fnoexpand[forpo]=FALSE;
          }

          oppo++;
          if (oppo>=20) return MSERR;
          opbuf[oppo]=code[po];
          prebuf[oppo]=0;
          argbuf[oppo]=argbuf[oppo-1]-1;
          po++;
          numeric=FALSE;
          break;
        case '=':
          if (!numeric) return MSERR;
          mathexpand(1,&oppo,&numpo,&ifpo,&forpo,numbuf,numstat,
                     opbuf,prebuf,argbuf,noexpand,fnoexpand,
                     memory,memorystat,sumdata,sumstat,difdata,difstat,
                     color,marksize,marktype,
                     ufcodef,ufcodeg,ufcodeh);
          if (oppo>-1) return MSERR;
          if (numpo!=0) return MSERR;
          po++;
          *value=numbuf[0];
          numpo=-1;
          numeric=FALSE;
          break;
        case '(':
          if (numeric) return MSERR;
          if ((oppo==-1) || (opbuf[oppo]=='(') || (opbuf[oppo]==',')) argnum=1;
          else argnum=argbuf[oppo];
          oppo++;
          if (oppo>=20) return MSERR;
          opbuf[oppo]=code[po];
          prebuf[oppo]=0;
          argbuf[oppo]=argnum;
          po++;
          break;
        default:
          return MSERR;
        }
    }
    return numstat[0];
}


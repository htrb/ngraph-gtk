/* 
 * $Id: oprm.c,v 1.11 2009/03/26 06:25:15 hito Exp $
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
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <fcntl.h>
#include <sys/types.h>
#include <utime.h>
#include <time.h>
#ifndef WINDOWS
#include <unistd.h>
#else
#include <io.h>
#endif
#include "ngraph.h"
#include "mathfn.h"
#include "jnstring.h"
#include "ntime.h"
#include "object.h"
#include "gra.h"
#include "ioutil.h"
#include "odraw.h"

#define NAME "prm"
#define PARENT "object"
#define OVERSION  "1.00.00"

#define TRUE  1
#define FALSE 0

#define ERROPEN 100
#define ERRREAD 101
#define ERRPRM 102
#define ERREXIST 103

static char *prmerrorlist[]={
  "I/O error: open file",
  "I/O error: read file",
  "unsupported PRM file",
  "skip existing file",
};

#define ERRNUM (sizeof(prmerrorlist) / sizeof(*prmerrorlist))

static int 
prminit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{  
  int greek;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  greek=TRUE;
  if (_putobj(obj,"symbol_greek",inst,&greek)) return 1;
  return 0;
}

static int 
prmdone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static char *
pathconv(char *s,int ignorepath)
{
  int i;
  char *file;

  if (s==NULL) return NULL;
  if (s[0]=='\0') return NULL;
  if (ignorepath) {
    file=getbasename(s);
    memfree(s);
  } else file=s;
  changefilename(file);
  for (i=0;file[i]!='\0';i++) {
    if (niskanji(file[i])) i++;
    else file[i]=tolower(file[i]);
  }
  return file;
}


static struct 
narray *linestyleconv(int attr,int dottedsize)
{
  int i,dt[4],num;
  struct narray *array;

  if (attr==1) {
    dt[0]=dottedsize*10;
    dt[1]=dottedsize*10;
    num=2;
  } else if (attr==2) {
    dt[0]=dottedsize*30;
    dt[1]=dottedsize*10;
    num=2;
  } else if (attr==3) {
    dt[0]=dottedsize*30;
    dt[1]=dottedsize*10;
    dt[2]=dottedsize*10;
    dt[3]=dottedsize*10;
    num=4;
  } else {
    return NULL;
  }
  array=arraynew(sizeof(int));
  for (i=0;i<num;i++) arrayadd(array,&(dt[i]));
  return array;
}

static void 
addfontcontrol(char *s,int *po,int *fchange,int *jchange,
                    int *fff,int *ffb,int *ffj,int script)
{
  int j;

  j=*po;
  if (fchange[script]) {
    j+=sprintf(s+j,"%%F{%s}",fontchar[fff[script]+ffb[script]]);
    fchange[script]=FALSE;
  }
  if (jchange[script]) {
    j+=sprintf(s+j,"%%J{%s}",jfontchar[ffj[script]]);
    jchange[script]=FALSE;
  }
  *po=j;
}


static char *
remarkconv(char *s,int ff,int fj,int fb,int *fnameid,char *prmfile,
                 int greek)
/* %C is ignored
   %F ---> %F{}
   %d ---> %{system:0:date:1}
   %01C0101 ---> %{file:file:column:01 01}
*/
{
  int i,j,k,fchange[2],jchange[2],script,fff[2],ffj[2],ffb[2];
  char *s2;
  int file,line,col;
  unsigned int code;

  if ((s2=memalloc(strlen(s)+200))==NULL) return NULL;
  j=0;
  script=0;
  for (i=0;i<2;i++) {
    fff[i]=ff;
    ffj[i]=fj;
    ffb[i]=fb;
    fchange[i]=FALSE;
    jchange[i]=FALSE;
  }
  for (i=0;s[i]!='\0';i++) {
    if ((s[i]=='\\') && (s[i+1]!='\0')) {
      addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
      s2[j]=s[i];
      s2[j+1]=s[i+1];
      i++;
      j+=2;
    } else if (niskanji((unsigned char)s[i])) {
      addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
      code=njms2jis(((unsigned char)s[i] << 8)+(unsigned char)s[i+1]);
      if (((code>>8)==0x26) && greek) {
        for (k = 0; k < greektable_num(); k++) {
	  if (greektable[k].jis==(code & 0xff)) 
	    break;
	}
        if (k != greektable_num()) {
          j+=sprintf(s2+j,"%%F{%s}%c%%F{%s}%%J{%s}",
                     fontchar[12],greektable[k].symbol,
                     fontchar[fff[script]+ffb[script]],
                     jfontchar[ffj[script]]);
	} else {
          s2[j]=s[i];
          s2[j+1]=s[i+1];
          j+=2;
        }
      } else {
        s2[j]=s[i];
        s2[j+1]=s[i+1];
        j+=2;
      }
      i++;
    } else if (s[i]=='%') {
      if ((toupper(s[i+1])=='S') && isdigit(s[i+2]) && isdigit(s[i+3])) {
        s2[j]=s[i];
        s2[j+1]=s[i+1];
        s2[j+2]='{';
        s2[j+3]=s[i+2];
        s2[j+4]=s[i+3];
        s2[j+5]='}';
        i+=3;
        j+=6;
      } else if ((strchr("PXYpxy",s[i+1])!=NULL)
        && ((s[i+2]=='+') || (s[i+2]=='-'))
        && isdigit(s[i+3]) && isdigit(s[i+4])) {
        s2[j]=s[i];
        s2[j+1]=s[i+1];
        s2[j+2]='{';
        s2[j+3]=s[i+2];
        s2[j+4]=s[i+3];
        s2[j+5]=s[i+4];
        s2[j+6]='}';
        i+=4;
        j+=7;
      } else if ((toupper(s[i+1])=='F') && (strchr("THCthc",s[i+2])!=NULL)) {
        fchange[script]=TRUE;
        if (toupper(s[i+2]=='T')) fff[script]=0;
        else if (toupper(s[i+2]=='H')) fff[script]=4;
        else if (toupper(s[i+2]=='C')) fff[script]=8;
        i+=2;
      } else if ((toupper(s[i+1])=='J') && (strchr("GMgm",s[i+2])!=NULL)) {
        jchange[script]=TRUE;
        if (toupper(s[i+2]=='M')) ffj[script]=0;
        else if (toupper(s[i+2]=='G')) ffj[script]=1;
        i+=2;
      } else if (toupper(s[i+1])=='R') {
        fchange[script]=TRUE;
        ffb[script]=0;
        i++;
      } else if (toupper(s[i+1])=='B') {
        fchange[script]=TRUE;
        ffb[script]=1;
        i++;
      } else if (toupper(s[i+1])=='I') {
        fchange[script]=TRUE;
        ffb[script]=2;
        i++;
      } else if (toupper(s[i+1])=='O') {
        fchange[script]=TRUE;
        ffb[script]=3;
        i++;
      } else if (s[i+1]=='d') {
        addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
        j+=sprintf(s2+j,"%%{system:0:date:1}");
        i++;
      } else if (s[i+1]=='D') {
        addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
        j+=sprintf(s2+j,"%%{system:0:date:0}");
        i++;
      } else if (s[i+1]=='t') {
        addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
        j+=sprintf(s2+j,"%%{system:0:time:2}");
        i++;
      } else if (s[i+1]=='t') {
        addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
        j+=sprintf(s2+j,"%%{system:0:time:3}");
        i++;
      } else if (toupper(s[i+1])=='M') {
        addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
        j+=sprintf(s2+j,"%s",prmfile);
        i++;
      } else if ((toupper(s[i+1])=='C') && isdigit(s[i+2])) {
        i+=2;
      } else if (isdigit(s[i+1]) && isdigit(s[i+2])) {
        file=(s[i+1]-'0')*10+(s[i+2]-'0')-1;
        if (file==-1) file=0;
        if (toupper(s[i+3])=='N') {
          addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
          if (file<20) {
            if (s[i+3]=='N')
              j+=sprintf(s2+j,"%%{file:%d:file}",fnameid[file]);
            else
              j+=sprintf(s2+j,"%%{file:%d:basename}",fnameid[file]);
          }
          i+=3;
        } else if (s[i+3]=='d') {
          addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
          j+=sprintf(s2+j,"%%{file:%d:date:1}",fnameid[file]);
          i+=3;
        } else if (s[i+3]=='D') {
          addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
          j+=sprintf(s2+j,"%%{file:%d:date:0}",fnameid[file]);
          i+=3;
        } else if (s[i+3]=='t') {
          addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
          j+=sprintf(s2+j,"%%{file:%d:time:4}",fnameid[file]);
          i+=3;
        } else if (s[i+3]=='t') {
          addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
          j+=sprintf(s2+j,"%%{system:%d:time:3}",fnameid[file]);
          i+=3;
        } else if ((toupper(s[i+3])=='C') && isdigit(s[i+4])
        && isdigit(s[i+5]) && isdigit(s[i+6]) && isdigit(s[i+7])) {
          addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
          line=(s[i+4]-'0')*10+(s[i+5]-'0');
          col=(s[i+6]-'0')*10+(s[i+7]-'0');
          if (file<20)
            j+=sprintf(s2+j,"%%{file:%d:column:%d %d}",
                            fnameid[file],line,col);
          i+=7;
        }
      }
    } else {
      addfontcontrol(s2,&j,fchange,jchange,fff,ffb,ffj,script);
      if ((s[i]=='^') || (s[i]=='_')) script=1;
      else if (s[i]=='@') script=0;
      s2[j]=s[i];
      j++;
    }
  }
  s2[j]='\0';
  return s2;
}

static char *
mathconv(char *math)
/* INTEG ---> SUM
   F(,) ---> F(,,)
   NAN ---> CONT
   UNDEF ---> BREAK
*/
{
  int i,j;
  int f,g,fb[50],gb[50];
  char *m;

  if ((m=memalloc(strlen(math)+100))==NULL) return NULL;
  j=0;
  f=g=0;
  for (i=0;math[i]!='\0';i++) {
    if (strncmp(math+i,"NAN",3)==0) {
      strcpy(m+j,"CONT");
      i+=2;
      j+=4;
    } else if (strncmp(math+i,"NONE",3)==0) {
      strcpy(m+j,"BREAK");
      i+=3;
      j+=5;
    } else if (strncmp(math+i,"INTEG",5)==0) {
      strcpy(m+j,"SUM");
      i+=4;
      j+=3;
    } else if (strncmp(math+i,"DIF(",4)==0) {
      strcpy(m+j,"DIF");
      i+=2;
      j+=3;
    } else if (strncmp(math+i,"IF(",3)==0) {
      strcpy(m+j,"IF");
      i++;
      j+=2;
    } else if (strncmp(math+i,"LOG(",4)==0) {
      strcpy(m+j,"LOG");
      i+=2;
      j+=3;
    } else if (strncmp(math+i,"F(",2)==0) {
      f++;
      fb[f]=0;
    } else if (strncmp(math+i,"G(",2)==0) {
      g++;
      gb[g]=0;
    } else if (math[i]=='(') {
      if (f) fb[f]++;
      if (g) gb[f]++;
      m[j]=math[i];
      j++;
    } else if (math[i]==')') {
      if (f) {
        fb[f]--;
        if (fb[f]==0) {
          m[j]=',';
          m[j+1]='0';
          f--;
          j+=2;
        }
      }
      if (g) {
        gb[g]--;
        if (gb[g]==0) {
          m[j]=',';
          m[j+1]='0';
          g--;
          j+=2;
        }
      }
      m[j]=math[i];
      j++;
    } else if ((math[i]!=' ') && (math[i]!='\t')) {
      m[j]=math[i];
      j++;
    }
  }
  m[j]='\0';
  if (m[0]=='\0') {
    memfree(m);
    m=NULL;
  }
  return m;
}

#define BUFSIZE 512


static int 
prmloadline(struct objlist *obj,char *file,FILE *fp,char *buf,int err)
{
  if (fgetnline(fp,buf,BUFSIZE)) {
    if (err) error2(obj,ERRREAD,file);
    return -1;
  }
  return 0;
}

static int 
sscanf2(char *buffer,char *format,...)
{
  va_list ap;
  int i,num;
  int *d;
  double *e;
  char *s;
  char *endptr;

  va_start(ap,format);
  s=buffer;
  num=0;
  i=0;
  while (format[i]!='\0') {
    if (format[i]=='d') {
      d=va_arg(ap,int *);
      *d=strtol(s,&endptr,10);
      num++;
      if (endptr[0]=='\0') break;
      s=endptr;
    } else if (format[i]=='e') {
      e=va_arg(ap,double *);
      *e=strtod(s,&endptr);
      num++;
      if (endptr[0]=='\0') break;
      s=endptr;
    }
    i++;
  }
  va_end(ap);
  return num;
}

char buf[BUFSIZE];
char buf2[BUFSIZE];

static int 
prmload(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  char *file;
  FILE *fp,*fp2;
  int i,j,k,filetype,num,greek,ignorepath;
  struct objlist *fobj,*fitobj,*aobj,*agdobj;
  struct objlist *lobj,*pobj,*mobj,*tobj,*robj,*cobj;
  struct objlist *mgobj,*gobj,*cmobj;
  int fid,fidroot,fitid,aid,agdid,lid,pid,mid,tid,rid,rid2,cid;
  int mgid,gid,cmid;
  char *s,*s2;
  int d1,d2,d3,d4,d5,d6,d7,d8,d9,d10,R,G,B,gx[11],gy[11];
  double f1,f2,f3,f4;
  int type,intp,mark,fittype;
  struct narray *iarray;
  int graphtype,vx,vy,szx,szy,dmode[3];
  int fff[7],ffj[7],ffb[7],ffs[7],ffp[7],ffR[7],ffG[7],ffB[7];
  char *argv2[5];
  int anameid[4],anameoid[4];
  int fitnameid[20],nameid;
  int fnameid[20];
  int masknum,movenum;
  struct narray *mask,*move,*movex,*movey;
  double datax,datay,data2,data3;
  int statx,staty,stat2,stat3,line,plottype;
  char str4[5],*endptr;
  int hiddenaxis[4];
  int setaxis[4],sccros[4],scauto[4];
  int scnum,scstart,scstep,sczero,scplus,scperiod,scposs,scpose;
  int scdir,sclr,sclog,scposx,scposy,scfull;
  char scfig[5];
  int la0,la1,la2,la3,la4,lw0,lw1,lw2,lw3,lw4,lc0,lc1,lc4;
  int ll1,ll2,ll3;
  double amax,amin,ainc;
  int atype,aid2[4],posx,posy;
  char format[10],*EOD;
  char *filename;
  time_t ftime;
  struct utimbuf tm;
  int mkdata;
  struct narray group;
  int gtype;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  _getobj(obj,"file",inst,&file);
  _getobj(obj,"symbol_greek",inst,&greek);
  _getobj(obj,"ignore_path",inst,&ignorepath);
  str4[4]='\0';
  if ((fobj=getobject("file"))==NULL) return 1;
  if ((fitobj=getobject("fit"))==NULL) return 1;
  if ((aobj=getobject("axis"))==NULL) return 1;
  if ((agdobj=getobject("axisgrid"))==NULL) return 1;
  if ((lobj=getobject("line"))==NULL) return 1;
  if ((pobj=getobject("polygon"))==NULL) return 1;
  if ((mobj=getobject("mark"))==NULL) return 1;
  if ((robj=getobject("rectangle"))==NULL) return 1;
  if ((tobj=getobject("text"))==NULL) return 1;
  if ((cobj=getobject("curve"))==NULL) return 1;
  if ((mgobj=getobject("merge"))==NULL) return 1;
  if ((gobj=getobject("gra"))==NULL) return 1;
  if (file==NULL) return 0;
  if ((fp=nfopen(file,"rt"))==NULL) {
    error2(obj,ERROPEN,file);
    return 1;
  }
  if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
  if (strcmp(buf," Ngraph  ver 5.2  PRM file")==0) filetype=2;
  else if (strcmp(buf," Ngraph ver. 5.3  PRM file")==0) filetype=3;
  else if (strcmp(buf," Ngraph  ver 5.2  DPM file")==0) filetype=12;
  else if (strcmp(buf," Ngraph ver. 5.3  DPM file")==0) filetype=13;
  else {
    error2(obj,ERRPRM,file);
    goto errexit;
  }
  if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
  if (sscanf2(buf,"%d",(int *)&num)!=1) {
    error2(obj,ERRPRM,file);
    goto errexit;
  }

  anameid[0]=anameid[1]=anameid[2]=anameid[3]=-1;
  anameoid[0]=anameoid[1]=anameoid[2]=anameoid[3]=-1;

/* FilePar */

  for (i=0;i<num;i++) {
    fitnameid[i]=-1;
    if ((fid=newobj(fobj))==-1) goto errexit;
    fnameid[i]=fid;
    if (i==0) fidroot=fid;
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if ((s=memalloc(strlen(buf)+1))==NULL) goto errexit;
    strcpy(s,buf);
    s=pathconv(s,ignorepath);
    putobj(fobj,"file",fid,s);
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%d%d%d%d%d%d%d%d%d%d",
               (int *)&d1,(int *)&d2,(int *)&d3,(int *)&d4,(int *)&d5,
               (int *)&d6,(int *)&d7,(int *)&d8,(int *)&d9,(int *)&d10)!=10) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    putobj(fobj,"x",fid,&d1);
    putobj(fobj,"y",fid,&d2);
    if (d3==0) {
      if (anameid[0]==-1) {
        if ((aid=newobj(aobj))==-1) goto errexit;
        anameid[0]=aid;
        getobj(aobj,"oid",aid,0,NULL,&(anameoid[0]));
      }
      if ((s=memalloc(30))==NULL) goto errexit;
      sprintf(s,"axis:%d",anameid[0]);
    } else {
      if (anameid[2]==-1) {
        if ((aid=newobj(aobj))==-1) goto errexit;
        anameid[2]=aid;
        getobj(aobj,"oid",aid,0,NULL,&(anameoid[2]));
      }
      if ((s=memalloc(30))==NULL) goto errexit;
      sprintf(s,"axis:%d",anameid[2]);
    }
    putobj(fobj,"axis_x",fid,s);
    if (d4==0) {
      if (anameid[1]==-1) {
        if ((aid=newobj(aobj))==-1) goto errexit;
        anameid[1]=aid;
        getobj(aobj,"oid",aid,0,NULL,&(anameoid[1]));
      }
      if ((s=memalloc(30))==NULL) goto errexit;
      sprintf(s,"axis:%d",anameid[1]);
    } else {
      if (anameid[3]==-1) {
        if ((aid=newobj(aobj))==-1) goto errexit;
        anameid[3]=aid;
        getobj(aobj,"oid",aid,0,NULL,&(anameoid[3]));
      }
      if ((s=memalloc(30))==NULL) goto errexit;
      sprintf(s,"axis:%d",anameid[3]);
    }
    putobj(fobj,"axis_y",fid,s);
    if (d5==0) type=1;
    else if (d5==1) type=2;
    else if (d5<=6) {
      type=3;
      if ((d5==2) || (d5==4)) intp=0;
/* "Spline" is "Spline S" */
      else if (d5==3) intp=1;
      else if (d5==5) intp=2;
      else if (d5==6) intp=3;
      putobj(fobj,"interpolation",fid,&intp);
    } else if (d5<=27) {
      type=0;
      switch (d5) {
      case 7: mark=-1; break;
      case 8: case 9: mark=0; break;
      case 10: mark=1; break;
      case 11: mark=2; break;
      case 12: mark=3; break;
      case 13: mark=80; break;
      case 14: mark=10; break;
      case 15: mark=11; break;
      case 16: mark=12; break;
      case 17: mark=20; break;
      case 18: mark=21; break;
      case 19: mark=22; break;
      case 20: mark=30; break;
      case 21: mark=31; break;
      case 22: mark=32; break;
      case 23: mark=40; break;
      case 24: mark=41; break;
      case 25: mark=42; break;
      case 26: mark=70; break;
      case 27: mark=71; break;
      }
      putobj(fobj,"mark_type",fid,&mark);
    } else if (d5==28) type=18;
    else if (d5==29) type=16;
    else if (d5==30) type=14;
    else if (d5==31) type=17;
    else if (d5==32) type=15;
    else if (d5==33) type=13;
    else if (d5==34) type=12;
    else if (d5==35) type=11;
    else if (d5==36) type=4;
    else if (d5==37) type=8;
    else if (d5==38) type=7;
    else if (d5==39) type=6;
    else if (d5==40) type=10;
    else if (d5==41) type=9;
    else {
      type=19;
      if ((fitid=newobj(fitobj))==-1) goto errexit;
      getobj(fitobj,"oid",fitid,0,NULL,&nameid);
      if (d5==51) fitnameid[i]=fitid;
      if ((s=memalloc(30))==NULL) goto errexit;
      sprintf(s,"fit:^%d",nameid);
      putobj(fobj,"fit",fid,s);
      if (d5<=47) {
        fittype=0;
        putobj(fitobj,"type",fitid,&fittype);
        fittype=d5-41;
        putobj(fitobj,"poly_dimension",fitid,&fittype);
      } else {
        fittype=d5-47;
        putobj(fitobj,"type",fitid,&fittype);
      }
    }
    putobj(fobj,"type",fid,&type);
    iarray=linestyleconv(d6,d10);
    putobj(fobj,"line_style",fid,iarray);
    putobj(fobj,"line_width",fid,&d7);
    R=(d8 & 4)?255:0;
    G=(d8 & 2)?255:0;
    B=(d8 & 1)?255:0;
    putobj(fobj,"R",fid,&R);
    putobj(fobj,"G",fid,&G);
    putobj(fobj,"B",fid,&B);
    R=G=B=255;
    putobj(fobj,"R2",fid,&R);
    putobj(fobj,"G2",fid,&G);
    putobj(fobj,"B2",fid,&B);
    d9*=10;
    if (d5==8) d9/=2;
    putobj(fobj,"mark_size",fid,&d9);
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%d%d%d%d%d",(int *)&d1,(int *)&d2,(int *)&d3,
                                (int *)&d4,(int *)&d5)!=5) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    putobj(fobj,"head_skip",fid,&d1);
    putobj(fobj,"read_step",fid,&d2);
    putobj(fobj,"final_line",fid,&d3);
    putobj(fobj,"smooth_x",fid,&d4);
    putobj(fobj,"smooth_y",fid,&d5);

    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%d%le%le",(int *)&d1,(double *)&f1,(double *)&f2)!=3) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    if (type==19) {
      putobj(fitobj,"through_point",fitid,&d1);
      putobj(fitobj,"point_x",fitid,&f1);
      putobj(fitobj,"point_y",fitid,&f2);
    }
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if ((strlen(buf)!=0) && (strcmp(buf,"X")!=0)) {
      s=mathconv(buf);
      putobj(fobj,"math_x",fid,s);
    }
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if ((strlen(buf)!=0) && (strcmp(buf,"Y")!=0)) {
      s=mathconv(buf);
      putobj(fobj,"math_y",fid,s);
    }
  }

/* LSQPar */

  if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
  if (sscanf2(buf,"%le",(double *)&f1)!=1) {
    error2(obj,ERRPRM,file);
    goto errexit;
  }
  for (i=0;i<num;i++) if (fitnameid[i]!=-1) {
    putobj(fitobj,"converge",fitnameid[i],&f1);
  }
  if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
  d1=(buf[0]=='Y')?TRUE:FALSE;
  for (i=0;i<num;i++) if (fitnameid[i]!=-1) {
    putobj(fitobj,"derivative",fitnameid[i],&d1);
  }

  for (i=0;i<7;i++) {
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%le",(double *)&f1)!=1) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    for (j=0;j<num;j++) if (fitnameid[j]!=-1) {
      fitid=fitnameid[j];
      if (i==0) {
        putobj(fitobj,"parameter0",fitid,&f1);
      } else if (i==1) {
        putobj(fitobj,"parameter1",fitid,&f1);
      } else if (i==2) {
        putobj(fitobj,"parameter2",fitid,&f1);
      } else if (i==3) {
        putobj(fitobj,"parameter3",fitid,&f1);
      } else if (i==4) {
        putobj(fitobj,"parameter4",fitid,&f1);
      } else if (i==5) {
        putobj(fitobj,"parameter5",fitid,&f1);
      } else if (i==6) {
        putobj(fitobj,"parameter6",fitid,&f1);
      } else if (i==7) {
        putobj(fitobj,"parameter7",fitid,&f1);
      }
    }
  }
  for (i=0;i<8;i++) {
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    for (j=0;j<num;j++) if (fitnameid[j]!=-1) {
      s=mathconv(buf);
      fitid=fitnameid[j];
      if (i==0) {
        putobj(fitobj,"user_func",fitid,s);
      } else if (i==1) {
        putobj(fitobj,"derivative0",fitid,s);
     } else if (i==2) {
        putobj(fitobj,"derivative1",fitid,s);
      } else if (i==3) {
        putobj(fitobj,"derivative2",fitid,s);
      } else if (i==4) {
        putobj(fitobj,"derivative3",fitid,s);
      } else if (i==5) {
        putobj(fitobj,"derivative4",fitid,s);
      } else if (i==6) {
        putobj(fitobj,"derivative5",fitid,s);
      } else if (i==7) {
        putobj(fitobj,"derivative6",fitid,s);
      }
    }
  }

/* NGRAPH */

  if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
  if (sscanf2(buf,"%d%d%d%d%d%d%d%d",
                  (int *)&graphtype,(int *)&d1,(int *)&d2,(int *)&d3,
                  (int *)&vx,(int *)&vy,(int *)&szx,(int *)&szy)!=8) {
    error2(obj,ERRPRM,file);
    goto errexit;
  }
  if ((gid=newobj(gobj))==-1) goto errexit;
  d1*=10;
  d2*=10;
  d3*=10;
  vx*=10;
  vy*=10;
  szx*=10;
  szy*=10;
  putobj(gobj,"top_margin",gid,&d1);
  putobj(gobj,"left_margin",gid,&d2);
  d3=d3*10000/21000;
  putobj(gobj,"zoom",gid,&d3);
  d1=21000;
  putobj(gobj,"paper_width",gid,&d1);
  d1=29700;
  putobj(gobj,"paper_height",gid,&d1);
  if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
  dmode[0]=(buf[0]=='Y')?FALSE:TRUE;
  dmode[1]=(buf[4]=='Y')?FALSE:TRUE;
  dmode[2]=(buf[8]=='Y')?FALSE:TRUE;
  if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
  for (i=0;i<7;i++) {
    for (j=i*20;j<(i+1)*20;j++) {
      if (buf[j]=='F') {
        if (buf[j+1]=='H') fff[i]=4;
        else if (buf[j+1]=='T') fff[i]=0;
        else if (buf[j+1]=='C') fff[i]=8;
        j++;
      } else if (buf[j]=='J') {
        if (buf[j+1]=='G') ffj[i]=1;
        else if (buf[j+1]=='M') ffj[i]=0;
        j++;
      } else if (buf[j]=='S') {
        d1=((buf[j+1]-'0')*10+(buf[j+2]-'0'))*100;
        ffs[i]=d1;
        j+=2;
      } else if (buf[j]=='P') {
        if (buf[j+1]=='+') d2=1;
        else d2=-1;
        d1=((buf[j+2]-'0')*10+(buf[j+3]-'0'))*100;
        ffp[i]=d1*d2;
        j+=3;
      } else if (buf[j]=='R') ffb[i]=0;
      else if (buf[j]=='B') ffb[i]=1;
      else if (buf[j]=='I') ffb[i]=2;
      else if (buf[j]=='O') ffb[i]=3;
      else if (buf[j]=='C') {
        d1=(buf[j+1]-'0');
        ffR[i]=(d1 & 4)?255:0;
        ffG[i]=(d1 & 2)?255:0;
        ffB[i]=(d1 & 1)?255:0;
        j++;
      }
    }
  }
  if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
/* ASattr is ignored */
  if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
  for (i=0;i<num;i++) {
    s=mathconv(buf);
    putobj(fobj,"func_f",fidroot+i,s);
  }
  if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
  for (i=0;i<num;i++) {
    s=mathconv(buf);
    putobj(fobj,"func_g",fidroot+i,s);
  }
  for (i=0;i<num;i++)
    putobj(fobj,"hidden",fidroot+i,&(dmode[0]));

/* Mask */

  if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
  if (sscanf2(buf,"%d",(int *)&masknum)!=1) {
    error2(obj,ERRPRM,file);
    goto errexit;
  }
  for (i=0;i<20;i++) {
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (i<masknum) {
      if (sscanf2(buf,"%d%d%le%le",(int *)&d1,(int *)&d2,
                                  (double *)&f1,(double *)&f2)!=4) {
        error2(obj,ERRPRM,file);
        goto errexit;
      }
      fid=fidroot+d1-1;
      getobj(fobj,"mask",fid,0,NULL,&mask);
      if (mask==NULL) {
        if ((mask=arraynew(sizeof(int)))==NULL) goto errexit;
        putobj(fobj,"mask",fid,mask);
      }
      arrayins(mask,&d2,0);
    }
  }

/* MaskReg */
/* MaskReg is expanded to Mask */

  if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
  if (sscanf2(buf,"%d",(int *)&masknum)!=1) {
    error2(obj,ERRPRM,file);
    goto errexit;
  }
  for (i=0;i<20;i++) {
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (i<masknum) {
      if (sscanf2(buf,"%d%le%le%le%le",(int *)&d1,
      (double *)&f1,(double *)&f2,(double *)&f3,(double *)&f4)!=5) {
        error2(obj,ERRPRM,file);
        goto errexit;
      }
      fid=fidroot+d1-1;
      getobj(fobj,"mask",fid,0,NULL,&mask);
      getobj(fobj,"type",fid,0,NULL,&plottype);
      exeobj(fobj,"closedata",fid,0,NULL);
      if (exeobj(fobj,"opendata",fid,0,NULL)==0) {
        while (exeobj(fobj,"getdata",fid,0,NULL)==0) {
          getobj(fobj,"data_x",fid,0,NULL,&datax);
          getobj(fobj,"data_y",fid,0,NULL,&datay);
          getobj(fobj,"data_2",fid,0,NULL,&data2);
          getobj(fobj,"data_3",fid,0,NULL,&data3);
          getobj(fobj,"stat_x",fid,0,NULL,&statx);
          getobj(fobj,"stat_y",fid,0,NULL,&staty);
          getobj(fobj,"stat_2",fid,0,NULL,&stat2);
          getobj(fobj,"stat_3",fid,0,NULL,&stat3);
          getobj(fobj,"line",fid,0,NULL,&line);
          switch (plottype) {
          case 4: case 5: case 6: case 7: case 8:
            break;
          case 9:
            datax=data3;
            statx=stat3;
            data3=datax;
            stat3=statx;
            break;
          case 10:
            datay=data2;
            staty=stat2;
            data2=datax;
            stat2=statx;
            break;
          default:
            data2=datax;
            stat2=statx;
            data3=datay;
            stat3=staty;
            break;
          }
          if ((statx==0) && (staty==0) && (stat2==0) && (stat3==0)) {
            if ((f1<datax) && (datax<f2) && (f1<data2) && (data2<f2)
             && (f3<datay) && (datay<f4) && (f3<data3) && (data2<f4)) {
              if (mask==NULL) {
                if ((mask=arraynew(sizeof(int)))==NULL) {
                  exeobj(fobj,"closedata",fid,0,NULL);
                  goto errexit;
                }
                putobj(fobj,"mask",fid,mask);
              }
              arrayins(mask,&line,0);
            }
          }
        }
        exeobj(fobj,"closedata",fid,0,NULL);
      }
    }
  }

/* ChgData */

  if ((filetype==3) || (filetype==13)) {
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%d",(int *)&movenum)!=1) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    for (i=0;i<20;i++) {
      if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
      if (i<movenum) {
        if (sscanf2(buf,"%d%d%le%le",(int *)&d1,(int *)&d2,
                                    (double *)&f1,(double *)&f2)!=4) {
          error2(obj,ERRPRM,file);
          goto errexit;
        }
        fid=fidroot+d1-1;
        exeobj(fobj, "move_data_adjust", fid, 0, NULL);
        getobj(fobj,"move_data",fid,0,NULL,&move);
        getobj(fobj,"move_data_x",fid,0,NULL,&movex);
        getobj(fobj,"move_data_y",fid,0,NULL,&movey);
        if (move==NULL) {
          if ((move=arraynew(sizeof(int)))==NULL) goto errexit;
          putobj(fobj,"move_data",fid,move);
        }
        if (movex==NULL) {
          if ((movex=arraynew(sizeof(double)))==NULL) goto errexit;
          putobj(fobj,"move_data_x",fid,movex);
        }
        if (movey==NULL) {
          if ((movey=arraynew(sizeof(double)))==NULL) goto errexit;
          putobj(fobj,"move_data_y",fid,movey);
        }
        arrayins(move,&d2,0);
        arrayins(movex,&f1,0);
        arrayins(movey,&f2,0);
      }
    }
  }


/* Title */

  for (i=0;i<6;i++) {
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (i<=1) j=0;
    else j=1;
    if ((strlen(buf)!=0)
    && ((s=remarkconv(buf,fff[j],ffj[j],ffb[j],fnameid,file,greek))!=NULL)) {
      if ((tid=newobj(tobj))==-1) {
        memfree(s);
        goto errexit;
      }
      putobj(tobj,"text",tid,s);
      if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
      if (sscanf2(buf,"%d%d",(int *)&d1,(int *)&d2)!=2) {
        error2(obj,ERRPRM,file);
        goto errexit;
      }
      switch (i) {
      case 0: case 4:
        d1+=vx;
        d2+=vy;
        break;
      case 1: case 2: case 3:
        d1+=vx;
        d2+=vy+szy;
        break;
      case 5:
        d1+=vx+szx;
        d2+=vy+szy;
        break;
      }
      putobj(tobj,"x",tid,&d1);
      putobj(tobj,"y",tid,&d2);
      if ((i==3) || (i==5)) d3=9000;
      else d3=0;
      putobj(tobj,"direction",tid,&d3);
      if ((s=memalloc(strlen(fontchar[fff[j]+ffb[j]])+1))==NULL) goto errexit;
      strcpy(s,fontchar[fff[j]+ffb[j]]);
      putobj(tobj,"font",tid,s);
      if ((s=memalloc(strlen(jfontchar[ffj[j]])+1))==NULL) goto errexit;
      strcpy(s,jfontchar[ffj[j]]);
      putobj(tobj,"jfont",tid,s);
      putobj(tobj,"pt",tid,&(ffs[j]));
      putobj(tobj,"space",tid,&(ffp[j]));
      putobj(tobj,"R",tid,&(ffR[j]));
      putobj(tobj,"G",tid,&(ffG[j]));
      putobj(tobj,"B",tid,&(ffB[j]));
    } else {
      if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    }
  }

/* Remark */

  for (i=0;i<20;i++) {
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if ((strlen(buf)!=0)
    && ((s=remarkconv(buf,fff[2],ffj[2],ffb[2],fnameid,file,greek))!=NULL)) {
      if ((tid=newobj(tobj))==-1) {
        memfree(s);
        goto errexit;
      }
      putobj(tobj,"text",tid,s);
      if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
      if (sscanf2(buf,"%d%d%d",(int *)&d1,(int *)&d2,(int *)&d3)!=3) {
        error2(obj,ERRPRM,file);
        goto errexit;
      }
      putobj(tobj,"x",tid,&d1);
      putobj(tobj,"y",tid,&d2);
      d3*=9000;
      putobj(tobj,"direction",tid,&d3);
      if ((s=memalloc(strlen(fontchar[fff[2]+ffb[2]])+1))==NULL) goto errexit;
      strcpy(s,fontchar[fff[2]+ffb[2]]);
      putobj(tobj,"font",tid,s);
      if ((s=memalloc(strlen(jfontchar[ffj[2]])+1))==NULL) goto errexit;
      strcpy(s,jfontchar[ffj[2]]);
      putobj(tobj,"jfont",tid,s);
      putobj(tobj,"pt",tid,&(ffs[2]));
      putobj(tobj,"space",tid,&(ffp[2]));
      putobj(tobj,"R",tid,&(ffR[2]));
      putobj(tobj,"G",tid,&(ffG[2]));
      putobj(tobj,"B",tid,&(ffB[2]));
      putobj(tobj,"hidden",tid,&(dmode[1]));
    } else {
      if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    }
  }

/* Arrow */
  for (i=0;i<20;i++) {
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%d%d%d%d%d%d%d%d%d",
                   (int *)&d1,(int *)&d2,(int *)&d3,(int *)&d4,(int *)&d5,
                   (int *)&d6,(int *)&d7,(int *)&d8,(int *)&d9)!=9) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    if (d9) {
      if ((lid=newobj(lobj))==-1) goto errexit;
      if ((iarray=arraynew(sizeof(int)))==NULL) goto errexit;
      arrayadd(iarray,&d1);
      arrayadd(iarray,&d2);
      arrayadd(iarray,&d3);
      arrayadd(iarray,&d4);
      putobj(lobj,"points",lid,iarray);
      d1=d5*10000.0/d7;
      d2=d1*0.828427;
      if (d1==0) d3=0;
      else {
        d3=1;
        putobj(lobj,"arrow_length",lid,&d1);
        putobj(lobj,"arrow_width",lid,&d2);
      }
      putobj(lobj,"arrow",lid,&d3);
      iarray=linestyleconv(d6,15);
      putobj(lobj,"style",lid,iarray);
      putobj(lobj,"width",lid,&d7);
      R=(d8 & 4)?255:0;
      G=(d8 & 2)?255:0;
      B=(d8 & 1)?255:0;
      putobj(lobj,"R",lid,&R);
      putobj(lobj,"G",lid,&G);
      putobj(lobj,"B",lid,&B);
      putobj(lobj,"hidden",lid,&(dmode[1]));
    }
  }

/* Dot */

  for (i=0;i<20;i++) {
    for (j=0;j<10;j++) {
      if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
      if (sscanf2(buf,"%d%d",(int *)&(gx[j]),(int *)&(gy[j]))!=2) {
        error2(obj,ERRPRM,file);
        goto errexit;
      }
      gy[j]=21000-gy[j];
    }
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%d%d%d%d%d%d%d",(int *)&d1,(int *)&d2,(int *)&d3,
    (int *)&d4,(int *)&d5,(int *)&d6,(int *)&d7)!=7) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    if (d5!=0) {
      if ((d3==0) || (d3==25)) {
        if ((lid=newobj(lobj))==-1) goto errexit;
        cmobj=lobj;
        cmid=lid;
      } else if (d3==1) {
        if ((pid=newobj(pobj))==-1) goto errexit;
        d10=FALSE;
        putobj(pobj,"fill",pid,&d10);
        cmobj=pobj;
        cmid=pid;
      } else if ((d3==2) || (d3==3)) {
        if ((cid=newobj(cobj))==-1) goto errexit;
        if (d3==2) intp=0;
        else intp=1;
        putobj(cobj,"interpolation",cid,&intp);
        cmobj=cobj;
        cmid=cid;
      } else if  (d3>=26) {
        if ((rid=newobj(robj))==-1) goto errexit;
        putobj(robj,"x1",rid,&(gx[0]));
        putobj(robj,"y1",rid,&(gy[0]));
        putobj(robj,"x2",rid,&(gx[1]));
        putobj(robj,"y2",rid,&(gy[1]));
        if ((d3==26) || (d3==27)) d10=TRUE;
        else d10=FALSE;
        putobj(robj,"fill",rid,&d10);
        cmid=rid;
        cmobj=robj;
        if (d3==27) {
          if ((rid2=newobj(robj))==-1) goto errexit;
          putobj(robj,"x1",rid2,&(gx[0]));
          putobj(robj,"y1",rid2,&(gy[0]));
          putobj(robj,"x2",rid2,&(gx[1]));
          putobj(robj,"y2",rid2,&(gy[1]));
          d10=FALSE;
          putobj(robj,"fill",rid2,&d10);
        }
     } else {
        if ((mid=newobj(mobj))==-1) goto errexit;
        putobj(mobj,"x",mid,&(gx[0]));
        putobj(mobj,"y",mid,&(gy[0]));
        if ((d3==5) || (d3==6)) type=0;
        else if (d3==7) type=1;
        else if (d3==8) type=2;
        else if (d3==9) type=3;
        else if (d3==10) type=80;
        else if (d3==11) type=10;
        else if (d3==12) type=11;
        else if (d3==13) type=12;
        else if (d3==14) type=20;
        else if (d3==15) type=21;
        else if (d3==16) type=22;
        else if (d3==17) type=30;
        else if (d3==18) type=31;
        else if (d3==19) type=32;
        else if (d3==20) type=40;
        else if (d3==21) type=41;
        else if (d3==22) type=42;
        else if (d3==23) type=70;
        else if (d3==24) type=71;
        putobj(mobj,"type",mid,&type);
        d6*=10;
        if (d3==5) d6/=2;
        putobj(mobj,"size",mid,&d6);
        cmobj=mobj;
        cmid=mid;
      }
      if ((d3<=3) || (d3==25)) {
        if ((iarray=arraynew(sizeof(int)))==NULL) goto errexit;
        for (j=0;j<d5;j++) {
          arrayadd(iarray,&(gx[j]));
          arrayadd(iarray,&(gy[j]));
        }
        putobj(cmobj,"points",cmid,iarray);
      }
      putobj(cmobj,"hidden",cmid,&(dmode[1]));
      if (d3!=27) {
        putobj(cmobj,"width",cmid,&d2);
        iarray=linestyleconv(d1,d7);
        putobj(cmobj,"style",cmid,iarray);
        R=(d4 & 4)?255:0;
        G=(d4 & 2)?255:0;
        B=(d4 & 1)?255:0;
        putobj(cmobj,"R",cmid,&R);
        putobj(cmobj,"G",cmid,&G);
        putobj(cmobj,"B",cmid,&B);
      } else {
        putobj(cmobj,"style",cmid,NULL);
        R=255;
        G=255;
        B=255;
        putobj(cmobj,"R",cmid,&R);
        putobj(cmobj,"G",cmid,&G);
        putobj(cmobj,"B",cmid,&B);
        iarray=linestyleconv(d1,d7);
        putobj(cmobj,"style",rid2,iarray);
        putobj(cmobj,"width",rid2,&d2);
        R=(d4 & 4)?255:0;
        G=(d4 & 2)?255:0;
        B=(d4 & 1)?255:0;
        putobj(cmobj,"R",rid2,&R);
        putobj(cmobj,"G",rid2,&G);
        putobj(cmobj,"B",rid2,&B);
        putobj(cmobj,"hidden",rid2,&(dmode[1]));
      }
    }
  }

/* AxisPar */

  for (i=0;i<4;i++) {
    if (anameid[i]!=-1) {
      aid=anameid[i];
    } else {
      if ((aid=newobj(aobj))==-1) goto errexit;
      anameid[i]=aid;
      getobj(aobj,"oid",aid,0,NULL,&(anameoid[i]));
    }
    aid2[i]=aid;
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%le%le%le%d%d%d",(double *)&amin,(double *)&amax,
               (double *)&ainc,(int *)&d1,(int *)&d2,(int *)&d3)!=6) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    if (!d1 || !d2 || !d3) {
      amin=amax=ainc=0;
      setaxis[i]=FALSE;
    } else setaxis[i]=TRUE;
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%d",(int *)&atype)!=1) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    j=0;
    if (buf[j]=='A') d1=0;
    else {
      for (k=0;k<4;k++) str4[k]=buf[j+k];
      d1=strtol(str4,&endptr,10);
    }
    putobj(aobj,"div",aid,&d1);
    j+=4;
    hiddenaxis[i]=(buf[j]=='Y')?FALSE:TRUE;
    j+=4;
    scfull=(buf[j]=='Y')?TRUE:FALSE;
    j+=4;
    if (buf[j]=='A') sccros[i]=0;
    else {
      for (k=0;k<4;k++) str4[k]=buf[j+k];
      sccros[i]=strtol(str4,&endptr,10);
    }
    j+=4;
    scauto[i]=(buf[j]=='Y')?0:1;
    j+=4;
    if (buf[j]=='A') scnum=-1;
    else {
      for (k=0;k<4;k++) str4[k]=buf[j+k];
      scnum=strtol(str4,&endptr,10);
    }
    j+=4;
    if (buf[j]=='A') scstart=0;
    else {
      for (k=0;k<4;k++) str4[k]=buf[j+k];
      scstart=strtol(str4,&endptr,10);
    }
    j+=4;
    if (buf[j]=='A') scstep=0;
    else {
      for (k=0;k<4;k++) str4[k]=buf[j+k];
      scstep=strtol(str4,&endptr,10);
    }
    j+=4;
    for (k=0;k<4;k++) scfig[k]=buf[j+k];
    j+=4;
    sczero=(buf[j]=='Y')?TRUE:FALSE;
    j+=4;
    scplus=(buf[j]=='Y')?TRUE:FALSE;
    j+=4;
    scperiod=(buf[j]=='Y')?TRUE:FALSE;
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%d%d%d%d%d%d%d",
        (int *)&scposs,(int *)&scpose,(int *)&scdir,(int *)&sclr,
        (int *)&sclog,(int *)&scposx,(int *)&scposy)!=7) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%d%d%d%d%d",(int *)&la0,(int *)&d1,(int *)&d2,
                                (int *)&d3,(int *)&la4)!=5) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    if (i==0) {
      la1=d1;
      la2=d2;
      la3=d3;
    }
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%d%d%d%d%d",(int *)&lw0,(int *)&d1,(int *)&d2,
                                (int *)&d3,(int *)&lw4)!=5) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    if (i==0) {
      lw1=d1;
      lw2=d2;
      lw3=d3;
    }
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%d%d%d%d%d",(int *)&lc0,(int *)&d1,(int *)&d2,
                                (int *)&d3,(int *)&lc4)!=5) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    if (i==0) {
      lc1=d1;
    }
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (sscanf2(buf,"%d%d%d",(int *)&ll1,(int *)&ll2,(int *)&ll3)!=3) {
      error2(obj,ERRPRM,file);
      goto errexit;
    }
    if (i==0) {
      d1=vx;
      d2=vy+szy;
      d3=0;
      d4=szx;
    } else if (i==1) {
      d1=vx;
      d2=vy+szy;
      d3=9000;
      d4=szy;
    } else if (i==2) {
      d1=vx;
      d2=vy;
      d3=0;
      d4=szx;
    } else if (i==3) {
      d1=vx+szx;
      d2=vy+szy;
      d3=9000;
      d4=szy;
    }
    if (!scfull && setaxis[i]) {
      if ((i==0) || (i==2)) {
        d1=d1+d4*scposs/10000;
        d4=d4*(scpose-scposs)/10000;
        amin=amin+(amax-amin)*scposs/10000;
        amax=amin+(amax-amin)*scpose/10000;
      } else {
        d2=d2-d4*scposs/10000;
        d4=d4*abs(scpose-scposs)/10000;
        amin=amin+(amax-amin)*scposs/10000;
        amax=amin+(amax-amin)*scpose/10000;
      }
    }

    if (setaxis[i]) {
      if (atype==1) {
        amin=pow(10.0,amin);
        amax=pow(10.0,amax);
        ainc=pow(10.0,ainc);
      } else if (atype==2) {
        amin=1/amin;
        amax=1/amax;
        ainc=1/ainc;
      }
      putobj(aobj,"min",aid,&amin);
      putobj(aobj,"max",aid,&amax);
      putobj(aobj,"inc",aid,&ainc);
    }
    if ((graphtype==1) && (i>1)) {
      if ((s=memalloc(30))==NULL) goto errexit;
      if (i==2) sprintf(s,"axis:^%d",anameoid[0]);
      else sprintf(s,"axis:^%d",anameoid[1]);
      putobj(aobj,"reference",aid,s);
    }

    putobj(aobj,"type",aid,&atype);
    putobj(aobj,"x",aid,&d1);
    putobj(aobj,"y",aid,&d2);
    putobj(aobj,"direction",aid,&d3);
    putobj(aobj,"length",aid,&d4);

    iarray=linestyleconv(la0,15);
    putobj(aobj,"style",aid,iarray);
    R=(lc0 & 4)?255:0;
    G=(lc0 & 2)?255:0;
    B=(lc0 & 1)?255:0;
    putobj(aobj,"R",aid,&R);
    putobj(aobj,"G",aid,&G);
    putobj(aobj,"B",aid,&B);
    putobj(aobj,"width",aid,&lw0);

    if ((graphtype==1) || ((i<=1) && (graphtype==2))) {
      d1=0;
      if ((i==0) || (i==3)) {
        if (scdir==0) d1=2;
        else if (scdir==1) d1=3;
        else if (scdir==2) d1=1;
      } else {
        if (scdir==0) d1=3;
        else if (scdir==1) d1=2;
        else if (scdir==2) d1=1;
      }
      putobj(aobj,"gauge",aid,&d1);
      iarray=linestyleconv(la4,15);
      putobj(aobj,"gauge_style",aid,iarray);
      R=(lc4 & 4)?255:0;
      G=(lc4 & 2)?255:0;
      B=(lc4 & 1)?255:0;
      putobj(aobj,"gauge_R",aid,&R);
      putobj(aobj,"gauge_G",aid,&G);
      putobj(aobj,"gauge_B",aid,&B);
      putobj(aobj,"gauge_width1",aid,&lw4);
      putobj(aobj,"gauge_width2",aid,&lw4);
      putobj(aobj,"gauge_width3",aid,&lw4);
      ll1*=10;
      ll2*=10;
      ll3*=10;
      putobj(aobj,"gauge_length1",aid,&ll1);
      putobj(aobj,"gauge_length2",aid,&ll2);
      putobj(aobj,"gauge_length3",aid,&ll3);
    }

    if ((i<=1) || ((i>=2) && (graphtype==1))) {
      putobj(aobj,"num_num",aid,&scnum);
      putobj(aobj,"num_begin",aid,&scstart);
      putobj(aobj,"num_step",aid,&scstep);
      if (sclog==0) sclog=FALSE;
      else sclog=TRUE;
      putobj(aobj,"num_log_pow",aid,&sclog);
      if (sclr==0) sclr=3;
      else if (sclr==1) sclr=2;
      else if (sclr==2) sclr=0;
      else if (sclr==3) sclr=1;
      putobj(aobj,"num_align",aid,&sclr);
      if (i==0){
        d1=2;
        posx=scposx*10;
        posy=scposy*10;
      } else if (i==1) {
        d1=1;
        posy=-scposx*10;
        posx=-scposy*10;
      } else if (i==2) {
        d1=1;
        posx=scposx*10;
        posy=-scposy*10;
      } else if (i==3) {
        d1=2;
        posy=scposx*10;
        posx=-scposy*10;
      }
      putobj(aobj,"num",aid,&d1);
      putobj(aobj,"num_shift_p",aid,&posx);
      putobj(aobj,"num_shift_n",aid,&posy);
      j=0;
      format[j++]='%';
      if (scplus) format[j++]='+';
      if (sczero) {
        format[j++]=scfig[0];
        format[j++]=scfig[1];
        format[j++]=scfig[2];
        format[j++]='f';
      } else format[j++]='g';
      format[j]='\0';
      if ((s=memalloc(strlen(format)+1))==NULL)
        goto errexit;
      strcpy(s,format);
      putobj(aobj,"num_format",aid,s);
      if ((s=memalloc(strlen(fontchar[fff[3+i]+ffb[3+i]])+1))==NULL)
        goto errexit;
      strcpy(s,fontchar[fff[3+i]+ffb[3+i]]);
      putobj(aobj,"num_font",aid,s);
      if ((s=memalloc(strlen(jfontchar[ffj[3+i]])+1))==NULL) goto errexit;
      strcpy(s,jfontchar[ffj[3+i]]);
      putobj(aobj,"num_jfont",aid,s);
      putobj(aobj,"num_pt",aid,&(ffs[3+i]));
      putobj(aobj,"num_space",aid,&(ffp[3+i]));
      putobj(aobj,"num_R",aid,&(ffR[3+i]));
      putobj(aobj,"num_G",aid,&(ffG[3+i]));
      putobj(aobj,"num_B",aid,&(ffB[3+i]));
    }
    if (hiddenaxis[i]) {
      d1=0;
      putobj(aobj,"baseline",aid,&d1);
      putobj(aobj,"gauge",aid,&d1);
      putobj(aobj,"num",aid,&d1);
    }
  }

  if (graphtype==0) {
    if ((agdid=newobj(agdobj))==-1) goto errexit;
    if ((s=memalloc(30))==NULL) goto errexit;
    sprintf(s,"axis:^%d",anameoid[0]);
    putobj(agdobj,"axis_x",agdid,s);
    if ((s=memalloc(30))==NULL) goto errexit;
    sprintf(s,"axis:^%d",anameoid[1]);
    putobj(agdobj,"axis_y",agdid,s);
    iarray=linestyleconv(la1,15);
    putobj(agdobj,"style1",agdid,iarray);
    iarray=linestyleconv(la2,15);
    putobj(agdobj,"style2",agdid,iarray);
    iarray=linestyleconv(la3,15);
    putobj(agdobj,"style3",agdid,iarray);
    R=(lc1 & 4)?255:0;
    G=(lc1 & 2)?255:0;
    B=(lc1 & 1)?255:0;
    putobj(agdobj,"R",agdid,&R);
    putobj(agdobj,"G",agdid,&G);
    putobj(agdobj,"B",agdid,&B);
    putobj(agdobj,"width1",agdid,&lw1);
    putobj(agdobj,"width2",agdid,&lw2);
    putobj(agdobj,"width3",agdid,&lw3);
    gtype=2;
    arrayinit(&group,sizeof(int));
    arrayadd(&group,&gtype);
    arrayadd(&group,&aid2[0]);
    arrayadd(&group,&aid2[1]);
    arrayadd(&group,&aid2[2]);
    arrayadd(&group,&aid2[3]);
    argv2[0]=(void *)&group;
    argv2[1]=NULL;
    exeobj(aobj,"grouping",aid2[3],1,argv2);
    arraydel(&group);
  } else if (graphtype==1) {
    gtype=1;
    arrayinit(&group,sizeof(int));
    arrayadd(&group,&gtype);
    arrayadd(&group,&aid2[0]);
    arrayadd(&group,&aid2[1]);
    arrayadd(&group,&aid2[2]);
    arrayadd(&group,&aid2[3]);
    argv2[0]=(void *)&group;
    argv2[1]=NULL;
    exeobj(aobj,"grouping",aid2[3],1,argv2);
    arraydel(&group);
  } else if (graphtype==2) {
    if ((s=memalloc(30))==NULL) goto errexit;
    sprintf(s,"axis:^%d",anameoid[1]);
    putobj(aobj,"adjust_axis",aid2[0],s);
    putobj(aobj,"adjust_position",aid2[0],&sccros[1]);
    exeobj(aobj,"adjust",aid2[0],0,NULL);
    if ((s=memalloc(30))==NULL) goto errexit;
    sprintf(s,"axis:^%d",anameoid[0]);
    putobj(aobj,"adjust_axis",aid2[1],s);
    putobj(aobj,"adjust_position",aid2[1],&sccros[0]);
    exeobj(aobj,"adjust",aid2[1],0,NULL);
    if (aid2[2]>aid2[3]) {
      delobj(aobj,aid2[2]);
      delobj(aobj,aid2[3]);
    } else {
      delobj(aobj,aid2[3]);
      delobj(aobj,aid2[2]);
    }
    gtype=3;
    arrayinit(&group,sizeof(int));
    arrayadd(&group,&gtype);
    arrayadd(&group,&aid2[0]);
    arrayadd(&group,&aid2[1]);
    argv2[0]=(void *)&group;
    argv2[1]=NULL;
    exeobj(aobj,"grouping",aid2[1],1,argv2);
    arraydel(&group);
  }

/* Merge */

  for (i=0;i<10;i++) {
    if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    if (strlen(buf)!=0) {
      if ((mgid=newobj(mgobj))==-1) goto errexit;
      if ((s=memalloc(strlen(buf)+1))==NULL) goto errexit;
      strcpy(s,buf);
      s=pathconv(s,ignorepath);
      putobj(mgobj,"file",mgid,s);
      if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
      if (sscanf2(buf,"%d%d%d",(int *)&d1,(int *)&d2,(int *)&d3)!=3) {
        error2(obj,ERRPRM,file);
        goto errexit;
      }
      d1*=10;
      d2*=10;
      d3=nround(d3/0.21);
      putobj(mgobj,"left_margin",mgid,&d1);
      putobj(mgobj,"top_margin",mgid,&d2);
      putobj(mgobj,"zoom",mgid,&d3);
      putobj(mgobj,"hidden",mgid,&(dmode[2]));
    } else {
      if (prmloadline(obj,file,fp,buf,TRUE)!=0) goto errexit;
    }
  }

  EOD="\n[EOF]\n";
  if ((filetype==12) || (filetype==13)) {
    while (prmloadline(obj,file,fp,buf,FALSE)==0) {
      if (buf[0]=='[') {
        if ((s2=strchr(buf,']'))!=NULL) {
	  int ch;

          if ((filename=memalloc(s2-buf))==NULL) goto errexit;
          strncpy(filename,buf+1,s2-buf-1);
          filename[s2-buf-1]='\0';
          filename=pathconv(filename,ignorepath);
          if (access(filename,R_OK)!=0) mkdata=TRUE;
          else {
            sprintf(buf2,"`%s' Overwrite existing file?",filename);
            mkdata=inputyn(buf2);
          }
          if (mkdata) {
            if ((fp2=nfopen(filename,"wt"))==NULL) {
              error2(obj,ERROPEN,filename);
              goto errexit;
            }
          }
          i=0;
          do {
            ch=nfgetc(fp);
            if (ch==EOD[i]) i++;
            else {
              if (mkdata) for (j=0;j<i;j++) fputc(EOD[j],fp2);
              i=0;
              if (ch==EOD[i]) i++;
              else if (mkdata && (ch!=EOF)) fputc(ch,fp2);
            }
          } while ((ch!=EOF) && (i<7));
          if (mkdata) {
            fclose(fp2);
            if (gettimeval2(s2+1,&ftime)==0) {
              tm.actime=ftime;
              tm.modtime=ftime;
              utime(filename,&tm);
            }
          }
          memfree(filename);
        }
      }
    }
  }

  fclose(fp);
  return 0;

errexit:
  fclose(fp);
  return 1;
}

static struct objtable prm[] = {
  {"init",NVFUNC,NEXEC,prminit,NULL,0},
  {"done",NVFUNC,NEXEC,prmdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"file",NSTR,NREAD|NWRITE,NULL,NULL,0},
  {"symbol_greek",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"ignore_path",NBOOL,NREAD|NWRITE,NULL,NULL,0},
  {"load",NVFUNC,NREAD|NEXEC,prmload,NULL,0},
};

#define TBLNUM (sizeof(prm) / sizeof(*prm))

void *
addprm(void)
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,prm,ERRNUM,prmerrorlist,NULL,NULL);
}

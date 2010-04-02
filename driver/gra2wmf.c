/**
 *
 * $Id: gra2wmf.c,v 1.9 2010-04-01 06:08:22 hito Exp $
 *
 * This is free software; you can redistribute it and/or modify it.
 *
 * Original author: Satoshi ISHIZAKA
 *                  isizaka@msa.biglobe.ne.jp
 **/

#include <sys/stat.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <unistd.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "dir_defs.h"

#include "wmfapi.h"

#define MPI 3.1415926535897932385
#define GRAF "%Ngraph GRAF"
#define CONF "gra2wmf.ini"
#define G2WINCONF "[gra2wmf]"
#define DIRSEP '/'
#define CONFSEP "/"
#define NSTRLEN 256
#define LINETOLIMIT 500
#define VERSION "1.00.01"

#define FONT_ROMAN 0
#define FONT_BOLD 1
#define FONT_ITALIC 2
#define FONT_BOLDITALIC 3
#define FONT_UNKNOWN 4

struct fontmap;

struct fontmap {
  char *fontalias;
  char *fontname;
  int charset;
  int italic;
  struct fontmap *next;
};

struct fontmap *fontmaproot;

char *libdir,*homedir;
int minus_hyphen;
int windpi;


int printfstderr(char *fmt,...)
{
  int len;
  va_list ap;

  va_start(ap,fmt);
  len=vfprintf(stderr,fmt,ap);
  va_end(ap);
  return len;
}

int niskanji(unsigned int code)
{
  if (((0x81<=code) && (code<=0x9f))
   || ((0xe0<=code) && (code<=0xff))) return TRUE;
  return FALSE;
}

unsigned int njms2jis(unsigned int code)
{
  unsigned char dh,dl;

  dh=code >> 8;
  dl=code & 0xff;
  if (dh<=0x9f) dh-=0x70;
  else dh-=0xb0;
  dh=dh<<1;
  if (dl>=0x9f) dl-=0x7e;
  else {
    dh--;
    if (dl>=0x80) dl-=0x20;
    else dl-=0x1f;
  }
  return ((unsigned int)(dh << 8))+dl;
}

char *nstrnew(void)
{
  char *po;

  po=(char *)malloc(NSTRLEN);
  po[0]='\0';
  return po;
}

char *nstrccat(char *po,char ch)
{
  size_t len,num;

  if (po==NULL) return NULL;
  len=strlen(po);
  num=len/NSTRLEN;
  if (len%NSTRLEN==NSTRLEN-1) po=(char *)realloc(po,NSTRLEN*(num+2));
  po[len]=ch;
  po[len+1]='\0';
  return po;
}

int fgetline(FILE *fp,char **buf)
{
  char *s;
  char ch;

  *buf=NULL;
  ch=fgetc(fp);
  if (ch==EOF) return 1;
  s=nstrnew();
  while (TRUE) {
    if ((ch=='\0') || (ch=='\n') || (ch==EOF)) {
      *buf=s;
      return 0;
    }
    if (ch!='\r') s=nstrccat(s,ch);
    ch=fgetc(fp);
  }
}

FILE *openconfig(char *section)
{
  char *s,*buf,*homeconf,*libconf;
  FILE *fp;
  struct stat homestat,libstat;

  homeconf=(char *)malloc(strlen(homedir)+strlen(CONFSEP)+strlen(CONF)+1);
  strcpy(homeconf,homedir);
  if ((strlen(homedir)>0) && (homeconf[strlen(homedir)-1]=='/'))
    homeconf[strlen(homedir)-1]='\0';
  strcat(homeconf,CONFSEP);
  strcat(homeconf,CONF);
  libconf=(char *)malloc(strlen(libdir)+strlen(CONFSEP)+strlen(CONF)+1);
  strcpy(libconf,libdir);
  if ((strlen(libdir)>0) && (libconf[strlen(libdir)-1]=='/'))
    libconf[strlen(libdir)-1]='\0';
  strcat(libconf,CONFSEP);
  strcat(libconf,CONF);
  if (access(homeconf,04)==0) {
    if (stat(homeconf,&homestat)!=0) {
      free(homeconf);
      homeconf=NULL;
    }
  } else {
    free(homeconf);
    homeconf=NULL;
  }
  if (access(libconf,04)==0) {
    if (stat(libconf,&libstat)!=0) {
      free(libconf);
      libconf=NULL;
    }
  } else {
    free(libconf);
    libconf=NULL;
  }
  if (homeconf!=NULL) {
    if (libconf==NULL) s=homeconf;
    else if (homestat.st_mtime>=libstat.st_mtime) {
      s=homeconf;
      free(libconf);
    } else {
      s=libconf;
      free(homeconf);
    }
  } else if (libconf!=NULL) s=libconf;
  else return NULL;
  if ((fp=fopen(s,"rt"))==NULL) return NULL;
  free(s);
  while (fgetline(fp,&buf)==0) {
    if (strcmp(buf,section)==0) {
      free(buf);
      return fp;
    }
    free(buf);
  }
  fclose(fp);
  return NULL;
}

char *getitok(char **s,int *len,char *ifs)
{
  char *po,*spo;
  int i;

  if (*s==NULL) return NULL;
  po=*s;
  for (i=0;(po[i]!='\0') && (strchr(ifs,po[i])!=NULL);i++);
  if (po[i]=='\0') {
    *len=0;
    return NULL;
  }
  spo=po+i;
  for (;(po[i]!='\0') && (strchr(ifs,po[i])==NULL);i++);
  *s+=i;
  *len=*s-spo;
  return spo;
}

char *getitok2(char **s,int *len,char *ifs)
{
  char *po,*s2;

  if ((s2=getitok(s,len,ifs))==NULL) return NULL;
  po=(char *)malloc(*len+1);
  strncpy(po,s2,*len);
  po[*len]='\0';
  return po;
}

char *getconfig(FILE *fp,char **val)
{
  char *s,*tok,*buf;
  int len;

  while (TRUE) {
    if (fgetline(fp,&buf)!=0) return NULL;
    else {
      if (buf[0]=='[') {
        free(buf);
        return NULL;
      } else {
        s=buf;
        if ((tok=getitok2(&s,&len," \x09=,"))!=NULL) {
          for (;(s[0]!='\0') && (strchr(" \x09=,",s[0])!=NULL);s++);
          *val=(char *)malloc(strlen(s)+1);
          strcpy(*val,s);
          free(buf);
          return tok;
        }
        free(buf);
        free(tok);
      }
    }
  }
}

void closeconfig(FILE *fp)
{
  fclose(fp);
}

int getintpar(char *s,int num,int cpar[])
{
  int i,pos1,pos2;
  char s2[256];
  char *endptr;

  pos1=0;
  for (i=0;i<num;i++) {
	while ((s[pos1]!='\0') && (strchr(" \x09,",s[pos1])!=NULL)) pos1++;
	if (s[pos1]=='\0') return FALSE;
	pos2=0;
	while ((s[pos1]!='\0') && (strchr(" \x09,",s[pos1])==NULL)) {
	  s2[pos2]=s[pos1];
	  pos2++;
	  pos1++;
	}
	s2[pos2]='\0';
	cpar[i]=strtol(s2,&endptr,10);
	if (endptr[0]!='\0') return FALSE;
  }
  return TRUE;
}

int GRAinput(char *s,void (*draw)(char code,int *cpar,char *cstr))
{
  int pos,i;
  int num;
  char code;
  int *cpar;
  char *cstr;

  code='\0';
  cpar=NULL;
  cstr=NULL;
  for (i=0;s[i]!='\0';i++)
	if (strchr("\n\r",s[i])!=NULL) {
	  s[i]='\0';
	  break;
	}
  pos=0;
  while ((s[pos]==' ') || (s[pos]=='\x09')) pos++;
  if (s[pos]=='\0') return TRUE;
  if (strchr("IE%VAGOMNLTCBPRDFHSK",s[pos])==NULL) return FALSE;
  code=s[pos];
  if (strchr("%FSK",code)==NULL) {
	if (!getintpar(s+pos+1,1,&num)) return FALSE;
    num++;
    cpar=(int *)malloc(sizeof(int)*num);
    if (!getintpar(s+pos+1,num,cpar)) goto errexit;
  } else {
    cpar=(int *)malloc(sizeof(int));
    cpar[0]=-1;
    cstr=(char *)malloc(strlen(s)-pos);
    strcpy(cstr,s+pos+1);
  }
  draw(code,cpar,cstr);
  free(cpar);
  free(cstr);
  return TRUE;

errexit:
  free(cpar);
  free(cstr);
  return FALSE;
}

int nround(double x)
{
  int ix;
  double dx;

  if (x>INT_MAX) return INT_MAX;
  else if (x<INT_MIN) return INT_MIN;
  else {
    ix=(int )x;
    dx=x-ix;
    if (dx>=0.5) return ix+1;
    else if (dx<=-0.5) return ix-1;
    else return ix;
  }
}

HDC DDC;
HPEN ThePen,OrgPen;
HBRUSH TheBrush,OrgBrush;
HFONT TheFont,OrgFont;
COLORREF Col;
LOGFONT IDFont;
int style,width,dashn;
unsigned int *dashlist;
int xx0,yy0;
int dashi,dotf;
double dashlen;
double pixel_dot;
int offsetx,offsety;
int scrollx,scrolly;
int cpx,cpy;
int loadfontf;
char *fontalias;
int charset;
double fontsize,fontspace,fontcos,fontsin;
int linetonum;

int HelveticaSet[256]={
278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
278,278,355,556,556,889,667,222,333,333,389,584,278,584,278,278,
556,556,556,556,556,556,556,556,556,556,278,278,584,584,584,556,
1015,667,667,722,722,667,611,778,722,278,500,667,556,833,722,778,
667,778,722,667,611,722,667,944,667,667,611,278,278,278,469,556,
222,556,556,500,556,556,278,556,556,222,222,500,222,833,556,556,
556,556,333,500,278,556,500,722,500,500,500,334,260,334,584,278,
278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
278,333,333,333,333,333,333,333,333,278,333,333,278,333,333,333,
278,333,556,556,556,556,260,556,333,737,370,556,584,333,737,333,
400,584,333,333,333,556,537,278,333,333,365,556,834,834,834,611,
667,667,667,667,667,667,1000,722,667,667,667,667,278,278,278,278,
722,722,778,778,778,778,778,584,778,722,722,722,722,667,667,611,
556,556,556,556,556,556,889,500,556,556,556,556,278,278,278,278,
556,556,556,556,556,556,556,584,611,556,556,556,556,500,556,500};

int HelveticaBoldSet[256]={
278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
278,333,474,556,556,889,722,278,333,333,389,584,278,584,278,278,
556,556,556,556,556,556,556,556,556,556,333,333,584,584,584,611,
975,722,722,722,722,667,611,778,722,278,556,722,611,833,722,778,
667,778,722,667,611,722,667,944,667,667,611,333,278,333,584,556,
278,556,611,556,611,556,333,611,611,278,278,556,278,889,611,611,
611,611,389,556,333,611,556,778,556,556,500,389,280,389,584,278,
278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
278,333,333,333,333,333,333,333,333,278,333,333,278,333,333,333,
278,333,556,556,556,556,280,556,333,737,370,556,584,333,737,333,
400,584,333,333,333,611,556,278,333,333,365,556,834,834,834,611,
722,722,722,722,722,722,1000,722,667,667,667,667,278,278,278,278,
722,722,778,778,778,778,778,584,778,722,722,722,722,667,667,611,
556,556,556,556,556,556,889,556,556,556,556,556,278,278,278,278,
611,611,611,611,611,611,611,584,611,611,611,611,611,556,611,556};

int TimesRomanSet[256]={
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,333,408,500,500,833,778,333,333,333,500,564,250,564,250,278,
500,500,500,500,500,500,500,500,500,500,278,278,564,564,564,444,
921,722,667,667,722,611,556,722,722,333,389,722,611,889,722,722,
556,722,667,556,611,722,722,944,722,722,611,333,278,333,469,500,
333,444,500,444,500,444,333,500,500,278,278,500,278,778,500,500,
500,500,333,389,278,500,500,722,500,500,444,480,200,480,541,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
278,333,333,333,333,333,333,333,333,250,333,333,250,333,333,333,
250,333,500,500,500,500,200,500,333,760,276,500,564,333,760,333,
400,564,300,300,333,500,453,250,333,300,310,500,750,750,750,444,
722,722,722,722,722,722,889,667,611,611,611,611,333,333,333,333,
722,722,722,722,722,722,722,564,722,722,722,722,722,722,556,500,
444,444,444,444,444,444,667,444,444,444,444,444,278,278,278,278,
500,500,500,500,500,500,500,564,500,500,500,500,500,500,500,500};

int TimesBoldSet[256]={
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,333,555,500,500,1000,833,333,333,333,500,570,250,570,250,278,
500,500,500,500,500,500,500,500,500,500,333,333,570,570,570,500,
930,722,667,722,722,667,611,778,778,389,500,778,667,944,722,778,
611,778,722,556,667,722,722,1000,722,722,667,333,278,333,581,500,
333,500,556,444,556,444,333,500,556,278,333,556,278,833,556,500,
556,556,444,389,333,556,500,722,500,500,444,394,220,394,520,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
278,333,333,333,333,333,333,333,333,250,333,333,250,333,333,333,
250,333,500,500,500,500,220,500,333,747,300,500,570,333,747,333,
400,570,300,300,333,556,540,250,333,300,330,500,750,750,750,500,
722,722,722,722,722,722,1000,722,667,667,667,667,389,389,389,389,
722,722,778,778,778,778,778,570,778,722,722,722,722,722,611,556,
500,500,500,500,500,500,722,444,444,444,444,444,278,278,278,278,
500,556,500,500,500,500,500,570,500,556,556,556,556,500,556,500};

int TimesItalicSet[256]={
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,333,420,500,500,833,778,333,333,333,500,675,250,675,250,278,
500,500,500,500,500,500,500,500,500,500,333,333,675,675,675,500,
920,611,611,667,722,611,611,722,722,333,444,667,556,833,667,722,
611,722,611,500,556,722,611,833,611,556,556,389,278,389,422,500,
333,500,500,444,500,444,278,500,500,278,278,444,278,722,500,500,
500,500,389,389,278,500,444,667,444,444,389,400,275,400,541,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
278,333,333,333,333,333,333,333,333,250,333,333,250,333,333,333,
250,389,500,500,500,500,275,500,333,760,276,500,675,333,760,333,
400,675,300,300,333,500,523,250,333,300,310,500,750,750,750,500,
611,611,611,611,611,611,889,667,611,611,611,611,333,333,333,333,
722,667,722,722,722,722,722,675,722,722,722,722,722,556,611,500,
500,500,500,500,500,500,667,444,444,444,444,444,278,278,278,278,
500,500,500,500,500,500,500,675,500,500,500,500,500,444,500,444};

int TimesBoldItalicSet[256]={
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,389,555,500,500,833,778,333,333,333,500,570,250,606,250,278,
500,500,500,500,500,500,500,500,500,500,333,333,570,570,570,500,
832,667,667,667,722,667,667,722,778,389,500,667,611,889,722,722,
611,722,667,556,611,722,667,889,667,611,611,333,278,333,570,500,
333,500,500,444,500,444,333,500,556,278,278,500,278,778,556,500,
500,500,389,389,278,556,444,667,500,444,389,348,220,348,570,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
278,333,333,333,333,333,333,333,333,250,333,333,250,333,333,333,
250,389,500,500,500,500,220,500,333,747,266,500,606,333,747,333,
400,570,300,300,333,576,500,250,333,300,300,500,750,750,750,500,
667,667,667,667,667,667,944,667,667,667,667,667,389,389,389,389,
722,722,722,722,722,722,722,570,722,722,722,722,722,611,611,500,
500,500,500,500,500,500,722,444,444,444,444,444,278,278,278,278,
500,556,500,500,500,500,500,570,500,556,556,556,556,444,500,444};

int SymbolSet[256]={
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,333,713,500,549,833,778,439,333,333,500,549,250,549,250,278,
500,500,500,500,500,500,500,500,500,500,278,278,549,549,549,444,
549,722,667,722,612,611,763,603,722,333,631,722,686,889,722,722,
768,741,556,592,611,690,439,768,645,795,611,333,863,333,658,500,
500,631,549,549,494,439,521,411,603,329,603,549,549,576,521,549,
549,521,549,603,439,576,713,686,493,686,494,480,200,480,549,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,620,247,549,167,713,500,753,753,753,753,1042,987,603,987,603,
400,549,411,549,549,713,494,460,549,549,549,549,1000,603,1000,658,
823,686,795,987,768,768,823,768,768,713,713,713,713,713,713,713,
768,713,790,790,890,823,549,250,713,603,603,1042,987,603,987,603,
494,329,790,790,786,713,384,384,384,384,384,384,494,494,494,494,
250,329,274,686,686,686,384,384,384,384,384,384,494,494,494,250};

int charwidth(unsigned int ch, char *font,int size)
{
  if ((strcmp(font,"Times")==0) || (strcmp(font,"Tim")==0))
    return (int )(25.4/72000.0*size*TimesRomanSet[ch]);
  else if ((strcmp(font,"TimesBold")==0) || (strcmp(font,"TimB")==0))
    return (int )(25.4/72000.0*size*TimesBoldSet[ch]);
  else if ((strcmp(font,"TimesItalic")==0) || (strcmp(font,"TimI")==0))
    return (int )(25.4/72000.0*size*TimesItalicSet[ch]);
  else if ((strcmp(font,"TimesBoldItalic")==0) || (strcmp(font,"TimBI")==0))
    return (int )(25.4/72000.0*size*TimesBoldItalicSet[ch]);
  else if ((strcmp(font,"Helvetica")==0) || (strcmp(font,"Helv")==0)
        || (strcmp(font,"HelveticaOblique")==0) || (strcmp(font,"HelvO")==0)
        || (strcmp(font,"HelveticaItalic")==0) || (strcmp(font,"HelvI")==0))
    return (int )(25.4/72000.0*size*HelveticaSet[ch]);
  else if ((strcmp(font,"HelveticaBold")==0) || (strcmp(font,"HelvB")==0)
      || (strcmp(font,"HelveticaBoldOblique")==0) || (strcmp(font,"HelvBO")==0)
      || (strcmp(font,"HelveticaBoldItalic")==0) || (strcmp(font,"HelvBI")==0))
    return (int )(25.4/72000.0*size*HelveticaBoldSet[ch]);
  else if ((strcmp(font,"Symbol")==0) || (strcmp(font,"Sym")==0)
        || (strcmp(font,"SymbolBold")==0) || (strcmp(font,"SymB")==0)
        || (strcmp(font,"SymbolItalic")==0) || (strcmp(font,"SymI")==0)
        || (strcmp(font,"SymbolBoldItalic")==0) || (strcmp(font,"SymBI")==0))
    return (int )(25.4/72000.0*size*SymbolSet[ch]);
  else if ((strcmp(font,"Gothic")==0) || (strcmp(font,"Goth")==0)
        || (strcmp(font,"GothicBold")==0) || (strcmp(font,"GothB")==0)
        || (strcmp(font,"GothicItalic")==0) || (strcmp(font,"GothI")==0)
        || (strcmp(font,"GothicBoldItalic")==0) || (strcmp(font,"GothBI")==0)
        || (strcmp(font,"Mincho")==0) || (strcmp(font,"Min")==0)
        || (strcmp(font,"MinchoBold")==0) || (strcmp(font,"MinB")==0)
        || (strcmp(font,"MinchoItalic")==0) || (strcmp(font,"MinI")==0)
        || (strcmp(font,"MinchoBoldItalic")==0) || (strcmp(font,"MinBI")==0))
    return (int )(25.4/72000.0*size*1000);
  else return (int )(25.4/72000.0*size*600);
}

int charheight(char *font,int size)
{
  if ((strcmp(font,"Times")==0) || (strcmp(font,"Tim")==0))
    return (int )(25.4/72000.0*size*662);
  else if ((strcmp(font,"TimesBold")==0) || (strcmp(font,"TimB")==0))
    return (int )(25.4/72000.0*size*676);
  else if ((strcmp(font,"TimesItalic")==0) || (strcmp(font,"TimI")==0))
    return (int )(25.4/72000.0*size*653);
  else if ((strcmp(font,"TimesBoldItalic")==0) || (strcmp(font,"TimBI")==0))
    return (int )(25.4/72000.0*size*669);
  else if ((strcmp(font,"Helvetica")==0) || (strcmp(font,"Helv")==0)
        || (strcmp(font,"HelveticaOblique")==0) || (strcmp(font,"HelvO")==0)
        || (strcmp(font,"HelveticaItalic")==0) || (strcmp(font,"HelvI")==0))
    return (int )(25.4/72000.0*size*718);
  else if ((strcmp(font,"HelveticaBold")==0) || (strcmp(font,"HelvB")==0)
     || (strcmp(font,"HelveticaBoldOblique")==0) || (strcmp(font,"HelvBO")==0)
     || (strcmp(font,"HelveticaBoldItalic")==0) || (strcmp(font,"HelvBI")==0))
    return (int )(25.4/72000.0*size*718);
  else if ((strcmp(font,"Symbol")==0) || (strcmp(font,"Sym")==0)
        || (strcmp(font,"SymbolBold")==0) || (strcmp(font,"SymB")==0)
        || (strcmp(font,"SymbolItalic")==0) || (strcmp(font,"SymI")==0)
        || (strcmp(font,"SymbolBoldItalic")==0) || (strcmp(font,"SymBI")==0))
    return (int )(25.4/72000.0*size*673);
  else if ((strcmp(font,"CourierBold")==0) || (strcmp(font,"CourB")==0)
        || (strcmp(font,"CourierBoldOblique")==0) || (strcmp(font,"CourBO")==0)
        || (strcmp(font,"CourierBoldItalic")==0) || (strcmp(font,"CourBI")==0))
    return (int )(25.4/72000.0*size*562);
  else if ((strcmp(font,"Gothic")==0) || (strcmp(font,"Goth")==0)
        || (strcmp(font,"GothicBold")==0) || (strcmp(font,"GothB")==0)
        || (strcmp(font,"GothicItalic")==0) || (strcmp(font,"GothI")==0)
        || (strcmp(font,"GothicBoldItalic")==0) || (strcmp(font,"GothBI")==0))
    return (int )(25.4/72000.0*size*791);
  else if ((strcmp(font,"Mincho")==0) || (strcmp(font,"Min")==0)
        || (strcmp(font,"MinchoBold")==0) || (strcmp(font,"MinB")==0)
        || (strcmp(font,"MinchoItalic")==0) || (strcmp(font,"MinI")==0)
        || (strcmp(font,"MinchoBoldItalic")==0) || (strcmp(font,"MinBI")==0))
    return (int )(25.4/72000.0*size*807);
  else return (int )(25.4/72000.0*size*562);
}

int chardescent(char *font,int size)
{
  return (int )(25.4/72000.0*size*250);
}

char *bfontalias=NULL;
int bloadfont=FALSE;
int boffsetx=0;
int boffsety=0;
int bminx=INT_MAX;
int bminy=INT_MAX;
int bmaxx=INT_MIN;
int bmaxy=INT_MIN;
int bposx=0;
int bposy=0;
int bpt=0;
int bspc=0;
int bdir=0;
int blinew=0;
int bclip=TRUE;
int bclipsizex=21000;
int bclipsizey=29700;

void setbbminmax(int x1,int y1,int x2,int y2,int lw)
{
  int x,y;

  if (x1>x2) {
    x=x1; x1=x2; x2=x;
  }
  if (y1>y2) {
    y=y1; y1=y2; y2=y;
  }
  if (lw) {
    x1-=blinew;
    y1-=blinew;
    x2+=blinew;
    y2+=blinew;
  }
  if (!bclip || !((x2<0) || (y2<0) || (x1>bclipsizex) || (y1>bclipsizey))) {
    if (bclip) {
      if (x1<0) x1=0;
      if (y1<0) y1=0;
      if (x2>bclipsizex) x2=bclipsizex;
      if (y2>bclipsizey) y1=bclipsizey;
    }
    x1+=boffsetx;
    x2+=boffsetx;
    y1+=boffsety;
    y2+=boffsety;
    if (x1<bminx) bminx=x1;
    if (y1<bminy) bminy=y1;
    if (x2>bmaxx) bmaxx=x2;
    if (y2>bmaxy) bmaxy=y2;
  }
}

void getboundingbox(char code,int *cpar,char *cstr)
{
  int i,n,lw;
  double x,y,csin,ccos;
  int w,h,d,x1,y1,x2,y2,x3,y3,x4,y4;
  int c1,c2;
  char ch;

  switch (code) {
  case 'X': case 'I': case 'E': case '%': case 'G':
    break;
  case 'V':
    boffsetx=cpar[1];
    boffsety=cpar[2];
    bposx=0;
    bposy=0;
    if (cpar[5]==1) bclip=TRUE;
    else bclip=FALSE;
    bclipsizex=cpar[3]-cpar[1];
    bclipsizey=cpar[4]-cpar[2];
    break;
  case 'A':
    blinew=cpar[2]/2;
    break;
  case 'M':
    bposx=cpar[1];
    bposy=cpar[2];
    break;
  case 'N':
    bposx+=cpar[1];
    bposy+=cpar[2];
    break;
  case 'L':
    setbbminmax(cpar[1],cpar[2],cpar[3],cpar[4],TRUE);
    break;
  case 'T':
    setbbminmax(bposx,bposy,cpar[1],cpar[2],TRUE);
    bposx=cpar[1];
    bposy=cpar[2];
    break;
  case 'C':
    if (cpar[7]==0) lw=TRUE;
    else lw=FALSE;
    if (cpar[7]==1) setbbminmax(cpar[1],cpar[2],cpar[1],cpar[2],lw);
    setbbminmax(cpar[1]+(int )(cpar[3]*cos(cpar[5]/18000.0*MPI)),
                cpar[2]-(int )(cpar[4]*sin(cpar[5]/18000.0*MPI)),
                cpar[1]+(int )(cpar[3]*cos((cpar[5]+cpar[6])/18000.0*MPI)),
                cpar[2]-(int )(cpar[4]*sin((cpar[5]+cpar[6])/18000.0*MPI)),lw);
    cpar[6]+=cpar[5];
    cpar[5]-=9000;
    cpar[6]-=9000;
    if ((cpar[5]<0) && (cpar[6]>0))
      setbbminmax(cpar[1],cpar[2]-cpar[4],cpar[1],cpar[2]-cpar[4],lw);
    cpar[5]-=9000;
    cpar[6]-=9000;
    if ((cpar[5]<0) && (cpar[6]>0))
      setbbminmax(cpar[1]-cpar[3],cpar[2],cpar[1]-cpar[3],cpar[2],lw);
    cpar[5]-=9000;
    cpar[6]-=9000;
    if ((cpar[5]<0) && (cpar[6]>0))
      setbbminmax(cpar[1],cpar[2]+cpar[4],cpar[1],cpar[2]+cpar[4],lw);
    cpar[5]-=9000;
    cpar[6]-=9000;
    if ((cpar[5]<0) && (cpar[6]>0))
      setbbminmax(cpar[1]+cpar[3],cpar[2],cpar[1]+cpar[3],cpar[2],lw);
    break;
  case 'B':
    if (cpar[5]==1) lw=FALSE;
    else lw=TRUE;
    setbbminmax(cpar[1],cpar[2],cpar[3],cpar[4],lw);
    break;
  case 'P':
    setbbminmax(cpar[1],cpar[2],cpar[1],cpar[2],FALSE);
    break;
  case 'R':
    for (i=0;i<(cpar[1]-1);i++)
      setbbminmax(cpar[i*2+2],cpar[i*2+3],cpar[i*2+4],cpar[i*2+5],TRUE);
    break;
  case 'D':
    if (cpar[2]==0) lw=TRUE;
    else lw=FALSE;
    for (i=0;i<(cpar[1]-1);i++)
      setbbminmax(cpar[i*2+3],cpar[i*2+4],cpar[i*2+5],cpar[i*2+6],lw);
    break;
  case 'F':
    free(bfontalias);
    bfontalias=(char *)malloc(strlen(cstr)+1);
    strcpy(bfontalias,cstr);
    break;
  case 'H':
    bpt=cpar[1];
    bspc=cpar[2];
    bdir=cpar[3];
    bloadfont=TRUE;
    break;
  case 'S':
    if (!bloadfont) break;
    csin=sin(bdir/18000.0*MPI);
    ccos=cos(bdir/18000.0*MPI);
    i=0;
    n = strlen(cstr);
    while (i < n) {
      if ((cstr[i]=='\\') && (cstr[i+1]=='x')) {
        if (toupper(cstr[i+2])>='A') c1=toupper(cstr[i+2])-'A'+10;
        else c1=cstr[i+2]-'0';
        if (toupper(cstr[i+3])>='A') c2=toupper(cstr[i+3])-'A'+10;
        else c2=cstr[i+3]-'0';
        ch=c1*16+c2;
        i+=4;
      } else {
        ch=cstr[i];
        i++;
      }
      w=charwidth((unsigned char)ch,bfontalias,bpt);
      h=charheight(bfontalias,bpt);
      d=chardescent(bfontalias,bpt);
      x=0;
      y=d;
      x1=(int )(bposx+(x*ccos+y*csin));
      y1=(int )(bposy+(-x*csin+y*ccos));
      x=0;
      y=-h;
      x2=(int )(bposx+(x*ccos+y*csin));
      y2=(int )(bposy+(-x*csin+y*ccos));
      x=w;
      y=d;
      x3=(int )(bposx+(x*ccos+y*csin));
      y3=(int )(bposy+(-x*csin+y*ccos));
      x=w;
      y=-h;
      x4=(int )(bposx+(int )(x*ccos+y*csin));
      y4=(int )(bposy+(int )(-x*csin+y*ccos));
      setbbminmax(x1,y1,x4,y4,FALSE);
      setbbminmax(x2,y2,x3,y3,FALSE);
      bposx+=(int )((w+bspc*25.4/72)*ccos);
      bposy-=(int )((w+bspc*25.4/72)*csin);
    }
    break;
  case 'K':
    if (!bloadfont) break;
    csin=sin(bdir/18000.0*MPI);
    ccos=cos(bdir/18000.0*MPI);
    i=0;
    n = strlen(cstr);
    while (i < n) {
      if (niskanji((unsigned char)cstr[i]) && (cstr[i+1]!='\0')) {
        w=charwidth((((unsigned char)cstr[i+1])<<8)+(unsigned char)cstr[i],
                       bfontalias,bpt);
        h=charheight(bfontalias,bpt);
        d=chardescent(bfontalias,bpt);
        x=0;
        y=d;
        x1=(int )(bposx+(x*ccos+y*csin));
        y1=(int )(bposy+(-x*csin+y*ccos));
        x=0;
        y=-h;
        x2=(int )(bposx+(x*ccos+y*csin));
        y2=(int )(bposy+(-x*csin+y*ccos));
        x=w;
        y=d;
        x3=(int )(bposx+(x*ccos+y*csin));
        y3=(int )(bposy+(-x*csin+y*ccos));
        x=w;
        y=-h;
        x4=(int )(bposx+(int )(x*ccos+y*csin));
        y4=(int )(bposy+(int )(-x*csin+y*ccos));
        setbbminmax(x1,y1,x4,y4,FALSE);
        setbbminmax(x2,y2,x3,y3,FALSE);
        bposx+=(int )((w+bspc*25.4/72)*ccos);
        bposy-=(int )((w+bspc*25.4/72)*csin);
        i+=2;
      } else i++;
    }
    break;
  }
}

void moveto(HDC DC,int x,int y)
{
  dashlen=0;
  dashi=0;
  dotf=TRUE;
  xx0=x;
  yy0=y;
  MoveTo(DC,xx0,yy0);
}

void lineto(HDC DC,int x,int y)
{
  double dd,len,len2;
  double dx,dy;
  int gx,gy,gx1,gy1,gx2,gy2;

  gx1=xx0;
  gy1=yy0;
  gx2=x;
  gy2=y;
  if (dashn==0) LineTo(DC,gx2,gy2);
  else {
    dx=(gx2-gx1);
    dy=(gy2-gy1);
    len2=len=sqrt(dx*dx+dy*dy);
    while (len2>((dashlist)[dashi]-dashlen)) {
      dd=(len-len2+(dashlist)[dashi]-dashlen)/len;
      gx=gx1+nround(dx*dd);
      gy=gy1+nround(dy*dd);
      if (dotf) LineTo(DC,gx,gy);
      else MoveTo(DC,gx,gy);
      dotf=dotf ? FALSE : TRUE;
      len2-=((dashlist)[dashi]-dashlen);
      dashlen=0;
      dashi++;
      if (dashi>=dashn) {
        dashi=0;
        dotf=TRUE;
      }
    }
    if (dotf) LineTo(DC,gx2,gy2);
    dashlen+=len2;
  }
  xx0=x;
  yy0=y;
}

int dot2pixel(int r)
{
  return nround(r*pixel_dot);
}

int dot2pixelx(int x)
{
  return nround(x*pixel_dot+offsetx-scrollx);
}

int dot2pixely(int y)
{
  return nround(y*pixel_dot+offsety-scrolly);
}

int pixel2dot(int r)
{
  return nround(r/pixel_dot);
}

void draw(char code,int *cpar,char *cstr)
{
  char ch[1],*s;
  HPEN TmpPen;
  HBRUSH TmpBrush;
  HFONT TmpFont;
  HDC DC;
  unsigned int cap,join;
  int i,n,R,G,B;
  double Theta1,Theta2;
  POINT *Points;
  int italic;
  char *fontname;
  struct fontmap *fcur;
  double fontdir;
  double x0,y0,fontwidth;

  DC=DDC;
  if (DC<0) return;
  if (linetonum!=0) {
	if ((code!='T') || (linetonum>=LINETOLIMIT)) {
	  linetonum=0;
	}
  }
  switch (code) {
  case 'I':
    break;
  case 'E': case '%': case 'X':
    break;
  case 'V':
    offsetx=dot2pixel(cpar[1]);
    offsety=dot2pixel(cpar[2]);
    cpx=0;
    cpy=0;
    break;
   case 'A':
    if (cpar[1]==0) {
      style=PS_SOLID;
      dashn=0;
      free(dashlist);
      dashlist=NULL;
    } else {
      free(dashlist);
      style=PS_USERSTYLE;
      if ((dashlist=(unsigned int *)malloc(sizeof(int)*cpar[1]))==NULL) break;
      for (i=0;i<cpar[1];i++)
       if ((dashlist[i]=dot2pixel(cpar[6+i]))<=0)
         dashlist[i]=1;
      dashn=cpar[1];
    }
    width=dot2pixel(cpar[2]);
    if (cpar[3]==2) cap=PS_ENDCAP_SQUARE;
    else if (cpar[3]==1) cap=PS_ENDCAP_ROUND;
    else cap=PS_ENDCAP_FLAT;
    if (cpar[4]==2) join=PS_JOIN_BEVEL;
    else if (cpar[4]==1) join=PS_JOIN_ROUND;
    else join=PS_JOIN_MITER;
    style=PS_SOLID;
    TmpPen=CreatePen(style,width,Col);
    SelectObject(DC,TmpPen);
    DeleteObject(ThePen);
    ThePen=TmpPen;
    break;
  case 'G':
    R=cpar[1];
    if (R>255) R=255;
    else if (R<0) R=0;
    G=cpar[2];
    if (G>255) G=255;
    else if (G<0) G=0;
    B=cpar[3];
    if (B>255) B=255;
    else if (B<0) B=0;
    Col=RGB(R,G,B);
    TmpPen=CreatePen(style,width,Col);
    SelectObject(DC,TmpPen);
    DeleteObject(ThePen);
    ThePen=TmpPen;
    TmpBrush=CreateSolidBrush(Col);
    SelectObject(DC,TmpBrush);
    DeleteObject(TheBrush);
    SetTextColor(DC,Col);
    TheBrush=TmpBrush;
    break;
  case 'M':
    cpx=cpar[1];
    cpy=cpar[2];
    break;
  case 'N':
    cpx+=cpar[1];
    cpy+=cpar[2];
    break;
  case 'L':
    if ((dashn!=0) && ((style & PS_USERSTYLE)==0)) {
      moveto(DC,dot2pixelx(cpar[1]),dot2pixely(cpar[2]));
      lineto(DC,dot2pixelx(cpar[3]),dot2pixely(cpar[4]));
    } else {
      MoveTo(DC,dot2pixelx(cpar[1]),dot2pixely(cpar[2]));
      LineTo(DC,dot2pixelx(cpar[3]),dot2pixely(cpar[4]));
    }
    break;
  case 'T':
    if (linetonum==0) {
      if ((dashn!=0) && ((style & PS_USERSTYLE)==0))
        moveto(DC,dot2pixelx(cpx),dot2pixely(cpy));
      else
        MoveTo(DC,dot2pixelx(cpx),dot2pixely(cpy));
    }
    if ((dashn!=0) && ((style & PS_USERSTYLE)==0))
      lineto(DC,dot2pixelx(cpar[1]),dot2pixely(cpar[2]));
    else
      LineTo(DC,dot2pixelx(cpar[1]),dot2pixely(cpar[2]));
    linetonum++;
    cpx=cpar[1];
    cpy=cpar[2];
    break;
  case 'C':
    if (cpar[6]==36000) {
      if (cpar[7]==0) {
        TmpBrush=OrgBrush;
        SelectObject(DC,TmpBrush);
        Ellipse(DC,
               dot2pixelx(cpar[1]-cpar[3]),
               dot2pixely(cpar[2]-cpar[4]),
               dot2pixelx(cpar[1]+cpar[3]),
               dot2pixely(cpar[2]+cpar[4]));
        SelectObject(DC,TheBrush);
      } else {
        TmpPen=OrgPen;
        SelectObject(DC,TmpPen);
        Ellipse(DC,
               dot2pixelx(cpar[1]-cpar[3]),
               dot2pixely(cpar[2]-cpar[4]),
               dot2pixelx(cpar[1]+cpar[3]),
               dot2pixely(cpar[2]+cpar[4]));
        SelectObject(DC,ThePen);
      }
    } else {
      Theta1=cpar[5]*MPI/18000.0;
      Theta2=Theta1+cpar[6]*MPI/18000.0;
      if (cpar[7]==0) {
        Arc(DC,dot2pixelx(cpar[1]-cpar[3]),
               dot2pixely(cpar[2]-cpar[4]),
               dot2pixelx(cpar[1]+cpar[3]),
               dot2pixely(cpar[2]+cpar[4]),
               dot2pixelx(cpar[1]+nround(cpar[3]*cos(Theta1)))-1,
               dot2pixely(cpar[2]-nround(cpar[4]*sin(Theta1)))-1,
               dot2pixelx(cpar[1]+nround(cpar[3]*cos(Theta2)))-1,
               dot2pixely(cpar[2]-nround(cpar[4]*sin(Theta2)))-1);
      } else {
        TmpPen=OrgPen;
        SelectObject(DC,TmpPen);
        if (cpar[7]==1) {
          if ((dot2pixel(cpar[3])<2) && (dot2pixel(cpar[4])<2)) {
            SetPixel(DC,dot2pixelx(cpar[1]),dot2pixely(cpar[2]),Col);
          }
          Pie(DC,dot2pixelx(cpar[1]-cpar[3]),
                 dot2pixely(cpar[2]-cpar[4]),
                 dot2pixelx(cpar[1]+cpar[3]),
                 dot2pixely(cpar[2]+cpar[4]),
                 dot2pixelx(cpar[1]+nround(cpar[3]*cos(Theta1)))-1,
                 dot2pixely(cpar[2]-nround(cpar[4]*sin(Theta1)))-1,
                 dot2pixelx(cpar[1]+nround(cpar[3]*cos(Theta2)))-1,
                 dot2pixely(cpar[2]-nround(cpar[4]*sin(Theta2)))-1);
        } else {
          Chord(DC,dot2pixelx(cpar[1]-cpar[3]),
                   dot2pixely(cpar[2]-cpar[4]),
                   dot2pixelx(cpar[1]+cpar[3]),
                   dot2pixely(cpar[2]+cpar[4]),
                   dot2pixelx(cpar[1]+nround(cpar[3]*cos(Theta1)))-1,
                   dot2pixely(cpar[2]-nround(cpar[4]*sin(Theta1)))-1,
                   dot2pixelx(cpar[1]+nround(cpar[3]*cos(Theta2)))-1,
                   dot2pixely(cpar[2]-nround(cpar[4]*sin(Theta2)))-1);
        }
        SelectObject(DC,ThePen);
      }
    }
    break;
  case 'B':
    if (cpar[5]==0) {
      TmpBrush=OrgBrush;
      SelectObject(DC,TmpBrush);
      Rectangle(DC,dot2pixelx(cpar[1]),dot2pixely(cpar[2]),
                   dot2pixelx(cpar[3]),dot2pixely(cpar[4]));
      SelectObject(DC,TheBrush);
    } else {
      TmpPen=OrgPen;
      SelectObject(DC,TmpPen);
      Rectangle(DC,dot2pixelx(cpar[1]),dot2pixely(cpar[2]),
                   dot2pixelx(cpar[3]),dot2pixely(cpar[4]));
      SelectObject(DC,ThePen);
    }
    break;
  case 'P':
    SetPixel(DC,dot2pixelx(cpar[1]),dot2pixely(cpar[2]),Col);
    break;
  case 'R':
    if (cpar[1]==0) break;
    MoveTo(DC,dot2pixelx(cpar[2]),dot2pixely(cpar[3]));
    for (i=1;i<cpar[1];i++) {
      LineTo(DC,dot2pixelx(cpar[i*2+2]),dot2pixely(cpar[i*2+3]));
    }
    break;
  case 'D':
    if (cpar[1]==0) break;
    if ((Points=(POINT *)malloc(sizeof(POINT)*cpar[1]))==NULL) break;
    for (i=0;i<cpar[1];i++) {
      Points[i].x=dot2pixelx(cpar[i*2+3]);
      Points[i].y=dot2pixely(cpar[i*2+4]);
    }
    if (cpar[2]==0) {
      TmpBrush=OrgBrush;
      SelectObject(DC,TmpBrush);
      Polygon(DC,Points,cpar[1]);
      SelectObject(DC,TheBrush);
    } else {
      TmpPen=OrgPen;
      SelectObject(DC,TmpPen);
      if (cpar[2]==1) {
        SetPolyFillMode(DC,ALTERNATE);
        Polygon(DC,Points,cpar[1]);
      } else {
        SetPolyFillMode(DC,WINDING);
        Polygon(DC,Points,cpar[1]);
      }
      SelectObject(DC,ThePen);
    }
    free(Points);
    break;
  case 'F':
    free(fontalias);
    if ((fontalias=(char *)malloc(strlen(cstr)+1))==NULL) break;
    strcpy(fontalias,cstr);
    break;
  case 'H':
    loadfontf=FALSE;
    fontspace=cpar[2]/72.0*25.4;
    fontsize=cpar[1]/72.0*25.4;
    fontdir=cpar[3]/100.0;
    fontsin=sin(fontdir/180*MPI);
    fontcos=cos(fontdir/180*MPI);
    fcur=fontmaproot;
    fontname=NULL;
    while (fcur!=NULL) {
      if (strcmp(fontalias,fcur->fontalias)==0) {
        fontname=fcur->fontname;
        charset=fcur->charset;
        italic=fcur->italic;
        break;
      }
      fcur=fcur->next;
    }
    if (fontname==NULL) {
      loadfontf=FALSE;
      break;
    }
    IDFont.lfHeight=-nround(pixel_dot*fontsize);
    IDFont.lfWidth=0;
    IDFont.lfEscapement=IDFont.lfOrientation=nround(fontdir*10);
    IDFont.lfUnderline=0;
    IDFont.lfStrikeOut=0;
    IDFont.lfPitchAndFamily=0;
    IDFont.lfCharSet=charset;
    IDFont.lfOutPrecision=0;
    IDFont.lfClipPrecision=0;
    IDFont.lfQuality=0;
    if (italic==FONT_ROMAN) {
      IDFont.lfWeight=400;
      IDFont.lfItalic=FALSE;
    } else if (italic==FONT_ITALIC) {
      IDFont.lfWeight=400;
      IDFont.lfItalic=TRUE;
    } else if (italic==FONT_BOLD) {
      IDFont.lfWeight=700;
      IDFont.lfItalic=FALSE;
    } else if (italic==FONT_BOLDITALIC) {
      IDFont.lfWeight=700;
      IDFont.lfItalic=TRUE;
    } else {
      IDFont.lfWeight=0;
      IDFont.lfItalic=0;
    }
    strcpy((char *) IDFont.lfFaceName, fontname);
    if ((TmpFont=CreateFontIndirect(&IDFont))!=NULL) {
      SelectObject(DC,TmpFont);
      DeleteObject(TheFont);
      TheFont=TmpFont;
      SetTextAlign(DC,TA_BASELINE | TA_LEFT);
      SetTextCharacterExtra(DC,dot2pixel(fontspace));
      SetBkMode(DC,TRANSPARENT);
      loadfontf=TRUE;
    }
    IDFont.lfEscapement=IDFont.lfOrientation=0;
    break;
  case 'S':
    if (!loadfontf) break;
    if ((s=nstrnew())==NULL) break;
    i=0;
    n = strlen(cstr);
    while (i < n) {
      if (cstr[i]=='\\') {
        if (cstr[i+1]=='x') {
          if (toupper(cstr[i+2])>='A') ch[0]=toupper(cstr[i+2])-'A'+10;
          else ch[0]=cstr[i+2]-'0';
          if (toupper(cstr[i+3])>='A') ch[0]=ch[0]*16+toupper(cstr[i+3])-'A'+10;
          else ch[0]=ch[0]*16+cstr[i+3]-'0';
          i+=4;
        } else if (cstr[i+1]=='\\') {
          ch[0]=cstr[i+1];
          i+=2;
        } else {
          ch[0]='\0';
          i++;
        }
      } else {
        ch[0]=cstr[i];
        i++;
      }
      if ((charset==ANSI_CHARSET) && minus_hyphen) {
        if (ch[0]=='-') ch[0]='\x96';
        else if (ch[0]=='\x96') ch[0]='-';
      }
      s=nstrccat(s,ch[0]);
    }
    if (s==NULL) break;
    x0=cpx;
    y0=cpy;
    TextOut(DC,dot2pixelx(nround(x0)),
               dot2pixely(nround(y0)),s,strlen(s));
    fontwidth=0;
    n = strlen(s);
    for (i = 0; i < n; i++) {
      fontwidth+=charwidth((unsigned char)s[i],fontalias,fontsize/25.4*72);
      fontwidth+=fontspace;
    }
    free(s);
    x0+=fontwidth*fontcos;
    y0-=fontwidth*fontsin;
    cpx=nround(x0);
    cpy=nround(y0);
    break;
  case 'K':
    if (!loadfontf) break;
    x0=cpx;
    y0=cpy;
    TextOut(DC,dot2pixelx(nround(x0)),
               dot2pixely(nround(y0)),cstr,strlen(cstr));
    fontwidth=0;
    n = strlen(cstr);
    for (i = 0; i < n; i += 2) {
      fontwidth+=charwidth((((unsigned char)cstr[i+1])<<8)+(unsigned char)cstr[i],
                           fontalias,fontsize/25.4*72);
      fontwidth+=fontspace;
    }
    x0+=fontwidth*fontcos;
    y0-=fontwidth*fontsin;
    cpx=nround(x0);
    cpy=nround(y0);
    break;
  default: break;
  }
}

void PrintPage(HDC DC,char *FileName,int scx,int scy)
{
  FILE *fp;
  char *buf;
  LOGBRUSH lBrush;

  DDC=DC;
  style=PS_SOLID;
  width=1;
  Col=RGB(0,0,0);
  dashlist=NULL;
  dashn=0;
  dashlen=0;
  dashi=0;
  dotf=TRUE;
  xx0=0;
  yy0=0;
  offsetx=0;
  offsety=0;
  scrollx=scx;
  scrolly=scy;
  cpx=0;
  cpy=0;
  pixel_dot=windpi/25.4/100;
  fontalias=NULL;
  loadfontf=FALSE;
  linetonum=0;

  ThePen=NULL;
  TheBrush=NULL;
  TheFont=NULL;
  OrgPen=CreatePen(PS_NULL,0,RGB(0,0,0));
  lBrush.lbStyle=BS_NULL;
  lBrush.lbColor=RGB(0,0,0);
  lBrush.lbHatch=0;
  OrgBrush=CreateBrushIndirect(&lBrush);
  OrgFont=NULL;
  SelectObject(DDC,OrgPen);
  SelectObject(DDC,OrgBrush);

  if ((fp=fopen(FileName,"rt"))!=NULL) {
    if (fgetline(fp,&buf)!=1) {
      if (strcmp(buf,GRAF)==0) {
        free(buf);
        while (fgetline(fp,&buf)!=1) {
          if (!GRAinput(buf,draw)) {
            printfstderr("illegal GRA format.");
            fclose(fp);
            free(buf);
            goto error;
          }
          free(buf);
        }
        fclose(fp);
      } else free(buf);
    }
  }

error:
  DeleteObject(ThePen);
  DeleteObject(TheBrush);
  DeleteObject(TheFont);
  DeleteObject(OrgPen);
  DeleteObject(OrgBrush);
}

int loadconfig(void)
{
  FILE *fp;
  char *tok,*str,*s2;
  char *f1,*f2,*f3,*f4;
  struct fontmap *fcur,*fnew;
  int len,val;
  char *endptr;

  if ((fp=openconfig(G2WINCONF))==NULL) return 0;
  fcur=fontmaproot;
  while ((tok=getconfig(fp,&str))!=NULL) {
    s2=str;
    if (strcmp(tok,"font_map")==0) {
      f1=getitok2(&s2,&len," \x09,");
      f3=getitok2(&s2,&len," \x09,");
      f4=getitok2(&s2,&len," \x09,");
      for (;(s2[0]!='\0') && (strchr(" \x09,",s2[0])!=NULL);s2++);
      f2=getitok2(&s2,&len,"");
      if ((f1!=NULL) && (f2!=NULL) && (f3!=NULL) && (f4!=NULL)) {
        if ((fnew=(struct fontmap *)malloc(sizeof(struct fontmap)))==NULL) {
          free(tok);
          free(f1);
          free(f2);
          free(f3);
          free(f4);
          closeconfig(fp);
          return 1;
        }
        if (fcur==NULL) fontmaproot=fnew;
        else fcur->next=fnew;
        fcur=fnew;
        fcur->next=NULL;
        fcur->fontalias=f1;
        fcur->fontname=f2;
        if (strcmp(f4,"roman")==0) fcur->italic=FONT_ROMAN;
        else if (strcmp(f4,"italic")==0) fcur->italic=FONT_ITALIC;
        else if (strcmp(f4,"bold")==0) fcur->italic=FONT_BOLD;
        else if (strcmp(f4,"bold_italic")==0) fcur->italic=FONT_BOLDITALIC;
        else fcur->italic=FONT_UNKNOWN;
        if (strcmp(f3,"shiftjis")==0) fcur->charset=SHIFTJIS_CHARSET;
        else if (strcmp(f3,"symbol")==0) fcur->charset=SYMBOL_CHARSET;
        else if (strcmp(f3,"ansi")==0) fcur->charset=ANSI_CHARSET;
        else if (strcmp(f3,"oem")==0) fcur->charset=OEM_CHARSET;
        else if (strcmp(f3,"hangeul")==0) fcur->charset=HANGEUL_CHARSET;
        else if (strcmp(f3,"chinesebig5")==0) fcur->charset=CHINESEBIG5_CHARSET;
        else fcur->charset=DEFAULT_CHARSET;
        free(f3);
        free(f4);
      } else {
        free(f1);
        free(f2);
        free(f3);
        free(f4);
      }
    } else if (strcmp(tok,"dpi")==0) {
      f1=getitok2(&s2,&len," \x09,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0')  windpi=abs(val);
      free(f1);
    } else if (strcmp(tok,"minus_hyphen")==0) {
      f1=getitok2(&s2,&len," \x09,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') minus_hyphen=val;
      free(f1);
    }
    free(tok);
    free(str);
  }
  closeconfig(fp);
  return 0;
}

int main(int argc,char **argv)
{
  int i;
  char *lib,*home;
  char *filename,*metafile;

  int sx,sy,left,right,top,bottom;
  HDC DC;
  int sd2,len;
  METAFILEHEADER MetaHeader;
  METAHEADER mh;
  WORD *Header;
  FILE *fp;
  char *buf,buf2[2048];
  gchar tmpfile[] = "GMXXXXXX";

  printfstderr("Ngraph - Windows(R) Metafile Driver: version "VERSION"\n");

  if ((lib=getenv("NGRAPHCONF"))!=NULL) {
    if ((libdir=(char *)malloc(strlen(lib)+1))==NULL) exit(1);
    strcpy(libdir,lib);
  } else {
    if ((libdir=(char *)malloc(strlen(CONFDIR)+1))==NULL) exit(1);
    strcpy(libdir,CONFDIR);
  }
  if ((home=getenv("NGRAPHHOME"))!=NULL) {
    if ((homedir=(char *)malloc(strlen(home)+1))==NULL) exit(1);
    strcpy(homedir,home);
  } else if ((home=getenv("HOME"))!=NULL) {
    if ((homedir=(char *)malloc(strlen(home)+1))==NULL) exit(1);
    strcpy(homedir,home);
  } else if ((home=getenv("Home"))!=NULL) {
    if ((homedir=(char *)malloc(strlen(home)+1))==NULL) exit(1);
    strcpy(homedir,home);
  } else {
    if ((homedir=(char *)malloc(strlen(libdir)+1))==NULL) exit(1);
    strcpy(homedir,libdir);
  }

  metafile=NULL;
  windpi=576;
  minus_hyphen=TRUE;

  loadconfig();

  for (i=1;(i<argc) && (argv[i][0]=='-');i++) {
    switch (argv[i][1]) {
    case 'o':
      if (((i+1)<argc) && (argv[i+1][0]!='-')) {
        metafile=argv[i+1];
        i++;
      }
      break;
    case 'd':
      if (((i+1)<argc) && (argv[i+1][0]!='-')) {
        windpi=atoi(argv[i+1]);
        if (windpi<96) windpi=96;
        if (windpi>2540) windpi=2540;
        i++;
      }
      break;
    default:
      printfstderr("error: unknown option `%s'.\n",argv[i]);
      exit(1);
    }
  }

  if (i>=argc) {
    printfstderr("Usage: gra2wmf [options] grafile\n");
    printfstderr("Options:\n");
    printfstderr(" -o file     : output file\n");
    printfstderr(" -d dpi      : resolution in dpi (96-2540)\n");
    exit(1);
  }

  filename=argv[i];
  if ((fp=fopen(filename,"rt"))==NULL) {
    printfstderr("error: file not found `%s'.",filename);
    exit(1);
  }
  if (fgetline(fp,&buf)==1) {
    printfstderr("error: illegal GRA format.");
    exit(1);
  }
  if (strcmp(buf,GRAF)!=0) {
    printfstderr("error: illegal GRA format.");
    free(buf);
    exit(1);
  }
  free(buf);
  fclose(fp);

  if ((fp=fopen(filename,"rt"))!=NULL) {
    if (fgetline(fp,&buf)!=1) {
      if (strcmp(buf,GRAF)==0) {
        free(buf);
        while (fgetline(fp,&buf)!=1) {
          if (!GRAinput(buf,getboundingbox)) {
            printfstderr("error: illegal GRA format.");
            fclose(fp);
            free(buf);
            exit(1);
          }
          free(buf);
        }
        fclose(fp);
      } else free(buf);
    }
  }

  sx=(bmaxx-bminx)/100;
  sy=(bmaxy-bminy)/100;
  bminx-=sx;
  bminy-=sy;
  bmaxx+=sx;
  bmaxy+=sy;
  left=0;
  top=0;
  right=bmaxx-bminx;
  bottom=bmaxy-bminy;

  MetaHeader.key=0x9AC6CDD7L;
  MetaHeader.hmf=0;
  MetaHeader.bbox.left=left*windpi/2540;
  MetaHeader.bbox.top=top*windpi/2540;
  MetaHeader.bbox.right=right*windpi/2540;
  MetaHeader.bbox.bottom=bottom*windpi/2540;
  MetaHeader.inch=windpi;
  MetaHeader.reserved=0;
  Header=(WORD *)&MetaHeader;
  MetaHeader.checksum= *(Header+0) ^ *(Header+1) ^ *(Header+2) ^ 
                       *(Header+3) ^ *(Header+4) ^ *(Header+5) ^
                       *(Header+6) ^ *(Header+7) ^ *(Header+8) ^ *(Header+9);
#if (GLIB_MAJOR_VERSION > 2 || (GLIB_MAJOR_VERSION == 2 && GLIB_MINOR_VERSION >= 22))
  DC = g_mkstemp_full(tmpfile, O_RDWR | O_BINARY, S_IRUSR | S_IWUSR);
#else	/* GLIB_MAJOR_VERSION */
  DC = g_mkstemp(tmpfile);
#ifdef __MINGW32__
  setmode(DC, O_BINARY);
#endif	/* __MINGW32__ */
#endif	/* GLIB_MAJOR_VERSION */
  if (DC < 0) {
    printfstderr("error: open temp file.");
    exit(1);
  }

  CreateMetaFile(DC);

  SetMapMode(DC,MM_ANISOTROPIC);
  SetWindowExt(DC,right*windpi/2540,bottom*windpi/2540);
  SetWindowOrg(DC,0,0);

  PrintPage(DC,filename,bminx*windpi/2540,bminy*windpi/2540);

  CloseMetaFile(DC, &mh);

  sd2=open(metafile,O_CREAT|O_TRUNC|O_WRONLY | O_BINARY,S_IREAD|S_IWRITE);
  if (sd2<0) {
    printfstderr("error: file open `%s'.",metafile);
    goto ErrExit;
  }

  chk_write(sd2,&(MetaHeader.key),sizeof(MetaHeader.key));
  chk_write(sd2,&(MetaHeader.hmf),sizeof(MetaHeader.hmf));
  chk_write(sd2,&(MetaHeader.bbox),sizeof(MetaHeader.bbox));
  chk_write(sd2,&(MetaHeader.inch),sizeof(MetaHeader.inch));
  chk_write(sd2,&(MetaHeader.reserved),sizeof(MetaHeader.reserved));
  chk_write(sd2,&(MetaHeader.checksum),sizeof(MetaHeader.checksum));
  chk_write(sd2,&(mh.mtType),sizeof(mh.mtType));
  chk_write(sd2,&(mh.mtHeaderSize),sizeof(mh.mtHeaderSize));
  chk_write(sd2,&(mh.mtVersion),sizeof(mh.mtVersion));
  chk_write(sd2,&(mh.mtSize),sizeof(mh.mtSize));
  chk_write(sd2,&(mh.mtNoObjects),sizeof(mh.mtNoObjects));
  chk_write(sd2,&(mh.mtMaxRecord),sizeof(mh.mtMaxRecord));

  if (chk_write(sd2,&(mh.mtNoParameters),sizeof(mh.mtNoParameters))) { 
    printfstderr("error: write file `%s'.", metafile);
    goto ErrExit;
  }

  while ((len=read(DC,buf2,sizeof(buf2)))>0) {
    if (chk_write(sd2,buf2,len)) {
      printfstderr("error: write file `%s'.", metafile);
      goto ErrExit;
    }
  }
  close(DC);
  close(sd2);

  g_unlink(tmpfile);

  free(libdir);
  free(homedir);
  return 0;

 ErrExit:
  close(DC);
  g_unlink(tmpfile);

  return 1;
}


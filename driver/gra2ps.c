/**
 *
 * $Id: gra2ps.c,v 1.5 2009/04/04 06:18:47 hito Exp $
 *
 * This is free software; you can redistribute it and/or modify it.
 *
 * Original author: Satoshi ISHIZAKA
 *                  isizaka@msa.biglobe.ne.jp
 **/

#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#ifdef WINDOWS
#include <io.h>
#include <dir.h>
#else
#include <unistd.h>
#endif

#include "dir_defs.h"

#define MPI 3.1415926535897932385

#define GRAF "%Ngraph GRAF"
#ifndef LIBDIR
#define LIBDIR "/usr/local/lib/Ngraph"
#endif
#define CONF "gra2ps.ini"
#define GRA2CONF "[gra2ps]"
#define DIRSEP '/'
#define CONFSEP "/"
#define NSTRLEN 256
#define LINETOLIMIT 500
#define VERSION "2.03.20"

#define TRUE  1
#define FALSE 0

enum {A3=0,A4,B4,A5,B5,LETTER,LEGAL};
enum {NORMAL=0,BOLD,ITALIC,BOLDITALIC};

struct fontmap;
struct fontmap {
  char *fontalias;
  char *fontname;
  int type;
  int used;
  struct fontmap *next;
} *fmap=NULL;

char *outfilename;
FILE *outfp;
char *libdir,*homedir;
char *includefile=NULL;
int include=FALSE;
int epsf=FALSE;
int color=FALSE;
int landscape=FALSE;
int paper=A4;
int rotate=FALSE;
int bottom=29700;
int sjis=FALSE;
int font_slant=12;

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

  po=malloc(NSTRLEN);
  po[0]='\0';
  return po;
}

char *nstrccat(char *po,char ch)
{
  size_t len,num;

  if (po==NULL) return NULL;
  len=strlen(po);
  num=len/NSTRLEN;
  if (len%NSTRLEN==NSTRLEN-1) po=realloc(po,NSTRLEN*(num+2));
  po[len]=ch;
  po[len+1]='\0';
  return po;
}

int fgetline(FILE *fp,char **buf)
{
  char *s;
  int ch;

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
  po=malloc(*len+1);
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
        if ((tok=getitok2(&s,&len," \t=,"))!=NULL) {
          for (;(s[0]!='\0') && (strchr(" \t=,",s[0])!=NULL);s++);
          *val=malloc(strlen(s)+1);
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
    while ((s[pos1]!='\0') && (strchr(" \t,",s[pos1])!=NULL)) pos1++;
    if (s[pos1]=='\0') return FALSE;
    pos2=0;
    while ((s[pos1]!='\0') && (strchr(" \t,",s[pos1])==NULL)) {
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
  while ((s[pos]==' ') || (s[pos]=='\t')) pos++;
  if (s[pos]=='\0') return TRUE;
  if (strchr("IE%VAGOMNLTCBPRDFHSK",s[pos])==NULL) return FALSE;
  code=s[pos];
  if (strchr("%FSK",code)==NULL) {
    if (!getintpar(s+pos+1,1,&num)) return FALSE;
    num++;
    cpar=malloc(sizeof(int)*num);
    if (!getintpar(s+pos+1,num,cpar)) goto errexit;
  } else {
    cpar=malloc(sizeof(int));
    cpar[0]=-1;
    cstr=malloc(strlen(s)-pos);
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

void putstdout(char *s)
{
  fputs(s,outfp);
  fputs("\n",outfp);
}

void printfstdout(char *fmt,...)
{
  va_list ap;

  va_start(ap,fmt);
  vfprintf(outfp,fmt,ap);
  va_end(ap);
}

char *fontalias=NULL;
double fontsize=2000;
int fonttype=NORMAL;
char textspace[256];
int loadfont=FALSE;
int setcp=FALSE;
int cpx=0;
int cpy=0;
int lineto=FALSE;
int linetonum=0;

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
  int i,lw;
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
    setbbminmax(cpar[1]+(int )(cpar[3]*cos(MPI*cpar[5]/18000.0)),
                cpar[2]-(int )(cpar[4]*sin(MPI*cpar[5]/18000.0)),
                cpar[1]+(int )(cpar[3]*cos((MPI*cpar[5]+cpar[6])/18000.0)),
                cpar[2]-(int )(cpar[4]*sin((MPI*cpar[5]+cpar[6])/18000.0)),lw);
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
    bfontalias=malloc(strlen(cstr)+1);
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
    csin=sin(MPI*bdir/18000.0);
    ccos=cos(MPI*bdir/18000.0);
    i=0;
    while (i<strlen(cstr)) {
      ch=0;
      if (cstr[i]=='\\') {
        if (cstr[i+1]=='x') {
          if (tolower(cstr[i+2])>='a') c1=tolower(cstr[i+2])-'a'+10;
          else c1=tolower(cstr[i+2])-'0';
          if (tolower(cstr[i+3])>='a') c2=tolower(cstr[i+3])-'a'+10;
          else c2=tolower(cstr[i+3])-'0';
          ch=c1*16+c2;
          i+=4;
        } else if (cstr[i+1]=='\\') {
          ch=cstr[i];
          i+=2;
        } else i++;
      } else {
       switch (cstr[i]) {
       case '\\':
         ch='\\';
         break;
       case ')':
         ch=')';
         break;
       case '(':
         ch='(';
         break;
       default:
         if (cstr[i]&0x80) {
           c1=((unsigned char)cstr[i])>>4;
           c2=((unsigned char)cstr[i]) & 0xF;
           ch=c1*16+c2;
         } else ch=cstr[i];
         break;
       }
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
    csin=sin(MPI*bdir/18000.0);
    ccos=cos(MPI*bdir/18000.0);
    i=0;
    while (i<strlen(cstr)) {
      if (niskanji((unsigned char)cstr[i]) && (cstr[i+1]!='\0')) {
        w=charwidth(cstr[i],bfontalias,bpt);
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

void draw(char code,int *cpar,char *cstr)
{
  time_t t;
  struct tm *ltime;
  char *buf,*fontname;
  FILE *fp;
  int i,a;
  struct fontmap *fcur;
  unsigned int jis,R,G,B;
  int c1,c2,sx,sy;
  double d,ftan;

  if (lineto) {
    if (code!='T') {
      putstdout("st");
      setcp=FALSE;
      lineto=FALSE;
      linetonum=0;
    } else if (linetonum>LINETOLIMIT) {
      putstdout("st");
      printfstdout("%d %d mto\n",cpx,cpy);
      lineto=FALSE;
      linetonum=1;
    }
  }
  switch (code) {
  case 'X':
    printfstdout("%s",cstr);
    break;
  case 'I':
    printfstderr("Include (%s)\n",includefile);
    if ((fp=fopen(includefile,"rt"))==NULL) {
      printfstderr("Error: no include file %s\n",includefile);
      exit(1);
    }
    while (fgetline(fp,&buf)!=1) {
      if (strcmp(buf,"%%Creator:")==0) {
        printfstdout("%s GRA2PS %s\n",buf,VERSION);
      } else if (strcmp(buf,"%%CreationDate:")==0) {
        t=time(NULL);
        ltime=localtime(&t);
        printfstdout("%s %d-%d-%d %d:%d\n",buf,
                     ltime->tm_mon+1,ltime->tm_mday,1900+ltime->tm_year,
					 ltime->tm_hour,ltime->tm_min);
	  } else if (strcmp(buf,"%%Pages:")==0) {
        printfstdout("%s %d\n",buf,1);
      } else if (strcmp(buf,"%%BoundingBox:")==0) {
        if (epsf) {
          if (bminx>bmaxx) {
            bminx=0;
            bmaxx=0;
          }
          if (bminy>bmaxy) {
            bminy=bottom;
            bmaxy=bottom;
          }
          sx=(bmaxx-bminx)/100;
          sy=(bmaxy-bminy)/100;
          bminx-=sx;
          bminy-=sy;
          bmaxx+=sx;
          bmaxy+=sy;
          if (rotate)
            printfstdout("%s %d %d %d %d\n",buf,
                        (int )(bminy/2540.0*72),
                        (int )(bminx/2540.0*72),
                        (int )(bmaxy/2540.0*72),
                        (int )(bmaxx/2540.0*72));
          else
            printfstdout("%s %d %d %d %d\n",buf,
                        (int )(bminx/2540.0*72),
                        (int )((bottom-bmaxy)/2540.0*72),
                        (int )(bmaxx/2540.0*72),
                        (int )((bottom-bminy)/2540.0*72));
        } else {
          if (rotate)
            printfstdout("%s %d %d %d %d\n",buf,
                        (int )(0),
                        (int )(0),
                        (int )(cpar[4]/2540.0*72),
                        (int )(cpar[3]/2540.0*72));
          else
            printfstdout("%s %d %d %d %d\n",buf,
                        (int )(0),
                        (int )((bottom-cpar[4])/2540.0*72),
                        (int )(cpar[3]/2540.0*72),
                        (int )(bottom/2540.0*72));
        }
      } else if ((strcmp(buf,"%%EndComments")==0) && (epsf)) {
        putstdout(buf);
      } else {
        putstdout(buf);
      }
      free(buf);
    }
    fclose(fp);
    putstdout("%%Page: Figure 1");
    putstdout("%%BeginPageSetup");
    if (rotate) printfstdout("/ANGLE %d def\n",-90);
    else printfstdout("/ANGLE %d def\n",0);
    printfstdout("/bottom %d def\n",-bottom);
    putstdout("NgraphDict begin");
    putstdout("PageSave");
    putstdout("BeginPage");
    putstdout("%%EndPageSetup");
    include=TRUE;
    break;
  case 'E':
    putstdout("EndPage");
    putstdout("PageRestore");
    putstdout("showpage");
    putstdout("end");
    putstdout("%%Trailer");
    putstdout("%%DocumentNeededResources: ");
    fcur=fmap;
    while (fcur!=NULL) {
      if (fcur->used)
        printfstdout("%% font %s\n",fcur->fontname);
      fcur=fcur->next;
    }
    putstdout("%%EOF");
    break;
  case '%':
    if (include) printfstdout("%c%s\n",'%',cstr);
    printfstderr("%c%s\n",'%',cstr);
    break;
  case 'V':
   if (color)
     printfstdout("%d %d %d %d %d viewc\n",
     cpar[1],cpar[2],cpar[3]-cpar[1],cpar[4]-cpar[2],cpar[5]);
   else
     printfstdout("%d %d %d %d %d view\n",
     cpar[1],cpar[2],cpar[3]-cpar[1],cpar[4]-cpar[2],cpar[5]);
    break;
  case 'A':
    if (cpar[1]==0) printfstdout("[] 0 ");
    else {
      printfstdout("[");
      for (i=0;i<cpar[1];i++) printfstdout("%d ",cpar[i+6]);
      printfstdout("] 0 ");
    }
    printfstdout("%d %d %d %g lsty\n",cpar[2],cpar[3],cpar[4],cpar[5]/100.0);
    break;
  case 'G':
    R=cpar[1];
    G=cpar[2];
    B=cpar[3];
    if (color)
      printfstdout(
      "%d 255 div %d 255 div %d 255 div setrgbcolor\n",R,G,B);
    else {
      d=0.3*R+0.59*G+0.11*B;
      a=(int )d;
      if (d-a>=0.5) a++;
      printfstdout("%d 255 div setgray\n",a);
    }
    break;
  case 'M':
    printfstdout("%d %d mto\n",cpar[1],cpar[2]);
    cpx=cpar[1];
    cpy=cpar[2];
    setcp=TRUE;
    break;
  case 'N':
    printfstdout("%d %d rmto\n",cpar[1],cpar[2]);
    cpx+=cpar[1];
    cpy+=cpar[2];
    setcp=TRUE;
    break;
  case 'L':
    printfstdout("%d %d %d %d l\n",cpar[1],cpar[2],cpar[3],cpar[4]);
    setcp=FALSE;
    break;
  case 'T':
    if (!setcp) printfstdout("%d %d mto\n",cpx,cpy);
    printfstdout("%d %d lto\n",cpar[1],cpar[2]);
    cpx=cpar[1];
    cpy=cpar[2];
    lineto=TRUE;
    linetonum++;
    setcp=TRUE;
    break;
  case 'C':
	if (cpar[7]==0)
	  printfstdout("%d %d %d %d %d %d e\n",
        -cpar[5]-cpar[6],-cpar[5],cpar[3],cpar[4],cpar[1],cpar[2]);
    else if (cpar[7]==1)
      printfstdout("%d %d %d %d %d %d fe\n",
        -cpar[5]-cpar[6],-cpar[5],cpar[3],cpar[4],cpar[1],cpar[2]);
    else
      printfstdout("%d %d %d %d %d %d ae\n",
        -cpar[5]-cpar[6],-cpar[5],cpar[3],cpar[4],cpar[1],cpar[2]);
    setcp=FALSE;
    break;
  case 'B':
    if (cpar[5]==0)
      printfstdout("%d %d %d %d rec\n",
                   cpar[3]-cpar[1],cpar[4]-cpar[2],cpar[1],cpar[2]);
    else
      printfstdout("%d %d %d %d bar\n",
                   cpar[3]-cpar[1],cpar[4]-cpar[2],cpar[1],cpar[2]);
    setcp=FALSE;
    break;
  case 'P':
    printfstdout("%d %d pix\n",cpar[1],cpar[2]);
    break;
  case 'R':
    for (i=0;i<cpar[1];i++)
      printfstdout("%d %d ",cpar[i*2+2],cpar[i*2+3]);
    printfstdout("%d ",cpar[1]);
    putstdout("rpo");
    setcp=FALSE;
    break;
  case 'D':
    for (i=0;i<cpar[1];i++)
      printfstdout("%d %d ",cpar[i*2+3],cpar[i*2+4]);
    printfstdout("%d ",cpar[1]);
    if (cpar[2]==0) putstdout("dpo");
    else if (cpar[2]==1) putstdout("epo");
    else putstdout("fpo");
    setcp=FALSE;
    break;
  case 'F':
    free(fontalias);
    fontalias=malloc(strlen(cstr)+1);
    strcpy(fontalias,cstr);
    break;
  case 'H':
    fontname=NULL;
    if (fontalias==NULL) {
      loadfont=FALSE;
      break;
    }
    fcur=fmap;
    while (fcur!=NULL) {
      if (strcmp(fontalias,fcur->fontalias)==0) {
        fontname=fcur->fontname;
        fonttype=fcur->type;
        if (!(fcur->used))
          printfstdout("%%IncludeResource: font %s\n",fcur->fontname);
        fcur->used=TRUE;
        break;
      }
      fcur=fcur->next;
    }
    if (fontname==NULL) {
      loadfont=FALSE;
      break;
    }
    fontsize=cpar[1]/72.0*25.4;
    sprintf(textspace,"%g %g",cpar[2]*cos(MPI*cpar[3]/18000.0)*25.4/72,
                             -cpar[2]*sin(MPI*cpar[3]/18000.0)*25.4/72);
    if ((fonttype==ITALIC) || (fonttype==BOLDITALIC)) {
      ftan=tan(font_slant*MPI/180.0);
      printfstdout("[%g %g %g %g 0 0] ",
         cpar[1]*cos(MPI*cpar[3]/18000.0)*25.4/72,
        -cpar[1]*sin(MPI*cpar[3]/18000.0)*25.4/72,
        -cpar[1]*(sin(MPI*cpar[3]/18000.0)
         -ftan*cos(MPI*cpar[3]/18000.0))*25.4/72,
        -cpar[1]*(cos(MPI*cpar[3]/18000.0)
         +ftan*sin(MPI*cpar[3]/18000.0))*25.4/72);
    } else {
      printfstdout("[%g %g %g %g 0 0] ",
         cpar[1]*cos(MPI*cpar[3]/18000.0)*25.4/72,
        -cpar[1]*sin(MPI*cpar[3]/18000.0)*25.4/72,
        -cpar[1]*sin(MPI*cpar[3]/18000.0)*25.4/72,
        -cpar[1]*cos(MPI*cpar[3]/18000.0)*25.4/72);
    }
    printfstdout("%s F\n",fontname);
    loadfont=TRUE;
    break;
  case 'S':
    if (!loadfont) break;
    if ((fonttype==BOLD) || (fonttype==BOLDITALIC)) {
      printfstdout("/BOLDS %g def\n",fontsize*0.018);
    }
    printfstdout("%s (",textspace);
    i=0;
    while (i<strlen(cstr)) {
      if (cstr[i]=='\\') {
        if (cstr[i+1]=='x') {
          if (tolower(cstr[i+2])>='a') c1=tolower(cstr[i+2])-'a'+10;
          else c1=tolower(cstr[i+2])-'0';
          if (tolower(cstr[i+3])>='a') c2=tolower(cstr[i+3])-'a'+10;
          else c2=tolower(cstr[i+3])-'0';
          printfstdout("\\%c%c%c",c1/4+'0',(c1%4)*2+c2/8+'0',c2%8+'0');
          i+=4;
        } else if (cstr[i+1]=='\\') {
          printfstdout("\\\\");
          i+=2;
        } else i++;
      } else {
        switch (cstr[i]) {
        case '\\':
          printfstdout("\\\\");
          break;
        case ')':
          printfstdout("\\)");
          break;
        case '(':
          printfstdout("\\(");
          break;
        default:
          if (cstr[i]&0x80) {
            c1=((unsigned char)cstr[i])>>4;
            c2=((unsigned char)cstr[i]) & 0xF;
            printfstdout("\\%c%c%c",c1/4+'0',(c1%4)*2+c2/8+'0',c2%8+'0');
          } else printfstdout("%c",cstr[i]);
          break;
        }
        i++;
      }
    }
    if ((fonttype==BOLD) || (fonttype==BOLDITALIC)) {
      printfstdout(") ashowB\n");
    } else {
      printfstdout(") ashow\n");
    }
    break;
  case 'K':
    if (!loadfont) break;
    if ((fonttype==BOLD) || (fonttype==BOLDITALIC)) {
      printfstdout("/BOLDS %g def\n",fontsize*0.018);
    }
    printfstdout("%s <",textspace);
    i=0;
    while (i<strlen(cstr)) {
      if (niskanji((unsigned char)cstr[i]) && (cstr[i+1]!='\0')) {
        if (!sjis)
          jis=njms2jis(((unsigned char)cstr[i] << 8)+(unsigned char)cstr[i+1]);
        else
          jis=((unsigned char)cstr[i] << 8)+(unsigned char)cstr[i+1];
        printfstdout(" %2hx%2hx",jis>>8,jis &0xff);
        i+=2;
      } else i++;
    }
    if ((fonttype==BOLD) || (fonttype==BOLDITALIC)) {
      printfstdout("> ashowB\n");
    } else {
      printfstdout("> ashow\n");
    }
    break;
  }
}

void loadconfig(void)
{
  FILE *fp;
  char *tok,*str,*s2;
  char *f1,*f2,*f3;
  struct fontmap *fcur,*fnew;
  int val;
  char *endptr;
  int len;

  if ((fp=openconfig(GRA2CONF))==NULL) {
    printfstderr("Error: configuration file (%s) is not found.\n",CONF);
    exit(1);
  }
  fcur=NULL;
  while ((tok=getconfig(fp,&str))!=NULL) {
    s2=str;
    if (strcmp(tok,"font_map")==0) {
      f1=getitok2(&s2,&len," \t,");
      f2=getitok2(&s2,&len," \t,");
      f3=getitok2(&s2,&len," \t,");
      if ((f1!=NULL) && (f2!=NULL) && (f3!=NULL)) {
        fnew=malloc(sizeof(struct fontmap));
        if (fcur==NULL) fmap=fnew;
        else fcur->next=fnew;
        fcur=fnew;
        fcur->next=NULL;
        fcur->fontalias=f1;
        if (strcmp(f2,"bold")==0) fcur->type=BOLD;
        else if (strcmp(f2,"italic")==0) fcur->type=ITALIC;
        else if (strcmp(f2,"bold_italic")==0) fcur->type=BOLDITALIC;
        else fcur->type=NORMAL;
        fcur->fontname=f3;
        fcur->used=FALSE;
        free(f2);
      } else {
        free(f1);
        free(f2);
        free(f3);
      }
    } else if (strcmp(tok,"include")==0) {
      includefile=getitok2(&s2,&len,"");
    } else if (strcmp(tok,"color")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') {
        if (val==0) color=FALSE;
        else color=TRUE;
      }
      free(f1);
    } else if (strcmp(tok,"sjis")==0) {
      f1=getitok2(&s2,&len," \t,");
      val=strtol(f1,&endptr,10);
      if (endptr[0]=='\0') {
        if (val==0) sjis=FALSE;
        else sjis=TRUE;
      }
      free(f1);
    }
    free(tok);
    free(str);
  }
  closeconfig(fp);
}

int main(int argc,char **argv)
{
  int i;
  char *filename;
  FILE *fp;
  char *buf;
  char *lib,*home;
  char *tmpname;
  int ch;
  char *endptr;
  int a;

  printfstderr("Ngraph - PostScript(R) Printer Driver version: "VERSION"\n");

#ifndef WINDOWS
  if ((lib=getenv("NGRAPHLIB"))!=NULL) {
    if ((libdir=(char *)malloc(strlen(lib)+1))==NULL) exit(1);
    strcpy(libdir,lib);
  } else {
    if ((libdir=(char *)malloc(strlen(DATADIR)+1))==NULL) exit(1);
    strcpy(libdir,DATADIR);
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
#else
  if ((lib=getenv("NGRAPHLIB"))!=NULL) {
    if ((libdir=(char *)malloc(strlen(lib)+1))==NULL) exit(1);
    strcpy(libdir,lib);
  } else {
    for (i=strlen(argv[0]);(argv[0][i]!='\\') && (i>0);i--);
    if ((libdir=(char *)malloc(i+1))==NULL) exit(1);
    strncpy(libdir,argv[0],i);
    libdir[i]='\0';
  }
  if ((home=getenv("NGRAPHHOME"))!=NULL) {
    if ((homedir=(char *)malloc(strlen(home)+1))==NULL) exit(1);
    strcpy(homedir,home);
  } else if ((home=getenv("HOME"))!=NULL) {
    if ((homedir=(char *)malloc(strlen(home)+1))==NULL) exit(1);
    strcpy(homedir,home);
  } else {
    if ((homedir=(char *)malloc(strlen(libdir)+1))==NULL) exit(1);
    strcpy(homedir,libdir);
  }
#endif
  includefile=malloc(strlen(libdir)+11);
  strcpy(includefile,libdir);
  strcat(includefile,"/Ngraph.ps");

  loadconfig();

  outfilename=NULL;
  for (i=1;(i<argc) && (argv[i][0]=='-') && (argv[i][1]!='\0');i++) {
    switch (argv[i][1]) {
    case 'o':
      if (((i+1)<argc) && (argv[i+1][0]!='-')) {
        outfilename=argv[i+1];
        i++;
      }
      break;
    case 'i':
      if (((i+1)<argc) && (argv[i+1][0]!='-')) {
        free(includefile);
        if ((includefile=malloc(strlen(argv[i+1])+1))!=NULL)
          strcpy(includefile,argv[i+1]);
        i++;
      }
      break;
    case 'c':
      color=TRUE;
      break;
    case 'e':
      epsf=TRUE;
      break;
    case 'p':
      if (((i+1)<argc) && (argv[i+1][0]!='-')) {
        if ((strcmp(argv[i+1],"A3")==0) || (strcmp(argv[i+1],"a3")==0))
          paper=A3;
        else if ((strcmp(argv[i+1],"A4")==0) || (strcmp(argv[i+1],"a4")==0))
          paper=A4;
        else if ((strcmp(argv[i+1],"B4")==0) || (strcmp(argv[i+1],"b4")==0))
          paper=B4;
        else if ((strcmp(argv[i+1],"A5")==0) || (strcmp(argv[i+1],"a5")==0))
          paper=A5;
        else if ((strcmp(argv[i+1],"B5")==0) || (strcmp(argv[i+1],"b5")==0))
          paper=B5;
        else if ((strcmp(argv[i+1],"LETTER")==0)
        || (strcmp(argv[i+1],"letter")==0))
          paper=LETTER;
        else if ((strcmp(argv[i+1],"LEGAL")==0)
        || (strcmp(argv[i+1],"legal")==0))
          paper=LEGAL;
        i++;
      }
      break;
    case 'l':
      landscape=TRUE;
      break;
    case 'r':
      rotate=TRUE;
      break;
    case 's':
      if ((i+1)<argc) {
        a=strtol(argv[i+1],&endptr,10);
        if ((endptr[0]=='\0') && (0<=a) && (a<=60)) font_slant=a;
        i++;
      }
      break;
    default:
      printfstderr("unknown option `%s'.\n",argv[i]);
      free(includefile);
      free(libdir);
      free(homedir);
      exit(1);
    }
  }

  if (i>=argc) {
    printfstderr("Usage: gra2ps [options] grafile\n");
    printfstderr("Options:\n");
    printfstderr(" -o file                        : output file\n");
    printfstderr(" -i file                        : include file\n");
    printfstderr(" -c                             : color output\n");
    printfstderr(" -e                             : EPSF output\n");
    printfstderr(" -p a3|a4|b4|a5|b5|letter|legal : paper\n");
    printfstderr(" -l                             : landscape\n");
    printfstderr(" -r                             : rotate by 90 degree\n");
    printfstderr(" -s theta                       : slant angle (degree)\n");
    free(includefile);
    free(libdir);
    free(homedir);
    exit(1);
  }

  if (!rotate) {
    switch (paper) {
    case A3:
      if (landscape) bottom=29700;
      else bottom=42000;
      break;
    case A4:
      if (landscape) bottom=21000;
      else bottom=29700;
      break;
    case B4:
      if (landscape) bottom=25700;
      else bottom=36400;
      break;
    case B5:
      if (landscape) bottom=18200;
      else bottom=25700;
      break;
    case LETTER:
      if (landscape) bottom=21590;
      else bottom=27940;
      break;
    case LEGAL:
      if (landscape) bottom=21590;
      else bottom=35560;
      break;
    }
  } else bottom=0;

  if (outfilename!=NULL) {
    if ((outfp=fopen(outfilename,"wt"))==NULL) {
      fprintf(stderr,"error: file open `%s'.\n",outfilename);
      free(includefile);
      free(libdir);
      free(homedir);
      exit(1);
    }
  } else outfp=stdout;

  filename=argv[i];
  tmpname=NULL;
  if (epsf && (filename[0]=='-') && (filename[1]=='\0')) {
	if ((tmpname=tempnam(NULL,"GR2PS"))==NULL) exit(1);
    if ((fp=fopen(tmpname,"wt"))==NULL) exit(1);
    while ((ch=fgetc(stdin))!=EOF) fputc(ch,fp);
    fclose(fp);
    filename=tmpname;
  }
  if (epsf) {
    if ((fp=fopen(filename,"rt"))==NULL) {
      printfstderr("error: file not found `%s'.\n",filename);
      goto errexit;
    }
    if (fgetline(fp,&buf)==1) {
      printfstderr("error: illegal GRA format.\n");
      goto errexit;
    }
    if (strcmp(buf,GRAF)!=0) {
      printfstderr("error: illegal GRA format.\n");
      goto errexit;
    }
    free(buf);
    while (fgetline(fp,&buf)!=1) {
      if (!GRAinput(buf,getboundingbox)) {
        printfstderr("error: illegal GRA format.\n");
        goto errexit;
      }
      free(buf);
    }
    fclose(fp);
  }
  if ((filename[0]=='-') && (filename[1]=='\0')) fp=stdin;
  else {
    if ((fp=fopen(filename,"rt"))==NULL) {
      printfstderr("error: file not found `%s'.\n",filename);
      goto errexit;
    }
  }
  if (fgetline(fp,&buf)==1) {
    printfstderr("error: illegal GRA format.\n");
    goto errexit;
  }
  if (strcmp(buf,GRAF)!=0) {
    printfstderr("error: illegal GRA format.\n");
    goto errexit;
  }
  free(buf);
  while (fgetline(fp,&buf)!=1) {
    if (!GRAinput(buf,draw)) {
      printfstderr("error: illegal GRA format.\n");
      goto errexit;
    }
    free(buf);
  }
  fclose(outfp);
  fclose(fp);
  if (tmpname!=NULL) {
    unlink(tmpname);
    free(tmpname);
  }
  free(includefile);
  free(libdir);
  free(homedir);
  return 0;
errexit:
  if (tmpname!=NULL) {
    unlink(tmpname);
    free(tmpname);
  }
  free(includefile);
  free(libdir);
  free(homedir);
  return 1;
}

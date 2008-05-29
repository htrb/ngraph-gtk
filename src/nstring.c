/* 
 * $Id: nstring.c,v 1.1.1.1 2008/05/29 09:37:33 hito Exp $
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
#include "object.h"
#include "nstring.h"

#define NSTRLEN 0x400

#define TRUE 1
#define FALSE 0

char *nstrnew(void)
{
  char *po;

  if ((po=memalloc(NSTRLEN))==NULL) return NULL;
  po[0]='\0';
  return po;
}

char *
nstrdup(const char *src)
{
  char *dest;

  if (src == NULL)
    return NULL;

  dest = memalloc(strlen(src) + 1);

  if (dest == NULL)
    return NULL;

  strcpy(dest, src);

  return dest;
}


char *nstrccat(char *po,char ch)
{
  size_t len,num;
  char *po2;

  if (po==NULL) return NULL;
  len=strlen(po);
  num=len/NSTRLEN;
  if (len%NSTRLEN==NSTRLEN-1) {
    if ((po2=memrealloc(po,NSTRLEN*(num+2)))==NULL) {
      memfree(po);
      return NULL;
    }
    po=po2;
  }
  po[len]=ch;
  po[len+1]='\0';
  return po;
}

char *
nstraddchar(char *po, int len, char ch)
{
  char *po2;

  if (po == NULL) return NULL;

  if ((len & (NSTRLEN - 1)) == NSTRLEN - 1) {
    po2 = memrealloc(po, NSTRLEN * (len / NSTRLEN + 2));
    if (po2 == NULL) {
      memfree(po);
      return NULL;
    }
    po = po2;
  }
  po[len] = ch;
  po[len + 1] = '\0';
  return po;
}

char *nstrcat(char *po,char *s)
{
  size_t i, len;

  /* modified */

  if (po == NULL) return NULL;
  if (s == NULL) return po; 
  len = strlen(po);
  for (i = 0; s[i] != '\0'; i++) 
    po = nstraddchar(po, len + i, s[i]);
    if (po == NULL)
      return NULL;
  return po;
}

char *nstrncat(char *po,char *s,size_t n)
{
  size_t i;

  if (po==NULL) return NULL;
  if (s==NULL) return po; 
  for (i=0;(s[i]!='\0') && (i<n);i++) 
    if ((po=nstrccat(po,s[i]))==NULL) return NULL;
  return po;
}

int strcmp0(const char *s1, const char *s2)
{
  const char *s3,*s4;

  if ((s1==NULL) || (s2==NULL)) return 1;
  s3=s1;
  s4=s2;
  while (s3[0]==s4[0]) {
    if ((s3[0]=='\0') && (s4[0]=='\0')) return 0;
    s3++;
    s4++;
  }
  return 1;
}

int strcmp2(char *s1,char *s2)
{
  int len1,len2,len,c;

  len1=strlen(s1);
  len2=strlen(s2);
  if (len1<len2) len=len1;
  else len=len2;
  c=strncmp(s1,s2,len);
  if (c==0) {
    if (len1<len2) return -1;
    else if (len1>len2) return 1;
    else return 0;
  } else return c;
}

int wildmatch2(char *pat,char *s,int flags)
{
  char *spo,*patpo,*po;

  if ((s==NULL) || (pat==NULL)) return 0;
  spo=s;
  patpo=pat;
  while (1) {
    if ((*spo=='\0') && (*patpo=='\0')) return 1;
    else if (*patpo=='\0') return 0;
    else if ((flags & WILD_PATHNAME) && (*spo=='/')) {
      if (*patpo!='/') return 0;
      patpo++;        
      spo++;
    } else if (*patpo=='?') {
      if (*spo=='\0') return 0;
      patpo++;
      spo++;
    } else if (*patpo=='*') {
      patpo++;
      while (1) {
        if (wildmatch2(patpo,spo,flags)) return 1;
        if (*spo=='\0') return 0;
        spo++;
      }
    } else if (*patpo=='[') {
      for(po=patpo+1;(*po!='\0') && (*po!=']');po++);
      if (*po=='\0') {
        if (*patpo==*spo) {
          patpo++;
          spo++;
        } else return 0;
      } else {
        patpo++;
        while (patpo!=po) {
          if ((*(patpo+1)=='-') && (*(patpo+2)!=']')) {
            if ((*patpo<=*spo) && (*spo<=*(patpo+2))) {
              patpo=po+1;
              spo++;
              break;
	    } else patpo+=3;
          } else {
            if (*patpo==*spo) {
              patpo=po+1;
              spo++;
              break;
            } else patpo++;
	  }
	}
      }
    } else if (*patpo==*spo) {
      patpo++;
      spo++;
    } else return 0;
  }
}

int wildmatch(char *pat,char *s,int flags)
{
  if ((s==NULL) || (pat==NULL)) return 0;
  if (flags & WILD_PERIOD) {
    if (s[0]=='.') {
      if (pat[0]=='.') return wildmatch2(pat+1,s+1,flags);
      else return 0;
    } else return wildmatch2(pat,s,flags);
  } else return wildmatch2(pat,s,flags);
}

char *getitok(char **s,int *len,char *ifs)
{
  char *po,*spo;
  int i;

  if (*s == NULL) return NULL;
  po= *s;
  for (i = 0; (po[i]!='\0') && (strchr(ifs, po[i]) != NULL); i++);
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

char *getitok2(char **s, int *len, char *ifs)
{
  char *po,*s2;

  if ((s2 = getitok(s, len, ifs))==NULL) return NULL;
  if ((po=memalloc(*len+1))==NULL) {
    *len=-1;
    return NULL;
  }
  strncpy(po,s2,*len);
  po[*len]='\0';
  return po;
}

char *getitok3(char **s,int *len,char *ifs)
{
  char *po,*spo;
  int i,quote;

  if (*s==NULL) return NULL;
  quote=FALSE;
  po=*s;
  for (i=0;(po[i]!='\0') && (po[i]!='"') && (strchr(ifs,po[i])!=NULL);i++);
  if (po[i]=='\0') {
    *len=0;
    return NULL;
  }
  if (po[i]=='"') {
    quote=TRUE;
    i++;
  }
  spo=po+i;
  if (quote) {
    for (;(po[i]!='\0') && (po[i]!='"');i++);
    *s+=i;
    *len=*s-spo;
    if (po[i]=='"') (*s)++;
  } else {
    for (;(po[i]!='\0') && (strchr(ifs,po[i])==NULL);i++);
    *s+=i;
    *len=*s-spo;
  }
  return spo;
}

char *getitok4(char **s,int *len,char *ifs)
{
  char *po,*s2;

  if ((s2=getitok3(s,len,ifs))==NULL) return NULL;
  if ((po=memalloc(*len+1))==NULL) {
    *len=-1;
    return NULL;
  }
  strncpy(po,s2,*len);
  po[*len]='\0';
  return po;
}


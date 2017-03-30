/*
 * $Id: nstring.c,v 1.10 2010-03-04 08:30:16 hito Exp $
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

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>

#include "object.h"
#include "nstring.h"

#define NSTRLEN 0x400

char *
nstrnew(void)
{
  char *po;

  if ((po=g_malloc(NSTRLEN))==NULL) return NULL;
  po[0]='\0';
  return po;
}

char *
nstrccat(char *po,char ch)
{
  size_t len,num;
  char *po2;

  if (po==NULL) return NULL;
  len=strlen(po);
  num=len/NSTRLEN;
  if (len%NSTRLEN==NSTRLEN-1) {
    if ((po2=g_realloc(po,NSTRLEN*(num+2)))==NULL) {
      g_free(po);
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
  //  if (po == NULL) return NULL;

  if (len >= NSTRLEN - 1 && ! ((len + 1) & (NSTRLEN - 1))) {
    char *po2;

    po2 = g_realloc(po, NSTRLEN * (len / NSTRLEN + 2));
    if (po2 == NULL) {
      g_free(po);
      return NULL;
    }
    po = po2;
  }
  po[len] = ch;
  //  po[len + 1] = '\0';
  return po;
}

char *
nstrcat(char *po,char *s)
{
  size_t i, len;

  /* modified */

  if (po == NULL) return NULL;
  if (s == NULL) return po;
  len = strlen(po);
  for (i = 0; s[i] != '\0'; i++) {
    po = nstraddchar(po, len + i, s[i]);
    if (po == NULL) {
      return NULL;
    }
  }
  po[len + i] = '\0'; /* nstraddchar() is not terminate string */
  return po;
}

#ifdef COMPILE_UNUSED_FUNCTIONS
char *
nstrncat(char *po,char *s,size_t n)
{
  size_t i;

  if (po==NULL) return NULL;
  if (s==NULL) return po;
  for (i=0;(s[i]!='\0') && (i<n);i++)
    if ((po=nstrccat(po,s[i]))==NULL) return NULL;
  return po;
}
#endif

int
strcmp0(const char *s1, const char *s2)
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

int
strcmp2(char *s1,char *s2)
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

static int
wildmatch2(const char *pat, const char *s,int flags)
{
  const char *spo,*patpo,*po;

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

int
wildmatch(const char *pat, const char *s,int flags)
{
  if ((s==NULL) || (pat==NULL)) return 0;
  if (flags & WILD_PERIOD) {
    /* "." and ".." should not match "*" */
    if (s[0]=='.') {
      if (pat[0]=='.') return wildmatch2(pat+1,s+1,flags);
      else return 0;
    } else return wildmatch2(pat,s,flags);
  } else return wildmatch2(pat,s,flags);
}

char *
getitok(char **s, int *len, const char *ifs)
{
  char *po,*spo;
  int i;

  if (*s == NULL) return NULL;
  po = *s;
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

char *
getitok2(char **s, int *len, const char *ifs)
{
  char *po,*s2;

  if ((s2 = getitok(s, len, ifs))==NULL) return NULL;
  if ((po=g_malloc(*len+1))==NULL) {
    *len=-1;
    return NULL;
  }
  strncpy(po,s2,*len);
  po[*len]='\0';
  return po;
}

static char *
get_printf_format_str(const char *str, int *len, int *pow)
{
  int n;
  GString *format;

  if (pow) {
    *pow = FALSE;
  }
  if (len) {
    *len = 0;
  }

  n = 0;
  if (str[n] != '%') {
    return NULL;
  }

  format = g_string_new("");
  if (format == NULL) {
    return NULL;
  }
  g_string_append_c(format, str[n]);
  n++;

  while (strchr("#0- +^", str[n])) {
    if (str[n] == '^') {
      if (pow) {
	*pow = TRUE;
      }
    } else {
      g_string_append_c(format, str[n]);
    }
    n++;
  }

  for (; isdigit(str[n]); n++) {
    g_string_append_c(format, str[n]);
  }

  if (str[n] == '.') {
    g_string_append_c(format, str[n]);
    n++;
    for (; isdigit(str[n]); n++) {
      g_string_append_c(format, str[n]);
    }
  }

  if (str[n] == 'l') {
    g_string_append_c(format, str[n]);
    n++;
  }

  if (str[n] == 'l') {
    g_string_append_c(format, str[n]);
    n++;
  }

  if (strchr("diouxXeEfFgGcs", str[n]) == NULL) {
    g_string_free(format, TRUE);
    return NULL;
  }

  g_string_append_c(format, str[n]);
  if (len) {
    *len = n;
  }

  return g_string_free(format, FALSE);
}

static char *
str_to_pow(const char *str)
{
  int n, i, len, pow;
  GString *pow_str;

  if (str == NULL) {
    return NULL;
  }

  n = -1;
  len = strlen(str);
  for (i = 0; i < len; i++) {
    if (str[i] == 'E' ||
	str[i] == 'e' ||
	str[i] == 'D' ||
	str[i] == 'd') {
      n = i;
      break;
    }
  }

  if (n < 0) {
    return NULL;
  }

  pow_str = g_string_new("");
  if (pow_str == NULL) {
    return NULL;
  }
  pow = atoi(str + n + 1);
  g_string_append_len(pow_str, str, n);
  if (pow) {
    g_string_append_printf(pow_str, "×10^%d@", pow);
  }
  return g_string_free(pow_str, FALSE);
}

int
add_printf_formated_str(GString *str, const char *format, const char *arg, int *len)
{
  int i, formated, pow;
  char *format2, *buf, *endptr;
  int vi;
  long long int vll;
  double vd;

  formated = FALSE;
  format2 = get_printf_format_str(format, &i, &pow);
  if (len) {
    *len = i;
  }
  if (format2 == NULL) {
    return formated;
  }

  buf = NULL;
  switch (format[i]) {
  case 'd': case 'i': case 'o': case 'u': case 'x': case 'X':
    if (i > 2 && strncmp(format2 + i - 2, "ll", 2) == 0) {
      vll = 0;
      if (arg) {
	vll = strtod(arg, &endptr);
      }
      buf = g_strdup_printf(format2, vll);
    }else {
      vi = 0;
      if (arg) {
	vi = strtod(arg, &endptr);
      }
      buf = g_strdup_printf(format2, vi);
    }
    formated = TRUE;
    break;
  case 'e': case 'E': case 'f': case 'F': case 'g': case 'G':
    if (i > 2 && strncmp(format2 + i - 2, "ll", 2) == 0) {
      break;
    }
    vd = 0.0;
    if (arg) {
      vd = strtod(arg,&endptr);
    }
    buf = g_strdup_printf(format2, vd);
    if (pow) {
      char *new_buf;
      new_buf = str_to_pow(buf);
      if (new_buf) {
	g_free(buf);
	buf = new_buf;
      }
    }
    formated = TRUE;
    break;
  case 's':
    if (i > 1 && format2[i - 1] == 'l') {
      break;
    }
    if (arg) {
      buf = g_strdup_printf(format2, arg);
    }
    formated = TRUE;
    break;
  case 'c':
    if (i > 1 && format2[i - 1] == 'l') {
      break;
    }
    if (arg) {
      buf = g_strdup_printf(format2, arg[0]);
    }
    formated = TRUE;
    break;
  }

  if (buf) {
    g_string_append(str, buf);
    g_free(buf);
  }

  g_free(format2);

  return formated;
}

#ifdef COMPILE_UNUSED_FUNCTIONS
char *
getitok3(char **s,int *len,char *ifs)
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

char *
getitok4(char **s,int *len,char *ifs)
{
  char *po,*s2;

  if ((s2=getitok3(s,len,ifs))==NULL) return NULL;
  if ((po=g_malloc(*len+1))==NULL) {
    *len=-1;
    return NULL;
  }
  strncpy(po,s2,*len);
  po[*len]='\0';
  return po;
}
#endif /* COMPILE_UNUSED_FUNCTIONS */

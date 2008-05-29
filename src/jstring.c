/* 
 * $Id: jstring.c,v 1.1.1.1 2008/05/29 09:37:33 hito Exp $
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

#include <string.h>
#include "jstring.h"

#define TRUE  1
#define FALSE 0

void njms2euc(char *s)
{
  int i;
  unsigned int jis;

  for (i=0;i<strlen(s);i++) {
    if (niskanji((unsigned char)s[i])) {
      if (i+1<strlen(s)) {
        jis=njms2jis(((unsigned char)s[i] << 8)+(unsigned char)s[i+1]);
        s[i]=(jis >> 8) | 0x80;
        s[i+1]=(jis & 0xff) | 0x80; 
      } else s[i]=' ';
      i++;
    }
  }
}

void neuc2jms(char *s)
{
  int i;
  unsigned int jms;

  for (i=0;i<strlen(s);i++) {
    if ((s[i] & 0x80) && (s[i+1] & 0x80)) {
      jms=njis2jms((((unsigned char)s[i] << 8)
                    +(unsigned char)s[i+1]) & 0x7f7f);
      s[i]=jms >> 8;
      s[i+1]=jms & 0xff;
      i++;
    }
  }
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

unsigned int njis2jms(unsigned int code)
{
  unsigned char dh,dl;

  dh=code >> 8;
  dl=code & 0xff;
  if (dh & 0x1) dl+=0x1f;
  else dl+=0x7d;
  if (dl>=0x7f) dl++;
  if (dh>=0x5f) {
    dh-=0x5f;
    dh=dh>>1;
    dh+=0xe0;
  } else {
    dh-=0x21;
    dh=dh>>1;
    dh+=0x81;
  }
  return ((unsigned int)(dh << 8))+dl;
}

int niskanji(unsigned char code)
{
  if (((0x81<=code) && (code<=0x9f))
   || ((0xe0<=code) && (code<=0xff))) return TRUE;
  return FALSE;
}

int niskanji2(char *s,int pos)
{
  int k;

  if (pos>=strlen(s)) return FALSE;
  else {
    k=pos;
    do {
      k--;
    } while ((k>=0) && niskanji((unsigned char)s[k]));
    if ((pos-k)%2==1) return FALSE;
    else return TRUE;
  }
}


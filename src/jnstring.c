/* 
 * $Id: jnstring.c,v 1.8 2010-03-04 08:30:16 hito Exp $
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

#include <string.h>
#include <glib.h>
#include "jnstring.h"

unsigned int 
njms2jis(unsigned int code)
{
  unsigned char dh,dl;

  dh=code >> 8;
  dl=code & 0xffU;
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

#ifdef COMPILE_UNUSED_FUNCTIONS
static unsigned int 
njis2jms(unsigned int code)
{
  unsigned char dh,dl;

  dh=code >> 8;
  dl=code & 0xffU;
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

static void 
njms2euc(char *s)
{
  unsigned int i, n;
  unsigned int jis;

  n = strlen(s);
  for (i=0;i<n;i++) {
    if (niskanji((unsigned char)s[i])) {
      if (i+1<n) {
        jis=njms2jis(((unsigned char)s[i] << 8)+(unsigned char)s[i+1]);
        s[i]=(jis >> 8) | 0x80;
        s[i+1]=(jis & 0xffU) | 0x80; 
      } else s[i]=' ';
      i++;
    }
  }
}

static void 
neuc2jms(char *s)
{
  unsigned int i, n;
  unsigned int jms;

  n = strlen(s);
  for (i=0;i<n;i++) {
    if ((s[i] & 0x80) && (s[i+1] & 0x80)) {
      jms=njis2jms((((unsigned char)s[i] << 8)
                    +(unsigned char)s[i+1]) & 0x7f7f);
      s[i]=jms >> 8;
      s[i+1]=jms & 0xffU;
      i++;
    }
  }
}
#endif /* COMPILE_UNUSED_FUNCTIONS */

int 
niskanji(unsigned char code)
{
  if ((0x81 <= code && code <= 0x9f) ||
      (0xe0 <= code && code <= 0xffU)) return TRUE;
  return FALSE;
}

int 
niskanji2(char *s,int pos)
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


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

int 
niskanji(unsigned char code)
{
  if ((0x81 <= code && code <= 0x9f) ||
      (0xe0 <= code)) return TRUE;
  return FALSE;
}

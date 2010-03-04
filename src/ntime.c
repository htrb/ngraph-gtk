/* 
 * $Id: ntime.c,v 1.5 2010/03/04 08:30:16 hito Exp $
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

#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include "object.h"
#include "nstring.h"
#include "ntime.h"

char *weekstr[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
char *monthstr[12]={"Jan","Feb","Mar","Apr","May","Jun",
                    "Jul","Aug","Sep","Oct","Nov","Dec"};

char *
ndate(time_t *timep,int style)
{
  struct tm *ltime;
  char c[32];

  ltime=localtime(timep);
  switch (style) {
  case 1:
    snprintf(c, sizeof(c),
	     "%d-%d-%d",
	     ltime->tm_mon + 1, ltime->tm_mday, 1900 + ltime->tm_year);
    break;
  case 2:
    snprintf(c, sizeof(c),
	     "%s %d %d",
	     monthstr[ltime->tm_mon], ltime->tm_mday, 1900 + ltime->tm_year);
    break;
  case 3:
    snprintf(c, sizeof(c),
	     "%d-%d-%d",
	     ltime->tm_mday, ltime->tm_mon + 1, 1900 + ltime->tm_year);
    break;
  case 4:
    snprintf(c, sizeof(c),
	     "%d/%d/%d", ltime->tm_mon + 1, ltime->tm_mday, 1900 + ltime->tm_year);
    break;
  default:
    snprintf(c, sizeof(c),
	     "%s %s %d %d",
	     weekstr[ltime->tm_wday], monthstr[ltime->tm_mon], ltime->tm_mday, 1900 + ltime->tm_year);
    break;
  }

  return g_strdup(c);
}

char *
ntime(time_t *timep,int style)
{
  struct tm *ltime;
  char c[32];

  ltime=localtime(timep);
  switch (style) {
  case 1:
    if (ltime->tm_hour<12)
      snprintf(c, sizeof(c),
	       "%02d:%02d:%02d am",
	       ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
    else
      snprintf(c, sizeof(c),
	       "%02d:%02d:%02d pm",
	       ltime->tm_hour - 12, ltime->tm_min, ltime->tm_sec);
    break;
  case 2:
    snprintf(c, sizeof(c),
	     "%02d:%02d",
	     ltime->tm_hour, ltime->tm_min);
    break;
  case 3:
    if (ltime->tm_hour<12) {
      snprintf(c, sizeof(c),
	       "%02d:%02d am",
	       ltime->tm_hour, ltime->tm_min);
    } else {
      snprintf(c, sizeof(c),
	       "%02d:%02d pm",
	       ltime->tm_hour - 12, ltime->tm_min);
    }
    break;
  default:
    snprintf(c, sizeof(c),
	     "%02d:%02d:%02d",
	     ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
    break;
  }

  return g_strdup(c);
}

int 
gettimeval(char *s,time_t *time)
{
  char *endptr;
  struct tm tm;
  int year;

  tm.tm_mday=strtol(s,&endptr,10);
  if (endptr[0]!='-') return -1;
  s=endptr+1;
  tm.tm_mon=strtol(s,&endptr,10)-1;
  if (endptr[0]!='-') return -1;
  s=endptr+1;
  year=strtol(s,&endptr,10)-1900;
  if (year<0) year+=1900;
  tm.tm_year=year;
  if (endptr[0]!=' ') return -1;
  s=endptr+1;
  tm.tm_hour=strtol(s,&endptr,10);
  if (endptr[0]!=':') return -1;
  s=endptr+1;
  tm.tm_min=strtol(s,&endptr,10);
  if (endptr[0]!=':') return -1;
  s=endptr+1;
  tm.tm_sec=strtol(s,&endptr,10);
  tm.tm_isdst=0;
  *time=mktime(&tm);
  return 0;
}

int 
gettimeval2(char *s,time_t *time)
{
  char *endptr;
  struct tm tm;

  tm.tm_year=strtol(s,&endptr,10)-1900;
  if (endptr[0]!='-') return -1;
  s=endptr+1;
  tm.tm_mon=strtol(s,&endptr,10)-1;
  if (endptr[0]!='-') return -1;
  s=endptr+1;
  tm.tm_mday=strtol(s,&endptr,10);
  if (endptr[0]!=' ') return -1;
  s=endptr+1;
  tm.tm_hour=strtol(s,&endptr,10);
  if (endptr[0]!=':') return -1;
  s=endptr+1;
  tm.tm_min=strtol(s,&endptr,10);
  if (endptr[0]!=':') return -1;
  s=endptr+1;
  tm.tm_sec=strtol(s,&endptr,10);
  tm.tm_isdst=0;
  *time=mktime(&tm);
  return 0;
}


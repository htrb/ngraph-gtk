/*
 * $Id: ntime.c,v 1.5 2010-03-04 08:30:16 hito Exp $
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
#include <math.h>
#include <glib.h>

#include "object.h"
#include "nstring.h"
#include "ntime.h"
#include "mathfn.h"

char *weekstr[7]={"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};
char *monthstr[12]={"Jan","Feb","Mar","Apr","May","Jun",
                    "Jul","Aug","Sep","Oct","Nov","Dec"};

char *
ndate(const time_t *timep, int style)
{
  struct tm *ltime;
  char *c;

  ltime=localtime(timep);
  if (ltime == NULL) {
    return NULL;
  }
  switch (style) {
  case 1:
    c = g_strdup_printf("%d-%d-%d",
			ltime->tm_mon + 1, ltime->tm_mday, 1900 + ltime->tm_year);
    break;
  case 2:
    c = g_strdup_printf("%s %d %d",
			monthstr[ltime->tm_mon], ltime->tm_mday, 1900 + ltime->tm_year);
    break;
  case 3:
    c = g_strdup_printf("%d-%d-%d",
			ltime->tm_mday, ltime->tm_mon + 1, 1900 + ltime->tm_year);
    break;
  case 4:
    c = g_strdup_printf("%d/%d/%d",
			ltime->tm_mon + 1, ltime->tm_mday, 1900 + ltime->tm_year);
    break;
  default:
    c = g_strdup_printf("%s %s %d %d",
			weekstr[ltime->tm_wday], monthstr[ltime->tm_mon], ltime->tm_mday, 1900 + ltime->tm_year);
    break;
  }

  return c;
}

char *
ntime(const time_t *timep, int style)
{
  struct tm *ltime;
  char *c;

  ltime=localtime(timep);
  if (ltime == NULL) {
    return NULL;
  }
  switch (style) {
  case 1:
    if (ltime->tm_hour<12)
      c = g_strdup_printf("%02d:%02d:%02d am",
			  ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
    else
      c = g_strdup_printf("%02d:%02d:%02d pm",
			  ltime->tm_hour - 12, ltime->tm_min, ltime->tm_sec);
    break;
  case 2:
    c = g_strdup_printf("%02d:%02d",
			ltime->tm_hour, ltime->tm_min);
    break;
  case 3:
    if (ltime->tm_hour<12) {
      c = g_strdup_printf("%02d:%02d am",
			  ltime->tm_hour, ltime->tm_min);
    } else {
      c = g_strdup_printf("%02d:%02d pm",
			  ltime->tm_hour - 12, ltime->tm_min);
    }
    break;
  default:
    c = g_strdup_printf("%02d:%02d:%02d",
			ltime->tm_hour, ltime->tm_min, ltime->tm_sec);
    break;
  }

  return c;
}

int
gettimeval(const char *s, time_t *time)
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

void
mjd2gd(double mjd, struct tm *tm)
{
  double t;
  int wd, sec;
  /* Date */

#if 1
  /* http://en.wikipedia.org/wiki/Julian_day */
  int j, g, dg, c, dc, b, db, a, da, y, m, d, Y, M, D;

  j = floor(mjd + 2400001 + 32044);
  g = j / 146097;
  dg = j % 146097;
  c = (dg / 36524 + 1) * 3 / 4;
  dc = dg - c * 36524;
  b = dc / 1461;
  db = dc % 1461;
  a = (db / 365 + 1) * 3 / 4;
  da = db - a * 365;
  y = g * 400 + c * 100 + b * 4 + a;
  m = (da * 5 + 308) / 153 - 2;
  d = da - (m + 4) * 153 / 5 + 122;
  Y = y - 4800 + (m + 2) / 12;
  M = (m + 2) % 12 + 1;
  D = d + 1;
#else
  /* http://www.astro.uu.nl/~strous/AA/en/reken/juliaansedag.html */
  double x0, x1, x2, c1, c2;
  int Y, M, D, da;

  x2 = floor(mjd) + (2400000.5 - 1721119.5);
  c2 = floor((4 * x2 + 3) / 146097.0);
  x1 = x2 - floor(146097 * c2 / 4.0);
  c1 = floor((100 * x1 + 99) / 36525.0);
  x0 = x1 - floor(36525 * c1 / 100.0);
  Y = 100 * c2 + c1;
  M = floor((5 * x0 + 461) / 153.0);
  D = x0 - floor((153 * M - 457) / 5.0) + 1;
  if (M > 12) {
    M -= 12;
    Y += 1;
  }
  da = floor(x0);
#endif

  wd = floor(mjd) + 2400000 + 2;
  wd %= 7;

  tm->tm_year = Y - 1900;
  tm->tm_mon  = M - 1;
  tm->tm_mday = D;
  tm->tm_wday = wd;

  if (Y % 400 == 0) {
    da += 60;
    da %= 366;
  } else if (Y % 100 == 0) {
    da += 59;
    da %= 365;
  } else if (Y % 4 == 0) {
    da += 60;
    da %= 366;
  } else {
    da += 59;
    da %= 365;
  }
  tm->tm_yday = da;

  /* Time */
  t = fmod(mjd, 1);
  t = (t < 0) ? 1 + t : t;
  sec = nround(t * 86400);

  tm->tm_hour = sec / 3600;
  sec %= 3600;

  tm->tm_min = sec / 60;
  sec %= 60;

  tm->tm_sec = sec;
  tm->tm_isdst = -1;
}

static int
get_iso_8601_week(const struct tm *t)
{
  int y, w, a, b, c, x;

  w = t->tm_wday;
  a = t->tm_yday % 7;
  b = (w > a) ? w - a : 7 + w - a;
  c = (b == 0) ? 6 : b -1;
  x = (t->tm_yday + c) / 7;
  y = (b + 3) % 7;
  if (y > 4 || y == 0) {
    x += 1;
  }
  if ((t->tm_yday == 0 && (w == 5 || w == 6 || w == 0)) ||
      (t->tm_yday == 1 && (w == 6 || w == 0)) ||
      (t->tm_yday == 2 && w == 0)) {
    x = 53;
  } else if (t->tm_mon == 11) {
    if ((t->tm_mday == 31 && (w == 1 || w == 2 || w == 3)) ||
	(t->tm_mday == 30 && (w == 1 || w == 2)) ||
	(t->tm_mday == 29 && w == 1)) {
      x = 1;
    }
  }
  return x;
}

static int
append_date_str(GString *str, const gchar *fmt, struct tm *t)
{
  const char *wfname[]={"Sunday",   "Monday", "Tuesday", "Wednesday",
			"Thursday", "Friday", "Saturday"};
  const char *mfname[]={"January",   "February", "March",    "April",
			"May",       "June",     "July",     "August",
			"September", "October",  "November", "December"};
  int y, m, d, w, h24, h12, mm, s, a, b, c, x;

  if (fmt == NULL || *fmt == '\0') {
    return 0;
  }

  if (t == NULL ||
      t->tm_mon < 0 || t->tm_mon > 11 ||
      t->tm_wday < 0 || t->tm_wday > 6) {
    return 1;
  }

  y = t->tm_year + 1900;
  m = t->tm_mon + 1;
  d = t->tm_mday;
  w = t->tm_wday;

  h24 = t->tm_hour;
  if (h24 == 0) {
    h12 = 12;
  } else if (h24 > 12) {
    h12 = h24 - 12;
  } else {
    h12 = h24;
  }

  mm = t->tm_min;
  s = t->tm_sec;

  switch (*fmt) {
  case 'a':
    g_string_append_printf(str, "%s", weekstr[w]);
    break;
  case 'A':
    g_string_append_printf(str, "%s", wfname[w]);
    break;
  case 'b':
  case 'h':
    g_string_append_printf(str, "%s", monthstr[m - 1]);
    break;
  case 'B':
    g_string_append_printf(str, "%s", mfname[m - 1]);
    break;
  case 'c':
    g_string_append_printf(str, "%s %s % 2d %02d:%02d:%02d %d",
			   weekstr[w], monthstr[m - 1], d,
			   h24, mm, s, y);
    break;
  case 'C':
    g_string_append_printf(str, "%d", y / 100);
    break;
  case 'd':
    g_string_append_printf(str, "%02d", d);
    break;
  case 'D':
  case 'x':
    g_string_append_printf(str, "%02d/%02d/%02d", m, d, y % 100);
    break;
  case 'e':
    g_string_append_printf(str, "% 2d", d);
    break;
  case 'F':
    g_string_append_printf(str, "% 4d-%02d-%02d", y, m, d);
    break;
  case 'G':
    x = get_iso_8601_week(t);
    if (m == 1 && x == 53) {
      y -= 1;
    } else if (m == 12 && x == 1) {
      y += 1;
    }
    g_string_append_printf(str, "% 4d", y);
    break;
  case 'g':
    x = get_iso_8601_week(t);
    if (m == 1 && x == 53) {
      y -= 1;
    } else if (m == 12 && x == 1) {
      y += 1;
    }
    g_string_append_printf(str, "%02d", y % 100);
    break;
  case 'H':
    g_string_append_printf(str, "%02d", h24);
    break;
  case 'I':
    g_string_append_printf(str, "%02d", h12);
    break;
  case 'j':
    g_string_append_printf(str, "%03d", t->tm_yday + 1);
    break;
  case 'k':
    g_string_append_printf(str, "% 2d", h24);
    break;
  case 'l':
    g_string_append_printf(str, "% 2d", h12);
    break;
  case 'm':
    g_string_append_printf(str, "%02d", m);
    break;
  case 'M':
    g_string_append_printf(str, "%02d", mm);
    break;
  case 'n':
    g_string_append_c(str, '\n');
    break;
  case 'p':
    g_string_append(str, (h24 > 11) ? "PM" : "AM");
    break;
  case 'P':
    g_string_append(str, (h24 > 11) ? "pm" : "am");
    break;
  case 'r':
    g_string_append_printf(str, "%02d:%02d:%02d %s",
			   h12, mm, s,
			   (h24 > 11) ? "PM" : "AM");
    break;
  case 'R':
    g_string_append_printf(str, "%02d:%02d", h24, mm);
    break;
  case 's':
    break;
  case 'S':
    g_string_append_printf(str, "%02d", s);
    break;
  case 't':
    g_string_append_c(str, '\t');
    break;
  case 'T':
  case 'X':
    g_string_append_printf(str, "%02d:%02d:%02d", h24, mm, s);
    break;
  case 'u':
    g_string_append_printf(str, "%d", (w == 0) ? 7 : w);
    break;
  case 'U':
    a = t->tm_yday % 7;
    b = (w > a) ? w - a : 7 + w - a;
    c = (b == 0) ? 7 : b;
    g_string_append_printf(str, "%02d", (t->tm_yday + c)/ 7);
    break;
  case 'V':
    x = get_iso_8601_week(t);
    g_string_append_printf(str, "%02d", x);
    break;
  case 'w':
    g_string_append_printf(str, "%d", w);
    break;
  case 'W':
    a = t->tm_yday % 7;
    b = (w > a) ? w - a : 7 + w - a;
    c = (b == 0) ? 6 : b -1;
    g_string_append_printf(str, "%02d", (t->tm_yday + c) / 7);
    break;
  case 'y':
    g_string_append_printf(str, "%02d", y % 100);
    break;
  case 'Y':
    g_string_append_printf(str, "%d", y);
    break;
  case 'z':
    g_string_append(str, "+0000");
    break;
  case 'Z':
    g_string_append(str, "GMT");
    break;
  case '+':
    g_string_append_printf(str, "%s %s % 2d %02d:%02d:%02d GMT %d",
			   weekstr[w], monthstr[m - 1], d,
			   h24, mm, s, y);
    break;
  case '%':
  default:
    g_string_append_c(str, *fmt);
  }

  return 1;
}

char *
nstrftime(const gchar *fmt, double mjd)
{
  GString *str;
  struct tm t;
  int n;

  if (fmt == NULL || fmt[0] == '\0') {
    return NULL;
  }

  mjd2gd(mjd, &t);
  str = g_string_sized_new(64);

  while (*fmt) {
    switch (*fmt) {
    case '%':
      fmt++;
      while (*fmt == 'O' || *fmt == 'E') {
	fmt++;
      }
      n = append_date_str(str, fmt, &t);
      fmt += n;
      break;
    default:
      g_string_append_c(str, *fmt);
      fmt++;
    }
  }

  return g_string_free(str, FALSE);
}

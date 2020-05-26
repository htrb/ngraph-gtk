/*
 * $Id: nstring.h,v 1.4 2009-11-16 09:13:04 hito Exp $
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

#ifndef NSTRING_HEADER
#define NSTRING_HEADER

#include "math/math_equation.h"

#define STRLEN 256
#define CHK_STR(s) (((s) == NULL) ? "" : (s))

char *nstrnew(void);
char *nstrccat(char *po,char ch);
char *nstrcat(char *po,char *s);
char *nstraddchar(char *po, int len, char ch);

int strcmp0(const char *s1,const char *s2);
int strcmp2(char *s1,char *s2);
#define WILD_PATHNAME 2
#define WILD_PERIOD 4
int wildmatch(const char *pat, const char *s,int flags);
char *getitok(char **s, int *len, const char *ifs);
char *getitok2(char **s, int *len, const char *ifs);
int add_printf_formated_str(GString *str, const char *format, const char *arg, int *len);
int add_printf_formated_double(GString *str, const char *format, MathValue *mval, int *len);
char *n_locale_to_utf8(const char *s);

#endif

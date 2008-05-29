/* 
 * $Id: nstring.h,v 1.1.1.1 2008/05/29 09:37:33 hito Exp $
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

#define STRLEN 256

char *nstrnew(void);
char *nstrdup(const char *src);
char *nstrccat(char *po,char ch);
char *nstrcat(char *po,char *s);
char *nstrncat(char *po,char *s,size_t n);
char *nstraddchar(char *po, int len, char ch);

int strcmp0(const char *s1,const char *s2);
int strcmp2(char *s1,char *s2);
#define WILD_PATHNAME 2
#define WILD_PERIOD 4
int wildmatch(char *pat,char *s,int flags);
char *getitok(char **s,int *len,char *ifs);
char *getitok2(char **s,int *len,char *ifs);
char *getitok3(char **s,int *len,char *ifs);
char *getitok4(char **s,int *len,char *ifs);


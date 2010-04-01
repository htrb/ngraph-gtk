/* 
 * $Id: mathcode.h,v 1.5 2009-08-13 08:52:00 hito Exp $
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

extern char *matherrorchar[];

#define MEMORYNUM 20

enum MATH_CODE_ERROR_NO {
  MCNOERR = 0,
  MCSYNTAX,
  MCILLEGAL,
  MCNEST,
};

#define MATH_CODE_ERROR_NUM (MCNEST + 1)

int mathcode_error(struct objlist *obj, enum MATH_CODE_ERROR_NO rcode, int *ecode);

enum MATH_CODE_ERROR_NO mathcode(const char *str,char **code,
				 struct narray *needdata,struct narray *needfile,
				 int *maxdim,int *twopass,
				 int datax,int datay,int dataz,
				 int column,int multi,int minmax,int memory,int userfn,
				 int color,int marksize,int file);

#define MNOERR 0
#define MERR 1
#define MNAN 2
#define MUNDEF 3
#define MSERR 4
#define MSCONT 5
#define MSBREAK 6
#define MNONUM 7
#define MEOF 8

int calculate(char *code,
              int first,
              double x,char xstat,
              double y,char ystat,
              double z,char zstat,
              double minx,char minxstat,
              double maxx,char maxxstat,
              double miny,char minystat,
              double maxy,char maxystat,
              int num,
              double sumx,double sumy,double sumxx,double sumyy,double sumxy,
              double *data,char *datastat,
              double *memory,char *memorystat,
              double *sumdata,char *sumstat,
              double *difdata,char *difstat,
              int *color,int *marksize,int *marktype,
              char *ufcodef,char *ufcodeg,char *ufcodeh,
              int fnum,int *needfile,double *fdata,char *fdatastat,int file,
              double *value);

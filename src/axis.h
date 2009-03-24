/* 
 * $Id: axis.h,v 1.2 2009/03/24 09:14:52 hito Exp $
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

#define AXISNORMAL   0
#define AXISLOGSMALL 1
#define AXISLOGNORM  2
#define AXISLOGBIG   3
#define AXISINVERSE  4

struct axislocal {
    int atype;
    double min,max,inc;
    int div;
    int tighten;
    double posst,posed;
    double posl,posm,dposl,dposm,dposs;
    int counts,countm,countsend,countmend,count;
    int num;
};

int getaxispositionini(struct axislocal *alocal,
               int type,double min,double max,double inc,int div,int tighten);
int getaxisposition(struct axislocal *alocal,double *po);
double scale(double x);
double roundmin(double min,double sc);
extern char *axistypechar[];


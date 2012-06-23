/* 
 * $Id: ngraph.h,v 1.5 2010-03-04 08:30:16 hito Exp $
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

#ifndef NGRAPH_HEADER
#define NGRAPH_HEADER

#include "common.h"

#include <glib.h>

#define MARK_TYPE_NUM 90

int printfconsole(char *fmt,...);
int putconsole(const char *s);
int printfconsole(char *fmt,...);
void displaydialog(const char *str);
void displaystatus(const char *str);
void pausewindowconsole(char *title,char *str);

#endif	/* NGRAPH_HEADER */

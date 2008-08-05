/* 
 * $Id: ngraph.h,v 1.2 2008/08/05 02:45:24 hito Exp $
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

extern char *systemname;
int printfconsole(char *fmt,...);
int putconsole(char *s);
int printfconsole(char *fmt,...);
void displaydialog(char *str);
void displaystatus(char *str);
void pausewindowconsole(char *title,char *str);

#define ALIGNSIZE 8

#ifdef WINDOWS
int winputstderr(char *s);
int winprintfstderr(char *fmt,...);
void winpausewindow(char *title,char *str);
int nallocconsole(void);
void nsetconsolemode(void);
void nfreeconsole(void);
void nforegroundconsole(void);

#define PLATFORM "for Windows"
#else
#ifndef PLATFORM
#define PLATFORM "for X11"
#endif
#endif

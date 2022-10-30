/*
 * $Id: x11print.h,v 1.7 2008-09-12 09:12:08 hito Exp $
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

#define PRINT_SHOW_DIALOG_NONE    0
#define PRINT_SHOW_DIALOG_PREVIEW 1
#define PRINT_SHOW_DIALOG_DIALOG  2

#if GTK_CHECK_VERSION(4, 0, 0)
void CmOutputPrinter(int select_file, int show_dialog, response_cb cb, gpointer user_data);
#else
void CmOutputPrinter(int select_file, int show_dialog);
#endif
void CmOutputMenu(void *w, gpointer client_data);
void CmOutputViewerB(void *w, gpointer client_data);
void CmOutputPrinterB(void *wi, gpointer client_data);

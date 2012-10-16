/*
 * $Id: x11gui.h,v 1.15 2010-03-04 08:30:17 hito Exp $
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

#ifndef GTK_GUI_HEADER
#define GTK_GUI_HEADER

#define RESPONS_OK 1
#define RESPONS_YESNOCANCEL 2
#define RESPONS_YESNO 3
#define RESPONS_OKCANCEL 4
#define RESPONS_ERROR 5

#ifdef IDOK
#undef IDOK
#undef IDCANCEL
#undef IDYES
#undef IDNO
#undef IDCLOSE
#endif	/* IDOK */

#define IDLOOP   0
#define IDOK     1
#define IDCANCEL 2
#define IDYES    3
#define IDNO     4
#define IDDELETE 101
#define IDFAPPLY 102
#define IDCOPY   103
#define IDLOAD   104
#define IDSAVE   105
#define IDCLOSE  106
#define IDSALL     201
#define IDCOPYALL  202

#define DPI_MAX 2540
#define DEFAULT_DPI 70

typedef struct _tpoint {
  int x, y;
} TPoint;

int DialogExecute(GtkWidget *parent, void *dialog);
void message_beep(GtkWidget *parent);
int message_box(GtkWidget *parent, const char *message, const char *title, int yesno);
int DialogInput(GtkWidget *parent, const char *title, const char *mes, const char *init_str, char **s, int *x, int *y);
int DialogRadio(GtkWidget *parent, const char *title, const char *caption, struct narray *ary, int *r, int *x, int *y);
int DialogCheck(GtkWidget *parent, const char *title, const char *caption, struct narray *array, int *r, int *x, int *y);
int DialogCombo(GtkWidget *parent, const char *title, const char *caption, struct narray *array, int sel, char **r, int *x, int *y);
int DialogComboEntry(GtkWidget *parent, const char *title, const char *caption, struct narray *array, int sel, char **r, int *x, int *y);
int DialogSpinEntry(GtkWidget *parent, const char *title, const char *caption, double min, double max, double inc, double *r, int *x, int *y);
int nGetOpenFileNameMulti(GtkWidget * parent,
			  char *title, char *defext, char **initdir,
			  const char *initfil, char ***file, int chd);
int nGetOpenFileName(GtkWidget * parent, char *title, char *defext,
		     char **initdir, const char *initfil, char **file,
		     int exist, int chd);
int nGetSaveFileName(GtkWidget * parent, char *title, char *defext,
		     char **initdir, const char *initfil, char **file,
		     int overwrite, int chdir);
void get_window_geometry(GtkWidget *win, gint *x, gint *y, gint *w, gint *h);
void set_sensitivity_by_check_instance(GtkWidget *widget, gpointer user_data);
int ndialog_run(GtkWidget *dlg);
#endif

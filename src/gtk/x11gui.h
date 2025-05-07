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

#include "x11dialg.h"

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
#define IDSAVE   105
#define IDSALL     201
#define IDEVMASK   301
#define IDEVMOVE   302

#define DPI_MAX 2540
#define DEFAULT_DPI 70

typedef struct _tpoint {
  int x, y;
} TPoint;

void dialog_response(gint res_id, struct DialogType *data);
GtkWidget *dialog_add_all_button (struct DialogType *data);
GtkWidget *dialog_add_delete_button (struct DialogType *data);
GtkWidget *dialog_add_apply_all_button (struct DialogType *data);
GtkWidget *dialog_add_save_button (struct DialogType *data);
void dialog_add_move_button (struct DialogType *data);
void dialog_wait(volatile const int *wait);
void DialogExecute(GtkWidget *parent, void *dialog);
void markup_message_box(GtkWidget * parent, const char *message, const char *title, int mode, int markup);
void message_box(GtkWidget *parent, const char *message, const char *title, int yesno);
void response_message_box(GtkWidget *parent, const char *message, const char *title, int mode, response_cb cb, gpointer user_data);
void input_dialog(GtkWidget *parent, const char *title, const char *mes, const char *init_str, const char *button, const struct narray *buttons, int *res_btn, string_response_cb cb, gpointer user_data);
void spin_dialog(GtkWidget *parent, const char *title, const char *caption, double min, double max, double inc, const struct narray *buttons, int *res_btn, double *r, response_cb cb, gpointer user_data);
void markup_message_box_full(GtkWidget *parent, const char *message, const char *title, int mode, int markup, response_cb cb, gpointer user_data);
void radio_dialog(GtkWidget *parent, const char *title, const char *caption, struct narray *array, const char *button, const struct narray *buttons, int *res_btn, int selected, response_cb cb, gpointer user_data);
void check_dialog(GtkWidget *parent, const char *title, const char *caption, struct narray *array, const struct narray *buttons, int *res_btn, int *r, response_cb cb, gpointer user_data);
void button_dialog(GtkWidget *parent, const char *title, const char *caption, const struct narray *buttons, response_cb cb, gpointer user_data);
void combo_dialog(GtkWidget *parent, const char *title, const char *caption, struct narray *array, const struct narray *buttons, int *res_btn, int sel, string_response_cb cb, gpointer user_data);
void combo_entry_dialog(GtkWidget *parent, const char *title, const char *caption, struct narray *array, const struct narray *buttons, int *res_btn, int sel, string_response_cb cb, gpointer user_data);
void message_beep(GtkWidget *parent);
void nGetOpenFileNameMulti(GtkWidget * parent,
                           char *title, char *defext, char **initdir,
                           const char *initfil, int chd,
                           files_response_cb cb, gpointer user_data);
void nGetOpenFileName(GtkWidget * parent, char *title, char *defext,
                      char **initdir, const char *initfil, int chd,
                      file_response_cb cb, gpointer user_data);
void nGetSaveFileName(GtkWidget * parent, char *title, char *defext,
                      char **initdir, const char *initfil, int chdir,
                      file_response_cb cb, gpointer user_data);
void get_window_geometry(GtkWidget *win, gint *x, gint *y, gint *w, gint *h);
void set_sensitivity_by_check_instance(GtkWidget *widget, gpointer user_data);
#endif

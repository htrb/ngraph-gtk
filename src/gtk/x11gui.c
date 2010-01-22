/* 
 * $Id: x11gui.c,v 1.39 2010/01/22 02:02:24 hito Exp $
 * 
 * This file is part of "Ngraph for X11".
 * 
 * Copyright (C) 2002,  Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 * 
 * "Ngraph for X11" is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License,  or (at your option) any later version.
 * 
 * "Ngraph for X11" is distributed in the hope that it will be useful, 
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not,  write to the Free Software
 * Foundation,  Inc.,  59 Temple Place - Suite 330,  Boston,  MA  02111-1307,  USA.
 * 
 */

#include "gtk_common.h"

#include <sys/stat.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <libgen.h>
#include <math.h>

#include "object.h"
#include "nstring.h"

#include "gtk_widget.h"
#include "gtk_combo.h"

#include "ox11menu.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "x11commn.h"

struct nGetOpenFileData
{
  GtkWidget *parent, *widget, *chdir_cb;
  int ret;
  char *title;
  char **initdir;
  const char *initfil;
  int chdir;
  char *ext;
  char **file;
  int mustexist;
  int overwrite;
  int multi;
  int changedir;
};

static struct nGetOpenFileData FileSelection = {NULL, NULL};

void
set_sensitivity_by_check_instance(GtkWidget *widget, gpointer user_data)
{
  char *name;
  struct objlist *obj;
  int n;

  name = (char *) user_data;

  obj = chkobject(name);
  n = chkobjlastinst(obj);

  gtk_widget_set_sensitive(widget, n > 0);
}

static void
dialog_destroyed_cb(GtkWidget *w, gpointer user_data)
{
  ((struct DialogType *) user_data)->widget = NULL;
}

static gboolean
dialog_key_down_cb(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  GdkEventKey *e;

  g_return_val_if_fail(w != NULL, FALSE);
  g_return_val_if_fail(event != NULL, FALSE);

  e = (GdkEventKey *)event;


  switch (e->keyval) {
  case GDK_w:
    if (e->state & GDK_CONTROL_MASK) {
      gtk_dialog_response(GTK_DIALOG(w), GTK_RESPONSE_CANCEL);
      return TRUE;
    }
    return FALSE;
  }
  return FALSE;
}


static gboolean 
dialog_delete_cb(GtkWidget *w, GdkEvent *e, gpointer user_data)
{
  return TRUE;
}

int
ndialog_run(GtkWidget *dlg)
{
  int lock_state, r;

  if (dlg == NULL) {
    return GTK_RESPONSE_CANCEL;
  }

  lock_state = DnDLock;
  r = gtk_dialog_run(GTK_DIALOG(dlg));
  DnDLock = lock_state;

  return r;
}

int
DialogExecute(GtkWidget *parent, void *dialog)
{
  GtkWidget *dlg, *win_ptr;
  struct DialogType *data;
  gint res_id, lockstate;

  lockstate = DnDLock;
  DnDLock = TRUE;

  data = (struct DialogType *) dialog;

  if (data->widget && (data->parent != parent)) {
#if 1
    gtk_window_set_transient_for(GTK_WINDOW(data->widget), GTK_WINDOW(parent));
    data->parent = parent;
#else
    gtk_widget_destroy(data->widget);
    ResetEvent();
    data->widget = NULL;
#endif
  }

  if (data->widget == NULL) {
    dlg = gtk_dialog_new();

    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(parent));
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_resizable(GTK_WINDOW(dlg), TRUE);
    gtk_window_set_destroy_with_parent(GTK_WINDOW(dlg), TRUE);

    g_signal_connect(dlg, "delete-event", G_CALLBACK(dialog_delete_cb), data);
    g_signal_connect(dlg, "destroy", G_CALLBACK(dialog_destroyed_cb), data);
    g_signal_connect(dlg, "key-press-event", G_CALLBACK(dialog_key_down_cb), NULL);

    data->parent = parent;
    data->widget = dlg;
    data->vbox = GTK_VBOX((GTK_DIALOG(dlg)->vbox));
    data->show_buttons = TRUE;
    data->show_cancel = TRUE;
    data->ok_button = GTK_STOCK_OK;

    gtk_window_set_title(GTK_WINDOW(dlg), _(data->resource));

    data->SetupWindow(dlg, data, TRUE);

    if (data->show_cancel) {
      gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);
    }

    gtk_dialog_add_button(GTK_DIALOG(dlg), data->ok_button, GTK_RESPONSE_OK);

    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  } else {
    dlg = data->widget;
    data->SetupWindow(dlg, data, FALSE);
  }

  gtk_widget_hide_all(dlg);
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  data->widget = dlg;
  data->ret = IDLOOP;

  gtk_widget_show_all(dlg);
  win_ptr = get_current_window();
  set_current_window(dlg);
  if (data->focus)
    gtk_widget_grab_focus(data->focus);

  if (! data->show_buttons) {
    gtk_widget_hide(GTK_DIALOG(dlg)->action_area);
    gtk_dialog_set_has_separator(GTK_DIALOG(dlg), FALSE);
  }

  while (data->ret == IDLOOP) {
    res_id = ndialog_run(dlg);

    if (res_id < 0) {
      switch (res_id) {
      case GTK_RESPONSE_OK:
	data->ret = IDOK;
	break;
      default:
	data->ret = IDCANCEL;
	break;
      }
    } else {
      data->ret = res_id;
    }

    if (data->CloseWindow) {
      data->CloseWindow(dlg, data);
    }
  }

  //  gtk_widget_destroy(dlg);
  //  data->widget = NULL;
  set_current_window(win_ptr);
  gtk_widget_hide_all(dlg);
  ResetEvent();

  DnDLock = lockstate;

  return data->ret;
}

void
MessageBeep(GtkWidget * parent)
{
  gdk_beep();
  ResetEvent();
}

static void
set_dialog_position(GtkWidget *w, const int *x, const int *y)
{
  if (x == NULL || y == NULL || *x < 0 || *y < 0)
    return;

  gtk_window_move(GTK_WINDOW(w), *x, *y);
}

static void
get_dialog_position(GtkWidget *w, int *x, int *y)
{
  if (x == NULL || y == NULL)
    return;

  gtk_window_get_position(GTK_WINDOW(w), x, y);

  if (*x < 0)
    *x = 0;

  if (*y < 0)
    *y = 0;
}

int
MessageBox(GtkWidget * parent, char *message, char *title, int mode)
{
  GtkWidget *dlg;
  int data;
  GtkMessageType dlg_type;
  GtkButtonsType dlg_button;
  gint res_id;
 
  if (title == NULL)
    title = "Error";

  switch (mode) {
  case MB_YESNOCANCEL:
  case MB_YESNO:
    dlg_button = GTK_BUTTONS_YES_NO;
    dlg_type = GTK_MESSAGE_QUESTION;
    break;
  case MB_ERROR:
    dlg_button = GTK_BUTTONS_OK;
    dlg_type = GTK_MESSAGE_ERROR;
    break;
  default:
    dlg_button = GTK_BUTTONS_OK;
    dlg_type = GTK_MESSAGE_INFO;
  }

  if (parent == NULL)
    parent = get_current_window();

  dlg = gtk_message_dialog_new(GTK_WINDOW(parent),
			       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			       dlg_type,
			       dlg_button,
			       "%s", message);

  switch (mode) {
  case MB_YESNOCANCEL:
  case MB_YESNO:
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_YES);
    break;
  }

  gtk_window_set_title(GTK_WINDOW(dlg), title);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);

  if (mode == MB_YESNOCANCEL) {
    gtk_dialog_add_button(GTK_DIALOG(dlg),
			  GTK_STOCK_CANCEL,
			  GTK_RESPONSE_CANCEL);
  }

  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);

  switch (res_id) {
  case GTK_RESPONSE_OK:
    data = IDYES;
    break;
  case GTK_RESPONSE_YES:
    data = IDYES;
    break;
  case GTK_RESPONSE_NO:
    data = IDNO;
    break;
  case GTK_RESPONSE_CANCEL:
    data = IDCANCEL;
    break;
  default:
    if ((mode == MB_OK) || (mode == MB_ERROR)) {
      data = IDOK;
    } else if (mode == MB_YESNO) {
      data = IDNO;
    } else {
      data = IDCANCEL; 
    }
  }

  gtk_widget_destroy(dlg);
  ResetEvent();

  return data;
}

int
DialogInput(GtkWidget * parent, char *title, char *mes, char **s, int *x, int *y)
{
  GtkWidget *dlg, *text;
  GtkVBox *vbox;
  int data;
  gint res_id;

  dlg = gtk_dialog_new_with_buttons(title,
				    GTK_WINDOW(parent),
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_STOCK_OK, GTK_RESPONSE_OK,
				    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				    NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_VBOX((GTK_DIALOG(dlg)->vbox));

  if (mes) {
    GtkWidget *label;
    label = gtk_label_new(mes);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
  }
 
  text = create_text_entry(FALSE, TRUE);
  gtk_box_pack_start(GTK_BOX(vbox), text, FALSE, FALSE, 5);

  set_dialog_position(dlg, x, y);
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);

  switch (res_id) {
  case GTK_RESPONSE_OK:
    *s = g_strdup(gtk_entry_get_text(GTK_ENTRY(text)));
    data = IDOK;
    break;
  default:
    data = IDCANCEL; 
    break;
  }

  get_dialog_position(dlg, x, y);
  gtk_widget_destroy(dlg);
  ResetEvent();

  return data;
}

int
DialogRadio(GtkWidget *parent, char *title, char *caption, struct narray *array, int *r, int *x, int *y)
{
  GtkWidget *dlg, *btn, **btn_ary;
  GtkVBox *vbox;
  int data;
  gint res_id;
  char **d;
  int i, anum;

  d = arraydata(array);
  anum = arraynum(array);

  btn_ary = g_malloc(anum * sizeof(*btn_ary));
  if (btn_ary == NULL)
    return IDCANCEL;

  dlg = gtk_dialog_new_with_buttons(title,
				    GTK_WINDOW(parent),
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_STOCK_OK, GTK_RESPONSE_OK,
				    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				    NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_VBOX((GTK_DIALOG(dlg)->vbox));

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
  }


  btn = NULL;
  for (i = 0; i < anum; i++) {
    btn = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(btn), d[i]);
    gtk_box_pack_start(GTK_BOX(vbox), btn, FALSE, FALSE, 2);
    btn_ary[i] = btn;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), i == *r);
  }

  set_dialog_position(dlg, x, y);
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);

  switch (res_id) {
  case GTK_RESPONSE_OK:
    *r = -1;
    for (i = 0; i < anum; i++) {
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn_ary[i]))) {
	*r = i;
	break;
      }
    }
    data = IDOK;
    break;
  default:
    data = IDCANCEL; 
    break;
  }


  g_free(btn_ary);

  get_dialog_position(dlg, x, y);
  gtk_widget_destroy(dlg);
  ResetEvent();

  return data;
}

int
DialogCombo(GtkWidget *parent, char *title, char *caption, struct narray *array, int sel, char **r, int *x, int *y)
{
  GtkWidget *dlg, *combo;
  GtkVBox *vbox;
  int data;
  gint res_id;
  char **d;
  int i, anum;

  d = arraydata(array);
  anum = arraynum(array);

  *r = NULL;

  dlg = gtk_dialog_new_with_buttons(title,
				    GTK_WINDOW(parent),
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_STOCK_OK, GTK_RESPONSE_OK,
				    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				    NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_VBOX((GTK_DIALOG(dlg)->vbox));

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
  }

  combo = combo_box_create();
  for (i = 0; i < anum; i++) {
    combo_box_append_text(combo, d[i]);
  }

  if (sel < 0 || sel >= anum) {
    sel = 0;
  }
  combo_box_set_active(combo, sel);

  gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 2);

  set_dialog_position(dlg, x, y);
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);

  switch (res_id) {
  case GTK_RESPONSE_OK:
    i = combo_box_get_active(combo);
    if (i >= 0)
      *r = g_strdup(d[i]);
    data = IDOK;
    break;
  default:
    data = IDCANCEL; 
    break;
  }

  get_dialog_position(dlg, x, y);
  gtk_widget_destroy(dlg);
  ResetEvent();

  return data;
}

int
DialogComboEntry(GtkWidget *parent, char *title, char *caption, struct narray *array, int sel, char **r, int *x, int *y)
{
  GtkWidget *dlg, *combo;
  GtkVBox *vbox;
  int data;
  gint res_id;
  char **d;
  const char *s;
  int i, anum;

  d = arraydata(array);
  anum = arraynum(array);

  *r = NULL;

  dlg = gtk_dialog_new_with_buttons(title,
				    GTK_WINDOW(parent),
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_STOCK_OK, GTK_RESPONSE_OK,
				    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				    NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_VBOX((GTK_DIALOG(dlg)->vbox));

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
  }

  combo = combo_box_entry_create();
  for (i = 0; i < anum; i++) {
    combo_box_append_text(combo, d[i]);
  }

  if (sel >= 0 && sel < anum) {
    combo_box_set_active(combo, sel);
  }

  gtk_box_pack_start(GTK_BOX(vbox), combo, FALSE, FALSE, 2);

  set_dialog_position(dlg, x, y);
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);

  switch (res_id) {
  case GTK_RESPONSE_OK:
    s = combo_box_entry_get_text(combo);
    if (s) {
      *r = g_strdup(s);
    } else {
      *r = NULL;
    }
    data = IDOK;
    break;
  default:
    data = IDCANCEL; 
    break;
  }

  get_dialog_position(dlg, x, y);
  gtk_widget_destroy(dlg);
  ResetEvent();

  return data;
}

int
DialogSpinEntry(GtkWidget *parent, char *title, char *caption, double min, double max, double inc, double *r, int *x, int *y)
{
  GtkWidget *dlg, *spin;
  GtkVBox *vbox;
  int data, n;
  gint res_id;
  double prec;

  dlg = gtk_dialog_new_with_buttons(title,
				    GTK_WINDOW(parent),
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_STOCK_OK, GTK_RESPONSE_OK,
				    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				    NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_VBOX((GTK_DIALOG(dlg)->vbox));

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
  }

  if (inc == 0)
    inc = 1;

  spin = gtk_spin_button_new_with_range(min, max, inc);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), *r);
  gtk_box_pack_start(GTK_BOX(vbox), spin, FALSE, FALSE, 2);
  gtk_entry_set_activates_default(GTK_ENTRY(spin), TRUE);

  prec = log10(fabs(inc));
  if (prec < 0) {
    n = ceil(- prec);
  } else {
    n = 0;
  }
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), n);

  set_dialog_position(dlg, x, y);
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);

  switch (res_id) {
  case GTK_RESPONSE_OK:
    *r = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
    data = IDOK;
    break;
  default:
    data = IDCANCEL; 
    break;
  }

  get_dialog_position(dlg, x, y);
  gtk_widget_destroy(dlg);
  ResetEvent();

  return data;
}

int
DialogCheck(GtkWidget *parent, char *title, char *caption, struct narray *array, int *r, int *x, int *y)
{
  GtkWidget *dlg, *btn, **btn_ary;
  GtkVBox *vbox;
  int data;
  gint res_id;
  char **d;
  int i, anum;

  d = arraydata(array);
  anum = arraynum(array);

  btn_ary = g_malloc(anum * sizeof(*btn_ary));
  if (btn_ary == NULL)
    return IDCANCEL;

  dlg = gtk_dialog_new_with_buttons(title,
				    GTK_WINDOW(parent),
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    GTK_STOCK_OK, GTK_RESPONSE_OK,
				    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				    NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_VBOX((GTK_DIALOG(dlg)->vbox));

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 5);
  }


  btn = NULL;
  for (i = 0; i < anum; i++) {
    btn = gtk_check_button_new_with_mnemonic(d[i]);
    gtk_box_pack_start(GTK_BOX(vbox), btn, FALSE, FALSE, 2);
    btn_ary[i] = btn;
  }

  for (i = 0; i < anum; i++) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_ary[i]), r[i]);
  }

  set_dialog_position(dlg, x, y);
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);

  switch (res_id) {
  case GTK_RESPONSE_OK:
    for (i = 0; i < anum; i++) {
      r[i] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn_ary[i]));
    }
    data = IDOK;
    break;
  default:
    data = IDCANCEL; 
    break;
  }

  g_free(btn_ary);

  get_dialog_position(dlg, x, y);
  gtk_widget_destroy(dlg);
  ResetEvent();

  return data;
}
static void
free_str_list(GSList *top)
{
  int i, n;
  GSList *list;

  n = g_slist_length(top);
  for (i = 0, list = top; i < n; i++, list = list->next) {
    g_free(list->data);
  }
  g_slist_free(top);
}

static void 
fsok(GtkWidget *dlg)
{
  struct nGetOpenFileData *data;
  char *file, *file2, **farray;
  const char *filter_name;
  int i, k, len, n;
  struct stat buf;
  GSList *top, *list;
  GtkFileFilter *filter;

  data = &FileSelection;

  top = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dlg));
  filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dlg));

  if (filter) {
    filter_name = gtk_file_filter_get_name(filter);
  } else {
    filter_name = NULL;
  }

  if (filter_name == NULL || strcmp(filter_name, _("All")) == 0) {
    data->ext = NULL;
  }

  n = g_slist_length(top);
  farray = g_malloc(sizeof(*farray) * (n + 1));
  if (farray == NULL) {
    free_str_list(top);
    return;
  }
  data->file = farray;

  k = 0;
  for (list = top; list; list = list->next) {
    file = (char *) list->data;
    if ((file == NULL) || (strlen(file) < 1)) {
      gdk_beep();
      continue;
    }

    for (i = strlen(file) - 1; (i > 0) && (file[i] != '/') && (file[i] != '.'); i--);
    if ((file[i] != '.') && data->ext) {
      len = strlen(data->ext) + 1;
    } else {
      len = 0;
    }

    if (len) {
      file2 = g_strdup_printf("%s.%s", file, data->ext);
    } else {
      file2 = g_strdup(file);
    }
    if (file2) {
      if (data->mustexist) {
	if ((stat(file2, &buf) != 0) || ((buf.st_mode & S_IFMT) != S_IFREG) 
	    || (access(file2, R_OK) != 0)) {
	  gdk_beep();
	  error22(NULL, 0, "I/O error", file2);
	  g_free(file2);
	  continue;
	}
      } else {
	if ((stat(file2, &buf) == 0) && ((buf.st_mode & S_IFMT) != S_IFREG)) {
	  gdk_beep();
	  error22(NULL, 0, "I/O error", file2);
	  g_free(file2);
	  continue;
	}
      }
      farray[k] = file2;
      k++;
    }
  }

  if (k == 0)
    return;

  if (data->changedir && k > 0) {
    data->chdir = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->chdir_cb));
    if (data->chdir && data->initdir) {
      char *dir, *tmp, *tmp2;

      g_free(*(data->initdir));
      tmp = g_strdup(farray[0]);
      dir = dirname(tmp);
      tmp2 = g_strdup(dir);
      *(data->initdir) = tmp2;
      g_free(tmp);
    }
  }
  farray[k] = NULL;
  free_str_list(top);
  data->ret = IDOK;
}

static void 
file_dialog_set_current_neme(GtkWidget *dlg, const char *full_name)
{
  char *name, *ptr;

  if (dlg == NULL || full_name == NULL)
    return;

  ptr = filename_to_utf8(full_name);
  if (ptr) {
    name = basename(ptr);
    if (name) {
      gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), name);
    }
    g_free(ptr);
  }
}

static int
FileSelectionDialog(GtkWidget *parent, int type, char *stock)
{
  struct nGetOpenFileData *data;
  GtkWidget *dlg, *rc;
  GtkFileFilter *filter;

  data = &FileSelection;

  data->parent = (parent) ? parent : TopLevel;
  dlg = gtk_file_chooser_dialog_new(data->title,
				    GTK_WINDOW(data->parent),
				    type,
				    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				    stock, GTK_RESPONSE_ACCEPT,
				    NULL);
  rc = gtk_check_button_new_with_mnemonic(_("_Change current directory"));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), rc, FALSE, FALSE, 5);
  data->chdir_cb = rc;
  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dlg), data->multi);
  data->widget = dlg;

  if (data->ext) {
    char *filter_str, *filter_name, *ext_name;

    filter_str = g_strdup_printf("*.%s", data->ext);
    ext_name = g_ascii_strup(data->ext, -1);
    filter_name = g_strdup_printf(_("%s file (*.%s)"), ext_name, data->ext);
    g_free(ext_name);

    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, filter_str);
    gtk_file_filter_set_name(filter, filter_name);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);
    g_free(filter_str);
    g_free(filter_name);

    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_filter_set_name(filter, _("All"));
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);
  }

  if (data->initdir && *(data->initdir)) {
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), *(data->initdir));
  }
  gtk_widget_show_all(dlg);

  if (data->changedir && data->initdir) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->chdir_cb), data->chdir);
  } else {
    gtk_widget_hide(data->chdir_cb);
  }

  if (data->initfil) {
    if (type == GTK_FILE_CHOOSER_ACTION_SAVE) {
      file_dialog_set_current_neme(dlg, data->initfil);
    } else {
      gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), data->initfil);
    }
  }

  data->ret = IDCANCEL;

  while (1) {
    if (ndialog_run(dlg) != GTK_RESPONSE_ACCEPT)
      break;

    fsok(dlg);
    if (data->ret == IDOK && type == GTK_FILE_CHOOSER_ACTION_SAVE) {
      file_dialog_set_current_neme(dlg, FileSelection.file[0]);
      if (! data->overwrite && check_overwrite(dlg, FileSelection.file[0])) {
	data->ret = IDCANCEL;
	continue;
      }
    }
    break;
  }

  gtk_widget_destroy(dlg);
  ResetEvent();
  data->widget = NULL;

  return data->ret;
}

int
nGetOpenFileNameMulti(GtkWidget * parent,
		      char *title, char *defext, char **initdir,
		      const char *initfil, char ***file, int chd)
{
  int ret;
  
  FileSelection.title = title;
  FileSelection.initdir = initdir;
  FileSelection.initfil = initfil;
  FileSelection.file = NULL;
  FileSelection.chdir = chd;
  FileSelection.ext = defext;
  FileSelection.mustexist = TRUE;
  FileSelection.overwrite = FALSE;
  FileSelection.multi = TRUE;
  FileSelection.changedir = TRUE;
  ret = FileSelectionDialog(parent, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_OPEN);
  if (ret == IDOK) {
    *file = FileSelection.file;
    if (FileSelection.chdir && initdir && chdir(*initdir)) {
      ErrorMessage();
    }
  } else {
    *file = NULL;
  }

  return ret;
}

int
nGetOpenFileName(GtkWidget *parent,
		 char *title, char *defext, char **initdir, const char *initfil,
		 char **file, int exist, int chd)
{
  int ret;
  
  FileSelection.title = title;
  FileSelection.initdir = initdir;
  FileSelection.initfil = initfil;
  FileSelection.file = NULL;
  FileSelection.chdir = chd;
  FileSelection.ext = defext;
  FileSelection.mustexist = exist;
  FileSelection.overwrite = FALSE;
  FileSelection.multi = FALSE;
  FileSelection.changedir = TRUE;

  ret = FileSelectionDialog(parent,
			    (exist) ?
			    GTK_FILE_CHOOSER_ACTION_OPEN :
			    GTK_FILE_CHOOSER_ACTION_SAVE,
			    GTK_STOCK_OPEN);
  if (ret == IDOK) {
    *file = FileSelection.file[0];
    g_free(FileSelection.file);
    if (FileSelection.chdir && initdir && chdir(*initdir)) {
      ErrorMessage();
    }
  } else {
    *file = NULL;
  }

  return ret;
}

int
nGetSaveFileName(GtkWidget * parent,
		 char *title, char *defext, char **initdir, const char *initfil,
		 char **file, int overwrite, int chd)
{
  int ret;

  FileSelection.title = title;
  FileSelection.initdir = initdir;
  FileSelection.initfil = initfil;
  FileSelection.file = NULL;
  FileSelection.chdir = chd;
  FileSelection.ext = defext;
  FileSelection.mustexist = FALSE;
  FileSelection.overwrite = overwrite;
  FileSelection.multi = FALSE;
  FileSelection.changedir = TRUE;
  ret = FileSelectionDialog(parent, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_SAVE);
  if (ret == IDOK) {
    *file = FileSelection.file[0];
    g_free(FileSelection.file);
    if (FileSelection.chdir && initdir && chdir(*initdir)) {
      ErrorMessage();
    }
  } else {
    *file = NULL;
  }

  return ret;
}

void
get_window_geometry(GtkWidget *win, gint *x, gint *y, gint *w, gint *h, GdkWindowState *state)
{
  if (state)
    *state = gdk_window_get_state(win->window);

  gtk_window_get_size(GTK_WINDOW(win), w, h);
  gtk_window_get_position(GTK_WINDOW(win), x, y);
}

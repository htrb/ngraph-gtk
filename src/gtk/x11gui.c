/*
 * $Id: x11gui.c,v 1.41 2010-04-01 06:08:23 hito Exp $
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
#include "ioutil.h"

#include "gtk_widget.h"
#include "gtk_combo.h"

#include "ox11menu.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "x11commn.h"

struct nGetOpenFileData
{
  GtkWidget *widget, *chdir_cb;
  int ret;
  char *title;
  char **init_dir;
  const char *init_file;
  int chdir;
  char *ext;
  char **file;
  const char *button;
  int type;
  int mustexist;
  int overwrite;
  int multi;
  int changedir;
};

static int add_buttons(GtkWidget *dlg, struct narray *array);

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

#if GTK_CHECK_VERSION(4, 0, 0)
static int
dialog_run(GtkWidget *dlg, struct DialogType *data)
{
  int lock_state;
  GMainContext *context;

  if (dlg == NULL || data == NULL) {
    return GTK_RESPONSE_CANCEL;
  }

  context = g_main_context_default();
  lock_state = DnDLock;
  while (data->ret == IDLOOP) {
    g_main_context_iteration(context, TRUE);
  }
  DnDLock = lock_state;

  return data->ret;
}

static void
ndialog_response(GtkWidget *dlg, gint res_id, gpointer user_data)
{
  int *data;
  data = user_data;
  *data = res_id;
}

static gboolean
ndialog_close_request(GtkWindow* window, gpointer user_data)
{
  int *data;
  data = user_data;
  *data = GTK_RESPONSE_CANCEL;
  return TRUE;
}

int
ndialog_run(GtkWidget *dlg, int *response)
{
  int lock_state;
  GMainContext *context;

  if (dlg == NULL || response == NULL) {
    return GTK_RESPONSE_CANCEL;
  }

  g_signal_connect(dlg, "response", G_CALLBACK(ndialog_response), response);
  g_signal_connect(dlg, "close_request", G_CALLBACK(ndialog_close_request), response);

  gtk_widget_show(dlg);
  context = g_main_context_default();
  lock_state = DnDLock;
  while (*response == IDLOOP) {
    g_main_context_iteration(context, TRUE);
  }
  DnDLock = lock_state;
  gtk_widget_hide(dlg);

  return *response;
}
#else
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
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
static void
dialog_response(GtkWidget *dlg, gint res_id, gpointer user_data)
{
  struct DialogType *data;
  data = user_data;
  data->ret = res_id;
}

static gboolean
dialog_close_request(GtkWindow* window, gpointer user_data)
{
  struct DialogType *data;
  data = user_data;
  data->ret = GTK_RESPONSE_CANCEL;
  return TRUE;
}
#endif

int
DialogExecute(GtkWidget *parent, void *dialog)
{
  GtkWidget *dlg, *win_ptr;
  struct DialogType *data;
  gint res_id, lockstate;
#if OSX
  int menulock;
#endif

  lockstate = DnDLock;
  DnDLock = TRUE;

  data = (struct DialogType *) dialog;

  if (data->widget && (data->parent != parent)) {
#if 1
    gtk_window_set_transient_for(GTK_WINDOW(data->widget), GTK_WINDOW(parent));
    data->parent = parent;
#else
    gtk_widget_destroy(data->widget);
    reset_event();
    data->widget = NULL;
#endif
  }

  if (data->widget == NULL) {
    dlg = gtk_dialog_new_with_buttons(_(data->resource),
				      GTK_WINDOW(parent),
#if USE_HEADER_BAR
				      GTK_DIALOG_USE_HEADER_BAR |
#endif
				      GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				      _("_Cancel"), GTK_RESPONSE_CANCEL,
				      NULL);

    gtk_window_set_resizable(GTK_WINDOW(dlg), TRUE);

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
    g_signal_connect(dlg, "response", G_CALLBACK(dialog_response), data);
    g_signal_connect(dlg, "close_request", G_CALLBACK(dialog_close_request), data);
#else
    g_signal_connect(dlg, "delete-event", G_CALLBACK(gtk_true), data);
#endif
    g_signal_connect(dlg, "destroy", G_CALLBACK(dialog_destroyed_cb), data);

    data->parent = parent;
    data->widget = dlg;
    data->vbox = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg)));
    gtk_orientable_set_orientation(GTK_ORIENTABLE(data->vbox), GTK_ORIENTATION_VERTICAL);
    data->show_cancel = TRUE;
    data->ok_button = _("_OK");

    data->SetupWindow(dlg, data, TRUE);
    gtk_dialog_add_button(GTK_DIALOG(dlg), data->ok_button, GTK_RESPONSE_OK);

    if (! data->show_cancel) {
      GtkWidget *btn;
      btn = gtk_dialog_get_widget_for_response(GTK_DIALOG(dlg), GTK_RESPONSE_CANCEL);
      gtk_widget_hide(btn);
    }

    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  } else {
    dlg = data->widget;
    data->SetupWindow(dlg, data, FALSE);
  }

  gtk_widget_hide(dlg);
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  data->widget = dlg;
  data->ret = IDLOOP;

#if OSX
  menulock = Menulock;
  Menulock = TRUE;
#endif
  gtk_widget_show(dlg);
  win_ptr = get_current_window();
  set_current_window(dlg);
  if (data->focus)
    gtk_widget_grab_focus(data->focus);

  while (data->ret == IDLOOP) {
#if GTK_CHECK_VERSION(4, 0, 0)
    res_id = dialog_run(dlg, data);
#else
    res_id = ndialog_run(dlg);
#endif

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
#if OSX
  Menulock = menulock;
#endif

  //  gtk_widget_destroy(dlg);
  //  data->widget = NULL;
  set_current_window(win_ptr);
  gtk_widget_hide(dlg);
  reset_event();

  DnDLock = lockstate;

  return data->ret;
}

void
message_beep(GtkWidget * parent)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  if (parent) {
    GdkSurface *window;
    window = gtk_native_get_surface(GTK_NATIVE(parent));
    gdk_surface_beep(window);
  }
#else
  if (parent) {
    GdkWindow *window;
    window = gtk_widget_get_window(parent);
    gdk_window_beep(window);
  }
#endif
  else {
    GdkDisplay *disp;
    disp = gdk_display_get_default();
    if (disp) {
      gdk_display_beep(disp);
    }
  }
  //  reset_event();
}

static void
set_dialog_position(GtkWidget *w, const int *x, const int *y)
{
  if (x == NULL || y == NULL || *x < 0 || *y < 0)
    return;

#if GTK_CHECK_VERSION(4, 0, 0)
  /* a number of GtkWindow APIs that were X11-specific have been removed. */
#else
  gtk_window_move(GTK_WINDOW(w), *x, *y);
#endif
}

static void
get_dialog_position(GtkWidget *w, int *x, int *y)
{
  if (x == NULL || y == NULL)
    return;

#if GTK_CHECK_VERSION(4, 0, 0)
  /* a number of GtkWindow APIs that were X11-specific have been removed. */
#else
  gtk_window_get_position(GTK_WINDOW(w), x, y);

  if (*x < 0)
    *x = 0;

  if (*y < 0)
    *y = 0;
#endif
}

int
message_box(GtkWidget * parent, const char *message, const char *title, int mode)
{
  return markup_message_box(parent, message, title, mode, FALSE);
}

int
markup_message_box(GtkWidget * parent, const char *message, const char *title, int mode, int markup)
{
  GtkWidget *dlg;
  int data;
  GtkMessageType dlg_type;
  GtkButtonsType dlg_button;
  gint res_id;

  if (title == NULL) {
    title = _("Error");
  }

  switch (mode) {
  case RESPONS_YESNOCANCEL:
    dlg_button = GTK_BUTTONS_CANCEL;
    dlg_type = GTK_MESSAGE_QUESTION;
    break;
  case RESPONS_YESNO:
    dlg_button = GTK_BUTTONS_YES_NO;
    dlg_type = GTK_MESSAGE_QUESTION;
    break;
  case RESPONS_ERROR:
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
			       "%.512s", message);
  if (markup) {
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dlg), message);
  }

  switch (mode) {
  case RESPONS_YESNOCANCEL:
    gtk_dialog_add_button(GTK_DIALOG(dlg), _("_No"), GTK_RESPONSE_NO);
    gtk_dialog_add_button(GTK_DIALOG(dlg), _("_Yes"), GTK_RESPONSE_YES);
    /* fall through */
  case RESPONS_YESNO:
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_YES);
    break;
  }

  gtk_window_set_title(GTK_WINDOW(dlg), title);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(dlg);
  res_id = IDLOOP;
  ndialog_run(dlg, &res_id);
#else
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);
#endif

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
    if ((mode == RESPONS_OK) || (mode == RESPONS_ERROR)) {
      data = IDOK;
    } else if (mode == RESPONS_YESNO) {
      data = IDNO;
    } else {
      data = IDCANCEL;
    }
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_destroy(GTK_WINDOW(dlg));
#else
  gtk_widget_destroy(dlg);
#endif
  reset_event();

  return data;
}

int
DialogInput(GtkWidget * parent, const char *title, const char *mes, const char *init_str, struct narray *buttons, int *res_btn, char **s, int *x, int *y)
{
  GtkWidget *dlg, *text;
  GtkBox *vbox;
  int data;
  gint res_id;

  dlg = gtk_dialog_new_with_buttons(title,
				    GTK_WINDOW(parent),
#if USE_HEADER_BAR
				    GTK_DIALOG_USE_HEADER_BAR |
#endif
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    NULL, NULL);
  if (add_buttons(dlg, buttons)) {
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
			   _("_Cancel"), GTK_RESPONSE_CANCEL,
			   _("_OK"), GTK_RESPONSE_OK,
			   NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  }
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg)));
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);
#endif

  if (mes) {
    GtkWidget *label;
    label = gtk_label_new(mes);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(vbox, label);
#else
    gtk_box_pack_start(vbox, label, FALSE, FALSE, 5);
#endif
  }

  text = create_text_entry(FALSE, TRUE);
  if (init_str) {
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_editable_set_text(GTK_EDITABLE(text), init_str);
#else
    gtk_entry_set_text(GTK_ENTRY(text), init_str);
#endif
  }
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(vbox, text);
#else
  gtk_box_pack_start(vbox, text, FALSE, FALSE, 5);
#endif

  set_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(dlg);
  res_id = IDLOOP;
  ndialog_run(dlg, &res_id);
#else
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);
#endif

  if (res_id > 0 || res_id == GTK_RESPONSE_OK) {
    const char *str;
#if GTK_CHECK_VERSION(4, 0, 0)
    str = gtk_editable_get_text(GTK_EDITABLE(text));
#else
    str = gtk_entry_get_text(GTK_ENTRY(text));
#endif
    *s = g_strdup(str);
    data = IDOK;
  } else {
    data = IDCANCEL;
  }

  if (buttons && res_btn) {
    *res_btn = res_id;
  }

  get_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_destroy(GTK_WINDOW(dlg));
#else
  gtk_widget_destroy(dlg);
#endif
  reset_event();

  return data;
}

int
DialogRadio(GtkWidget *parent, const char *title, const char *caption, struct narray *array, struct narray *buttons, int *res_btn, int *r, int *x, int *y)
{
  GtkWidget *dlg, *btn, **btn_ary;
  GtkBox *vbox;
  int data;
  gint res_id;
  char **d;
  int i, anum;
#if GTK_CHECK_VERSION(4, 0, 0)
  GtkWidget *group = NULL;
#endif

  d = arraydata(array);
  anum = arraynum(array);

  btn_ary = g_malloc(anum * sizeof(*btn_ary));
  if (btn_ary == NULL)
    return IDCANCEL;

  dlg = gtk_dialog_new_with_buttons(title,
				    GTK_WINDOW(parent),
#if USE_HEADER_BAR
				    GTK_DIALOG_USE_HEADER_BAR |
#endif
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    NULL, NULL);
  if (add_buttons(dlg, buttons)) {
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
			   _("_Cancel"), GTK_RESPONSE_CANCEL,
			   _("_OK"), GTK_RESPONSE_OK,
			   NULL);
  }

  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg)));
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);
#endif

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(vbox, label);
#else
    gtk_box_pack_start(vbox, label, FALSE, FALSE, 5);
#endif
  }

  btn = NULL;
  for (i = 0; i < anum; i++) {
#if GTK_CHECK_VERSION(4, 0, 0)
    btn = gtk_check_button_new_with_mnemonic(d[i]);
    if (group) {
      gtk_check_button_set_group(GTK_CHECK_BUTTON(btn), GTK_CHECK_BUTTON(group));
    } else {
      group = btn;
    }
    gtk_box_append(vbox, btn);
    btn_ary[i] = btn;
    gtk_check_button_set_active(GTK_CHECK_BUTTON(btn), i == *r);
#else
    btn = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(btn), d[i]);
    gtk_box_pack_start(vbox, btn, FALSE, FALSE, 2);
    btn_ary[i] = btn;
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn), i == *r);
#endif
  }

  set_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(dlg);
  res_id = IDLOOP;
  ndialog_run(dlg, &res_id);
#else
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);
#endif

  if (res_id > 0 || res_id == GTK_RESPONSE_OK) {
    *r = -1;
    for (i = 0; i < anum; i++) {
#if GTK_CHECK_VERSION(4, 0, 0)
      if (gtk_check_button_get_active(GTK_CHECK_BUTTON(btn_ary[i]))) {
	*r = i;
	break;
      }
#else
      if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn_ary[i]))) {
	*r = i;
	break;
      }
#endif
    }
    data = IDOK;
  } else {
    data = IDCANCEL;
  }

  if (buttons && res_btn) {
    *res_btn = res_id;
  }

   g_free(btn_ary);

  get_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_destroy(GTK_WINDOW(dlg));
#else
  gtk_widget_destroy(dlg);
#endif
  reset_event();

  return data;
}

static int
add_buttons(GtkWidget *dlg, struct narray *array)
{
  char **d;
  int i, anum;

  if (array == NULL) {
    return 1;
  }

  d = arraydata(array);
  anum = arraynum(array);
  if (anum < 1) {
    return 1;
  }

  for (i = 0; i < anum; i++) {
    if (d[i] && g_utf8_validate(d[i], -1, NULL)) {
      gtk_dialog_add_button(GTK_DIALOG(dlg), d[i], i + 1);
    }
  }
  return 0;
}

int
DialogButton(GtkWidget *parent, const char *title, const char *caption, struct narray *buttons, int *x, int *y)
{
  GtkWidget *dlg;
  gint res_id;

  dlg = gtk_dialog_new();
  if (add_buttons(dlg, buttons)) {
    return 1;
  }

  if (title && g_utf8_validate(title, -1, NULL)) {
    gtk_window_set_title(GTK_WINDOW(dlg), title);
  }

  if (caption && g_utf8_validate(caption, -1, NULL)) {
    GtkWidget *box, *label;
    box = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    label = gtk_label_new(caption);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_orientable_set_orientation(GTK_ORIENTABLE(box), GTK_ORIENTATION_VERTICAL);
    gtk_box_append(GTK_BOX(box), label);
#else
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 4);
#endif
  }

  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  if (parent) {
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(parent));
    gtk_window_set_modal(GTK_WINDOW(parent), TRUE);
  }

  set_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(dlg);
  res_id = IDLOOP;
  ndialog_run(dlg, &res_id);
#else
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);
#endif
  get_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_destroy(GTK_WINDOW(dlg));
#else
  gtk_widget_destroy(dlg);
#endif
  reset_event();

  return res_id;
}

int
DialogCombo(GtkWidget *parent, const char *title, const char *caption, struct narray *array, struct narray *buttons, int *res_btn, int sel, char **r, int *x, int *y)
{
  GtkWidget *dlg, *combo;
  GtkBox *vbox;
  int data;
  gint res_id;
  char **d;
  int i, anum;

  d = arraydata(array);
  anum = arraynum(array);

  *r = NULL;

  dlg = gtk_dialog_new_with_buttons(title,
				    GTK_WINDOW(parent),
#if USE_HEADER_BAR
				    GTK_DIALOG_USE_HEADER_BAR |
#endif
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    NULL, NULL);
  if (add_buttons(dlg, buttons)) {
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
			   _("_Cancel"), GTK_RESPONSE_CANCEL,
			   _("_OK"), GTK_RESPONSE_OK,
			   NULL);
  }
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg)));
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);
#endif

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(vbox, label);
#else
    gtk_box_pack_start(vbox, label, FALSE, FALSE, 5);
#endif
  }

  combo = combo_box_create();
  for (i = 0; i < anum; i++) {
    combo_box_append_text(combo, d[i]);
  }

  if (sel < 0 || sel >= anum) {
    sel = 0;
  }
  combo_box_set_active(combo, sel);

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(vbox, combo);
#else
  gtk_box_pack_start(vbox, combo, FALSE, FALSE, 2);
#endif

  set_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(dlg);
  res_id = IDLOOP;
  ndialog_run(dlg, &res_id);
#else
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);
#endif

  if (res_id > 0 || res_id == GTK_RESPONSE_OK) {
    i = combo_box_get_active(combo);
    if (i >= 0) {
      *r = g_strdup(d[i]);
    }
    data = IDOK;
  } else {
    data = IDCANCEL;
  }

  if (buttons && res_btn) {
    *res_btn = res_id;
  }

  get_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_destroy(GTK_WINDOW(dlg));
#else
  gtk_widget_destroy(dlg);
#endif
  reset_event();

  return data;
}

int
DialogComboEntry(GtkWidget *parent, const char *title, const char *caption, struct narray *array, struct narray *buttons, int *res_btn, int sel, char **r, int *x, int *y)
{
  GtkWidget *dlg, *combo;
  GtkBox *vbox;
  int data;
  gint res_id;
  char **d;
  int i, anum;

  d = arraydata(array);
  anum = arraynum(array);

  *r = NULL;

  dlg = gtk_dialog_new_with_buttons(title,
				    GTK_WINDOW(parent),
#if USE_HEADER_BAR
				    GTK_DIALOG_USE_HEADER_BAR |
#endif
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    NULL, NULL);
  if (add_buttons(dlg, buttons)) {
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
			   _("_Cancel"), GTK_RESPONSE_CANCEL,
			   _("_OK"), GTK_RESPONSE_OK,
			   NULL);
  }
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg)));
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);
#endif

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(vbox, label);
#else
    gtk_box_pack_start(vbox, label, FALSE, FALSE, 5);
#endif
  }

  combo = combo_box_entry_create();
  for (i = 0; i < anum; i++) {
    combo_box_append_text(combo, d[i]);
  }

  if (sel >= 0 && sel < anum) {
    combo_box_set_active(combo, sel);
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(vbox, combo);
#else
  gtk_box_pack_start(vbox, combo, FALSE, FALSE, 2);
#endif

  set_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(dlg);
  res_id = IDLOOP;
  ndialog_run(dlg, &res_id);
#else
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);
#endif

  if (res_id > 0 || res_id == GTK_RESPONSE_OK) {
    const char *s;
    s = combo_box_entry_get_text(combo);
    if (s) {
      *r = g_strdup(s);
    } else {
      *r = NULL;
    }
    data = IDOK;
  } else {
    data = IDCANCEL;
  }

  if (buttons && res_btn) {
    *res_btn = res_id;
  }

  get_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_destroy(GTK_WINDOW(dlg));
#else
  gtk_widget_destroy(dlg);
#endif
  reset_event();

  return data;
}

int
DialogSpinEntry(GtkWidget *parent, const char *title, const char *caption, double min, double max, double inc, struct narray *buttons, int *res_btn, double *r, int *x, int *y)
{
  GtkWidget *dlg, *spin;
  GtkBox *vbox;
  int data, n;
  gint res_id;
  double prec;

  dlg = gtk_dialog_new_with_buttons(title,
				    GTK_WINDOW(parent),
#if USE_HEADER_BAR
				    GTK_DIALOG_USE_HEADER_BAR |
#endif
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    NULL, NULL);
  if (add_buttons(dlg, buttons)) {
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
			   _("_Cancel"), GTK_RESPONSE_CANCEL,
			   _("_OK"), GTK_RESPONSE_OK,
			   NULL);
  }
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg)));
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);
#endif

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(vbox, label);
#else
    gtk_box_pack_start(vbox, label, FALSE, FALSE, 5);
#endif
  }

  if (inc == 0)
    inc = 1;

  spin = gtk_spin_button_new_with_range(min, max, inc);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), *r);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(vbox, spin);
  /* must be implemented */
#else
  gtk_box_pack_start(vbox, spin, FALSE, FALSE, 2);
  gtk_entry_set_activates_default(GTK_ENTRY(spin), TRUE);
#endif

  prec = log10(fabs(inc));
  if (prec < 0) {
    n = ceil(- prec);
  } else {
    n = 0;
  }
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), n);

  set_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(dlg);
  res_id = IDLOOP;
  ndialog_run(dlg, &res_id);
#else
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);
#endif

  if (res_id > 0 || res_id == GTK_RESPONSE_OK) {
    *r = gtk_spin_button_get_value(GTK_SPIN_BUTTON(spin));
    data = IDOK;
  } else {
    data = IDCANCEL;
  }

  if (buttons && res_btn) {
    *res_btn = res_id;
  }

  get_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_destroy(GTK_WINDOW(dlg));
#else
  gtk_widget_destroy(dlg);
#endif
  reset_event();

  return data;
}

int
DialogCheck(GtkWidget *parent, const char *title, const char *caption, struct narray *array, struct narray *buttons, int *res_btn, int *r, int *x, int *y)
{
  GtkWidget *dlg, *btn, **btn_ary;
  GtkBox *vbox;
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
#if USE_HEADER_BAR
				    GTK_DIALOG_USE_HEADER_BAR |
#endif
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
				    NULL, NULL);
  if (add_buttons(dlg, buttons)) {
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
			   _("_Cancel"), GTK_RESPONSE_CANCEL,
			   _("_OK"), GTK_RESPONSE_OK,
			   NULL);
  }
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg)));
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);
#endif

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(vbox, label);
#else
    gtk_box_pack_start(vbox, label, FALSE, FALSE, 5);
#endif
  }


  btn = NULL;
  for (i = 0; i < anum; i++) {
    btn = gtk_check_button_new_with_mnemonic(d[i]);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(vbox, btn);
#else
    gtk_box_pack_start(vbox, btn, FALSE, FALSE, 2);
#endif
    btn_ary[i] = btn;
  }

  for (i = 0; i < anum; i++) {
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_ary[i]), r[i]);
#else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_ary[i]), r[i]);
#endif
  }

  set_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(dlg);
  res_id = IDLOOP;
  ndialog_run(dlg, &res_id);
#else
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);
#endif

  if (res_id > 0 || res_id == GTK_RESPONSE_OK) {
    for (i = 0; i < anum; i++) {
#if GTK_CHECK_VERSION(4, 0, 0)
      r[i] = gtk_check_button_get_active(GTK_CHECK_BUTTON(btn_ary[i]));
#else
      r[i] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn_ary[i]));
#endif
    }
    data = IDOK;
  } else {
    data = IDCANCEL;
  }

  if (buttons && res_btn) {
    *res_btn = res_id;
  }

  g_free(btn_ary);

  get_dialog_position(dlg, x, y);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_destroy(GTK_WINDOW(dlg));
#else
  gtk_widget_destroy(dlg);
#endif
  reset_event();

  return data;
}
#if ! GTK_CHECK_VERSION(4, 0, 0)
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
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
static void
fsok(GtkWidget *dlg, struct nGetOpenFileData *data)
{
  char *file, *file2, **farray, *dir;
  const char *filter_name;
  int i, j, k, len, n;
  GStatBuf buf;
  GListModel *top;
  GtkFileFilter *filter;

  top = gtk_file_chooser_get_files(GTK_FILE_CHOOSER(dlg));
  filter = gtk_file_chooser_get_filter(GTK_FILE_CHOOSER(dlg));

  if (filter) {
    filter_name = gtk_file_filter_get_name(filter);
  } else {
    filter_name = NULL;
  }

  if (filter_name == NULL || strcmp(filter_name, _("All")) == 0) {
    data->ext = NULL;
  }

  n = g_list_model_get_n_items(top);
  farray = g_malloc(sizeof(*farray) * (n + 1));
  if (farray == NULL) {
    g_object_unref(top);
    return;
  }
  data->file = farray;

  k = 0;
  for (j = 0; j < n; j++) {
    GFile *path;
    char *tmp;

    path = g_list_model_get_item(top, j);
    if (path == NULL) {
      message_beep(TopLevel);
      continue;
    }

    tmp = g_file_get_path(path);
    if (tmp == NULL || strlen(tmp) < 1) {
      g_free(tmp);
      message_beep(TopLevel);
      continue;
    }

    file = get_utf8_filename(tmp);

    for (i = strlen(file) - 1; (i > 0) && (file[i] != '/') && (file[i] != '.'); i--);
    if ((file[i] != '.') && data->ext) {
      len = strlen(data->ext) + 1;
    } else {
      len = 0;
    }

    if (len) {
      file2 = g_strdup_printf("%s.%s", file, data->ext);
      g_free(file);
    } else {
      file2 = file;
    }
    if (file2) {
      if (data->mustexist) {
	if ((nstat(file2, &buf) != 0) || ((buf.st_mode & S_IFMT) != S_IFREG)
	    || (naccess(file2, R_OK) != 0)) {
	  message_beep(TopLevel);
	  error22(NULL, 0, "I/O error", file2);
	  g_free(file2);
	  continue;
	}
      } else {
	if ((nstat(file2, &buf) == 0) && ((buf.st_mode & S_IFMT) != S_IFREG)) {
	  message_beep(TopLevel);
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

  if (data->init_dir) {
    g_free(*(data->init_dir));
    dir = g_path_get_dirname(farray[0]);
    *(data->init_dir) = dir;
  }

  farray[k] = NULL;
  g_object_unref(top);
  data->ret = IDOK;
}
#else
static void
fsok(GtkWidget *dlg, struct nGetOpenFileData *data)
{
  char *file, *file2, **farray;
  const char *filter_name;
  int i, k, len, n;
  GStatBuf buf;
  GSList *top, *list;
  GtkFileFilter *filter;

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
    char *tmp;

    tmp = (char *) list->data;
    if (tmp == NULL || strlen(tmp) < 1) {
      message_beep(TopLevel);
      continue;
    }

    file = get_utf8_filename(tmp);

    for (i = strlen(file) - 1; (i > 0) && (file[i] != '/') && (file[i] != '.'); i--);
    if ((file[i] != '.') && data->ext) {
      len = strlen(data->ext) + 1;
    } else {
      len = 0;
    }

    if (len) {
      file2 = g_strdup_printf("%s.%s", file, data->ext);
      g_free(file);
    } else {
      file2 = file;
    }
    if (file2) {
      if (data->mustexist) {
	if ((nstat(file2, &buf) != 0) || ((buf.st_mode & S_IFMT) != S_IFREG)
	    || (naccess(file2, R_OK) != 0)) {
	  message_beep(TopLevel);
	  error22(NULL, 0, "I/O error", file2);
	  g_free(file2);
	  continue;
	}
      } else {
	if ((nstat(file2, &buf) == 0) && ((buf.st_mode & S_IFMT) != S_IFREG)) {
	  message_beep(TopLevel);
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
    if (data->chdir && data->init_dir) {
      char *dir;

      g_free(*(data->init_dir));
      dir = g_path_get_dirname(farray[0]);
      *(data->init_dir) = dir;
    }
  }
  farray[k] = NULL;
  free_str_list(top);
  data->ret = IDOK;
}
#endif

#if ! GTK_CHECK_VERSION(4, 0, 0)
static void
file_dialog_set_current_neme(GtkWidget *dlg, const char *full_name)
{
  char *name;

  if (dlg == NULL || full_name == NULL)
    return;

  name = getbasename(full_name);
  if (name) {
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), name);
    g_free(name);
  }
}

static int
check_overwrite(GtkWidget *parent, const char *filename)
{
  int r;
  char *buf;

  if (filename == NULL || naccess(filename, W_OK))
    return 0;

  buf = g_strdup_printf(_("`%s'\n\nOverwrite existing file?"), CHK_STR(filename));

  r = message_box(parent, buf, "Driver", RESPONS_YESNO);
  g_free(buf);

  return r != IDYES;
}

static char *
get_filename_with_ext(const char *basename, const char *ext)
{
  char *filename;
  int len, ext_len, i;

  if (basename == NULL) {
    return NULL;
  }

  if (ext == NULL || ext[0] == '\0') {
    return g_strdup(basename);
  }

  ext_len = strlen(ext);
  len = strlen(basename);
  i = len - ext_len;

  if (i > 0 && g_strcmp0(basename + i, ext) == 0 && basename[i - 1] == '.') {
    return g_strdup(basename);
  }

  filename = g_strdup_printf("%s%s%s",
			     basename,
			     (basename[len -1] == '.') ? "" : ".",
			     ext);
  return filename;
}
#endif

static int
FileSelectionDialog(GtkWidget *parent, struct nGetOpenFileData *data)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  GtkWidget *dlg;
  GtkFileFilter *filter;
#else
  GtkWidget *dlg, *rc;
  GtkFileFilter *filter;
  char *fname;
#endif

  dlg = gtk_file_chooser_dialog_new(data->title,
				    GTK_WINDOW((parent) ? parent : TopLevel),
				    data->type,
				    _("_Cancel"), GTK_RESPONSE_CANCEL,
				    data->button, GTK_RESPONSE_ACCEPT,
				    NULL);
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dlg), TRUE);
  rc = gtk_check_button_new_with_mnemonic(_("_Change current directory"));
  gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dlg), rc);
  data->chdir_cb = rc;
#endif
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
  } else {
    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*");
    gtk_file_filter_set_name(filter, _("All"));
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.txt");
    gtk_file_filter_set_name(filter, "Text file (*.txt)");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);

    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.dat");
    gtk_file_filter_set_name(filter, "Data file (*.dat)");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);
  }

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  if (data->init_dir && *(data->init_dir)) {
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), *(data->init_dir));
  }
  gtk_widget_show_all(dlg);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  if (data->changedir && data->init_dir) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->chdir_cb), data->chdir);
  } else {
    gtk_widget_hide(data->chdir_cb);
  }

  fname = get_filename_with_ext(data->init_file, data->ext);
  if (fname) {
    if (data->type == GTK_FILE_CHOOSER_ACTION_SAVE) {
      file_dialog_set_current_neme(dlg, fname);
    } else {
      gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(dlg), fname);
    }
    g_free(fname);
  }
#endif

  data->ret = IDCANCEL;

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  while (1) {
#if GTK_CHECK_VERSION(4, 0, 0)
    int res_id;
    res_id = IDLOOP;
    if (ndialog_run(dlg, &res_id) != GTK_RESPONSE_ACCEPT)
      break;
#else
    if (ndialog_run(dlg) != GTK_RESPONSE_ACCEPT)
      break;
#endif

    fsok(dlg, data);
    if (data->ret == IDOK && data->type == GTK_FILE_CHOOSER_ACTION_SAVE) {
      file_dialog_set_current_neme(dlg, data->file[0]);
      if (! data->overwrite && check_overwrite(dlg, data->file[0])) {
	data->ret = IDCANCEL;
	continue;
      }
    }
    break;
  }
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_destroy(GTK_WINDOW(dlg));
#else
  gtk_widget_destroy(dlg);
#endif
  reset_event();
  data->widget = NULL;

  return data->ret;
}

int
nGetOpenFileNameMulti(GtkWidget * parent,
		      char *title, char *defext, char **init_dir,
		      const char *init_file, char ***file, int chd)
{
  int ret;
  struct nGetOpenFileData data;

  data.title = title;
  data.init_dir = init_dir;
  data.init_file = init_file;
  data.file = NULL;
  data.chdir = chd;
  data.ext = defext;
  data.mustexist = TRUE;
  data.overwrite = FALSE;
  data.multi = TRUE;
  data.changedir = TRUE;
  data.type = GTK_FILE_CHOOSER_ACTION_OPEN;
  data.button = _("_Open");
  ret = FileSelectionDialog(parent, &data);
  if (ret == IDOK) {
    *file = data.file;
    if (data.chdir && init_dir && nchdir(*init_dir)) {
      ErrorMessage();
    }
  } else {
    *file = NULL;
  }

  return ret;
}

int
nGetOpenFileName(GtkWidget *parent,
		 char *title, char *defext, char **init_dir, const char *init_file,
		 char **file, int exist, int chd)
{
  struct nGetOpenFileData data;
  int ret;

  data.title = title;
  data.init_dir = init_dir;
  data.init_file = init_file;
  data.file = NULL;
  data.chdir = chd;
  data.ext = defext;
  data.mustexist = exist;
  data.overwrite = FALSE;
  data.multi = FALSE;
  data.changedir = TRUE;
  data.type = (exist) ? GTK_FILE_CHOOSER_ACTION_OPEN : GTK_FILE_CHOOSER_ACTION_SAVE;
  data.button = _("_Open");
  ret = FileSelectionDialog(parent, &data);
  if (ret == IDOK) {
    *file = data.file[0];
    g_free(data.file);
    if (data.chdir && init_dir && nchdir(*init_dir)) {
      ErrorMessage();
    }
  } else {
    *file = NULL;
  }

  return ret;
}

int
nGetSaveFileName(GtkWidget * parent,
		 char *title, char *defext, char **init_dir, const char *init_file,
		 char **file, int overwrite, int chd)
{
  struct nGetOpenFileData data;
  int ret;

  data.title = title;
  data.init_dir = init_dir;
  data.init_file = init_file;
  data.file = NULL;
  data.chdir = chd;
  data.ext = defext;
  data.mustexist = FALSE;
  data.overwrite = overwrite;
  data.multi = FALSE;
  data.changedir = TRUE;
  data.type = GTK_FILE_CHOOSER_ACTION_SAVE,
  data.button = _("_Save");
  ret = FileSelectionDialog(parent, &data);
  if (ret == IDOK) {
    *file = data.file[0];
    g_free(data.file);
    if (data.chdir && init_dir && nchdir(*init_dir)) {
      ErrorMessage();
    }
  } else {
    *file = NULL;
  }

  return ret;
}

void
get_window_geometry(GtkWidget *win, gint *x, gint *y, gint *w, gint *h)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  GdkSurface *surface;
  /* must be implemented */
  surface = gtk_native_get_surface(GTK_NATIVE(win));
  *w = gdk_surface_get_width(surface);
  *h = gdk_surface_get_height(surface);
  *x = 0;
  *y = 0;
#else
  gtk_window_get_size(GTK_WINDOW(win), w, h);
  gtk_window_get_position(GTK_WINDOW(win), x, y);
#endif
}

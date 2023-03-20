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
#include "shell.h"

#include "gtk_widget.h"
#include "gtk_combo.h"

#include "ox11menu.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "x11commn.h"

struct nGetOpenFileData
{
  GtkWidget *widget;
#if GTK_CHECK_VERSION(4, 0, 0)
  void (* response) (struct nGetOpenFileData *);
  union
  {
    files_response_cb files;
    file_response_cb file;
  } callback;
  gpointer data;
#else
  GtkWidget *chdir_cb;
#endif
  int chdir;
  int changedir;
  int ret;
  char *title;
  char **init_dir;
  const char *init_file;
  char *ext;
  char **file;
  const char *button;
  int type;
  int mustexist;
  int overwrite;
  int multi;
};

static int add_buttons(GtkWidget *dlg, struct narray *array);

#if GTK_CHECK_VERSION(4, 0, 0)
void
dialog_wait(int *wait)
{
  while (*wait) {
    msleep(BLOCKING_DIALOG_WAIT);
  }
}
#endif

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
void
ndialog_run(GtkWidget *dlg, GCallback cb, gpointer user_data)
{
  if (cb) {
    g_signal_connect(dlg, "response", cb, user_data);
  }
  gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
  gtk_widget_show(dlg);
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
call_response_cb(struct response_callback *cb)
{
  if (cb->cb) {
    cb->cb(cb);
  }
  if (cb->free) {
    cb->free(cb);
  }
}


static void
dialog_response(GtkWidget *dlg, gint res_id, gpointer user_data)
{
  struct DialogType *data;

  data = (struct DialogType *) user_data;

  if (data->focus)
    gtk_widget_grab_focus(data->focus);

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

  if (data->ret == IDLOOP) {
    return;
  }

#if OSX
  Menulock = data->menulock;
#endif

  //  gtk_widget_destroy(dlg);
  //  data->widget = NULL;
  set_current_window(data->win_ptr);
  gtk_widget_hide(dlg);

  DnDLock = data->lockstate;

  if (data->response_cb) {
    struct response_callback *cb;
    cb = data->response_cb;
    data->response_cb = NULL;
    cb->dialog = data;
    cb->return_value = data->ret;
    call_response_cb(cb);
    g_free(cb);
  }
}

static gboolean
dialog_close_request(GtkWindow* window, gpointer user_data)
{
  gtk_dialog_response(GTK_DIALOG(window), GTK_RESPONSE_CANCEL);
  return TRUE;
}

void
DialogExecute(GtkWidget *parent, void *dialog)
{
  GtkWidget *dlg;
  struct DialogType *data;

  data = (struct DialogType *) dialog;

  data->lockstate = DnDLock;
  DnDLock = TRUE;

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

    /* must be implemented */
    g_signal_connect(dlg, "response", G_CALLBACK(dialog_response), data);
    g_signal_connect(dlg, "close_request", G_CALLBACK(dialog_close_request), data);
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
  data->menulock = Menulock;
  Menulock = TRUE;
#endif
  gtk_widget_show(dlg);
  data->win_ptr = get_current_window();
  set_current_window(dlg);
}
#else
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

    g_signal_connect(dlg, "delete-event", G_CALLBACK(gtk_true), data);
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
#endif

void
message_beep(GtkWidget * parent)
{
  if (parent) {
    GdkSurface *window;
    window = gtk_native_get_surface(GTK_NATIVE(parent));
    gdk_surface_beep(window);
  } else {
    GdkDisplay *disp;
    disp = gdk_display_get_default();
    if (disp) {
      gdk_display_beep(disp);
    }
  }
  //  reset_event();
}

#if GTK_CHECK_VERSION(4, 0, 0)
struct response_message_box_data {
  gpointer data;
  response_cb cb;
};

static void
response_message_box_response (GtkWindow *dlg, gint res_id, gpointer user_data)
{
  struct response_message_box_data *data;
  int r;

  data = (struct response_message_box_data *) user_data;
  switch (res_id) {
  case GTK_RESPONSE_OK:
  case GTK_RESPONSE_YES:
    r = IDYES;
    break;
  case GTK_RESPONSE_NO:
    r = IDNO;
    break;
  default:
    r = IDCANCEL;
    break;
  }

  if (data->cb) {
    data->cb(r, data->data);
  }
  g_free(data);
  gtk_window_destroy(dlg);
}

void
response_message_box(GtkWidget *parent, const char *message, const char *title, int mode, response_cb cb, gpointer user_data)
{
  GtkWidget *dlg;
  struct response_message_box_data *data;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    if (cb) {
      cb(IDCANCEL, user_data);
    }
    return;
  }
  data->cb = cb;
  data->data = user_data;

  if (title == NULL) {
    title = _("Error");
  }

  if (parent == NULL)
    parent = get_current_window();

  dlg = gtk_message_dialog_new(GTK_WINDOW(parent),
			       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			       GTK_MESSAGE_QUESTION,
			       GTK_BUTTONS_NONE,
			       "%.512s", message);

  if (mode == RESPONS_YESNOCANCEL) {
    gtk_dialog_add_button(GTK_DIALOG(dlg), _("_Cancel"), GTK_RESPONSE_CANCEL);
  }
  gtk_dialog_add_button(GTK_DIALOG(dlg), _("_No"), GTK_RESPONSE_NO);
  gtk_dialog_add_button(GTK_DIALOG(dlg), _("_Yes"), GTK_RESPONSE_YES);
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_YES);
  gtk_window_set_title(GTK_WINDOW(dlg), title);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);

  ndialog_run(dlg, G_CALLBACK(response_message_box_response), data);
}

void
message_box(GtkWidget *parent, const char *message, const char *title, int mode)
{
  markup_message_box(parent, message, title, mode, FALSE);
}

struct markup_message_box_data {
  response_cb cb;
  gpointer data;
};

static void
markup_message_box_cb(GtkWidget *dlg, int res, gpointer user_data)
{
  struct markup_message_box_data *data;
  data = (struct markup_message_box_data *) user_data;
  if (data->cb) {
    data->cb(res, data->data);
  }
  g_free(data);
  gtk_window_destroy(GTK_WINDOW(dlg));
}

void
markup_message_box_full(GtkWidget *parent, const char *message, const char *title, int mode, int markup, response_cb cb, gpointer user_data)
{
  GtkWidget *dlg;
  GtkMessageType dlg_type;
  struct markup_message_box_data *data;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    if (cb) {
      cb(IDCANCEL, user_data);
    }
    return;
  }
  data->cb = cb;
  data->data = user_data;

  if (title == NULL) {
    title = _("Error");
  }

  switch (mode) {
  case RESPONS_ERROR:
    dlg_type = GTK_MESSAGE_ERROR;
    break;
  default:
    dlg_type = GTK_MESSAGE_INFO;
    break;
  }

  if (parent == NULL)
    parent = get_current_window();

  dlg = gtk_message_dialog_new(GTK_WINDOW(parent),
			       GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
			       dlg_type,
			       GTK_BUTTONS_OK,
			       "%.512s", message);
  if (markup) {
    gtk_message_dialog_set_markup(GTK_MESSAGE_DIALOG(dlg), message);
  }

  gtk_window_set_title(GTK_WINDOW(dlg), title);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);

  ndialog_run(dlg, G_CALLBACK(markup_message_box_cb), data);
}

void
markup_message_box(GtkWidget *parent, const char *message, const char *title, int mode, int markup)
{
  markup_message_box_full(parent, message, title, mode, markup, NULL, NULL);
}
#else
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
    if ((mode == RESPONS_OK) || (mode == RESPONS_ERROR)) {
      data = IDOK;
    } else if (mode == RESPONS_YESNO) {
      data = IDNO;
    } else {
      data = IDCANCEL;
    }
  }

  gtk_widget_destroy(dlg);
  reset_event();

  return data;
}
#endif

struct input_dialog_data {
  GtkWidget *text;
  string_response_cb cb;
  int *res_btn;
  struct narray *buttons;
  gpointer data;
};

static void
input_dialog_response(GtkWindow *dlg, int response_id, gpointer user_data)
{
  const char *str;
  int res = IDCANCEL;
  struct input_dialog_data *data;

  data = (struct input_dialog_data *) user_data;
  if (data == NULL) {
    return;
  }
  if (response_id > 0 || response_id == GTK_RESPONSE_OK) {
    res = IDOK;
    str = gtk_editable_get_text(GTK_EDITABLE(data->text));
  } else {
    res = IDCANCEL;
    str = NULL;
  }
  if (data->buttons && data->res_btn) {
    *data->res_btn = response_id;
  }
  data->cb(res, str, data->data);
  g_free(data);
  gtk_window_destroy(GTK_WINDOW(dlg));
}

void
input_dialog(GtkWidget *parent, const char *title, const char *mes, const char *init_str, const char *button, struct narray *buttons, int *res_btn, string_response_cb cb, gpointer user_data)
{
  GtkWidget *dlg, *text;
  GtkBox *vbox;
  struct input_dialog_data *data;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    if (cb) {
      cb(IDCANCEL, NULL, user_data);
    }
    return;
  }
  dlg = gtk_dialog_new_with_buttons(title,
				    GTK_WINDOW(parent),
#if USE_HEADER_BAR
				    GTK_DIALOG_USE_HEADER_BAR |
#endif
				    GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                    NULL,
				    NULL);
  if (add_buttons(dlg, buttons)) {
    gtk_dialog_add_buttons(GTK_DIALOG(dlg),
			   _("_Cancel"), GTK_RESPONSE_CANCEL,
			   button, GTK_RESPONSE_OK,
			   NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  }
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg)));
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);

  if (mes) {
    GtkWidget *label;
    label = gtk_label_new(mes);
    gtk_box_append(vbox, label);
  }

  text = create_text_entry(FALSE, TRUE);
  if (init_str) {
    gtk_editable_set_text(GTK_EDITABLE(text), init_str);
  }
  gtk_box_append(vbox, text);

  data->cb = cb;
  data->data = user_data;
  data->text = text;
  data->buttons = buttons;
  data->res_btn = res_btn;
  g_signal_connect(dlg, "response", G_CALLBACK(input_dialog_response), data);
  gtk_widget_show(dlg);
}

struct radio_dialog_data {
  response_cb cb;
  int anum;
  struct narray *buttons;
  int *res_btn;
  GtkWidget **btn_ary;
  gpointer data;
};

static void
radio_dialog_response(GtkWidget *dlg, int res_id, gpointer user_data)
{
  struct radio_dialog_data *data;
  int i, selected;
  data = (struct radio_dialog_data *) user_data;
  selected = -1;
  if (res_id > 0 || res_id == GTK_RESPONSE_OK) {
    for (i = 0; i < data->anum; i++) {
      if (gtk_check_button_get_active(GTK_CHECK_BUTTON(data->btn_ary[i]))) {
	selected = i;
	break;
      }
    }
  }

  if (data->buttons && data->res_btn) {
    *data->res_btn = res_id;
  }

  gtk_window_destroy(GTK_WINDOW(dlg));
  data->cb(selected, data->data);

  g_free(data->btn_ary);
  g_free(data);
}

void
radio_dialog(GtkWidget *parent, const char *title, const char *caption, struct narray *array, const char *button, struct narray *buttons, int *res_btn, int selected, response_cb cb, gpointer user_data)
{
  GtkWidget *dlg, *btn, **btn_ary;
  GtkBox *vbox;
  char **d;
  int i, anum;
  GtkWidget *group = NULL;
  struct radio_dialog_data *data;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    if (cb) {
      cb(IDCANCEL, user_data);
    }
    return;
  }

  d = arraydata(array);
  anum = arraynum(array);

  btn_ary = g_malloc(anum * sizeof(*btn_ary));
  if (btn_ary == NULL) {
    if (cb) {
      cb(IDCANCEL, user_data);
    }
    g_free(data);
    return;
  }

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
			   button, GTK_RESPONSE_OK,
			   NULL);
  }

  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  vbox = GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg)));
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
    gtk_box_append(vbox, label);
  }

  btn = NULL;
  for (i = 0; i < anum; i++) {
    btn = gtk_check_button_new_with_mnemonic(d[i]);
    if (group) {
      gtk_check_button_set_group(GTK_CHECK_BUTTON(btn), GTK_CHECK_BUTTON(group));
    } else {
      group = btn;
    }
    gtk_box_append(vbox, btn);
    btn_ary[i] = btn;
    gtk_check_button_set_active(GTK_CHECK_BUTTON(btn), i == selected);
  }

  data->cb = cb;
  data->data = user_data;
  data->buttons = buttons;
  data->res_btn = res_btn;
  data->anum = anum;
  data->btn_ary = btn_ary;
  g_signal_connect(dlg, "response", G_CALLBACK(radio_dialog_response), data);
  gtk_widget_show(dlg);
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

#if GTK_CHECK_VERSION(4, 0, 0)
struct button_dialog_data {
  response_cb cb;
  gpointer data;
};

static void
button_dialog_response(GtkWidget *dlg, int res_id, gpointer user_data)
{
  struct button_dialog_data *data;
  data = (struct button_dialog_data *) user_data;
  gtk_window_destroy(GTK_WINDOW(dlg));
  data->cb(res_id, data->data);
  g_free(data);
}

void
button_dialog(GtkWidget *parent, const char *title, const char *caption, struct narray *buttons, response_cb cb, gpointer user_data)
{
  GtkWidget *dlg;
  struct button_dialog_data *data;

  dlg = gtk_dialog_new();
  if (add_buttons(dlg, buttons)) {
    return;
  }

  if (title && g_utf8_validate(title, -1, NULL)) {
    gtk_window_set_title(GTK_WINDOW(dlg), title);
  }

  if (caption && g_utf8_validate(caption, -1, NULL)) {
    GtkWidget *box, *label;
    box = gtk_dialog_get_content_area(GTK_DIALOG(dlg));
    label = gtk_label_new(caption);
    gtk_orientable_set_orientation(GTK_ORIENTABLE(box), GTK_ORIENTATION_VERTICAL);
    gtk_box_append(GTK_BOX(box), label);
  }

  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  if (parent) {
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(parent));
    gtk_window_set_modal(GTK_WINDOW(parent), TRUE);
  }

  data = g_malloc0(sizeof(*data));
  data->cb = cb;
  data->data = user_data;
  g_signal_connect(dlg, "response", G_CALLBACK(button_dialog_response), data);
  gtk_widget_show(dlg);
}
#else
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
    gtk_box_pack_start(GTK_BOX(box), label, TRUE, TRUE, 4);
  }

  gtk_window_set_resizable(GTK_WINDOW(dlg), FALSE);
  if (parent) {
    gtk_window_set_transient_for(GTK_WINDOW(dlg), GTK_WINDOW(parent));
    gtk_window_set_modal(GTK_WINDOW(parent), TRUE);
  }

  set_dialog_position(dlg, x, y);
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);
  get_dialog_position(dlg, x, y);
  gtk_widget_destroy(dlg);
  reset_event();

  return res_id;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
struct combo_dialog_data {
  GtkWidget *combo;
  string_response_cb cb;
  int *res_btn;
  char **label;
  struct narray *buttons;
  gpointer data;
};

static void
combo_dialog_response(GtkWidget *dlg, int response, gpointer user_data)
{
  struct combo_dialog_data *data;
  int i, r;
  const char *selected;

  data = (struct combo_dialog_data *) user_data;
  selected = NULL;
  r = IDCANCEL;
  if (response > 0 || response == GTK_RESPONSE_OK) {
    i = combo_box_get_active(data->combo);
    if (i >= 0) {
      selected = data->label[i];
    }
    r = IDOK;
  }

  if (data->buttons && data->res_btn) {
    *data->res_btn = response;
  }
  if (data->cb) {
    data->cb(r, selected, data->data);
  }
  g_free(data);
  gtk_window_destroy(GTK_WINDOW(dlg));
}

void
combo_dialog(GtkWidget *parent, const char *title, const char *caption, struct narray *array, struct narray *buttons, int *res_btn, int sel, string_response_cb cb, gpointer user_data)
{
  GtkWidget *dlg, *combo;
  GtkBox *vbox;
  char **d;
  int i, anum;
  struct combo_dialog_data *data;

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
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
    gtk_box_append(vbox, label);
  }

  combo = combo_box_create();
  d = arraydata(array);
  anum = arraynum(array);
  for (i = 0; i < anum; i++) {
    combo_box_append_text(combo, d[i]);
  }

  if (sel < 0 || sel >= anum) {
    sel = 0;
  }
  combo_box_set_active(combo, sel);
  gtk_box_append(vbox, combo);

  data = g_malloc0(sizeof(*data));
  data->combo = combo;
  data->cb = cb;
  data->res_btn = res_btn;
  data->label = d;
  data->buttons = buttons;
  data->data = user_data;
  g_signal_connect(dlg, "response", G_CALLBACK(combo_dialog_response), data);
  gtk_widget_show(dlg);
}
#else
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

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(dlg);
  res_id = IDLOOP;
  ndialog_run(dlg, NULL, &res_id);
#else
  set_dialog_position(dlg, x, y);
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

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_destroy(GTK_WINDOW(dlg));
#else
  get_dialog_position(dlg, x, y);
  gtk_widget_destroy(dlg);
  reset_event();
#endif

  return data;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
static void
combo_entry_dialog_response(GtkWidget *dlg, int response, gpointer user_data)
{
  struct combo_dialog_data *data;
  int r;
  const char *selected;

  data = (struct combo_dialog_data *) user_data;
  selected = NULL;
  r = IDCANCEL;

  if (response > 0 || response == GTK_RESPONSE_OK) {
    selected = combo_box_entry_get_text(data->combo);
    r = IDOK;
  }
  if (data->buttons && data->res_btn) {
    *data->res_btn = response;
  }
  if (data->cb) {
    data->cb(r, selected, data->data);
  }
  g_free(data);
  gtk_window_destroy(GTK_WINDOW(dlg));
}

void
combo_entry_dialog(GtkWidget *parent, const char *title, const char *caption, struct narray *array, struct narray *buttons, int *res_btn, int sel, string_response_cb cb, gpointer user_data)
{
  GtkWidget *dlg, *combo;
  GtkBox *vbox;
  char **d;
  int i, anum;
  struct combo_dialog_data *data;

  d = arraydata(array);
  anum = arraynum(array);

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
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
    gtk_box_append(vbox, label);
  }

  combo = combo_box_entry_create();
  for (i = 0; i < anum; i++) {
    combo_box_append_text(combo, d[i]);
  }

  if (sel >= 0 && sel < anum) {
    combo_box_set_active(combo, sel);
  }

  gtk_box_append(vbox, combo);

  data = g_malloc0(sizeof(*data));
  data->combo = combo;
  data->cb = cb;
  data->res_btn = res_btn;
  data->label = d;
  data->buttons = buttons;
  data->data = user_data;
  g_signal_connect(dlg, "response", G_CALLBACK(combo_entry_dialog_response), data);
  gtk_widget_show(dlg);
}
#else
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

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(dlg);
  res_id = IDLOOP;
  ndialog_run(dlg, NULL, &res_id);
#else
  set_dialog_position(dlg, x, y);
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

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_destroy(GTK_WINDOW(dlg));
#else
  get_dialog_position(dlg, x, y);
  gtk_widget_destroy(dlg);
  reset_event();
#endif

  return data;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
struct spin_dialog_data {
  GtkWidget *spin;
  response_cb cb;
  int *res_btn;
  double *val;
  struct narray *buttons;
  gpointer data;
};

static void
spin_dialog_response(GtkWidget *dialog, int response, gpointer user_data)
{
  int res;
  struct spin_dialog_data *data;
  data = (struct spin_dialog_data *) user_data;

  if (response > 0 || response == GTK_RESPONSE_OK) {
    *data->val = gtk_spin_button_get_value(GTK_SPIN_BUTTON(data->spin));
    res = IDOK;
  } else {
    res = IDCANCEL;
  }

  if (data->buttons && data->res_btn) {
    *data->res_btn = response;
  }
  data->cb(res, data->data);
  g_free(data);
  gtk_window_destroy(GTK_WINDOW(dialog));
}

void
spin_dialog(GtkWidget *parent, const char *title, const char *caption, double min, double max, double inc, struct narray *buttons, int *res_btn, double *r, response_cb cb, gpointer user_data)
{
  GtkWidget *dlg, *spin;
  GtkBox *vbox;
  int n;
  double prec;
  struct spin_dialog_data *data;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    if (cb) {
      cb(IDCANCEL, user_data);
    }
    return;
  }

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
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
    gtk_box_append(vbox, label);
  }

  if (inc == 0)
    inc = 1;

  spin = gtk_spin_button_new_with_range(min, max, inc);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), *r);
  spin_button_set_activates_default(spin);
  gtk_box_append(vbox, spin);

  prec = log10(fabs(inc));
  if (prec < 0) {
    n = ceil(- prec);
  } else {
    n = 0;
  }
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(spin), n);

  data->spin = spin;
  data->cb = cb;
  data->data = user_data;
  data->val = r;
  data->buttons = buttons;
  data->res_btn = res_btn;
  g_signal_connect(dlg, "response", G_CALLBACK(spin_dialog_response), data);
  gtk_widget_show(dlg);
}
#else
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

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
    gtk_box_pack_start(vbox, label, FALSE, FALSE, 5);
  }

  if (inc == 0)
    inc = 1;

  spin = gtk_spin_button_new_with_range(min, max, inc);
  gtk_spin_button_set_numeric(GTK_SPIN_BUTTON(spin), TRUE);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(spin), *r);
  gtk_box_pack_start(vbox, spin, FALSE, FALSE, 2);
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
  gtk_widget_destroy(dlg);
  reset_event();

  return data;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
struct check_dialog_data {
  GtkWidget **btn_ary;
  response_cb cb;
  int *res_btn, anum, *r;
  double *val;
  struct narray *buttons;
  gpointer data;
};

static void
check_dialog_response(GtkWidget *dialog, int response, gpointer user_data)
{
  int i, res;
  struct check_dialog_data *data;
  data = (struct check_dialog_data *) user_data;

  res = IDCANCEL;
  if (response > 0 || response == GTK_RESPONSE_OK) {
    for (i = 0; i < data->anum; i++) {
      data->r[i] = gtk_check_button_get_active(GTK_CHECK_BUTTON(data->btn_ary[i]));
    }
    res = IDOK;
  }

  if (data->buttons && data->res_btn) {
    *data->res_btn = response;
  }
  data->cb(res, data->data);
  g_free(data->btn_ary);
  gtk_window_destroy(GTK_WINDOW(dialog));
  g_free(data);
}

void
check_dialog(GtkWidget *parent, const char *title, const char *caption, struct narray *array, struct narray *buttons, int *res_btn, int *r, response_cb cb, gpointer user_data)
{
  GtkWidget *dlg, *btn, **btn_ary;
  GtkBox *vbox;
  char **d;
  int i, anum;
  struct check_dialog_data *data;

  data = g_malloc0(sizeof(*data));
  d = arraydata(array);
  anum = arraynum(array);

  btn_ary = g_malloc(anum * sizeof(*btn_ary));
  if (btn_ary == NULL)
    return;

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
  gtk_orientable_set_orientation(GTK_ORIENTABLE(vbox), GTK_ORIENTATION_VERTICAL);

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
    gtk_box_append(vbox, label);
  }


  btn = NULL;
  for (i = 0; i < anum; i++) {
    btn = gtk_check_button_new_with_mnemonic(d[i]);
    gtk_box_append(vbox, btn);
    btn_ary[i] = btn;
  }

  for (i = 0; i < anum; i++) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(btn_ary[i]), r[i]);
  }

  data->anum = anum;
  data->btn_ary = btn_ary;
  data->cb = cb;
  data->data = user_data;
  data->r = r;
  data->buttons = buttons;
  data->res_btn = res_btn;
  g_signal_connect(dlg, "response", G_CALLBACK(check_dialog_response), data);
  gtk_widget_show(dlg);
}
#else
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

  if (caption) {
    GtkWidget *label;
    label = gtk_label_new(caption);
    gtk_box_pack_start(vbox, label, FALSE, FALSE, 5);
  }


  btn = NULL;
  for (i = 0; i < anum; i++) {
    btn = gtk_check_button_new_with_mnemonic(d[i]);
    gtk_box_pack_start(vbox, btn, FALSE, FALSE, 2);
    btn_ary[i] = btn;
  }

  for (i = 0; i < anum; i++) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_ary[i]), r[i]);
  }

  set_dialog_position(dlg, x, y);
  gtk_widget_show_all(dlg);
  res_id = ndialog_run(dlg);

  if (res_id > 0 || res_id == GTK_RESPONSE_OK) {
    for (i = 0; i < anum; i++) {
      r[i] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(btn_ary[i]));
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
  gtk_widget_destroy(dlg);
  reset_event();

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
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
#define FILE_CHOOSER_OPTION_CHDIR "chdir"

static void
fsok(GtkWidget *dlg, struct nGetOpenFileData *data)
{
  char *file2, **farray;
  const char *filter_name;
  int j, k, n;
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

#if GTK_CHECK_VERSION(4, 0, 0)
    file2 = get_utf8_filename(tmp);
#else
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
#endif
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
    const char *chdir;
    chdir = gtk_file_chooser_get_choice(GTK_FILE_CHOOSER(dlg), FILE_CHOOSER_OPTION_CHDIR);
    data->chdir = (chdir && (g_strcmp0(chdir, "true") == 0));
    if (data->chdir && data->init_dir) {
      char *dir;

      g_free(*(data->init_dir));
      dir = g_path_get_dirname(farray[0]);
      *(data->init_dir) = dir;
    }
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

#if ! GTK_CHECK_VERSION(4, 0, 0)
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
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
static void
FileSelectionDialog_response(GtkWidget* dlg, gint response_id, gpointer user_data)
{
  struct nGetOpenFileData *data;

  data = (struct nGetOpenFileData *) user_data;

  data->ret = IDCANCEL;
  if (response_id == GTK_RESPONSE_ACCEPT) {
    fsok(dlg, data);
  }

  if (data->response) {
    data->response(data);
  }
  gtk_window_destroy(GTK_WINDOW(dlg));
  g_free(data);
}

static void
FileSelectionDialog(GtkWidget *parent, struct nGetOpenFileData *data)
{
  GtkWidget *dlg;
  GtkFileFilter *filter;
  char *fname;

  dlg = gtk_file_chooser_dialog_new(data->title,
				    GTK_WINDOW((parent) ? parent : TopLevel),
				    data->type,
				    _("_Cancel"), GTK_RESPONSE_CANCEL,
				    data->button, GTK_RESPONSE_ACCEPT,
				    NULL);
  gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
  if (data->changedir && data->init_dir) {
    gtk_file_chooser_add_choice(GTK_FILE_CHOOSER(dlg), FILE_CHOOSER_OPTION_CHDIR, _("Change current directory"), NULL, NULL);
    gtk_file_chooser_set_choice(GTK_FILE_CHOOSER(dlg), FILE_CHOOSER_OPTION_CHDIR, (data->chdir) ? "true" : "false");
  }
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

  if (data->init_dir && *(data->init_dir)) {
    GFile *path;
    path = g_file_new_for_path(*(data->init_dir));
    if (path) {
      gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), path, NULL);
      g_object_unref(path);
    }
  }
  g_signal_connect(dlg, "response", G_CALLBACK(FileSelectionDialog_response), data);

  fname = get_filename_with_ext(data->init_file, data->ext);
  if (fname) {
    if (data->type == GTK_FILE_CHOOSER_ACTION_SAVE) {
      file_dialog_set_current_neme(dlg, fname);
    } else {
      GFile *file;
      file = g_file_new_for_path(fname);
      gtk_file_chooser_set_file(GTK_FILE_CHOOSER(dlg), file, NULL);
      g_object_unref(file);
    }
    g_free(fname);
  }

  gtk_widget_show(dlg);
}
#else
static int
FileSelectionDialog(GtkWidget *parent, struct nGetOpenFileData *data)
{
  GtkWidget *dlg, *rc;
  GtkFileFilter *filter;
  char *fname;

  dlg = gtk_file_chooser_dialog_new(data->title,
				    GTK_WINDOW((parent) ? parent : TopLevel),
				    data->type,
				    _("_Cancel"), GTK_RESPONSE_CANCEL,
				    data->button, GTK_RESPONSE_ACCEPT,
				    NULL);
  gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
  gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(dlg), TRUE);
  rc = gtk_check_button_new_with_mnemonic(_("_Change current directory"));
  gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dlg), rc);
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

  if (data->init_dir && *(data->init_dir)) {
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), *(data->init_dir));
  }
  gtk_widget_show_all(dlg);

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

  data->ret = IDCANCEL;

  while (1) {
    if (ndialog_run(dlg) != GTK_RESPONSE_ACCEPT)
      break;

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

  gtk_widget_destroy(dlg);
  reset_event();
  data->widget = NULL;

  return data->ret;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
static void
nGetOpenFileNameMulti_response(struct nGetOpenFileData *data)
{
  char **files;

  files = NULL;
  if (data->file) {
    files = data->file;
    if (data->chdir && data->init_dir && nchdir(*data->init_dir)) {
      ErrorMessage();
    }
  }
  if (data->callback.files) {
    data->callback.files(files, data->data);
  } else {
    g_free(files);
  }
}

void
nGetOpenFileNameMulti(GtkWidget * parent,
		      char *title, char *defext, char **init_dir,
		      const char *init_file, int chd,
                      files_response_cb cb, gpointer user_data)
{
  struct nGetOpenFileData *data;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    if (cb) {
      cb(NULL, user_data);
    }
    return;
  }

  data->title = title;
  data->init_dir = init_dir;
  data->init_file = init_file;
  data->file = NULL;
  data->chdir = chd;
  data->ext = defext;
  data->mustexist = TRUE;
  data->overwrite = FALSE;
  data->multi = TRUE;
  data->changedir = TRUE;
  data->type = GTK_FILE_CHOOSER_ACTION_OPEN;
  data->button = _("_Open");
  data->callback.files = cb;
  data->response = nGetOpenFileNameMulti_response;
  data->data = user_data;
  FileSelectionDialog(parent, data);
}
#else
char **
nGetOpenFileNameMulti(GtkWidget * parent,
		      char *title, char *defext, char **init_dir,
		      const char *init_file, int chd, gpointer user_data)
{
  int ret;
  struct nGetOpenFileData *data;
  char **file;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return NULL;
  }

  data->title = title;
  data->init_dir = init_dir;
  data->init_file = init_file;
  data->file = NULL;
  data->chdir = chd;
  data->ext = defext;
  data->mustexist = TRUE;
  data->overwrite = FALSE;
  data->multi = TRUE;
  data->changedir = TRUE;
  data->type = GTK_FILE_CHOOSER_ACTION_OPEN;
  data->button = _("_Open");
#if GTK_CHECK_VERSION(4, 0, 0)
  data->response = NULL;
#endif
  ret = FileSelectionDialog(parent, data);
  if (ret == IDOK) {
    file = data->file;
#if GTK_CHECK_VERSION(4, 0, 0)
    if (chd && init_dir && nchdir(*init_dir)) {
      ErrorMessage();
    }
#else
    if (data->chdir && init_dir && nchdir(*init_dir)) {
      ErrorMessage();
    }
#endif
  } else {
    file = NULL;
  }

#if ! GTK_CHECK_VERSION(4, 0, 0)
  g_free(data);
#endif
  return file;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
static void
nGetOpenFileName_response(struct nGetOpenFileData *data)
{
  char *file;
  if (data->file) {
    file = data->file[0];
    g_free(data->file);
    if (data->chdir && data->init_dir && nchdir(*data->init_dir)) {
      ErrorMessage();
    }
  } else {
    file = NULL;
  }
  if (data->callback.file) {
    data->callback.file(file, data->data);
  } else {
    g_free(file);
  }
}

void
nGetOpenFileName(GtkWidget *parent,
		 char *title, char *defext, char **init_dir, const char *init_file,
                 int chd, file_response_cb cb, gpointer user_data)
{
  struct nGetOpenFileData *data;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    if (cb) {
      cb(NULL, user_data);
    }
    return;
  }
  data->title = title;
  data->init_dir = init_dir;
  data->init_file = init_file;
  data->file = NULL;
  data->chdir = chd;
  data->ext = defext;
  data->mustexist = TRUE;
  data->overwrite = FALSE;
  data->multi = FALSE;
  data->changedir = TRUE;
  data->type = GTK_FILE_CHOOSER_ACTION_OPEN;
  data->button = _("_Open");
  data->response = nGetOpenFileName_response;
  data->callback.file = cb;
  data->data = user_data;
  FileSelectionDialog(parent, data);
}
#else
char *
nGetOpenFileName(GtkWidget *parent,
		 char *title, char *defext, char **init_dir, const char *init_file,
                 int exist, int chd)
{
  struct nGetOpenFileData *data;
  int ret;
  char *file;

  data->title = title;
  data->init_dir = init_dir;
  data->init_file = init_file;
  data->file = NULL;
  data->chdir = chd;
  data->ext = defext;
  data->mustexist = TRUE;
  data->overwrite = FALSE;
  data->multi = FALSE;
  data->changedir = TRUE;
  data->type = (exist) ? GTK_FILE_CHOOSER_ACTION_OPEN : GTK_FILE_CHOOSER_ACTION_SAVE;
  data->button = _("_Open");
  ret = FileSelectionDialog(parent, data);
  if (ret == IDOK) {
    file = data->file[0];
    g_free(data->file);
    if (data->chdir && init_dir && nchdir(*init_dir)) {
      ErrorMessage();
    }
  } else {
    file = NULL;
  }

  g_free(data);
  return file;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
static void
nGetSaveFileName_response(struct nGetOpenFileData *data)
{
  char *file;
  if (data->file) {
    file = data->file[0];
    g_free(data->file);
    if (data->chdir && data->init_dir && nchdir(*data->init_dir)) {
      ErrorMessage();
    }
  } else {
    file = NULL;
  }
  if (data->callback.file) {
    data->callback.file(file, data->data);
  } else {
    g_free(file);
  }
}

void
nGetSaveFileName(GtkWidget * parent,
		 char *title, char *defext, char **init_dir, const char *init_file,
                 int chd, file_response_cb cb, gpointer user_data)
{
  struct nGetOpenFileData *data;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    if (cb) {
      cb(NULL, user_data);
    }
    return;
  }
  data->title = title;
  data->init_dir = init_dir;
  data->init_file = init_file;
  data->file = NULL;
  data->chdir = chd;
  data->ext = defext;
  data->mustexist = FALSE;
  data->overwrite = FALSE;
  data->multi = FALSE;
  data->changedir = TRUE;
  data->type = GTK_FILE_CHOOSER_ACTION_SAVE,
  data->button = _("_Save");
  data->response = nGetSaveFileName_response;
  data->callback.file = cb;
  data->data = user_data;
  FileSelectionDialog(parent, data);
}
#else
char *
nGetSaveFileName(GtkWidget * parent,
		 char *title, char *defext, char **init_dir, const char *init_file, int overwrite, int chd)
{
  struct nGetOpenFileData *data;
  int ret;
  char *file;

  data->title = title;
  data->init_dir = init_dir;
  data->init_file = init_file;
  data->file = NULL;
  data->chdir = chd;
  data->ext = defext;
  data->mustexist = FALSE;
  data->overwrite = overwrite;
  data->multi = FALSE;
  data->changedir = TRUE;
  data->type = GTK_FILE_CHOOSER_ACTION_SAVE,
  data->button = _("_Save");
  ret = FileSelectionDialog(parent, data);
  if (ret == IDOK) {
    file = data->file[0];
    g_free(data->file);
    if (data->chdir && init_dir && nchdir(*init_dir)) {
      ErrorMessage();
    }
  } else {
    *file = NULL;
  }

  g_free(data);
  return ret;
}
#endif

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

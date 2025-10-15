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
  void (* response) (struct nGetOpenFileData *);
  union
  {
    files_response_cb files;
    file_response_cb file;
  } callback;
  gpointer data;
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

static int add_buttons(GtkWidget *dlg, const struct narray *array);

void
dialog_wait(volatile const int *wait)
{
  while (*wait) {
    msleep(BLOCKING_DIALOG_WAIT);
  }
}

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
ndialog_run(GtkWidget *dlg, GCallback cb, gpointer user_data)
{
  if (cb) {
    g_signal_connect(dlg, "response", cb, user_data);
  }
  gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);
  gtk_window_present(GTK_WINDOW (dlg));
}

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

void
dialog_response(gint res_id, struct DialogType *data)
{
  GtkWidget *dlg;

  dlg = data->widget;

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
  gtk_widget_set_visible(dlg, FALSE);

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
dialog_close_request(struct DialogType *data)
{
  dialog_response (IDCANCEL, data);
  return TRUE;
}

static void
dialog_response_ok (struct DialogType *data)
{
  dialog_response (IDOK, data);
}

static void
dialog_response_cancel (struct DialogType *data)
{
  dialog_response (IDCANCEL, data);
}

static void
dialog_response_all (struct DialogType *data)
{
  dialog_response (IDSALL, data);
}

GtkWidget *
dialog_add_button (struct DialogType *data, const char *msg, void (* cb) (struct DialogType *))
{
  GtkWidget *btn, *headerbar;
  headerbar = gtk_window_get_titlebar (GTK_WINDOW (data->widget));
  btn = gtk_button_new_with_mnemonic (msg);
  gtk_header_bar_pack_end (GTK_HEADER_BAR (headerbar), btn);
  g_signal_connect_swapped (btn, "clicked", G_CALLBACK (cb), data);
  return btn;
}

GtkWidget *
dialog_add_all_button (struct DialogType *data)
{
  return dialog_add_button (data, _("_All"), dialog_response_all);
}

static void
dialog_response_delete (struct DialogType *data)
{
  dialog_response (IDDELETE, data);
}

GtkWidget *
dialog_add_delete_button (struct DialogType *data)
{
  GtkWidget *btn;
  btn = dialog_add_button (data, _("_Delete"), dialog_response_delete);
  gtk_widget_add_css_class (btn, "destructive-action");
  return btn;
}

static void
dialog_response_apply_all (struct DialogType *data)
{
  dialog_response (IDFAPPLY, data);
}

GtkWidget *
dialog_add_apply_all_button (struct DialogType *data)
{
  return dialog_add_button (data, _("_Apply all"), dialog_response_apply_all);
}

static void
dialog_response_save (struct DialogType *data)
{
  dialog_response (IDSAVE, data);
}

GtkWidget *
dialog_add_save_button (struct DialogType *data)
{
  return dialog_add_button (data, _("_Save"), dialog_response_save);
}

static void
dialog_response_mask (struct DialogType *data)
{
  dialog_response (IDEVMASK, data);
}

static void
dialog_response_move (struct DialogType *data)
{
  dialog_response (IDEVMOVE, data);
}

void
dialog_add_move_button (struct DialogType *data)
{
  GtkWidget *btn, *headerbar;
  headerbar = gtk_window_get_titlebar (GTK_WINDOW (data->widget));
  btn = gtk_button_new_with_mnemonic (_("_Mask"));
  gtk_header_bar_pack_start (GTK_HEADER_BAR (headerbar), btn);
  g_signal_connect_swapped (btn, "clicked", G_CALLBACK (dialog_response_mask), data);

  btn = gtk_button_new_with_mnemonic (_("_Move"));
  gtk_header_bar_pack_start (GTK_HEADER_BAR (headerbar), btn);
  g_signal_connect_swapped (btn, "clicked", G_CALLBACK (dialog_response_move), data);
}

static gboolean
dialog_escape (GtkWidget* widget, GVariant* args, gpointer user_data)
{
  gtk_window_close (GTK_WINDOW (widget));
  return TRUE;
}

#define DESTROY_DIALOG 0
void
DialogExecute(GtkWidget *parent, void *dialog)
{
  GtkWidget *dlg, *parent_window;
  struct DialogType *data;
#if DESTROY_DIALOG
  int w, h;
#endif

  data = (struct DialogType *) dialog;

  data->lockstate = DnDLock;
  DnDLock = TRUE;

#if DESTROY_DIALOG
  w = h = 0;
  if (data->widget) {
    gtk_window_get_default_size (GTK_WINDOW (data->widget), &w, &h);
    gtk_window_destroy (GTK_WINDOW (data->widget));
    data->widget = NULL;
  }
#else
  if (data->widget && (data->parent != parent)) {
    gtk_window_set_transient_for(GTK_WINDOW(data->widget), GTK_WINDOW(parent));
    data->parent = parent;
  }
#endif

  if (data->widget == NULL) {
    GtkWidgetClass *widget_class;
    GObjectClass *class;
    GtkWidget *headerbar;
    GtkWidget *btn;
    dlg = gtk_window_new();

    g_signal_connect_swapped(dlg, "close-request", G_CALLBACK(dialog_close_request), data);

    data->parent = parent;
    data->widget = dlg;
    gtk_window_set_title (GTK_WINDOW (dlg), _(data->resource));
    data->vbox = GTK_BOX(gtk_box_new (GTK_ORIENTATION_VERTICAL, 0));
    headerbar = gtk_header_bar_new();
    gtk_header_bar_set_decoration_layout (GTK_HEADER_BAR (headerbar), "");
    gtk_window_set_titlebar (GTK_WINDOW (dlg), headerbar);
    gtk_window_set_child (GTK_WINDOW (dlg), GTK_WIDGET (data->vbox));
    data->show_cancel = TRUE;
    data->ok_button = _("_OK");

    data->SetupWindow(dlg, data, TRUE);
    btn = gtk_button_new_with_mnemonic (data->ok_button);
    gtk_window_set_default_widget (GTK_WINDOW (dlg), btn);
    data->ok = btn;

    g_signal_connect_swapped (btn, "clicked", G_CALLBACK (dialog_response_ok), data);
    gtk_widget_add_css_class (btn, "suggested-action");
    gtk_header_bar_pack_end (GTK_HEADER_BAR (headerbar), btn);

    if (data->show_cancel) {
      btn = gtk_button_new_with_mnemonic (_("_Cancel"));
      g_signal_connect_swapped (btn, "clicked", G_CALLBACK (dialog_response_cancel), data);
      gtk_header_bar_pack_start (GTK_HEADER_BAR (headerbar), btn);
    }

    gtk_window_set_resizable (GTK_WINDOW (dlg), TRUE);
    parent_window = gtk_widget_get_ancestor (parent, GTK_TYPE_WINDOW);
    gtk_window_set_transient_for (GTK_WINDOW (dlg), GTK_WINDOW (parent_window));
    gtk_window_set_modal (GTK_WINDOW (dlg), TRUE);
    class = G_OBJECT_GET_CLASS(dlg);
    widget_class = GTK_WIDGET_CLASS (class);
    gtk_widget_class_add_binding (widget_class, GDK_KEY_Escape, 0, dialog_escape, NULL);
#if DESTROY_DIALOG
    if (w && h) {
      gtk_window_set_default_size (GTK_WINDOW (data->widget), w, h);
    }
#endif
  } else {
    dlg = data->widget;
    data->SetupWindow(dlg, data, FALSE);
  }

  data->ret = IDLOOP;

#if OSX
  data->menulock = Menulock;
  Menulock = TRUE;
#endif
  if (data->focus) {
    gtk_window_set_focus (GTK_WINDOW (dlg), data->focus);
  } else {
    gtk_window_set_focus (GTK_WINDOW (dlg), data->ok);
  }
  data->win_ptr = get_current_window();
  set_current_window(dlg);
#if 1
  g_idle_add_once ((GSourceOnceFunc) gtk_window_present, dlg);
#else
  gtk_window_present (GTK_WINDOW (dlg));
#endif
}

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

struct input_dialog_data {
  GtkWidget *text;
  string_response_cb cb;
  int *res_btn;
  const struct narray *buttons;
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
input_dialog(GtkWidget *parent, const char *title, const char *mes, const char *init_str, const char *button, const struct narray *buttons, int *res_btn, string_response_cb cb, gpointer user_data)
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
  gtk_window_present(GTK_WINDOW (dlg));
}

struct radio_dialog_data {
  response_cb cb;
  int anum;
  const struct narray *buttons;
  int *res_btn;
  GtkWidget **btn_ary;
  gpointer data;
};

static void
radio_dialog_response(GtkWidget *dlg, int res_id, gpointer user_data)
{
  struct radio_dialog_data *data;
  int selected;
  data = (struct radio_dialog_data *) user_data;
  selected = -1;
  if (res_id > 0 || res_id == GTK_RESPONSE_OK) {
    int i;
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
radio_dialog(GtkWidget *parent, const char *title, const char *caption, struct narray *array, const char *button, const struct narray *buttons, int *res_btn, int selected, response_cb cb, gpointer user_data)
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
  gtk_window_present(GTK_WINDOW (dlg));
}

static int
add_buttons(GtkWidget *dlg, const struct narray *array)
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
button_dialog(GtkWidget *parent, const char *title, const char *caption, const struct narray *buttons, response_cb cb, gpointer user_data)
{
  GtkWidget *dlg;
  struct button_dialog_data *data;

  dlg = gtk_dialog_new();
  if (add_buttons(dlg, buttons)) {
    if (cb) {
      cb(IDCANCEL, user_data);
    }
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
  gtk_window_present(GTK_WINDOW (dlg));
}

struct combo_dialog_data {
  GtkWidget *combo;
  string_response_cb cb;
  int *res_btn;
  char **label;
  const struct narray *buttons;
  gpointer data;
};

static void
combo_dialog_response(GtkWidget *dlg, int response, gpointer user_data)
{
  struct combo_dialog_data *data;
  int r;
  const char *selected;

  data = (struct combo_dialog_data *) user_data;
  selected = NULL;
  r = IDCANCEL;
  if (response > 0 || response == GTK_RESPONSE_OK) {
    int i;
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
combo_dialog(GtkWidget *parent, const char *title, const char *caption, struct narray *array, const struct narray *buttons, int *res_btn, int sel, string_response_cb cb, gpointer user_data)
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
  gtk_window_present(GTK_WINDOW (dlg));
}

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
combo_entry_dialog(GtkWidget *parent, const char *title, const char *caption, struct narray *array, const struct narray *buttons, int *res_btn, int sel, string_response_cb cb, gpointer user_data)
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
  gtk_window_present(GTK_WINDOW (dlg));
}

struct spin_dialog_data {
  GtkWidget *spin;
  response_cb cb;
  int *res_btn;
  double *val;
  const struct narray *buttons;
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
spin_dialog(GtkWidget *parent, const char *title, const char *caption, double min, double max, double inc, const struct narray *buttons, int *res_btn, double *r, response_cb cb, gpointer user_data)
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
  gtk_window_present(GTK_WINDOW (dlg));
}

struct check_dialog_data {
  GtkWidget **btn_ary;
  response_cb cb;
  int *res_btn, anum, *r;
  double *val;
  const struct narray *buttons;
  gpointer data;
};

static void
check_dialog_response(GtkWidget *dialog, int response, gpointer user_data)
{
  int res;
  struct check_dialog_data *data;
  data = (struct check_dialog_data *) user_data;

  res = IDCANCEL;
  if (response > 0 || response == GTK_RESPONSE_OK) {
    int i;
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
check_dialog(GtkWidget *parent, const char *title, const char *caption, struct narray *array, const struct narray *buttons, int *res_btn, int *r, response_cb cb, gpointer user_data)
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
  gtk_window_present(GTK_WINDOW (dlg));
}

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
    g_object_unref (path);
    if (tmp == NULL || strlen(tmp) < 1) {
      g_free(tmp);
      message_beep(TopLevel);
      continue;
    }

    file2 = get_utf8_filename(tmp);
    g_free (tmp);
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

  gtk_window_present(GTK_WINDOW (dlg));
}

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

void
get_window_geometry(GtkWidget *win, gint *x, gint *y, gint *w, gint *h)
{
  GdkSurface *surface;
  /* In Wayland the compositor owns window positioning. */
  surface = gtk_native_get_surface(GTK_NATIVE(win));
  *w = gdk_surface_get_width(surface);
  *h = gdk_surface_get_height(surface);
  *x = 0;
  *y = 0;
}

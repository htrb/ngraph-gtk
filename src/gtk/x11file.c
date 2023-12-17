// -*- coding: utf-8 -*-
/*
 * $Id: x11file.c,v 1.136 2010-03-04 08:30:17 hito Exp $
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

#include "gtk_common.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <math.h>
#include <ctype.h>

#include "gtk_entry_completion.h"
#include "gtk_subwin.h"
#include "gtk_combo.h"
#include "gtk_widget.h"

#include "shell.h"
#include "object.h"
#include "ioutil.h"
#include "nstring.h"
#include "mathfn.h"
#include "gra.h"
#include "spline.h"
#include "nconfig.h"
#include "odata.h"
#include "ofit.h"

#include "math_equation.h"

#include "gtk_columnview.h"
#include "gtk_listview.h"
#include "x11bitmp.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "ogra2cairo.h"
#include "ogra2gdk.h"
#include "ox11menu.h"
#include "x11graph.h"
#include "x11view.h"
#include "x11file.h"
#include "x11commn.h"
#include "sourcecompletionwords.h"
#include "completion_info.h"

static void bind_axis(struct objlist *obj, int id, const char *field, GtkWidget *w);
static void bind_type(struct objlist *obj, int id, const char *field, GtkWidget *w);
static void bind_file (struct objlist *obj, int id, const char *field, GtkWidget *w);

static n_list_store Flist[] = {
  {" ",	        G_TYPE_BOOLEAN, TRUE, FALSE,  "hidden"},
  {"#",		G_TYPE_INT,     FALSE, FALSE, "id"},
  {N_("file/range"),	G_TYPE_STRING, TRUE, TRUE,  "file", bind_file, 0, 0, 0, 0, PANGO_ELLIPSIZE_END},
  {"x   ",	G_TYPE_INT,     TRUE, FALSE,  "x", NULL,  0, 999, 1, 10},
  {"y   ",	G_TYPE_INT,     TRUE, FALSE,  "y", NULL,  0, 999, 1, 10},
  {N_("ax"),	G_TYPE_PARAM,   TRUE, FALSE,  "axis_x", bind_axis},
  {N_("ay"),	G_TYPE_PARAM,   TRUE, FALSE,  "axis_y", bind_axis},
  {N_("type"),	G_TYPE_OBJECT,  TRUE, FALSE,  "type",   bind_type},
  {N_("size"),	G_TYPE_DOUBLE,  TRUE, FALSE,  "mark_size",  NULL, 0,       SPIN_ENTRY_MAX, 100, 1000},
  {N_("width"),	G_TYPE_DOUBLE,  TRUE, FALSE,  "line_width", NULL, 0,       SPIN_ENTRY_MAX, 10,   100},
  {N_("skip"),	G_TYPE_INT,     TRUE, FALSE,  "head_skip",  NULL, 0,       INT_MAX,         1,    10},
  {N_("step"),	G_TYPE_INT,     TRUE, FALSE,  "read_step",  NULL, 1,       INT_MAX,         1,    10},
  {N_("final"),	G_TYPE_INT,     TRUE, FALSE,  "final_line", NULL, INT_MIN, INT_MAX,    1,    10},
  {N_("num"), 	G_TYPE_INT,     FALSE, FALSE, "data_num"},
  {"^#",	G_TYPE_INT,     FALSE, FALSE, "oid"},
};

#define FILE_WIN_COL_NUM G_N_ELEMENTS (Flist)

static void file_delete_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data);
static void file_copy2_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data);
static void file_copy_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data);
static void file_edit_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data);
static void file_draw_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data);
static void FileDialogType(GtkWidget *w, gpointer client_data);
static void func_entry_focused(GtkEventControllerFocus *ev, gpointer user_data);

static GActionEntry Popup_list[] =
{
  {"dataEditAction",            file_edit_popup_func, NULL, NULL, NULL},
  {"dataOrderTopAction",        list_sub_window_move_top, NULL, NULL, NULL},
  {"dataOrderUpAction",         list_sub_window_move_up, NULL, NULL, NULL},
  {"dataOrderDownAction",       list_sub_window_move_down, NULL, NULL, NULL},
  {"dataOrderBottomAction",     list_sub_window_move_last, NULL, NULL, NULL},
  {"dataAddFileAction",         CmFileOpen, NULL, NULL, NULL},
  {"dataAddRangeAction",        CmRangeAdd, NULL, NULL, NULL},

  //  {"_Recent file",      NULL, NULL, NULL, NULL},
  {"dataDuplicateAction",       file_copy_popup_func, NULL, NULL, NULL},
  {"dataDuplicateBefindAction", file_copy2_popup_func, NULL, NULL, NULL},
  {"dataDeleteAction",          file_delete_popup_func, NULL, NULL, NULL},
  {"dataDrawAction",            file_draw_popup_func, NULL, NULL, NULL},
  {"dataUpdateAction",          list_sub_window_update, NULL, NULL, NULL},
  {"dataInstanceNameAction",    list_sub_window_object_name, NULL, NULL, NULL},
};

#define POPUP_ITEM_NUM ((int) (sizeof(Popup_list) / sizeof(*Popup_list)))
#define POPUP_ITEM_EDIT   0
#define POPUP_ITEM_TOP    1
#define POPUP_ITEM_UP     2
#define POPUP_ITEM_DOWN   3
#define POPUP_ITEM_BOTTOM 4
#define POPUP_ITEM_ADD_F  5
#define POPUP_ITEM_ADD_R  6

#define RANGE_ENTRY_WIDTH 26

#define FITSAVE "fit.ngp"

enum MATH_FNC_TYPE {
  TYPE_MATH_X = 0,
  TYPE_MATH_Y,
  TYPE_FUNC_F,
  TYPE_FUNC_G,
  TYPE_FUNC_H,
};

static char *FieldStr[] = {"math_x", "math_y", "func_f", "func_g", "func_h"};

static void
add_completion_provider(GtkSourceView *source_view, GtkSourceCompletionProvider *provider)
{
  GtkSourceCompletion *comp;

  comp = gtk_source_view_get_completion(source_view);
  gtk_source_completion_add_provider(comp, provider);
  g_object_unref(G_OBJECT(provider));
}

static void
add_completion_provider_math(GtkSourceView *source_view)
{
  SourceCompletionWords *words;

  words = source_completion_words_new(_("functions"), completion_info_func_populate);
  add_completion_provider(source_view, GTK_SOURCE_COMPLETION_PROVIDER(words));

  words = source_completion_words_new(_("constants"), completion_info_const_populate);
  add_completion_provider(source_view, GTK_SOURCE_COMPLETION_PROVIDER(words));
}

static GtkWidget *
create_source_view(void)
{
  GtkSourceView *source_view;
  GtkSourceLanguageManager *lm;
  GtkSourceBuffer *buffer;
  GtkSourceLanguage *lang;
  GtkSourceCompletionWords *words;
  GValue value = G_VALUE_INIT;
  GtkSourceCompletion *comp;

  source_view = GTK_SOURCE_VIEW(gtk_source_view_new());
  buffer = gtk_source_buffer_new(NULL);
  gtk_text_view_set_buffer(GTK_TEXT_VIEW(source_view), GTK_TEXT_BUFFER(buffer));
  gtk_text_view_set_monospace(GTK_TEXT_VIEW(source_view), TRUE);
  gtk_source_view_set_tab_width(source_view, 2);
  gtk_source_view_set_insert_spaces_instead_of_tabs(source_view, TRUE);
  gtk_source_view_set_smart_home_end(source_view, GTK_SOURCE_SMART_HOME_END_BEFORE);
  gtk_source_view_set_smart_backspace(source_view, TRUE);
  gtk_source_view_set_indent_width(source_view, -1);
  gtk_source_view_set_indent_on_tab(source_view, TRUE);
  gtk_source_view_set_auto_indent(source_view, TRUE);
  gtk_source_view_set_show_line_numbers(source_view, TRUE);

  comp = gtk_source_view_get_completion(source_view);
  g_value_init(&value, G_TYPE_BOOLEAN);
  g_value_set_boolean(&value, TRUE);
  g_object_set_property(G_OBJECT(comp), "select-on-show", &value);

  lm = gtk_source_language_manager_get_default();
  lang = gtk_source_language_manager_get_language(lm, "ngraph-math");
  gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(buffer), lang);
  gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(buffer), TRUE);

  add_completion_provider_math(source_view);
  words = gtk_source_completion_words_new(_("current equations"));
  gtk_source_completion_words_register(words, GTK_TEXT_BUFFER(buffer));
  add_completion_provider(source_view, GTK_SOURCE_COMPLETION_PROVIDER(words));

  return GTK_WIDGET(source_view);
}

static void
set_source_style(GtkWidget *view)
{
  GtkSourceBuffer *buffer;
  GtkSourceStyleScheme *style;
  GtkSourceStyleSchemeManager *sman;
  const char *style_id;

  if (Menulocal.source_style_id == NULL) {
    return;
  }

  buffer = GTK_SOURCE_BUFFER(gtk_text_view_get_buffer(GTK_TEXT_VIEW(view)));
  style = gtk_source_buffer_get_style_scheme(buffer);
  style_id = gtk_source_style_scheme_get_id(style);
  if (g_strcmp0(Menulocal.source_style_id, style_id) == 0) {
    return;
  }
  sman = gtk_source_style_scheme_manager_get_default();
  style = gtk_source_style_scheme_manager_get_scheme(sman, Menulocal.source_style_id);
  if (style == NULL) {
    return;
  }
  gtk_source_buffer_set_style_scheme(buffer, style);
}

static void
set_text_to_source_buffer(GtkWidget *view, const char *text)
{
  GtkTextBuffer *buffer;
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
  /* This is automatically marked as an irreversible action in the undo stack. */
  gtk_text_buffer_set_text(buffer, text, -1);
}

static void
MathTextDialogChangeInputType(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data)
{
  struct MathTextDialog *d;
  gchar *text;
  GtkTextBuffer *buffer;
  const char *str;

  d = user_data;
  d->page = page_num;
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(d->text));
  switch (page_num) {
  case 0:
    text = get_text_from_buffer(buffer);
    editable_set_init_text(d->list, text);
    g_free(text);
    break;
  case 1:
    str = gtk_editable_get_text(GTK_EDITABLE(d->list));
    set_text_to_source_buffer(d->text, str);
    break;
  }
}

static void
MathTextDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct MathTextDialog *d;
  static char *label[] = {N_("Math X"), N_("Math Y"), "F(X,Y,Z)", "G(X,Y,Z)", "H(X,Y,Z)"};

  d = (struct MathTextDialog *) data;
  if (makewidget) {
    GtkWidget *w, *title, *vbox, *tab, *swin;
    tab = gtk_notebook_new();
    gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tab), GTK_POS_BOTTOM);
    d->input_tab = tab;

    title = gtk_label_new(_("single line"));
    w = create_text_entry(TRUE, TRUE);
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_notebook_append_page(GTK_NOTEBOOK(tab), vbox, title);
    d->list = w;

    title = gtk_label_new(_("multi line"));
    w = create_source_view();
    swin = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(swin, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);
    gtk_notebook_append_page(GTK_NOTEBOOK(tab), swin, title);
    d->text = w;

    g_signal_connect(tab, "switch-page", G_CALLBACK(MathTextDialogChangeInputType), d);

    gtk_box_append(GTK_BOX(d->vbox), tab);
    gtk_window_set_default_size(GTK_WINDOW(wi), 800, 500);
  }

#if USE_ENTRY_COMPLETIONf
  switch (d->Mode) {
  case TYPE_MATH_X:
    entry_completion_set_entry(NgraphApp.x_math_list, d->list);
    break;
  case TYPE_MATH_Y:
    entry_completion_set_entry(NgraphApp.y_math_list, d->list);
    break;
  case TYPE_FUNC_F:
  case TYPE_FUNC_G:
  case TYPE_FUNC_H:
    entry_completion_set_entry(NgraphApp.func_list, d->list);
    break;
  }
#endif

  set_source_style(d->text);
  gtk_window_set_title(GTK_WINDOW(wi), _(label[d->Mode]));
  editable_set_init_text(d->list, d->Text);
  set_text_to_source_buffer(d->text, d->Text);
  gtk_notebook_set_current_page(GTK_NOTEBOOK(d->input_tab), Menulocal.math_input_mode);
  if (Menulocal.math_input_mode) {
    gtk_widget_grab_focus(d->text);
  } else {
    gtk_widget_grab_focus(d->list);
  }
}

static void
move_cursor_to_error_line(GtkWidget *view)
{
  GtkTextIter iter;
  GtkTextBuffer *buffer;
  int ln, ofst;

  math_err_get_recent_error_position(&ln, &ofst);
  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
  gtk_text_buffer_get_iter_at_line_offset(buffer, &iter, ln - 1, ofst - 1);
  gtk_text_buffer_place_cursor(buffer, &iter);
  gtk_text_view_scroll_to_iter(GTK_TEXT_VIEW(view), &iter, 0, FALSE, 0, 0);
}

static int
get_id_from_instance_selection (GtkSelectionModel *gsel, int row)
{
  NgraphInst *ni;
  int id;
  ni = g_list_model_get_item (G_LIST_MODEL (gsel), row);
  if (ni == NULL) {
    return -1;
  }
  id = ni->id;
  g_object_unref (ni);
  return id;
}

static void
MathTextDialogClose(GtkWidget *w, void *data)
{
  struct MathTextDialog *d;
  const char *p;
  char *obuf, *ptr;
  int n, i;
  GtkSelectionModel *gsel;

  d = (struct MathTextDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  case IDCANCEL:
    return;
  default:
    d->ret = IDLOOP;
    return;
  }

  switch (d->page) {
  case 0:
    p = gtk_editable_get_text(GTK_EDITABLE(d->list));
    ptr = g_strdup(p);
    break;
  case 1:
    ptr = get_text_from_buffer(gtk_text_view_get_buffer(GTK_TEXT_VIEW(d->text)));
    break;
  default:
    /* not reached */
    return;
  }
  if (ptr == NULL) {
    return;
  }

  gsel = gtk_column_view_get_model(GTK_COLUMN_VIEW(d->tree));
  n = g_list_model_get_n_items (G_LIST_MODEL (gsel));
  for (i = 0; i < n; i++) {
    int id;
    if (! gtk_selection_model_is_selected (gsel, i)) {
      continue;
    }
    id = get_id_from_instance_selection (gsel, i);

    sgetobjfield(d->Obj, id, FieldStr[d->Mode], NULL, &obuf, FALSE, FALSE, FALSE);
    if (obuf == NULL || strcmp(obuf, ptr)) {
      if (sputobjfield(d->Obj, id, FieldStr[d->Mode], ptr)) {
	g_free(ptr);
	d->ret = IDLOOP;
        switch (d->page) {
        case 0:
          gtk_widget_grab_focus(d->list);
          break;
        case 1:
          gtk_widget_grab_focus(d->text);
          move_cursor_to_error_line(d->text);
          break;
        }
	return;
      }
      set_graph_modified();
      d->modified = TRUE;
    }
    g_free(obuf);
  }

  switch (d->Mode) {
  case TYPE_MATH_X:
    entry_completion_append(NgraphApp.x_math_list, ptr);
    break;
  case TYPE_MATH_Y:
    entry_completion_append(NgraphApp.y_math_list, ptr);
    break;
  case TYPE_FUNC_F:
  case TYPE_FUNC_G:
  case TYPE_FUNC_H:
    entry_completion_append(NgraphApp.func_list, ptr);
    break;
  }
  g_free(ptr);
  Menulocal.math_input_mode = gtk_notebook_get_current_page(GTK_NOTEBOOK(d->input_tab));
}

void
MathTextDialog(struct MathTextDialog *data, char *text, int mode, struct objlist *obj, GtkWidget *tree)
{
  if (mode < 0 || mode >= MATH_FNC_NUM)
    mode = 0;

  data->SetupWindow = MathTextDialogSetup;
  data->CloseWindow = MathTextDialogClose;
  data->tree = tree;
  data->Text = text;
  data->Mode = mode;
  data->modified = FALSE;
  data->Obj = obj;
}

static void
MathDialogSetupItem(GtkWidget *w, struct MathDialog *d)
{
  int i;
  char *math, *field = NULL;

  columnview_clear(d->list);

  if (d->Mode < 0 || d->Mode >= MATH_FNC_NUM)
    d->Mode = 0;

  field = FieldStr[d->Mode];

  for (i = 0; i <= chkobjlastinst(d->Obj); i++) {
    math = NULL;
    getobj(d->Obj, field, i, 0, NULL, &math);
    columnview_append_ngraph_inst(d->list, math, i, d->Obj);
  }

  if (d->Mode >= 0 && d->Mode < MATH_FNC_NUM) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->func[d->Mode]), TRUE);
  }
}

static void
MathDialogMode(GtkWidget *w, gpointer client_data)
{
  struct MathDialog *d;
  int i;

  if (! gtk_check_button_get_active(GTK_CHECK_BUTTON(w)))
    return;

  d = (struct MathDialog *) client_data;

  for (i = 0; i < MATH_FNC_NUM; i++) {
    if (w == d->func[i])
      d->Mode = i;
  }
  MathDialogSetupItem(d->widget, d);
}

struct math_dialog_list_data {
  char *buf;
  struct MathDialog *d;
};

static void
update_selected_item(struct MathDialog *d)
{
  GtkSelectionModel *gsel;
  struct narray array;
  int n, i;

  arrayinit(&array, sizeof(int));
  gsel = gtk_column_view_get_model(GTK_COLUMN_VIEW(d->list));
  n = g_list_model_get_n_items (G_LIST_MODEL (gsel));
  for (i = 0; i < n; i++) {
    if (gtk_selection_model_is_selected (gsel, i)) {
      int id;
      id = get_id_from_instance_selection (gsel, i);
      arrayadd(&array, &id);
    }
  }
  MathDialogSetupItem(d->widget, d);
  for (i = 0; i < n; i++) {
    int id;
    id = get_id_from_instance_selection (gsel, i);
    if (array_find_int(&array, id) < 0) {
      continue;
    }
    gtk_selection_model_select_item (gsel, i, FALSE);
  }
  arraydel(&array);
}

static void
math_dialog_list_respone(struct response_callback *cb)
{
  struct MathDialog *d;
  struct math_dialog_list_data *res_data;

  res_data = (struct math_dialog_list_data *) cb->data;
  d = res_data->d;
  if (DlgMathText.modified) {
    d->modified = DlgMathText.modified;
    update_selected_item(d);
  }
  g_free(res_data->buf);
  g_free(res_data);
}

static void
MathDialogList(GtkButton *w, gpointer client_data)
{
  struct MathDialog *d;
  int a, i;
  guint64 n;
  char *field = NULL, *buf;
  GtkSelectionModel *gsel;
  struct math_dialog_list_data *res_data;
  GtkBitset *selected;

  d = (struct MathDialog *) client_data;

  gsel = gtk_column_view_get_model(GTK_COLUMN_VIEW(d->list));
  selected = gtk_selection_model_get_selection (gsel);
  n = gtk_bitset_get_size(selected);
  if (n < 1) {
    gtk_bitset_unref (selected);
    return;
  }
  i = gtk_bitset_get_nth (selected, n - 1);
  gtk_bitset_unref (selected);
  a = get_id_from_instance_selection (gsel, i);

  if (d->Mode < 0 || d->Mode >= MATH_FNC_NUM)
    d->Mode = 0;

  field = FieldStr[d->Mode];

  sgetobjfield(d->Obj, a, field, NULL, &buf, FALSE, FALSE, FALSE);
  if (buf == NULL) {
    return;
  }

  res_data = g_malloc0(sizeof(*res_data));
  if (res_data == NULL) {
    return;
  }
  res_data->d = d;
  res_data->buf = buf;

  MathTextDialog(&DlgMathText, buf, d->Mode, d->Obj, d->list);
  response_callback_add(&DlgMathText, math_dialog_list_respone, NULL, res_data);
  DialogExecute(d->widget, &DlgMathText);
}

static void
math_dialog_activated_cb(GtkColumnView *view, guint pos, gpointer user_data)
{
  struct MathDialog *d;

  d = (struct MathDialog *) user_data;
  MathDialogList(NULL, d);
}

static void
set_btn_sensitivity_selected_cb (GtkSelectionModel* self, guint position, guint n_items, gpointer user_data)
{
  int n;
  GtkWidget *w;
  GtkBitset *sel;

  w = GTK_WIDGET(user_data);
  sel = gtk_selection_model_get_selection (self);
  n = gtk_bitset_get_size (sel);
  gtk_widget_set_sensitive(w, n > 0);
  gtk_bitset_unref (sel);
}

static void
set_sensitivity_by_selected(GtkWidget *tree, GtkWidget *btn)
{
  GtkSelectionModel *sel;

  if (G_TYPE_CHECK_INSTANCE_TYPE(tree, GTK_TYPE_COLUMN_VIEW)) {
    sel = gtk_column_view_get_model(GTK_COLUMN_VIEW(tree));
  } else if (G_TYPE_CHECK_INSTANCE_TYPE(tree, GTK_TYPE_LIST_VIEW)) {
    sel = gtk_list_view_get_model(GTK_LIST_VIEW(tree));
  } else {
    return;
  }
  g_signal_connect(sel, "selection-changed", G_CALLBACK(set_btn_sensitivity_selected_cb), btn);
  gtk_widget_set_sensitive(btn, FALSE);
}

static void
setup_column (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
  GtkWidget *label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), GPOINTER_TO_INT (user_data) ? 0.0 : 1.0);
  gtk_label_set_ellipsize (GTK_LABEL (label), PANGO_ELLIPSIZE_END);
  gtk_list_item_set_child (list_item, label);
}

static void
bind_column (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
  GtkWidget *label = gtk_list_item_get_child (list_item);
  NgraphInst *item = NGRAPH_INST(gtk_list_item_get_item (list_item));

  if (GPOINTER_TO_INT (user_data)) {
    const char *str;
    char *tmpstr = NULL;
    str = item->name ? item->name : "";
    if (strchr(str, '\n')) {
      tmpstr = g_strescape(str, "\\");
      str = tmpstr;
    }
    gtk_label_set_text(GTK_LABEL(label), str);
    g_free(tmpstr);
  } else {
    char text[20];

    snprintf(text, sizeof(text), "%d", item->id);
    gtk_label_set_text(GTK_LABEL(label), text);
  }
}

static char *
sort_column (NgraphInst *item, gpointer user_data)
{
  return g_strdup (item->name);
}

static int
sort_by_id (NgraphInst *item, gpointer user_data)
{
  return item->id;
}

static void
MathDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct MathDialog *d;

  d = (struct MathDialog *) data;

  if (makewidget) {
    int i;
    GtkWidget *w, *swin, *vbox, *hbox;
    GtkColumnViewColumn *col;
    char *button_str[] = {
      N_("_X math"),
      N_("_Y math"),
      "_F(X, Y, Z)",
      "_G(X, Y, Z)",
      "_H(X, Y, Z)",
    };
    GtkWidget *group = NULL;

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    gtk_box_append(GTK_BOX(vbox), hbox);

    w = columnview_create(NGRAPH_TYPE_INST, N_SELECTION_TYPE_MULTI);
    col = columnview_create_column(w, _("id"), G_CALLBACK(setup_column), G_CALLBACK(bind_column), NULL, GINT_TO_POINTER (0), FALSE);
    columnview_set_numeric_sorter(col, G_TYPE_INT, G_CALLBACK(sort_by_id), NULL);
    columnview_create_column(w, _("math"), G_CALLBACK(setup_column), G_CALLBACK(bind_column), G_CALLBACK(sort_column), GINT_TO_POINTER (1), TRUE);

    g_signal_connect(w, "activate", G_CALLBACK(math_dialog_activated_cb), d);
    d->list = w;

    swin = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(swin, TRUE);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);

    w = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(w), swin);
    gtk_box_append(GTK_BOX(vbox), w);

    w = NULL;
    for (i = 0; i < MATH_FNC_NUM; i++) {
      w = gtk_check_button_new_with_mnemonic(_(button_str[i]));
      if (group) {
	gtk_check_button_set_group(GTK_CHECK_BUTTON(w), GTK_CHECK_BUTTON(group));
      } else {
	group = w;
      }
      gtk_box_append(GTK_BOX(hbox), w);
      d->func[i] = w;
      g_signal_connect(w, "toggled", G_CALLBACK(MathDialogMode), d);
    }

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    w = gtk_button_new_with_mnemonic(_("Select _All"));
    set_button_icon(w, "edit-select-all");
    g_signal_connect_swapped(w, "clicked", G_CALLBACK(columnview_select_all), d->list);
    gtk_box_append(GTK_BOX(hbox), w);

    w = gtk_button_new_with_mnemonic(_("_Edit"));
    g_signal_connect(w, "clicked", G_CALLBACK(MathDialogList), d);
    gtk_box_append(GTK_BOX(hbox), w);
    gtk_box_append(GTK_BOX(vbox), hbox);
    set_sensitivity_by_selected(d->list, w);

    gtk_box_append(GTK_BOX(d->vbox), vbox);

    d->show_cancel = FALSE;
    d->ok_button = _("_Close");

    gtk_window_set_default_size(GTK_WINDOW(wi), -1, 300);

    d->Mode = 0;
  }

  MathDialogSetupItem(wi, d);
}

static void
MathDialogClose(GtkWidget *w, void *data)
{
}

void
MathDialog(struct MathDialog *data, struct objlist *obj)
{
  data->SetupWindow = MathDialogSetup;
  data->CloseWindow = MathDialogClose;
  data->Obj = obj;
  data->modified = FALSE;
}

static void
FitSaveDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct FitSaveDialog *d;
  int i;
  char *s;

  d = (struct FitSaveDialog *) data;
  if (makewidget) {
    GtkWidget *w, *hbox;
    gtk_dialog_add_buttons(GTK_DIALOG(wi),
			   _("_Delete"), IDDELETE,
			   NULL);

    w = combo_box_entry_create();
    combo_box_entry_set_width(w, 20);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    item_setup(hbox, w, _("_Profile:"), TRUE);
    d->profile = w;
    gtk_box_append(GTK_BOX(d->vbox), hbox);
  }
  combo_box_clear(d->profile);
  for (i = d->Sid; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    combo_box_append_text(d->profile, CHK_STR(s));
  }
  combo_box_entry_set_text(d->profile, "");
}

static void
FitSaveDialogClose(GtkWidget *w, void *data)
{
  struct FitSaveDialog *d;
  const char *s;

  d = (struct FitSaveDialog *) data;

  if (d->ret != IDOK && d->ret != IDDELETE)
    return;

  s = combo_box_entry_get_text(d->profile);
  if (s) {
    char *ptr;

    ptr = g_strdup(s);
    g_strstrip(ptr);
    if (ptr[0] != '\0') {
      d->Profile = ptr;
      return;
    }
    g_free(ptr);
  }

  message_box(d->widget, _("Please specify the profile."), NULL, RESPONS_OK);

  d->ret = IDLOOP;
  return;
}

void
FitSaveDialog(struct FitSaveDialog *data, struct objlist *obj, int sid)
{
  data->SetupWindow = FitSaveDialogSetup;
  data->CloseWindow = FitSaveDialogClose;
  data->Obj = obj;
  data->Sid = sid;
  data->Profile = NULL;
}

static void
FitDialogSetupItem(GtkWidget *w, struct FitDialog *d, int id)
{
  int a, i;

  SetWidgetFromObjField(d->type, d->Obj, id, "type");

  getobj(d->Obj, "poly_dimension", id, 0, NULL, &a);
  combo_box_set_active(d->dim, a - 1);

  SetWidgetFromObjField(d->weight, d->Obj, id, "weight_func");

  SetWidgetFromObjField(d->through_point, d->Obj, id, "through_point");

  SetWidgetFromObjField(d->x, d->Obj, id, "point_x");

  SetWidgetFromObjField(d->y, d->Obj, id, "point_y");

  SetWidgetFromObjField(d->min, d->Obj, id, "min");

  SetWidgetFromObjField(d->max, d->Obj, id, "max");

  SetWidgetFromObjField(d->div, d->Obj, id, "div");

  SetWidgetFromObjField(d->interpolation, d->Obj, id, "interpolation");

  SetWidgetFromObjField(d->converge, d->Obj, id, "converge");

  SetWidgetFromObjField(d->derivatives, d->Obj, id, "derivative");

  SetWidgetFromObjField(d->formula, d->Obj, id, "user_func");

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "parameter0", dd[] = "derivative0";

    p[sizeof(p) - 2] += i;
    dd[sizeof(dd) - 2] += i;

    SetWidgetFromObjField(d->p[i], d->Obj, id, p);
    SetWidgetFromObjField(d->d[i], d->Obj, id, dd);
  }
}

static char *
FitCB(struct objlist *obj, int id)
{
  char *valstr, *profile;

  getobj(obj, "profile", id, 0, NULL, &profile);

  valstr = NULL;
  if (profile == NULL) {
    char *tmp;

    sgetobjfield(obj, id, "type", NULL, &tmp, FALSE, FALSE, FALSE);
    if (tmp) {
      valstr = g_strdup(_(tmp));
      g_free(tmp);
    }
  }

  return valstr;
}

static void
fit_dialog_copy_response(int sel, gpointer user_data)
{
  struct FitDialog *d;
  d = (struct FitDialog *) user_data;
  if (sel != -1) {
    FitDialogSetupItem(d->widget, d, sel);
  }
}

static void
FitDialogCopy(GtkButton *btn, gpointer user_data)
{
  struct FitDialog *d;
  d = (struct FitDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, FitCB, fit_dialog_copy_response, d);
}

static int
FitDialogLoadConfig(struct FitDialog *d, int errmes)
{
  int lastid;

  lastid = chkobjlastinst(d->Obj);
  if (lastid == d->Lastid) {
    struct objlist *shell;
    struct narray sarray;
    char *argv[2];
    char *file;
    int newid;
    if ((file = searchscript(FITSAVE)) == NULL) {
      if (errmes)
	message_box(d->widget, _("Setting file not found."), FITSAVE, RESPONS_OK);
      return FALSE;
    }
    if ((shell = chkobject("shell")) == NULL)
      return FALSE;
    newid = newobj(shell);
    if (newid < 0) {
      g_free(file);
      return FALSE;
    }
    arrayinit(&sarray, sizeof(char *));
    changefilename(file);
    if (arrayadd(&sarray, &file) == NULL) {
      g_free(file);
      arraydel2(&sarray);
      return FALSE;
    }
    argv[0] = (char *) &sarray;
    argv[1] = NULL;
    exeobj(shell, "shell", newid, 1, argv);
    arraydel2(&sarray);
    delobj(shell, newid);
  }
  return TRUE;
}

struct copy_settings_to_fitobj_data {
  struct FitDialog *d;
  char *profile;
  int i;
  response_cb cb;
  gpointer data;
};

static void
copy_settings_to_fitobj_response(int ret, gpointer user_data)
{
  struct copy_settings_to_fitobj_data *data;
  struct FitDialog *d;
  char *profile;
  int i, id, num;
  response_cb cb;
  gpointer ud;

  data = (struct copy_settings_to_fitobj_data *) user_data;
  d = data->d;
  profile = data->profile;
  i = data->i;
  cb = data->cb;
  ud = data->data;
  g_free(data);
  if (ret != IDYES) {
    g_free(profile);
    cb(TRUE, ud);
    return;
  }

  if (i > chkobjlastinst(d->Obj)) {
    id = newobj(d->Obj);
  } else {
    id = i;
  }

  if (putobj(d->Obj, "profile", id, profile) == -1) {
    g_free(profile);
    cb(TRUE, ud);
    return;
  }

  if (SetObjFieldFromWidget(d->type, d->Obj, id, "type")) {
    cb(TRUE, ud);
    return;
  }

  num = combo_box_get_active(d->dim);
  if (num < 0) {
    return;
  }
  num++;
  if (num > 0 && putobj(d->Obj, "poly_dimension", id, &num) == -1) {
    cb(TRUE, ud);
    return;
  }

  if (SetObjFieldFromWidget(d->weight, d->Obj, id, "weight_func")) {
    cb(TRUE, ud);
    return;
  }

  if (SetObjFieldFromWidget
      (d->through_point, d->Obj, id, "through_point")) {
    cb(TRUE, ud);
    return;
  }

  if (SetObjFieldFromWidget(d->x, d->Obj, id, "point_x")) {
    cb(TRUE, ud);
    return;
  }

  if (SetObjFieldFromWidget(d->y, d->Obj, id, "point_y")) {
    cb(TRUE, ud);
    return;
  }

  if (SetObjFieldFromWidget(d->min, d->Obj, id, "min")) {
    cb(TRUE, ud);
    return;
  }

  if (SetObjFieldFromWidget(d->max, d->Obj, id, "max")) {
    cb(TRUE, ud);
    return;
  }

  if (SetObjFieldFromWidget(d->div, d->Obj, id, "div")) {
    cb(TRUE, ud);
    return;
  }

  if (SetObjFieldFromWidget(d->interpolation, d->Obj, id,
			    "interpolation")) {
    cb(TRUE, ud);
    return;
  }
  if (SetObjFieldFromWidget(d->formula, d->Obj, id, "user_func")) {
    cb(TRUE, ud);
    return;
  }

  if (SetObjFieldFromWidget(d->derivatives, d->Obj, id, "derivative")) {
    cb(TRUE, ud);
    return;
  }

  if (SetObjFieldFromWidget(d->converge, d->Obj, id, "converge")) {
    cb(TRUE, ud);
    return;
  }

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "parameter0", dd[] = "derivative0";

    p[sizeof(p) - 2] += i;
    dd[sizeof(dd) - 2] += i;

    if (SetObjFieldFromWidget(d->p[i], d->Obj, id, p)) {
      cb(TRUE, ud);
      return;
    }

    if (SetObjFieldFromWidget(d->d[i], d->Obj, id, dd)) {
      cb(TRUE, ud);
      return;
    }
  }

  cb(FALSE, ud);

  return;
}

static void
copy_settings_to_fitobj(struct FitDialog *d, const char *str, response_cb cb, gpointer user_data)
{
  int i;
  char *s, *profile;;
  struct copy_settings_to_fitobj_data *data;

  profile = g_strdup(str);
  if (profile == NULL) {
    return;
  }

  data = g_malloc0(sizeof(*data));
  data->d = d;
  data->profile = profile;
  data->cb = cb;
  data->data = user_data;

  for (i = d->Lastid + 1; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    if (s && strcmp(s, profile) == 0) {
      data->i = i;
      response_message_box(d->widget, _("Overwrite existing profile?"), "Confirm",
			   RESPONS_YESNO, copy_settings_to_fitobj_response, data);
      return;
    }
  }
  data->i = i;
  copy_settings_to_fitobj_response(IDYES, data);
  return;
}

static void
delete_fitobj_response(int ret, gpointer user_data)
{
  struct copy_settings_to_fitobj_data *data;
  struct FitDialog *d;
  char *profile;
  int i;
  response_cb cb;
  gpointer ud;

  data = (struct copy_settings_to_fitobj_data *) user_data;
  d = data->d;
  profile = data->profile;
  i = data->i;
  ud = data->data;
  cb = data->cb;
  g_free(data);

  if (ret != IDYES) {
    g_free(profile);
    cb(TRUE, ud);
    return;
  }

  if (i > chkobjlastinst(d->Obj)) {
    char *ptr;
    ptr = g_strdup_printf(_("The profile '%s' is not exist."), profile);
    message_box(d->widget, ptr, "Confirm", RESPONS_OK);
    g_free(ptr);
    g_free(profile);
    cb(TRUE, ud);
    return;
  }
  delobj(d->Obj, i);
  cb(FALSE, ud);
  g_free(profile);
}

static void
delete_fitobj(struct FitDialog *d, const char *str, response_cb cb, gpointer user_data)
{
  int i;
  char *s, *ptr, *profile;
  struct copy_settings_to_fitobj_data *data;

  profile = g_strdup(str);
  if (profile == NULL)
    return;

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    g_free(profile);
    return;
  }
  data->d = d;
  data->profile = profile;
  data->cb = cb;
  data->data = user_data;

  for (i = d->Lastid + 1; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    if (s && strcmp(s, profile) == 0) {
      data->i = i;
      ptr = g_strdup_printf(_("Delete the profile '%s'?"), profile);
      response_message_box(d->widget, ptr, "Confirm", RESPONS_YESNO,
			   delete_fitobj_response, data);
      g_free(ptr);
      return;
    }
  }
  data->i = i;
  delete_fitobj_response(IDYES, data);
  return;
}

struct fit_dialog_save_response_data {
  struct FitDialog *d;
  int return_value;
  char *profile;
};

static void
fit_dialog_save_response_response(int ret, gpointer user_data)
{
  char *s, *ngpfile, *profile;
  struct FitDialog *d;
  int return_value, error, hFile;
  struct fit_dialog_save_response_data *data;

  data = (struct fit_dialog_save_response_data *) user_data;
  d = data->d;
  profile = data->profile;
  return_value = data->return_value;
  g_free(data);

  if (ret) {
    g_free(profile);
    return;
  }

  ngpfile = getscriptname(FITSAVE);
  if (ngpfile == NULL) {
    g_free(profile);
    return;
  }

  error = FALSE;

  hFile = nopen(ngpfile, O_CREAT | O_TRUNC | O_RDWR, NFMODE_NORMAL_FILE);
  if (hFile < 0) {
    error = TRUE;
  } else {
    int i;
    for (i = d->Lastid + 1; i <= chkobjlastinst(d->Obj); i++) {
      int len;
      getobj(d->Obj, "save", i, 0, NULL, &s);
      len = strlen(s);

      if (len != nwrite(hFile, s, len))
	error = TRUE;

      if (nwrite(hFile, "\n", 1) != 1)
	error = TRUE;
    }
    nclose(hFile);
  }

  if (error) {
    ErrorMessage();
  } else {
    char *ptr;
    switch (return_value) {
    case IDOK:
      ptr = g_strdup_printf(_("The profile '%s' is saved."), profile);
      message_box(d->widget, ptr, "Confirm", RESPONS_OK);
      g_free(ptr);
      break;
    case IDDELETE:
      ptr = g_strdup_printf(_("The profile '%s' is deleted."), profile);
      message_box(d->widget, ptr, "Confirm", RESPONS_OK);
      g_free(ptr);
      break;
    }
  }

  g_free(profile);
  g_free(ngpfile);
}

static void
fit_dialog_save_response(struct response_callback *cb)
{
  struct FitDialog *d;
  struct fit_dialog_save_response_data *data;

  d = (struct FitDialog *) cb->data;
  if (cb->return_value != IDOK && cb->return_value != IDDELETE)
    return;

  if (DlgFitSave.Profile == NULL)
    return;

  if (DlgFitSave.Profile[0] == '\0') {
    g_free(DlgFitSave.Profile);
    return;
  }

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  data->d = d;
  data->return_value = cb->return_value;
  data->profile = DlgFitSave.Profile;
  DlgFitSave.Profile = NULL;
  switch (cb->return_value) {
  case IDOK:
    copy_settings_to_fitobj(d, data->profile, fit_dialog_save_response_response, data);
    return;
  case IDDELETE:
    delete_fitobj(d, data->profile, fit_dialog_save_response_response, data);
    return;
  }
  fit_dialog_save_response_response(FALSE, data);
}

static void
FitDialogSave(GtkWidget *w, gpointer client_data)
{
  struct FitDialog *d;

  d = (struct FitDialog *) client_data;

  if (!FitDialogLoadConfig(d, FALSE))
    return;

  FitSaveDialog(&DlgFitSave, d->Obj, d->Lastid + 1);

  response_callback_add(&DlgFitSave, fit_dialog_save_response, NULL, d);
  DialogExecute(d->widget, &DlgFitSave);
}

static int
check_fit_func(GtkEditable *w, gpointer client_data)
{
  struct FitDialog *d;
  MathEquation *code;
  MathEquationParametar *prm;
  const char *math;
  int dim, i, deriv;

  d = (struct FitDialog *) client_data;

  code = math_equation_basic_new();
  if (code == NULL)
    return FALSE;

  if (math_equation_add_parameter(code, 0, 1, 2, MATH_EQUATION_PARAMETAR_USE_ID)) {
    math_equation_free(code);
    return FALSE;
  }

  math = gtk_editable_get_text(GTK_EDITABLE(d->formula));
  if (math_equation_parse(code, math)) {
    math_equation_free(code);
    return FALSE;
  }

  prm = math_equation_get_parameter(code, 0, NULL);
  dim = prm->id_num;
  deriv = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->derivatives));

  for (i = 0; i < FIT_PARM_NUM; i++) {
    set_widget_sensitivity_with_label(d->p[i], FALSE);
    set_widget_sensitivity_with_label(d->d[i], FALSE);
  }

  for (i = 0; i < dim; i++) {
    int n;
    n = prm->id[i];

    if (n < FIT_PARM_NUM) {
      set_widget_sensitivity_with_label(d->p[n], TRUE);
      if (deriv) {
	set_widget_sensitivity_with_label(d->d[n], TRUE);
      }
    }
  }

  math_equation_free(code);

  return TRUE;
}

static void
FitDialogResult(GtkWidget *w, gpointer client_data)
{
  struct FitDialog *d;
  double derror, correlation, coe[FIT_PARM_NUM];
  char *equation, *math;
  N_VALUE *inst;
  int i, j, dim, dimension, type, num;
  GString *buf;

  d = (struct FitDialog *) client_data;

  if ((inst = chkobjinst(d->Obj, d->Id)) == NULL)
    return;

  if (_getobj(d->Obj, "type", inst, &type))
    return;

  if (_getobj(d->Obj, "poly_dimension", inst, &dimension))
    return;

  if (_getobj(d->Obj, "number", inst, &num))
    return;

  if (_getobj(d->Obj, "error", inst, &derror))
    return;

  if (_getobj(d->Obj, "correlation", inst, &correlation))
    return;

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "%00";

    p[sizeof(p) - 2] += i;

    if (_getobj(d->Obj, p, inst, coe + i)) {
      return;
    }
  }

  if (_getobj(d->Obj, "equation", inst, &equation))
    return;

  if (_getobj(d->Obj, "user_func", inst, &math))
    return;

  buf = g_string_new("<tt>");
  if (buf == NULL) {
    return;
  }

  if (equation == NULL) {
    g_string_append_printf(buf, "Undefined");
  } else if (type != FIT_TYPE_USER) {
    if (type == FIT_TYPE_POLY) {
      dim = dimension + 1;
    } else {
      dim = 2;
    }

    switch (type) {
    case FIT_TYPE_POLY:
      g_string_append_printf(buf, "Eq: %%0i*X<sup>i</sup> (i=0-%d)\n\n", dim - 1);
      break;
    case FIT_TYPE_POW:
      g_string_append_printf(buf, "Eq: exp(%%00)*X<sup>%%01</sup>\n\n");
      break;
    case FIT_TYPE_EXP:
      g_string_append_printf(buf, "Eq: exp(%%01*X+%%00)\n\n");
      break;
    case FIT_TYPE_LOG:
      g_string_append_printf(buf, "Eq: %%01*Ln(X)+%%00\n\n");
      break;
    }

    for (j = 0; j < dim; j++) {
      g_string_append_printf(buf, "       %%0%d = %+.7e\n", j, coe[j]);
    }
    g_string_append_printf(buf, "\n");
    g_string_append_printf(buf, "    points = %d\n", num);
    g_string_append_printf(buf, "    &lt;DY<sup>2</sup>&gt; = %.7e\n", derror);

    if (correlation >= 0) {
      g_string_append_printf(buf, "|r| or |R| = %.7e\n", correlation);
    } else {
      g_string_append_printf(buf, "|r| or |R| = -------------");
    }
  } else {
    int tbl[FIT_PARM_NUM];
    MathEquation *code;
    MathEquationParametar *prm;

    code = math_equation_basic_new();
    if (code == NULL)
      return;

    if (math_equation_add_parameter(code, 0, 1, 2, MATH_EQUATION_PARAMETAR_USE_ID)) {
      math_equation_free(code);
      return;
    }

    if (math_equation_parse(code, math)) {
      math_equation_free(code);
      return;
    }
    prm = math_equation_get_parameter(code, 0, NULL);
    dim = prm->id_num;
    for (i = 0; i < dim; i++) {
      tbl[i] = prm->id[i];
    }
    math_equation_free(code);
    g_string_append_printf(buf, "Eq: User defined\n\n");

    for (j = 0; j < dim; j++) {
      g_string_append_printf(buf, "       %%0%d = %+.7e\n", tbl[j], coe[tbl[j]]);
    }
    g_string_append_printf(buf, "\n");
    g_string_append_printf(buf, "    points = %d\n", num);
    g_string_append_printf(buf, "    &lt;DY<sup>2</sup>&gt; = %.7e\n", derror);

    if (correlation >= 0) {
      g_string_append_printf(buf, "|r| or |R| = %.7e\n", correlation);
    } else {
      g_string_append_printf(buf, "|r| or |R| = -------------");
    }
  }
  g_string_append(buf, "</tt>");
  markup_message_box(d->widget, buf->str, _("Fitting Results"), RESPONS_OK, TRUE);
  g_string_free(buf, TRUE);
}

static int
FitDialogApply(GtkWidget *w, struct FitDialog *d)
{
  int i, num, dim;
  const gchar *s;

  if (SetObjFieldFromWidget(d->type, d->Obj, d->Id, "type"))
    return FALSE;

  if (getobj(d->Obj, "poly_dimension", d->Id, 0, NULL, &dim) == -1)
    return FALSE;

  num = combo_box_get_active(d->dim);
  if (num < 0) {
    return FALSE;
  }
  num++;
  if (num > 0 && putobj(d->Obj, "poly_dimension", d->Id, &num) == -1)
    return FALSE;

  if (num != dim)
    set_graph_modified();

  if (SetObjFieldFromWidget(d->weight, d->Obj, d->Id, "weight_func"))
    return FALSE;

  if (SetObjFieldFromWidget(d->through_point, d->Obj, d->Id, "through_point"))
    return FALSE;

  if (SetObjFieldFromWidget(d->x, d->Obj, d->Id, "point_x"))
    return FALSE;

  if (SetObjFieldFromWidget(d->y, d->Obj, d->Id, "point_y"))
    return FALSE;

  if (SetObjFieldFromWidget(d->min, d->Obj, d->Id, "min"))
    return FALSE;

  if (SetObjFieldFromWidget(d->max, d->Obj, d->Id, "max"))
    return FALSE;

  if (SetObjFieldFromWidget(d->div, d->Obj, d->Id, "div"))
    return FALSE;

  if (SetObjFieldFromWidget(d->interpolation, d->Obj, d->Id, "interpolation"))
    return FALSE;

  if (SetObjFieldFromWidget(d->derivatives, d->Obj, d->Id, "derivative"))
    return FALSE;

  if (SetObjFieldFromWidget(d->converge, d->Obj, d->Id, "converge"))
    return FALSE;

  if (SetObjFieldFromWidget(d->formula, d->Obj, d->Id, "user_func"))
    return FALSE;

  s = gtk_editable_get_text(GTK_EDITABLE(d->formula));
  entry_completion_append(NgraphApp.fit_list, s);

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "parameter0", dd[] = "derivative0";

    p[sizeof(p) - 2] += i;
    dd[sizeof(dd) - 2] += i;

    if (SetObjFieldFromWidget(d->p[i], d->Obj, d->Id, p))
      return FALSE;

    if (SetObjFieldFromWidget(d->d[i], d->Obj, d->Id, dd))
      return FALSE;

    s = gtk_editable_get_text(GTK_EDITABLE(d->d[i]));
    entry_completion_append(NgraphApp.fit_list, s);
  }

  return TRUE;
}

static void
FitDialogDraw(GtkWidget *w, gpointer client_data)
{
  struct FitDialog *d;

  d = (struct FitDialog *) client_data;
  if (!FitDialogApply(d->widget, d))
    return;
  FitDialogSetupItem(d->widget, d, d->Id);
  Draw(FALSE, NULL, NULL);
}

static void
set_user_fit_sensitivity(struct FitDialog *d, int active)
{
  int i;

  for (i = 0; i < FIT_PARM_NUM; i++) {
    set_widget_sensitivity_with_label(d->d[i], active);
  }
}

static void
set_fitdialog_sensitivity(struct FitDialog *d, int type, int through)
{
  int i;

  set_user_fit_sensitivity(d, FALSE);
  for (i = 0; i < FIT_PARM_NUM; i++) {
    set_widget_sensitivity_with_label(d->p[i], FALSE);
  }

  set_widget_sensitivity_with_label(d->dim, type == FIT_TYPE_POLY);
  gtk_widget_set_sensitive(d->usr_def_frame, FALSE);
  gtk_widget_set_sensitive(d->usr_def_prm_tbl, FALSE);
  gtk_widget_set_sensitive(d->through_box, through);
  gtk_widget_set_sensitive(d->through_point, TRUE);
}

static void
FitDialogSetSensitivity(GtkWidget *widget, gpointer user_data)
{
  struct FitDialog *d;
  int type, through, deriv, dim;

  d = (struct FitDialog *) user_data;

  type = combo_box_get_active(d->type);
  through = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->through_point));

  switch (type) {
  case FIT_TYPE_POLY:
    dim = combo_box_get_active(d->dim);
    if (dim == 0) {
      gtk_label_set_markup(GTK_LABEL(d->func_label), "Equation: Y=<i>a·</i>X<i>+b</i>");
    } else if (dim > 0) {
      char buf[1024];
      snprintf(buf, sizeof(buf), "Equation: Y=<i>∑ a<sub>i</sub>·</i>X<sup><i>i</i></sup> (<i>i=0-%d</i>)", dim + 1);
      gtk_label_set_markup(GTK_LABEL(d->func_label), buf);
    }
    set_fitdialog_sensitivity(d, type, through);
    break;
  case FIT_TYPE_POW:
    gtk_label_set_markup(GTK_LABEL(d->func_label), "Equation: Y=<i>a·</i>X<i><sup>b</sup></i>");
    set_fitdialog_sensitivity(d, type, through);
    break;
  case FIT_TYPE_EXP:
    gtk_label_set_markup(GTK_LABEL(d->func_label), "Equation: Y=<i>e</i><sup><i>(a·</i>X<i>+b)</i></sup>");
    set_fitdialog_sensitivity(d, type, through);
    break;
  case FIT_TYPE_LOG:
    gtk_label_set_markup(GTK_LABEL(d->func_label), "Equation: Y=<i>a·Ln(</i>X<i>)+b</i>");
    set_fitdialog_sensitivity(d, type, through);
    break;
  case FIT_TYPE_USER:
    gtk_label_set_text(GTK_LABEL(d->func_label), "");
    deriv = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->derivatives));

    set_widget_sensitivity_with_label(d->dim, FALSE);
    gtk_widget_set_sensitive(d->through_point, FALSE);
    gtk_widget_set_sensitive(d->through_box, FALSE);
    gtk_widget_set_sensitive(d->usr_def_frame, TRUE);
    gtk_widget_set_sensitive(d->usr_def_prm_tbl, TRUE);
    set_user_fit_sensitivity(d, deriv);
    check_fit_func(NULL, d);
    break;
  }
}

static void
add_focus_in_event(GtkWidget *w, gpointer user_data)
{
  GtkEventController *ev;
  ev = gtk_event_controller_focus_new();
  g_signal_connect(ev, "enter", G_CALLBACK(func_entry_focused), user_data);
  gtk_widget_add_controller(w, ev);
}

static GtkWidget *
create_user_fit_frame(struct FitDialog *d)
{
  GtkWidget *table, *w, *vbox;
  int i, j;

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  table = gtk_grid_new();

  j = 0;
  w = create_text_entry(FALSE, TRUE);
  add_widget_to_table_sub(table, w, _("_Formula:"), TRUE, 0, 2, 3, j++);
  add_focus_in_event(w, NgraphApp.fit_list);
  g_signal_connect(w, "changed", G_CALLBACK(check_fit_func), d);
  d->formula = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table_sub(table, w, _("_Converge (%):"), TRUE, 0, 1, 3, j);
  d->converge = w;

  w = gtk_check_button_new_with_mnemonic(_("_Derivatives"));
  add_widget_to_table_sub(table, w, NULL, FALSE, 2, 1, 3, j++);
  d->derivatives = w;

  gtk_box_append(GTK_BOX(vbox), table);

  table = gtk_grid_new();

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "%0_0:", dd[] = "dF/d(%0_0):";

    p[sizeof(p) - 3] += i;
    dd[sizeof(dd) - 4] += i;

    w = create_text_entry(TRUE, TRUE);
    add_widget_to_table_sub(table, w, p, TRUE, 0, 1, 4, j);
    d->p[i] = w;

    w = create_text_entry(TRUE, TRUE);
    add_focus_in_event(w, NgraphApp.fit_list);
    add_widget_to_table_sub(table, w, dd, TRUE, 2, 1, 4, j++);
    d->d[i] = w;
  }

  w = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(w, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w), table);
  gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(w), FALSE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(w), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(GTK_WIDGET(w), -1, 200);

  gtk_box_append(GTK_BOX(vbox), w);
  d->usr_def_prm_tbl = table;

  w = gtk_frame_new(_("User definition"));
  gtk_frame_set_child(GTK_FRAME(w), vbox);

  return w;
}

static void
fit_type_notify(GtkWidget *w, GParamSpec* pspec, gpointer user_data)
{
  FitDialogSetSensitivity(w, user_data);
}

static void
select_fit_item_cb(GtkWidget *list_view, guint position, gpointer user_data)
{
  struct FitDialog *d;
  int id;
  GtkWidget *popover;

  d = (struct FitDialog *) user_data;

  popover = widget_get_grandparent(list_view);
  if (G_TYPE_CHECK_INSTANCE_TYPE(popover, GTK_TYPE_POPOVER)) {
    gtk_popover_popdown(GTK_POPOVER(popover));
  }

  id = position + d->Lastid + 1;
  FitDialogSetupItem(d->widget, d, id);
}

static void
fit_load_menu_show_cb(GtkWidget *popover, gpointer user_data)
{
  struct FitDialog *d;
  GtkWidget *list_view;
  GtkStringList *list;
  int i, lastid, n;
  char *s;

  d = (struct FitDialog *) user_data;

  list_view = gtk_popover_get_child (GTK_POPOVER (popover));
  listview_clear (list_view);

  if (!FitDialogLoadConfig(d, TRUE)) {
    return;
  }

  lastid = chkobjlastinst(d->Obj);
  if ((d->Lastid < 0) || (lastid == d->Lastid)) {
    return;
  }

  list = listview_get_string_list (list_view);
  n = chkobjlastinst(d->Obj);
  for (i = d->Lastid + 1; i <= n; i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    gtk_string_list_append(list, CHK_STR(s));
  }
}

static void
fit_load_button_setup(GtkWidget *menu_button, struct FitDialog *d)
{
  GtkWidget *popover, *menu;

  menu = listview_create(N_SELECTION_TYPE_SINGLE, NULL, NULL, NULL);
  gtk_list_view_set_single_click_activate (GTK_LIST_VIEW (menu), TRUE);
  g_signal_connect(menu, "activate", G_CALLBACK(select_fit_item_cb), d);

  popover = gtk_popover_new();
  gtk_popover_set_child(GTK_POPOVER (popover), menu);
  g_signal_connect(popover, "show", G_CALLBACK(fit_load_menu_show_cb), d);

  gtk_menu_button_set_popover (GTK_MENU_BUTTON (menu_button), popover);
}
static void
FitDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct FitDialog *d;
  char title[20];

  d = (struct FitDialog *) data;
  snprintf(title, sizeof(title), _("Fit %d"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    char **enumlist, mes[10];
    GtkWidget *w, *hbox, *hbox2, *vbox, *frame, *table;
    int i;

    gtk_dialog_add_button(GTK_DIALOG(wi), _("_Delete"), IDDELETE);

    table = gtk_grid_new();

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    w = combo_box_create();
    add_widget_to_table_sub(table, w, _("_Type:"), FALSE, 0, 1, 5, 0);
    d->type = w;
    enumlist = (char **) chkobjarglist(d->Obj, "type");
    for (i = 0; enumlist[i] && enumlist[i][0]; i++) {
      combo_box_append_text(d->type, _(enumlist[i]));
    }

    w = combo_box_create();
    add_widget_to_table_sub(table, w, _("_Dim:"), FALSE, 2, 1, 5, 0);
    d->dim = w;
    for (i = 0; i < FIT_PARM_NUM - 1; i++) {
      snprintf(mes, sizeof(mes), "%d", i + 1);
      combo_box_append_text(d->dim, mes);
    }

    w = gtk_label_new("");
    add_widget_to_table_sub(table, w, NULL, TRUE, 4, 1, 5, 0);
    gtk_widget_set_halign(w, GTK_ALIGN_START);
    d->func_label = w;

    w = create_text_entry(TRUE, TRUE);
    add_widget_to_table_sub(table, w, _("_Weight:"), TRUE, 0, 4, 5, 1);
    d->weight = w;

    gtk_box_append(GTK_BOX(vbox), table);


    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Through"));
    gtk_box_append(GTK_BOX(hbox), w);
    d->through_point = w;

    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox2, w, "_X:", TRUE);
    d->x = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox2, w, "_Y:", TRUE);
    d->y = w;

    d->through_box = hbox2;

    gtk_box_append(GTK_BOX(hbox), hbox2);
    gtk_box_append(GTK_BOX(vbox), hbox);


    frame = gtk_frame_new(_("Action"));
    gtk_frame_set_child(GTK_FRAME(frame), vbox);
    gtk_box_append(GTK_BOX(d->vbox), frame);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Min:"), TRUE);
    d->min = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox, w, _("_Max:"), TRUE);
    d->max = w;

    w = create_spin_entry(1, 65535, 1, TRUE, TRUE);
    item_setup(hbox, w, _("_Div:"), FALSE);
    d->div = w;

    w = gtk_check_button_new_with_mnemonic(_("_Interpolation"));
    gtk_box_append(GTK_BOX(hbox), w);
    d->interpolation = w;

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_append(GTK_BOX(vbox), hbox);

    frame = gtk_frame_new(_("Draw X range"));
    gtk_frame_set_child(GTK_FRAME(frame), vbox);
    gtk_box_append(GTK_BOX(d->vbox), frame);


    frame = create_user_fit_frame(d);
    d->usr_def_frame = frame;
    gtk_box_append(GTK_BOX(d->vbox), frame);


    hbox = add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(FitDialogCopy), d, "fit");

    w = gtk_menu_button_new();
    gtk_menu_button_set_use_underline (GTK_MENU_BUTTON (w), TRUE);
    gtk_menu_button_set_label(GTK_MENU_BUTTON(w), _("_Load"));
    fit_load_button_setup(w, d);
    gtk_box_append(GTK_BOX(hbox), w);

    w = gtk_button_new_with_mnemonic(_("_Save"));
    set_button_icon(w, "document-save");
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogSave), d);
    gtk_box_append(GTK_BOX(hbox), w);


    w = gtk_button_new_with_mnemonic(_("_Draw"));
    gtk_box_append(GTK_BOX(hbox), w);
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogDraw), d);

    w = gtk_button_new_with_mnemonic(_("_Result"));
    gtk_box_append(GTK_BOX(hbox), w);
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogResult), d);


    g_signal_connect(d->dim, "notify::selected", G_CALLBACK(fit_type_notify), d);
    g_signal_connect(d->type, "notify::selected", G_CALLBACK(fit_type_notify), d);
    g_signal_connect(d->through_point, "toggled", G_CALLBACK(FitDialogSetSensitivity), d);
    g_signal_connect(d->derivatives, "toggled", G_CALLBACK(FitDialogSetSensitivity), d);
  }

  FitDialogSetupItem(wi, d, d->Id);
}

static void
FitDialogClose(GtkWidget *w, void *data)
{
  struct FitDialog *d;
  int ret;
  int i, lastid;

  d = (struct FitDialog *) data;
  switch (d->ret) {
  case IDOK:
    break;
  case IDDELETE:
    break;
  case IDCANCEL:
    break;
  default:
    d->ret = IDLOOP;
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;
  if (ret == IDOK && ! FitDialogApply(w, d)) {
    return;
  }
  d->ret = ret;
  lastid = chkobjlastinst(d->Obj);
  for (i = lastid; i > d->Lastid; i--) {
    delobj(d->Obj, i);
  }
}

void
FitDialog(struct FitDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FitDialogSetup;
  data->CloseWindow = FitDialogClose;
  data->Obj = obj;
  data->Id = id;
  data->Lastid = chkobjlastinst(obj);
}

static void
move_tab_setup_item(struct FileDialog *d, int id)
{
  unsigned int j, movenum;
  struct narray *move, *movex, *movey;
  GListStore *list;

  columnview_clear(d->move.list);

  exeobj(d->Obj, "move_data_adjust", id, 0, NULL);
  getobj(d->Obj, "move_data", id, 0, NULL, &move);
  getobj(d->Obj, "move_data_x", id, 0, NULL, &movex);
  getobj(d->Obj, "move_data_y", id, 0, NULL, &movey);

  movenum = arraynum(move);

  if (arraynum(movex) < movenum) {
    movenum = arraynum(movex);
  }

  if (arraynum(movey) < movenum) {
    movenum = arraynum(movey);
  }

  if (movenum < 1) {
    return;
  }
  list = columnview_get_list(d->move.list);
  for (j = 0; j < movenum; j++) {
    int line;
    double x, y;
    line = arraynget_int(move, j);
    x = arraynget_double(movex, j);
    y = arraynget_double(movey, j);

    list_store_append_ngraph_data(list, d->Id, line, x, y);
  }
}

static void
FileMoveDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  int a;
  double x, y;
  const char *buf;
  char *endptr;
  GListStore *list;

  d = (struct FileDialog *) client_data;

  a = spin_entry_get_val(d->move.line);

  buf = gtk_editable_get_text(GTK_EDITABLE(d->move.x));
  if (buf[0] == '\0') return;

  x = strtod(buf, &endptr);
  if (x != x || x == HUGE_VAL || x == - HUGE_VAL || endptr[0] != '\0')
    return;

  buf = gtk_editable_get_text(GTK_EDITABLE(d->move.y));
  if (buf[0] == '\0') return;

  y = strtod(buf, &endptr);
  if (y != y || y == HUGE_VAL || y == - HUGE_VAL || endptr[0] != '\0')
    return;

  list = columnview_get_list (d->move.list);
  list_store_append_ngraph_data(list, d->Id, a, x, y);

  editable_set_init_text(d->move.x, "");
  editable_set_init_text(d->move.y, "");
  d->move.changed = TRUE;
}

static void
FileMoveDialogRemove(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) client_data;

  columnview_remove_selected(d->move.list);
  d->move.changed = TRUE;
}

static void
move_tab_copy_response(int sel, gpointer user_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) user_data;
  if (sel != -1) {
    move_tab_setup_item(d, sel);
    d->move.changed = TRUE;
  }
}

static void
move_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, FileCB, move_tab_copy_response, d);
}

void
spin_button_set_activates_signal(GtkWidget *w, GCallback cb, gpointer user_data)
{
  GtkEditable *editable;
  editable = gtk_editable_get_delegate(GTK_EDITABLE(w));
  g_signal_connect(editable, "activate", cb, user_data);
}

static void
move_setup_column (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
  GtkWidget *label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_list_item_set_child (list_item, label);
}

static void
move_bind_column (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
  GtkWidget *label = gtk_list_item_get_child (list_item);
  NgraphData *item = NGRAPH_DATA(gtk_list_item_get_item (list_item));
  char buf[64];

  switch (GPOINTER_TO_INT (user_data)) {
  case 'L':
    snprintf(buf, sizeof(buf), "%d", item->line);
    break;
  case 'X':
    snprintf(buf, sizeof(buf), DOUBLE_STR_FORMAT, item->x);
    break;
  case 'Y':
    snprintf(buf, sizeof(buf), DOUBLE_STR_FORMAT, item->y);
    break;
  }
  gtk_label_set_text(GTK_LABEL(label), buf);
}

static int
sort_by_line (NgraphData *item, gpointer user_data)
{
  return item->line;
}

static double
sort_by_data (NgraphData *item, gpointer user_data)
{
  int col = GPOINTER_TO_INT (user_data);
  switch (col) {
  case 'X':
    return item->x;
    break;
  case 'Y':
    return item->y;
    break;
  }
  return 0.0;
}

static GtkWidget *
move_tab_create(struct FileDialog *d)
{
  GtkWidget *w, *hbox, *swin, *table, *vbox;
  GtkColumnViewColumn *col;
  int i;

  swin = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(swin, TRUE);
  gtk_widget_set_hexpand(swin, TRUE);
  gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(swin), TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);

  w = columnview_create(NGRAPH_TYPE_DATA, N_SELECTION_TYPE_MULTI);
  col = columnview_create_column(w, _("Line No."), G_CALLBACK(move_setup_column), G_CALLBACK(move_bind_column), NULL, GINT_TO_POINTER ('L'), FALSE);
  columnview_set_numeric_sorter(col, G_TYPE_INT, G_CALLBACK(sort_by_line), NULL);
  col = columnview_create_column(w, "X", G_CALLBACK(move_setup_column), G_CALLBACK(move_bind_column), NULL, GINT_TO_POINTER ('X'), FALSE);
  columnview_set_numeric_sorter(col, G_TYPE_DOUBLE, G_CALLBACK(sort_by_data), GINT_TO_POINTER('X'));
  col = columnview_create_column(w, "Y", G_CALLBACK(move_setup_column), G_CALLBACK(move_bind_column), NULL, GINT_TO_POINTER ('Y'), TRUE);
  columnview_set_numeric_sorter(col, G_TYPE_DOUBLE, G_CALLBACK(sort_by_data), GINT_TO_POINTER('Y'));

  d->move.list = w;
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);
  set_widget_margin(swin, WIDGET_MARGIN_TOP | WIDGET_MARGIN_BOTTOM);

  table = gtk_grid_new();

  i = 0;
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_NATURAL, TRUE, FALSE);
  spin_button_set_activates_signal(w, G_CALLBACK(FileMoveDialogAdd), d);
  add_widget_to_table(table, w, _("_Line:"), FALSE, i++);
  d->move.line = w;

  w = create_text_entry(TRUE, FALSE);
  g_signal_connect(w, "activate", G_CALLBACK(FileMoveDialogAdd), d);
  add_widget_to_table(table, w, "_X:", FALSE, i++);
  d->move.x = w;

  w = create_text_entry(TRUE, FALSE);
  g_signal_connect(w, "activate", G_CALLBACK(FileMoveDialogAdd), d);
  add_widget_to_table(table, w, "_Y:", FALSE, i++);
  d->move.y = w;

  w = gtk_button_new_with_mnemonic(_("_Add"));
  set_button_icon(w, "list-add");
  add_widget_to_table(table, w, "", FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(FileMoveDialogAdd), d);

  w = gtk_button_new_with_mnemonic(_("_Remove"));
  set_button_icon(w, "list-remove");
  add_widget_to_table(table, w, NULL, FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(FileMoveDialogRemove), d);
  set_sensitivity_by_selected(d->move.list, w);

  w = gtk_button_new_with_mnemonic(_("Select _All"));
  set_button_icon(w, "edit-select-all");
  add_widget_to_table(table, w, NULL, FALSE, i++);
  g_signal_connect_swapped(w, "clicked", G_CALLBACK(columnview_select_all), d->move.list);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_append(GTK_BOX(hbox), table);
  gtk_box_append(GTK_BOX(hbox), swin);

  w = gtk_frame_new(NULL);
  gtk_frame_set_child(GTK_FRAME(w), hbox);
  set_widget_margin(w, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append(GTK_BOX(vbox), w);

  add_copy_button_to_box(vbox, G_CALLBACK(move_tab_copy), d, "data");

  return vbox;
}

static int
move_tab_set_value(struct FileDialog *d)
{
  int line, a, i, n;
  struct narray *move, *movex, *movey;
  GListStore *list;

  if (d->move.changed == FALSE) {
    return 0;
  }

  set_graph_modified();
  exeobj(d->Obj, "move_data_adjust", d->Id, 0, NULL);
  getobj(d->Obj, "move_data", d->Id, 0, NULL, &move);
  getobj(d->Obj, "move_data_x", d->Id, 0, NULL, &movex);
  getobj(d->Obj, "move_data_y", d->Id, 0, NULL, &movey);
  if (move) {
    putobj(d->Obj, "move_data", d->Id, NULL);
    move = NULL;
  }
  if (movex) {
    putobj(d->Obj, "move_data_x", d->Id, NULL);
    movex = NULL;
  }
  if (movey) {
    putobj(d->Obj, "move_data_y", d->Id, NULL);
    movey = NULL;
  }

  list = columnview_get_list(d->move.list);
  n = g_list_model_get_n_items(G_LIST_MODEL (list));
  for (i = 0; i < n; i++) {
    unsigned int movenum, j;
    NgraphData *ndata;
    ndata = g_list_model_get_item (G_LIST_MODEL (list), i);
    a = ndata->line;

    if (move == NULL)
      move = arraynew(sizeof(int));

    if (movex == NULL)
      movex = arraynew(sizeof(double));

    if (movey == NULL)
      movey = arraynew(sizeof(double));

    movenum = arraynum(move);
    if (arraynum(movex) < movenum)
      movenum = arraynum(movex);

    if (arraynum(movey) < movenum)
      movenum = arraynum(movey);

    for (j = 0; j < movenum; j++) {
      line = arraynget_int(move, j);

      if (line == a)
	break;
    }
    if (j == movenum) {
      arrayadd(move, &a);
      arrayadd(movex, &ndata->x);
      arrayadd(movey, &ndata->y);
    }
    g_object_unref (ndata);
  }

  putobj(d->Obj, "move_data", d->Id, move);
  putobj(d->Obj, "move_data_x", d->Id, movex);
  putobj(d->Obj, "move_data_y", d->Id, movey);

  return 0;
}

static void
mask_tab_setup_item(struct FileDialog *d, int id)
{
  int masknum;
  struct narray *mask;
  GListStore *list;

  list = columnview_get_list(d->mask.list);
  g_list_store_remove_all (list);
  getobj(d->Obj, "mask", id, 0, NULL, &mask);
  if ((masknum = arraynum(mask)) > 0) {
    int j;
    for (j = 0; j < masknum; j++) {
      int line;
      line = arraynget_int(mask, j);
      list_store_append_ngraph_data(list, d->Id, line, 0, 0);
    }
  }
}

static void
FileMaskDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  int a;
  GListStore *list;

  d = (struct FileDialog *) client_data;

  list = columnview_get_list(d->mask.list);
  a = spin_entry_get_val(d->mask.line);
  list_store_append_ngraph_data(list, d->Id, a, 0, 0);
  d->mask.changed = TRUE;
}

static void
mask_tab_copy_response(int sel, gpointer user_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) user_data;
  if (sel != -1) {
    mask_tab_setup_item(d, sel);
    d->mask.changed = TRUE;
  }
}

static void
mask_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, FileCB, mask_tab_copy_response, d);
}


static void
FileMaskDialogRemove(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) client_data;

  columnview_remove_selected(d->mask.list);
  d->mask.changed = TRUE;
}

static void
mask_setup_column (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
  GtkWidget *label = gtk_label_new (NULL);
  gtk_label_set_xalign (GTK_LABEL (label), 1.0);
  gtk_list_item_set_child (list_item, label);
}

static void
mask_bind_column (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data) {
  char buf[64];
  GtkWidget *label = gtk_list_item_get_child (list_item);
  NgraphData *item = NGRAPH_DATA(gtk_list_item_get_item (list_item));
  snprintf(buf, sizeof(buf), "%d", item->line);
  gtk_label_set_text(GTK_LABEL(label), buf);
}

static GtkWidget *
mask_tab_create(struct FileDialog *d)
{
  GtkWidget *w, *swin, *hbox, *table, *vbox, *frame;
  GtkColumnViewColumn *col;
  int i;

  table = gtk_grid_new();

  i = 0;
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_NATURAL, TRUE, FALSE);
  spin_button_set_activates_signal(w, G_CALLBACK(FileMaskDialogAdd), d);
  add_widget_to_table(table, w, _("_Line:"), FALSE, i++);
  d->mask.line = w;

  swin = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(swin, TRUE);
  gtk_widget_set_hexpand(swin, TRUE);
  gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(swin), TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  w = columnview_create(NGRAPH_TYPE_DATA, N_SELECTION_TYPE_MULTI);
  col = columnview_create_column(w, _("Line No."), G_CALLBACK(mask_setup_column), G_CALLBACK(mask_bind_column), NULL, NULL, TRUE);
  columnview_set_numeric_sorter(col, G_TYPE_INT, G_CALLBACK(sort_by_line), NULL);

  d->mask.list = w;
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);
  set_widget_margin(swin, WIDGET_MARGIN_TOP | WIDGET_MARGIN_BOTTOM);

  w = gtk_button_new_with_mnemonic(_("_Add"));
  set_button_icon(w, "list-add");
  add_widget_to_table(table, w, "", FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(FileMaskDialogAdd), d);

  w = gtk_button_new_with_mnemonic(_("_Remove"));
  set_button_icon(w, "list-remove");
  add_widget_to_table(table, w, NULL, FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(FileMaskDialogRemove), d);
  set_sensitivity_by_selected(d->mask.list, w);

  w = gtk_button_new_with_mnemonic(_("Select _All"));
  set_button_icon(w, "edit-select-all");
  add_widget_to_table(table, w, NULL, FALSE, i++);
  g_signal_connect_swapped(w, "clicked", G_CALLBACK(columnview_select_all), d->mask.list);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_append(GTK_BOX(hbox), table);
  gtk_box_append(GTK_BOX(hbox), swin);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_child(GTK_FRAME(frame), hbox);
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append(GTK_BOX(vbox), frame);

  add_copy_button_to_box(vbox, G_CALLBACK(mask_tab_copy), d, "data");

  return vbox;
}

static int
mask_tab_set_value(struct FileDialog *d)
{
  int n, i;
  struct narray *mask;
  GListModel *list;

  if (d->mask.changed == FALSE) {
    return 0;
  }

  getobj(d->Obj, "mask", d->Id, 0, NULL, &mask);
  if (mask) {
    putobj(d->Obj, "mask", d->Id, NULL);
    mask = NULL;
  }

  list = G_LIST_MODEL (gtk_list_view_get_model (GTK_LIST_VIEW (d->mask.list)));
  n = g_list_model_get_n_items (list);
  for (i = 0; i < n; i++) {
    GObject *item;
    const char *str;
    int a;
    item = g_list_model_get_object (list, i);
    str = gtk_string_object_get_string (GTK_STRING_OBJECT (item));
    if (mask == NULL)
      mask = arraynew(sizeof(int));

    a = atoi (str);
    arrayadd(mask, &a);
  }
  putobj(d->Obj, "mask", d->Id, mask);
  set_graph_modified();

  return 0;
}

static void
load_tab_setup_item(struct FileDialog *d, int id)
{
  char *ifs, *s;
  unsigned int i, j, l;

  SetWidgetFromObjField(d->load.headskip, d->Obj, id, "head_skip");
  SetWidgetFromObjField(d->load.readstep, d->Obj, id, "read_step");
  SetWidgetFromObjField(d->load.finalline, d->Obj, id, "final_line");
  if (d->source != DATA_SOURCE_FILE) {
    return;
  }

  SetWidgetFromObjField(d->load.remark, d->Obj, id, "remark");
  SetWidgetFromObjField(d->load.csv, d->Obj, id, "csv");
  sgetobjfield(d->Obj, id, "ifs", NULL, &ifs, FALSE, FALSE, FALSE);
  if (ifs == NULL) {
    return;
  }

  l = strlen(ifs);
  s = g_malloc(l * 2 + 1);
  if (s == NULL) {
    g_free(ifs);
    return;
  }
  j = 0;
  for (i = 0; i < l; i++) {
    if (ifs[i] == '\t') {
      s[j++] = '\\';
      s[j++] = 't';
    } else if (ifs[i] == '\\') {
      s[j++] = '\\';
      s[j++] = '\\';
    } else {
      s[j++] = ifs[i];
    }
  }
  s[j] = '\0';
  editable_set_init_text(d->load.ifs, s);
  g_free(s);
  g_free(ifs);
}

static void
load_tab_copy_response(int sel, gpointer user_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) user_data;
  if (sel != -1) {
    load_tab_setup_item(d, sel);
  }
}

static void
load_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, FileCB, load_tab_copy_response, d);
}

static GtkWidget *
load_tab_create(struct FileDialog *d)
{
  GtkWidget *w, *table, *frame, *vbox;
  int i;

  table = gtk_grid_new();

  i = 0;
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_UINT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Head skip:"), FALSE, i++);
  d->load.headskip = w;

  w = create_spin_entry(1, INT_MAX, 1, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Read step:"), FALSE, i++);
  d->load.readstep = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_INT, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Final line:"), FALSE, i++);
  d->load.finalline = w;

  if (d->source == DATA_SOURCE_FILE) {
    w = create_text_entry(TRUE, TRUE);
    add_widget_to_table(table, w, _("_Remark:"), TRUE, i++);
    d->load.remark = w;

    w = create_text_entry(TRUE, TRUE);
    add_widget_to_table(table, w, _("_IFS:"), TRUE, i++);
    d->load.ifs = w;

    w = gtk_check_button_new_with_mnemonic(_("_CSV"));
    add_widget_to_table(table, w, NULL, TRUE, i++);
    d->load.csv = w;
  }

  frame = gtk_frame_new(NULL);
  gtk_frame_set_child(GTK_FRAME(frame), table);
  gtk_widget_set_vexpand(frame, TRUE);
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append(GTK_BOX(vbox), frame);

  add_copy_button_to_box(vbox, G_CALLBACK(load_tab_copy), d, "data");

  return vbox;
}

static void
decode_ifs_text(GString *s, const char *ifs)
{
  int i, l;

  l = strlen(ifs);
  for (i = 0; i < l; i++) {
    if ((ifs[i] == '\\') && (ifs[i + 1] == 't')) {
      g_string_append_c(s, 0x09);
      i++;
    } else if (ifs[i] == '\\') {
      g_string_append_c(s, '\\');
      i++;
    } else if (isascii(ifs[i])) {
      g_string_append_c(s, ifs[i]);
    }
  }
}

static int
load_tab_set_value(struct FileDialog *d)
{
  const char *ifs;
  char *obuf;
  GString *s;

  if (SetObjFieldFromWidget(d->load.headskip, d->Obj, d->Id, "head_skip"))
    return 1;

  if (SetObjFieldFromWidget(d->load.readstep, d->Obj, d->Id, "read_step"))
    return 1;

  if (SetObjFieldFromWidget(d->load.finalline, d->Obj, d->Id, "final_line"))
    return 1;

  if (d->source != DATA_SOURCE_FILE) {
    return 0;
  }

  if (SetObjFieldFromWidget(d->load.remark, d->Obj, d->Id, "remark"))
    return 1;

  ifs = gtk_editable_get_text(GTK_EDITABLE(d->load.ifs));
  s = g_string_new("");
  decode_ifs_text(s, ifs);

  sgetobjfield(d->Obj, d->Id, "ifs", NULL, &obuf, FALSE, FALSE, FALSE);
  if (obuf == NULL || strcmp(s->str, obuf)) {
    if (sputobjfield(d->Obj, d->Id, "ifs", s->str) != 0) {
      g_free(obuf);
      g_string_free(s, TRUE);
      return 1;
    }
    set_graph_modified();
  }

  g_free(obuf);
  g_string_free(s, TRUE);

  if (SetObjFieldFromWidget(d->load.csv, d->Obj, d->Id, "csv"))
    return 1;

  return 0;
}

static void
copy_text_to_entry(GtkWidget *text, GtkWidget *entry)
{
  GtkTextBuffer *buffer;
  gchar *str;

  buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(text));
  str = get_text_from_buffer(buffer);
  editable_set_init_text(entry, str);
  g_free(str);
}

static void
copy_entry_to_text(GtkWidget *text, GtkWidget *entry)
{
  const gchar *str;

  str = gtk_editable_get_text(GTK_EDITABLE(entry));
  set_text_to_source_buffer(text, str);
}

static void
copy_text_to_entry_all(struct FileDialog *d)
{
  copy_text_to_entry(d->math.text_x, d->math.x);
  copy_text_to_entry(d->math.text_y, d->math.y);
  copy_text_to_entry(d->math.text_f, d->math.f);
  copy_text_to_entry(d->math.text_g, d->math.g);
  copy_text_to_entry(d->math.text_h, d->math.h);
}

static void
copy_entry_to_text_all(struct FileDialog *d)
{
  copy_entry_to_text(d->math.text_x, d->math.x);
  copy_entry_to_text(d->math.text_y, d->math.y);
  copy_entry_to_text(d->math.text_f, d->math.f);
  copy_entry_to_text(d->math.text_g, d->math.g);
  copy_entry_to_text(d->math.text_h, d->math.h);
}

static void
math_tab_setup_item(struct FileDialog *d, int id)
{
  SetWidgetFromObjField(d->math.xsmooth, d->Obj, id, "smooth_x");
  SetWidgetFromObjField(d->math.ysmooth, d->Obj, id, "smooth_y");
  SetWidgetFromObjField(d->math.averaging_type, d->Obj, id, "averaging_type");
  SetWidgetFromObjField(d->math.x, d->Obj, id, "math_x");
  SetWidgetFromObjField(d->math.y, d->Obj, id, "math_y");
  SetWidgetFromObjField(d->math.f, d->Obj, id, "func_f");
  SetWidgetFromObjField(d->math.g, d->Obj, id, "func_g");
  SetWidgetFromObjField(d->math.h, d->Obj, id, "func_h");
  copy_entry_to_text_all(d);

  entry_completion_set_entry(NgraphApp.x_math_list, d->math.x);
  entry_completion_set_entry(NgraphApp.y_math_list, d->math.y);
  set_source_style(d->math.text_x);
  set_source_style(d->math.text_y);
  set_source_style(d->math.text_f);
  set_source_style(d->math.text_g);
  set_source_style(d->math.text_h);
}

static void
math_tab_copy_response(int sel, gpointer user_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) user_data;
  if (sel != -1) {
    math_tab_setup_item(d, sel);
  }
}

static void
math_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, FileCB, math_tab_copy_response, d);
}

static void
func_entry_focused(GtkEventControllerFocus *ev, gpointer user_data)
{
#if USE_ENTRY_COMPLETIONf
  GtkEntryCompletion *compl;
  GtkWidget *w;

  compl = GTK_ENTRY_COMPLETION(user_data);
  w = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(ev));
  entry_completion_set_entry(compl, w);
#endif
}

static GtkWidget *
create_math_text_tab(GtkWidget *tab, const gchar *label)
{
  GtkWidget *w, *title, *swin;

  w = create_source_view();
  swin = gtk_scrolled_window_new();
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);
  title = gtk_label_new_with_mnemonic(label);
  gtk_notebook_append_page(GTK_NOTEBOOK(tab), swin, title);
  return w;
}

static void
MathDialogChangeInputType(GtkNotebook *notebook, GtkWidget *page, guint page_num, gpointer user_data)
{
  struct FileDialog *d;

  d = user_data;
  d->math_page = page_num;

  switch (page_num) {
  case 0:
    copy_text_to_entry_all(d);
    break;
  case 1:
    copy_entry_to_text_all(d);
    break;
  }
}

static GtkWidget *
math_text_widgets_create(struct FileDialog *d)
{
  GtkWidget *tab;

  tab = gtk_notebook_new();
  d->math_tab = GTK_NOTEBOOK(tab);
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tab), GTK_POS_LEFT);
  d->math.text_x = create_math_text_tab(tab, _("_X math:"));
  d->math.text_y = create_math_text_tab(tab, _("_Y math:"));
  d->math.text_f = create_math_text_tab(tab, "_F(X,Y,Z):");
  d->math.text_g = create_math_text_tab(tab, "_G(X,Y,Z):");
  d->math.text_h = create_math_text_tab(tab, "_H(X,Y,Z):");

  return tab;
}

static int
math_common_widgets_create(struct FileDialog *d, GtkWidget *grid, int pos)
{
  GtkWidget *w, *tab, *title, *table;
  int i;

  tab = gtk_notebook_new();
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(tab), GTK_POS_BOTTOM);
  gtk_widget_set_vexpand(tab, TRUE);
  d->math_input_tab = tab;

  table = gtk_grid_new();
  i = 0;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_X math:"), TRUE, i++);
  d->math.x = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table(table, w, _("_Y math:"), TRUE, i++);
  d->math.y = w;

  w = create_text_entry(TRUE, TRUE);
  add_focus_in_event(w, NgraphApp.func_list);
  add_widget_to_table(table, w, "_F(X,Y,Z):", TRUE, i++);
  d->math.f = w;

  w = create_text_entry(TRUE, TRUE);
  add_focus_in_event(w, NgraphApp.func_list);
  add_widget_to_table(table, w, "_G(X,Y,Z):", TRUE, i++);
  d->math.g = w;

  w = create_text_entry(TRUE, TRUE);
  add_focus_in_event(w, NgraphApp.func_list);
  add_widget_to_table(table, w, "_H(X,Y,Z):", TRUE, i++);
  d->math.h = w;

  title = gtk_label_new(_("single line"));
  gtk_notebook_append_page(GTK_NOTEBOOK(tab), table, title);

  title = gtk_label_new(_("multi line"));
  w = math_text_widgets_create(d);
  gtk_notebook_append_page(GTK_NOTEBOOK(tab), w, title);
  g_signal_connect(tab, "switch-page", G_CALLBACK(MathDialogChangeInputType), d);

  add_widget_to_table_sub(grid, tab, NULL, TRUE, 0, 4, 2, pos++);
  return pos;
}

static GtkWidget *
math_tab_create(struct FileDialog *d)
{
  GtkWidget *table, *w, *vbox, *frame;
  int i;

  table = gtk_grid_new();

  i = 0;
  w = create_spin_entry(0, FILE_OBJ_SMOOTH_MAX, 1, FALSE, TRUE);
  gtk_widget_set_halign (w, GTK_ALIGN_START);
  add_widget_to_table(table, w, _("_X smooth:"), FALSE, i);
  d->math.xsmooth = w;

  w = combo_box_create();
  gtk_widget_set_hexpand(w, TRUE);
  add_widget_to_table_sub(table, w, _("_Averaging type:"), FALSE, 2, 1, 2, i++);
  d->math.averaging_type = w;

  w = create_spin_entry(0, FILE_OBJ_SMOOTH_MAX, 1, FALSE, TRUE);
  gtk_widget_set_halign (w, GTK_ALIGN_START);
  add_widget_to_table(table, w, _("_Y smooth:"), FALSE, i++);
  d->math.ysmooth = w;

  math_common_widgets_create(d, table, i);

  frame = gtk_frame_new(NULL);
  gtk_frame_set_child(GTK_FRAME(frame), table);
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append(GTK_BOX(vbox), frame);

  add_copy_button_to_box(vbox, G_CALLBACK(math_tab_copy), d, "data");

  return vbox;
}

enum MATH_ERROR_FIELD {
  MATH_ERROR_FIELD_NONE,
  MATH_ERROR_FIELD_X,
  MATH_ERROR_FIELD_Y,
  MATH_ERROR_FIELD_F,
  MATH_ERROR_FIELD_G,
  MATH_ERROR_FIELD_H,
  MATH_ERROR_FIELD_SX,
  MATH_ERROR_FIELD_SY,
};

static int
math_set_value_common(struct FileDialog *d)
{
  const char *s;

  if (d->math_page == 1) {
    copy_text_to_entry_all(d);
  }

  if (SetObjFieldFromWidget(d->math.x, d->Obj, d->Id, "math_x"))
    return MATH_ERROR_FIELD_X;

  if (SetObjFieldFromWidget(d->math.y, d->Obj, d->Id, "math_y"))
    return MATH_ERROR_FIELD_Y;

  if (SetObjFieldFromWidget(d->math.f, d->Obj, d->Id, "func_f"))
    return MATH_ERROR_FIELD_F;

  if (SetObjFieldFromWidget(d->math.g, d->Obj, d->Id, "func_g"))
    return MATH_ERROR_FIELD_G;

  if (SetObjFieldFromWidget(d->math.h, d->Obj, d->Id, "func_h"))
    return MATH_ERROR_FIELD_H;

  s = gtk_editable_get_text(GTK_EDITABLE(d->math.y));
  entry_completion_append(NgraphApp.y_math_list, s);

  s = gtk_editable_get_text(GTK_EDITABLE(d->math.x));
  entry_completion_append(NgraphApp.x_math_list, s);

  s = gtk_editable_get_text(GTK_EDITABLE(d->math.f));
  entry_completion_append(NgraphApp.func_list, s);

  s = gtk_editable_get_text(GTK_EDITABLE(d->math.g));
  entry_completion_append(NgraphApp.func_list, s);

  s = gtk_editable_get_text(GTK_EDITABLE(d->math.h));
  entry_completion_append(NgraphApp.func_list, s);

  return MATH_ERROR_FIELD_NONE;
}

static int
math_tab_set_value(void *data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) data;

  if (SetObjFieldFromWidget(d->math.xsmooth, d->Obj, d->Id, "smooth_x"))
    return MATH_ERROR_FIELD_SX;

  if (SetObjFieldFromWidget(d->math.ysmooth, d->Obj, d->Id, "smooth_y"))
    return MATH_ERROR_FIELD_SY;

  if (SetObjFieldFromWidget(d->math.averaging_type, d->Obj, d->Id, "averaging_type"))
    return MATH_ERROR_FIELD_SX;

  return math_set_value_common(d);
}

static void
MarkDialogCB(GtkWidget *w, gpointer client_data)
{
  int i;
  struct MarkDialog *d;

  d = (struct MarkDialog *) client_data;

  if (! d->cb_respond)
    return;

  for (i = 0; i < MARK_TYPE_NUM; i++) {
    if (w == d->toggle[i])
      break;
  }

  d->Type = i;
  d->ret = IDOK;
  gtk_dialog_response(GTK_DIALOG(d->widget), GTK_RESPONSE_OK);
}

void
button_set_mark_image(GtkWidget *w, int type)
{
  GtkWidget *img;
  char buf[128];
  if (type < 0 || type >= MARK_TYPE_NUM) {
    type = 0;
  }

  snprintf(buf, sizeof(buf), "ngraph_mark%02d-symbolic", type);
  img = gtk_image_new_from_icon_name(buf);
  gtk_image_set_icon_size(GTK_IMAGE(img), Menulocal.icon_size);
  gtk_button_set_child(GTK_BUTTON(w), img);
  snprintf(buf, sizeof(buf), "%02d", type);
  gtk_widget_set_tooltip_text(w, buf);
}

static void
MarkDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct MarkDialog *d;
  int type;
#define COL 10

  d = (struct MarkDialog *) data;

  if (makewidget) {
    GtkWidget *w, *grid;
    grid = gtk_grid_new();
    gtk_grid_set_column_spacing(GTK_GRID(grid), 8);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 8);
    gtk_widget_set_margin_end(grid, 4);
    gtk_widget_set_margin_start(grid, 4);
    for (type = 0; type < MARK_TYPE_NUM; type++) {
      w = gtk_toggle_button_new();
      button_set_mark_image(w, type);
      g_signal_connect(w, "clicked", G_CALLBACK(MarkDialogCB), d);
      d->toggle[type] = w;
      gtk_grid_attach(GTK_GRID(grid), w, type % COL, type / COL, 1, 1);
    }
    gtk_box_append(GTK_BOX(d->vbox), grid);
  }

  d->cb_respond = FALSE;
  for (type = 0; type < MARK_TYPE_NUM; type++) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->toggle[type]), FALSE);
  }
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->toggle[d->Type]), TRUE);
  d->focus = d->toggle[d->Type];
  d->cb_respond = TRUE;
}

static void
MarkDialogClose(GtkWidget *w, void *data)
{
}

void
MarkDialog(struct MarkDialog *data, GtkWidget *parent, int type)
{
  if (type < 0 || type >= MARK_TYPE_NUM) {
    type = 0;
  }
  data->SetupWindow = MarkDialogSetup;
  data->CloseWindow = MarkDialogClose;
  data->Type = type;
  data->parent = parent;
}

static void
file_setup_item(struct FileDialog *d, int id)
{
  if (d->source != DATA_SOURCE_RANGE) {
    SetWidgetFromObjField(d->xcol, d->Obj, id, "x");
    SetWidgetFromObjField(d->ycol, d->Obj, id, "y");
  }
  axis_combo_box_setup(d->xaxis, d->Obj, id, "axis_x");
  axis_combo_box_setup(d->yaxis, d->Obj, id, "axis_y");
}

static void
set_fit_button_label(GtkWidget *btn, const char *str)
{
  char buf[128];

  if (str && str[0] != '\0') {
    snprintf(buf, sizeof(buf), "Fit:%s", str);
  } else {
    snprintf(buf, sizeof(buf), _("Create"));
  }
  gtk_button_set_label(GTK_BUTTON(btn), buf);
}

static void
plot_tab_setup_item(struct FileDialog *d, int id)
{
  int a;

  SetWidgetFromObjField(d->type, d->Obj, id, "type");

  SetWidgetFromObjField(d->curve, d->Obj, id, "interpolation");

  getobj(d->Obj, "mark_type", id, 0, NULL, &a);
  button_set_mark_image(d->mark_btn, a);
  MarkDialog(&(d->mark), d->widget, a);

  SetWidgetFromObjField(d->size, d->Obj, id, "mark_size");

  SetWidgetFromObjField(d->width, d->Obj, id, "line_width");

  SetStyleFromObjField(d->style, d->Obj, id, "line_style");

  SetWidgetFromObjField(d->join, d->Obj, id, "line_join");

  SetWidgetFromObjField(d->miter, d->Obj, id, "line_miter_limit");

  SetWidgetFromObjField(d->clip, d->Obj, id, "data_clip");

  set_color(d->col1, d->Obj, id, NULL);
  set_color2(d->col2, d->Obj, id);

  FileDialogType(d->type, d);
}

static void
FileDialogSetupItem(GtkWidget *w, struct FileDialog *d)
{
  char *valstr;

  plot_tab_setup_item(d, d->Id);
  math_tab_setup_item(d, d->Id);
  load_tab_setup_item(d, d->Id);
  mask_tab_setup_item(d, d->Id);
  move_tab_setup_item(d, d->Id);
  file_setup_item(d, d->Id);

  switch (d->source) {
  case DATA_SOURCE_FILE:
    SetWidgetFromObjField(d->file, d->Obj, d->Id, "file");
    gtk_editable_set_position(GTK_EDITABLE(d->file), -1);
    break;
  case DATA_SOURCE_ARRAY:
    SetWidgetFromObjField(d->file, d->Obj, d->Id, "array");
    break;
  case DATA_SOURCE_RANGE:
    SetWidgetFromObjField(d->min, d->Obj, d->Id, "range_min");
    SetWidgetFromObjField(d->max, d->Obj, d->Id, "range_max");
    SetWidgetFromObjField(d->div, d->Obj, d->Id, "range_div");
    break;
  }

  sgetobjfield(d->Obj, d->Id, "fit", NULL, &valstr, FALSE, FALSE, FALSE);
  if (valstr) {
    int i;
    for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
    if (valstr[i] == ':') {
      i++;
    }
    set_fit_button_label(d->fit, valstr + i);
    g_free(valstr);
  }

  gtk_widget_set_sensitive(d->apply_all, d->multi_open);
  gtk_notebook_set_current_page(GTK_NOTEBOOK(d->math_input_tab), Menulocal.math_input_mode);
}

struct file_dialog_mark_data {
  struct FileDialog *d;
  GtkWidget *w;
};

static void
file_dialog_mark_response(struct response_callback *cb)
{
  struct file_dialog_mark_data *data;
  struct FileDialog *d;
  GtkWidget *w;
  data = (struct file_dialog_mark_data *) cb->data;
  d = data->d;
  w = data->w;
  button_set_mark_image(w, d->mark.Type);
  g_free(data);
}

static void
FileDialogMark(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  struct file_dialog_mark_data *data;

  d = (struct FileDialog *) client_data;
  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  data->d = d;
  data->w = w;
  response_callback_add(&d->mark, file_dialog_mark_response, NULL, data);
  DialogExecute(d->widget, &(d->mark));
}

struct execute_fit_dialog_data {
  struct objlist *fileobj;
  int fileid;
  int save_type;
  response_cb cb;
  gpointer user_data;
};

static void
execute_fit_dialog_response(struct response_callback *cb)
{
  struct execute_fit_dialog_data *data;
  struct objlist *fileobj;
  int fileid, save_type;
  data = (struct execute_fit_dialog_data *) cb->data;
  fileobj = data->fileobj;
  fileid = data->fileid;
  save_type = data->save_type;
  putobj(fileobj, "type", fileid, &save_type);
  if (data->cb) {
    data->cb(cb->return_value, data->user_data);
  }
  g_free(data);
}

static void
execute_fit_dialog(GtkWidget *w, struct objlist *fileobj, int fileid, struct objlist *fitobj, int fitid, response_cb cb, gpointer user_data)
{
  int save_type, type;
  struct execute_fit_dialog_data *data;

  type = PLOT_TYPE_FIT;
  getobj(fileobj, "type", fileid, 0, NULL, &save_type);
  putobj(fileobj, "type", fileid, &type);

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  data->fileobj = fileobj;
  data->fileid = fileid;
  data->save_type = save_type;
  data->cb = cb;
  data->user_data = user_data;
  FitDialog(&DlgFit, fitobj, fitid);
  response_callback_add(&DlgFit, execute_fit_dialog_response, NULL, data);
  DialogExecute(w, &DlgFit);
}

struct show_fit_dialog_data
{
  struct objlist *obj, *fitobj;
  int id, fitid, create, undo;
  response_cb cb;
  gpointer data;
};

static void
show_fit_dialog_response(int ret, gpointer user_data)
{
  struct show_fit_dialog_data *data;
  struct objlist *obj, *fitobj;
  int id, fitid, create;

  data = (struct show_fit_dialog_data *) user_data;
  obj = data->obj;
  id = data->id;
  fitobj = data->fitobj;
  fitid = data->fitid;
  create = data->create;
  switch (ret) {
  case IDCANCEL:
    if (! create)
      break;
    /* fall through */
  case IDDELETE:
    delobj(fitobj, fitid);
    putobj(obj, "fit", id, NULL);
    if (! create)
      set_graph_modified();
    break;
  case IDOK:
    if (create)
      set_graph_modified();
    break;
  }
  if (data->cb) {
    data->cb(ret, data->data);
  }
  g_free(data);
}

static void
call_cb(int response, response_cb cb, gpointer user_data)
{
  if (cb) {
    cb(response, user_data);
  }
}

static void
show_fit_dialog(struct objlist *obj, int id, GtkWidget *parent, response_cb cb, gpointer user_data)
{
  struct objlist *fitobj, *robj;
  char *fit;
  int fitid = 0, fitoid, create = FALSE;
  struct narray iarray;
  struct show_fit_dialog_data *data;

  if ((fitobj = chkobject("fit")) == NULL) {
    call_cb(-1, cb, user_data);
    return;
  }

  if (getobj(obj, "fit", id, 0, NULL, &fit) == -1) {
    call_cb(-1, cb, user_data);
    return;
  }

  if (fit) {
    int idnum;
    arrayinit(&iarray, sizeof(int));
    if (getobjilist(fit, &robj, &iarray, FALSE, NULL)) {
      call_cb(-1, cb, user_data);
      return;
    }

    idnum = arraynum(&iarray);
    if ((robj != fitobj) || (idnum < 1)) {
      if (putobj(obj, "fit", id, NULL) == -1) {
	arraydel(&iarray);
	call_cb(-1, cb, user_data);
	return;
      }
    } else {
      fitid = arraylast_int(&iarray);
    }
    arraydel(&iarray);
  }

  if (fit == NULL) {
    N_VALUE *inst;
    fitid = newobj(fitobj);
    inst = getobjinst(fitobj, fitid);

    _getobj(fitobj, "oid", inst, &fitoid);

    if ((fit = mkobjlist(fitobj, NULL, fitoid, NULL, TRUE)) == NULL) {
      call_cb(-1, cb, user_data);
      return;
    }

    if (putobj(obj, "fit", id, fit) == -1) {
      g_free(fit);
      call_cb(-1, cb, user_data);
      return;
    }
    create = TRUE;
  }

  data = g_malloc0(sizeof(*data));
  data->fitobj = fitobj;
  data->obj = obj;
  data->fitid = fitid;
  data->id = id;
  data->create = create;
  data->cb = cb;
  data->data = user_data;
  execute_fit_dialog(parent, obj, id, fitobj, fitid, show_fit_dialog_response, data);
}

static void
fit_dialog_fit_response(int ret, gpointer user_data)
{
  struct FileDialog *d;
  char *valstr;
  d = (struct FileDialog *) user_data;
  sgetobjfield(d->Obj, d->Id, "fit", NULL, &valstr, FALSE, FALSE, FALSE);
  if (valstr) {
    int i;
    for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
    if (valstr[i] == ':')
      i++;
    set_fit_button_label(d->fit, valstr + i);
    g_free(valstr);
  }
}

static void
FileDialogFit(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) client_data;
  show_fit_dialog(d->Obj, d->Id, d->widget, fit_dialog_fit_response, client_data);
}

static void
plot_tab_copy_response(int sel, gpointer user_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) user_data;
  if (sel != -1) {
    plot_tab_setup_item(d, sel);
  }
}

static void
plot_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, FileCB, plot_tab_copy_response, d);
}

void
copy_file_obj_field(struct objlist *obj, int id, int sel, int copy_filename)
{
  char *field[] = {"name", "fit", "file", "array", NULL};

  if (copy_filename) {
    field[2] = NULL;
  }

  copy_obj_field(obj, id, sel, field);

  FitCopy(obj, id, sel);
  set_graph_modified();
}

static void
FileDialogOption(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) client_data;
  exeobj(d->Obj, "load_settings", d->Id, 0, NULL);
  FileDialogSetupItem(d->widget, d);
}

static void
edit_file(const char *file)
{
  char *cmd, *localize_name;

  if (file == NULL)
    return;

  localize_name = get_localized_filename(file);
  if (localize_name == NULL)
    return;

#if OSX
  cmd = g_strdup_printf("%s \"%s\"", Menulocal.editor, localize_name);
#else
  cmd = g_strdup_printf("\"%s\" \"%s\"", Menulocal.editor, localize_name);
#endif
  g_free(localize_name);

  system_bg(cmd);

  g_free(cmd);
}

static void
FileDialogEdit(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  const char *file;

  d = (struct FileDialog *) client_data;

  if (Menulocal.editor == NULL)
    return;

  file = gtk_editable_get_text(GTK_EDITABLE(d->file));
  if (file == NULL)
    return;

  edit_file(file);
}

#if 1
static void
FileDialogType(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  int type;

  d = (struct FileDialog *) client_data;

  type = combo_box_get_active(w);
  if (type < 0) {
    return;
  }

  set_widget_sensitivity_with_label(d->curve, TRUE);
  set_widget_sensitivity_with_label(d->fit, TRUE);

  switch (type) {
  case PLOT_TYPE_MARK:
  case PLOT_TYPE_LINE:
  case PLOT_TYPE_POLYGON:
  case PLOT_TYPE_DIAGONAL:
  case PLOT_TYPE_RECTANGLE:
  case PLOT_TYPE_POLYGON_SOLID_FILL:
  case PLOT_TYPE_RECTANGLE_SOLID_FILL:
  case PLOT_TYPE_ARROW:
  case PLOT_TYPE_RECTANGLE_FILL:
  case PLOT_TYPE_ERRORBAR_X:
  case PLOT_TYPE_ERRORBAR_Y:
  case PLOT_TYPE_ERRORBAND_X:
  case PLOT_TYPE_ERRORBAND_Y:
  case PLOT_TYPE_STAIRCASE_X:
  case PLOT_TYPE_STAIRCASE_Y:
  case PLOT_TYPE_BAR_X:
  case PLOT_TYPE_BAR_Y:
  case PLOT_TYPE_BAR_SOLID_FILL_X:
  case PLOT_TYPE_BAR_SOLID_FILL_Y:
  case PLOT_TYPE_BAR_FILL_X:
  case PLOT_TYPE_BAR_FILL_Y:
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_CURVE:
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_FIT:
    set_widget_sensitivity_with_label(d->curve, FALSE);
    break;
  }
}
#else
static void
FileDialogType(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  int type;

  d = (struct FileDialog *) client_data;

  type = combo_box_get_active(w);
  if (type < 0) {
    return;
  }

  set_widget_sensitivity_with_label(d->mark_btn, TRUE);
  set_widget_sensitivity_with_label(d->curve, TRUE);
  set_widget_sensitivity_with_label(d->col2, TRUE);
  set_widget_sensitivity_with_label(d->size, TRUE);
  set_widget_sensitivity_with_label(d->miter, TRUE);
  set_widget_sensitivity_with_label(d->join, TRUE);
  set_widget_sensitivity_with_label(d->fit, TRUE);
  set_widget_sensitivity_with_label(d->style, TRUE);
  set_widget_sensitivity_with_label(d->width, TRUE);

  switch (type) {
  case PLOT_TYPE_MARK:
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_LINE:
  case PLOT_TYPE_POLYGON:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->size, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_CURVE:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->size, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_DIAGONAL:
  case PLOT_TYPE_RECTANGLE:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->size, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_POLYGON_SOLID_FILL:
  case PLOT_TYPE_RECTANGLE_SOLID_FILL:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->size, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    set_widget_sensitivity_with_label(d->style, FALSE);
    set_widget_sensitivity_with_label(d->width, FALSE);
    break;
  case PLOT_TYPE_ARROW:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_RECTANGLE_FILL:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->size, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_ERRORBAR_X:
  case PLOT_TYPE_ERRORBAR_Y:
  case PLOT_TYPE_ERRORBAND_X:
  case PLOT_TYPE_ERRORBAND_Y:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_STAIRCASE_X:
  case PLOT_TYPE_STAIRCASE_Y:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->size, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_BAR_X:
  case PLOT_TYPE_BAR_Y:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_BAR_SOLID_FILL_X:
  case PLOT_TYPE_BAR_SOLID_FILL_Y:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    set_widget_sensitivity_with_label(d->style, FALSE);
    set_widget_sensitivity_with_label(d->width, FALSE);
    break;
  case PLOT_TYPE_BAR_FILL_X:
  case PLOT_TYPE_BAR_FILL_Y:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->miter, FALSE);
    set_widget_sensitivity_with_label(d->join, FALSE);
    set_widget_sensitivity_with_label(d->fit, FALSE);
    break;
  case PLOT_TYPE_FIT:
    set_widget_sensitivity_with_label(d->mark_btn, FALSE);
    set_widget_sensitivity_with_label(d->curve, FALSE);
    set_widget_sensitivity_with_label(d->col2, FALSE);
    set_widget_sensitivity_with_label(d->size, FALSE);
    break;
  }
}
#endif

static void
file_settings_copy_response(int sel, gpointer user_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) user_data;
  if (sel != -1) {
    file_setup_item(d, sel);
  }
}

static void
file_settings_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) user_data;
  CopyClick(d->widget, d->Obj, d->Id, FileCB, file_settings_copy_response, d);
}

static void
selct_type_notify(GtkWidget *w, GParamSpec* pspec, gpointer user_data)
{
  struct FileDialog *d = (struct FileDialog *) user_data;
  FileDialogType(d->type, d);
}

static GtkWidget *
plot_tab_create(GtkWidget *parent, struct FileDialog *d)
{
  GtkWidget *table, *hbox, *w, *vbox;
  int i;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  table = gtk_grid_new();

  i = 0;
  w = combo_box_create();
  add_widget_to_table(table, w, _("_Type:"), FALSE, i++);
  d->type = w;
  g_signal_connect(w, "notify::selected", G_CALLBACK(selct_type_notify), d);

  w = gtk_button_new();
  add_widget_to_table(table, w, _("_Mark:"), FALSE, i++);
  d->mark_btn = w;
  g_signal_connect(w, "clicked", G_CALLBACK(FileDialogMark), d);

  w = combo_box_create();
  add_widget_to_table(table, w, _("_Curve:"), FALSE, i++);
  d->curve = w;

  d->fit_table = table;
  d->fit_row = i;

  i++;
  w = create_color_button(parent);
  add_widget_to_table(table, w, _("_Color 1:"), FALSE, i++);
  d->col1 = w;

  w = create_color_button(parent);
  add_widget_to_table(table, w, _("_Color 2:"), FALSE, i++);
  d->col2 = w;

  gtk_box_append(GTK_BOX(hbox), table);

  table = gtk_grid_new();

  i = 0;
  w = combo_box_entry_create();
  combo_box_entry_set_width(w, NUM_ENTRY_WIDTH);
  add_widget_to_table(table, w, _("Line _Style:"), TRUE, i++);
  d->style = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_WIDTH, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Line Width:"), FALSE, i++);
  d->width = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Size:"), FALSE, i++);
  d->size = w;

  w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, TRUE, TRUE);
  add_widget_to_table(table, w, _("_Miter:"), FALSE, i++);
  d->miter = w;

  w = combo_box_create();
  add_widget_to_table(table, w, _("_Join:"), FALSE, i++);
  d->join = w;

  w = gtk_check_button_new_with_mnemonic(_("_Clip"));
  add_widget_to_table(table, w, NULL, FALSE, i++);
  d->clip = w;

  gtk_box_append(GTK_BOX(hbox), table);

  w = gtk_frame_new(NULL);
  gtk_frame_set_child(GTK_FRAME(w), hbox);
  gtk_widget_set_vexpand(w, TRUE);
  set_widget_margin(w, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
  gtk_box_append(GTK_BOX(vbox), w);

  add_copy_button_to_box(vbox, G_CALLBACK(plot_tab_copy), d, "data");

  return vbox;
}

static void
FileDialogSetupCommon(GtkWidget *wi, struct FileDialog *d)
{
  GtkWidget *w, *hbox, *vbox2, *frame, *notebook, *label;


  vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  if (d->source != DATA_SOURCE_RANGE) {
    w = create_spin_entry(0, FILE_OBJ_MAXCOL, 1, FALSE, TRUE);
    item_setup(hbox, w, _("_X column:"), TRUE);
    d->xcol = w;
  }

  w = axis_combo_box_create(AXIS_COMBO_BOX_NONE);
  item_setup(hbox, w, _("_X axis:"), TRUE);
  d->xaxis = w;

  gtk_box_append(GTK_BOX(vbox2), hbox);


  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  if (d->source != DATA_SOURCE_RANGE) {
    w = create_spin_entry(0, FILE_OBJ_MAXCOL, 1, FALSE, TRUE);
    item_setup(hbox, w, _("_Y column:"), TRUE);
    d->ycol = w;
  }

  w = axis_combo_box_create(AXIS_COMBO_BOX_NONE);
  item_setup(hbox, w, _("_Y axis:"), TRUE);
  d->yaxis = w;

  gtk_box_append(GTK_BOX(vbox2), hbox);

  add_copy_button_to_box(vbox2, G_CALLBACK(file_settings_copy), d, "data");

  hbox = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(hbox), 4);
  d->comment_box = hbox;
  frame = gtk_frame_new(NULL);
  gtk_frame_set_child(GTK_FRAME(frame), vbox2);

  gtk_widget_set_hexpand(frame, FALSE);
  gtk_grid_attach(GTK_GRID(hbox), frame, 0, 0, 1, 1);
  gtk_box_append(GTK_BOX(d->vbox), hbox);

  notebook = gtk_notebook_new();

  d->tab = GTK_NOTEBOOK(notebook);
  gtk_notebook_set_scrollable(d->tab, FALSE);
  gtk_notebook_set_tab_pos(d->tab, GTK_POS_TOP);


  w = plot_tab_create(wi, d);
  label = gtk_label_new_with_mnemonic(_("_Plot"));
  gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

  w = math_tab_create(d);
  label = gtk_label_new_with_mnemonic(_("_Math"));
  d->math.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

  w = load_tab_create(d);
  label = gtk_label_new_with_mnemonic(_("_Load"));
  d->load.tab_id = gtk_notebook_append_page(GTK_NOTEBOOK(notebook), w, label);

  gtk_box_append(GTK_BOX(d->vbox), notebook);
}

static void
set_headlines(struct FileDialog *d, const char *s)
{
  gboolean valid;
  const gchar *sptr;

  if (s == NULL) {
    return;
  }

  if (Menulocal.file_preview_font) {
    text_view_with_line_number_set_font(d->comment_view, Menulocal.file_preview_font);
  }
  valid = g_utf8_validate(s, -1, &sptr);

  if (valid) {
    text_view_with_line_number_set_text(d->comment_view, s);
  } else {
    char *ptr;

    ptr = n_locale_to_utf8(s);
    if (ptr) {
      text_view_with_line_number_set_text(d->comment_view, ptr);
      g_free(ptr);
    } else {
      text_view_with_line_number_set_text(d->comment_view, _("This file contain invalid UTF-8 strings."));
    }
  }
}

#define CHECK_VISIBILITY(i, skip, step, remark, c)    (! CHECK_CHR(remark, c) && (i >= skip && ! ((i - skip) % step)))
#define CHECK_VISIBILITY_ARRAY(i, skip, step)    ((i >= skip && ! ((i - skip) % step)))

#define MAX_COLS 100

static void
set_headline_table_header(struct FileDialog *d)
{
  int x, y, type, i;
  GString *str;
  GListModel *columns;

  type = combo_box_get_active(d->type);
  if (type < 0) {
    return;
  }
  x = spin_entry_get_val(d->xcol);
  y = spin_entry_get_val(d->ycol);

  str = g_string_new("");
  if (str == NULL) {
    return;
  }

  columns = gtk_column_view_get_columns (GTK_COLUMN_VIEW (d->comment_table));
  for (i = 0; i < MAX_COLS; i++) {
    GtkColumnViewColumn *col;

    g_string_set_size(str, 0);

    col = GTK_COLUMN_VIEW_COLUMN (g_list_model_get_object (columns, i));

    if (i == x) {
      switch (type) {
      case PLOT_TYPE_DIAGONAL:
      case PLOT_TYPE_ARROW:
      case PLOT_TYPE_RECTANGLE:
      case PLOT_TYPE_RECTANGLE_FILL:
      case PLOT_TYPE_RECTANGLE_SOLID_FILL:
	g_string_append(str, "X1");
	break;
      default:
	g_string_append(str, "X");
      }
    } else if (i == x + 1) {
      switch (type) {
      case PLOT_TYPE_DIAGONAL:
      case PLOT_TYPE_ARROW:
      case PLOT_TYPE_RECTANGLE:
      case PLOT_TYPE_RECTANGLE_FILL:
      case PLOT_TYPE_RECTANGLE_SOLID_FILL:
	g_string_append(str, "Y1");
	break;
      case PLOT_TYPE_ERRORBAR_X:
      case PLOT_TYPE_ERRORBAND_X:
	g_string_append(str, "Ex1");
	break;
      }
    } else if (i == x + 2) {
      switch (type) {
      case PLOT_TYPE_ERRORBAR_X:
      case PLOT_TYPE_ERRORBAND_X:
	g_string_append(str, "Ex2");
	break;
      }
    }

    if (str->len) {
      g_string_append_c(str, ' ');
    }

    if (i == y) {
      switch (type) {
      case PLOT_TYPE_DIAGONAL:
      case PLOT_TYPE_ARROW:
      case PLOT_TYPE_RECTANGLE:
      case PLOT_TYPE_RECTANGLE_FILL:
      case PLOT_TYPE_RECTANGLE_SOLID_FILL:
	g_string_append(str, "X2");
	break;
      default:
	g_string_append(str, "Y");
      }
    } else if (i == y + 1) {
      switch (type) {
      case PLOT_TYPE_DIAGONAL:
      case PLOT_TYPE_ARROW:
      case PLOT_TYPE_RECTANGLE:
      case PLOT_TYPE_RECTANGLE_FILL:
      case PLOT_TYPE_RECTANGLE_SOLID_FILL:
	g_string_append(str, "Y2");
	break;
      case PLOT_TYPE_ERRORBAR_Y:
      case PLOT_TYPE_ERRORBAND_Y:
	g_string_append(str, "Ey1");
	break;
      }
    } else if (i == y + 2) {
      switch (type) {
      case PLOT_TYPE_ERRORBAR_Y:
      case PLOT_TYPE_ERRORBAND_Y:
	g_string_append(str, "Ey2");
	break;
      }
    }

    if (str->len) {
      if (str->str[str->len - 1] != ' ') {
	g_string_append_c(str, ' ');
      }
      g_string_append_printf(str, "(%%%d)", i);
    } else {
      g_string_append_printf(str, "%%%d", i);
    }
    gtk_column_view_column_set_title (col, str->str);
  }
  g_string_free(str, TRUE);
}

static void
set_headline_table_array(struct FileDialog *d, int max_lines)
{
  struct array_prm ary;
  int i, j, l, m, n, skip, step;
  char *array;
  GListStore *model;
  char *text[MAX_COLS + 2];

  getobj(d->Obj, "array", d->Id, 0, NULL, &array);
  open_array(array, &ary);

  skip = spin_entry_get_val(d->load.headskip);
  if (skip < 0) {
    skip = 0;
  }

  step = spin_entry_get_val(d->load.readstep);
  if (step < 1) {
    step = 1;
  }

  columnview_clear(d->comment_table);
  model = columnview_get_list (d->comment_table);

  n = (ary.data_num > max_lines) ? max_lines : ary.data_num;
  m = (ary.col_num < MAX_COLS) ? ary.col_num : MAX_COLS;
  l = 1;
  for (i = 0; i < n; i++) {
    int v;

    for (j = 0; j < m; j++) {
      void *ptr;
      ptr = arraynget(ary.ary[j], i);
      if (ptr) {
        double val;
	val = * (double *) ptr;
	text[j + 1] = g_strdup_printf("%G", val);
      }
    }
    text[j + 1] = NULL;
    v = CHECK_VISIBILITY_ARRAY(i, skip, step);
    if (v) {
      text[0] = g_strdup_printf ("%d", l);
      l++;
    } else {
      text[0] = g_strdup ("");
    }
    list_store_append_ngraph_text(model, text, v);
    for (j = 0; j < m + 1; j++) {
      g_free (text[j]);
    }
  }
}

static void
set_headline_table(struct FileDialog *d, char *s, int max_lines)
{
  struct narray *lines;
  int i, j, l, n, skip, step, csv;
  const char *tmp, *remark, *po;
  GString *ifs;
  GListStore *model;
  char *text[MAX_COLS + 2];

  if (! d->initialized || s == NULL || max_lines < 1) {
    return;
  }

  lines = g_malloc0(sizeof(*lines) * max_lines);
  if (lines == NULL) {
    return;
  }

  skip = spin_entry_get_val(d->load.headskip);
  if (skip < 0) {
    skip = 0;
  }

  step = spin_entry_get_val(d->load.readstep);
  if (step < 1) {
    step = 1;
  }

  csv = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->load.csv));

  remark = gtk_editable_get_text(GTK_EDITABLE(d->load.remark));
  if (remark == NULL) {
    remark = "";
  }

  tmp = gtk_editable_get_text(GTK_EDITABLE(d->load.ifs));
  if (tmp == NULL) {
    tmp = "";
  }

  ifs = g_string_new("");
  decode_ifs_text(ifs, tmp);

  po = s;
  for (n = 0; n < max_lines; n++) {
    arrayinit(lines + n, sizeof(char *));
    po = parse_data_line(lines + n, po, ifs->str, csv);
    if (po == NULL) {
      n++;
      break;
    }
  }
  g_string_free(ifs, TRUE);
  if (n == 0) {
    goto exit;
  }

  columnview_clear (d->comment_table);
  model = columnview_get_list(d->comment_table);

  l = 1;
  for (i = 0; i < n; i++) {
    int m, c, v;
    const char *str;
    char buf[64];

    m = arraynum(lines + i);
    m = (m < MAX_COLS) ? m : MAX_COLS;
    for (j = 0; j < m; j++) {
      text[j + 1] = arraynget_str(lines + i, j);
    }
    text[j + 1] = NULL;
    str = arraynget_str(lines + i, 0);
    if (str) {
      c = (g_ascii_isprint(str[0]) || g_ascii_isspace(str[0])) ? str[0] : 0;
    } else {
      c = 0;
    }
    v = CHECK_VISIBILITY(i, skip, step, remark, c);
    if (v) {
      snprintf (buf, sizeof (buf), "%d", l);
      text[0] = buf;
      l++;
    } else {
      text[0] = "";
    }
    list_store_append_ngraph_text (model, text, v);
  }

 exit:
  for (i = 0; i < n; i++) {
    arraydel2(lines + i);
  }
  g_free(lines);
}

static void
setup_table (GtkListItemFactory *factory, GtkListItem *list_item)
{
  GtkWidget *label;

  label = gtk_label_new (NULL);
  gtk_widget_set_halign (label, GTK_ALIGN_END);
  gtk_list_item_set_child (list_item, label);
}

static void
bind_table (GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
  GtkLabel *label;
  NgraphText *text;
  const char *str;
  guint i;

  label = GTK_LABEL (gtk_list_item_get_child (list_item));
  text = NGRAPH_TEXT (gtk_list_item_get_item (list_item));
  i = GPOINTER_TO_INT (user_data);
  if (i < text->size) {
    str = text->text[i];
  } else {
    str = "";
  }
  gtk_label_set_ellipsize (label, text->attribute ? PANGO_ELLIPSIZE_NONE : PANGO_ELLIPSIZE_END);
  gtk_widget_set_sensitive (GTK_WIDGET (label), text->attribute);
  gtk_label_set_text(label, str);
}

static GtkWidget *
create_preview_table(struct FileDialog *d)
{
  GtkWidget *view;
  int i;
  view = columnview_create(NGRAPH_TYPE_TEXT, N_SELECTION_TYPE_NONE);
  set_widget_font(view, Menulocal.file_preview_font);
  for (i = 0; i < MAX_COLS; i++) {
    char buf[32];
    snprintf(buf, sizeof(buf), "%%%d", i);
    columnview_create_column (view, buf, G_CALLBACK (setup_table), G_CALLBACK (bind_table), NULL, GINT_TO_POINTER (i), FALSE);
  }

  return view;
}

static void
update_table(struct FileDialog *d)
{
  set_headline_table(d, d->head_lines, Menulocal.data_head_lines);
}

static void
FileDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct FileDialog *d;
  int line;
  char title[32], *argv[2], *s;

  d = (struct FileDialog *) data;

  snprintf(title, sizeof(title), _("Data %d (File)"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    GtkWidget *w, *hbox, *view, *label;
    d->apply_all = gtk_dialog_add_button(GTK_DIALOG(wi), _("_Apply all"), IDFAPPLY);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    w = create_file_entry(d->Obj);
    item_setup(GTK_WIDGET(hbox), w, _("_File:"), TRUE);
    d->file = w;

    w = gtk_button_new_with_mnemonic(_("_Load settings"));
    gtk_box_append(GTK_BOX(hbox), w);
    d->load_settings = w;
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogOption), d);

    w = gtk_button_new_with_mnemonic(_("_Edit"));
    gtk_box_append(GTK_BOX(hbox), w);
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogEdit), d);


    gtk_box_append(GTK_BOX(d->vbox), hbox);


    FileDialogSetupCommon(wi, d);

    w = mask_tab_create(d);
    label = gtk_label_new_with_mnemonic(_("_Mask"));
    d->mask.tab_id = gtk_notebook_append_page(d->tab, w, label);

    w = move_tab_create(d);
    label = gtk_label_new_with_mnemonic(_("_Move"));
    d->move.tab_id = gtk_notebook_append_page(d->tab, w, label);

    w = gtk_notebook_new();

    view = create_preview_table(d);
    if (view) {
      GtkWidget *swin;
      label = gtk_label_new_with_mnemonic(_("_Table"));
      swin = gtk_scrolled_window_new();
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
      gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), view);
      gtk_notebook_append_page(GTK_NOTEBOOK(w), swin, label);
    }
    d->comment_table = view;

    view = create_text_view_with_line_number(&d->comment_view);
    label = gtk_label_new_with_mnemonic(_("_Plain"));
    gtk_notebook_append_page(GTK_NOTEBOOK(w), view, label);

    g_signal_connect_swapped(d->load.ifs, "changed", G_CALLBACK(update_table), d);
    g_signal_connect_swapped(d->load.csv, "toggled", G_CALLBACK(update_table), d);

    g_signal_connect_swapped(d->load.remark, "changed", G_CALLBACK(update_table), d);
    g_signal_connect_swapped(d->load.readstep, "value-changed", G_CALLBACK(update_table), d);
    g_signal_connect_swapped(d->load.headskip, "value-changed", G_CALLBACK(update_table), d);

    g_signal_connect_swapped(d->xcol, "changed", G_CALLBACK(set_headline_table_header), d);
    g_signal_connect_swapped(d->ycol, "changed", G_CALLBACK(set_headline_table_header), d);
    g_signal_connect_swapped(d->type, "notify::selected", G_CALLBACK(set_headline_table_header), d);

    gtk_grid_attach(GTK_GRID(d->comment_box), w, 1, 0, 1, 1);
    w = gtk_button_new_with_label(_("Create"));
    add_widget_to_table(d->fit_table, w, _("_Fit:"), FALSE, d->fit_row);
    d->fit = w;
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogFit), d);
  }

  line = Menulocal.data_head_lines;
  argv[0] = (char *) &line;
  argv[1] = NULL;
  getobj(d->Obj, "head_lines", d->Id, 1, argv, &s);
  FileDialogSetupItem(wi, d);

  d->initialized = TRUE;
  set_headlines(d, s);
  set_headline_table_header(d);
  set_headline_table(d, s, line);
  d->head_lines = g_strdup(s);
}

static void
ArrayDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct FileDialog *d;
  char title[32];

  d = (struct FileDialog *) data;

  snprintf(title, sizeof(title), _("Data %d (Array)"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    GtkWidget *w, *hbox, *view, *label, *swin;
    d->apply_all = gtk_dialog_add_button(GTK_DIALOG(wi), _("_Apply all"), IDFAPPLY);

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    w = create_text_entry(TRUE, TRUE);
    item_setup(GTK_WIDGET(hbox), w, _("_Array:"), TRUE);
    d->file = w;

    gtk_box_append(GTK_BOX(d->vbox), hbox);

    FileDialogSetupCommon(wi, d);

    w = mask_tab_create(d);
    label = gtk_label_new_with_mnemonic(_("_Mask"));
    d->mask.tab_id = gtk_notebook_append_page(d->tab, w, label);

    w = move_tab_create(d);
    label = gtk_label_new_with_mnemonic(_("_Move"));
    d->move.tab_id = gtk_notebook_append_page(d->tab, w, label);

    view = create_preview_table(d);
    swin = gtk_scrolled_window_new();
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), view);
    d->comment_table = view;

    g_signal_connect_swapped(d->load.readstep, "value-changed", G_CALLBACK(update_table), d);
    g_signal_connect_swapped(d->load.headskip, "value-changed", G_CALLBACK(update_table), d);

    g_signal_connect_swapped(d->xcol, "changed", G_CALLBACK(set_headline_table_header), d);
    g_signal_connect_swapped(d->ycol, "changed", G_CALLBACK(set_headline_table_header), d);
    g_signal_connect_swapped(d->type, "notify::selected", G_CALLBACK(set_headline_table_header), d);

    gtk_widget_set_hexpand(swin, TRUE);
    gtk_widget_set_vexpand(swin, TRUE);
    gtk_grid_attach(GTK_GRID(d->comment_box), swin, 1, 0, 1, 1);
    w = gtk_button_new_with_label(_("Create"));
    add_widget_to_table(d->fit_table, w, _("_Fit:"), FALSE, d->fit_row);
    d->fit = w;
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogFit), d);
  }

  FileDialogSetupItem(wi, d);

  d->initialized = TRUE;
  set_headline_table_header(d);
  set_headline_table_array(d, Menulocal.data_head_lines);
}

static void
RangeDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct FileDialog *d;
  char title[32];

  d = (struct FileDialog *) data;

  snprintf(title, sizeof(title), _("Data %d (Range)"), d->Id);
  gtk_window_set_title(GTK_WINDOW(wi), title);

  if (makewidget) {
    GtkWidget *w, *table, *label;
    int i;
    d->apply_all = gtk_dialog_add_button(GTK_DIALOG(wi), _("_Apply all"), IDFAPPLY);

    table = gtk_grid_new();

    i = 0;
    w = create_text_entry(FALSE, TRUE);
    gtk_editable_set_width_chars(GTK_EDITABLE(w), RANGE_ENTRY_WIDTH);
    add_widget_to_table(table, w, _("_Minimum:"), FALSE, i++);
    d->min = w;

    w = create_text_entry(FALSE, TRUE);
    gtk_editable_set_width_chars(GTK_EDITABLE(w), RANGE_ENTRY_WIDTH);
    add_widget_to_table(table, w, _("_Maximum:"), FALSE, i++);
    d->max = w;

    w = create_spin_entry(2, 65536, 1, FALSE, TRUE);
    add_widget_to_table(table, w, _("di_ViSion:"), FALSE, i++);
    d->div = w;

    FileDialogSetupCommon(wi, d);

    gtk_grid_insert_column(GTK_GRID(d->comment_box), 0);
    gtk_grid_attach(GTK_GRID(d->comment_box), table, 0, 0, 1, 1);

    w = mask_tab_create(d);
    label = gtk_label_new_with_mnemonic(_("_Mask"));
    d->mask.tab_id = gtk_notebook_append_page(d->tab, w, label);

    w = move_tab_create(d);
    label = gtk_label_new_with_mnemonic(_("_Move"));
    d->move.tab_id = gtk_notebook_append_page(d->tab, w, label);

    w = gtk_button_new_with_label(_("Create"));
    add_widget_to_table(d->fit_table, w, _("_Fit:"), FALSE, d->fit_row);
    d->fit = w;
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogFit), d);
  }

  FileDialogSetupItem(wi, d);
}

static int
plot_tab_set_value(struct FileDialog *d)
{
  if (SetObjFieldFromWidget(d->type, d->Obj, d->Id, "type"))
    return TRUE;

  if (SetObjFieldFromWidget(d->curve, d->Obj, d->Id, "interpolation"))
    return TRUE;

  if (putobj(d->Obj, "mark_type", d->Id, &(d->mark.Type)) == -1)
    return TRUE;

  if (SetObjFieldFromWidget(d->size, d->Obj, d->Id, "mark_size"))
    return TRUE;

  if (SetObjFieldFromWidget(d->width, d->Obj, d->Id, "line_width"))
    return TRUE;

  if (SetObjFieldFromStyle(d->style, d->Obj, d->Id, "line_style"))
    return TRUE;

  if (SetObjFieldFromWidget(d->join, d->Obj, d->Id, "line_join"))
    return TRUE;

  if (SetObjFieldFromWidget(d->miter, d->Obj, d->Id, "line_miter_limit"))
    return TRUE;

  if (SetObjFieldFromWidget(d->clip, d->Obj, d->Id, "data_clip"))
    return TRUE;

  if (putobj_color(d->col1, d->Obj, d->Id, NULL))
    return TRUE;

  if (putobj_color2(d->col2, d->Obj, d->Id))
    return TRUE;

  return 0;
}

static void
focus_math_widget(struct FileDialog *d, int math_tab_id, GtkWidget *text, GtkWidget *entry)
{
  if (d->math_page == 0) {
    gtk_widget_grab_focus(entry);
  } else {
    gtk_notebook_set_current_page(d->math_tab, math_tab_id);
    gtk_widget_grab_focus(text);
    move_cursor_to_error_line(text);
  }
}

static int
FileDialogCloseCommon(GtkWidget *w, struct FileDialog *d)
{
  int r;

  if (SetObjFieldFromWidget(d->xcol, d->Obj, d->Id, "x"))
    return TRUE;

  if (SetObjAxisFieldFromWidget(d->xaxis, d->Obj, d->Id, "axis_x"))
    return TRUE;

  if (SetObjFieldFromWidget(d->ycol, d->Obj, d->Id, "y"))
    return TRUE;

  if (SetObjAxisFieldFromWidget(d->yaxis, d->Obj, d->Id, "axis_y"))
    return TRUE;

  if (plot_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, 0);
    return TRUE;
  }

  r = math_tab_set_value(d);
  if (r != MATH_ERROR_FIELD_NONE) {
    gtk_notebook_set_current_page(d->tab, d->math.tab_id);
    switch (r) {
    case MATH_ERROR_FIELD_X:
      focus_math_widget(d, r -1, d->math.text_x, d->math.x);
      break;
    case MATH_ERROR_FIELD_Y:
      focus_math_widget(d, r -1, d->math.text_y, d->math.y);
      break;
    case MATH_ERROR_FIELD_F:
      focus_math_widget(d, r -1, d->math.text_f, d->math.f);
      break;
    case MATH_ERROR_FIELD_G:
      focus_math_widget(d, r -1, d->math.text_g, d->math.g);
      break;
    case MATH_ERROR_FIELD_H:
      focus_math_widget(d, r -1, d->math.text_h, d->math.h);
      break;
    case MATH_ERROR_FIELD_SX:
      gtk_widget_grab_focus(d->math.xsmooth);
      break;
    case MATH_ERROR_FIELD_SY:
      gtk_widget_grab_focus(d->math.ysmooth);
      break;
    }
    return TRUE;
  }

  if (load_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, d->load.tab_id);
    return TRUE;
  }

  Menulocal.math_input_mode = gtk_notebook_get_current_page(GTK_NOTEBOOK(d->math_input_tab));

  return FALSE;
}

static void
FileDialogClose(GtkWidget *w, void *data)
{
  struct FileDialog *d;
  int ret;

  d = (struct FileDialog *) data;

  switch (d->ret) {
  case IDOK:
  case IDFAPPLY:
    break;
  default:
    goto End;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  switch (d->source) {
  case DATA_SOURCE_FILE:
    if (SetObjFieldFromWidget(d->file, d->Obj, d->Id, "file")) {
      return;
    }
    break;
  case DATA_SOURCE_ARRAY:
    if (SetObjFieldFromWidget(d->file, d->Obj, d->Id, "array")) {
      return;
    }
    break;
  case DATA_SOURCE_RANGE:
    if (SetObjFieldFromWidget(d->min, d->Obj, d->Id, "range_min")) {
      return;
    }
    if (SetObjFieldFromWidget(d->max, d->Obj, d->Id, "range_max")) {
      return;
    }
    if (SetObjFieldFromWidget(d->div, d->Obj, d->Id, "range_div")) {
      return;
    }
    break;
  }

  if (FileDialogCloseCommon(w, d))
    return;

  if (mask_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, d->mask.tab_id);
    return;
  }

  if (move_tab_set_value(d)) {
    gtk_notebook_set_current_page(d->tab, d->move.tab_id);
    return;
  }

  d->initialized = FALSE;
  d->ret = ret;

 End:
  g_free(d->head_lines);
  d->head_lines = NULL;
}

void
FileDialog(struct obj_list_data *data, int id, int multi)
{
  struct FileDialog *d;
  int source;

  getobj(data->obj, "source", id, 0, NULL, &source);
  switch (source) {
  case DATA_SOURCE_FILE:
    d = &DlgFile;
    data->dialog = d;
    d->SetupWindow = FileDialogSetup;
    break;
  case DATA_SOURCE_ARRAY:
    d = &DlgArray;
    data->dialog = d;
    d->SetupWindow = ArrayDialogSetup;
    break;
  case DATA_SOURCE_RANGE:
    d = &DlgRange;
    data->dialog = d;
    d->SetupWindow = RangeDialogSetup;
    break;
  default:
    return; 			/* never reached */
  }

  d->CloseWindow = FileDialogClose;
  d->Obj = data->obj;
  d->Id = id;
  d->source = source;
  d->multi_open = multi > 0;
  d->initialized = FALSE;
  d->head_lines = NULL;
}

static void
FileDialogDefSetupItem(GtkWidget *w, struct FileDialog *d, int id)
{
  plot_tab_setup_item(d, d->Id);
  math_tab_setup_item(d, d->Id);
  load_tab_setup_item(d, d->Id);
  file_setup_item(d, d->Id);
}

static void
FileDefDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct FileDialog *d;

  d = (struct FileDialog *) data;

  if (makewidget) {
    FileDialogSetupCommon(wi, d);
    gtk_notebook_set_tab_pos(d->tab, GTK_POS_TOP);
  }
  FileDialogDefSetupItem(wi, d, d->Id);
}

static void
FileDefDialogClose(GtkWidget *w, void *data)
{
  struct FileDialog *d;
  int ret;

  d = (struct FileDialog *) data;

  switch (d->ret) {
  case IDOK:
    break;
  default:
    return;
  }

  ret = d->ret;
  d->ret = IDLOOP;

  if (FileDialogCloseCommon(w, d))
    return;

  d->ret = ret;
}

void
FileDefDialog(struct FileDialog *data, struct objlist *obj, int id)
{
  data->SetupWindow = FileDefDialogSetup;
  data->CloseWindow = FileDefDialogClose;
  data->Obj = obj;
  data->Id = id;
  data->source = DATA_SOURCE_FILE;
}

static void
delete_file_obj(struct obj_list_data *data, int id)
{
  FitDel(data->obj, id);
  delobj(data->obj, id);
  /* don't delete darray object */
}

static int
data_save_undo(int type)
{
  char *arg[3];
  arg[0] = "data";
  arg[1] = "fit";
  arg[2] = NULL;
  return menu_save_undo(type, arg);
}

struct load_data_data
{
  int undo;
  char *fname;
  struct obj_list_data *data;
};

static void
load_data_response(struct response_callback *cb)
{
  char *fname;
  int undo;
  struct load_data_data *res_data;
  struct obj_list_data *data;
  res_data = (struct load_data_data *) cb->data;
  undo = res_data->undo;
  data = res_data->data;
  fname = res_data->fname;
  if (cb->return_value == IDCANCEL) {
    menu_undo_internal(undo);
  } else {
    set_graph_modified();
    AddDataFileList(fname);
  }
  FileWinUpdate(data, TRUE, DRAW_NOTIFY);
  g_free(res_data);
}

void
load_data(const char *name)
{
  char *fname;
  int id, undo;
  struct objlist *obj;
  struct obj_list_data *data;
  struct load_data_data *res_data;

  if (Menulock || Globallock) {
    return;
  }

  obj = chkobject("data");
  if (obj == NULL) {
    return;
  }

  undo = data_save_undo(UNDO_TYPE_OPEN_FILE);
  id = newobj(obj);
  if (id < 0) {
    menu_delete_undo(undo);
    return;
  }

  fname = g_strdup(name);
  if (fname == NULL) {
    menu_delete_undo(undo);
    return;
  }

  res_data = g_malloc0(sizeof(*data));
  if (res_data == NULL) {
    menu_delete_undo(undo);
    return;
  }

  putobj(obj, "file", id, fname);
  data = NgraphApp.FileWin.data.data;
  res_data->data = data;
  res_data->undo = undo;
  res_data->fname = fname;
  FileDialog(data, id, FALSE);
  response_callback_add(data->dialog, load_data_response, NULL, res_data);
  DialogExecute(TopLevel, data->dialog);
}

struct range_add_data
{
  int undo;
  struct obj_list_data *data;
};

static void
range_add_response(struct response_callback *cb)
{
  int undo;
  struct obj_list_data *data;
  struct range_add_data *res_data;
  res_data = (struct range_add_data *) cb->data;
  undo = res_data->undo;
  data = res_data->data;
  if (cb->return_value == IDCANCEL) {
    menu_undo_internal(undo);
  } else {
    set_graph_modified();
    FileWinUpdate(data, TRUE, DRAW_REDRAW);
  }
  g_free(res_data);
}

void
CmRangeAdd
(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  int id, val, undo;
  struct objlist *obj;
  struct obj_list_data *data;
  struct range_add_data *res_data;

  if (Menulock || Globallock)
    return;

  obj = chkobject("data");
  if (obj == NULL) {
    return;
  }

  undo = data_save_undo(UNDO_TYPE_ADD_RANGE);
  id = newobj(obj);
  if (id < 0) {
    menu_delete_undo(undo);
    return;
  }

  res_data = g_malloc0(sizeof(*data));
  if (res_data == NULL) {
    menu_delete_undo(undo);
    return;
  }

  data = NgraphApp.FileWin.data.data;
  val = DATA_SOURCE_RANGE;
  putobj(obj, "source", id, &val);
  val = PLOT_TYPE_LINE;
  putobj(obj, "type", id, &val);
  res_data->data = data;
  res_data->undo = undo;
  FileDialog(data, id, FALSE);
  response_callback_add(data->dialog, range_add_response, NULL, res_data);
  DialogExecute(TopLevel, data->dialog);
}

struct file_open_data {
  int n, undo;
  struct narray *farray;
  struct objlist *obj;
};

static void
file_open_response(int ret, gpointer user_data)
{
  struct file_open_data *data;
  int n, undo;
  struct narray *farray;
  struct objlist *obj;

  data = (struct file_open_data *) user_data;
  undo = data->undo;
  n = data->n;
  farray = data->farray;
  obj = data->obj;

  if (ret) {
    menu_delete_undo(undo);
  }

  if (n == chkobjlastinst(obj)) {
    menu_delete_undo(undo);
  } else {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, DRAW_NOTIFY);
    set_graph_modified();
  }

  arrayfree(farray);
  g_free(data);
}

static void
CmFileOpen_response(char **file, gpointer user_data)
{
  int id, n, undo = -1;
  struct objlist *obj;
  struct narray *farray;
  struct file_open_data *data;

  obj = chkobject("data");
  if (obj == NULL) {
    return;
  }

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  n = chkobjlastinst(obj);

  farray = arraynew(sizeof(int));
  if (file) {
    char **ptr;
    undo = data_save_undo(UNDO_TYPE_OPEN_FILE);
    for (ptr = file; *ptr; ptr++) {
      char *name;
      name = *ptr;
      id = newobj(obj);
      if (id >= 0) {
	arrayadd(farray, &id);
	changefilename(name);
	putobj(obj, "file", id, name);
      }
    }
    g_free(file);
  }

  data->undo = undo;
  data->n = n;
  data->farray = farray;
  data->obj = obj;
  update_file_obj_multi(obj, farray, TRUE, file_open_response, data);
}

void
CmFileOpen(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  int chd;

  if (Menulock || Globallock)
    return;

  chd = Menulocal.changedirectory;
  nGetOpenFileNameMulti(TopLevel, _("Add Data file"), NULL,
                        &(Menulocal.fileopendir), NULL, chd, CmFileOpen_response, NULL);
}

static void
file_close_response(struct response_callback *cb)
{
  struct narray *farray;
  farray = (struct narray *) cb->data;
  if (cb->return_value == IDOK) {
    int i, *array, num;
    struct obj_list_data *data;
    data = NgraphApp.FileWin.data.data;
    num = arraynum(farray);
    if (num > 0) {
      data_save_undo(UNDO_TYPE_DELETE);
    }
    array = arraydata(farray);
    for (i = num - 1; i >= 0; i--) {
      delete_file_obj(data, array[i]);
      set_graph_modified();
    }
    FileWinUpdate(data, TRUE, DRAW_REDRAW);
  }
  arrayfree(farray);
}

void
CmFileClose(void *w, gpointer client_data)
{
  struct narray *farray;
  struct objlist *obj;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("data")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  farray = arraynew(sizeof(int));
  SelectDialog(&DlgSelect, obj, _("close data (multi select)"), FileCB, (struct narray *) farray, NULL);
  response_callback_add(&DlgSelect, file_close_response, NULL, farray);
  DialogExecute(TopLevel, &DlgSelect);
}

static void
update_file_obj_multi_response_all(struct objlist *obj, int *array, int top, int num, int id0, int new_file)
{
  int i;
  for (i = top; i < num; i++) {
    copy_file_obj_field(obj, array[i], array[id0], FALSE);
    if (new_file) {
      char *name = NULL;
      getobj(obj, "file", array[i], 0, NULL, &name);
      AddDataFileList(name);
    }
  }
}

struct update_file_obj_multi_data {
  int i, num, *array, modified, undo, new_file;
  struct obj_list_data *data;
  struct objlist *obj;
  response_cb cb;
  gpointer user_data;
};

static void
update_file_obj_multi_response(struct response_callback *cb)
{
  int i, j, num, *array, id0, ret, undo, new_file;
  char *name;
  struct obj_list_data *data;
  struct objlist *obj;
  struct update_file_obj_multi_data *rdata;

  rdata = (struct update_file_obj_multi_data *) cb->data;
  i = rdata->i;
  num = rdata->num;
  array = rdata->array;
  new_file = rdata->new_file;
  undo = rdata->undo;
  data = rdata->data;
  obj = rdata->obj;

  ret = cb->return_value;
  if (ret == IDCANCEL && new_file) {
    ret = IDDELETE;
  }
  switch (ret) {
  case IDDELETE:
    delete_file_obj(data, array[i]);
    rdata->modified = TRUE;
    if (! new_file) {
      set_graph_modified();
    }
    for (j = i + 1; j < num; j++) {
      array[j]--;
    }
    menu_delete_undo(undo);
    break;
  case IDFAPPLY:
    id0 = i;
    /* fall-through */
  case IDOK:
    if (new_file) {
      getobj(obj, "file", array[i], 0, NULL, &name);
      AddDataFileList(name);
    }
    menu_delete_undo(undo);
    rdata->modified = TRUE;
    break;
  case IDCANCEL:
    menu_undo_internal(undo);
    break;
  }
  if (ret == IDFAPPLY) {
    update_file_obj_multi_response_all(obj, array, i + 1, num, id0, new_file);
    rdata->cb(rdata->modified, rdata->user_data);
    return;
  }
  i++;
  if (i < num) {
    FileDialog(data, array[i], i < num - 1);
    rdata->i = i;
    cb->data = NULL;
    rdata->undo = data_save_undo(UNDO_TYPE_DUMMY);
    response_callback_add(data->dialog, update_file_obj_multi_response, NULL, rdata);
    DialogExecute(TopLevel, data->dialog);
  } else {
    rdata->cb(rdata->modified, rdata->user_data);
    g_free(rdata);
  }
}

void
update_file_obj_multi(struct objlist *obj, struct narray *farray, int new_file, response_cb cb, gpointer user_data)
{
  int num, *array, undo;
  struct obj_list_data *data;
  struct update_file_obj_multi_data *rdata;

  num = arraynum(farray);
  if (num < 1) {
    call_cb(0, cb, user_data);
    return;
  }

  array = arraydata(farray);

  data_save_undo(UNDO_TYPE_EDIT);
  data = NgraphApp.FileWin.data.data;

  rdata = g_malloc0(sizeof(*rdata));
  if (rdata == NULL) {
    call_cb(0, cb, user_data);
    return;
  }

  undo = data_save_undo(UNDO_TYPE_DUMMY);

  rdata->i = 0;
  rdata->num = num;
  rdata->array = array;
  rdata->modified = FALSE;
  rdata->undo = undo;
  rdata->new_file = new_file;
  rdata->data = data;
  rdata->obj = obj;
  rdata->cb = cb;
  rdata->user_data = user_data;

  FileDialog(data, array[0], 0 < num - 1);
  response_callback_add(data->dialog, update_file_obj_multi_response, NULL, rdata);
  DialogExecute(TopLevel, data->dialog);
}

static void
file_update_response_response(int ret, gpointer user_data)
{
  struct narray *farray;
  farray = (struct narray *) user_data;
  if (ret) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, DRAW_REDRAW);
  }
  arrayfree(farray);
}

static void
file_update_response(struct response_callback *cb)
{
  struct narray *farray;
  struct SelectDialog *d;
  farray = (struct narray *) cb->data;
  d = (struct SelectDialog *) cb->dialog;
  if (cb->return_value == IDOK) {
    update_file_obj_multi(d->Obj, farray, FALSE, file_update_response_response, farray);
  } else {
    arrayfree(farray);
  }
}

void
CmFileUpdate(void *w, gpointer client_data)
{
  struct objlist *obj;
  struct narray *farray;
  int last;

  if (Menulock || Globallock)
    return;

  if ((obj = chkobject("data")) == NULL)
    return;

  last = chkobjlastinst(obj);
  if (last == -1) {
    return;
  } else if (last == 0) {
    farray = arraynew(sizeof(int));
    arrayadd(farray, &last);
    update_file_obj_multi(obj, farray, FALSE, file_update_response_response, farray);
  } else {
    farray = arraynew(sizeof(int));
    SelectDialog(&DlgSelect, obj, _("data property (multi select)"), FileCB, (struct narray *) farray, NULL);
    response_callback_add(&DlgSelect, file_update_response, NULL, farray);
    DialogExecute(TopLevel, &DlgSelect);
  }
}

static int
check_plot_obj_file(struct objlist *obj)
{
  int last, i, source;

  last = chkobjlastinst(obj);
  for (i = 0; i < last; i++) {
    getobj(obj, "source", i, 0, NULL, &source);
    if (source == DATA_SOURCE_FILE) {
      return i;
    }
  }
  return -1;
}

static void
file_edit_response(struct response_callback *cb)
{
  int i;
  struct objlist *obj;
  struct CopyDialog *d;
  char *name;
  if (cb->return_value != IDOK) {
    return;
  }
  d = (struct CopyDialog *) cb->dialog;
  obj = d->Obj;
  i = d->sel;

  if (i < 0) {
    return;
  }

  if (getobj(obj, "file", i, 0, NULL, &name) == -1) {
    return;
  }

  edit_file(name);
}

void
CmFileEdit(void *w, gpointer client_data)
{
  struct objlist *obj;
  int last;

  if (Menulock || Globallock)
    return;

  if (Menulocal.editor == NULL)
    return;

  if ((obj = chkobject("data")) == NULL)
    return;

  last = check_plot_obj_file(obj);
  if (last == -1) {
    return;
  }
  CopyDialog(&DlgCopy, obj, -1, _("edit data file (single select)"), PlotFileCB);
  response_callback_add(&DlgCopy, file_edit_response, NULL, NULL);
  DialogExecute(TopLevel, &DlgCopy);
}

static void
option_file_def_response_response(int ret, struct objlist *obj, int id, int modified)
{
  char *objs[2];
  if (ret) {
    exeobj(obj, "save_config", id, 0, NULL);
  }
  delobj(obj, id);
  objs[0] = obj->name;
  objs[1] = NULL;
  UpdateAll2(objs, TRUE);
  if (! modified) {
    reset_graph_modified();
  }
}

static void
option_file_def_response(struct response_callback *cb)
{
  int id;
  struct objlist *obj;
  struct FileDialog *d;
  int modified;
  d = (struct FileDialog *) cb->dialog;
  modified = GPOINTER_TO_INT(cb->data);
  obj = d->Obj;
  id = d->Id;
  if (cb->return_value == IDOK) {
    CheckIniFile(option_file_def_response_response, obj, id, modified);
  } else {
    option_file_def_response_response(FALSE, obj, id, modified);
  }
}

void
CmOptionFileDef(void *w, gpointer client_data)
{
  struct objlist *obj;
  int id;

  if (Menulock || Globallock)
    return;

  if ((obj = chkobject("data")) == NULL)
    return;

  id = newobj(obj);
  if (id >= 0) {
    int modified;

    modified = get_graph_modified();
    FileDefDialog(&DlgFileDef, obj, id);
    response_callback_add(&DlgFileDef, option_file_def_response, NULL, GINT_TO_POINTER(modified));
    DialogExecute(TopLevel, &DlgFileDef);
  }
}

static void
FileWinFileEdit(struct obj_list_data *d)
{
  int sel, num;
  char *name;

  if (Menulock || Globallock)
    return;

  if (Menulocal.editor == NULL)
    return;

  sel = d->select;
  num = chkobjlastinst(d->obj);

  if (sel < 0 || sel > num)
    return;

  if (getobj(d->obj, "file", sel, 0, NULL, &name) == -1)
    return;

  edit_file(name);
}

static void
file_edit_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;
  FileWinFileEdit(d);
}

static void
FileWinFileDelete(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;

  sel = columnview_get_active (d->text);;
  num = chkobjlastinst(d->obj);

  if ((sel >= 0) && (sel <= num)) {
    int update;
    data_save_undo(UNDO_TYPE_DELETE);
    delete_file_obj(d, sel);
    num = chkobjlastinst(d->obj);
    update = FALSE;
    if (num < 0) {
      d->select = -1;
      update = TRUE;
    } else if (sel > num) {
      d->select = num;
    } else {
      d->select = sel;
    }
    FileWinUpdate(d, update, DRAW_REDRAW);
    set_graph_modified();
  }
}

static void
file_delete_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data*) client_data;
  FileWinFileDelete(d);
}

static int
file_obj_copy(struct obj_list_data *d)
{
  int sel, id, num;

  if (Menulock || Globallock)
    return -1;

  sel = columnview_get_active (d->text);;
  num = chkobjlastinst(d->obj);

  if ((sel < 0) || (sel > num))
    return -1;

  id = newobj(d->obj);

  if (id < 0)
    return -1;

  copy_file_obj_field(d->obj, id, sel, TRUE);

  return id;
}

static void
FileWinFileCopy(struct obj_list_data *d)
{
  data_save_undo(UNDO_TYPE_COPY);
  d->select = file_obj_copy(d);
  FileWinUpdate(d, FALSE, DRAW_NOTIFY);
}

static void
file_copy_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;
  FileWinFileCopy(d);
}

static void
FileWinFileCopy2(struct obj_list_data *d)
{
  int id, sel, j, num;

  if (Menulock || Globallock)
    return;

  data_save_undo(UNDO_TYPE_COPY);
  sel = columnview_get_active (d->text);;
  id = file_obj_copy(d);
  num = chkobjlastinst(d->obj);

  if (id < 0) {
    d->select = sel;
    FileWinUpdate(d, TRUE, DRAW_NOTIFY);
    return;
  }

  for (j = num; j > sel + 1; j--) {
    moveupobj(d->obj, j);
  }

  d->select = sel + 1;
  FileWinUpdate(d, FALSE, DRAW_NOTIFY);
}

static void
file_copy2_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;
  FileWinFileCopy2(d);
}

struct file_update_data {
  int undo;
  struct obj_list_data *d;
};

static void
file_win_file_update_response(struct response_callback *cb)
{
  struct file_update_data *data;
  struct obj_list_data *d;
  int undo;
  data = (struct file_update_data *) cb->data;
  d = data->d;
  undo = data->undo;
  set_graph_modified();
  switch (cb->return_value) {
  case IDCANCEL:
    menu_undo_internal(undo);
    break;
  default:
    d->update(d, FALSE, DRAW_NOTIFY);
  }
  g_free(data);
}

static void
FileWinFileUpdate(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;
  sel = columnview_get_active (d->text);;
  num = chkobjlastinst(d->obj);

  if ((sel >= 0) && (sel <= num)) {
    int undo;
    GtkWidget *parent;
    struct file_update_data *data;
    undo = data_save_undo(UNDO_TYPE_EDIT);
    d->setup_dialog(d, sel, FALSE);
    d->select = sel;

    parent = TopLevel;
    data = g_malloc0(sizeof(*data));
    if (data == NULL) {
      return;
    }
    data->d = d;
    data->undo = undo;
    response_callback_add(d->dialog, file_win_file_update_response, NULL, data);
    DialogExecute(parent, d->dialog);
  }
}

static void
draw_callback(gpointer user_data)
{
  struct obj_list_data *d;
  d = (struct obj_list_data *) user_data;
  FileWinUpdate(d, FALSE, DRAW_NONE);
}

static void
FileWinFileDraw(struct obj_list_data *d)
{
  int i, sel, hidden, h, num, modified, undo;

  if (Menulock || Globallock)
    return;

  sel = columnview_get_active(d->text);
  num = chkobjlastinst(d->obj);

  modified = FALSE;
  undo = data_save_undo(UNDO_TYPE_EDIT);
  if ((sel >= 0) && (sel <= num)) {
    for (i = 0; i <= num; i++) {
      hidden = (i != sel);
      getobj(d->obj, "hidden", i, 0, NULL, &h);
      putobj(d->obj, "hidden", i, &hidden);
      if (h != hidden) {
	modified = TRUE;
      }
    }
    d->select = sel;
  } else {
    hidden = FALSE;
    for (i = 0; i <= num; i++) {
      getobj(d->obj, "hidden", i, 0, NULL, &h);
      putobj(d->obj, "hidden", i, &hidden);
      if (h != hidden) {
	modified = TRUE;
      }
    }
    d->select = -1;
  }
  if (modified) {
    set_graph_modified();
  } else {
    menu_delete_undo(undo);
  }
  Draw(FALSE, draw_callback, d);
}

static void
file_draw_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data)
{
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;
  FileWinFileDraw(d);
}

void
FileWinUpdate(struct obj_list_data *d, int clear, int draw)
{
  int redraw;
  if (Menulock || Globallock)
    return;

  if (d == NULL)
    return;

  if (list_sub_window_must_rebuild(d)) {
    list_sub_window_build(d);
  } else {
    list_sub_window_set(d);
  }

  if (! clear && d->select >= 0) {
    columnview_set_active (d->text, d->select, TRUE);
  }

  switch (draw) {
  case DRAW_REDRAW:
    getobj(Menulocal.obj, "redraw_flag", 0, 0, NULL, &redraw);
    if (redraw) {
//      NgraphApp.Viewer.allclear = TRUE;
      update_viewer(d);
    } else {
      draw_notify(TRUE);
    }
    break;
  case DRAW_NOTIFY:
    draw_notify(TRUE);
    break;
  }
}

static void
file_win_fit_response(int ret, gpointer user_data)
{
  struct show_fit_dialog_data *data;
  struct objlist *obj, *fitobj;
  int id, fitid, undo;

  data = (struct show_fit_dialog_data *) user_data;
  obj = data->obj;
  id = data->id;
  fitobj = data->fitobj;
  fitid = data->fitid;
  undo = data->undo;
  switch (ret) {
  case IDCANCEL:
    menu_delete_undo(undo);
    break;
  case IDDELETE:
    delobj(fitobj, fitid);
    putobj(obj, "fit", id, NULL);
    break;
  }
  UnFocus();
  g_free(data);
}

static void
FileWinFit(struct obj_list_data *d)
{
  struct objlist *fitobj, *obj2;
  char *fit;
  int sel, fitid = 0, num, undo;
  struct narray iarray;
  GtkWidget *parent;
  struct show_fit_dialog_data *data;

  if (Menulock || Globallock)
    return;

  sel = columnview_get_active (d->text);;
  num = chkobjlastinst(d->obj);

  if (sel < 0 || sel > num)
    return;

  if ((fitobj = chkobject("fit")) == NULL)
    return;

  if (getobj(d->obj, "fit", sel, 0, NULL, &fit) == -1)
    return;

  if (fit) {
    int idnum;
    arrayinit(&iarray, sizeof(int));
    if (getobjilist(fit, &obj2, &iarray, FALSE, NULL)) {
      arraydel(&iarray);
      return;
    }
    idnum = arraynum(&iarray);

    if ((obj2 != fitobj) || (idnum < 1)) {
      if (putobj(d->obj, "fit", sel, NULL) == -1) {
	arraydel(&iarray);
	return;
      }
    } else {
      fitid = arraylast_int(&iarray);
    }
    arraydel(&iarray);
  }

  if (fit == NULL)
    return;

  undo = data_save_undo(UNDO_TYPE_EDIT);
  parent = TopLevel;
  data = g_malloc0(sizeof(*data));
  data->fitobj = fitobj;
  data->obj = d->obj;
  data->fitid = fitid;
  data->id = sel;
  data->undo = undo;
  execute_fit_dialog(parent, d->obj, sel, fitobj, fitid, file_win_fit_response, data);
}

#define MARK_PIX_LINE_WIDTH 2

static void
set_line_style(struct objlist *obj, int id, int ggc)
{
  struct narray *line_style;
  int n;

  getobj(obj, "line_style", id, 0, NULL, &line_style);
  n = arraynum(line_style);
  if (n > 0) {
    int i, *style, *ptr;
    style = g_malloc(sizeof(*style) * n);
    if (style == NULL) {
      GRAlinestyle(ggc, 0, NULL, MARK_PIX_LINE_WIDTH, GRA_LINE_CAP_BUTT, GRA_LINE_JOIN_MITER, 1000);
      return;
    }
    ptr = arraydata(line_style);
    for (i = 0; i < n; i++) {
      style[i] = ptr[i] / 40;
    }
    GRAlinestyle(ggc, n, style, MARK_PIX_LINE_WIDTH, GRA_LINE_CAP_BUTT, GRA_LINE_JOIN_MITER, 1000);
    g_free(style);
  } else {
    GRAlinestyle(ggc, 0, NULL, MARK_PIX_LINE_WIDTH, GRA_LINE_CAP_BUTT, GRA_LINE_JOIN_MITER, 1000);
  }
}

#define CURVE_POINTS_MAX 7

static void
draw_curve(int ggc, double *spx, double *spy, int spnum, int spcond)
{
  double spz[CURVE_POINTS_MAX], spc[6][CURVE_POINTS_MAX], spc2[6];
  int j, k;

  for (j = 0; j < CURVE_POINTS_MAX; j++) {
    spz[j] = j;
  }

  spline(spz, spx, spc[0], spc[1], spc[2], spnum, spcond, spcond, 0, 0);
  spline(spz, spy, spc[3], spc[4], spc[5], spnum, spcond, spcond, 0, 0);
  GRAcurvefirst(ggc, 0, NULL, NULL, NULL, splinedif, splineint, NULL, spx[0], spy[0]);
  for (j = 0; j < spnum - 1; j++) {
    for (k = 0; k < 6; k++) {
      spc2[k] = spc[k][j];
    }
    if (!GRAcurve(ggc, spc2, spx[j], spy[j])) break;
  }
}

static GdkPixbuf *
draw_type_pixbuf(struct objlist *obj, int i)
{
  int ggc, fr, fg, fb, fr2, fg2, fb2,
    type, w, width = 40, height = 20, poly[14], marktype,
    intp, spcond, spnum, lockstate, found, output;
  double spx[CURVE_POINTS_MAX], spy[CURVE_POINTS_MAX];
  cairo_surface_t *pix;
  GdkPixbuf *pixbuf;
  struct objlist *gobj, *robj;
  N_VALUE *inst;
  struct gra2cairo_local *local;

  lockstate = Globallock;
  Globallock = TRUE;

  found = find_gra2gdk_inst(&gobj, &inst, &robj, &output, &local);
  if (! found) {
    return NULL;
  }

  pix = gra2gdk_create_pixmap(local, width, height,
			      Menulocal.bg_r, Menulocal.bg_g, Menulocal.bg_b);
  if (pix == NULL) {
    return NULL;
  }


  getobj(obj, "type", i, 0, NULL, &type);
  getobj(obj, "R", i, 0, NULL, &fr);
  getobj(obj, "G", i, 0, NULL, &fg);
  getobj(obj, "B", i, 0, NULL, &fb);

  getobj(obj, "R2", i, 0, NULL, &fr2);
  getobj(obj, "G2", i, 0, NULL, &fg2);
  getobj(obj, "B2", i, 0, NULL, &fb2);

  ggc = _GRAopen("gra2gdk", "_output",
		 robj, inst, output, -1, -1, -1, NULL, local);
  if (ggc < 0) {
    _GRAclose(ggc);
    g_object_unref(G_OBJECT(pix));
    return NULL;
  }
  GRAview(ggc, 0, 0, width, height, 0);
  GRAcolor(ggc, fr, fg, fb, 255);
  set_line_style(obj, i, ggc);

  switch (type) {
  case PLOT_TYPE_MARK:
    getobj(obj, "mark_type", i, 0, NULL, &marktype);
    GRAmark(ggc, marktype, width / 2, height / 2, height - 2,
	    fr, fg, fb, 255, fr2, fg2, fb2, 255);
    break;
  case PLOT_TYPE_LINE:
    GRAline(ggc, 1, height / 2, width - 1, height / 2);
    break;
  case PLOT_TYPE_POLYGON:
  case PLOT_TYPE_POLYGON_SOLID_FILL:
    poly[0] = 1;
    poly[1] = height / 2;

    poly[2] = width / 4;
    poly[3] = 1;

    poly[4] = width * 3 / 4;
    poly[5] = height - 1;

    poly[6] = width - 1;
    poly[7] = height / 2;

    poly[8] = width * 3 / 4;
    poly[9] = 1;

    poly[10] = width / 4;
    poly[11] = height - 1;

    poly[12] = 1;
    poly[13] = height / 2;
    GRAdrawpoly(ggc, 7, poly, (type == PLOT_TYPE_POLYGON) ? GRA_FILL_MODE_NONE : GRA_FILL_MODE_WINDING);
    break;
  case PLOT_TYPE_CURVE:
    getobj(obj, "interpolation", i, 0, NULL, &intp);
    w = (intp >= 2) ? height : width;
    spx[0] = 1;
    spx[1] = w / 3 + 1;
    spx[2] = w * 2 / 3;
    spx[3] = w - 1;
    spx[4] = w * 2 / 3;
    spx[5] = w / 3 + 1;
    spx[6] = 1;

    spy[0] = height / 2;
    spy[1] = 1;
    spy[2] = height - 1;
    spy[3] = height / 2;
    spy[4] = 1;
    spy[5] = height - 1;
    spy[6] = height / 2;

    if ((intp == 0) || (intp == 2)) {
      spcond = SPLCND2NDDIF;
      spnum = 4;
    } else {
      spcond = SPLCNDPERIODIC;
      spnum = 7;
    }

    draw_curve(ggc, spx, spy, spnum, spcond);
    if (intp >= 2) {
      GRAmoveto(ggc, height, height * 3 / 4);
      GRAtextstyle(ggc, "Serif", GRA_FONT_STYLE_NORMAL, 52, 0, 0);
      GRAouttext(ggc, "B");
    }
    break;
  case PLOT_TYPE_DIAGONAL:
    GRAline(ggc, 1, height - 1, width - 1, 1);
    break;
  case PLOT_TYPE_ARROW:
    GRAline(ggc, 1, height - 1, width - 1, 1);
    poly[0] = width - 8;
    poly[1] = 1;

    poly[2] = width - 1;
    poly[3] = 1;

    poly[4] = width - 5;
    poly[5] = 6;
    GRAdrawpoly(ggc, 3, poly, GRA_FILL_MODE_EVEN_ODD);
    break;
  case PLOT_TYPE_RECTANGLE:
    GRArectangle(ggc, 1, height - 1, width - 1, 1, 0);
    break;
  case PLOT_TYPE_RECTANGLE_FILL:
    GRAcolor(ggc, fr2, fg2, fb2, 255);
    GRArectangle(ggc, 1, height - 1, width - 1, 1, 1);
    GRAcolor(ggc, fr, fg, fb, 255);
    GRArectangle(ggc, 1, height - 1, width - 1, 1, 0);
    break;
  case PLOT_TYPE_RECTANGLE_SOLID_FILL:
    GRArectangle(ggc, 1, height - 1, width - 1, 1, 1);
    break;
  case PLOT_TYPE_ERRORBAR_X:
  case PLOT_TYPE_ERRORBAND_X:
    GRAline(ggc, 1, height / 2, width - 1, height / 2);
    GRAline(ggc, 1, height / 4, 1, height * 3 / 4);
    GRAline(ggc, width - 1, height / 4, width - 1, height * 3 / 4);
    break;
  case PLOT_TYPE_ERRORBAR_Y:
  case PLOT_TYPE_ERRORBAND_Y:
    GRAline(ggc, width / 2, 1, width / 2, height - 1);
    GRAline(ggc, width * 3 / 8, 1, width * 5 / 8, 1);
    GRAline(ggc, width * 3 / 8, height -1, width * 5 / 8, height - 1);
    break;
  case PLOT_TYPE_STAIRCASE_X:
    GRAmoveto(ggc, 1, height - 1);
    GRAlineto(ggc, width / 4, height - 1);
    GRAlineto(ggc, width / 4, height / 2);
    GRAlineto(ggc, width * 3 / 4, height / 2);
    GRAlineto(ggc, width * 3 / 4, 1);
    GRAlineto(ggc, width - 1, 1);
    break;
  case PLOT_TYPE_STAIRCASE_Y:
    GRAmoveto(ggc, 1, height - 1);
    GRAlineto(ggc, 1, height / 2 + 1);
    GRAlineto(ggc, width / 2, height / 2 + 1);
    GRAlineto(ggc, width / 2, height / 4);
    GRAlineto(ggc, width - 1, height / 4);
    GRAlineto(ggc, width - 1, 1);
    break;
  case PLOT_TYPE_BAR_X:
    GRArectangle(ggc, 1, height / 4, width - 1, height * 3 / 4, 0);
    break;
  case PLOT_TYPE_BAR_Y:
    GRArectangle(ggc, width * 3 / 8, 1, width * 5 / 8, height - 1, 0);
    break;
  case PLOT_TYPE_BAR_FILL_X:
    GRAcolor(ggc, fr2, fg2, fb2, 255);
    GRArectangle(ggc, 1, height / 4, width - 1, height * 3 / 4, 1);
    GRAcolor(ggc, fr, fg, fb, 255);
    GRArectangle(ggc, 1, height / 4, width - 1, height * 3 / 4, 0);
    break;
  case PLOT_TYPE_BAR_FILL_Y:
    GRAcolor(ggc, fr2, fg2, fb2, 255);
    GRArectangle(ggc, width * 3 / 8, 1, width * 5 / 8, height - 1, 1);
    GRAcolor(ggc, fr, fg, fb, 255);
    GRArectangle(ggc, width * 3 / 8, 1, width * 5 / 8, height - 1, 0);
    break;
  case PLOT_TYPE_BAR_SOLID_FILL_X:
    GRArectangle(ggc, 1, height / 4, width - 1, height * 3 / 4, 1);
    break;
  case PLOT_TYPE_BAR_SOLID_FILL_Y:
    GRArectangle(ggc, width * 3 / 8, 1, width * 5 / 8, height - 1, 1);
    break;
  case PLOT_TYPE_FIT:
    spx[0] = width * 3 / 6 - 1;
    spx[1] = width * 4 / 6 - 1;
    spx[2] = width * 5 / 6 - 1;
    spx[3] = width - 1;

    spy[0] = height - 1;
    spy[1] = height / 3;
    spy[2] = height * 2 / 3;
    spy[3] = 1;

    draw_curve(ggc, spx, spy, 4, SPLCND2NDDIF);
    GRAmoveto(ggc, 1, height * 3 / 4);
    GRAtextstyle(ggc, "Serif", GRA_FONT_STYLE_NORMAL, 52, 0, 0);
    GRAouttext(ggc, "fit");
    break;
  }

  _GRAclose(ggc);
  gra2cairo_draw_path(local);

  pixbuf = gdk_pixbuf_get_from_surface(pix, 0, 0, width, height);
  cairo_surface_destroy(pix);

  Globallock = lockstate;

  return pixbuf;
}

static char *
get_axis_obj_str(struct objlist *obj, int id, int axis)
{
  int aid;
  N_VALUE *inst;
  struct objlist *aobj;
  char *name, *tmp;

  inst = chkobjinst(obj, id);
  if (inst == NULL) {
    return NULL;
  }

  aid = get_axis_id(obj, inst, &aobj, axis);
  if (aid < 0){
    return NULL;
  }
  getobj(aobj, "group", aid, 0, NULL, &name);
  if (name) {
    tmp = g_strdup(name);
  } else {
    tmp = g_strdup_printf("%d", aid);
  }

  return tmp;
}

const char *
get_plot_info_str(struct objlist *obj, int id, int src)
{
  char *str;

  str = NULL;
  switch (src) {
  case DATA_SOURCE_FILE:
    getobj(obj, "file", id, 0, NULL, &str);
    break;
  case DATA_SOURCE_RANGE:
    getobj(obj, "math_y", id, 0, NULL, &str);
    if (str == NULL) {
      getobj(obj, "math_x", id, 0, NULL, &str);
      if (str == NULL) {
	str = "X";
      }
    }
    break;
  case DATA_SOURCE_ARRAY:
    getobj(obj, "array", id, 0, NULL, &str);
    break;
  }

  return str;
}

static void
bind_axis(struct objlist *obj, int id, const char *field, GtkWidget *w)
{
  char *axis;
  int type;
  type = strcmp (field, "axis_y");
  axis = get_axis_obj_str(obj, id, (type) ? AXIS_X : AXIS_Y);
  if (axis) {
    gtk_label_set_text(GTK_LABEL (w), axis);
    g_free(axis);
  } else {
    gtk_label_set_text(GTK_LABEL (w), NULL);
  }
}

static void
bind_type(struct objlist *obj, int id, const char *field, GtkWidget *w)
{
  GdkPixbuf *pixbuf;
  pixbuf = draw_type_pixbuf(obj, id);
  if (pixbuf) {
    gtk_picture_set_pixbuf (GTK_PICTURE (w), pixbuf);
    g_object_unref(pixbuf);
  }
}

static void
bind_file (struct objlist *obj, int id, const char *field, GtkWidget *w)
{
  int src, masked;
  struct narray *mask, *move;
  const char *str;

  getobj(obj, "source", id, 0, NULL, &src);
  str = get_plot_info_str(obj, id, src);
  gtk_widget_set_tooltip_text (w, str);

  if (str == NULL) {
    gtk_label_set_text (GTK_LABEL (w), FILL_STRING);
    return;
  }

  getobj(obj, "mask", id, 0, NULL, &mask);
  getobj(obj, "move_data", id, 0, NULL, &move);
  masked = ((arraynum(mask) != 0) || (arraynum(move) != 0));

  if (src == DATA_SOURCE_FILE) {
    char *bfile;
    bfile = getbasename(str);
    if (bfile) {
      if (masked) {
	label_set_italic_text (w, bfile);
      } else {
	gtk_label_set_text (GTK_LABEL (w), bfile);
      }
      g_free(bfile);
    } else {
      gtk_label_set_text (GTK_LABEL (w), FILL_STRING);
    }
  } else {
    char *tmpstr;
    tmpstr = g_strescape(str, "\\");
    if (masked) {
      label_set_italic_text (w, tmpstr);
    } else {
      gtk_label_set_text (GTK_LABEL (w), tmpstr);
    }
    g_free (tmpstr);
  }
}

static void
file_math_response(struct response_callback *cb)
{
  if (DlgMath.modified) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, DRAW_REDRAW);
  } else {
    int undo;
    undo = GPOINTER_TO_INT(cb->data);
    menu_delete_undo(undo);
  }
}

void
CmFileMath(void *w, gpointer client_data)
{
  struct objlist *obj;
  int undo;

  if (Menulock || Globallock)
    return;

  obj = chkobject("data");

  if (chkobjlastinst(obj) < 0)
    return;

  undo = menu_save_undo_single(UNDO_TYPE_EDIT, obj->name);
  MathDialog(&DlgMath, obj);
  response_callback_add(&DlgMath, file_math_response, NULL, GINT_TO_POINTER(undo));
  DialogExecute(TopLevel, &DlgMath);
}

static void
get_draw_files_response(struct response_callback *cb)
{
  response_cb save_cb;
  struct SelectDialog *d;
  struct narray *ifarray, *farray;
  d = (struct SelectDialog *) cb->dialog;
  save_cb = (response_cb) cb->data;
  farray = d->sel;
  ifarray = d->isel;
  if (cb->return_value != IDOK) {
    arrayfree(ifarray);
    arraydel(farray);
    save_cb(1, farray);
    return;
  }
  arrayfree(ifarray);
  save_cb(0, farray);
}

static void
GetDrawFiles(struct narray *farray, response_cb cb)
{
  struct objlist *fobj;
  int lastinst;
  struct narray *ifarray;
  int i, a;

  if (farray == NULL) {
    call_cb(1, cb, farray);
    return;
  }

  fobj = chkobject("data");
  if (fobj == NULL) {
    call_cb(1, cb, farray);
    return;
  }

  lastinst = chkobjlastinst(fobj);
  if (lastinst < 0) {
    call_cb(1, cb, farray);
    return;
  }

  ifarray = arraynew(sizeof(int));
  for (i = 0; i <= lastinst; i++) {
    getobj(fobj, "hidden", i, 0, NULL, &a);
    if (!a)
      arrayadd(ifarray, &i);
  }
  SelectDialog(&DlgSelect, fobj, NULL, FileCB, farray, ifarray);
  response_callback_add(&DlgSelect, get_draw_files_response, NULL, cb);
  DialogExecute(TopLevel, &DlgSelect);
}

struct save_data_data
{
  char *file;
  struct narray *farray;
};

static void
save_data_finalize(gpointer user_data)
{
  struct save_data_data *data;

  data = (struct save_data_data *) user_data;
  arrayfree(data->farray);
  g_free(data->file);
  g_free(data);

  ResetStatusBar();
  main_window_redraw();
}

static void
save_data_main(gpointer user_data)
{
  char *file;
  struct narray *farray;
  struct objlist *obj;
  char buf[1024];
  int i, *array, num, onum;
  char *argv[4];
  struct save_data_data *data;

  data = (struct save_data_data *) user_data;
  file = data->file;
  farray = data->farray;

  obj = chkobject("data");
  array = arraydata(farray);
  num = arraynum(farray);
  onum = chkobjlastinst(obj);

  argv[0] = (char *) file;
  argv[1] = (char *) &div;
  argv[3] = NULL;
  for (i = 0; i < num; i++) {
    int append;
    if (array[i] < 0 || array[i] > onum)
      continue;

    snprintf(buf, sizeof(buf), "%d/%d", i, num);
    set_progress(1, buf, 1.0 * (i + 1) / num);

    append = (i == 0) ? FALSE : TRUE;
    argv[2] = (char *) &append;
    if (exeobj(obj, "output_file", array[i], 3, argv)) {
      break;
    }
  }
}

static void
save_data_response(char *file, gpointer user_data)
{
  struct save_data_data *data;
  if (file == NULL) {
    arrayfree((struct narray *) user_data);
    return;
  }
  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    arrayfree((struct narray *) user_data);
    g_free(file);
    return;
  }
  data->file = file;
  data->farray = (struct narray *) user_data;
  SetStatusBar(_("Making data file."));
  ProgressDialogCreate(_("Making data file"), save_data_main, save_data_finalize, data);
}

static void
save_data(struct narray *farray, int div)
{
  int chd;
  chd = Menulocal.changedirectory;
  nGetSaveFileName(TopLevel, _("Data file"), NULL, NULL, NULL, chd, save_data_response, farray);
}

static void
file_save_curve_data_response(struct response_callback *cb)
{
  int div;
  struct narray *farray;

  farray = (struct narray *) cb->data;
  if (cb->return_value != IDOK) {
    arrayfree(farray);
    return;
  }
  div = DlgOutputData.div;
  save_data(farray, div);
}

static void
file_save_data_response(int ret, gpointer user_data)
{
  struct narray *farray;
  struct objlist *obj;
  int i, num, onum, type, div, curve = FALSE, *array;

  farray = (struct narray *) user_data;

  if (ret) {
    arrayfree(farray);
    return;
  }

  obj = chkobject("data");
  if (obj == NULL) {
    arrayfree(farray);
    return;
  }

  onum = chkobjlastinst(obj);
  num = arraynum(farray);

  if (num == 0) {
    arrayfree(farray);
    return;
  }

  array = arraydata(farray);
  for (i = 0; i < num; i++) {
    if (array[i] < 0 || array[i] > onum) {
      continue;
    }

    getobj(obj, "type", array[i], 0, NULL, &type);
    if (type == 3) {
      curve = TRUE;
    }
  }

  div = 10;

  if (curve) {
    OutputDataDialog(&DlgOutputData, div);
    response_callback_add(&DlgOutputData, file_save_curve_data_response, NULL, farray);
    DialogExecute(TopLevel, &DlgOutputData);
  } else {
    save_data(farray, div);
  }
}

void
CmFileSaveData(void *w, gpointer client_data)
{
  struct narray *farray;

  if (Menulock || Globallock)
    return;

  farray = arraynew(sizeof(int));
  if (farray == NULL) {
    return;
  }

  GetDrawFiles(farray, file_save_data_response);
}

static gboolean
filewin_ev_key_down(GtkWidget *w, guint keyval, GdkModifierType state, gpointer user_data)
{
  struct obj_list_data *d;

  g_return_val_if_fail(w != NULL, FALSE);

  if (Menulock || Globallock)
    return TRUE;

  d = (struct obj_list_data *) user_data;

  switch (keyval) {
  case GDK_KEY_Delete:
    FileWinFileDelete(d);
    UnFocus();
    break;
  case GDK_KEY_Return:
    if (state & GDK_SHIFT_MASK) {
      return FALSE;
    }

    FileWinFileUpdate(d);
    UnFocus();
    break;
  case GDK_KEY_Insert:
    if (state & GDK_SHIFT_MASK) {
      FileWinFileCopy2(d);
    } else {
      FileWinFileCopy(d);
    }
    UnFocus();
    break;
  case GDK_KEY_space:
    if (state & GDK_CONTROL_MASK)
      return FALSE;

    FileWinFileDraw(d);
    UnFocus();
    break;
  case GDK_KEY_f:
    if (state & GDK_CONTROL_MASK) {
      FileWinFit(d);
    }
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

static void
popup_show_cb(GtkWidget *widget, gpointer user_data)
{
  int sel, num, source, i;
  struct obj_list_data *d;

  d = (struct obj_list_data *) user_data;

  sel = d->select;
  num = chkobjlastinst(d->obj);
  for (i = 0; i < POPUP_ITEM_NUM; i++) {
    GAction *action;
    action = g_action_map_lookup_action(G_ACTION_MAP(GtkApp), Popup_list[i].name);
    switch (i) {
    case POPUP_ITEM_TOP:
    case POPUP_ITEM_UP:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), sel > 0 && sel <= num);
      break;
    case POPUP_ITEM_DOWN:
    case POPUP_ITEM_BOTTOM:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), sel >= 0 && sel < num);
      break;
    case POPUP_ITEM_ADD_F:
    case POPUP_ITEM_ADD_R:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), TRUE);
      break;
    case POPUP_ITEM_EDIT:
      if (sel >= 0 && sel <= num) {
	getobj(d->obj, "source", sel, 0, NULL, &source);
	g_simple_action_set_enabled(G_SIMPLE_ACTION(action), source == DATA_SOURCE_FILE);
      } else {
	g_simple_action_set_enabled(G_SIMPLE_ACTION(action), FALSE);
      }
      break;
    default:
      g_simple_action_set_enabled(G_SIMPLE_ACTION(action), sel >= 0 && sel <= num);
    }
  }
}

static int
drop_file(const GValue *value, int type)
{
  struct narray *files;

  if (Globallock || Menulock || DnDLock)
    return FALSE;;

  files = get_dropped_files(value);
  if (files == NULL){
    return TRUE;
  }
  return ! data_dropped(files, type);
}

static gboolean
drag_drop_cb(GtkDropTarget *self, const GValue *value, gdouble x, gdouble y, gpointer user_data)
{
  return drop_file(value, GPOINTER_TO_INT(user_data));
}

void
init_dnd_file(struct SubWin *d, int type)
{
  GtkDropTarget *target;

  target = gtk_drop_target_new(GDK_TYPE_FILE_LIST, GDK_ACTION_COPY);
  g_signal_connect(target, "drop", G_CALLBACK(drag_drop_cb), GINT_TO_POINTER(type));
  gtk_widget_add_controller(d->data.data->text, GTK_EVENT_CONTROLLER(target));
}

GtkWidget *
create_data_list(struct SubWin *d)
{
  if (d->Win) {
    return d->Win;
  }

  list_sub_window_create(d, FILE_WIN_COL_NUM, Flist);

  d->data.data->update = FileWinUpdate;
  d->data.data->setup_dialog = FileDialog;
  d->data.data->dialog = &DlgFile;
  d->data.data->ev_key = filewin_ev_key_down;
  d->data.data->delete = delete_file_obj;
  d->data.data->undo_save = data_save_undo;
  d->data.data->obj = chkobject("data");

  sub_win_create_popup_menu(d->data.data, POPUP_ITEM_NUM,  Popup_list, G_CALLBACK(popup_show_cb));

  init_dnd_file(d, FILE_TYPE_DATA);

  return d->Win;
}

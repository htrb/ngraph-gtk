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
#include "gtk_liststore.h"
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

static n_list_store Flist[] = {
  {" ",	        G_TYPE_BOOLEAN, TRUE, TRUE,  "hidden"},
  {"#",		G_TYPE_INT,     TRUE, FALSE, "id"},
  {N_("file/range"),	G_TYPE_STRING,  TRUE, TRUE,  "file", 0, 0, 0, 0, PANGO_ELLIPSIZE_END},
  {"x   ",	G_TYPE_INT,     TRUE, TRUE,  "x",  0, 999, 1, 10},
  {"y   ",	G_TYPE_INT,     TRUE, TRUE,  "y",  0, 999, 1, 10},
  {N_("ax"),	G_TYPE_PARAM,   TRUE, TRUE,  "axis_x"},
  {N_("ay"),	G_TYPE_PARAM,   TRUE, TRUE,  "axis_y"},
  {N_("type"),	G_TYPE_OBJECT,  TRUE, TRUE,  "type"},
  {N_("size"),	G_TYPE_DOUBLE,  TRUE, TRUE,  "mark_size",  0,       SPIN_ENTRY_MAX, 100, 1000},
  {N_("width"),	G_TYPE_DOUBLE,  TRUE, TRUE,  "line_width", 0,       SPIN_ENTRY_MAX, 10,   100},
  {N_("skip"),	G_TYPE_INT,     TRUE, TRUE,  "head_skip",  0,       INT_MAX,         1,    10},
  {N_("step"),	G_TYPE_INT,     TRUE, TRUE,  "read_step",  1,       INT_MAX,         1,    10},
  {N_("final"),	G_TYPE_INT,     TRUE, TRUE,  "final_line", INT_MIN, INT_MAX,    1,    10},
  {N_("num"), 	G_TYPE_INT,     TRUE, FALSE, "data_num"},
  {"^#",	G_TYPE_INT,     TRUE, FALSE, "oid"},
  {"masked",	G_TYPE_INT,     FALSE, FALSE, "masked"},
  {"tip",	G_TYPE_STRING,  FALSE, FALSE, "file"},
  {"not_func",	G_TYPE_INT,     FALSE, FALSE, "source"},
  {"is_file",	G_TYPE_INT,     FALSE, FALSE, "source"},
};

enum {
  FILE_WIN_COL_HIDDEN,
  FILE_WIN_COL_ID,
  FILE_WIN_COL_FILE,
  FILE_WIN_COL_X,
  FILE_WIN_COL_Y,
  FILE_WIN_COL_X_AXIS,
  FILE_WIN_COL_Y_AXIS,
  FILE_WIN_COL_TYPE,
  FILE_WIN_COL_SIZE,
  FILE_WIN_COL_WIDTH,
  FILE_WIN_COL_SKIP,
  FILE_WIN_COL_STEP,
  FILE_WIN_COL_FINAL,
  FILE_WIN_COL_DNUM,
  FILE_WIN_COL_OID,
  FILE_WIN_COL_MASKED,
  FILE_WIN_COL_TIP,
  FILE_WIN_COL_NOT_RANGE,
  FILE_WIN_COL_IS_FILE,
  FILE_WIN_COL_NUM,
};

static void file_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row);
#if GTK_CHECK_VERSION(4, 0, 0)
static void file_delete_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data);
static void file_copy2_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data);
static void file_copy_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data);
static void file_edit_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data);
static void file_draw_popup_func(GSimpleAction *action, GVariant *parameter, gpointer client_data);
#else
static void file_delete_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_copy2_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_copy_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_edit_popup_func(GtkMenuItem *w, gpointer client_data);
static void file_draw_popup_func(GtkMenuItem *w, gpointer client_data);
#endif
static void FileDialogType(GtkWidget *w, gpointer client_data);
#if GTK_CHECK_VERSION(4, 0, 0)
static void create_type_combo_item(GtkWidget *cbox, GtkTreeStore *list, struct objlist *obj, int id);
#else
static void create_type_combo_item(GtkTreeStore *list, struct objlist *obj, int id);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
static void func_entry_focused(GtkEventControllerFocus *ev, gpointer user_data);
#else
static gboolean func_entry_focused(GtkWidget *w, GdkEventFocus *event, gpointer user_data);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
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
#else
static struct subwin_popup_list add_menu_list[] = {
  {N_("_File"),  G_CALLBACK(CmFileOpen), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Range"), G_CALLBACK(CmRangeAdd), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, NULL,  POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {N_("_Recent file"),  NULL, NULL, POP_UP_MENU_ITEM_TYPE_RECENT_DATA},
  {NULL, NULL, NULL, POP_UP_MENU_ITEM_TYPE_END},
};

static struct subwin_popup_list Popup_list[] = {
  {N_("_Add"),        NULL, add_menu_list, POP_UP_MENU_ITEM_TYPE_MENU},
  {NULL, NULL, NULL,  POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {N_("_Duplicate"),  G_CALLBACK(file_copy_popup_func), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("duplicate _Behind"),   G_CALLBACK(file_copy2_popup_func), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Delete"),     G_CALLBACK(file_delete_popup_func), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, NULL,  POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {N_("_Draw"),       G_CALLBACK(file_draw_popup_func), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Properties"), G_CALLBACK(list_sub_window_update), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Instance name"), G_CALLBACK(list_sub_window_object_name), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Edit"),       G_CALLBACK(file_edit_popup_func), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, NULL,  POP_UP_MENU_ITEM_TYPE_SEPARATOR},
  {N_("_Top"),        G_CALLBACK(list_sub_window_move_top), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Up"),         G_CALLBACK(list_sub_window_move_up), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Down"),       G_CALLBACK(list_sub_window_move_down), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {N_("_Bottom"),     G_CALLBACK(list_sub_window_move_last), NULL, POP_UP_MENU_ITEM_TYPE_NORMAL},
  {NULL, NULL, NULL,  POP_UP_MENU_ITEM_TYPE_END},
};

#define POPUP_ITEM_NUM (sizeof(Popup_list) / sizeof(*Popup_list) - 1)
#define POPUP_ITEM_EDIT    9
#define POPUP_ITEM_TOP    11
#define POPUP_ITEM_UP     12
#define POPUP_ITEM_DOWN   13
#define POPUP_ITEM_BOTTOM 14
#endif

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
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_source_completion_add_provider(comp, provider);
#else
  gtk_source_completion_add_provider(comp, provider, NULL);
#endif
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
#if ! GTK_CHECK_VERSION(4, 0, 0)
  g_value_set_boolean(&value, FALSE); /* fix-me: proposals are not
                                       * shown 2nd time in linux if
                                       * TRUE */
  g_object_set_property(G_OBJECT(comp), "remember-info-visibility", &value);
#else
  g_value_set_boolean(&value, TRUE);
  g_object_set_property(G_OBJECT(comp), "select-on-show", &value);
#endif

  lm = gtk_source_language_manager_get_default();
  lang = gtk_source_language_manager_get_language(lm, "ngraph-math");
  gtk_source_buffer_set_language(GTK_SOURCE_BUFFER(buffer), lang);
  gtk_source_buffer_set_highlight_syntax(GTK_SOURCE_BUFFER(buffer), TRUE);

  add_completion_provider_math(source_view);
#if GTK_CHECK_VERSION(4, 0, 0)
  words = gtk_source_completion_words_new(_("current equations"));
#else
  words = gtk_source_completion_words_new(_("current equations"), NULL);
#endif
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
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  gtk_source_buffer_begin_not_undoable_action(GTK_SOURCE_BUFFER(buffer));
#endif
  gtk_text_buffer_set_text(buffer, text, -1);
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  gtk_source_buffer_end_not_undoable_action(GTK_SOURCE_BUFFER(buffer));
#endif
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
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_editable_set_text(GTK_EDITABLE(d->list), text);
#else
    gtk_entry_set_text(GTK_ENTRY(d->list), text);
#endif
    g_free(text);
    break;
  case 1:
#if GTK_CHECK_VERSION(4, 0, 0)
    str = gtk_editable_get_text(GTK_EDITABLE(d->list));
#else
    str = gtk_entry_get_text(GTK_ENTRY(d->list));
#endif
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
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
#endif
    gtk_notebook_append_page(GTK_NOTEBOOK(tab), vbox, title);
    d->list = w;

    title = gtk_label_new(_("multi line"));
    w = create_source_view();
#if GTK_CHECK_VERSION(4, 0, 0)
    swin = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(swin, TRUE);
#else
    swin = gtk_scrolled_window_new(NULL, NULL);
#endif
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);
#else
    gtk_container_add(GTK_CONTAINER(swin), w);
#endif
    gtk_notebook_append_page(GTK_NOTEBOOK(tab), swin, title);
    d->text = w;

    g_signal_connect(tab, "switch-page", G_CALLBACK(MathTextDialogChangeInputType), d);

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), tab);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), tab, TRUE, TRUE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
    gtk_window_set_default_size(GTK_WINDOW(wi), 800, 500);
  }

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

  set_source_style(d->text);
  gtk_window_set_title(GTK_WINDOW(wi), _(label[d->Mode]));
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_editable_set_text(GTK_EDITABLE(d->list), d->Text);
#else
  gtk_entry_set_text(GTK_ENTRY(d->list), d->Text);
#endif
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

static void
MathTextDialogClose(GtkWidget *w, void *data)
{
  struct MathTextDialog *d;
  const char *p;
  char *obuf, *ptr;
  int id;
  GList *id_ptr;

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
#if GTK_CHECK_VERSION(4, 0, 0)
    p = gtk_editable_get_text(GTK_EDITABLE(d->list));
#else
    p = gtk_entry_get_text(GTK_ENTRY(d->list));
#endif
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

  for (id_ptr = d->id_list; id_ptr; id_ptr = id_ptr->next) {
    int r;
    r = list_store_path_get_int(d->tree, id_ptr->data, 0, &id);
    if (r) {
      continue;
    }

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
MathTextDialog(struct MathTextDialog *data, char *text, int mode, struct objlist *obj, GList *list, GtkWidget *tree)
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
  data->id_list = list;
}

static void
set_escaped_str(GtkWidget *list, GtkTreeIter *iter, int col, const char *math)
{
  const char *str;
  char *tmpstr;
  str = CHK_STR(math);
  tmpstr = NULL;
  if (strchr(str, '\n')) {
    tmpstr = g_strescape(str, "\\");
    str = tmpstr;
  }
  list_store_set_string(list, iter, col, str);
  if (tmpstr) {
    g_free(tmpstr);
  }
}

static void
MathDialogSetupItem(GtkWidget *w, struct MathDialog *d)
{
  int i;
  char *math, *field = NULL;
  GtkTreeIter iter;

  list_store_clear(d->list);

  if (d->Mode < 0 || d->Mode >= MATH_FNC_NUM)
    d->Mode = 0;

  field = FieldStr[d->Mode];

  for (i = 0; i <= chkobjlastinst(d->Obj); i++) {
    math = NULL;
    getobj(d->Obj, field, i, 0, NULL, &math);
    list_store_append(d->list, &iter);
    list_store_set_int(d->list, &iter, 0, i);
    set_escaped_str(d->list, &iter, 1, math);
  }

  if (d->Mode >= 0 && d->Mode < MATH_FNC_NUM) {
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->func[d->Mode]), TRUE);
#else
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->func[d->Mode]), TRUE);
#endif
  }
}

static void
MathDialogMode(GtkWidget *w, gpointer client_data)
{
  struct MathDialog *d;
  int i;

#if GTK_CHECK_VERSION(4, 0, 0)
  if (! gtk_check_button_get_active(GTK_CHECK_BUTTON(w)))
    return;
#else
  if (! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(w)))
    return;
#endif

  d = (struct MathDialog *) client_data;

  for (i = 0; i < MATH_FNC_NUM; i++) {
    if (w == d->func[i])
      d->Mode = i;
  }
  MathDialogSetupItem(d->widget, d);
}

#if GTK_CHECK_VERSION(4, 0, 0)
struct math_dialog_list_data {
  GList *list;
  char *buf;
  struct MathDialog *d;
  GtkTreeSelection *gsel;
};

static int
math_dialog_list_respone(struct response_callback *cb)
{
  struct MathDialog *d;
  GList *list, *data;
  struct math_dialog_list_data *res_data;
  GtkTreeSelection *gsel;
  int *ary;

  res_data = (struct math_dialog_list_data *) cb->data;
  d = res_data->d;
  list = res_data->list;
  gsel = res_data->gsel;
  d->modified = DlgMathText.modified;
  g_free(res_data->buf);

  MathDialogSetupItem(d->widget, d);

  for (data = list; data; data = data->next) {
    ary = gtk_tree_path_get_indices(data->data);
    if (ary == NULL) {
      continue;
    }

    gtk_tree_selection_select_path(gsel, data->data);
  }

  g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
  g_free(res_data);
  return IDOK;
}

static void
MathDialogList(GtkButton *w, gpointer client_data)
{
  struct MathDialog *d;
  int a, r;
  char *field = NULL, *buf;
  GtkTreeSelection *gsel;
  GtkTreePath *path;
  GList *list, *data;
  struct math_dialog_list_data *res_data;

  d = (struct MathDialog *) client_data;

  gsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
  list = gtk_tree_selection_get_selected_rows(gsel, NULL);

  if (list == NULL)
    return;

  gtk_tree_view_get_cursor(GTK_TREE_VIEW(d->list), &path, NULL);

  if (path) {
    r = list_store_path_get_int(d->list, path, 0, &a);
    gtk_tree_path_free(path);
  } else {
    data = g_list_last(list);
    r = list_store_path_get_int(d->list, data->data, 0, &a);
  }

  if (r) {
    g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
    return;
  }

  if (d->Mode < 0 || d->Mode >= MATH_FNC_NUM)
    d->Mode = 0;

  field = FieldStr[d->Mode];

  sgetobjfield(d->Obj, a, field, NULL, &buf, FALSE, FALSE, FALSE);
  if (buf == NULL) {
    g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
    return;
  }

  res_data = g_malloc0(sizeof(*res_data));
  if (res_data == NULL) {
    g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
    return;
  }
  res_data->d = d;
  res_data->list = list;
  res_data->buf = buf;

  MathTextDialog(&DlgMathText, buf, d->Mode, d->Obj, list, d->list);
  DlgMathText.response_cb = response_callback_new(math_dialog_list_respone, NULL, res_data);
  DialogExecute(d->widget, &DlgMathText);
}
#else
static void
MathDialogList(GtkButton *w, gpointer client_data)
{
  struct MathDialog *d;
  int a, *ary, r;
  char *field = NULL, *buf;
  GtkTreeSelection *gsel;
  GtkTreePath *path;
  GList *list, *data;

  d = (struct MathDialog *) client_data;

  gsel = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->list));
  list = gtk_tree_selection_get_selected_rows(gsel, NULL);

  if (list == NULL)
    return;

  gtk_tree_view_get_cursor(GTK_TREE_VIEW(d->list), &path, NULL);

  if (path) {
    r = list_store_path_get_int(d->list, path, 0, &a);
    gtk_tree_path_free(path);
  } else {
    data = g_list_last(list);
    r = list_store_path_get_int(d->list, data->data, 0, &a);
  }

  if (r)
    goto END;

  if (d->Mode < 0 || d->Mode >= MATH_FNC_NUM)
    d->Mode = 0;

  field = FieldStr[d->Mode];

  sgetobjfield(d->Obj, a, field, NULL, &buf, FALSE, FALSE, FALSE);
  if (buf == NULL)
    goto END;

  MathTextDialog(&DlgMathText, buf, d->Mode, d->Obj, list, d->list);
  DialogExecute(d->widget, &DlgMathText);
  d->modified = DlgMathText.modified;
  g_free(buf);

  MathDialogSetupItem(d->widget, d);

  for (data = list; data; data = data->next) {
    ary = gtk_tree_path_get_indices(data->data);
    if (ary == NULL)
      continue;

    gtk_tree_selection_select_path(gsel, data->data);
  }

 END:
  g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
}
#endif

static void
math_dialog_activated_cb(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
  struct MathDialog *d;
  GtkTreeSelection *gsel;
  int n;

  d = (struct MathDialog *) user_data;

  gsel = gtk_tree_view_get_selection(view);
  n = gtk_tree_selection_count_selected_rows(gsel);
  if (n < 1)
    return;

  MathDialogList(NULL, d);
}

static gboolean
key_pressed_cb(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
  struct MathDialog *d;

  d = (struct MathDialog *) user_data;
  if (keyval == GDK_KEY_Return) {
    math_dialog_activated_cb(GTK_TREE_VIEW(d->list), NULL, NULL, user_data);
    return TRUE;
  }
  return FALSE;
}

static void
set_btn_sensitivity_delete_cb(GtkTreeModel *tree_model, GtkTreePath *path, gpointer user_data)
{
  int n;
  GtkWidget *w;

  w = GTK_WIDGET(user_data);
  n = gtk_tree_model_iter_n_children(tree_model, NULL);
  gtk_widget_set_sensitive(w, n > 0);
}

static void
set_btn_sensitivity_insert_cb(GtkTreeModel *tree_model, GtkTreePath *path, GtkTreeIter *iter, gpointer user_data)
{
  set_btn_sensitivity_delete_cb(tree_model, path, user_data);
}

static void
set_sensitivity_by_row_num(GtkWidget *tree, GtkWidget *btn)
{
  GtkTreeModel *model;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(tree));
  g_signal_connect(model, "row-deleted", G_CALLBACK(set_btn_sensitivity_delete_cb), btn);
  g_signal_connect(model, "row-inserted", G_CALLBACK(set_btn_sensitivity_insert_cb), btn);
  gtk_widget_set_sensitive(btn, FALSE);
}

static gboolean
set_btn_sensitivity_selection_cb(GtkTreeSelection *sel, gpointer user_data)
{
  int n;
  GtkWidget *w;

  w = GTK_WIDGET(user_data);
  n = gtk_tree_selection_count_selected_rows(sel);
  gtk_widget_set_sensitive(w, n > 0);

  return FALSE;
}

static void
set_sensitivity_by_selection(GtkWidget *tree, GtkWidget *btn)
{
  GtkTreeSelection *sel;

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(tree));
  g_signal_connect(sel, "changed", G_CALLBACK(set_btn_sensitivity_selection_cb), btn);
  gtk_widget_set_sensitive(btn, FALSE);
}

static void
MathDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct MathDialog *d;

  d = (struct MathDialog *) data;

  if (makewidget) {
    int i;
    GtkWidget *w, *swin, *vbox, *hbox;
    static n_list_store list[] = {
      {"id",       G_TYPE_INT,    TRUE, FALSE, NULL},
      {N_("math"), G_TYPE_STRING, TRUE, FALSE, NULL, 0, 0, 0, 0, PANGO_ELLIPSIZE_END},
    };
    char *button_str[] = {
      N_("_X math"),
      N_("_Y math"),
      "_F(X, Y, Z)",
      "_G(X, Y, Z)",
      "_H(X, Y, Z)",
    };
#if GTK_CHECK_VERSION(4, 0, 0)
    GtkWidget *group = NULL;
#endif

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), hbox);
#else
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
#endif

    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_sort_all(w);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    add_event_key(w, G_CALLBACK(key_pressed_cb), NULL,  d);
    g_signal_connect(w, "row-activated", G_CALLBACK(math_dialog_activated_cb), d);
    d->list = w;

#if GTK_CHECK_VERSION(4, 0, 0)
    swin = gtk_scrolled_window_new();
    gtk_widget_set_vexpand(swin, TRUE);
#else
    swin = gtk_scrolled_window_new(NULL, NULL);
#endif
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);
#else
    gtk_container_add(GTK_CONTAINER(swin), w);
#endif

    w = gtk_frame_new(NULL);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_frame_set_child(GTK_FRAME(w), swin);
    gtk_box_append(GTK_BOX(vbox), w);
#else
    gtk_container_add(GTK_CONTAINER(w), swin);
    gtk_box_pack_start(GTK_BOX(vbox), w, TRUE, TRUE, 4);
#endif

    w = NULL;
    for (i = 0; i < MATH_FNC_NUM; i++) {
#if GTK_CHECK_VERSION(4, 0, 0)
      w = gtk_check_button_new_with_mnemonic(_(button_str[i]));
      if (group) {
	gtk_check_button_set_group(GTK_CHECK_BUTTON(w), GTK_CHECK_BUTTON(group));
      } else {
	group = w;
      }
      gtk_box_append(GTK_BOX(hbox), w);
#else
      w = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(w), _(button_str[i]));
      gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif
      d->func[i] = w;
      g_signal_connect(w, "toggled", G_CALLBACK(MathDialogMode), d);
    }

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    w = gtk_button_new_with_mnemonic(_("Select _All"));
    set_button_icon(w, "edit-select-all");
    g_signal_connect(w, "clicked", G_CALLBACK(list_store_select_all_cb), d->list);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), w);
#else
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif
    set_sensitivity_by_row_num(d->list, w);

    w = gtk_button_new_with_mnemonic(_("_Edit"));
    g_signal_connect(w, "clicked", G_CALLBACK(MathDialogList), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), w);
    gtk_box_append(GTK_BOX(vbox), hbox);
#else
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
#endif
    set_sensitivity_by_selection(d->list, w);

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), vbox);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, TRUE, TRUE, 4);
#endif

    d->show_cancel = FALSE;
    d->ok_button = _("_Close");

    gtk_window_set_default_size(GTK_WINDOW(wi), -1, 300);
#if ! GTK_CHECK_VERSION(4, 0, 0)
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif

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
FitLoadDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  char *s;
  struct FitLoadDialog *d;
  int i;
  GtkWidget *w;

  d = (struct FitLoadDialog *) data;
  if (makewidget) {
    w = combo_box_create();
    d->list = w;
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), w);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), w, FALSE, FALSE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
  }
  combo_box_clear(d->list);
  for (i = d->Sid; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    combo_box_append_text(d->list, CHK_STR(s));
  }
  combo_box_set_active(d->list, 0);
  /*
  if (makewidget) {
    XtManageChild(d->widget);
    d->widget = NULL;
    XtVaSetValues(d->list, XmNwidth, 200, NULL);
  }
  */
}

static void
FitLoadDialogClose(GtkWidget *w, void *data)
{
  struct FitLoadDialog *d;

  d = (struct FitLoadDialog *) data;
  if (d->ret == IDCANCEL)
    return;
  d->sel = combo_box_get_active(d->list);
}

void
FitLoadDialog(struct FitLoadDialog *data, struct objlist *obj, int sid)
{
  data->SetupWindow = FitLoadDialogSetup;
  data->CloseWindow = FitLoadDialogClose;
  data->Obj = obj;
  data->Sid = sid;
  data->sel = -1;
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
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), hbox);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
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
FitDialogCopy(GtkButton *btn, gpointer user_data)
{
  struct FitDialog *d;
  int sel;

  d = (struct FitDialog *) user_data;
  sel = CopyClick(d->widget, d->Obj, d->Id, FitCB);
  if (sel != -1)
    FitDialogSetupItem(d->widget, d, sel);
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

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
static int
file_dialog_load_response(struct response_callback *cb)
{
  struct FitDialog *d;
  d = (struct FitDialog *) cb->data;
  if ((cb->return_value == IDOK) && (DlgFitLoad.sel >= 0)) {
    int id;
    id = DlgFitLoad.sel + d->Lastid + 1;
    FitDialogSetupItem(d->widget, d, id);
  }
  return IDOK;
}

static void
FitDialogLoad(GtkButton *btn, gpointer user_data)
{
  struct FitDialog *d;
  int lastid;

  d = (struct FitDialog *) user_data;

  if (!FitDialogLoadConfig(d, TRUE))
    return;

  lastid = chkobjlastinst(d->Obj);
  if ((d->Lastid < 0) || (lastid == d->Lastid)) {
    message_box(d->widget, _("No settings."), FITSAVE, RESPONS_OK);
    return;
  }

  FitLoadDialog(&DlgFitLoad, d->Obj, d->Lastid + 1);
  DlgMathText.response_cb = response_callback_new(file_dialog_load_response, NULL, d);
  DialogExecute(d->widget, &DlgFitLoad);
}
#else
static void
FitDialogLoad(GtkButton *btn, gpointer user_data)
{
  struct FitDialog *d;
  int lastid;

  d = (struct FitDialog *) user_data;

  if (!FitDialogLoadConfig(d, TRUE))
    return;

  lastid = chkobjlastinst(d->Obj);
  if ((d->Lastid < 0) || (lastid == d->Lastid)) {
    message_box(d->widget, _("No settings."), FITSAVE, RESPONS_OK);
    return;
  }

  FitLoadDialog(&DlgFitLoad, d->Obj, d->Lastid + 1);
  if ((DialogExecute(d->widget, &DlgFitLoad) == IDOK)
      && (DlgFitLoad.sel >= 0)) {
    int id;
    id = DlgFitLoad.sel + d->Lastid + 1;
    FitDialogSetupItem(d->widget, d, id);
  }
}
#endif

static int
copy_settings_to_fitobj(struct FitDialog *d, char *profile)
{
  int i, id, num;
  char *s;

  for (i = d->Lastid + 1; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    if (s && strcmp(s, profile) == 0) {
      if (message_box(d->widget, _("Overwrite existing profile?"), "Confirm",
		     RESPONS_YESNO) != IDYES) {
	return 1;
      }
      break;
    }
  }

  if (i > chkobjlastinst(d->Obj)) {
    id = newobj(d->Obj);
  } else {
    id = i;
  }

  if (putobj(d->Obj, "profile", id, profile) == -1)
    return 1;

  if (SetObjFieldFromWidget(d->type, d->Obj, id, "type"))
    return 1;

  num = combo_box_get_active(d->dim);
  num++;
  if (num > 0 && putobj(d->Obj, "poly_dimension", id, &num) == -1)
    return 1;

  if (SetObjFieldFromWidget(d->weight, d->Obj, id, "weight_func"))
    return 1;

  if (SetObjFieldFromWidget
      (d->through_point, d->Obj, id, "through_point"))
    return 1;

  if (SetObjFieldFromWidget(d->x, d->Obj, id, "point_x"))
    return 1;

  if (SetObjFieldFromWidget(d->y, d->Obj, id, "point_y"))
    return 1;

  if (SetObjFieldFromWidget(d->min, d->Obj, id, "min"))
    return 1;

  if (SetObjFieldFromWidget(d->max, d->Obj, id, "max"))
    return 1;

  if (SetObjFieldFromWidget(d->div, d->Obj, id, "div"))
    return 1;

  if (SetObjFieldFromWidget(d->interpolation, d->Obj, id,
			    "interpolation"))
    return 1;
  if (SetObjFieldFromWidget(d->formula, d->Obj, id, "user_func"))
    return 1;

  if (SetObjFieldFromWidget(d->derivatives, d->Obj, id, "derivative"))
    return 1;

  if (SetObjFieldFromWidget(d->converge, d->Obj, id, "converge"))
    return 1;

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "parameter0", dd[] = "derivative0";

    p[sizeof(p) - 2] += i;
    dd[sizeof(dd) - 2] += i;

    if (SetObjFieldFromWidget(d->p[i], d->Obj, id, p))
      return 1;

    if (SetObjFieldFromWidget(d->d[i], d->Obj, id, dd))
      return 1;
  }

  return 0;
}

static int
delete_fitobj(struct FitDialog *d, char *profile)
{
  int i, r;
  char *s, *ptr;

  if (profile == NULL)
    return 1;

  for (i = d->Lastid + 1; i <= chkobjlastinst(d->Obj); i++) {
    getobj(d->Obj, "profile", i, 0, NULL, &s);
    if (s && strcmp(s, profile) == 0) {
      ptr = g_strdup_printf(_("Delete the profile '%s'?"), profile);
      r = message_box(d->widget, ptr, "Confirm", RESPONS_YESNO);
      g_free(ptr);
      if (r != IDYES) {
	return 1;
      }
      break;
    }
  }

  if (i > chkobjlastinst(d->Obj)) {
    ptr = g_strdup_printf(_("The profile '%s' is not exist."), profile);
    message_box(d->widget, ptr, "Confirm", RESPONS_OK);
    g_free(ptr);
    return 1;
  }

  delobj(d->Obj, i);

  return 0;
}

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
static int
fit_dialog_save_response(struct response_callback *cb)
{
  char *s, *ngpfile;
  int error;
  int hFile;
  struct FitDialog *d;

  d = (struct FitDialog *) cb->data;
  if (cb->return_value != IDOK && cb->return_value != IDDELETE)
    return IDOK;

  if (DlgFitSave.Profile == NULL)
    return IDOK;

  if (DlgFitSave.Profile[0] == '\0') {
    g_free(DlgFitSave.Profile);
    return IDOK;
  }

  switch (cb->return_value) {
  case IDOK:
    if (copy_settings_to_fitobj(d, DlgFitSave.Profile)) {
      g_free(DlgFitSave.Profile);
      return IDOK;
    }
    break;
  case IDDELETE:
    if (delete_fitobj(d, DlgFitSave.Profile)) {
      g_free(DlgFitSave.Profile);
      return IDOK;
    }
    break;
  }

  ngpfile = getscriptname(FITSAVE);
  if (ngpfile == NULL) {
    return IDOK;
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
    switch (cb->return_value) {
    case IDOK:
      ptr = g_strdup_printf(_("The profile '%s' is saved."), DlgFitSave.Profile);
      message_box(d->widget, ptr, "Confirm", RESPONS_OK);
      g_free(ptr);
      break;
    case IDDELETE:
      ptr = g_strdup_printf(_("The profile '%s' is deleted."), DlgFitSave.Profile);
      message_box(d->widget, ptr, "Confirm", RESPONS_OK);
      g_free(ptr);
      g_free(DlgFitSave.Profile);
      break;
    }
  }

  g_free(ngpfile);
  return IDOK;
}

static void
FitDialogSave(GtkWidget *w, gpointer client_data)
{
  struct FitDialog *d;

  d = (struct FitDialog *) client_data;

  if (!FitDialogLoadConfig(d, FALSE))
    return;

  FitSaveDialog(&DlgFitSave, d->Obj, d->Lastid + 1);

  DlgFitSave.response_cb = response_callback_new(fit_dialog_save_response, NULL, d);
  DialogExecute(d->widget, &DlgFitSave);
}
#else
static void
FitDialogSave(GtkWidget *w, gpointer client_data)
{
  int r;
  char *s, *ngpfile;
  int error;
  int hFile;
  struct FitDialog *d;

  d = (struct FitDialog *) client_data;

  if (!FitDialogLoadConfig(d, FALSE))
    return;

  FitSaveDialog(&DlgFitSave, d->Obj, d->Lastid + 1);

  r = DialogExecute(d->widget, &DlgFitSave);
  if (r != IDOK && r != IDDELETE)
    return;

  if (DlgFitSave.Profile == NULL)
    return;

  if (DlgFitSave.Profile[0] == '\0') {
    g_free(DlgFitSave.Profile);
    return;
  }

  switch (r) {
  case IDOK:
    if (copy_settings_to_fitobj(d, DlgFitSave.Profile)) {
      g_free(DlgFitSave.Profile);
      return;
    }
    break;
  case IDDELETE:
    if (delete_fitobj(d, DlgFitSave.Profile)) {
      g_free(DlgFitSave.Profile);
      return;
    }
    break;
  }

  ngpfile = getscriptname(FITSAVE);
  if (ngpfile == NULL) {
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
    switch (r) {
    case IDOK:
      ptr = g_strdup_printf(_("The profile '%s' is saved."), DlgFitSave.Profile);
      message_box(d->widget, ptr, "Confirm", RESPONS_OK);
      g_free(ptr);
      break;
    case IDDELETE:
      ptr = g_strdup_printf(_("The profile '%s' is deleted."), DlgFitSave.Profile);
      message_box(d->widget, ptr, "Confirm", RESPONS_OK);
      g_free(ptr);
      g_free(DlgFitSave.Profile);
      break;
    }
  }

  g_free(ngpfile);
}
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
  math = gtk_editable_get_text(GTK_EDITABLE(d->formula));
#else
  math = gtk_entry_get_text(GTK_ENTRY(d->formula));
#endif
  if (math_equation_parse(code, math)) {
    math_equation_free(code);
    return FALSE;
  }

  prm = math_equation_get_parameter(code, 0, NULL);
  dim = prm->id_num;
#if GTK_CHECK_VERSION(4, 0, 0)
  deriv = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->derivatives));
#else
  deriv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->derivatives));
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
  s = gtk_editable_get_text(GTK_EDITABLE(d->formula));
#else
 s = gtk_entry_get_text(GTK_ENTRY(d->formula));
#endif
  entry_completion_append(NgraphApp.fit_list, s);

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "parameter0", dd[] = "derivative0";

    p[sizeof(p) - 2] += i;
    dd[sizeof(dd) - 2] += i;

    if (SetObjFieldFromWidget(d->p[i], d->Obj, d->Id, p))
      return FALSE;

    if (SetObjFieldFromWidget(d->d[i], d->Obj, d->Id, dd))
      return FALSE;

#if GTK_CHECK_VERSION(4, 0, 0)
    s = gtk_editable_get_text(GTK_EDITABLE(d->d[i]));
#else
    s = gtk_entry_get_text(GTK_ENTRY(d->d[i]));
#endif
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
  Draw(FALSE);
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
#if GTK_CHECK_VERSION(4, 0, 0)
  through = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->through_point));
#else
  through = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->through_point));
#endif

  switch (type) {
  case FIT_TYPE_POLY:
    dim = combo_box_get_active(d->dim);

    if (dim == 0) {
      gtk_label_set_markup(GTK_LABEL(d->func_label), "Equation: Y=<i>a·</i>X<i>+b</i>");
    } else {
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
#if GTK_CHECK_VERSION(4, 0, 0)
    deriv = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->derivatives));
#else
    deriv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->derivatives));
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
static void
add_focus_in_event(GtkWidget *w, gpointer user_data)
{
  GtkEventController *ev;
  ev = gtk_event_controller_focus_new();
  g_signal_connect(ev, "enter", G_CALLBACK(func_entry_focused), user_data);
  gtk_widget_add_controller(w, ev);
}
#endif

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
#if GTK_CHECK_VERSION(4, 0, 0)
  add_focus_in_event(w, NgraphApp.fit_list);
#else
  g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), NgraphApp.fit_list);
#endif
  g_signal_connect(w, "changed", G_CALLBACK(check_fit_func), d);
  d->formula = w;

  w = create_text_entry(TRUE, TRUE);
  add_widget_to_table_sub(table, w, _("_Converge (%):"), TRUE, 0, 1, 3, j);
  d->converge = w;

  w = gtk_check_button_new_with_mnemonic(_("_Derivatives"));
  add_widget_to_table_sub(table, w, NULL, FALSE, 2, 1, 3, j++);
  d->derivatives = w;

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), table);
#else
  gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 0);
#endif

  table = gtk_grid_new();

  for (i = 0; i < FIT_PARM_NUM; i++) {
    char p[] = "%0_0:", dd[] = "dF/d(%0_0):";

    p[sizeof(p) - 3] += i;
    dd[sizeof(dd) - 4] += i;

    w = create_text_entry(TRUE, TRUE);
    add_widget_to_table_sub(table, w, p, TRUE, 0, 1, 4, j);
    d->p[i] = w;

    w = create_text_entry(TRUE, TRUE);
#if GTK_CHECK_VERSION(4, 0, 0)
    add_focus_in_event(w, NgraphApp.fit_list);
#else
    g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), NgraphApp.fit_list);
#endif
    add_widget_to_table_sub(table, w, dd, TRUE, 2, 1, 4, j++);
    d->d[i] = w;
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  w = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(w, TRUE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(w), table);
  gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(w), FALSE);
#else
  w = gtk_scrolled_window_new(NULL, NULL);
  gtk_container_add(GTK_CONTAINER(w), table);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(w), GTK_SHADOW_NONE);
#endif
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(w), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_widget_set_size_request(GTK_WIDGET(w), -1, 200);
#if ! GTK_CHECK_VERSION(4, 0, 0)
  gtk_container_set_border_width(GTK_CONTAINER(w), 2);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, TRUE, TRUE, 0);
#endif
  d->usr_def_prm_tbl = table;

  w = gtk_frame_new(_("User definition"));
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(w), vbox);
#else
  gtk_container_add(GTK_CONTAINER(w), vbox);
#endif

  return w;
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
    gtk_widget_set_valign(w, GTK_ALIGN_END);
    d->func_label = w;

    w = create_text_entry(TRUE, TRUE);
    add_widget_to_table_sub(table, w, _("_Weight:"), TRUE, 0, 4, 5, 1);
    d->weight = w;

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), table);
#else
    gtk_box_pack_start(GTK_BOX(vbox), table, FALSE, FALSE, 4);
#endif


    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    w = gtk_check_button_new_with_mnemonic(_("_Through"));
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), w);
#else
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif
    d->through_point = w;

    hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox2, w, "_X:", TRUE);
    d->x = w;

    w = create_text_entry(TRUE, TRUE);
    item_setup(hbox2, w, "_Y:", TRUE);
    d->y = w;

    d->through_box = hbox2;

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), hbox2);
    gtk_box_append(GTK_BOX(vbox), hbox);
#else
    gtk_box_pack_start(GTK_BOX(hbox), hbox2, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
#endif


    frame = gtk_frame_new(_("Action"));
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_frame_set_child(GTK_FRAME(frame), vbox);
    gtk_box_append(GTK_BOX(d->vbox), frame);
#else
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);
#endif

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
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), w);
#else
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif
    d->interpolation = w;

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), hbox);
#else
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
#endif

    frame = gtk_frame_new(_("Draw X range"));
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_frame_set_child(GTK_FRAME(frame), vbox);
    gtk_box_append(GTK_BOX(d->vbox), frame);
#else
    gtk_container_add(GTK_CONTAINER(frame), vbox);
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, FALSE, FALSE, 4);
#endif


    frame = create_user_fit_frame(d);
    d->usr_def_frame = frame;
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), frame);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), frame, TRUE, TRUE, 4);
#endif


    hbox = add_copy_button_to_box(GTK_WIDGET(d->vbox), G_CALLBACK(FitDialogCopy), d, "fit");

    w = gtk_button_new_with_mnemonic(_("_Load"));
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogLoad), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), w);
#else
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif

    w = gtk_button_new_with_mnemonic(_("_Save"));
    set_button_icon(w, "document-save");
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogSave), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), w);
#else
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif


    w = gtk_button_new_with_mnemonic(_("_Draw"));
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), w);
#else
    gtk_box_pack_end(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogDraw), d);

    w = gtk_button_new_with_mnemonic(_("_Result"));
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), w);
#else
    gtk_box_pack_end(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif
    g_signal_connect(w, "clicked", G_CALLBACK(FitDialogResult), d);


    g_signal_connect(d->dim, "changed", G_CALLBACK(FitDialogSetSensitivity), d);
    g_signal_connect(d->type, "changed", G_CALLBACK(FitDialogSetSensitivity), d);
    g_signal_connect(d->through_point, "toggled", G_CALLBACK(FitDialogSetSensitivity), d);
    g_signal_connect(d->derivatives, "toggled", G_CALLBACK(FitDialogSetSensitivity), d);

#if ! GTK_CHECK_VERSION(4, 0, 0)
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
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
  GtkTreeIter iter;

  list_store_clear(d->move.list);

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
  for (j = 0; j < movenum; j++) {
    int line;
    double x, y;
    char buf[64];
    line = arraynget_int(move, j);
    x = arraynget_double(movex, j);
    y = arraynget_double(movey, j);

    list_store_append(d->move.list, &iter);
    list_store_set_int(d->move.list, &iter, 0, line);

    snprintf(buf, sizeof(buf), DOUBLE_STR_FORMAT, x);
    list_store_set_string(d->move.list, &iter, 1, buf);

    snprintf(buf, sizeof(buf), DOUBLE_STR_FORMAT, y);
    list_store_set_string(d->move.list, &iter, 2, buf);
  }
}

static void
FileMoveDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  int a;
  double x, y;
  const char *buf;
  char *endptr, buf2[64];
  GtkTreeIter iter;

  d = (struct FileDialog *) client_data;

  a = spin_entry_get_val(d->move.line);

#if GTK_CHECK_VERSION(4, 0, 0)
  buf = gtk_editable_get_text(GTK_EDITABLE(d->move.x));
#else
  buf = gtk_entry_get_text(GTK_ENTRY(d->move.x));
#endif
  if (buf[0] == '\0') return;

  x = strtod(buf, &endptr);
  if (x != x || x == HUGE_VAL || x == - HUGE_VAL || endptr[0] != '\0')
    return;

#if GTK_CHECK_VERSION(4, 0, 0)
  buf = gtk_editable_get_text(GTK_EDITABLE(d->move.y));
#else
  buf = gtk_entry_get_text(GTK_ENTRY(d->move.y));
#endif
  if (buf[0] == '\0') return;

  y = strtod(buf, &endptr);
  if (y != y || y == HUGE_VAL || y == - HUGE_VAL || endptr[0] != '\0')
    return;

  list_store_append(d->move.list, &iter);
  list_store_set_int(d->move.list, &iter, 0, a);

  snprintf(buf2, sizeof(buf2), DOUBLE_STR_FORMAT, x);
  list_store_set_string(d->move.list, &iter, 1, buf2);

  snprintf(buf2, sizeof(buf2), DOUBLE_STR_FORMAT, y);
  list_store_set_string(d->move.list, &iter, 2, buf2);

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_editable_set_text(GTK_EDITABLE(d->move.x), "");
  gtk_editable_set_text(GTK_EDITABLE(d->move.y), "");
#else
  gtk_entry_set_text(GTK_ENTRY(d->move.x), "");
  gtk_entry_set_text(GTK_ENTRY(d->move.y), "");
#endif
  d->move.changed = TRUE;
}

static void
FileMoveDialogRemove(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) client_data;

  list_store_remove_selected_cb(w, d->move.list);
  d->move.changed = TRUE;
}

static void
move_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  int sel;

  d = (struct FileDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);

  if (sel != -1) {
    move_tab_setup_item(d, sel);
    d->move.changed = TRUE;
  }
}

static GtkWidget *
move_tab_create(struct FileDialog *d)
{
  GtkWidget *w, *hbox, *swin, *table, *vbox;
  n_list_store list[] = {
    {N_("Line No."), G_TYPE_INT,    TRUE, FALSE, NULL},
    {"X",            G_TYPE_STRING, TRUE, FALSE, NULL},
    {"Y",            G_TYPE_STRING, TRUE, FALSE, NULL},
  };
  int i;

#if GTK_CHECK_VERSION(4, 0, 0)
  swin = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(swin, TRUE);
  gtk_widget_set_hexpand(swin, TRUE);
  gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(swin), TRUE);
#else
  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin), GTK_SHADOW_ETCHED_IN);
#endif
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  w = list_store_create(sizeof(list) / sizeof(*list), list);
  list_store_set_sort_column(w, 0);
  list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
  d->move.list = w;
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);
#else
  gtk_container_add(GTK_CONTAINER(swin), w);
#endif
  set_widget_margin(swin, WIDGET_MARGIN_TOP | WIDGET_MARGIN_BOTTOM);

  table = gtk_grid_new();

  i = 0;
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_NATURAL, TRUE, FALSE);
#if ! GTK_CHECK_VERSION(4, 0, 0)
  g_signal_connect(w, "activate", G_CALLBACK(FileMoveDialogAdd), d);
#endif
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
  set_sensitivity_by_selection(d->move.list, w);

  w = gtk_button_new_with_mnemonic(_("Select _All"));
  set_button_icon(w, "edit-select-all");
  add_widget_to_table(table, w, NULL, FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(list_store_select_all_cb), d->move.list);
  set_sensitivity_by_row_num(d->move.list, w);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), table);
  gtk_box_append(GTK_BOX(hbox), swin);
#else
  gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hbox), swin, TRUE, TRUE, 4);
#endif

  w = gtk_frame_new(NULL);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(w), hbox);
#else
  gtk_container_add(GTK_CONTAINER(w), hbox);
#endif
  set_widget_margin(w, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, TRUE, TRUE, 4);
#endif

  add_copy_button_to_box(vbox, G_CALLBACK(move_tab_copy), d, "data");

  return vbox;
}

static int
move_tab_set_value(struct FileDialog *d)
{
  int line, a;
  double x, y;
  struct narray *move, *movex, *movey;
  GtkTreeIter iter;
  gboolean state;

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

  state = list_store_get_iter_first(d->move.list, &iter);
  while (state) {
    unsigned int j, movenum;
    char *ptr, *endptr;
    a = list_store_get_int(d->move.list, &iter, 0);

    ptr = list_store_get_string(d->move.list, &iter, 1);
    x = strtod(ptr, &endptr);
    g_free(ptr);

    ptr = list_store_get_string(d->move.list, &iter, 2);
    y = strtod(ptr, &endptr);
    g_free(ptr);

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
      arrayadd(movex, &x);
      arrayadd(movey, &y);
    }

    state = list_store_iter_next(d->move.list, &iter);
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
  GtkTreeIter iter;

  list_store_clear(d->mask.list);
  getobj(d->Obj, "mask", id, 0, NULL, &mask);
  if ((masknum = arraynum(mask)) > 0) {
    int j;
    for (j = 0; j < masknum; j++) {
      int line;
      line = arraynget_int(mask, j);
      list_store_append(d->mask.list, &iter);
      list_store_set_int(d->mask.list, &iter, 0, line);
    }
  }
}

static void
FileMaskDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  int a;
  GtkTreeIter iter;

  d = (struct FileDialog *) client_data;

  a = spin_entry_get_val(d->mask.line);
  list_store_append(d->mask.list, &iter);
  list_store_set_int(d->mask.list, &iter, 0, a);
  d->mask.changed = TRUE;
}

static void
mask_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  int sel;

  d = (struct FileDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);

  if (sel != -1) {
    mask_tab_setup_item(d, sel);
    d->mask.changed = TRUE;
  }
}

static void
FileMaskDialogRemove(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  d = (struct FileDialog *) client_data;

  list_store_remove_selected_cb(w, d->mask.list);
  d->mask.changed = TRUE;
}

static GtkWidget *
mask_tab_create(struct FileDialog *d)
{
  GtkWidget *w, *swin, *hbox, *table, *vbox, *frame;
  n_list_store list[] = {
    {_("Line No."), G_TYPE_INT, TRUE, FALSE, NULL},
  };
  int i;

  table = gtk_grid_new();

  i = 0;
  w = create_spin_entry_type(SPIN_BUTTON_TYPE_NATURAL, TRUE, FALSE);
#if ! GTK_CHECK_VERSION(4, 0, 0)
  g_signal_connect(w, "activate", G_CALLBACK(FileMaskDialogAdd), d);
#endif
  add_widget_to_table(table, w, _("_Line:"), FALSE, i++);
  d->mask.line = w;

#if GTK_CHECK_VERSION(4, 0, 0)
  swin = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(swin, TRUE);
  gtk_widget_set_hexpand(swin, TRUE);
  gtk_scrolled_window_set_has_frame(GTK_SCROLLED_WINDOW(swin), TRUE);
#else
  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_shadow_type(GTK_SCROLLED_WINDOW(swin), GTK_SHADOW_ETCHED_IN);
#endif
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  w = list_store_create(sizeof(list) / sizeof(*list), list);
  list_store_set_sort_column(w, 0);
  list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
  d->mask.list = w;
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);
#else
  gtk_container_add(GTK_CONTAINER(swin), w);
#endif
  set_widget_margin(swin, WIDGET_MARGIN_TOP | WIDGET_MARGIN_BOTTOM);

  w = gtk_button_new_with_mnemonic(_("_Add"));
  set_button_icon(w, "list-add");
  add_widget_to_table(table, w, "", FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(FileMaskDialogAdd), d);

  w = gtk_button_new_with_mnemonic(_("_Remove"));
  set_button_icon(w, "list-remove");
  add_widget_to_table(table, w, NULL, FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(FileMaskDialogRemove), d);
  set_sensitivity_by_selection(d->mask.list, w);

  w = gtk_button_new_with_mnemonic(_("Select _All"));
  set_button_icon(w, "edit-select-all");
  add_widget_to_table(table, w, NULL, FALSE, i++);
  g_signal_connect(w, "clicked", G_CALLBACK(list_store_select_all_cb), d->mask.list);
  set_sensitivity_by_row_num(d->mask.list, w);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), table);
  gtk_box_append(GTK_BOX(hbox), swin);
#else
  gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hbox), swin, TRUE, TRUE, 4);
#endif

  frame = gtk_frame_new(NULL);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(frame), hbox);
#else
  gtk_container_add(GTK_CONTAINER(frame), hbox);
#endif
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), frame);
#else
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 4);
#endif

  add_copy_button_to_box(vbox, G_CALLBACK(mask_tab_copy), d, "data");

  return vbox;
}

static int
mask_tab_set_value(struct FileDialog *d)
{
  int a;
  struct narray *mask;
  GtkTreeIter iter;
  gboolean state;

  if (d->mask.changed == FALSE) {
    return 0;
  }

  getobj(d->Obj, "mask", d->Id, 0, NULL, &mask);
  if (mask) {
    putobj(d->Obj, "mask", d->Id, NULL);
    mask = NULL;
  }

  state = list_store_get_iter_first(d->mask.list, &iter);
  while (state) {
    a = list_store_get_int(d->mask.list, &iter, 0);
    if (mask == NULL)
      mask = arraynew(sizeof(int));

    arrayadd(mask, &a);
    state = list_store_iter_next(d->mask.list, &iter);
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
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_editable_set_text(GTK_EDITABLE(d->load.ifs), s);
#else
  gtk_entry_set_text(GTK_ENTRY(d->load.ifs), s);
#endif
  g_free(s);
  g_free(ifs);
}

static void
load_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  int sel;

  d = (struct FileDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);
  if (sel != -1) {
    load_tab_setup_item(d, sel);
  }
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
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(frame), table);
  gtk_widget_set_vexpand(frame, TRUE);
#else
  gtk_container_add(GTK_CONTAINER(frame), table);
#endif
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), frame);
#else
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 4);
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
  ifs = gtk_editable_get_text(GTK_EDITABLE(d->load.ifs));
#else
  ifs = gtk_entry_get_text(GTK_ENTRY(d->load.ifs));
#endif
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
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_editable_set_text(GTK_EDITABLE(entry), str);
#else
  gtk_entry_set_text(GTK_ENTRY(entry), str);
#endif
  g_free(str);
}

static void
copy_entry_to_text(GtkWidget *text, GtkWidget *entry)
{
  const gchar *str;

#if GTK_CHECK_VERSION(4, 0, 0)
  str = gtk_editable_get_text(GTK_EDITABLE(entry));
#else
  str = gtk_entry_get_text(GTK_ENTRY(entry));
#endif
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
math_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  int sel;

  d = (struct FileDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);

  if (sel != -1) {
    math_tab_setup_item(d, sel);
  }
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
func_entry_focused(GtkEventControllerFocus *ev, gpointer user_data)
{
  GtkEntryCompletion *compl;
  GtkWidget *w;

  compl = GTK_ENTRY_COMPLETION(user_data);
  w = gtk_event_controller_get_widget(GTK_EVENT_CONTROLLER(ev));
  entry_completion_set_entry(compl, w);
}
#else
static gboolean
func_entry_focused(GtkWidget *w, GdkEventFocus *event, gpointer user_data)
{
  GtkEntryCompletion *compl;

  compl = GTK_ENTRY_COMPLETION(user_data);
  entry_completion_set_entry(compl, w);

  return FALSE;
}
#endif

static GtkWidget *
create_math_text_tab(GtkWidget *tab, const gchar *label)
{
  GtkWidget *w, *title, *swin;

  w = create_source_view();
#if GTK_CHECK_VERSION(4, 0, 0)
  swin = gtk_scrolled_window_new();
#else
  swin = gtk_scrolled_window_new(NULL, NULL);
#endif
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);
#else
  gtk_container_add(GTK_CONTAINER(swin), w);
#endif
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
#if GTK_CHECK_VERSION(4, 0, 0)
  add_focus_in_event(w, NgraphApp.func_list);
#else
  g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), NgraphApp.func_list);
#endif
  add_widget_to_table(table, w, "_F(X,Y,Z):", TRUE, i++);
  d->math.f = w;

  w = create_text_entry(TRUE, TRUE);
#if GTK_CHECK_VERSION(4, 0, 0)
  add_focus_in_event(w, NgraphApp.func_list);
#else
  g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), NgraphApp.func_list);
#endif
  add_widget_to_table(table, w, "_G(X,Y,Z):", TRUE, i++);
  d->math.g = w;

  w = create_text_entry(TRUE, TRUE);
#if GTK_CHECK_VERSION(4, 0, 0)
  add_focus_in_event(w, NgraphApp.func_list);
#else
  g_signal_connect(w, "focus-in-event", G_CALLBACK(func_entry_focused), NgraphApp.func_list);
#endif
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
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(frame), table);
#else
  gtk_container_add(GTK_CONTAINER(frame), table);
#endif
  set_widget_margin(frame, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), frame);
#else
  gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 4);
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
  s = gtk_editable_get_text(GTK_EDITABLE(d->math.y));
#else
  s = gtk_entry_get_text(GTK_ENTRY(d->math.y));
#endif
  entry_completion_append(NgraphApp.y_math_list, s);

#if GTK_CHECK_VERSION(4, 0, 0)
  s = gtk_editable_get_text(GTK_EDITABLE(d->math.x));
#else
  s = gtk_entry_get_text(GTK_ENTRY(d->math.x));
#endif
  entry_completion_append(NgraphApp.x_math_list, s);

#if GTK_CHECK_VERSION(4, 0, 0)
  s = gtk_editable_get_text(GTK_EDITABLE(d->math.f));
#else
  s = gtk_entry_get_text(GTK_ENTRY(d->math.f));
#endif
  entry_completion_append(NgraphApp.func_list, s);

#if GTK_CHECK_VERSION(4, 0, 0)
  s = gtk_editable_get_text(GTK_EDITABLE(d->math.g));
#else
  s = gtk_entry_get_text(GTK_ENTRY(d->math.g));
#endif
  entry_completion_append(NgraphApp.func_list, s);

#if GTK_CHECK_VERSION(4, 0, 0)
  s = gtk_editable_get_text(GTK_EDITABLE(d->math.h));
#else
  s = gtk_entry_get_text(GTK_ENTRY(d->math.h));
#endif
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
  if (type < 0 || type >= MARK_TYPE_NUM) {
    type = 0;
  }

  if (NgraphApp.markpix[type]) {
    GtkWidget *img;
    char buf[64];
    GdkPixbuf *pixbuf;
    pixbuf = gdk_pixbuf_get_from_surface(NgraphApp.markpix[type],
					 0, 0, MARK_PIX_SIZE, MARK_PIX_SIZE);
    img = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
    if (img) {
#if GTK_CHECK_VERSION(4, 0, 0)
      gtk_button_set_child(GTK_BUTTON(w), img);
#else
      gtk_button_set_image(GTK_BUTTON(w), img);
#endif
    }
    snprintf(buf, sizeof(buf), "%02d", type);
    gtk_widget_set_tooltip_text(w, buf);
  }
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
      gtk_widget_add_css_class(w, TOOLBUTTON_CLASS);
      button_set_mark_image(w, type);
      g_signal_connect(w, "clicked", G_CALLBACK(MarkDialogCB), d);
      d->toggle[type] = w;
      gtk_grid_attach(GTK_GRID(grid), w, type % COL, type / COL, 1, 1);
    }
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), grid);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), grid, FALSE, FALSE, 4);

    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
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

#if GTK_CHECK_VERSION(4, 0, 0)
struct file_dialog_mark_data {
  struct FileDialog *d;
  GtkWidget *w;
};

static int
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
  return IDOK;
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
  d->mark.response_cb = response_callback_new(file_dialog_mark_response, NULL, data);
  DialogExecute(d->widget, &(d->mark));
}
#else
static void
FileDialogMark(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) client_data;
  DialogExecute(d->widget, &(d->mark));
  button_set_mark_image(w, d->mark.Type);
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
struct execute_fit_dialog_data {
  struct objlist *fileobj;
  int fileid;
  int save_type;
  response_cb cb;
  gpointer user_data;
};

static int
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
  return cb->return_value;
}

static int
execute_fit_dialog(GtkWidget *w, struct objlist *fileobj, int fileid, struct objlist *fitobj, int fitid, response_cb cb, gpointer user_data)
{
  int save_type, type;
  struct execute_fit_dialog_data *data;

  type = PLOT_TYPE_FIT;
  getobj(fileobj, "type", fileid, 0, NULL, &save_type);
  putobj(fileobj, "type", fileid, &type);

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return IDOK;
  }
  data->fileobj = fileobj;
  data->fileid = fileid;
  data->save_type = save_type;
  data->cb = cb;
  data->user_data = user_data;
  FitDialog(&DlgFit, fitobj, fitid);
  DlgFit.response_cb = response_callback_new(execute_fit_dialog_response, NULL, data);
  DialogExecute(w, &DlgFit);
  return IDOK;
}
#else
static int
execute_fit_dialog(GtkWidget *w, struct objlist *fileobj, int fileid, struct objlist *fitobj, int fitid)
{
  int save_type, type, ret;

  type = PLOT_TYPE_FIT;
  getobj(fileobj, "type", fileid, 0, NULL, &save_type);
  putobj(fileobj, "type", fileid, &type);

  FitDialog(&DlgFit, fitobj, fitid);
  ret = DialogExecute(w, &DlgFit);

  putobj(fileobj, "type", fileid, &save_type);

  return ret;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
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
#endif

static int
#if GTK_CHECK_VERSION(4, 0, 0)
show_fit_dialog(struct objlist *obj, int id, GtkWidget *parent, response_cb cb, gpointer user_data)
#else
show_fit_dialog(struct objlist *obj, int id, GtkWidget *parent)
#endif
{
  struct objlist *fitobj, *robj;
  char *fit;
  int fitid = 0, fitoid, ret, create = FALSE;
  struct narray iarray;
#if GTK_CHECK_VERSION(4, 0, 0)
  struct show_fit_dialog_data *data;
#endif

  if ((fitobj = chkobject("fit")) == NULL)
    return -1;

  if (getobj(obj, "fit", id, 0, NULL, &fit) == -1)
    return -1;

  if (fit) {
    int idnum;
    arrayinit(&iarray, sizeof(int));
    if (getobjilist(fit, &robj, &iarray, FALSE, NULL))
      return -1;

    idnum = arraynum(&iarray);
    if ((robj != fitobj) || (idnum < 1)) {
      if (putobj(obj, "fit", id, NULL) == -1) {
	arraydel(&iarray);
	return -1;
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

    if ((fit = mkobjlist(fitobj, NULL, fitoid, NULL, TRUE)) == NULL)
      return -1;

    if (putobj(obj, "fit", id, fit) == -1) {
      g_free(fit);
      return -1;
    }
    create = TRUE;
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  data = g_malloc0(sizeof(*data));
  data->fitobj = fitobj;
  data->obj = obj;
  data->fitid = fitid;
  data->id = id;
  data->create = create;
  data->cb = cb;
  data->data = user_data;
  ret = execute_fit_dialog(parent, obj, id, fitobj, fitid, show_fit_dialog_response, data);
#else
  ret = execute_fit_dialog(parent, obj, id, fitobj, fitid);

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
#endif

  return ret;
}

#if GTK_CHECK_VERSION(4, 0, 0)
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
#else
static void
FileDialogFit(GtkWidget *w, gpointer client_data)
{
  struct FileDialog *d;
  char *valstr;

  d = (struct FileDialog *) client_data;

  show_fit_dialog(d->Obj, d->Id, d->widget);

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
#endif

static void
plot_tab_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  int sel;

  d = (struct FileDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);
  if (sel != -1) {
    plot_tab_setup_item(d, sel);
  }
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

#if GTK_CHECK_VERSION(4, 0, 0)
  file = gtk_editable_get_text(GTK_EDITABLE(d->file));
#else
  file = gtk_entry_get_text(GTK_ENTRY(d->file));
#endif
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
file_settings_copy(GtkButton *btn, gpointer user_data)
{
  struct FileDialog *d;
  int sel;

  d = (struct FileDialog *) user_data;

  sel = CopyClick(d->widget, d->Obj, d->Id, FileCB);
  if (sel != -1) {
    file_setup_item(d, sel);
  }
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
  g_signal_connect(w, "changed", G_CALLBACK(FileDialogType), d);

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

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), table);
#else
  gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 4);
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), table);
#else
  gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 4);
#endif

  w = gtk_frame_new(NULL);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(w), hbox);
  gtk_widget_set_vexpand(w, TRUE);
#else
  gtk_container_add(GTK_CONTAINER(w), hbox);
#endif
  set_widget_margin(w, WIDGET_MARGIN_LEFT | WIDGET_MARGIN_RIGHT);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, TRUE, TRUE, 4);
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox2), hbox);
#else
  gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 4);
#endif


  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  if (d->source != DATA_SOURCE_RANGE) {
    w = create_spin_entry(0, FILE_OBJ_MAXCOL, 1, FALSE, TRUE);
    item_setup(hbox, w, _("_Y column:"), TRUE);
    d->ycol = w;
  }

  w = axis_combo_box_create(AXIS_COMBO_BOX_NONE);
  item_setup(hbox, w, _("_Y axis:"), TRUE);
  d->yaxis = w;

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox2), hbox);
#else
  gtk_box_pack_start(GTK_BOX(vbox2), hbox, FALSE, FALSE, 4);
#endif

  add_copy_button_to_box(vbox2, G_CALLBACK(file_settings_copy), d, "data");

  hbox = gtk_grid_new();
  gtk_grid_set_column_spacing(GTK_GRID(hbox), 4);
  d->comment_box = hbox;
  frame = gtk_frame_new(NULL);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(frame), vbox2);
#else
  gtk_container_add(GTK_CONTAINER(frame), vbox2);
#endif

  gtk_widget_set_hexpand(frame, FALSE);
  gtk_grid_attach(GTK_GRID(hbox), frame, 0, 0, 1, 1);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(d->vbox), hbox);
#else
  gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(d->vbox), notebook);
#else
  gtk_box_pack_start(GTK_BOX(d->vbox), notebook, TRUE, TRUE, 4);
#endif
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
#define HEADLINE_FIRST_CHAR_COLUMN (MAX_COLS + 0)
#define HEADLINE_LINE_NUM_COLUMN   (MAX_COLS + 1)
#define HEADLINE_VISIBILITY_COLUMN (MAX_COLS + 2)
#define HEADLINE_ELLIPSIZE_COLUMN  (MAX_COLS + 3)
#define HEADLINE_FONT_COLUMN       (MAX_COLS + 4)
#define HEADLINE_COLUMN_NUM        (MAX_COLS + 5)

static void
set_headline_table_header(struct FileDialog *d)
{
  int x, y, type, i;
  GString *str;

  type = combo_box_get_active(d->type);
  x = spin_entry_get_val(d->xcol);
  y = spin_entry_get_val(d->ycol);

  str = g_string_new("");
  if (str == NULL) {
    return;
  }

  for (i = 0; i < MAX_COLS; i++) {
    GtkTreeViewColumn *col;

    g_string_set_size(str, 0);

    col = gtk_tree_view_get_column(GTK_TREE_VIEW(d->comment_table), i);

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
	g_string_append(str, "Ex1");
	break;
      }
    } else if (i == x + 2) {
      switch (type) {
      case PLOT_TYPE_ERRORBAR_X:
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
	g_string_append(str, "Ey1");
	break;
      }
    } else if (i == y + 2) {
      switch (type) {
      case PLOT_TYPE_ERRORBAR_Y:
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
    gtk_tree_view_column_set_title(col, str->str);
    //    gtk_tree_view_column_set_visible(col, i < max_col);
  }
  g_string_free(str, TRUE);
}

static void
set_headline_table_array(struct FileDialog *d, int max_lines)
{
  struct array_prm ary;
  int i, j, l, m, n, skip, step;
  char *array, buf[64];
  GtkListStore *model;

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

  model = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(d->comment_table)));
  gtk_list_store_clear(model);
  gtk_tree_view_set_model(GTK_TREE_VIEW(d->comment_table), NULL);

  n = (ary.data_num > max_lines) ? max_lines : ary.data_num;
  m = (ary.col_num < MAX_COLS) ? ary.col_num : MAX_COLS;
  l = 1;
  for (i = 0; i < n; i++) {
    GtkTreeIter iter;
    int v;

    gtk_list_store_append(model, &iter);
    for (j = 0; j < m; j++) {
      void *ptr;
      ptr = arraynget(ary.ary[j], i);
      if (ptr) {
        double val;
	val = * (double *) ptr;
	snprintf(buf, sizeof(buf), "%G", val);
	gtk_list_store_set(model, &iter, j + 1, buf, -1);
      }
    }
    v = CHECK_VISIBILITY_ARRAY(i, skip, step);
    gtk_list_store_set(model, &iter,
		       0, l,
		       HEADLINE_LINE_NUM_COLUMN, i,
		       HEADLINE_VISIBILITY_COLUMN, v,
		       HEADLINE_ELLIPSIZE_COLUMN, (v) ? PANGO_ELLIPSIZE_NONE : PANGO_ELLIPSIZE_END,
		       HEADLINE_FONT_COLUMN, Menulocal.file_preview_font,
		       -1);
    if (v) {
      l++;
    }
  }

  gtk_tree_view_set_model(GTK_TREE_VIEW(d->comment_table), GTK_TREE_MODEL(model));
}

static void
set_headline_table(struct FileDialog *d, char *s, int max_lines)
{
  struct narray *lines;
  int i, j, l, n, skip, step, csv;
  const char *tmp, *remark, *po;
  GString *ifs;
  GtkListStore *model;

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

#if GTK_CHECK_VERSION(4, 0, 0)
  csv = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->load.csv));
#else
  csv = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->load.csv));
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  remark = gtk_editable_get_text(GTK_EDITABLE(d->load.remark));
#else
  remark = gtk_entry_get_text(GTK_ENTRY(d->load.remark));
#endif
  if (remark == NULL) {
    remark = "";
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  tmp = gtk_editable_get_text(GTK_EDITABLE(d->load.ifs));
#else
  tmp = gtk_entry_get_text(GTK_ENTRY(d->load.ifs));
#endif
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

  model = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(d->comment_table)));
  gtk_list_store_clear(model);
  gtk_tree_view_set_model(GTK_TREE_VIEW(d->comment_table), NULL);

  l = 1;
  for (i = 0; i < n; i++) {
    GtkTreeIter iter;
    int m, c, v;
    const char *str;

    gtk_list_store_append(model, &iter);
    m = arraynum(lines + i);
    m = (m < MAX_COLS) ? m : MAX_COLS;
    for (j = 0; j < m; j++) {
      gtk_list_store_set(model, &iter, j + 1, arraynget_str(lines + i, j), -1);
    }
    str = arraynget_str(lines + i, 0);
    if (str) {
      c = (g_ascii_isprint(str[0]) || g_ascii_isspace(str[0])) ? str[0] : 0;
    } else {
      c = 0;
    }
    v = CHECK_VISIBILITY(i, skip, step, remark, c);
    gtk_list_store_set(model, &iter,
		       0, l,
		       HEADLINE_LINE_NUM_COLUMN, i,
		       HEADLINE_FIRST_CHAR_COLUMN, c,
		       HEADLINE_VISIBILITY_COLUMN, v,
		       HEADLINE_ELLIPSIZE_COLUMN, (v) ? PANGO_ELLIPSIZE_NONE : PANGO_ELLIPSIZE_END,
		       HEADLINE_FONT_COLUMN, Menulocal.file_preview_font,
		       -1);
    if (v) {
      l++;
    }
  }

  gtk_tree_view_set_model(GTK_TREE_VIEW(d->comment_table), GTK_TREE_MODEL(model));

 exit:
  for (i = 0; i < n; i++) {
    arraydel2(lines + i);
  }
  g_free(lines);
}

static GtkWidget *
create_preview_table(struct FileDialog *d)
{
  GtkWidget *view;
  GtkListStore *model;
  GType *types;
  int i;

  view = gtk_tree_view_new();
  gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(view), GTK_TREE_VIEW_GRID_LINES_BOTH);
  types = g_malloc(sizeof(*types) * HEADLINE_COLUMN_NUM);
  if (types == NULL) {
    return NULL;
  }

  for (i = 1; i < MAX_COLS; i++) {
    types[i] = G_TYPE_STRING;
  }
  types[0] = G_TYPE_INT;
  types[HEADLINE_LINE_NUM_COLUMN] = G_TYPE_INT;
  types[HEADLINE_FIRST_CHAR_COLUMN] = G_TYPE_INT;
  types[HEADLINE_VISIBILITY_COLUMN] = G_TYPE_BOOLEAN;
  types[HEADLINE_ELLIPSIZE_COLUMN] = G_TYPE_INT;
  types[HEADLINE_FONT_COLUMN] = G_TYPE_STRING;

  model = gtk_list_store_newv(HEADLINE_COLUMN_NUM, types);

  g_free(types);

  gtk_tree_view_set_model(GTK_TREE_VIEW(view), GTK_TREE_MODEL(model));

  for (i = 0; i < MAX_COLS; i++) {
    char buf[32];
    GtkCellRenderer *cell;
    GtkTreeViewColumn *column;

    snprintf(buf, sizeof(buf), "%%%d", i);
    cell = gtk_cell_renderer_text_new();
    column = gtk_tree_view_column_new_with_attributes(buf, cell,
						      "text", i,
						      "sensitive", HEADLINE_VISIBILITY_COLUMN,
						      "ellipsize", HEADLINE_ELLIPSIZE_COLUMN,
						      "font", HEADLINE_FONT_COLUMN,
						      NULL);
    if (i == 0) {
      gtk_tree_view_column_add_attribute(column, cell, "visible", HEADLINE_VISIBILITY_COLUMN);
    }

    g_object_set((GObject *) cell, "xalign", (gfloat) 1.0, NULL);
    gtk_tree_view_column_set_alignment(column, 0.5);
    gtk_tree_view_column_set_sizing(column, GTK_TREE_VIEW_COLUMN_AUTOSIZE);
    gtk_tree_view_append_column(GTK_TREE_VIEW(view), column);
  }

  return view;
}

static void
update_table(GtkEditable *editable, gpointer user_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) user_data;
  set_headline_table(d, d->head_lines, Menulocal.data_head_lines);
}

static void
update_table_visibility(GtkEditable *editable, gpointer user_data)
{
  struct FileDialog *d;
  GtkTreeModel *model;
  GtkTreeIter iter;
  const char *remark;
  int skip, step, ln, fc, v, i;

  d = (struct FileDialog *) user_data;

  skip = spin_entry_get_val(d->load.headskip);
  if (skip < 0) {
    skip = 0;
  }

  step = spin_entry_get_val(d->load.readstep);
  if (step < 1) {
    step = 1;
  }

  remark = NULL;
  if (d->source == DATA_SOURCE_FILE) {
#if GTK_CHECK_VERSION(4, 0, 0)
    remark = gtk_editable_get_text(GTK_EDITABLE(d->load.remark));
#else
    remark = gtk_entry_get_text(GTK_ENTRY(d->load.remark));
#endif
  }
  if (remark == NULL) {
    remark = "";
  }

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(d->comment_table));

  if (! gtk_tree_model_get_iter_first(model, &iter)) {
    return;
  }

  i = 1;
  do {
    gtk_tree_model_get(model, &iter,
		       HEADLINE_LINE_NUM_COLUMN, &ln,
		       HEADLINE_FIRST_CHAR_COLUMN, &fc, -1);

    if (d->source == DATA_SOURCE_FILE) {
      v = CHECK_VISIBILITY(ln, skip, step, remark, fc);
    } else {
      v = CHECK_VISIBILITY_ARRAY(ln, skip, step);
    }
    gtk_list_store_set(GTK_LIST_STORE(model), &iter,
		       0, i,
		       HEADLINE_VISIBILITY_COLUMN, v,
		       HEADLINE_ELLIPSIZE_COLUMN, (v) ? PANGO_ELLIPSIZE_NONE : PANGO_ELLIPSIZE_END,
		       HEADLINE_FONT_COLUMN, Menulocal.file_preview_font,
		       -1);
    if (v) {
      i++;
    }
  } while (gtk_tree_model_iter_next(model, &iter));
}

static void
update_table_header(GtkEditable *editable, gpointer user_data)
{
  struct FileDialog *d;

  d = (struct FileDialog *) user_data;
  set_headline_table_header(d);
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
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), w);
#else
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif
    d->load_settings = w;
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogOption), d);

    w = gtk_button_new_with_mnemonic(_("_Edit"));
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(hbox), w);
#else
    gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogEdit), d);


#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), hbox);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
#endif


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
#if GTK_CHECK_VERSION(4, 0, 0)
      swin = gtk_scrolled_window_new();
#else
      swin = gtk_scrolled_window_new(NULL, NULL);
#endif
      gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(4, 0, 0)
      gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), view);
#else
      gtk_container_add(GTK_CONTAINER(swin), view);
#endif
      gtk_notebook_append_page(GTK_NOTEBOOK(w), swin, label);
    }
    d->comment_table = view;

    view = create_text_view_with_line_number(&d->comment_view);
    label = gtk_label_new_with_mnemonic(_("_Plain"));
    gtk_notebook_append_page(GTK_NOTEBOOK(w), view, label);

    g_signal_connect(d->load.ifs, "changed", G_CALLBACK(update_table), d);
    g_signal_connect(d->load.csv, "toggled", G_CALLBACK(update_table), d);

    g_signal_connect(d->load.remark, "changed", G_CALLBACK(update_table_visibility), d);
    g_signal_connect(d->load.readstep, "changed", G_CALLBACK(update_table_visibility), d);
    g_signal_connect(d->load.headskip, "changed", G_CALLBACK(update_table_visibility), d);

    g_signal_connect(d->xcol, "changed", G_CALLBACK(update_table_header), d);
    g_signal_connect(d->ycol, "changed", G_CALLBACK(update_table_header), d);
    g_signal_connect(d->type, "changed", G_CALLBACK(update_table_header), d);

    gtk_grid_attach(GTK_GRID(d->comment_box), w, 1, 0, 1, 1);
    w = gtk_button_new_with_label(_("Create"));
    add_widget_to_table(d->fit_table, w, _("_Fit:"), FALSE, d->fit_row);
    d->fit = w;
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogFit), d);
#if ! GTK_CHECK_VERSION(4, 0, 0)
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
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

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), hbox);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);
#endif

    FileDialogSetupCommon(wi, d);

    w = mask_tab_create(d);
    label = gtk_label_new_with_mnemonic(_("_Mask"));
    d->mask.tab_id = gtk_notebook_append_page(d->tab, w, label);

    w = move_tab_create(d);
    label = gtk_label_new_with_mnemonic(_("_Move"));
    d->move.tab_id = gtk_notebook_append_page(d->tab, w, label);

    view = create_preview_table(d);
#if GTK_CHECK_VERSION(4, 0, 0)
    swin = gtk_scrolled_window_new();
#else
    swin = gtk_scrolled_window_new(NULL, NULL);
#endif
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), view);
#else
    gtk_container_add(GTK_CONTAINER(swin), view);
#endif
    d->comment_table = view;

    g_signal_connect(d->load.readstep, "changed", G_CALLBACK(update_table_visibility), d);
    g_signal_connect(d->load.headskip, "changed", G_CALLBACK(update_table_visibility), d);

    g_signal_connect(d->xcol, "changed", G_CALLBACK(update_table_header), d);
    g_signal_connect(d->ycol, "changed", G_CALLBACK(update_table_header), d);
    g_signal_connect(d->type, "changed", G_CALLBACK(update_table_header), d);

    gtk_widget_set_hexpand(swin, TRUE);
    gtk_widget_set_vexpand(swin, TRUE);
    gtk_grid_attach(GTK_GRID(d->comment_box), swin, 1, 0, 1, 1);
    w = gtk_button_new_with_label(_("Create"));
    add_widget_to_table(d->fit_table, w, _("_Fit:"), FALSE, d->fit_row);
    d->fit = w;
    g_signal_connect(w, "clicked", G_CALLBACK(FileDialogFit), d);
#if ! GTK_CHECK_VERSION(4, 0, 0)
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
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
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_editable_set_width_chars(GTK_EDITABLE(w), RANGE_ENTRY_WIDTH);
#else
    gtk_entry_set_width_chars(GTK_ENTRY(w), RANGE_ENTRY_WIDTH);
#endif
    add_widget_to_table(table, w, _("_Minimum:"), FALSE, i++);
    d->min = w;

    w = create_text_entry(FALSE, TRUE);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_editable_set_width_chars(GTK_EDITABLE(w), RANGE_ENTRY_WIDTH);
#else
    gtk_entry_set_width_chars(GTK_ENTRY(w), RANGE_ENTRY_WIDTH);
#endif
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
#if ! GTK_CHECK_VERSION(4, 0, 0)
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
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
#if ! GTK_CHECK_VERSION(4, 0, 0)
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
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

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
struct load_data_data
{
  int undo;
  char *fname;
  struct obj_list_data *data;
};

static int
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
  return IDOK;
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
  ((struct DialogType *) data->dialog)->response_cb = response_callback_new(load_data_response, NULL, res_data);
  DialogExecute(TopLevel, data->dialog);
}
#else
void
CmFileHistory(GtkRecentChooser *w, gpointer client_data)
{
  int ret;
  char *name, *fname;
  int id, undo;
  struct objlist *obj;
  char *uri;
  struct obj_list_data *data;

  if (Menulock || Globallock) {
    return;
  }

  uri = gtk_recent_chooser_get_current_uri(w);
  if (uri == NULL) {
    return;
  }

  name = g_filename_from_uri(uri, NULL, NULL);
  g_free(uri);
  if (name == NULL) {
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

  putobj(obj, "file", id, name);
  data = NgraphApp.FileWin.data.data;
  FileDialog(data, id, FALSE);
  ret = DialogExecute(TopLevel, data->dialog);
  if (ret == IDCANCEL) {
    menu_undo_internal(undo);
  } else {
    set_graph_modified();
    AddDataFileList(fname);
  }
  g_free(fname);
  FileWinUpdate(data, TRUE, DRAW_NOTIFY);
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
struct range_add_data
{
  int undo;
  struct obj_list_data *data;
};

static int
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
  return IDOK;
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
  ((struct DialogType *) data->dialog)->response_cb = response_callback_new(range_add_response, NULL, res_data);
  DialogExecute(TopLevel, data->dialog);
}
#else
void
CmRangeAdd
(void *w, gpointer client_data)
{
  int id, ret, val, undo;
  struct objlist *obj;
  struct obj_list_data *data;

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

  data = NgraphApp.FileWin.data.data;
  val = DATA_SOURCE_RANGE;
  putobj(obj, "source", id, &val);
  val = PLOT_TYPE_LINE;
  putobj(obj, "type", id, &val);
  FileDialog(data, id, FALSE);
  ret = DialogExecute(TopLevel, data->dialog);

  if (ret == IDCANCEL) {
    menu_undo_internal(undo);
  } else {
    set_graph_modified();
    FileWinUpdate(data, TRUE, DRAW_REDRAW);
  }
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
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
#endif

void
CmFileOpen
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer client_data)
#else
(void *w, gpointer client_data)
#endif
{
  int id, ret, n, undo = -1, chd;
  char **file = NULL;
  struct objlist *obj;
  struct narray *farray;

  if (Menulock || Globallock)
    return;

  obj = chkobject("data");
  if (obj == NULL)
    return;

  chd = Menulocal.changedirectory;
  ret = nGetOpenFileNameMulti(TopLevel, _("Add Data file"), NULL,
			      &(Menulocal.fileopendir), NULL,
			      &file, chd);

  n = chkobjlastinst(obj);

  farray = arraynew(sizeof(int));
  if (ret == IDOK && file) {
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

  if (update_file_obj_multi(obj, farray, TRUE)) {
    menu_delete_undo(undo);
  }

  if (n == chkobjlastinst(obj)) {
    menu_delete_undo(undo);
  } else {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, DRAW_NOTIFY);
    set_graph_modified();
  }

  arrayfree(farray);
}

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
static int
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
  return IDOK;
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
  DlgSelect.response_cb = response_callback_new(file_close_response, NULL, farray);
  DialogExecute(TopLevel, &DlgSelect);
}
#else
void
CmFileClose(void *w, gpointer client_data)
{
  struct narray farray;
  struct objlist *obj;

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("data")) == NULL)
    return;
  if (chkobjlastinst(obj) == -1)
    return;
  SelectDialog(&DlgSelect, obj, _("close data (multi select)"), FileCB, (struct narray *) &farray, NULL);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    int i, *array, num;
    struct obj_list_data *data;
    data = NgraphApp.FileWin.data.data;
    num = arraynum(&farray);
    if (num > 0) {
      data_save_undo(UNDO_TYPE_DELETE);
    }
    array = arraydata(&farray);
    for (i = num - 1; i >= 0; i--) {
      delete_file_obj(data, array[i]);
      set_graph_modified();
    }
    FileWinUpdate(data, TRUE, DRAW_REDRAW);
  }
  arraydel(&farray);
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
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

static int
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
    return IDOK;
  }
  i++;
  if (i < num) {
    FileDialog(data, array[i], i < num - 1);
    rdata->i = i;
    cb->data = NULL;
    ((struct DialogType *) data->dialog)->response_cb = response_callback_new(update_file_obj_multi_response, NULL, rdata);
    DialogExecute(TopLevel, data->dialog);
  } else {
    if (! rdata->modified) {
      menu_undo_internal(undo);
    }
    rdata->cb(rdata->modified, rdata->user_data);
    g_free(rdata);
  }
  return IDOK;
}

int
update_file_obj_multi(struct objlist *obj, struct narray *farray, int new_file, response_cb cb, gpointer user_data)
{
  int num, *array, undo;
  struct obj_list_data *data;
  struct update_file_obj_multi_data *rdata;

  num = arraynum(farray);
  if (num < 1) {
    return 0;
  }

  array = arraydata(farray);

  data_save_undo(UNDO_TYPE_EDIT);
  data = NgraphApp.FileWin.data.data;

  rdata = g_malloc0(sizeof(*rdata));
  if (rdata == NULL) {
    return IDOK;
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
  ((struct DialogType *) data->dialog)->response_cb = response_callback_new(update_file_obj_multi_response, NULL, rdata);
  DialogExecute(TopLevel, data->dialog);

  return 0;
}
#else
int
update_file_obj_multi(struct objlist *obj, struct narray *farray, int new_file)
{
  int i, j, num, *array, id0, modified, ret, undo;
  char *name;
  struct obj_list_data *data;

  num = arraynum(farray);
  if (num < 1) {
    return 0;
  }

  array = arraydata(farray);
  id0 = -1;

  ret = IDCANCEL;
  modified = FALSE;
  data_save_undo(UNDO_TYPE_EDIT);
  for (i = 0; i < num; i++) {
    name = NULL;
    if (id0 != -1) {
      copy_file_obj_field(obj, array[i], array[id0], FALSE);
      if (new_file) {
	getobj(obj, "file", array[i], 0, NULL, &name);
	AddDataFileList(name);
      }
    } else {
      undo = data_save_undo(UNDO_TYPE_DUMMY);
      data = NgraphApp.FileWin.data.data;
      FileDialog(data, array[i], i < num - 1);
      ret = DialogExecute(TopLevel, data->dialog);
      if (ret == IDCANCEL && new_file) {
	ret = IDDELETE;
      }
      switch (ret) {
      case IDDELETE:
	delete_file_obj(data, array[i]);
	modified = TRUE;
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
	modified = TRUE;
	break;
      case IDCANCEL:
	menu_undo_internal(undo);
	break;
      }
    }
  }
  if (! modified) {
    menu_undo_internal(undo);
  }
  return modified;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
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

static int
file_update_response(struct response_callback *cb)
{
  struct narray *farray;
  struct SelectDialog *d;
  farray = (struct narray *) cb->data;
  d = (struct SelectDialog *) cb->dialog;
  if (cb->return_value == IDOK && update_file_obj_multi(d->Obj, farray, FALSE)) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, DRAW_REDRAW);
  }
  arrayfree(farray);
  return IDOK;
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
    if (update_file_obj_multi(obj, farray, FALSE)) {
      FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, DRAW_REDRAW);
    }
    arrayfree(farray);
  } else {
    farray = arraynew(sizeof(int));
    SelectDialog(&DlgSelect, obj, _("data property (multi select)"), FileCB, (struct narray *) farray, NULL);
    DlgSelect.response_cb = response_callback_new(file_update_response, NULL, farray);
    DialogExecute(TopLevel, &DlgSelect);
  }
}
#else
void
CmFileUpdate(void *w, gpointer client_data)
{
  struct objlist *obj;
  int ret;
  struct narray farray;
  int last;

  if (Menulock || Globallock)
    return;

  if ((obj = chkobject("data")) == NULL)
    return;

  last = chkobjlastinst(obj);
  if (last == -1) {
    return;
  } else if (last == 0) {
    arrayinit(&farray, sizeof(int));
    arrayadd(&farray, &last);
    ret = IDOK;
  } else {
    SelectDialog(&DlgSelect, obj, _("data property (multi select)"), FileCB, (struct narray *) &farray, NULL);
    ret = DialogExecute(TopLevel, &DlgSelect);
  }

  if (ret == IDOK && update_file_obj_multi(obj, &farray, FALSE)) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, DRAW_REDRAW);
  }
  arraydel(&farray);
}
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
static int
file_edit_response(struct response_callback *cb)
{
  int i;
  struct objlist *obj;
  struct CopyDialog *d;
  char *name;
  d = (struct CopyDialog *) cb->dialog;
  obj = d->Obj;
  if (cb->return_value == IDOK) {
    i = d->sel;
  } else {
    return IDOK;
  }

  if (i < 0) {
    return IDOK;
  }

  if (getobj(obj, "file", i, 0, NULL, &name) == -1) {
    return IDOK;
  }

  edit_file(name);
  return IDOK;
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
  } else {
    CopyDialog(&DlgCopy, obj, -1, _("edit data file (single select)"), PlotFileCB);
    DlgCopy.response_cb = response_callback_new(file_edit_response, NULL, NULL);
    DialogExecute(TopLevel, &DlgCopy);
  }
}
#else
void
CmFileEdit(void *w, gpointer client_data)
{
  struct objlist *obj;
  int i;
  char *name;
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
  } else {
    CopyDialog(&DlgCopy, obj, -1, _("edit data file (single select)"), PlotFileCB);
    if (DialogExecute(TopLevel, &DlgCopy) == IDOK) {
      i = DlgCopy.sel;
    } else {
      return;
    }
  }

  if (i < 0)
    return;

  if (getobj(obj, "file", i, 0, NULL, &name) == -1)
    return;

  edit_file(name);
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
static int
option_file_def_response(struct response_callback *cb)
{
  int id;
  struct objlist *obj;
  struct FileDialog *d;
  int modified;
  char *objs[2];
  d = (struct FileDialog *) cb->dialog;
  modified = GPOINTER_TO_INT(cb->data);
  obj = d->Obj;
  id = d->Id;
  if (cb->return_value == IDOK) {
    if (CheckIniFile()) {
      exeobj(obj, "save_config", id, 0, NULL);
    }
  }
  delobj(obj, id);
  objs[0] = obj->name;
  objs[1] = NULL;
  UpdateAll2(objs, TRUE);
  if (! modified) {
    reset_graph_modified();
  }
  return IDOK;
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
    DlgFileDef.response_cb = response_callback_new(option_file_def_response, NULL, GINT_TO_POINTER(modified));
    DialogExecute(TopLevel, &DlgFileDef);
  }
}
#else
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
    char *objs[2];

    modified = get_graph_modified();
    FileDefDialog(&DlgFileDef, obj, id);
    if (DialogExecute(TopLevel, &DlgFileDef) == IDOK) {
      if (CheckIniFile()) {
	exeobj(obj, "save_config", id, 0, NULL);
      }
    }
    delobj(obj, id);
    objs[0] = obj->name;
    objs[1] = NULL;
    UpdateAll2(objs, TRUE);
    if (! modified) {
      reset_graph_modified();
    }
  }
}
#endif

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
file_edit_popup_func
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer client_data)
#else
(GtkMenuItem *w, gpointer client_data)
#endif
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

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
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
file_delete_popup_func
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer client_data)
#else
(GtkMenuItem *w, gpointer client_data)
#endif
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

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
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
file_copy_popup_func
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer client_data)
#else
(GtkMenuItem *w, gpointer client_data)
#endif
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
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
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
file_copy2_popup_func
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer client_data)
#else
(GtkMenuItem *w, gpointer client_data)
#endif
{
  struct obj_list_data *d;

  d = (struct obj_list_data *) client_data;
  FileWinFileCopy2(d);
}

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
struct file_update_data {
  int undo;
  struct obj_list_data *d;
};

static int
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
  return IDOK;
}

static void
FileWinFileUpdate(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
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
    ((struct DialogType *) d->dialog)->response_cb = response_callback_new(file_win_file_update_response, NULL, data);
    DialogExecute(parent, d->dialog);
  }
}
#else
static void
FileWinFileUpdate(struct obj_list_data *d)
{
  int sel, num;

  if (Menulock || Globallock)
    return;
  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
  num = chkobjlastinst(d->obj);

  if ((sel >= 0) && (sel <= num)) {
    int ret, undo;
    GtkWidget *parent;
    undo = data_save_undo(UNDO_TYPE_EDIT);
    d->setup_dialog(d, sel, FALSE);
    d->select = sel;

    parent = TopLevel;
    ret = DialogExecute(parent, d->dialog);
    set_graph_modified();
    switch (ret) {
    case IDCANCEL:
      menu_undo_internal(undo);
      break;
    default:
      d->update(d, FALSE, DRAW_NOTIFY);
    }
  }
}
#endif

static void
FileWinFileDraw(struct obj_list_data *d)
{
  int i, sel, hidden, h, num, modified, undo;

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_index(GTK_WIDGET(d->text));
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
  CmViewerDraw(NULL, GINT_TO_POINTER(FALSE));
  FileWinUpdate(d, FALSE, DRAW_NONE);
}

static void
file_draw_popup_func
#if GTK_CHECK_VERSION(4, 0, 0)
(GSimpleAction *action, GVariant *parameter, gpointer client_data)
#else
(GtkMenuItem *w, gpointer client_data)
#endif
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
    list_sub_window_build(d, file_list_set_val);
  } else {
    list_sub_window_set(d, file_list_set_val);
  }

  if (! clear && d->select >= 0) {
    list_store_select_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID, d->select);
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

#if GTK_CHECK_VERSION(4, 0, 0)
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
#endif

static void
FileWinFit(struct obj_list_data *d)
{
  struct objlist *fitobj, *obj2;
  char *fit;
  int sel, fitid = 0, ret, num, undo;
  struct narray iarray;
  GtkWidget *parent;
#if GTK_CHECK_VERSION(4, 0, 0)
  struct show_fit_dialog_data *data;
#endif

  if (Menulock || Globallock)
    return;

  sel = list_store_get_selected_int(GTK_WIDGET(d->text), FILE_WIN_COL_ID);
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
#if GTK_CHECK_VERSION(4, 0, 0)
  data = g_malloc0(sizeof(*data));
  data->fitobj = fitobj;
  data->obj = d->obj;
  data->fitid = fitid;
  data->id = sel;
  data->undo = undo;
  ret = execute_fit_dialog(parent, d->obj, sel, fitobj, fitid, file_win_fit_response, data);
#else
  ret = execute_fit_dialog(parent, d->obj, sel, fitobj, fitid);

  switch (ret) {
  case IDCANCEL:
    menu_delete_undo(undo);
    break;
  case IDDELETE:
    delobj(fitobj, fitid);
    putobj(d->obj, "fit", sel, NULL);
    break;
  }
#endif
}

#define MARK_PIX_LINE_WIDTH 1

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
    GRAline(ggc, 1, height / 2, width - 1, height / 2);
    GRAline(ggc, 1, height / 4, 1, height * 3 / 4);
    GRAline(ggc, width - 1, height / 4, width - 1, height * 3 / 4);
    break;
  case PLOT_TYPE_ERRORBAR_Y:
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
file_list_set_val(struct obj_list_data *d, GtkTreeIter *iter, int row)
{
  int cx, style;
  unsigned int i;
  struct narray *mask, *move;
  char *bfile, *axis;
  GdkPixbuf *pixbuf = NULL;
  int src;
  const char *str;

  getobj(d->obj, "source", row, 0, NULL, &src);
  list_store_set_int(GTK_WIDGET(d->text), iter, FILE_WIN_COL_NOT_RANGE, src != DATA_SOURCE_RANGE);
  list_store_set_int(GTK_WIDGET(d->text), iter, FILE_WIN_COL_IS_FILE, src == DATA_SOURCE_FILE);
  getobj(d->obj, "mask", row, 0, NULL, &mask);
  getobj(d->obj, "move_data", row, 0, NULL, &move);
  if ((arraynum(mask) != 0) || (arraynum(move) != 0)) {
    style = PANGO_STYLE_ITALIC;
  } else {
    style = PANGO_STYLE_NORMAL;
  }
  list_store_set_int(GTK_WIDGET(d->text), iter, FILE_WIN_COL_MASKED, style);

  for (i = 0; i < FILE_WIN_COL_NUM; i++) {
    switch (i) {
    case FILE_WIN_COL_TIP:
      str = NULL;
      str = get_plot_info_str(d->obj, row, src);
      list_store_set_string(GTK_WIDGET(d->text), iter, i, (str) ? str : "");
      break;
    case FILE_WIN_COL_FILE:
      str = get_plot_info_str(d->obj, row, src);
      if (str == NULL) {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, FILL_STRING);
      } else {
	if (src == DATA_SOURCE_FILE) {
	  bfile = getbasename(str);
	  if (bfile) {
	    list_store_set_string(GTK_WIDGET(d->text), iter, i, bfile);
	    g_free(bfile);
	  } else {
	    list_store_set_string(GTK_WIDGET(d->text), iter, i, FILL_STRING);
	  }
        } else {
          set_escaped_str(GTK_WIDGET(d->text), iter, i, str);
        }
      }
      break;
    case FILE_WIN_COL_TYPE:
      pixbuf = draw_type_pixbuf(d->obj, row);
      if (pixbuf) {
	list_store_set_pixbuf(GTK_WIDGET(d->text), iter, i, pixbuf);
	g_object_unref(pixbuf);
      }
      break;
    case FILE_WIN_COL_X_AXIS:
    case FILE_WIN_COL_Y_AXIS:
      axis = get_axis_obj_str(d->obj, row, (Flist[i].title[1] == 'x') ? AXIS_X : AXIS_Y);
      if (axis) {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, axis);
	g_free(axis);
      } else {
	list_store_set_string(GTK_WIDGET(d->text), iter, i, "");
      }
      break;
    case FILE_WIN_COL_HIDDEN:
      getobj(d->obj, Flist[i].name, row, 0, NULL, &cx);
      cx = ! cx;
      list_store_set_val(GTK_WIDGET(d->text), iter, i, Flist[i].type, &cx);
      break;
    case FILE_WIN_COL_MASKED:
    case FILE_WIN_COL_NOT_RANGE:
    case FILE_WIN_COL_IS_FILE:
      break;
    default:
      if (Flist[i].type == G_TYPE_DOUBLE) {
	getobj(d->obj, Flist[i].name, row, 0, NULL, &cx);
	list_store_set_double(GTK_WIDGET(d->text), iter, i, cx / 100.0);
      } else {
	getobj(d->obj, Flist[i].name, row, 0, NULL, &cx);
	list_store_set_val(GTK_WIDGET(d->text), iter, i, Flist[i].type, &cx);
      }
    }
  }
}

#if GTK_CHECK_VERSION(4, 0, 0)
static int
file_math_response(struct response_callback *cb)
{
  if (DlgMath.modified) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, DRAW_REDRAW);
  } else {
    int undo;
    undo = GPOINTER_TO_INT(cb->data);
    menu_delete_undo(undo);
  }
  return IDOK;
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
  DlgMath.response_cb = response_callback_new(file_math_response, NULL, GINT_TO_POINTER(undo));
  DialogExecute(TopLevel, &DlgMath);
}
#else
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
  DialogExecute(TopLevel, &DlgMath);
  if (DlgMath.modified) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, DRAW_REDRAW);
  } else {
    menu_delete_undo(undo);
  }
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
static int
get_draw_files_response(struct response_callback *cb)
{
  struct SelectDialog *d;
  struct narray *ifarray, *farray;
  d = (struct SelectDialog *) cb->dialog;
  farray = d->sel;
  ifarray = d->isel;
  if (cb->return_value != IDOK) {
    arrayfree(ifarray);
    arraydel(farray);
    return IDCANCEL;
  }
  arrayfree(ifarray);
  return IDOK;
}

static int
GetDrawFiles(struct narray *farray)
{
  struct objlist *fobj;
  int lastinst;
  struct narray *ifarray;
  int i, a;

  if (farray == NULL)
    return 1;

  fobj = chkobject("data");
  if (fobj == NULL)
    return 1;

  lastinst = chkobjlastinst(fobj);
  if (lastinst < 0)
    return 1;

  ifarray = arraynew(sizeof(int));
  for (i = 0; i <= lastinst; i++) {
    getobj(fobj, "hidden", i, 0, NULL, &a);
    if (!a)
      arrayadd(ifarray, &i);
  }
  SelectDialog(&DlgSelect, fobj, NULL, FileCB, farray, ifarray);
  DlgSelect.response_cb = response_callback_new(get_draw_files_response, NULL, NULL);
  DialogExecute(TopLevel, &DlgSelect);
  return 0;
}
#else
static int
GetDrawFiles(struct narray *farray)
{
  struct objlist *fobj;
  int lastinst;
  struct narray ifarray;
  int i, a;

  if (farray == NULL)
    return 1;

  fobj = chkobject("data");
  if (fobj == NULL)
    return 1;

  lastinst = chkobjlastinst(fobj);
  if (lastinst < 0)
    return 1;

  arrayinit(&ifarray, sizeof(int));
  for (i = 0; i <= lastinst; i++) {
    getobj(fobj, "hidden", i, 0, NULL, &a);
    if (!a)
      arrayadd(&ifarray, &i);
  }
  SelectDialog(&DlgSelect, fobj, NULL, FileCB, farray, &ifarray);
  if (DialogExecute(TopLevel, &DlgSelect) != IDOK) {
    arraydel(&ifarray);
    arraydel(farray);
    return 1;
  }
  arraydel(&ifarray);

  return 0;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
static void
save_data(struct narray *farray, int div)
{
  struct objlist *obj;
  char *file, buf[1024];
  int chd, i, *array, num, onum;
  char *argv[4];

  chd = Menulocal.changedirectory;
  if (nGetSaveFileName(TopLevel, _("Data file"), NULL, NULL, NULL,
		       &file, FALSE, chd) != IDOK) {
    arrayfree(farray);
    return;
  }

  ProgressDialogCreate(_("Making data file"));
  SetStatusBar(_("Making data file."));

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
    if (exeobj(obj, "output_file", array[i], 3, argv))
      break;
  }
  ProgressDialogFinalize();
  ResetStatusBar();
  main_window_redraw();

  arrayfree(farray);
  g_free(file);
}

static int
file_save_curve_data_response(struct response_callback *cb)
{
  int div;
  struct narray *farray;

  farray = (struct narray *) cb->data;
  if (cb->return_value != IDOK) {
    arrayfree(farray);
    return IDCANCEL;
  }
  div = DlgOutputData.div;
  save_data(farray, div);
  return IDOK;
}

void
CmFileSaveData(void *w, gpointer client_data)
{
  struct narray *farray;
  struct objlist *obj;
  int i, num, onum, type, div, curve = FALSE, *array;

  if (Menulock || Globallock)
    return;

  farray = arraynew(sizeof(int));
  if (GetDrawFiles(farray)) {
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
    DlgOutputData.response_cb = response_callback_new(file_save_curve_data_response, NULL, farray);
    DialogExecute(TopLevel, &DlgOutputData);
  } else {
    save_data(farray, div);
  }
}
#else
void
CmFileSaveData(void *w, gpointer client_data)
{
  struct narray farray;
  struct objlist *obj;
  int i, num, onum, type, div, curve = FALSE, *array, append;
  char *file, buf[1024];
  char *argv[4];
  int chd;

  if (Menulock || Globallock)
    return;

  if (GetDrawFiles(&farray))
    return;

  obj = chkobject("data");
  if (obj == NULL)
    return;

  onum = chkobjlastinst(obj);
  num = arraynum(&farray);

  if (num == 0) {
    arraydel(&farray);
    return;
  }

  array = arraydata(&farray);
  for (i = 0; i < num; i++) {
    if (array[i] < 0 || array[i] > onum)
      continue;

    getobj(obj, "type", array[i], 0, NULL, &type);
    if (type == 3) {
      curve = TRUE;
    }
  }

  div = 10;

  if (curve) {
    OutputDataDialog(&DlgOutputData, div);
    if (DialogExecute(TopLevel, &DlgOutputData) != IDOK) {
      arraydel(&farray);
      return;
    }
    div = DlgOutputData.div;
  }

  chd = Menulocal.changedirectory;
  if (nGetSaveFileName(TopLevel, _("Data file"), NULL, NULL, NULL,
		       &file, FALSE, chd) != IDOK) {
    arraydel(&farray);
    return;
  }

  ProgressDialogCreate(_("Making data file"));
  SetStatusBar(_("Making data file."));

  argv[0] = (char *) file;
  argv[1] = (char *) &div;
  argv[3] = NULL;
  for (i = 0; i < num; i++) {
    if (array[i] < 0 || array[i] > onum)
      continue;

    snprintf(buf, sizeof(buf), "%d/%d", i, num);
    set_progress(1, buf, 1.0 * (i + 1) / num);

    append = (i == 0) ? FALSE : TRUE;
    argv[2] = (char *) &append;
    if (exeobj(obj, "output_file", array[i], 3, argv))
      break;
  }
  ProgressDialogFinalize();
  ResetStatusBar();
  main_window_redraw();

  arraydel(&farray);
  g_free(file);
}
#endif

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
#if ! GTK_CHECK_VERSION(4, 0, 0)
      UnFocus();
#endif
    }
    break;
  default:
    return FALSE;
  }
  return TRUE;
}

#if GTK_CHECK_VERSION(4, 0, 0)
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
#else
static void
popup_show_cb(GtkWidget *widget, gpointer user_data)
{
  int sel, num, source;
  unsigned int i;
  struct obj_list_data *d;

  d = (struct obj_list_data *) user_data;

  sel = d->select;
  num = chkobjlastinst(d->obj);
  for (i = 1; i < POPUP_ITEM_NUM; i++) {
    switch (i) {
    case POPUP_ITEM_TOP:
    case POPUP_ITEM_UP:
      gtk_widget_set_sensitive(d->popup_item[i], sel > 0 && sel <= num);
      break;
    case POPUP_ITEM_DOWN:
    case POPUP_ITEM_BOTTOM:
      gtk_widget_set_sensitive(d->popup_item[i], sel >= 0 && sel < num);
      break;
    case POPUP_ITEM_EDIT:
      if (sel >= 0 && sel <= num) {
	getobj(d->obj, "source", sel, 0, NULL, &source);
	gtk_widget_set_sensitive(d->popup_item[i], source == DATA_SOURCE_FILE);
      } else {
	gtk_widget_set_sensitive(d->popup_item[i], FALSE);
      }
      break;
    default:
      gtk_widget_set_sensitive(d->popup_item[i], sel >= 0 && sel <= num);
    }
  }
}
#endif

enum FILE_COMBO_ITEM {
  FILE_COMBO_ITEM_COLOR_1,
  FILE_COMBO_ITEM_COLOR_2,
  FILE_COMBO_ITEM_TYPE,
  FILE_COMBO_ITEM_MARK,
  FILE_COMBO_ITEM_INTP,
  FILE_COMBO_ITEM_LINESTYLE,
  FILE_COMBO_ITEM_JOIN,
  FILE_COMBO_ITEM_FIT,
  FILE_COMBO_ITEM_CLIP,
};


static void
add_fit_combo_item_to_cbox(GtkTreeStore *list, struct objlist *obj, int id)
{
  char *valstr, buf[1024];
  int i;

  sgetobjfield(obj, id, "fit", NULL, &valstr, FALSE, FALSE, FALSE);
  if (valstr == NULL) {
    return;
  }

  for (i = 0; (valstr[i] != '\0') && (valstr[i] != ':'); i++);
  if (valstr[i] == ':') {
    i++;
  }
  if (valstr[i]) {
    snprintf(buf, sizeof(buf), "Fit:%s", valstr + i);
  } else {
    snprintf(buf, sizeof(buf), "Fit:%s", _("Create"));
  }
  g_free(valstr);

  add_text_combo_item_to_cbox(list, NULL, NULL, FILE_COMBO_ITEM_FIT, -1, buf, TOGGLE_NONE, FALSE);
}

static void
create_type_color_combo_box(GtkWidget *cbox, struct objlist *obj, int type, int id)
{
  int count;
  GtkTreeStore *list;
  GtkTreeIter parent;

  count = combo_box_get_num(cbox);
  if (count > 0) {
    return;
  }

  list = GTK_TREE_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(cbox)));

#if GTK_CHECK_VERSION(4, 0, 0)
  create_type_combo_item(cbox, list, obj, id);
#else
  create_type_combo_item(list, obj, id);
#endif

  switch (type) {
  case PLOT_TYPE_MARK:
  case PLOT_TYPE_LINE:
  case PLOT_TYPE_POLYGON:
  case PLOT_TYPE_CURVE:
  case PLOT_TYPE_DIAGONAL:
  case PLOT_TYPE_ARROW:
  case PLOT_TYPE_RECTANGLE:
  case PLOT_TYPE_RECTANGLE_FILL:
  case PLOT_TYPE_ERRORBAR_X:
  case PLOT_TYPE_ERRORBAR_Y:
  case PLOT_TYPE_STAIRCASE_X:
  case PLOT_TYPE_STAIRCASE_Y:
  case PLOT_TYPE_BAR_X:
  case PLOT_TYPE_BAR_Y:
  case PLOT_TYPE_BAR_FILL_X:
  case PLOT_TYPE_BAR_FILL_Y:
  case PLOT_TYPE_FIT:
    add_line_style_item_to_cbox(list, NULL, FILE_COMBO_ITEM_LINESTYLE, obj, "line_style", id);
    break;
  }

  switch (type) {
  case PLOT_TYPE_LINE:
  case PLOT_TYPE_POLYGON:
  case PLOT_TYPE_CURVE:
  case PLOT_TYPE_STAIRCASE_X:
  case PLOT_TYPE_STAIRCASE_Y:
  case PLOT_TYPE_FIT:
    add_text_combo_item_to_cbox(list, &parent, NULL, -1, -1, _("Join"), TOGGLE_NONE, FALSE);
#if GTK_CHECK_VERSION(4, 0, 0)
    add_enum_combo_item_to_cbox(list, NULL, &parent, FILE_COMBO_ITEM_JOIN, obj, "line_join", id, NULL);
#else
    add_enum_combo_item_to_cbox(list, NULL, &parent, FILE_COMBO_ITEM_JOIN, obj, "line_join", id);
#endif
    break;
  }

  add_text_combo_item_to_cbox(list, NULL, NULL, FILE_COMBO_ITEM_COLOR_1, -1, _("Color 1"), TOGGLE_NONE, FALSE);

  switch (type) {
  case PLOT_TYPE_MARK:
  case PLOT_TYPE_RECTANGLE_FILL:
  case PLOT_TYPE_BAR_FILL_X:
  case PLOT_TYPE_BAR_FILL_Y:
    add_text_combo_item_to_cbox(list, NULL, NULL, FILE_COMBO_ITEM_COLOR_2, -1, _("Color 2"), TOGGLE_NONE, FALSE);
    break;
  }

  if (type == PLOT_TYPE_FIT) {
    add_fit_combo_item_to_cbox(list, obj, id);
  }

  add_bool_combo_item_to_cbox(list, NULL, NULL, FILE_COMBO_ITEM_CLIP, obj, "data_clip", id, _("Clip"));
}

static void
#if GTK_CHECK_VERSION(4, 0, 0)
create_type_combo_item(GtkWidget *cbox, GtkTreeStore *list, struct objlist *obj, int id)
#else
create_type_combo_item(GtkTreeStore *list, struct objlist *obj, int id)
#endif
{
  char **enumlist;
  int i, type;
  GtkTreeIter parent, iter;

  gtk_tree_store_append(list, &parent, NULL);
  gtk_tree_store_set(list, &parent,
		     OBJECT_COLUMN_TYPE_STRING, _("Type"),
		     OBJECT_COLUMN_TYPE_PIXBUF, NULL,
		     OBJECT_COLUMN_TYPE_INT, FILE_COMBO_ITEM_TYPE,
		     OBJECT_COLUMN_TYPE_TOGGLE_VISIBLE, FALSE,
		     OBJECT_COLUMN_TYPE_PIXBUF_VISIBLE, FALSE,
		     -1);

  enumlist = (char **) chkobjarglist(obj, "type");

  type = -1;
  getobj(obj, "type", id, 0, NULL, &type);

  for (i = 0; enumlist[i] && enumlist[i][0]; i++) {
    add_text_combo_item_to_cbox(list, &iter, &parent, FILE_COMBO_ITEM_TYPE, i, _(enumlist[i]), TOGGLE_RADIO,  type == i);
#if GTK_CHECK_VERSION(4, 0, 0)
    if (type == i) {
      gtk_combo_box_set_active_iter(GTK_COMBO_BOX(cbox), &iter);
    }
#endif
    if (strcmp(enumlist[i], "mark") == 0) {
      add_mark_combo_item_to_cbox(list, NULL, &iter, FILE_COMBO_ITEM_MARK, obj, "mark_type", id);
    } else if (strcmp(enumlist[i], "curve") == 0) {
#if GTK_CHECK_VERSION(4, 0, 0)
      add_enum_combo_item_to_cbox(list, NULL, &iter, FILE_COMBO_ITEM_INTP, obj, "interpolation", id, NULL);
#else
      add_enum_combo_item_to_cbox(list, NULL, &iter, FILE_COMBO_ITEM_INTP, obj, "interpolation", id);
#endif
    }
  }
}

#if GTK_CHECK_VERSION(4, 0, 0)
struct select_type_fit_data
{
  int sel, undo, type;
  struct obj_list_data *d;
};

static void
select_type_fit_response(int ret, gpointer user_data)
{
  struct obj_list_data *d;
  int sel, undo, type;
  struct select_type_fit_data *data;

  data = (struct select_type_fit_data *) user_data;
  sel = data->sel;
  undo = data->undo;
  type = data->type;
  d = data->d;
  g_free(data);
  if (ret != IDOK) {
    menu_delete_undo(undo);
    if (type > 0) {
      putobj(d->obj, "type", sel, &type);
    }
    return;
  }
  d->select = sel;
  d->update(d, FALSE, DRAW_REDRAW);
  set_graph_modified();
}
#endif

static void
select_type(GtkComboBox *w, gpointer user_data)
{
  int sel, col_type, type, mark_type, curve_type, enum_id, found, active, join, ret, undo;
  struct obj_list_data *d;
  GtkTreeStore *list;
  GtkTreeIter iter;
#if GTK_CHECK_VERSION(4, 0, 0)
  struct select_type_fit_data *data;
#endif

  menu_lock(FALSE);

  d = (struct obj_list_data *) user_data;

  gtk_widget_grab_focus(d->text);

  sel = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "user-data"));
  if (sel < 0)
    return;

  getobj(d->obj, "type", sel, 0, NULL, &type);

  list = GTK_TREE_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(w)));
  found = gtk_combo_box_get_active_iter(w, &iter);
  if (! found)
    return;

  gtk_tree_model_get(GTK_TREE_MODEL(list), &iter,
		     OBJECT_COLUMN_TYPE_INT, &col_type,
		     OBJECT_COLUMN_TYPE_ENUM, &enum_id,
		     -1);

  switch (col_type) {
  case FILE_COMBO_ITEM_COLOR_1:
    if (select_obj_color(d->obj, sel, OBJ_FIELD_COLOR_TYPE_1)) {
      return;
    }
    break;
  case FILE_COMBO_ITEM_COLOR_2:
    if (select_obj_color(d->obj, sel, OBJ_FIELD_COLOR_TYPE_2)) {
      return;
    }
    break;
  case FILE_COMBO_ITEM_TYPE:
    if (enum_id == type) {
      return;
    }
    undo = menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
    putobj(d->obj, "type", sel, &enum_id);
    if (enum_id == PLOT_TYPE_FIT) {
      char *fit;

      getobj(d->obj, "fit", sel, 0, NULL, &fit);
      if (fit == NULL) {
#if GTK_CHECK_VERSION(4, 0, 0)
        data = g_malloc0(sizeof(*data));
        if (data == NULL) {
          return;
        }
        data->sel = sel;
        data->undo = undo;
        data->type = type;
        data->d = d;
        show_fit_dialog(d->obj, sel, TopLevel, select_type_fit_response, data);
        return;
#else
	ret = show_fit_dialog(d->obj, sel, TopLevel);
	if (ret != IDOK) {
	  menu_delete_undo(undo);
	  putobj(d->obj, "type", sel, &type);
	  return;
	}
#endif
      }
    }
    break;
  case FILE_COMBO_ITEM_MARK:
    getobj(d->obj, "mark_type", sel, 0, NULL, &mark_type);
    if (type == PLOT_TYPE_MARK && enum_id == mark_type)
      return;

    menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
    putobj(d->obj, "mark_type", sel, &enum_id);

    type = PLOT_TYPE_MARK;
    putobj(d->obj, "type", sel, &type);

    break;
  case FILE_COMBO_ITEM_INTP:
    getobj(d->obj, "interpolation", sel, 0, NULL, &curve_type);
    if (type == PLOT_TYPE_CURVE && enum_id == curve_type)
      return;

    menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
    putobj(d->obj, "interpolation", sel, &enum_id);

    type = PLOT_TYPE_CURVE;
    putobj(d->obj, "type", sel, &type);

    break;
  case FILE_COMBO_ITEM_LINESTYLE:
    if (enum_id < 0 || enum_id >= FwNumStyleNum) {
      return;
    }
    menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
    if (chk_sputobjfield(d->obj, sel, "line_style", FwLineStyle[enum_id].list) != 0) {
      return;
    }
    if (! get_graph_modified()) {
      return;
    }
    break;
  case FILE_COMBO_ITEM_FIT:
    undo = data_save_undo(UNDO_TYPE_EDIT);
#if GTK_CHECK_VERSION(4, 0, 0)
    data = g_malloc0(sizeof(*data));
    if (data == NULL) {
      return;
    }
    data->sel = sel;
    data->undo = undo;
    data->type = -1;
    data->d = d;
    show_fit_dialog(d->obj, sel, TopLevel, select_type_fit_response, data);
    return;
#else
    ret = show_fit_dialog(d->obj, sel, TopLevel);
    if (ret != IDOK) {
      menu_delete_undo(undo);
      return;
    }
#endif
    break;
  case FILE_COMBO_ITEM_JOIN:
    gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, OBJECT_COLUMN_TYPE_ENUM, &enum_id, -1);
    getobj(d->obj, "line_join", sel, 0, NULL, &join);
    if (join == enum_id) {
      return;
    }
    menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
    putobj(d->obj, "line_join", sel, &enum_id);
    break;
  case FILE_COMBO_ITEM_CLIP:
    gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, OBJECT_COLUMN_TYPE_TOGGLE, &active, -1);
    active = ! active;
    menu_save_undo_single(UNDO_TYPE_EDIT, d->obj->name);
    putobj(d->obj, "data_clip", sel, &active);
    break;
  default:
    return;
  }

  d->select = sel;
  d->update(d, FALSE, DRAW_REDRAW);
  set_graph_modified();
}

static void
start_editing_type(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path_str, gpointer user_data)
{
  GtkTreeIter iter;
  struct obj_list_data *d;
  GtkComboBox *cbox;
  int sel, type;
  struct objlist *obj;

  menu_lock(TRUE);

  d = (struct obj_list_data *) user_data;

  sel = tree_view_get_selected_row_int_from_path(d->text, path_str, &iter, FILE_WIN_COL_ID);
  if (sel < 0) {
    return;
  }

  cbox = GTK_COMBO_BOX(editable);
  g_object_set_data(G_OBJECT(cbox), "user-data", GINT_TO_POINTER(sel));

  obj = getobject("data");
  if (obj == NULL)
    return;

  getobj(obj, "type", sel, 0, NULL, &type);
  create_type_color_combo_box(GTK_WIDGET(cbox), obj, type, sel);

  g_signal_connect(cbox, "editing-done", G_CALLBACK(select_type), d);
  gtk_widget_show(GTK_WIDGET(cbox));

  return;
}

static void
select_axis(GtkComboBox *w, gpointer user_data, char *axis)
{
  GtkTreeStore *list;
  GtkTreeIter iter;
  char buf[64];
  int j, sel, found;
  struct obj_list_data *d;

  sel = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), "user-data"));
  if (sel < 0)
    return;

  d = (struct obj_list_data *) user_data;

  list = GTK_TREE_STORE(gtk_combo_box_get_model(w));
  found = gtk_combo_box_get_active_iter(w, &iter);
  if (! found)
    return;

  gtk_tree_model_get(GTK_TREE_MODEL(list), &iter, OBJECT_COLUMN_TYPE_INT, &j, -1);
  if (j < 0)
    return;

  data_save_undo(UNDO_TYPE_EDIT);
  snprintf(buf, sizeof(buf), "axis:%d", j);
  if (sputobjfield(d->obj, sel, axis, buf) == 0) {
    d->select = sel;
  }
}

static void
select_axis_x(GtkComboBox *w, gpointer user_data)
{
  select_axis(w, user_data, "axis_x");
}

static void
select_axis_y(GtkComboBox *w, gpointer user_data)
{
  select_axis(w, user_data, "axis_y");
}

struct axis_combo_box_iter {
  char axis;
  int exist;
  GtkTreeIter iter;
};

static GtkTreeIter *
axis_combo_box_get_parent(struct axis_combo_box_iter *axis_iter, GtkTreeStore *list, char axis)
{
  int i;
  char name[] = "X";

  for (i = 0; axis_iter[i].axis; i++) {
    if (axis_iter[i].axis == axis) {
      if (! axis_iter[i].exist) {
	name[0] = axis;
	add_text_combo_item_to_cbox(list, &axis_iter[i].iter, NULL, -1, -1, name, TOGGLE_NONE, FALSE);
	axis_iter[i].exist = TRUE;
      }
      return &axis_iter[i].iter;
    }
  }
  return NULL;
}

static void
axis_select_done(GtkComboBox *w, gpointer user_data)
{
  menu_lock(FALSE);
}

static void
start_editing(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data, int axis)
{
  GtkTreeIter iter;
  GtkTreeStore *list;
  struct obj_list_data *d;
  GtkComboBox *cbox;
  int lastinst, j, sel;
  struct objlist *aobj;
  char *axis_name, *name;
  struct axis_combo_box_iter axis_iter[] = {
    {'X', FALSE},
    {'Y', FALSE},
    {'U', FALSE},
    {'R', FALSE},
    {'\0', FALSE},		/* sentinel value */
  };

  menu_lock(TRUE);

  d = (struct obj_list_data *) user_data;

  sel = tree_view_get_selected_row_int_from_path(d->text, path, &iter, FILE_WIN_COL_ID);
  if (sel < 0) {
    return;
  }

  cbox = GTK_COMBO_BOX(editable);
  g_object_set_data(G_OBJECT(cbox), "user-data", GINT_TO_POINTER(sel));

  init_object_combo_box(GTK_WIDGET(editable));
  list = GTK_TREE_STORE(gtk_combo_box_get_model(GTK_COMBO_BOX(editable)));
  gtk_tree_store_clear(list);

  aobj = getobject("axis");

  axis_name = get_axis_obj_str(d->obj, sel, axis);

  lastinst = chkobjlastinst(aobj);
  for (j = 0; j <= lastinst; j++) {
    GtkTreeIter *parent;
    int active;
    getobj(aobj, "group", j, 0, NULL, &name);
    name = CHK_STR(name);
    parent = NULL;
    if (lastinst > AXIS_SELECTION_LIMIT) {
      parent = axis_combo_box_get_parent(axis_iter, list, name[1]);
    }
    active = (g_strcmp0(axis_name, name) == 0);
    add_text_combo_item_to_cbox(list, &iter, parent, j, -1, name, TOGGLE_RADIO, active);
    if (active) {
      gtk_combo_box_set_active_iter(cbox, &iter);
    }
  }

  d->select = -1;
  g_signal_connect(cbox, "changed", G_CALLBACK((axis == AXIS_X) ? select_axis_x : select_axis_y), d);
  g_signal_connect(cbox, "editing-done", G_CALLBACK(axis_select_done), d);
  if (axis_name) {
    g_free(axis_name);
  }
}

static void
start_editing_x(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data)
{
  start_editing(renderer, editable, path, user_data, AXIS_X);
}

static void
start_editing_y(GtkCellRenderer *renderer, GtkCellEditable *editable, gchar *path, gpointer user_data)
{
  start_editing(renderer, editable, path, user_data, AXIS_Y);
}

static void
edited_axis(GtkCellRenderer *cell_renderer, gchar *path, gchar *str, gpointer user_data, char *axis)
{
  struct obj_list_data *d;

  menu_lock(FALSE);

  d = (struct obj_list_data *) user_data;

  gtk_widget_grab_focus(d->text);

  if (str == NULL || d->select < 0)
    return;

  d->update(d, FALSE, DRAW_REDRAW);
  set_graph_modified();
}

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
static int
drop_file(const GValue *value, int type)
{
  GFile *file;
  char *filenames[1];
  char *fname;
  int r;

  if (Globallock || Menulock || DnDLock)
    return FALSE;;

  if (! G_VALUE_HOLDS(value, G_TYPE_FILE)) {
    return FALSE;
  }

  file = g_value_get_object(value);
  if (file == NULL) {
    return FALSE;
  }

  fname = g_file_get_path(file);
  if (fname == NULL) {
    return FALSE;
  }
  if (strlen(fname) < 1) {
    g_free(fname);
    return FALSE;
  }

  filenames[0] = fname;
  r = data_dropped(filenames, G_N_ELEMENTS(filenames), type);
  g_free(fname);

  return ! r;
}

static gboolean
drag_drop_cb(GtkDropTarget *self, const GValue *value, gdouble x, gdouble y, gpointer user_data)
{
  return drop_file(value, GPOINTER_TO_INT(user_data));
}
#else
static void
drag_drop_cb(GtkWidget *w, GdkDragContext *context, gint x, gint y, GtkSelectionData *data, guint info, guint time, gpointer user_data)
{
  gchar **filenames;

  switch (info) {
  case DROP_TYPE_FILE:
    filenames = gtk_selection_data_get_uris(data);
    if (filenames) {
      int num;
      num = g_strv_length(filenames);
      data_dropped(filenames, num, FILE_TYPE_DATA);
      g_strfreev(filenames);
    }
    gtk_drag_finish(context, TRUE, FALSE, time);
    break;
  }
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
void
init_dnd_file(struct SubWin *d, int type)
{
  GtkDropTarget *target;

  target = gtk_drop_target_new(G_TYPE_FILE, GDK_ACTION_COPY);
  g_signal_connect(target, "drop", G_CALLBACK(drag_drop_cb), GINT_TO_POINTER(type));
  gtk_widget_add_controller(d->data.data->text, GTK_EVENT_CONTROLLER(target));
}
#else
static void
init_dnd(struct SubWin *d)
{
  GtkWidget *widget;
  GtkTargetEntry target[] = {
    {"text/uri-list", 0, DROP_TYPE_FILE},
  };

  widget = d->data.data->text;

  gtk_drag_dest_set(widget, GTK_DEST_DEFAULT_ALL, target, sizeof(target) / sizeof(*target), GDK_ACTION_COPY);
  g_signal_connect(widget, "drag-data-received", G_CALLBACK(drag_drop_cb), NULL);
}
#endif

GtkWidget *
create_data_list(struct SubWin *d)
{
  int n;
  int noexpand_colmns[] = {FILE_WIN_COL_X,
			   FILE_WIN_COL_Y,
			   FILE_WIN_COL_X_AXIS,
			   FILE_WIN_COL_Y_AXIS,
			   FILE_WIN_COL_TYPE,
			   FILE_WIN_COL_SIZE,
			   FILE_WIN_COL_WIDTH,
			   FILE_WIN_COL_SKIP,
			   FILE_WIN_COL_STEP,
			   FILE_WIN_COL_FINAL,
			   FILE_WIN_COL_DNUM};

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
  set_combo_cell_renderer_cb(d->data.data, FILE_WIN_COL_X_AXIS, Flist, G_CALLBACK(start_editing_x), G_CALLBACK(edited_axis));
  set_combo_cell_renderer_cb(d->data.data, FILE_WIN_COL_Y_AXIS, Flist, G_CALLBACK(start_editing_y), G_CALLBACK(edited_axis));
  set_obj_cell_renderer_cb(d->data.data, FILE_WIN_COL_TYPE, Flist, G_CALLBACK(start_editing_type));

#if GTK_CHECK_VERSION(4, 0, 0)
  init_dnd_file(d, FILE_TYPE_DATA);
#else
  init_dnd(d);
#endif

  gtk_tree_view_set_enable_search(GTK_TREE_VIEW(d->data.data->text), TRUE);
  gtk_tree_view_set_search_column(GTK_TREE_VIEW(d->data.data->text), FILE_WIN_COL_FILE);
  tree_view_set_tooltip_column(GTK_TREE_VIEW(d->data.data->text), FILE_WIN_COL_TIP);

  n = sizeof(noexpand_colmns) / sizeof(*noexpand_colmns);
  tree_view_set_no_expand_column(d->data.data->text, noexpand_colmns, n);

  set_cell_attribute_source(d, "style", FILE_WIN_COL_FILE, FILE_WIN_COL_MASKED);

  set_cell_attribute_source(d, "visible", FILE_WIN_COL_X, FILE_WIN_COL_NOT_RANGE);
  set_cell_attribute_source(d, "visible", FILE_WIN_COL_Y, FILE_WIN_COL_NOT_RANGE);

  set_cell_attribute_source(d, "editable", FILE_WIN_COL_FILE, FILE_WIN_COL_IS_FILE);

  return d->Win;
}

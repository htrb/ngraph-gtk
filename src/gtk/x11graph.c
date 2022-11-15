/*
 * $Id: x11graph.c,v 1.57 2010-03-04 08:30:17 hito Exp $
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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <libgen.h>

#include "dir_defs.h"

#include "object.h"
#include "ioutil.h"
#include "shell.h"
#include "nstring.h"
#include "odraw.h"

#include "init.h"
#include "x11dialg.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11gui.h"
#include "x11graph.h"
#include "x11view.h"
#include "x11axis.h"
#include "x11print.h"
#include "x11commn.h"
#include "x11info.h"
#include "x11bitmp.h"

#include "gtk_liststore.h"
#include "gtk_combo.h"
#include "gtk_widget.h"

#define PAPER_SIZE_MIN 1000

char *LoadPathStr[] = {
  N_("unchange"),
  N_("full"),
  N_("base"),
  NULL,
};

struct pagelisttype
{
  char *paper, *name;
  enum paper_id id;
  int width, height;
};

static struct pagelisttype pagelist[] = {
  {N_("Custom"),                  "custom",                 PAPER_ID_CUSTOM,    0,     0},
  {N_("normal display (4:3)"),    "normal display",         PAPER_ID_NORMAL,    21000, 28000},
  {N_("wide display (16:9)"),     "wide display",           PAPER_ID_WIDE,      21600, 38400},
  {N_("wide display (16:10)"),    "wide display",           PAPER_ID_WIDE2,     21000, 33600},
  {"A3 (297.00x420.00)",          GTK_PAPER_NAME_A3,        PAPER_ID_A3,        29700, 42000},
  {"A4 (210.00x297.00)",          GTK_PAPER_NAME_A4,        PAPER_ID_A4,        21000, 29700},
  {"A5 (148.00x210.00)",          GTK_PAPER_NAME_A5,        PAPER_ID_A5,        14800, 21000},
  {"B4 (257.00x364.00)",          "iso_b4",                 PAPER_ID_B4,        25700, 36400},
  {"B5 (182.00x257.00)",          GTK_PAPER_NAME_B5,        PAPER_ID_B5,        18200, 25700},
  {N_("Letter (215.90x279.40)"),  GTK_PAPER_NAME_LETTER,    PAPER_ID_LETTER,    21590, 27940},
  {N_("Legal (215.90x355.60)"),   GTK_PAPER_NAME_LEGAL,     PAPER_ID_LEGAL,     21590, 35560},
  {N_("Executive (184.2x266.7)"), GTK_PAPER_NAME_EXECUTIVE, PAPER_ID_EXECUTIVE, 18420, 26670},
};

#define PAGELISTNUM (sizeof(pagelist) / sizeof(*pagelist))
#define DEFAULT_PAPER_SIZE 0

struct graph_page_data
{
  response_cb cb;
  int sel;
};

static void GraphPage(int new_graph, struct graph_page_data *data);

int
set_paper_type(int w, int h)
{
  unsigned int j;
  int landscape = FALSE;

  if (w < 1 || h < 1)
    return 0;

  Menulocal.PaperWidth = w;
  Menulocal.PaperHeight = h;

  for (j = 0; j < PAGELISTNUM; j++) {
    if (w == pagelist[j].width &&  h == pagelist[j].height) {
      landscape = FALSE;
      break;
    } else if (h == pagelist[j].width &&  w == pagelist[j].height) {
      landscape = TRUE;
      break;
    }
  }

  if (j == PAGELISTNUM) {
    j = DEFAULT_PAPER_SIZE;
  }

  Menulocal.PaperName = pagelist[j].name;
  Menulocal.PaperId = pagelist[j].id;
  Menulocal.PaperLandscape = landscape;

  return j;
}

static void
PageDialogSetupItem(GtkWidget *w, struct PageDialog *d)
{
  int j;

  spin_entry_set_val(d->leftmargin, Menulocal.LeftMargin);
  spin_entry_set_val(d->topmargin, Menulocal.TopMargin);

  spin_entry_set_val(d->paperzoom, Menulocal.PaperZoom);

  j = set_paper_type(Menulocal.PaperWidth, Menulocal.PaperHeight);

  if (Menulocal.PaperLandscape) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->landscape), TRUE);
  } else {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->portrait), TRUE);
  }

  spin_entry_set_val(d->paperwidth, Menulocal.PaperWidth);
  spin_entry_set_val(d->paperheight, Menulocal.PaperHeight);

  combo_box_set_active(d->paper, j);
  combo_box_set_active(d->decimalsign, Menulocal.Decimalsign);
}

static void
PageDialogPage(GtkWidget *w, gpointer client_data)
{
  struct PageDialog *d;
  int paper, landscape;

  d = (struct PageDialog *) client_data;

  paper = combo_box_get_active(d->paper);
  landscape = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->landscape));

  if (paper < 0)
    return;

  set_widget_sensitivity_with_label(d->paperwidth, paper == 0);
  set_widget_sensitivity_with_label(d->paperheight, paper == 0);

  if (paper > 0) {
    if (landscape) {
      spin_entry_set_val(d->paperwidth, pagelist[paper].height);
      spin_entry_set_val(d->paperheight, pagelist[paper].width);
    } else {
      spin_entry_set_val(d->paperwidth, pagelist[paper].width);
      spin_entry_set_val(d->paperheight, pagelist[paper].height);
    }
  }
}

static void
PageDialogOrientation(GtkWidget *widget, gpointer client_data)
{
  struct PageDialog *d;
  int w, h;

  d = (struct PageDialog *) client_data;

  w = spin_entry_get_val(d->paperwidth);
  h = spin_entry_get_val(d->paperheight);
  spin_entry_set_val(d->paperwidth, h);
  spin_entry_set_val(d->paperheight, w);
}

static void
PageDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct PageDialog *d;

  d = (struct PageDialog *) data;

  if (makewidget) {
    GtkWidget *w, *table;
    unsigned int j;
    int i;
    GtkWidget *group;

    table = gtk_grid_new();

    i = 0;
    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, FALSE, TRUE);
    spin_entry_set_range(w, PAPER_SIZE_MIN, G_MAXUSHORT);
    add_widget_to_table(table, w, _("paper _Width:"), FALSE, i++);
    d->paperwidth = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_LENGTH, FALSE, TRUE);
    spin_entry_set_range(w, PAPER_SIZE_MIN, G_MAXUSHORT);
    add_widget_to_table(table, w, _("paper _Height:"), FALSE, i++);
    d->paperheight = w;

    w = combo_box_create();
    add_widget_to_table(table, w, _("_Paper:"), FALSE, i++);
    d->paper = w;
    g_signal_connect(w, "changed", G_CALLBACK(PageDialogPage), d);

    for (j = 0; j < PAGELISTNUM; j++) {
      combo_box_append_text(d->paper, _(pagelist[j].paper));
    }

    w = gtk_check_button_new_with_mnemonic(_("_Portrait"));
    group = w;
    gtk_check_button_set_active(GTK_CHECK_BUTTON(w), TRUE);
    add_widget_to_table(table, w, _("_Orientation:"), FALSE, i++);
    d->portrait = w;

    w = gtk_check_button_new_with_mnemonic(_("L_andscape"));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(w), GTK_CHECK_BUTTON(group));
    add_widget_to_table_sub(table, w, NULL,FALSE, 1, 1, 1, i++);
    d->landscape = w;
    g_signal_connect(w, "toggled", G_CALLBACK(PageDialogOrientation), d);

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, FALSE, TRUE);
    add_widget_to_table(table, w, _("_Left margin:"), FALSE, i++);
    d->leftmargin = w;


    w = create_spin_entry_type(SPIN_BUTTON_TYPE_POSITION, FALSE, TRUE);
    add_widget_to_table(table, w, _("_Top margin:"), FALSE, i++);
    d->topmargin = w;

    w = create_spin_entry_type(SPIN_BUTTON_TYPE_PERCENT, FALSE, TRUE);
    add_widget_to_table(table, w, _("paper _Zoom:"), FALSE, i++);
    d->paperzoom = w;

    w = combo_box_create();
    gtk_widget_set_hexpand(w, TRUE);
    add_widget_to_table(table, w, _("_Decimalsign:"), FALSE, i++);
    for (j = 0; gra_decimalsign_char[j]; j++) {
      combo_box_append_text(w, _(gra_decimalsign_char[j]));
    }
    d->decimalsign = w;

    gtk_box_append(GTK_BOX(d->vbox), table);
  }
  PageDialogSetupItem(wi, d);
#if 0
  d->show_cancel = ! d->new_graph;
#endif
}

static void
PageDialogClose(GtkWidget *wi, void *data)
{
  struct PageDialog *d;
  int w, h;

  d = (struct PageDialog *) data;
  if (d->ret != IDOK)
    return;

  w = spin_entry_get_val(d->paperwidth);
  h = spin_entry_get_val(d->paperheight);

  if (w < PAPER_SIZE_MIN || h < PAPER_SIZE_MIN) {
    d->ret = IDLOOP;
    return;
  }

  set_paper_type(w, h);

  Menulocal.LeftMargin = spin_entry_get_val(d->leftmargin);
  Menulocal.TopMargin = spin_entry_get_val(d->topmargin);

  Menulocal.PaperZoom = spin_entry_get_val(d->paperzoom);
  Menulocal.Decimalsign = combo_box_get_active(d->decimalsign);
}

void
PageDialog(struct PageDialog *data, int new_graph)
{
  data->SetupWindow = PageDialogSetup;
  data->CloseWindow = PageDialogClose;
  data->new_graph = new_graph;
}

static void
set_objlist_btn_state(struct SwitchDialog *d, gboolean b)
{
  gtk_widget_set_sensitive(d->add, b);
  gtk_widget_set_sensitive(d->ins, b);
}

static void
set_drawlist_btn_state(struct SwitchDialog *d, gboolean b)
{
  gtk_widget_set_sensitive(d->top, b);
  gtk_widget_set_sensitive(d->bottom, b);
  gtk_widget_set_sensitive(d->up, b);
  gtk_widget_set_sensitive(d->down, b);
  gtk_widget_set_sensitive(d->del, b);
}

static void
SwitchDialogSetupItem(GtkWidget *w, struct SwitchDialog *d)
{
  int j, num;
  GtkTreeIter iter;

  d->btn_lock = TRUE;

  list_store_clear(d->drawlist);
  num = arraynum(&(d->idrawrable));
  for (j = 0; j < num; j++) {
    char **buf;
    buf = (char **) arraynget(&(d->drawrable),
			      arraynget_int(&(d->idrawrable), j));
    list_store_append(d->drawlist, &iter);
    list_store_set_string(d->drawlist, &iter, 0, _(*buf));
  }

  d->btn_lock = FALSE;
}

static void
SwitchDialogAdd(GtkWidget *w, gpointer client_data)
{
  struct SwitchDialog *d;
  GtkTreeSelection *selected;
  GList *list, *ptr;
  int num, *data, i;

  d = (struct SwitchDialog *) client_data;

  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->objlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  data = arraydata(&(d->idrawrable));
  num = arraynum(&(d->idrawrable));

  for (ptr = list; ptr; ptr = ptr->next) {
    int *ary, a, duplicate;

    duplicate = FALSE;
    ary = gtk_tree_path_get_indices((GtkTreePath *)(ptr->data));
    a = ary[0];
    for (i = 0; i < num; i++) {
      if (data[i] == a) {
	duplicate = TRUE;
	break;
      }
    }
    if (!duplicate) {
      arrayadd(&(d->idrawrable), &a);
    }
  }

  if (list) {
    g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
  }

  SwitchDialogSetupItem(d->widget, d);
  set_drawlist_btn_state(d, FALSE);
}

static void
SwitchDialogInsert(GtkWidget *w, gpointer client_data)
{
  struct SwitchDialog *d;
  int i, a, pos, num2;
  int *data;
  GtkTreeSelection *selected;
  GList *list, *last;

  d = (struct SwitchDialog *) client_data;

  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->drawlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);
  pos = 0;
  if (list) {
    last = g_list_last(list);
    if (last) {
      int *ptr;
      ptr = gtk_tree_path_get_indices((GtkTreePath *)(last->data));
      pos = ptr[0];
    }
    g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
  }

  data = arraydata(&(d->idrawrable));

  num2 = arraynum(&(d->idrawrable));
  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->objlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  for (last = list; last; last = last->next) {
    int *ptr;
    ptr = gtk_tree_path_get_indices((GtkTreePath *)(last->data));
    a = ptr[0];
    for (i = 0; i < num2; i++) {
      if (data[i] == a) {
	break;
      }
    }
    if (i == num2) {
      arrayins(&(d->idrawrable), &a, pos);
    }
  }

  if (list) {
    g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
  }

  SwitchDialogSetupItem(d->widget, d);
  set_drawlist_btn_state(d, FALSE);
}

static void
switch_dialog_top_cb(gpointer data, gpointer user_data)
{
  GtkTreePath *path;
  int *ary, i, k;
  struct SwitchDialog *d;

  path = (GtkTreePath *) data;
  d = (struct SwitchDialog *) user_data;

  ary = gtk_tree_path_get_indices(path);

  if (! ary)
    return;

  i = ary[0];
  k = arraynget_int(&(d->idrawrable), i);
  arrayndel(&(d->idrawrable), i);
  arrayins(&(d->idrawrable), &k, 0);
}

static void
switch_dialog_last_cb(gpointer data, gpointer user_data)
{
  GtkTreePath *path;
  int *ary, i, k;
  struct SwitchDialog *d;

  path = (GtkTreePath *) data;
  d = (struct SwitchDialog *) user_data;

  ary = gtk_tree_path_get_indices(path);

  if (! ary)
    return;

  i = ary[0];
  k = arraynget_int(&(d->idrawrable), i);
  arrayndel(&(d->idrawrable), i);
  arrayadd(&(d->idrawrable), &k);
}

static void
SwitchDialogUp(GtkWidget *w, gpointer client_data)
{
  GtkTreeSelection *selected;
  GList *list, *ptr;
  struct SwitchDialog *d;
  int k, modified, *ary;
  GtkTreePath *path;

  d = (struct SwitchDialog *) client_data;
  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->drawlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  if (list == NULL) {
    return;
  }

  modified = FALSE;
  for (ptr = list; ptr; ptr = g_list_next(ptr)) {
    int i;
    path = (GtkTreePath *) ptr->data;
    ary = gtk_tree_path_get_indices(path);

    if (ary == NULL) {
      break;
    }

    i = ary[0];
    if (i <= 0) {
      break;
    }

    k = arraynget_int(&(d->idrawrable), i);
    arrayndel(&(d->idrawrable), i);
    i--;
    arrayins(&(d->idrawrable), &k, i);
    modified = TRUE;
  }

  if (modified) {
    SwitchDialogSetupItem(d->widget, d);

    for (ptr = list; ptr; ptr = g_list_next(ptr)) {
      path = (GtkTreePath *) ptr->data;
      ary = gtk_tree_path_get_indices(path);

      if (ary == NULL)
	break;

      list_store_select_nth(d->drawlist, ary[0] - 1);
    }
  }

  g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
}

static void
SwitchDialogDown(GtkWidget *w, gpointer client_data)
{
  GtkTreeSelection *selected;
  GList *list, *ptr;
  struct SwitchDialog *d;
  int k, num, modified, *ary;
  GtkTreePath *path;

  d = (struct SwitchDialog *) client_data;
  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->drawlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  if (list == NULL) {
    return;
  }

  num = list_store_get_num(d->drawlist);

  modified = FALSE;
  for (ptr = g_list_last(list); ptr; ptr = g_list_previous(ptr)) {
    int i;
    path = (GtkTreePath *) ptr->data;
    ary = gtk_tree_path_get_indices(path);

    if (ary == NULL)
      break;

    i = ary[0];
    if (i >= num - 1) {
      break;
    }

    k = arraynget_int(&(d->idrawrable), i);
    arrayndel(&(d->idrawrable), i);
    i++;
    arrayins(&(d->idrawrable), &k, i);
    modified = TRUE;
  }

  if (modified) {
    SwitchDialogSetupItem(d->widget, d);

    for (ptr = g_list_last(list); ptr; ptr = g_list_previous(ptr)) {
      path = (GtkTreePath *) ptr->data;
      ary = gtk_tree_path_get_indices(path);

      if (ary == NULL)
	break;

      list_store_select_nth(d->drawlist, ary[0] + 1);
    }
  }

  g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
}

static void
SwitchDialogTop(GtkWidget *w, gpointer client_data)
{
  GtkTreeSelection *selected;
  GList *list;
  struct SwitchDialog *d;
  int i, num = 0;

  d = (struct SwitchDialog *) client_data;
  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->drawlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  if (list) {
    num = g_list_length(list);
    g_list_foreach(list, switch_dialog_top_cb, d);
    g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
  }
  SwitchDialogSetupItem(d->widget, d);
  for (i = 0; i < num; i++) {
    list_store_select_nth(d->drawlist, i);
  }
}

static void
SwitchDialogLast(GtkWidget *w, gpointer client_data)
{
  struct SwitchDialog *d;
  GtkTreeSelection *selected;
  GList *list;
  int n, i, num = 0;

  d = (struct SwitchDialog *) client_data;
  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->drawlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  if (list) {
    num = g_list_length(list);
    list = g_list_reverse(list);
    g_list_foreach(list, switch_dialog_last_cb, d);
    g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
  }
  SwitchDialogSetupItem(d->widget, d);
  n = list_store_get_num(d->drawlist);
  for (i = 0; i < num; i++) {
    list_store_select_nth(d->drawlist, n - i - 1);
  }
}

static void
switch_dialog_remove_cb(gpointer data, gpointer user_data)
{
  GtkTreePath *path;
  int *ary;
  struct SwitchDialog *d;

  path = (GtkTreePath *) data;
  d = (struct SwitchDialog *) user_data;

  ary = gtk_tree_path_get_indices(path);

  if (! ary)
    return;

  arrayndel(&(d->idrawrable), ary[0]);
}

static void
SwitchDialogRemove(GtkWidget *w, gpointer client_data)
{
  struct SwitchDialog *d;
  GtkTreeSelection *selected;
  GList *list;

  d = (struct SwitchDialog *) client_data;
  selected = gtk_tree_view_get_selection(GTK_TREE_VIEW(d->drawlist));
  list = gtk_tree_selection_get_selected_rows(selected, NULL);

  if (list) {
    list = g_list_reverse(list);
    g_list_foreach(list, switch_dialog_remove_cb, d);
    g_list_free_full(list, (GDestroyNotify) gtk_tree_path_free);
  }
  SwitchDialogSetupItem(d->widget, d);
  set_drawlist_btn_state(d, FALSE);
}

static gboolean
drawlist_sel_cb(GtkTreeSelection *sel, gpointer user_data)
{
  struct SwitchDialog *d;

  d = (struct SwitchDialog *) user_data;

  if (! d->btn_lock) {
    int n;
    n = gtk_tree_selection_count_selected_rows(sel);
    set_drawlist_btn_state(d, n);
  }
  return FALSE;
}

static gboolean
objlist_sel_cb(GtkTreeSelection *sel, gpointer user_data)
{
  struct SwitchDialog *d;

  d = (struct SwitchDialog *) user_data;

  if (! d->btn_lock) {
    int n;
    n = gtk_tree_selection_count_selected_rows(sel);
    set_objlist_btn_state(d, n);
  }
  return FALSE;
}

static void
SwitchDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkTreeIter iter;
  struct SwitchDialog *d;
  int num2, num1, j, k, *obj_check;
  char **buf;
  static n_list_store list[] = {
    {N_("Object"), G_TYPE_STRING, TRUE, FALSE, NULL},
  };

  d = (struct SwitchDialog *) data;

  if (makewidget) {
    GtkWidget *w, *hbox, *vbox, *vbox2, *label, *frame;
    GtkTreeSelection *sel;

    hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    label = gtk_label_new_with_mnemonic(_("_Draw order"));
    gtk_box_append(GTK_BOX(vbox), label);

    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    d->drawlist = w;
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
    g_signal_connect(sel, "changed", G_CALLBACK(drawlist_sel_cb), d);


    frame = gtk_frame_new(NULL);
    gtk_frame_set_child(GTK_FRAME(frame), w);
    gtk_box_append(GTK_BOX(vbox), frame);
    gtk_box_append(GTK_BOX(hbox), vbox);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    w = gtk_button_new_with_mnemonic(_("_Add"));
    set_button_icon(w, "list-add");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogAdd), d);
    gtk_box_append(GTK_BOX(vbox), w);
    d->add = w;

    w = gtk_button_new_with_mnemonic(_("_Insert"));
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogInsert), d);
    gtk_box_append(GTK_BOX(vbox), w);
    d->ins = w;

    w = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
    gtk_box_append(GTK_BOX(vbox), w);

    w = gtk_button_new_with_mnemonic(_("_Top"));
    set_button_icon(w, "go-top");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogTop), d);
    gtk_box_append(GTK_BOX(vbox), w);
    d->top =w;

    w = gtk_button_new_with_mnemonic(_("_Up"));
    set_button_icon(w, "go-up");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogUp), d);
    gtk_box_append(GTK_BOX(vbox), w);
    d->up = w;

    w = gtk_button_new_with_mnemonic(_("_Down"));
    set_button_icon(w, "go-down");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogDown), d);
    gtk_box_append(GTK_BOX(vbox), w);
    d->down = w;

    w = gtk_button_new_with_mnemonic(_("_Bottom"));
    set_button_icon(w, "go-bottom");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogLast), d);
    gtk_box_append(GTK_BOX(vbox), w);
    d->bottom = w;

    w = gtk_button_new_with_mnemonic(_("_Remove"));
    set_button_icon(w, "list-remove");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogRemove), d);
    gtk_box_append(GTK_BOX(vbox), w);
    d->del = w;

    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
    gtk_box_append(GTK_BOX(vbox2), vbox);

    gtk_box_append(GTK_BOX(vbox2), vbox);

    gtk_box_append(GTK_BOX(hbox), vbox2);

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    label = gtk_label_new_with_mnemonic(_("_Objects"));
    gtk_box_append(GTK_BOX(vbox), label);

    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    d->objlist = w;
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
    g_signal_connect(sel, "changed", G_CALLBACK(objlist_sel_cb), d);


    frame = gtk_frame_new(NULL);

    gtk_frame_set_child(GTK_FRAME(frame), w);
    gtk_box_append(GTK_BOX(vbox), frame);
    gtk_box_append(GTK_BOX(hbox), vbox);
    gtk_box_append(GTK_BOX(d->vbox), hbox);

    d->btn_lock = FALSE;
  }

  menuadddrawrable(chkobject("draw"), &(d->drawrable));
  list_store_clear(d->objlist);
  num2 = arraynum(&(d->drawrable));
  for (j = 0; j < num2; j++) {
    buf = (char **) arraynget(&(d->drawrable), j);
    list_store_append(d->objlist, &iter);
    list_store_set_string(d->objlist, &iter, 0, _(*buf));
  }
  num1 = arraynum(&(Menulocal.drawrable));
  obj_check = g_malloc0(sizeof(*obj_check) * num2);
  if (obj_check == NULL) {
    return;
  }
  for (j = 0; j < num1; j++) {
    struct objlist *obj;

    buf = (char **) arraynget(&(Menulocal.drawrable), j);
    obj = chkobject(*buf);
    if (obj == NULL) {
      continue;
    }
    for (k = 0; k < num2; k++) {
      if (strcmp0(arraynget_str(&(d->drawrable), k), obj->name) == 0) {
	break;
      }
    }
    if (k != num2 && obj_check[k] == 0) {
      obj_check[k] = 1;
      arrayadd(&(d->idrawrable), &k);
    }
  }
  g_free(obj_check);
  SwitchDialogSetupItem(wi, d);
  set_objlist_btn_state(d, FALSE);
  set_drawlist_btn_state(d, FALSE);
}

static void
SwitchDialogClose(GtkWidget *w, void *data)
{
  struct SwitchDialog *d;

  d = (struct SwitchDialog *) data;
  if (d->ret == IDOK) {
    int num;
    arraydel2(&(Menulocal.drawrable));
    num = arraynum(&(d->idrawrable));
    if (num == 0) {
      menuadddrawrable(chkobject("draw"), &(Menulocal.drawrable));
    } else {
      int j;
      for (j = 0; j < num; j++) {
        char **buf;
	buf = (char **) arraynget(&(d->drawrable),
				  arraynget_int(&(d->idrawrable), j));
	if ((*buf) != NULL)
	  arrayadd2(&(Menulocal.drawrable), *buf);
      }
    }
  }
  arraydel2(&(d->drawrable));
  arraydel(&(d->idrawrable));
}

void
SwitchDialog(struct SwitchDialog *data)
{
  data->SetupWindow = SwitchDialogSetup;
  data->CloseWindow = SwitchDialogClose;
  arrayinit(&(data->drawrable), sizeof(char *));
  arrayinit(&(data->idrawrable), sizeof(int));
}

struct folder_chooser_data {
  char *folder, *title;
  GtkWidget *parent, *button;
};

#define FOLDER_CHOOSER_DATA_KEY "FOLDER_CHOOSER_DATA"

static void
folder_chooser_button_set_folder(GtkWidget *button, const char *path)
{
  char *bassename, *full_path;
  struct folder_chooser_data *data;
  if (button == NULL || path == NULL) {
    return;
  }
  data = g_object_get_data(G_OBJECT(button), FOLDER_CHOOSER_DATA_KEY);
  if (data == NULL) {
    return;
  }
  if (data->folder) {
    g_free(data->folder);
    data->folder = NULL;
  }

  full_path = g_canonicalize_filename(path, NULL);
  if (full_path == NULL) {
    return;
  }
  data->folder = full_path;

  bassename = getbasename(full_path);
  gtk_button_set_label(GTK_BUTTON(data->button), bassename);
  g_free(bassename);
  gtk_widget_set_tooltip_text(data->button, full_path);
}

const char *
folder_chooser_button_get_folder(GtkWidget *button)
{
  struct folder_chooser_data *data;
  data = g_object_get_data(G_OBJECT(button), FOLDER_CHOOSER_DATA_KEY);
  return data->folder;
}

static void
on_open_response(GtkDialog *dialog, int response, gpointer user_data)
{
  if (response == GTK_RESPONSE_ACCEPT) {
    struct folder_chooser_data *data;
    GFile *file;
    char *path;
    data = (struct folder_chooser_data *) user_data;
    file = gtk_file_chooser_get_file(GTK_FILE_CHOOSER(dialog));
    path = g_file_get_path(file);
    folder_chooser_button_set_folder(data->button, path);
    g_free(path);
    g_object_unref(file);
  }
  gtk_window_destroy(GTK_WINDOW (dialog));
}

static void
folder_chooser_button_clicked(GtkButton *self, gpointer user_data)
{
  GtkWidget *dialog;
  struct folder_chooser_data *data;

  data = (struct folder_chooser_data *) user_data;
  dialog = gtk_file_chooser_dialog_new(data->title,
				       GTK_WINDOW(data->parent),
				       GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER,
				       _("_Cancel"), GTK_RESPONSE_CANCEL,
				       _("_Open"), GTK_RESPONSE_ACCEPT,
				       NULL);
  gtk_file_chooser_set_create_folders(GTK_FILE_CHOOSER(dialog), TRUE);
  // gtk_file_chooser_set_file(GTK_FILE_CHOOSER(dialog), file, NULL);
  gtk_widget_show (dialog);
  g_signal_connect (dialog, "response", G_CALLBACK(on_open_response), user_data);
}

static GtkWidget *
folder_chooser_button_new(const char *title, GtkWidget *parent)
{
  struct folder_chooser_data *data;

  data = g_malloc(sizeof(*data));
  if (data == NULL) {
    return NULL;
  }
  data->title = g_strdup(title);
  data->folder = NULL;
  data->parent = parent;
  data->button = gtk_button_new();
  g_signal_connect(data->button, "clicked", G_CALLBACK(folder_chooser_button_clicked), data);
  g_object_set_data(G_OBJECT(data->button), FOLDER_CHOOSER_DATA_KEY, data);
  return data->button;
}

static void
DirectoryDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct DirectoryDialog *d;
  char *cwd;

  d = (struct DirectoryDialog *) data;
  if (makewidget) {
    GtkWidget *w, *table;
    table = gtk_grid_new();

    w = folder_chooser_button_new(_("directory"), wi);
    d->dir = w;
    add_widget_to_table(table, w, _("_Select Dir:"), TRUE, 0);

    w = gtk_label_new(_("Current Dir:"));
    gtk_widget_set_halign(w, GTK_ALIGN_START);
    set_widget_margin_all(w, 4);
    gtk_grid_attach(GTK_GRID(table), w, 0, 1, 1, 1);

    w = gtk_label_new("");
    gtk_label_set_ellipsize(GTK_LABEL(w), PANGO_ELLIPSIZE_START);
    d->dir_label = w;
    gtk_widget_set_hexpand(w, TRUE);
    gtk_widget_set_halign(w, GTK_ALIGN_START);
    set_widget_margin_all(w, 4);
    gtk_grid_attach(GTK_GRID(table), w, 1, 1, 1, 1);

    gtk_box_append(GTK_BOX(d->vbox), table);
  }

  cwd = ngetcwd();
  if (cwd) {
    folder_chooser_button_set_folder(d->dir, cwd);
    gtk_label_set_text(GTK_LABEL(d->dir_label), cwd);
    g_free(cwd);
  }
}

static void
DirectoryDialogClose(GtkWidget *w, void *data)
{
  struct DirectoryDialog *d;
  char *s;

  d = (struct DirectoryDialog *) data;
  if (d->ret == IDCANCEL)
    return;

  s = g_strdup(folder_chooser_button_get_folder(d->dir));

  if (s && strlen(s) > 0) {
    if (nchdir(s)) {
      ErrorMessage();
    }
  }

  if (s)
    g_free(s);
}

void
DirectoryDialog(struct DirectoryDialog *data)
{
  data->SetupWindow = DirectoryDialogSetup;
  data->CloseWindow = DirectoryDialogClose;
}

static void
LoadDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct LoadDialog *d;

  d = (struct LoadDialog *) data;
  if (makewidget) {
    GtkWidget *w, *vbox;
    int j;
    vbox = gtk_grid_new();

    w = gtk_check_button_new_with_mnemonic(_("_Expand included file"));
    d->expand_file = w;
    gtk_grid_attach(GTK_GRID(vbox), w, 0, 0, 2, 1);

    w = folder_chooser_button_new(_("Expand directory"), wi);
    add_widget_to_table(vbox, w, _("expand _Directory:"), FALSE, 1);
    d->dir = w;

    w = combo_box_create();
    add_widget_to_table(vbox, w, _("_Path:"), FALSE, 2);
    for (j = 0; LoadPathStr[j]; j++) {
      combo_box_append_text(w, _(LoadPathStr[j]));
    }
    d->load_path = w;

    gtk_box_append(GTK_BOX(d->vbox), vbox);
  }
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->expand_file), d->expand);
  combo_box_set_active(d->load_path, d->loadpath);
  folder_chooser_button_set_folder(d->dir, d->exdir);
}

static void
LoadDialogClose(GtkWidget *w, void *data)
{
  struct LoadDialog *d;

  d = (struct LoadDialog *) data;
  if (d->ret == IDCANCEL)
    return;
  d->expand = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->expand_file));
  if (d->exdir) {
    g_free(d->exdir);
  }
  d->exdir = g_strdup(folder_chooser_button_get_folder(d->dir));
  d->loadpath = combo_box_get_active(d->load_path);
}

void
LoadDialog(struct LoadDialog *data)
{
  data->SetupWindow = LoadDialogSetup;
  data->CloseWindow = LoadDialogClose;
  data->expand = Menulocal.expand;
  if (data->exdir) {
    g_free(data->exdir);
  }
  data->exdir = g_strdup(Menulocal.expanddir);
  data->loadpath = Menulocal.loadpath;
}

static void
SaveDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct SaveDialog *d;

  d = (struct SaveDialog *) data;
  if (makewidget) {
    GtkWidget *w, *vbox;
    int j;
    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    w = combo_box_create();
    item_setup(vbox, w, _("_Path:"), FALSE);
    for (j = 0; pathchar[j] != NULL; j++) {
      combo_box_append_text(w, _(pathchar[j]));
    }
    d->path = w;

    w = gtk_check_button_new_with_mnemonic(_("_Include data file"));
    d->include_data = w;
    gtk_box_append(GTK_BOX(vbox), w);

    w = gtk_check_button_new_with_mnemonic(_("_Include merge file"));
    d->include_merge = w;
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_box_append(GTK_BOX(d->vbox), vbox);
  }
  combo_box_set_active(d->path, Menulocal.savepath);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->include_data), Menulocal.savewithdata);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->include_merge), Menulocal.savewithmerge);
  d->focus = d->include_data;
}

static void
SaveDialogClose(GtkWidget *w, void *data)
{
  int num;
  struct SaveDialog *d;

  d = (struct SaveDialog *) data;
  if (d->ret == IDCANCEL)
    return;

  num = combo_box_get_active(d->path);
  if (num >= 0) {
    d->Path = num;
  }
  d->SaveData = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->include_data));
  d->SaveMerge = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->include_merge));
}

void
SaveDialog(struct SaveDialog *data)
{
  data->SetupWindow = SaveDialogSetup;
  data->CloseWindow = SaveDialogClose;
  data->SaveData = FALSE;
  data->SaveMerge = FALSE;
}

static void
draw_callback(gpointer user_data)
{
  UpdateAll2(NULL, FALSE);
  InfoWinClear();
  menu_clear_undo();
}

static void
graph_new_cb_response(int res, gpointer user_data)
{

  SetFileName(NULL);
  set_axis_undo_button_sensitivity(FALSE);
  reset_graph_modified();

  Draw(FALSE, draw_callback, NULL);
}

static void
graph_new_cb(int response, gpointer user_data)
{
  int sel;

  if (response != IDOK) {
    return;
  }

  DeleteDrawable();
  sel = GPOINTER_TO_INT(user_data);
  switch (sel) {
  case MenuIdGraphNewFrame:
    CmAxisNewFrame(FALSE, graph_new_cb_response);
    break;
  case MenuIdGraphNewSection:
    CmAxisNewSection(FALSE, graph_new_cb_response);
    break;
  case MenuIdGraphNewCross:
    CmAxisNewCross(FALSE, graph_new_cb_response);
    break;
  case MenuIdGraphAllClear:
    graph_new_cb_response(IDOK, NULL);
  default:
    break;
  }
}

static void
CmGraphNewMenu_response(int ret, gpointer user_data)
{
  struct graph_page_data *data;

  if (! ret) {
    return;
  }

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  data->cb = graph_new_cb;
  data->sel = GPOINTER_TO_INT(user_data);
  GraphPage(TRUE, data);
}

void
CmGraphNewMenu(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;

  CheckSave(CmGraphNewMenu_response, client_data);
}

static void
CmGraphLoad_response_response(char *file, gpointer user_data)
{
  char *cwd;
  cwd = (char *) user_data;
  if (file == NULL) {
    if (cwd) {
      g_free(cwd);
    }
    return;
  }

  LoadNgpFile(file, Menulocal.scriptconsole, "-f", cwd);
  if (cwd) {
    g_free(cwd);
  }
  g_free(file);
}

static void
CmGraphLoad_response(int ret, gpointer user_data)
{
  char *cwd;
  int chd;

  if (! ret) {
    return;
  }
  cwd = ngetcwd();
  chd = Menulocal.changedirectory;
  nGetOpenFileName(TopLevel,
                   _("Load NGP file"), "ngp", &(Menulocal.graphloaddir),
                   NULL, chd,
                   CmGraphLoad_response_response, cwd);
}

void
CmGraphLoad(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;

  CheckSave(CmGraphLoad_response, NULL);
}

void
CmGraphSave(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  GraphSave(FALSE);
}

void
CmGraphOverWrite(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  GraphSave(TRUE);
}

static void
CmGraphSwitch_response(struct response_callback *cb)
{
  if (cb->return_value == IDOK) {
    set_graph_modified_gra();
    ChangePage();
  }
}

void
CmGraphSwitch(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  SwitchDialog(&DlgSwitch);
  response_callback_add(&DlgSwitch, CmGraphSwitch_response, NULL, NULL);
  DialogExecute(TopLevel, &DlgSwitch);
}

static void
CmGraphPage_response(struct response_callback *cb)
{
  struct graph_page_data *data;

  data = (struct graph_page_data *) cb->data;
  if (cb->return_value == IDOK) {
    int new_graph;
    SetPageSettingsToGRA();
    ChangePage();
    GetPageSettingsFromGRA();
    new_graph = ((struct PageDialog *) cb->dialog)->new_graph;
    if (! new_graph) {
      set_graph_modified_gra();
    }
  }
  if (data) {
    data->cb(cb->return_value, GINT_TO_POINTER(data->sel));
    g_free(data);
  }
}

static void
GraphPage(int new_graph, struct graph_page_data *data)
{
  if (Menulock || Globallock)
    return;
  PageDialog(&DlgPage, new_graph);
  response_callback_add(&DlgPage, CmGraphPage_response, NULL, data);
  DialogExecute(TopLevel, &DlgPage);
}

void
CmGraphPage(void *w, gpointer client_data)
{
  GraphPage(GPOINTER_TO_INT(client_data), NULL);
}

void
CmGraphDirectory(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  DirectoryDialog(&DlgDirectory);
  DialogExecute(TopLevel, &DlgDirectory);
}

#if ! USE_EVENT_LOOP
static void
graph_shell_finalize(gpointer user_data)
{
  GThread *thread;

  thread = (GThread *) user_data;

  menu_lock(FALSE);
  set_graph_modified();
  UpdateAll(NULL);
  gtk_widget_set_sensitive(TopLevel, TRUE);

  g_thread_join(thread);
}

static gpointer
graph_shell_main(gpointer user_data)
{
  struct objlist *shell;
  GThread *thread;

  shell = chkobject("shell");
  if (shell) {
    int allocnow, n;
    n = chkobjlastinst(shell);
    if (n < 0) {
      newobj(shell);
    }
    allocnow = allocate_console();
    exeobj(shell, "shell", 0, 0, NULL);
    free_console(allocnow);
  }

  thread = g_thread_self();
  g_idle_add_once(graph_shell_finalize, thread);
  return NULL;
}
#endif

#if USE_EVENT_LOOP
void
CmGraphShell(void *w, gpointer client_data)
{
  struct objlist *obj, *robj, *shell;
  N_VALUE *inst;
  int idn;

  if (Menulock || Globallock)
    return;

  menu_lock(TRUE);

  menu_save_undo(UNDO_TYPE_SHLL, NULL);
  obj = Menulocal.obj;
  inst = Menulocal.inst;
  idn = getobjtblpos(obj, "_evloop", &robj);
  registerevloop(chkobjectname(obj), "_evloop", robj, idn, inst, NULL);
  if ((shell = chkobject("shell")) != NULL) {
    int allocnow, n;
    n = chkobjlastinst(shell);
    if (n < 0) {
      newobj(shell);
    }
    allocnow = allocate_console();
    exeobj(shell, "shell", 0, 0, NULL);
    free_console(allocnow);
  }
  unregisterevloop(robj, idn, inst);
  menu_lock(FALSE);
  gtk_widget_set_sensitive(TopLevel, FALSE);
  set_graph_modified();
  UpdateAll(NULL);
}
#else
void
CmGraphShell(void *w, gpointer client_data)
{
  if (Menulock || Globallock) {
    return;
  }
  menu_lock(TRUE);
  menu_save_undo(UNDO_TYPE_SHLL, NULL);
  gtk_widget_set_sensitive(TopLevel, FALSE);
  g_thread_new(NULL, graph_shell_main, NULL);
}
#endif

void
CmGraphQuit(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;

  QuitGUI();
}

int
chdir_to_ngp(const char *fname)
{
  char *path;
  int r;
  path = g_path_get_dirname(fname);
  if (path == NULL) {
    return 0;
  }
  r = nchdir(path);
  g_free(path);
  return r;
}

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
void
CmGraphHistory(GtkRecentChooser *w, gpointer client_data)
{
  char *uri, *fname, *cwd;

  if (Menulock || Globallock)
    return;

  uri = gtk_recent_chooser_get_current_uri(w);
  if (uri == NULL) {
    return;
  }

  fname = g_filename_from_uri(uri, NULL, NULL);
  g_free(uri);
  if (fname == NULL) {
    return;
  }

  if (!CheckSave()) {
    g_free(fname);
    return;
  }

  cwd = ngetcwd();
  if (chdir_to_ngp(fname)) {
    ErrorMessage();
    g_free(fname);
    if (cwd) {
      g_free(cwd);
    }
    return;
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  LoadNgpFile(fname, Menulocal.scriptconsole, "-f", cwd);
#else
  if (LoadNgpFile(fname, Menulocal.scriptconsole, "-f") && cwd) {
    nchdir(cwd);
  }
#endif
  if (cwd) {
    g_free(cwd);
  }
  g_free(fname);
}
#endif

void
CmHelpAbout(void *w, gpointer client_data)
{
  struct objlist *obj;
  char *web, *copyright;
#if GTK_CHECK_VERSION(4, 0, 0)
  struct objlist *system;
  GdkTexture *logo;
  char *lib_version, *compiler, *str;
#endif

  if (Menulock || Globallock)
    return;

  if ((obj = chkobject("system")) == NULL)
    return;

  getobj(obj, "copyright", 0, 0, NULL, &copyright);
  getobj(obj, "web", 0, 0, NULL, &web);

#if GTK_CHECK_VERSION(4, 0, 0)
  logo = gdk_texture_new_from_resource(NGRAPH_ICON128_FILE);

  system = getobject("system");
  getobj(system, "compiler", 0, 0, NULL, &compiler);
  getobj(Menulocal.obj, "lib_version", 0, 0, NULL, &lib_version);
  str = g_strdup_printf("compiler:\n"
			"%s\n"
			"\n"
			"library:\n"
			"%s\n",
			compiler,
			lib_version);
#endif
  gtk_show_about_dialog(GTK_WINDOW(TopLevel),
			"program-name", PACKAGE,
			"copyright", copyright,
			"version", VERSION,
			"website", web,
			"license-type", GTK_LICENSE_GPL_2_0,
			"wrap-license", TRUE,
			"authors", Auther,
			"translator-credits", Translator,
			"documenters", Documenter,
#if GTK_CHECK_VERSION(4, 0, 0)
			"logo", logo,
			"system-information", str,
#endif
			"comments", _("Ngraph is the program to create scientific 2-dimensional graphs for researchers and engineers."),
			NULL);
#if GTK_CHECK_VERSION(4, 0, 0)
  g_free(str);
  g_object_unref(logo);
#endif
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
CmHelpDemo_response(int ret, gpointer client_data)
{
  char *demo_file, *data_dir;

  if (! ret) {
    return;
  }

  data_dir = (char *) client_data;
  demo_file = g_strdup_printf("%s/demo/demo.ngp", data_dir);
  if (demo_file == NULL) {
    return;
  }

  LoadNgpFile(demo_file, Menulocal.scriptconsole, "-f", NULL);
  g_free(demo_file);
}
#endif

void
CmHelpDemo(void *w, gpointer client_data)
{
  struct objlist *obj;
  char *demo_file, *data_dir;

  if (Menulock || Globallock)
    return;

  obj = chkobject("system");
  if (obj == NULL) {
    return;
  }

  getobj(obj, "data_dir", 0, 0, NULL, &data_dir);
  if (data_dir == NULL) {
    return;
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  CheckSave(CmHelpDemo_response, data_dir);
#else
  if (!CheckSave()) {
    return;
  }

  demo_file = g_strdup_printf("%s/demo/demo.ngp", data_dir);
  if (demo_file == NULL) {
    return;
  }

  LoadNgpFile(demo_file, Menulocal.scriptconsole, "-f");
  g_free(demo_file);
#endif
}

void
CmHelpHelp(void *w, gpointer client_data)
{
  char *cmd;

  if (Menulock || Globallock)
    return;

  if (Menulocal.help_browser == NULL)
    return;

  cmd = g_strdup_printf("%s \"%s/%s\"", Menulocal.help_browser, DOCDIR, Menulocal.help_file);

  system_bg(cmd);

  g_free(cmd);
}

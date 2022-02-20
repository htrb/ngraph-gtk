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

#if GTK_CHECK_VERSION(4, 0, 0)
  if (Menulocal.PaperLandscape) {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->landscape), TRUE);
  } else {
    gtk_check_button_set_active(GTK_CHECK_BUTTON(d->portrait), TRUE);
  }
#else
  if (Menulocal.PaperLandscape) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->landscape), TRUE);
  } else {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->portrait), TRUE);
  }
#endif

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
#if GTK_CHECK_VERSION(4, 0, 0)
  landscape = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->landscape));
#else
  landscape = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->landscape));
#endif

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
#if GTK_CHECK_VERSION(4, 0, 0)
    GtkWidget *group;
#endif
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

#if GTK_CHECK_VERSION(4, 0, 0)
    w = gtk_check_button_new_with_mnemonic(_("_Portrait"));
    group = w;
    gtk_check_button_set_active(GTK_CHECK_BUTTON(w), TRUE);
#else
    w = gtk_radio_button_new_with_mnemonic(NULL, _("_Portrait"));
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TRUE);
#endif
    add_widget_to_table(table, w, _("_Orientation:"), FALSE, i++);
    d->portrait = w;

#if GTK_CHECK_VERSION(4, 0, 0)
    w = gtk_check_button_new_with_mnemonic(_("L_andscape"));
    gtk_check_button_set_group(GTK_CHECK_BUTTON(w), GTK_CHECK_BUTTON(group));
#else
    w = gtk_radio_button_new_with_mnemonic_from_widget(GTK_RADIO_BUTTON(w), _("L_andscape"));
#endif
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

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), table);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), table, FALSE, FALSE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
  }
  PageDialogSetupItem(wi, d);
  d->show_cancel = ! d->new_graph;
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
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), label);
#else
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 4);
#endif

    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    d->drawlist = w;
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
    g_signal_connect(sel, "changed", G_CALLBACK(drawlist_sel_cb), d);


    frame = gtk_frame_new(NULL);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_frame_set_child(GTK_FRAME(frame), w);
    gtk_box_append(GTK_BOX(vbox), frame);
    gtk_box_append(GTK_BOX(hbox), vbox);
#else
    gtk_container_add(GTK_CONTAINER(frame), w);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);
#endif

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    w = gtk_button_new_with_mnemonic(_("_Add"));
    set_button_icon(w, "list-add");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogAdd), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
#endif
    d->add = w;

    w = gtk_button_new_with_mnemonic(_("_Insert"));
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogInsert), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
#endif
    d->ins = w;

    w = gtk_separator_new(GTK_ORIENTATION_HORIZONTAL);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
#endif

    w = gtk_button_new_with_mnemonic(_("_Top"));
    set_button_icon(w, "go-top");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogTop), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
#endif
    d->top =w;

    w = gtk_button_new_with_mnemonic(_("_Up"));
    set_button_icon(w, "go-up");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogUp), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
#endif
    d->up = w;

    w = gtk_button_new_with_mnemonic(_("_Down"));
    set_button_icon(w, "go-down");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogDown), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
#endif
    d->down = w;

    w = gtk_button_new_with_mnemonic(_("_Bottom"));
    set_button_icon(w, "go-bottom");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogLast), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
#endif
    d->bottom = w;

    w = gtk_button_new_with_mnemonic(_("_Remove"));
    set_button_icon(w, "list-remove");
    g_signal_connect(w, "clicked", G_CALLBACK(SwitchDialogRemove), d);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
#endif
    d->del = w;

    vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox2), vbox);
#else
    gtk_box_pack_end(GTK_BOX(vbox2), vbox, FALSE, FALSE, 4);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox2), vbox);

    gtk_box_append(GTK_BOX(hbox), vbox2);
#else
    gtk_box_pack_end(GTK_BOX(vbox2), vbox, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(hbox), vbox2, FALSE, FALSE, 4);
#endif

    vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

    label = gtk_label_new_with_mnemonic(_("_Objects"));
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), label);
#else
    gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 4);
#endif

    w = list_store_create(sizeof(list) / sizeof(*list), list);
    list_store_set_selection_mode(w, GTK_SELECTION_MULTIPLE);
    d->objlist = w;
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);

    sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
    g_signal_connect(sel, "changed", G_CALLBACK(objlist_sel_cb), d);


    frame = gtk_frame_new(NULL);

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_frame_set_child(GTK_FRAME(frame), w);
    gtk_box_append(GTK_BOX(vbox), frame);
    gtk_box_append(GTK_BOX(hbox), vbox);
    gtk_box_append(GTK_BOX(d->vbox), hbox);
#else
    gtk_container_add(GTK_CONTAINER(frame), w);
    gtk_box_pack_start(GTK_BOX(vbox), frame, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(hbox), vbox, TRUE, TRUE, 4);
    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, TRUE, TRUE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
struct folder_chooser_data {
  char *folder, *title;
  GtkWidget *parent, *button;
};

#define FOLDER_CHOOSER_DATA_KEY "FOLDER_CHOOSER_DATA"

void
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

GtkWidget *
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
#endif

static void
DirectoryDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  struct DirectoryDialog *d;
  char *cwd;

  d = (struct DirectoryDialog *) data;
  if (makewidget) {
    GtkWidget *w, *table;
    table = gtk_grid_new();

#if GTK_CHECK_VERSION(4, 0, 0)
    w = folder_chooser_button_new(_("directory"), wi);
#else
    w = gtk_file_chooser_button_new(_("directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(w), TRUE);
#endif
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

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), table);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), table, FALSE, FALSE, 4);

    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
  }

  cwd = ngetcwd();
  if (cwd) {
#if GTK_CHECK_VERSION(4, 0, 0)
    folder_chooser_button_set_folder(d->dir, cwd);
#else
    gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(d->dir), cwd);
#endif
    gtk_label_set_text(GTK_LABEL(d->dir_label), cwd);
    g_free(cwd);
  }
}

static void
DirectoryDialogClose(GtkWidget *w, void *data)
{
  struct DirectoryDialog *d;
#if GTK_CHECK_VERSION(4, 0, 0)
  const char *tmp;
#endif
  char *s;

  d = (struct DirectoryDialog *) data;
  if (d->ret == IDCANCEL)
    return;

#if GTK_CHECK_VERSION(4, 0, 0)
  tmp = folder_chooser_button_get_folder(d->dir);
  s = g_strdup(tmp);
#else
  gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(d->dir), cwd);
  s = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d->dir));
#endif

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

#if ! GTK_CHECK_VERSION(4, 0, 0)
static void
set_directory_name(GtkFileChooserButton *widget, gpointer user_data)
{
  char *path;
  path = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget));
  gtk_widget_set_tooltip_text(GTK_WIDGET(widget), path);
  if (path) {
    g_free(path);
  }
}
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
    w = folder_chooser_button_new(_("Expand directory"), wi);
#else
    w = gtk_file_chooser_button_new(_("Expand directory"), GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER);
    gtk_file_chooser_set_local_only(GTK_FILE_CHOOSER(w), TRUE);
    g_signal_connect(w, "file-set", G_CALLBACK(set_directory_name), NULL);
#endif
    add_widget_to_table(vbox, w, _("expand _Directory:"), FALSE, 1);
    d->dir = w;

    w = combo_box_create();
    add_widget_to_table(vbox, w, _("_Path:"), FALSE, 2);
    for (j = 0; LoadPathStr[j]; j++) {
      combo_box_append_text(w, _(LoadPathStr[j]));
    }
    d->load_path = w;

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(d->vbox), vbox);
#else
    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
  }
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->expand_file), d->expand);
#else
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->expand_file), d->expand);
#endif
  combo_box_set_active(d->load_path, d->loadpath);
#if GTK_CHECK_VERSION(4, 0, 0)
  folder_chooser_button_set_folder(d->dir, d->exdir);
#else
  gtk_file_chooser_set_filename(GTK_FILE_CHOOSER(d->dir), d->exdir);
  set_directory_name(GTK_FILE_CHOOSER_BUTTON(d->dir), NULL);
#endif
}

static void
LoadDialogClose(GtkWidget *w, void *data)
{
  struct LoadDialog *d;

  d = (struct LoadDialog *) data;
  if (d->ret == IDCANCEL)
    return;
#if GTK_CHECK_VERSION(4, 0, 0)
  d->expand = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->expand_file));
#else
  d->expand = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->expand_file));
#endif
  if (d->exdir) {
    g_free(d->exdir);
  }
#if GTK_CHECK_VERSION(4, 0, 0)
  d->exdir = g_strdup(folder_chooser_button_get_folder(d->dir));
#else
  d->exdir = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(d->dir));
#endif
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
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
#endif

    w = gtk_check_button_new_with_mnemonic(_("_Include merge file"));
    d->include_merge = w;
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_box_append(GTK_BOX(vbox), w);
    gtk_box_append(GTK_BOX(d->vbox), vbox);
#else
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);

    gtk_box_pack_start(GTK_BOX(d->vbox), vbox, FALSE, FALSE, 4);
    gtk_widget_show_all(GTK_WIDGET(d->vbox));
#endif
  }
  combo_box_set_active(d->path, Menulocal.savepath);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->include_data), Menulocal.savewithdata);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(d->include_merge), Menulocal.savewithmerge);
#else
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->include_data), Menulocal.savewithdata);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(d->include_merge), Menulocal.savewithmerge);
#endif
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
#if GTK_CHECK_VERSION(4, 0, 0)
  *(d->SaveData) = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->include_data));
  *(d->SaveMerge) = gtk_check_button_get_active(GTK_CHECK_BUTTON(d->include_merge));
#else
  *(d->SaveData) = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->include_data));
  *(d->SaveMerge) = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(d->include_merge));
#endif
}

void
SaveDialog(struct SaveDialog *data, int *sdata, int *smerge)
{
  data->SetupWindow = SaveDialogSetup;
  data->CloseWindow = SaveDialogClose;
  data->SaveData = sdata;
  data->SaveMerge = smerge;
}

void
CmGraphNewMenu(void *w, gpointer client_data)
{
  int sel;

  if (Menulock || Globallock)
    return;

  if (!CheckSave())
    return;

  DeleteDrawable();

  CmGraphPage(NULL, GINT_TO_POINTER(TRUE));
  sel = GPOINTER_TO_INT(client_data);
  switch (sel) {
  case MenuIdGraphNewFrame:
    CmAxisNewFrame(FALSE);
    break;
  case MenuIdGraphNewSection:
    CmAxisNewSection(FALSE);
    break;
  case MenuIdGraphNewCross:
    CmAxisNewCross(FALSE);
    break;
  case MenuIdGraphAllClear:
  default:
    break;
  }

  SetFileName(NULL);
  set_axis_undo_button_sensitivity(FALSE);
  reset_graph_modified();

  CmViewerDraw(NULL, GINT_TO_POINTER(TRUE));
  UpdateAll2(NULL, FALSE);
  InfoWinClear();
  menu_clear_undo();
}

void
CmGraphLoad(void *w, gpointer client_data)
{
  char *file, *cwd;
  int chd;

  if (Menulock || Globallock)
    return;

  if (!CheckSave())
    return;

  cwd = ngetcwd();
#if GTK_CHECK_VERSION(4, 0, 0)
  chd = TRUE;
#else
  chd = Menulocal.changedirectory;
#endif
  if (nGetOpenFileName(TopLevel,
		       _("Load NGP file"), "ngp", &(Menulocal.graphloaddir),
		       NULL, &file, TRUE,
		       chd) != IDOK) {
    if (cwd) {
      g_free(cwd);
    }
    return;
  }

  if (LoadNgpFile(file, Menulocal.scriptconsole, "-f") && cwd) {
    nchdir(cwd);
  }
  if (cwd) {
    g_free(cwd);
  }
  g_free(file);
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

void
CmGraphSwitch(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  SwitchDialog(&DlgSwitch);
  if (DialogExecute(TopLevel, &DlgSwitch) == IDOK) {
    set_graph_modified_gra();
    ChangePage();
  }
}

void
CmGraphPage(void *w, gpointer client_data)
{
  int new_graph;
  new_graph = GPOINTER_TO_INT(client_data);
  if (Menulock || Globallock)
    return;
  PageDialog(&DlgPage, new_graph);
  if (DialogExecute(TopLevel, &DlgPage) == IDOK) {
    SetPageSettingsToGRA();
    ChangePage();
    GetPageSettingsFromGRA();
    if (! new_graph) {
      set_graph_modified_gra();
    }
  }
}

void
CmGraphDirectory(void *w, gpointer client_data)
{
  if (Menulock || Globallock)
    return;
  DirectoryDialog(&DlgDirectory);
  DialogExecute(TopLevel, &DlgDirectory);
}

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
  set_graph_modified();
  UpdateAll(NULL);
}

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

  if (LoadNgpFile(fname, Menulocal.scriptconsole, "-f") && cwd) {
    nchdir(cwd);
  }
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

  if (!CheckSave()) {
    return;
  }

  demo_file = g_strdup_printf("%s/demo/demo.ngp", data_dir);
  if (demo_file == NULL) {
    return;
  }

  LoadNgpFile(demo_file, Menulocal.scriptconsole, "-f");
  g_free(demo_file);
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

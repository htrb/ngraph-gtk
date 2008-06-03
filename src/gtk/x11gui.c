/* 
 * $Id: x11gui.c,v 1.2 2008/06/03 04:44:45 hito Exp $
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
  char *initfil;
  int chdir;
  char *filter;
  char *ext;
  char **file;
  int mustexist;
  int multi;
  int changedir;
};

static struct nGetOpenFileData FileSelection = {NULL, NULL};


static void
dialog_destroyed_cb(GtkWidget *w, gpointer user_data)
{
  ((struct DialogType *) user_data)->widget = NULL;
}

int
DialogExecute(GtkWidget *parent, void *dialog)
{
  GtkWidget *dlg;
  struct DialogType *data;
  gint res_id;

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

    g_signal_connect(dlg, "destroy", G_CALLBACK(dialog_destroyed_cb), data);
    // gtk_widget_destroyed(dlg, &(data->widget));
    /* it seems that gtk_widget_destroyed() is not work well. */

    data->parent = parent;
    data->widget = dlg;
    data->vbox = GTK_VBOX((GTK_DIALOG(dlg)->vbox));
    data->show_buttons = TRUE;
    data->show_cancel = TRUE;

    gtk_window_set_title(GTK_WINDOW(dlg), _(data->resource));

    data->SetupWindow(dlg, data, TRUE);

    if (data->show_cancel)
      gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL);

    gtk_dialog_add_button(GTK_DIALOG(dlg), GTK_STOCK_OK, GTK_RESPONSE_OK);
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  } else {
    dlg = data->widget;
    data->SetupWindow(dlg, data, FALSE);
  }

  gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_OK);
  data->widget = dlg;
  data->ret = IDLOOP;

  while (data->ret == IDLOOP) {
    gtk_widget_show_all(dlg);

    if (! data->show_buttons) {
      gtk_widget_hide(GTK_DIALOG(dlg)->action_area);
      gtk_dialog_set_has_separator(GTK_DIALOG(dlg), FALSE);
    }

    res_id = gtk_dialog_run(GTK_DIALOG(dlg));

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

    if (data->CloseWindow)
      data->CloseWindow(dlg, data);
  }

  //  gtk_widget_destroy(dlg);
  //  data->widget = NULL;
  gtk_widget_hide_all(dlg);
  ResetEvent();

  return data->ret;
}

void
MessageBeep(GtkWidget * parent)
{
  gdk_beep();
  ResetEvent();
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
  res_id = gtk_dialog_run(GTK_DIALOG(dlg));

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
DialogInput(GtkWidget * parent, char *title, char *mes, char **s)
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

  gtk_widget_show_all(dlg);
  res_id = gtk_dialog_run(GTK_DIALOG(dlg));

  switch (res_id) {
  case GTK_RESPONSE_OK:
    *s = strdup(gtk_entry_get_text(GTK_ENTRY(text)));
    data = IDOK;
    break;
  default:
    data = IDCANCEL; 
    break;
  }

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
  int i, k, len, n, l;
  struct stat buf;
  GSList *top, *list;

  data = &FileSelection;

  top = gtk_file_chooser_get_filenames(GTK_FILE_CHOOSER(dlg));
  n = g_slist_length(top);
  farray = malloc(sizeof(*farray) * n + 1);
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

    l = strlen(file);
    file2 = malloc(l + len + 1);
    if (file2) {
      strcpy(file2, file);
      if (len != 0) {
	file2[l] = '.';
	strcpy(file2 + l + 1, data->ext);
      }
      if (data->mustexist) {
	if ((stat(file2, &buf) != 0) || ((buf.st_mode & S_IFMT) != S_IFREG) 
	    || (access(file2, R_OK) != 0)) {
	  gdk_beep();
	  free(file2);
	  continue;
	}
      } else {
	if ((stat(file2, &buf) == 0) && ((buf.st_mode & S_IFMT) != S_IFREG)) {
	  gdk_beep();
	  free(file2);
	  continue;
	}
      }
      farray[k] = file2;
      k++;
    }
  }

  if (data->changedir && k > 0) {
    data->chdir = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(data->chdir_cb));
    if (data->chdir && data->initdir) {
      char *dir, *tmp, *tmp2;

      free(*(data->initdir));
      tmp = strdup(farray[0]);
      dir = dirname(tmp);
      tmp2 = strdup(dir);
      *(data->initdir) = tmp2;
      free(tmp);
    }
  }
  farray[k] = NULL;
  free_str_list(top);
  data->ret = IDOK;
}

static int
FileSelectionDialog(GtkWidget * parent, int type, char *stock)
{
  struct nGetOpenFileData *data;
  GtkWidget *dlg, *rc;
  GtkFileFilter *filter;

  data = &FileSelection;

  data->parent = parent;
  dlg = gtk_file_chooser_dialog_new(data->title,
				    GTK_WINDOW(parent),
				    type,
				    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				    stock, GTK_RESPONSE_ACCEPT,
				    NULL);
  rc = gtk_check_button_new_with_label(_("_Change current directory"));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dlg)->vbox), rc, FALSE, FALSE, 5);
  data->chdir_cb = rc;
  gtk_file_chooser_set_select_multiple(GTK_FILE_CHOOSER(dlg), data->multi);
  data->widget = dlg;

  if (data->filter) {
    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, data->filter);
    gtk_file_filter_set_name(filter, data->filter);
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);


    filter = gtk_file_filter_new();
    gtk_file_filter_add_pattern(filter, "*.*");
    gtk_file_filter_set_name(filter, _("All"));
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dlg), filter);
  }

  if (data->initdir && *(data->initdir)) {
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), *(data->initdir));
  }
  gtk_widget_show_all(dlg);

  if (data->changedir) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(data->chdir_cb), data->chdir);
  } else {
    gtk_widget_hide(data->chdir_cb);
  }

  if (data->initfil) {
    char *tmp, *name;
    tmp = strdup(data->initfil);
    name = basename(tmp);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), name);
    free(tmp);
  }

  data->ret = IDCANCEL;

  if (gtk_dialog_run(GTK_DIALOG(dlg)) == GTK_RESPONSE_ACCEPT) {
    fsok(dlg);
  }

  ResetEvent();
  gtk_widget_destroy(dlg);
  data->widget = NULL;

  return data->ret;
}

int
nGetOpenFileNameMulti(GtkWidget * parent,
		      char *title, char *defext, char **initdir,
		      char *initfil, char ***file, char *filter, int chd)
{
  int ret;
  
  FileSelection.title = title;
  FileSelection.initdir = initdir;
  FileSelection.initfil = initfil;
  FileSelection.filter = filter;
  FileSelection.file = NULL;
  FileSelection.chdir = chd;
  FileSelection.ext = defext;
  FileSelection.mustexist = TRUE;
  FileSelection.multi = TRUE;
  FileSelection.changedir = TRUE;
  ret = FileSelectionDialog(parent, GTK_FILE_CHOOSER_ACTION_OPEN, GTK_STOCK_OPEN);
  if (ret == IDOK) {
    *file = FileSelection.file;
    if (FileSelection.chdir && initdir) chdir(*initdir);
  } else {
    *file = NULL;
  }

  return ret;
}

int
nGetOpenFileName(GtkWidget * parent,
		 char *title, char *defext, char **initdir, char *initfil,
		 char **file, char *filter, int exist, int chd)
{
  int ret;
  
  FileSelection.title = title;
  FileSelection.initdir = initdir;
  FileSelection.initfil = initfil;
  FileSelection.filter = filter;
  FileSelection.file = NULL;
  FileSelection.chdir = chd;
  FileSelection.ext = defext;
  FileSelection.mustexist = exist;
  FileSelection.multi = FALSE;
  FileSelection.changedir = TRUE;
  ret = FileSelectionDialog(parent,
			    (exist) ?
			    GTK_FILE_CHOOSER_ACTION_OPEN :
			    GTK_FILE_CHOOSER_ACTION_SAVE,
			    GTK_STOCK_OPEN);
  if (ret == IDOK) {
    *file = FileSelection.file[0];
    free(FileSelection.file);
    if (FileSelection.chdir && initdir) chdir(*initdir);
  } else {
    *file = NULL;
  }


  return ret;
}

int
nGetSaveFileName(GtkWidget * parent,
		 char *title, char *defext, char **initdir, char *initfil,
		 char **file, char *filter, int chd)
{
  int ret;

  FileSelection.title = title;
  FileSelection.initdir = initdir;
  FileSelection.initfil = initfil;
  FileSelection.filter = filter;
  FileSelection.file = NULL;
  FileSelection.chdir = chd;
  FileSelection.ext = defext;
  FileSelection.mustexist = FALSE;
  FileSelection.multi = FALSE;
  FileSelection.changedir = TRUE;
  ret = FileSelectionDialog(parent, GTK_FILE_CHOOSER_ACTION_SAVE, GTK_STOCK_SAVE);
  if (ret == IDOK) {
    *file = FileSelection.file[0];
    free(FileSelection.file);
    if (FileSelection.chdir && initdir) chdir(*initdir);
  } else {
    *file = NULL;
  }

  return ret;
}

int
GetColor(GtkWidget * parent, int *r, int *g, int *b)
{
  GtkWidget *dlg;
  GtkColorSelection *csel;
  GdkColor color;
  gint rval;
  int ret;

  dlg = gtk_color_selection_dialog_new(_("Color selection"));
  csel = GTK_COLOR_SELECTION(dlg);

  gtk_color_selection_set_has_opacity_control(csel, FALSE);
  gtk_color_selection_set_has_palette(csel, TRUE);

  rval = gtk_dialog_run(GTK_DIALOG(dlg));

  ret = IDCANCEL;
  if (rval == GTK_RESPONSE_OK) {
    gtk_color_selection_get_current_color(csel, &color);
    *r = color.red;
    *g = color.green;
    *b = color.blue;
    ret = IDOK;
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

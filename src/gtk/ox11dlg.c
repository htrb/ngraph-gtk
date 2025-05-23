/*
 * $Id: ox11dlg.c,v 1.29 2010-03-04 08:30:17 hito Exp $
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
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>

#include "shell.h"
#include "object.h"
#include "mathfn.h"
#include "nstring.h"
#include "ioutil.h"
#include "x11menu.h"
#include "ox11menu.h"

#include "init.h"
#include "x11gui.h"

#define NAME "dialog"
#define PARENT "object"
#define NVERSION  "1.00.00"

#define ERRDISPLAY 100
#define ERRNODLGINST 101

static char *dlgerrorlist[] = {
  "cannot open display.",
  "no instance for dialog",
};

#define ERRNUM (sizeof(dlgerrorlist) / sizeof(*dlgerrorlist))

static GtkWidget *DLGTopLevel = NULL;

struct dialog_data {
  char *title, *msg, *initial_text, *response_text, *defext, **files;
  struct narray *buttons, *sarray;
  int response, wait, *button, selected, *ival, quit_main_loop;
  double min, max, inc, *val;
};

static int
dialog_run(char *title, char *mes, GSourceOnceFunc func, struct dialog_data *data)
{
  data->wait = TRUE;
  data->msg = mes;
  data->title = title;
  data->response_text = NULL;
  if (main_loop_is_running()) {
    data->quit_main_loop = FALSE;
    g_idle_add_once(func, data);
    dialog_wait(&data->wait);
  } else {
    data->quit_main_loop = TRUE;
    func(data);
    main_loop_run();
  }
  return data->response;
}

static GtkWidget *
get_toplevel_window(void)
{
  if (TopLevel) {
    return TopLevel;
  }

  if (DLGTopLevel == NULL) {
    DLGTopLevel = gtk_window_new();
  }

  return DLGTopLevel;
}

static int
dlginit(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
 if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  if (!OpenApplication()) {
    error(obj, ERRDISPLAY);
    return 1;
  }

  return 0;
}


static int
dlgdone(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;
  return 0;
}

static void
dlg_response(int response, gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  data->response = response;
  data->wait = FALSE;
  if (data->quit_main_loop) {
    g_idle_add_once((GSourceOnceFunc) g_main_loop_quit, main_loop());
  }
}

static void
dlgconfirm_main(gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  response_message_box(get_toplevel_window(), data->msg, data->title, RESPONS_YESNO, dlg_response, data);
}

static int
dlgconfirm(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *mes, *title;
  int rcode, locksave;
  struct dialog_data data;

  memset(&data, 0, sizeof(data));

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  mes = (char *)argv[2];
  locksave = Globallock;
  Globallock = TRUE;
  mes = CHK_STR(mes);
  rcode = dialog_run(title ? title : _("Confirm"), mes, dlgconfirm_main, &data);
  Globallock = locksave;
  if (rcode == IDYES) {
    rval->i = 1;
  } else {
    rval->i = 0;
  }
  return (rcode == IDYES)? 0 : 1;
}

static void
dlgmessage_main(gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  markup_message_box_full(get_toplevel_window(), CHK_STR(data->msg), (data->title) ? data->title : _("Message"), RESPONS_OK, FALSE, dlg_response, data);
}

static int
dlgmessage(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *mes, *title;
  int locksave;
  struct dialog_data data;

  memset(&data, 0, sizeof(data));

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  mes = (char *)argv[2];
  locksave = Globallock;
  Globallock = TRUE;
  dialog_run(title ? title : _("Message"), mes, dlgmessage_main, &data);
  Globallock = locksave;

  return 0;
}

static struct narray *
dlg_get_buttons(struct objlist *obj, N_VALUE *inst)
{
  struct narray *sarray;
  int num;

  if (_getobj(obj, "buttons", inst, &sarray)) {
    return NULL;
  }

  if (sarray == NULL) {
    return NULL;
  }

  num = arraynum(sarray);
  if (num < 1) {
    return NULL;
  }

  return sarray;
}

static void
dlginput_response(int response, const char *str, gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  dlg_response(response, data);
  data->response_text = g_strdup(str);
}

static void
dlginput_main(gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  input_dialog(get_toplevel_window(), data->title, data->msg, data->initial_text, _("_OK"), data->buttons, data->button, dlginput_response, data);
}

static int
dlginput(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *mes, *title, *init_str;
  int locksave, r;
  char *inputbuf;
  struct narray *buttons;
  int btn = -1;
  struct dialog_data data;

  memset(&data, 0, sizeof(data));

  locksave = Globallock;
  Globallock = TRUE;
  init_str = (char *)argv[2];
  g_free(rval->str);
  rval->str = NULL;
  inputbuf = NULL;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  if (_getobj(obj, "caption", inst, &mes)) {
    mes = NULL;
  }

  buttons = dlg_get_buttons(obj, inst);

  data.buttons = buttons;
  data.button = &btn;
  data.initial_text = init_str;
  r = dialog_run(title ? title : _("Confirm"), mes, dlginput_main, &data);
  inputbuf = data.response_text;
  _putobj(obj, "response_button", inst, &btn);
  if (r == IDOK && inputbuf != NULL) {
    rval->str = inputbuf;
  } else {
    g_free(inputbuf);
    Globallock = locksave;
    return 1;
  }
  Globallock = locksave;
  return 0;
}

struct narray *
get_sarray_argument(struct narray *sarray)
{
  int m, n, i, j, id;
  char *ptr, sa[] = "sarray:", *argv[2];
  struct narray iarray;
  struct objlist *saobj;

  n = arraynum(sarray);
  if (n != 1)
    return sarray;

  ptr = * (char **) arraydata(sarray);
  if (ptr == NULL)
    return sarray;

  if (strncmp(ptr, sa, sizeof(sa) / sizeof(*sa) - 1))
    return sarray;

  arrayinit(&iarray, sizeof(int));
  if (getobjilist(ptr, &saobj, &iarray, FALSE, NULL))
    return sarray;

  n = arraynum(&iarray);
  for (j = 0; j < n; j++) {
    id = arraynget_int(&iarray, j);
    if (getobj(saobj, "num", id, 0, NULL, &m) == -1)
      continue;

    if (m < 1)
      continue;

    for (i = 0; i < m; i++) {
      argv[0] = (char *) & i;
      argv[1] = NULL;
      getobj(saobj, "get", id, 1, argv, &ptr);

      if (arrayadd2(sarray, ptr) == NULL)
	goto End;
    }
  }

 End:

  if (arraynum(sarray) > 1) {
    arrayndel2(sarray, 0);
  }

  arraydel(&iarray);

  return sarray;
}

static void
dlgbutton_main(gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  button_dialog(get_toplevel_window(), data->title, data->msg, data->buttons, dlg_response, data);
}

static int
dlgbutton(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *title, *caption;
  int rcode;
  struct narray *sarray;
  struct dialog_data data;

  memset(&data, 0, sizeof(data));

  sarray = get_sarray_argument((struct narray *) argv[2]);
  if (arraynum(sarray) == 0) {
    return -1;
  }

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  if (_getobj(obj, "caption", inst, &caption)) {
    caption = NULL;
  }


  g_free(rval->str);
  rval->str = NULL;

  data.buttons = sarray;
  rcode = dialog_run(title ? title : _("Select"), caption, dlgbutton_main, &data);

  if (rcode > 0) {
    const char *str;
    str = arraynget_str(sarray, rcode - 1);
    if (str) {
      rval->str = g_strdup(str);
    }
  }

  return 0;
}

static void
dlgradio_main(gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  radio_dialog(get_toplevel_window(), data->title, data->msg, data->sarray, _("_OK"), data->buttons, data->button, data->selected, dlg_response, data);
}


static int
dlgradio(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *title, *caption;
  int locksave, r, ret;
  struct narray *iarray, *sarray;
  struct narray *buttons;
  int btn = -1;
  struct dialog_data data;

  memset(&data, 0, sizeof(data));

  sarray = get_sarray_argument((struct narray *) argv[2]);
  if (arraynum(sarray) == 0)
    return 1;

  locksave = Globallock;
  Globallock = TRUE;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  if (_getobj(obj, "caption", inst, &caption)) {
    caption = NULL;
  }

  if (_getobj(obj, "select", inst, &iarray)) {
    iarray = NULL;
  }

  r = arraylast_int(iarray);

  buttons = dlg_get_buttons(obj, inst);
  data.buttons = buttons;
  data.button = &btn;
  data.selected = r;
  data.sarray = sarray;
  r = dialog_run(title ? title : _("Select"), caption, dlgradio_main, &data);
  ret = (r < 0) ? IDCANCEL : IDOK;
  _putobj(obj, "response_button", inst, &btn);
  if (ret != IDOK) {
    Globallock = locksave;
    return 1;
  }

  rval->i = r;

  Globallock = locksave;
  return 0;
}

static void
dlgcombo_response(int response, const char *str, gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  dlg_response(response, data);
  data->response_text = g_strdup(str);
}

static void
dlgcombo_main(gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  combo_dialog(get_toplevel_window(), data->title, data->msg, data->sarray, data->buttons, data->button, data->selected, dlgcombo_response, data);
}

static void
dlgcombo_entry_main(gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  combo_entry_dialog(get_toplevel_window(), data->title, data->msg, data->sarray, data->buttons, data->button, data->selected, dlgcombo_response, data);
}

static int
dlgcombo(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int locksave, sel, ret;
  char *r, *title, *caption;
  struct narray *iarray, *sarray;
  struct narray *buttons;
  int btn = -1;
  struct dialog_data data;

  memset(&data, 0, sizeof(data));

  sarray = get_sarray_argument((struct narray *) argv[2]);
  if (arraynum(sarray) == 0)
    return 1;

  locksave = Globallock;
  Globallock = TRUE;

  g_free(rval->str);
  rval->str = NULL;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  if (_getobj(obj, "caption", inst, &caption)) {
    caption = NULL;
  }

  if (_getobj(obj, "select", inst, &iarray)) {
    iarray = NULL;
  }

  sel = arraylast_int(iarray);
  buttons = dlg_get_buttons(obj, inst);
  data.buttons = buttons;
  data.button = &btn;
  data.selected = sel;
  data.sarray = sarray;
  if (strcmp(argv[1], "combo") == 0) {
    ret = dialog_run(title ? title : _("Select"), caption, dlgcombo_main, &data);
  } else {
    ret = dialog_run(title ? title : _("Select"), caption, dlgcombo_entry_main, &data);
  }
  r = data.response_text;
  _putobj(obj, "response_button", inst, &btn);
  if (ret != IDOK) {
    Globallock = locksave;
    return 1;
  }

  rval->str = r;

  Globallock = locksave;
  return 0;
}

static void
dlgspin_main(gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  spin_dialog(get_toplevel_window(), data->title, data->msg, data->min, data->max, data->inc, data->buttons, data->button, data->val, dlg_response, data);
}

static int
dlgspin(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int locksave, ret, type;
  char *title, *caption;
  double min, max, inc, r;
  struct narray *buttons;
  int btn = -1;
  struct dialog_data data;

  memset(&data, 0, sizeof(data));

  locksave = Globallock;
  Globallock = TRUE;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  if (_getobj(obj, "caption", inst, &caption)) {
    caption = NULL;
  }

  type = argv[1][0];
  switch (type) {
  case 'd':
    min = arg_to_double(argv, 2);
    max = arg_to_double(argv, 3);
    inc = arg_to_double(argv, 4);
    r   = arg_to_double(argv, 5);
    break;
  case 'i':
    min = * (int *) argv[2];
    max = * (int *) argv[3];
    inc = * (int *) argv[4];
    r = * (int *) argv[5];
    break;
  default:
    Globallock = locksave;
    return 1;
  }

  buttons = dlg_get_buttons(obj, inst);
  data.buttons = buttons;
  data.button = &btn;
  data.val = &r;
  data.min = min;
  data.max = max;
  data.inc = inc;
  ret = dialog_run(title ? title : _("Input"), caption, dlgspin_main, &data);
  _putobj(obj, "response_button", inst, &btn);
  if (ret != IDOK) {
    Globallock = locksave;
    return 1;
  }

  switch (type) {
  case 'd':
    rval->d = r;
    break;
  case 'i':
    rval->i = nround(r);
    break;
  default:
    Globallock = locksave;
    return 1;
  }
  Globallock = locksave;

  return 0;
}

static void
dlgcheck_main(gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  check_dialog(get_toplevel_window(), data->title, data->msg, data->sarray, data->buttons, data->button, data->ival, dlg_response, data);
}

static int
dlgcheck(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int locksave, *r, i, n, inum, ret;
  struct narray *array, *sarray, *iarray;
  char *title, *caption;
  struct narray *buttons;
  int btn = -1;
  struct dialog_data data;

  memset(&data, 0, sizeof(data));

  sarray = get_sarray_argument((struct narray *) argv[2]);
  n = arraynum(sarray);
  if (n == 0)
    return 1;

  arrayfree(rval->array);
  rval->array = NULL;

  array = arraynew(sizeof(int));
  if (array == NULL) {
    return 1;
  }

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  if (_getobj(obj, "caption", inst, &caption)) {
    caption = NULL;
  }

  if (_getobj(obj, "select", inst, &iarray)) {
    iarray = NULL;
  }

  r = g_malloc(n * sizeof(int));
  if (r == NULL) {
    arrayfree(array);
    return 1;
  }
  memset(r, 0, n * sizeof(int));

  locksave = Globallock;
  Globallock = TRUE;

  inum = arraynum(iarray);
  for (i = 0; i < inum; i++) {
    const int *ptr;
    ptr = (int *) arraynget(iarray, i);
    if (ptr && *ptr >= 0 && *ptr < n)
      r[*ptr] = 1;
  }

  buttons = dlg_get_buttons(obj, inst);
  data.buttons = buttons;
  data.button = &btn;
  data.sarray = sarray;
  data.ival = r;
  ret = dialog_run(title ? title : _("Select"), caption, dlgcheck_main, &data);
  _putobj(obj, "response_button", inst, &btn);
  if (ret != IDOK) {
    arrayfree(array);
    g_free(r);
    Globallock = locksave;
    return 1;
  }

  for (i = 0; i < n; i++) {
    if (r[i])
      arrayadd(array, &i);
  }

  g_free(r);

  rval->array = array;

  Globallock = locksave;
  return 0;
}

static int
dlgbeep(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int locksave;

  locksave = Globallock;
  Globallock = TRUE;
  message_beep(get_toplevel_window());
  Globallock = locksave;

  return 0;
}

static void
dlggetopenfile_response(char *file, gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  dlg_response((file) ? IDOK : IDCANCEL, data);
  data->response_text = file;
}

static void
dlggetopenfile_main(gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  nGetOpenFileName(get_toplevel_window(), data->title, data->defext, NULL, data->initial_text, FALSE, dlggetopenfile_response, data);
}

static int
dlggetopenfile(struct objlist *obj, N_VALUE *inst, N_VALUE *rval,
	       int argc, char **argv)
{
  struct narray *array;
  char **d;
  int anum;
  char *filter = NULL, *initfile = NULL;
  int locksave;
  int ret;
  char *file;
  struct dialog_data data;

  memset(&data, 0, sizeof(data));

  locksave = Globallock;
  Globallock = TRUE;
  g_free(rval->str);
  rval->str = NULL;
  array = (struct narray *)argv[2];
  d = arraydata(array);
  anum = arraynum(array);
  switch (anum) {
  case 0:
    break;
  case 2:
    initfile = d[1];
    /* fall through */
  case 1:
    filter = d[0];
    break;
  default:
    filter = d[0];
    initfile = d[1];
  }
  data.initial_text = initfile;
  data.defext = filter;
  ret = dialog_run(_("Open file"), NULL, dlggetopenfile_main, &data);
  file = data.response_text;
  if (file) {
    changefilename(file);
    rval->str = file;
  }

  Globallock = locksave;
  return (ret == IDOK)? 0 : 1;
}

static void
dlggetopenfiles_response(char **files, gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  dlg_response((files) ? IDOK : IDCANCEL, data);
  data->files = files;
}

static void
dlggetopenfiles_main(gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  nGetOpenFileNameMulti(get_toplevel_window(), data->title, data->defext, NULL, data->initial_text, FALSE, dlggetopenfiles_response, data);
}

static int
dlggetopenfiles(struct objlist *obj, N_VALUE *inst, N_VALUE *rval,
		int argc, char **argv)
{
  struct narray *array;
  char **d;
  int anum;
  char *filter = NULL, *initfile = NULL;
  int locksave;
  int ret;
  char **file = NULL;
  struct narray *farray;
  struct dialog_data data;

  memset(&data, 0, sizeof(data));

  locksave = Globallock;
  Globallock = TRUE;
  arrayfree2(rval->array);
  rval->array = NULL;
  array = (struct narray *)argv[2];
  d = arraydata(array);
  anum = arraynum(array);
  switch (anum) {
  case 0:
    break;
  case 2:
    initfile = d[1];
    /* fall through */
  case 1:
    filter = d[0];
    break;
  default:
    filter = d[0];
    initfile = d[1];
  }
  data.initial_text = initfile;
  data.defext = filter;
  ret = dialog_run(_("Open file"), NULL, dlggetopenfiles_main, &data);
  file = data.files;
  if (file) {
    int i;
    farray = arraynew(sizeof(char *));
    for (i = 0; file[i]; i++) {
      changefilename(file[i]);
      arrayadd(farray, file + i);
    }
    rval->array = farray;
    g_free(file);
  }

  Globallock = locksave;

  return (ret == IDOK)? 0 : 1;
}

static void
dlggetsavefile_main(gpointer user_data)
{
  struct dialog_data *data;
  data = (struct dialog_data *) user_data;
  nGetSaveFileName(get_toplevel_window(), data->title, data->defext, NULL, data->initial_text, FALSE, dlggetopenfile_response, data);
}

static int
dlggetsavefile(struct objlist *obj, N_VALUE *inst, N_VALUE *rval,
	       int argc, char **argv)
{
  struct narray *array;
  char **d;
  int anum;
  char *filter = NULL, *initfile = NULL;
  int locksave;
  int ret;
  char *file;
  struct dialog_data data;

  memset(&data, 0, sizeof(data));

  locksave = Globallock;
  Globallock = TRUE;
  g_free(rval->str);
  rval->str = NULL;
  array = (struct narray *)argv[2];
  d = arraydata(array);
  anum = arraynum(array);
  switch (anum) {
  case 0:
    break;
  case 2:
    initfile = d[1];
    /* fall through */
  case 1:
    filter = d[0];
    break;
  default:
    filter = d[0];
    initfile = d[1];
  }
  data.initial_text = initfile;
  data.defext = filter;
  ret = dialog_run(_("Save file"), NULL, dlggetsavefile_main, &data);
  file = data.response_text;
  if (file) {
    changefilename(file);
    rval->str = file;
  }

  Globallock = locksave;
  return (ret == IDOK) ? 0 : 1;
}

static struct objtable dialog[] = {
  {"init", NVFUNC, NEXEC, dlginit, NULL, 0},
  {"done", NVFUNC, NEXEC, dlgdone, NULL, 0},
  {"next", NPOINTER, 0, NULL, NULL, 0},
  {"title", NSTR, NREAD | NWRITE, NULL, NULL, 0},
  {"caption", NSTR, NREAD | NWRITE, NULL, NULL, 0},
  {"select", NIARRAY, NREAD | NWRITE, NULL, NULL, 0},
  {"buttons", NSARRAY, NREAD | NWRITE, NULL, NULL, 0},
  {"response_button", NINT, NREAD, NULL, NULL, 0},
  {"yesno", NIFUNC, NREAD | NEXEC, dlgconfirm, "s", 0},
  {"message", NVFUNC, NREAD | NEXEC, dlgmessage, "s", 0},
  {"input", NSFUNC, NREAD | NEXEC, dlginput, "s", 0},
  {"radio", NIFUNC, NREAD | NEXEC, dlgradio, "sa", 0},
  {"check", NIAFUNC, NREAD | NEXEC, dlgcheck, "sa", 0},
  {"button", NSFUNC, NREAD | NEXEC, dlgbutton, "sa", 0},
  {"combo", NSFUNC, NREAD | NEXEC, dlgcombo, "sa", 0},
  {"combo_entry", NSFUNC, NREAD | NEXEC, dlgcombo, "sa", 0},
  {"double_entry", NDFUNC, NREAD | NEXEC, dlgspin, "dddd", 0},
  {"integer_entry", NIFUNC, NREAD | NEXEC, dlgspin, "iiii", 0},
  {"beep", NVFUNC, NREAD | NEXEC, dlgbeep, "", 0},
  {"get_open_file", NSFUNC, NREAD | NEXEC, dlggetopenfile, "sa", 0},
  {"get_open_files", NSAFUNC, NREAD | NEXEC, dlggetopenfiles, "sa", 0},
  {"get_save_file", NSFUNC, NREAD | NEXEC, dlggetsavefile, "sa", 0},
};

#define TBLNUM (sizeof(dialog) / sizeof(*dialog))

void *
adddialog()
{
  return addobject(NAME, NULL, PARENT, NVERSION, TBLNUM, dialog, ERRNUM,
		   dlgerrorlist, NULL, NULL);
}

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

static GtkWidget *
get_toplevel_window(void)
{
  if (TopLevel) {
    return TopLevel;
  }

  if (DLGTopLevel == NULL) {
    DLGTopLevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  }

  return DLGTopLevel;
}

static int
dlginit(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int pos = -1;

 if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  if (!OpenApplication()) {
    error(obj, ERRDISPLAY);
    return 1;
  }

  _putobj(obj, "x", inst, &pos);
  _putobj(obj, "y", inst, &pos);

  return 0;
}


static int
dlgdone(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;
  return 0;
}

static int
dlgconfirm(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *mes, *title;
  int rcode, locksave;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  mes = (char *)argv[2];
  locksave = Globallock;
  Globallock = TRUE;
  mes = CHK_STR(mes);
  rcode = message_box(get_toplevel_window(), mes, (title) ? title : _("Confirm"), RESPONS_YESNO);
  Globallock = locksave;
  if (rcode == IDYES) {
    rval->i = 1;
  } else {
    rval->i = 0;
  }
  return (rcode == IDYES)? 0 : 1;
}

static int
dlgmessage(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *mes, *title;
  int locksave;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  mes = (char *)argv[2];
  locksave = Globallock;
  Globallock = TRUE;
  message_box(get_toplevel_window(), CHK_STR(mes), (title) ? title : _("Message"), RESPONS_OK);
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

static int
dlginput(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *mes, *title, *init_str;
  int locksave, x, y, r;
  char *inputbuf;
  struct narray *buttons;
  int btn = -1;

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

  if (_getobj(obj, "x", inst, &x)) {
    x = -1;
  }

  if (_getobj(obj, "y", inst, &y)) {
    y = -1;
  }

  buttons = dlg_get_buttons(obj, inst);

  r = DialogInput(get_toplevel_window(), (title) ? title : _("Input"), mes, init_str, buttons, &btn, &inputbuf, &x, &y);
  _putobj(obj, "x", inst, &x);
  _putobj(obj, "y", inst, &y);
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

static int
dlgbutton(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *title, *caption;
  int rcode, x, y;
  struct narray *sarray;

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

  if (_getobj(obj, "x", inst, &x)) {
    x = -1;
  }

  if (_getobj(obj, "y", inst, &y)) {
    y = -1;
  }

  g_free(rval->str);
  rval->str = NULL;

  rcode = DialogButton(get_toplevel_window(), (title) ? title : _("Select"), caption, sarray, &x, &y);

  _putobj(obj, "x", inst, &x);
  _putobj(obj, "y", inst, &y);

  if (rcode > 0) {
    const char *str;
    str = arraynget_str(sarray, rcode - 1);
    if (str) {
      rval->str = g_strdup(str);
    }
  }

  return 0;
}

static int
dlgradio(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *title, *caption;
  int locksave, r, x, y, ret;
  struct narray *iarray, *sarray;
  struct narray *buttons;
  int btn = -1;

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

  if (_getobj(obj, "x", inst, &x)) {
    x = -1;
  }

  if (_getobj(obj, "y", inst, &y)) {
    y = -1;
  }

  r = arraylast_int(iarray);

  buttons = dlg_get_buttons(obj, inst);
  ret = DialogRadio(get_toplevel_window(), (title) ? title : _("Select"), caption, sarray, buttons, &btn, &r, &x, &y);
  _putobj(obj, "x", inst, &x);
  _putobj(obj, "y", inst, &y);
  _putobj(obj, "response_button", inst, &btn);
  if (ret != IDOK) {
    Globallock = locksave;
    return 1;
  }

  rval->i = r;

  Globallock = locksave;
  return 0;
}

static int
dlgcombo(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int locksave, sel, ret, x, y;
  char *r, *title, *caption;
  struct narray *iarray, *sarray;
  struct narray *buttons;
  int btn = -1;

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

  if (_getobj(obj, "x", inst, &x)) {
    x = -1;
  }

  if (_getobj(obj, "y", inst, &y)) {
    y = -1;
  }

  sel = arraylast_int(iarray);
  buttons = dlg_get_buttons(obj, inst);
  if (strcmp(argv[1], "combo") == 0) {
    ret = DialogCombo(get_toplevel_window(), (title) ? title : _("Select"), caption, sarray, buttons, &btn, sel, &r, &x, &y);
  } else {
    ret = DialogComboEntry(get_toplevel_window(), (title) ? title : _("Input"), caption, sarray, buttons, &btn, sel, &r, &x, &y);
  }

  _putobj(obj, "x", inst, &x);
  _putobj(obj, "y", inst, &y);
  _putobj(obj, "response_button", inst, &btn);
  if (ret != IDOK) {
    Globallock = locksave;
    return 1;
  }

  rval->str = r;

  Globallock = locksave;
  return 0;
}

static int
dlgspin(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int locksave, ret, type, x, y;
  char *title, *caption;
  double min, max, inc, r;
  struct narray *buttons;
  int btn = -1;

  locksave = Globallock;
  Globallock = TRUE;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  if (_getobj(obj, "caption", inst, &caption)) {
    caption = NULL;
  }

  if (_getobj(obj, "x", inst, &x)) {
    x = -1;
  }

  if (_getobj(obj, "y", inst, &y)) {
    y = -1;
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
  ret = DialogSpinEntry(get_toplevel_window(), (title) ? title : _("Input"), caption, min, max, inc, buttons, &btn, &r, &x, &y);

  _putobj(obj, "x", inst, &x);
  _putobj(obj, "y", inst, &y);
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

static int
dlgcheck(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int locksave, *r, i, n, inum, x, y, ret;
  struct narray *array, *sarray, *iarray;
  char *title, *caption;
  struct narray *buttons;
  int btn = -1;

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

  if (_getobj(obj, "x", inst, &x)) {
    x = -1;
  }

  if (_getobj(obj, "y", inst, &y)) {
    y = -1;
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
    int *ptr;
    ptr = (int *) arraynget(iarray, i);
    if (ptr && *ptr >= 0 && *ptr < n)
      r[*ptr] = 1;
  }

  buttons = dlg_get_buttons(obj, inst);
  ret = DialogCheck(get_toplevel_window(), (title) ? title : _("Select"), caption, sarray, buttons, &btn, r, &x, &y);
  _putobj(obj, "x", inst, &x);
  _putobj(obj, "y", inst, &y);
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
  ret = nGetOpenFileName(get_toplevel_window(), _("Open file"),
			 filter, NULL, initfile,
			 &file, TRUE, TRUE);
  if (ret == IDOK) {
    if (file) {
      changefilename(file);
      rval->str = file;
    }
  }

  Globallock = locksave;
  return (ret == IDOK)? 0 : 1;
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
  ret = nGetOpenFileNameMulti(get_toplevel_window(), _("Open files"),
			      filter, NULL, initfile, &file, TRUE);
  if (ret == IDOK) {
    int i;
    farray = arraynew(sizeof(char *));
    for (i = 0; file[i]; i++) {
      changefilename(file[i]);
      arrayadd(farray, file + i);
    }
    rval->array = farray;
  }
  g_free(file);

  Globallock = locksave;

  return (ret == IDOK)? 0 : 1;
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
  ret = nGetSaveFileName(get_toplevel_window(), _("Save file"),
			 filter, NULL, initfile,
			 &file, FALSE, TRUE);
  if (ret == IDOK) {
    if (file) {
      changefilename(file);
      rval->str = file;
    }
  }

  Globallock = locksave;
  return (ret == IDOK) ? 0 : 1;
}

static struct objtable dialog[] = {
  {"init", NVFUNC, NEXEC, dlginit, NULL, 0},
  {"done", NVFUNC, NEXEC, dlgdone, NULL, 0},
  {"next", NPOINTER, 0, NULL, NULL, 0},
  {"x", NINT, NREAD | NWRITE, NULL, NULL, 0},
  {"y", NINT, NREAD | NWRITE, NULL, NULL, 0},
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

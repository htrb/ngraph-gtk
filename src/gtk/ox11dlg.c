/* 
 * $Id: ox11dlg.c,v 1.28 2009/11/16 09:13:05 hito Exp $
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

#include "ngraph.h"
#include "object.h"
#include "nstring.h"
#include "jnstring.h"
#include "ioutil.h"
#include "ox11menu.h"

#include "main.h"
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

GtkWidget *DLGTopLevel = NULL;

static int
dlginit(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int pos = -1;

 if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  if (!OpenApplication()) {
    error(obj, ERRDISPLAY);
    return 1;
  }

  if (DLGTopLevel == NULL) {
    DLGTopLevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  }

  _putobj(obj, "x", inst, &pos);
  _putobj(obj, "y", inst, &pos);

  return 0;
}


static int
dlgdone(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;
  return 0;
}

static int
dlgconfirm(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char *mes, *title;
  int rcode, locksave;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  mes = (char *)argv[2];
  locksave = GlobalLock;
  GlobalLock = TRUE;
  mes = CHK_STR(mes);
  rcode = MessageBox(DLGTopLevel, mes, (title) ? title : "Select", MB_YESNO);
  GlobalLock = locksave;
  if (rcode == IDYES) {
    *(int *)rval = 1;
  } else {
    *(int *)rval = 0;
  }
  return (rcode == IDYES)? 0 : 1;
}

static int
dlgmessage(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char *mes, *title;
  int rcode, locksave;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  mes = (char *)argv[2];
  locksave = GlobalLock;
  GlobalLock = TRUE;
  rcode = MessageBox(DLGTopLevel, CHK_STR(mes), (title) ? title : "Confirm", MB_OK);
  GlobalLock = locksave;

  return 0;
}

static int
dlginput(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char *mes, *title;
  int locksave, x, y, r;
  char *inputbuf;

  locksave = GlobalLock;
  GlobalLock = TRUE;
  mes = (char *)argv[2];
  g_free(*(char **)rval);
  *(char **)rval = NULL;
  inputbuf = NULL;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  if (mes == NULL) {
    _getobj(obj, "caption", inst, &mes);
  }

  if (_getobj(obj, "x", inst, &x)) {
    x = -1;
  }

  if (_getobj(obj, "y", inst, &y)) {
    y = -1;
  }

  r = DialogInput(DLGTopLevel, (title) ? title : "Input", mes, &inputbuf, &x, &y);
  _putobj(obj, "x", inst, &x);
  _putobj(obj, "y", inst, &y);
  if (r == IDOK && inputbuf != NULL) {
    *(char **)rval = inputbuf;
  } else {
    g_free(inputbuf);
    GlobalLock = locksave;
    return 1;
  }
  GlobalLock = locksave;
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
    id = * (int *) arraynget(&iarray, j);
    if (getobj(saobj, "num", id, 0, NULL, &m) == -1)
      continue;

    if (m < 1)
      continue;

    for (i = 0; i < m; i++) {
      argv[0] = (char *) & i;
      argv[1] = NULL;
      getobj(saobj, "get", id, 1, argv, &ptr);

      if (arrayadd2(sarray, &ptr) == NULL)
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
dlgradio(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char *title, *caption;
  int locksave, r, *ptr, x, y, ret;
  struct narray *iarray, *sarray;

  sarray = get_sarray_argument((struct narray *) argv[2]);
  if (arraynum(sarray) == 0)
    return 1;

  locksave = GlobalLock;
  GlobalLock = TRUE;

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

  ptr = (int *) arraylast(iarray);
  if (ptr) {
    r = *ptr;
  } else {
    r = -1;
  }

  ret = DialogRadio(DLGTopLevel, (title) ? title : "Select", caption, sarray, &r, &x, &y);
  _putobj(obj, "x", inst, &x);
  _putobj(obj, "y", inst, &y);
  if (ret != IDOK) {
    GlobalLock = locksave;
    return 1;
  }

  *(int *) rval = r;

  GlobalLock = locksave;
  return 0;
}

static int
dlgcombo(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int locksave, *ptr, sel, ret, x, y;
  char *r, *title, *caption;
  struct narray *iarray, *sarray;

  sarray = get_sarray_argument((struct narray *) argv[2]);
  if (arraynum(sarray) == 0)
    return 1;

  locksave = GlobalLock;
  GlobalLock = TRUE;

  g_free(*(char **)rval);
  *(char **)rval = NULL;

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

  ptr = (int *) arraylast(iarray);
  if (ptr) {
    sel = *ptr;
  } else {
    sel = -1;
  }

  if (strcmp(argv[1], "combo") == 0) {
    ret = DialogCombo(DLGTopLevel, (title) ? title : "Select", caption, sarray, sel, &r, &x, &y);
  } else {
    ret = DialogComboEntry(DLGTopLevel, (title) ? title : "Input", caption, sarray, sel, &r, &x, &y);
  }

  _putobj(obj, "x", inst, &x);
  _putobj(obj, "y", inst, &y);
  if (ret != IDOK) {
    GlobalLock = locksave;
    return 1;
  }

  *(char **)rval = r;

  GlobalLock = locksave;
  return 0;
}

static int
dlgspin(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int locksave, ret, type, x, y;
  char *title, *caption;
  double min, max, inc, r;

  locksave = GlobalLock;
  GlobalLock = TRUE;

  g_free(*(char **)rval);
  *(char **)rval = NULL;

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
    min = * (double *) argv[2];
    max = * (double *) argv[3];
    inc = * (double *) argv[4];
    r = * (double *) argv[5];
    break;
  case'i':
    min = * (int *) argv[2];
    max = * (int *) argv[3];
    inc = * (int *) argv[4];
    r = * (int *) argv[5];
    break;
  default:
    GlobalLock = locksave;
    return 1;
  }

  ret = DialogSpinEntry(DLGTopLevel, (title) ? title : "Input", caption, min, max, inc, &r, &x, &y);

  _putobj(obj, "x", inst, &x);
  _putobj(obj, "y", inst, &y);
  if (ret != IDOK) {
    GlobalLock = locksave;
    return 1;
  }

  switch (type) {
  case 'f':
    *(char **)rval = g_strdup_printf("%.15e", r);
    break;
  case'i':
    *(char **)rval = g_strdup_printf("%d", (int) r);
    break;
  default:
    GlobalLock = locksave;
    return 1;
  }

  GlobalLock = locksave;

  return 0;
}

static int
dlgcheck(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int locksave, *r, i, n, inum, *ptr, x, y, ret;
  struct narray *array, *sarray, *iarray;
  char *title, *caption;

  sarray = get_sarray_argument((struct narray *) argv[2]);
  n = arraynum(sarray);
  if (n == 0)
    return 1;

  arrayfree(* (struct narray **) rval);
  *(char **) rval = NULL;

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

  locksave = GlobalLock;
  GlobalLock = TRUE;

  inum = arraynum(iarray);
  for (i = 0; i < inum; i++) {
    ptr = (int *) arraynget(iarray, i);
    if (ptr && *ptr >= 0 && *ptr < n)
      r[*ptr] = 1;
  }

  ret = DialogCheck(DLGTopLevel, (title) ? title : "Check", caption, sarray, r, &x, &y);
  _putobj(obj, "x", inst, &x);
  _putobj(obj, "y", inst, &y);
  if (ret != IDOK) {
    arrayfree(array);
    g_free(r);
    GlobalLock = locksave;
    return 1;
  }

  for (i = 0; i < n; i++) {
    if (r[i])
      arrayadd(array, &i);
  }

  g_free(r);

  *(struct narray **) rval = array;

  GlobalLock = locksave;
  return 0;
}

static int
dlgbeep(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int locksave;

  locksave = GlobalLock;
  GlobalLock = TRUE;
  MessageBeep(DLGTopLevel);
  GlobalLock = locksave;

  return 0;
}

static int
dlggetopenfile(struct objlist *obj, char *inst, char *rval,
	       int argc, char **argv)
{
  struct narray *array;
  char **d;
  int anum;
  char *filter = NULL, *initfile = NULL;
  int locksave;
  int ret;
  char *file;

  locksave = GlobalLock;
  GlobalLock = TRUE;
  g_free(*(char **)rval);
  *(char **)rval = NULL;
  array = (struct narray *)argv[2];
  d = arraydata(array);
  anum = arraynum(array);
  switch (anum) {
  case 0:
    break;
  case 2:
    initfile = d[1];
  case 1:
    filter = d[0];
    break;
  default:
    filter = d[0];
    initfile = d[1];
  }
  ret = nGetOpenFileName(DLGTopLevel, _("Open file"),
			 filter, NULL, initfile,
			 &file, TRUE, TRUE);
  if (ret == IDOK) {
    if (file) {
      changefilename(file);
      *(char **)rval = file;
    }
  }

  GlobalLock = locksave;
  return (ret == IDOK)? 0 : 1;
}

static int
dlggetopenfiles(struct objlist *obj, char *inst, char *rval,
		int argc, char **argv)
{
  struct narray *array;
  char **d;
  int anum, i;
  char *filter = NULL, *initfile = NULL;
  int locksave;
  int ret;
  char **file = NULL, *name;
  struct narray *farray;

  locksave = GlobalLock;
  GlobalLock = TRUE;

  arrayfree2(*(struct narray **)rval);
  *(char **)rval = NULL;

  array = (struct narray *)argv[2];
  d = arraydata(array);
  anum = arraynum(array);
  switch (anum) {
  case 0:
    break;
  case 2:
    initfile = d[1];
  case 1:
    filter = d[0];
    break;
  default:
    filter = d[0];
    initfile = d[1];
  }
  ret = nGetOpenFileNameMulti(DLGTopLevel, _("Open files"),
			      filter, NULL, initfile, &file, TRUE);
  if (ret == IDOK) {
    farray = arraynew(sizeof(char *));
    for (i = 0; file[i]; i++) {
      changefilename(file[i]);
      arrayadd(farray, &name);
    }
    *(struct narray **)rval = farray;
  }
  g_free(file);

  GlobalLock = locksave;

  return (ret == IDOK)? 0 : 1;
}

static int
dlggetsavefile(struct objlist *obj, char *inst, char *rval,
	       int argc, char **argv)
{
  struct narray *array;
  char **d;
  int anum;
  char *filter = NULL, *initfile = NULL;
  int locksave;
  int ret;
  char *file;

  locksave = GlobalLock;
  GlobalLock = TRUE;
  g_free(*(char **)rval);
  *(char **)rval = NULL;
  array = (struct narray *)argv[2];
  d = arraydata(array);
  anum = arraynum(array);
  switch (anum) {
  case 0:
    break;
  case 2:
    initfile = d[1];
  case 1:
    filter = d[0];
    break;
  default:
    filter = d[0];
    initfile = d[1];
  }
  ret = nGetSaveFileName(DLGTopLevel, _("Save file"),
			 filter, NULL, initfile,
			 &file, FALSE, TRUE);
  if (ret == IDOK) {
    if (file) {
      changefilename(file);
      *(char **)rval = file;
    }
  }

  GlobalLock = locksave;
  return (ret == IDOK)?0:1;
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
  {"yesno", NIFUNC, NREAD | NEXEC, dlgconfirm, "s", 0},
  {"message", NVFUNC, NREAD | NEXEC, dlgmessage, "s", 0},
  {"input", NSFUNC, NREAD | NEXEC, dlginput, "s", 0},
  {"radio", NIFUNC, NREAD | NEXEC, dlgradio, "sa", 0},
  {"check", NIAFUNC, NREAD | NEXEC, dlgcheck, "sa", 0},
  {"combo", NSFUNC, NREAD | NEXEC, dlgcombo, "sa", 0},
  {"combo_entry", NSFUNC, NREAD | NEXEC, dlgcombo, "sa", 0},
  {"double_entry", NSFUNC, NREAD | NEXEC, dlgspin, "dddd", 0},
  {"integer_entry", NSFUNC, NREAD | NEXEC, dlgspin, "iiii", 0},
  {"beep", NVFUNC, NREAD | NEXEC, dlgbeep, NULL, 0},
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

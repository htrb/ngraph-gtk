/* 
 * $Id: ox11dlg.c,v 1.11 2009/01/25 09:53:12 hito Exp $
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

char *dlgerrorlist[] = {
  "cannot open display.",
  "no instance for dialog",
};

#define ERRNUM (sizeof(dlgerrorlist) / sizeof(*dlgerrorlist))

GtkWidget *DLGTopLevel = NULL;

static gboolean
dialogclose(GtkWidget *w, GdkEvent  *event, gpointer user_data)
{
  return TRUE;
}

static int
dlginit(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
 if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  if (!OpenApplication()) {
    error(obj, ERRDISPLAY);
    return 1;
  }

  if (DLGTopLevel == NULL) {
    DLGTopLevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    //    g_signal_connect(DLGTopLevel, "delete", dialogclose, NULL);
  }
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
  if (mes == NULL) {
    mes = "";
  }
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
  if (mes == NULL) {
    mes="";
  }
  rcode = MessageBox(DLGTopLevel, mes, (title) ? title : "Confirm", MB_OK);
  GlobalLock = locksave;

  return 0;
}

static int
dlginput(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char *mes, *title;
  int locksave;
  char *inputbuf;
  char *s;

  locksave = GlobalLock;
  GlobalLock = TRUE;
  mes = (char *)argv[2];
  memfree(*(char **)rval);
  *(char **)rval = NULL;
  inputbuf = NULL;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  if (mes == NULL) {
    _getobj(obj, "caption", inst, &mes);
  }

  if (DialogInput(DLGTopLevel, (title) ? title : "Input", mes, &inputbuf) == IDOK &&
      inputbuf != NULL) {
    s = nstrdup(inputbuf);
    free(inputbuf);
    *(char **)rval = s;
  } else {
    free(inputbuf);
    GlobalLock = locksave;
    return 1;
  }
  GlobalLock = locksave;
  return 0;
}

static int
dlgradio(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char *title, *caption;
  int locksave, r;

  locksave = GlobalLock;
  GlobalLock = TRUE;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  if (_getobj(obj, "caption", inst, &caption)) {
    caption = NULL;
  }

  if (DialogRadio(DLGTopLevel, (title) ? title : "Select", caption, (struct narray *)argv[2], &r) != IDOK) {
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
  int locksave;
  char *r, *title, *caption;

  locksave = GlobalLock;
  GlobalLock = TRUE;

  free(*(char **)rval);
  *(char **)rval = NULL;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  if (_getobj(obj, "caption", inst, &caption)) {
    caption = NULL;
  }

  if (DialogCombo(DLGTopLevel, (title) ? title : "Select", caption, (struct narray *)argv[2], &r) != IDOK) {
    GlobalLock = locksave;
    return 1;
  }

  *(char **)rval = r;

  GlobalLock = locksave;
  return 0;
}

static int
dlgcombo_entry(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int locksave;
  char *r, *title, *caption;

  locksave = GlobalLock;
  GlobalLock = TRUE;

  free(*(char **)rval);
  *(char **)rval = NULL;

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  if (_getobj(obj, "caption", inst, &caption)) {
    caption = NULL;
  }

  if (DialogComboEntry(DLGTopLevel, (title) ? title : "Input", caption, (struct narray *)argv[2], &r) != IDOK) {
    GlobalLock = locksave;
    return 1;
  }

  *(char **)rval = r;

  GlobalLock = locksave;
  return 0;
}

static int
dlgcheck(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int locksave, *r, i, n;
  struct narray *array, *sarray;
  char *title, *caption;

  locksave = GlobalLock;
  GlobalLock = TRUE;

  array = *(struct narray **) rval;
  if (arraynum(array) != 0) {
    arraydel(array);
  }

  if (array == NULL && (array = arraynew(sizeof(int))) == NULL) {
    GlobalLock = locksave;
    return 1;
  }

  sarray = (struct narray *)argv[2];
  n = arraynum(sarray);

  if (_getobj(obj, "title", inst, &title)) {
    title = NULL;
  }

  if (_getobj(obj, "caption", inst, &caption)) {
    caption = NULL;
  }

  if (DialogCheck(DLGTopLevel, (title) ? title : "Check", caption, sarray, &r) != IDOK) {
    GlobalLock = locksave;
    return 1;
  }

  for (i = 0; i < n; i++) {
    arrayadd(array, &r[i]);
  }

  memfree(r);

  if (arraynum(array)==0) {
    arrayfree(array);
    GlobalLock = locksave;
    return 1;
  }

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
  char *file, *file2;

  locksave = GlobalLock;
  GlobalLock = TRUE;
  memfree(*(char **)rval);
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
			 NULL, NULL, initfile,
			 &file, filter, TRUE, TRUE);
  if (ret == IDOK) {
    file2 = nstrdup(file);
    if (file2) {
      changefilename(file2);
      *(char **)rval = file2;
    }
  }
  free(file);
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
  memfree(*(struct narray **)rval);
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
			      NULL, NULL, initfile,
			      &file, filter, TRUE);
  if (ret == IDOK) {
    farray = arraynew(sizeof(char *));
    for (i = 0; file[i]; i++) {
      name = nstrdup(file[i]);
      free(file[i]);
      changefilename(name);
      arrayadd(farray, &name);
    }
    *(struct narray **)rval = farray;
  }
  free(file);
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
  char *file, *file2;

  locksave = GlobalLock;
  GlobalLock = TRUE;
  memfree(*(char **)rval);
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
			 NULL, NULL, initfile,
			 &file, filter, TRUE);
  if (ret == IDOK) {
    file2 = nstrdup(file);
    if (file2) {
      changefilename(file2);
      *(char **)rval = file2;
    }
  }
  free(file);
  GlobalLock = locksave;
  return (ret == IDOK)?0:1;
}

struct objtable dialog[] = {
  {"init", NVFUNC, NEXEC, dlginit, NULL, 0},
  {"done", NVFUNC, NEXEC, dlgdone, NULL, 0},
  {"next", NPOINTER, 0, NULL, NULL, 0},
  {"title", NSTR, NREAD | NWRITE, NULL, NULL, 0},
  {"caption", NSTR, NREAD | NWRITE, NULL, NULL, 0},
  {"yesno", NIFUNC, NREAD | NEXEC, dlgconfirm, "s", 0},
  {"message", NVFUNC, NREAD | NEXEC, dlgmessage, "s", 0},
  {"input", NSFUNC, NREAD | NEXEC, dlginput, "s", 0},
  {"radio", NIFUNC, NREAD | NEXEC, dlgradio, "sa", 0},
  {"check", NIAFUNC, NREAD | NEXEC, dlgcheck, "sa", 0},
  {"combo", NSFUNC, NREAD | NEXEC, dlgcombo, "sa", 0},
  {"combo_entry", NSFUNC, NREAD | NEXEC, dlgcombo_entry, "sa", 0},
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

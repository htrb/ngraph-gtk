/* 
 * $Id: ox11dlg.c,v 1.2 2008/08/05 02:45:26 hito Exp $
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

#define ERRNUM 2

char *dlgerrorlist[ERRNUM] = {
  "cannot open display.",
  "no instance for dialog",
};

GtkWidget *DLGTopLevel = NULL;

extern int OpenApplication();
extern GdkDisplay *Disp;
extern int OpenApplication();

gboolean
dialogclose(GtkWidget *w, GdkEvent  *event, gpointer user_data)
{
  return TRUE;
}

int
dlginit(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
 if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  if (!OpenApplication()) {
    return 1;
  }

  if (DLGTopLevel == NULL) {
    DLGTopLevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    //    g_signal_connect(DLGTopLevel, "delete", dialogclose, NULL);
  }
  return 0;
}


int
dlgdone(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;
  return 0;
}

int
dlgconfirm(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char *mes;
  int rcode, locksave;

  mes = (char *)argv[2];
  locksave = GlobalLock;
  GlobalLock = TRUE;
  if (mes == NULL) {
    mes = "";
  }
  rcode = MessageBox(DLGTopLevel, mes, "Select", MB_YESNO);
  GlobalLock = locksave;
  if (rcode == IDYES) {
    *(int *)rval = 1;
  } else {
    *(int *)rval = 0;
  }
  return (rcode == IDYES)? 0 : 1;
}

int
dlgmessage(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char *mes;
  int rcode, locksave;

  mes = (char *)argv[2];
  locksave = GlobalLock;
  GlobalLock = TRUE;
  if (mes == NULL) {
    mes="";
  }
  rcode = MessageBox(DLGTopLevel, mes, "Confirm", MB_OK);
  GlobalLock = locksave;

  return 0;
}

int
dlginput(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char *mes;
  int locksave;
  char *inputbuf;
  char *s;

  locksave = GlobalLock;
  GlobalLock = TRUE;
  mes = (char *)argv[2];
  memfree(*(char **)rval);
  *(char **)rval = NULL;
  inputbuf = NULL;

  if (DialogInput(DLGTopLevel, "Input", mes, &inputbuf) == IDOK &&
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

int
dlgbeep(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int locksave;

  locksave = GlobalLock;
  GlobalLock = TRUE;
  MessageBeep(DLGTopLevel);
  GlobalLock = locksave;

  return 0;
}

int
dlggetopenfile(struct objlist *obj, char *inst, char *rval,
	       int argc, char **argv)
{
  struct narray *array;
  char **d;
  int anum;
  char *filter;
  int locksave;
  int ret;
  char *file, *file2;

  locksave = GlobalLock;
  GlobalLock = TRUE;
  free(*(char **)rval);
  *(char **)rval = NULL;
  array = (struct narray *)argv[2];
  d=arraydata(array);
  anum = arraynum(array)/2;
  if ((anum>0) && (d[1]!=NULL)) {
    filter=d[1];
  } else {
    filter=NULL;
  }
  ret = nGetOpenFileName(DLGTopLevel, _("Open file"),
			 NULL, NULL, NULL,
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

int
dlggetopenfiles(struct objlist *obj, char *inst, char *rval,
		int argc, char **argv)
{
  struct narray *array;
  char **d;
  int anum, i;
  char *filter;
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
  anum = arraynum(array) / 2;
  if ((anum > 0) && (d[1] != NULL)) {
    filter=d[1];
  } else {
    filter=NULL;
  }
  ret = nGetOpenFileNameMulti(DLGTopLevel, _("Open files"),
			      NULL, NULL, NULL,
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

int
dlggetsavefile(struct objlist *obj, char *inst, char *rval,
	       int argc, char **argv)
{
  struct narray *array;
  char **d;
  int anum;
  char *filter;
  int locksave;
  int ret;
  char *file, *file2;

  locksave = GlobalLock;
  GlobalLock=TRUE;
  free(*(char **)rval);
  *(char **)rval = NULL;
  array = (struct narray *)argv[2];
  d = arraydata(array);
  anum = arraynum(array)/2;
  if ((anum>0) && (d[1]!=NULL)) {
    filter = d[1];
  } else {
    filter = NULL;
  }
  ret = nGetSaveFileName(DLGTopLevel, _("Save file"),
			 NULL, NULL, NULL,
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


#define TBLNUM 10

struct objtable dialog[TBLNUM] = {
  {"init", NVFUNC, NEXEC, dlginit, NULL, 0},
  {"done", NVFUNC, NEXEC, dlgdone, NULL, 0},
  {"next", NPOINTER, 0, NULL, NULL, 0},
  {"yesno", NIFUNC, NREAD | NEXEC, dlgconfirm, "s", 0},
  {"message", NVFUNC, NREAD | NEXEC, dlgmessage, "s", 0},
  {"input", NSFUNC, NREAD | NEXEC, dlginput, "s", 0},
  {"beep", NVFUNC, NREAD | NEXEC, dlgbeep, NULL, 0},
  {"get_open_file", NSFUNC, NREAD | NEXEC, dlggetopenfile, "sa", 0},
  {"get_open_files", NSAFUNC, NREAD | NEXEC, dlggetopenfiles, "sa", 0},
  {"get_save_file", NSFUNC, NREAD | NEXEC, dlggetsavefile, "sa", 0},
};

void *
adddialog()
{
  return addobject(NAME, NULL, PARENT, NVERSION, TBLNUM, dialog, ERRNUM,
		   dlgerrorlist, NULL, NULL);
}

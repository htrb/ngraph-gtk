/* 
 * $Id: x11scrip.c,v 1.1 2008/05/29 09:37:33 hito Exp $
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

#include "gtk_liststore.h"

#include "ngraph.h"
#include "object.h"
#include "nstring.h"

#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "ox11menu.h"
#include "x11view.h"
#include "x11scrip.h"
#include "x11commn.h"

static void
ScriptDialogSetupItem(GtkWidget *w, struct ScriptDialog *d)
{
  int i;
  struct script *scur;
  GtkTreeIter iter;

  list_store_clear(d->list);
  scur = Menulocal.scriptroot;

  i = 0;
  while (scur != NULL) {
    list_store_append(d->list, &iter);
    list_store_set_string(d->list, &iter, 0, scur->name);
    scur = scur->next;
    i++;
  }
}

static gboolean
script_list_default_cb(GtkWidget *w, GdkEventAny *e, gpointer user_data)
{
  struct ScriptDialog *d;
  int i;

  d = (struct ScriptDialog *) user_data;

  if (e->type == GDK_2BUTTON_PRESS ||
      (e->type == GDK_KEY_PRESS && ((GdkEventKey *)e)->keyval == GDK_Return)){

    i = list_store_get_selected_index(d->list);
    if (i < 0)
      return FALSE;

    gtk_dialog_response(GTK_DIALOG(d->widget), GTK_RESPONSE_OK);

    return TRUE;
  }

  return FALSE;
}

static void
ScriptDialogSetup(GtkWidget *wi, void *data, int makewidget)
{
  GtkWidget *w, *swin, *hbox;
  struct ScriptDialog *d;
  n_list_store clist[] = {
    {N_("Addin"), G_TYPE_STRING, TRUE, FALSE, NULL},
  };

  d = (struct ScriptDialog *) data;
  if (makewidget) {
    w = list_store_create(sizeof(clist) / sizeof(*clist), clist);
    g_signal_connect(w, "button-press-event", G_CALLBACK(script_list_default_cb), d);
    g_signal_connect(w, "key-press-event", G_CALLBACK(script_list_default_cb), d);

    swin = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(swin), w);

    gtk_box_pack_start(GTK_BOX(d->vbox), swin, TRUE, TRUE, 4);

    d->list = w;

    hbox = gtk_hbox_new(FALSE, 4);
    w = create_text_entry(FALSE, TRUE);
    item_setup(hbox, w, _("_Option:"), TRUE);
    d->entry = w;

    gtk_box_pack_start(GTK_BOX(d->vbox), hbox, FALSE, FALSE, 4);

    gtk_window_set_default_size(GTK_WINDOW(wi), 200, 400);
  }
  ScriptDialogSetupItem(w, d);
}

static void
ScriptDialogClose(GtkWidget *w, void *data)
{
  struct ScriptDialog *d;
  struct script *pcur;
  const char *ptr;
  int i, n;

  d = (struct ScriptDialog *) data;

  if (d->ret != IDOK)
    return;

  d->execscript = NULL;

  n = list_store_get_selected_index(d->list);

  if (n < 0)
    return;

  
  i=0;
  for (pcur = Menulocal.scriptroot; pcur != NULL; pcur = pcur->next) {
    if (i == n && pcur->script) {
      d->execscript = nstrdup(pcur->script);
      break;
    }
    i++;
  }

  ptr = gtk_entry_get_text(GTK_ENTRY(d->entry));
  strncpy(d->option, ptr, 255);
}

void
ScriptDialog(struct ScriptDialog *data)
{
  data->SetupWindow = ScriptDialogSetup;
  data->CloseWindow = ScriptDialogClose;
}

void
CmScriptExec(void)
{
  struct objlist *obj;
  char *name;
  int newid, allocnow = FALSE;
  char *option, *s;
  int len;
  char *argv[2];
  struct narray sarray;
  char mes[256];
  struct objlist *robj, *shell;
  char *inst;
  int idn;

  if (Menulock || GlobalLock)
    return;

  if ((shell = chkobject("shell")) == NULL)
    return;

  ScriptDialog(&DlgScript);

  if ((DialogExecute(TopLevel, &DlgScript) == IDOK)
      && (DlgScript.execscript)) {
    name = DlgScript.execscript;
    newid = newobj(shell);
    if (newid >= 0) {
      arrayinit(&sarray, sizeof(char *));
      if (arrayadd(&sarray, &name) == NULL) {
	memfree(name);
	arraydel2(&sarray);
	return;
      }
      option = DlgScript.option;
      while ((s = getitok2(&option, &len, " \t")) != NULL) {
	if (arrayadd(&sarray, &s) == NULL) {
	  memfree(s);
	  arraydel2(&sarray);
	  return;
	}
      }
      if (Menulocal.addinconsole) {
	allocnow = AllocConsole();
      }
      argv[0] = (char *) &sarray;
      argv[1] = NULL;
      snprintf(mes, sizeof(mes), _("Executing `%.128s'."), name);
      SetStatusBar(mes);

      Menulock = TRUE;
      obj = Menulocal.obj;
      inst = Menulocal.inst;
      idn = getobjtblpos(obj, "_evloop", &robj);
      registerevloop(chkobjectname(obj), "_evloop", robj, idn, inst, NULL);

      exeobj(shell, "shell", newid, 1, argv);

      unregisterevloop(robj, idn, inst);
      Menulock = FALSE;

      ResetStatusBar();
      arraydel2(&sarray);
      if (Menulocal.addinconsole)
	FreeConsole(allocnow);
      GetPageSettingsFromGRA();
      UpdateAll2();
      NgraphApp.Changed = TRUE;
    } else
      memfree(name);
    delobj(shell, newid);
  }
}

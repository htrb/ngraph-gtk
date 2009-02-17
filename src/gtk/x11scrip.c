/* 
 * $Id: x11scrip.c,v 1.10 2009/02/17 08:35:56 hito Exp $
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

#include "gtk_widget.h"
#include "x11gui.h"
#include "x11dialg.h"
#include "x11menu.h"
#include "ox11menu.h"
#include "x11view.h"
#include "x11scrip.h"
#include "x11commn.h"

void
CmScriptExec(GtkWidget *w, gpointer client_data)
{
  char *name;
  int newid, allocnow = FALSE;
  char *option, *s;
  int len;
  char *argv[2];
  struct narray sarray;
  char mes[256];
  struct objlist *robj, *shell;
  int idn;
  struct script *fcur;

  if (Menulock || GlobalLock || client_data == NULL)
    return;

  shell = chkobject("shell");
  if (shell == NULL)
    return;

  fcur = (struct script *) client_data;

  if (fcur->script == NULL)
    return;

  name = nstrdup(fcur->script);
  if (name == NULL) {
    return;
  }

  newid = newobj(shell);
  if (newid < 0) {
    memfree(name);
    return;
  }

  arrayinit(&sarray, sizeof(char *));
  if (arrayadd(&sarray, &name) == NULL) {
    delobj(shell, newid);
    memfree(name);
    arraydel2(&sarray);
    return;
  }

  option = fcur->option;
  while ((s = getitok2(&option, &len, " \t")) != NULL) {
    if (arrayadd(&sarray, &s) == NULL) {
      delobj(shell, newid);
      memfree(s);
      arraydel2(&sarray);
      return;
    }
  }

  if (Menulocal.addinconsole) {
    allocnow = AllocConsole();
  }

  snprintf(mes, sizeof(mes), _("Executing `%.128s'."), name);
  SetStatusBar(mes);

  menu_lock(TRUE);

  idn = getobjtblpos(Menulocal.obj, "_evloop", &robj);
  registerevloop(chkobjectname(Menulocal.obj), "_evloop", robj, idn, Menulocal.inst, NULL);
  argv[0] = (char *) &sarray;
  argv[1] = NULL;
  exeobj(shell, "shell", newid, 1, argv);
  unregisterevloop(robj, idn, Menulocal.inst);

  menu_lock(FALSE);

  ResetStatusBar();
  arraydel2(&sarray);

  if (Menulocal.addinconsole) {
    FreeConsole(allocnow);
  }

  GetPageSettingsFromGRA();
  UpdateAll2();

  delobj(shell, newid);
  gdk_window_invalidate_rect(NgraphApp.Viewer.win, NULL, FALSE);
}

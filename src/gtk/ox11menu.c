/* 
 * $Id: ox11menu.c,v 1.30 2008/09/12 09:12:08 hito Exp $
 * 
 * This file is part of "Ngraph for GTK".
 * 
 * Copyright (C) 2002,  Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 * 
 * "Ngraph for GTK" is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License,  or (at your option) any later version.
 * 
 * "Ngraph for GTK" is distributed in the hope that it will be useful, 
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

#include <X11/Xlib.h>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <math.h>
#include <time.h>
#include <ctype.h>

#include "ngraph.h"
#include "object.h"
#include "ioutil.h"
#include "shell.h"
#include "nstring.h"
#include "jnstring.h"
#include "nconfig.h"
#include "mathfn.h"
#include "gra.h"
#include "spline.h"

#include "strconv.h"

#include "ogra2cairo.h"
#include "ogra2x11.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11gui.h"
#include "x11view.h"
#include "x11graph.h"
#include "x11print.h"

#define NAME "menu"
#define ALIAS "winmenu:gtkmenu"
#define PARENT "gra2cairo"
#define NVERSION  "1.00.00"
#define MGTKCONF "[x11menu]"
#define G2WINCONF "[gra2gtk]"

#define ERRNUM 3

static char *menuerrorlist[ERRNUM] = {
  "already running.",
  "not enough color cell.",
  "cannot create font. Check `windowfont' resource.",
};

extern int OpenApplication();
extern GdkDisplay *Disp;

struct menulocal Menulocal;
struct mxlocal *Mxlocal;
struct savedstdio GtkIOSave;

static int mxflush(struct objlist *obj, char *inst, char *rval, int argc,
		   char **argv);


static int
mgtkloadconfig(void)
{
  FILE *fp;
  char *tok, *str, *s2;
  char *f1, *f2, *f3, *f4, *f5;
  int val;
  char *endptr;
  int len;
  struct extprinter *pcur, *pnew;
  struct prnprinter *pcur2, *pnew2;
  struct script *scur, *snew;

  fp = openconfig(MGTKCONF);
  if (fp == NULL)
    return 0;

  pcur = Menulocal.extprinterroot;
  pcur2 = Menulocal.prnprinterroot;
  scur = Menulocal.scriptroot;
  while ((tok = getconfig(fp, &str))) {
    s2 = str;
    if (strcmp(tok, "script_console") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.scriptconsole = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "addin_console") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.addinconsole = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "preserve_width") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.preserve_width = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "change_directory") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.changedirectory = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "save_history") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.savehistory = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "expand_dir") == 0) {
      f1 = getitok2(&s2, &len, "");
      if (f1) {
	memfree(Menulocal.expanddir);
	Menulocal.expanddir = f1;
      }
    } else if (strcmp(tok, "expand") == 0) {
      f1 = getitok2(&s2, &len, " \x09, ");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.expand = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "ignore_path") == 0) {
      f1 = getitok2(&s2, &len, " \x09, ");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.ignorepath = val;
      }
      memfree(f1);
    } else if (strcmp(tok,"expand_to_fullpath")==0) {
      f1=getitok2(&s2,&len," \t,");
      if (f1!=NULL) {
        val=strtol(f1,&endptr,10);
        if (endptr[0]=='\0') Menulocal.expandtofullpath=val;
      }
      memfree(f1);
    } else if (strcmp(tok, "save_path") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.savepath = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "save_with_data") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.savewithdata = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "save_with_merge") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.savewithmerge = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "ngp_history") == 0) {
      for (; (s2[0] != '\0') && (strchr(" \t,", s2[0])); s2++);
      f1 = getitok2(&s2, &len, "");
      if (f1)
	arrayadd(Menulocal.ngpfilelist, &f1);
    } else if (strcmp(tok, "ngp_dir_history") == 0) {
      for (; (s2[0] != '\0') && (strchr(" \t,", s2[0])); s2++);
      f1 = getitok2(&s2, &len, "");
      if (f1)
	arrayadd(Menulocal.ngpdirlist, &f1);
    } else if (strcmp(tok, "data_history") == 0) {
      for (; (s2[0] != '\0') && (strchr(" \t,", s2[0])); s2++);
      f1 = getitok2(&s2, &len, "");
      if (f1)
	arrayadd(Menulocal.datafilelist, &f1);
    } else if (strcmp(tok, "framex") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.framex = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "framey") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.framey = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "menu_win") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      if (f1 && f2 && f3 && f4) {
	val = strtol(f1, &endptr, 10);
	if ((endptr[0] == '\0') && (val != 0))
	  Menulocal.menux = val;
	val = strtol(f2, &endptr, 10);
	if ((endptr[0] == '\0') && (val != 0))
	  Menulocal.menuy = val;
	val = strtol(f3, &endptr, 10);
	if ((endptr[0] == '\0') && (val != 0))
	  Menulocal.menuwidth = val;
	val = strtol(f4, &endptr, 10);
	if ((endptr[0] == '\0') && (val != 0))
	  Menulocal.menuheight = val;
      }
      memfree(f1);
      memfree(f2);
      memfree(f3);
      memfree(f4);
    } else if (strcmp(tok, "file_win") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      f5 = getitok2(&s2, &len, " \t,");
      if (f1 && f2 && f3 && f4 && f5) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.filex = val;
	val = strtol(f2, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.filey = val;
	val = strtol(f3, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.filewidth = val;
	val = strtol(f4, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.fileheight = val;
	val = strtol(f5, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.fileopen = val;
      }
      memfree(f1);
      memfree(f2);
      memfree(f3);
      memfree(f4);
      memfree(f5);
    } else if (strcmp(tok, "axis_win") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      f5 = getitok2(&s2, &len, " \t,");
      if (f1 && f2 && f3 && f4 && f5) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.axisx = val;
	val = strtol(f2, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.axisy = val;
	val = strtol(f3, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.axiswidth = val;
	val = strtol(f4, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.axisheight = val;
	val = strtol(f5, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.axisopen = val;
      }
      memfree(f1);
      memfree(f2);
      memfree(f3);
      memfree(f4);
      memfree(f5);
    } else if (strcmp(tok, "legend_win") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      f5 = getitok2(&s2, &len, " \t,");
      if (f1 && f2 && f3 && f4 && f5) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.legendx = val;
	val = strtol(f2, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.legendy = val;
	val = strtol(f3, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.legendwidth = val;
	val = strtol(f4, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.legendheight = val;
	val = strtol(f5, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.legendopen = val;
      }
      memfree(f1);
      memfree(f2);
      memfree(f3);
      memfree(f4);
      memfree(f5);
    } else if (strcmp(tok, "merge_win") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      f5 = getitok2(&s2, &len, " \t,");
      if (f1 && f2 && f3 && f4 && f5) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.mergex = val;
	val = strtol(f2, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.mergey = val;
	val = strtol(f3, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.mergewidth = val;
	val = strtol(f4, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.mergeheight = val;
	val = strtol(f5, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.mergeopen = val;
      }
      memfree(f1);
      memfree(f2);
      memfree(f3);
      memfree(f4);
      memfree(f5);
    } else if (strcmp(tok, "information_win") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      f5 = getitok2(&s2, &len, " \t,");
      if (f1 && f2 && f3 && f4 && f5) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.dialogx = val;
	val = strtol(f2, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.dialogy = val;
	val = strtol(f3, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.dialogwidth = val;
	val = strtol(f4, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.dialogheight = val;
	val = strtol(f5, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.dialogopen = val;
      }
      memfree(f1);
      memfree(f2);
      memfree(f3);
      memfree(f4);
      memfree(f5);
    } else if (strcmp(tok, "coordinate_win") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      f5 = getitok2(&s2, &len, " \t,");
      if (f1 && f2 && f3 && f4 && f5) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.coordx = val;
	val = strtol(f2, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.coordy = val;
	val = strtol(f3, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.coordwidth = val;
	val = strtol(f4, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.coordheight = val;
	val = strtol(f5, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.coordopen = val;
      }
      memfree(f1);
      memfree(f2);
      memfree(f3);
      memfree(f4);
      memfree(f5);
    } else if (strcmp(tok, "status_bar") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.statusb = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "show_tip") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.showtip = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "move_child_window") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      if (f1) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.movechild = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "editor") == 0) {
      memfree(Menulocal.editor);
      f1 = getitok2(&s2, &len, "");
      Menulocal.editor = f1;
    } else if (strcmp(tok, "history_size") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.hist_size = val;
      memfree(f1);
    } else if (strcmp(tok, "infowin_size") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.info_size = val;
      memfree(f1);
    } else if (strcmp(tok, "background_color") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 16);
      if (endptr[0] == '\0') {
	Menulocal.bg_r = (val >> 16) & 0xff;
	Menulocal.bg_g = (val >> 8) & 0xff;
	Menulocal.bg_b = val & 0xff;
      }
      memfree(f1);
    } else if (strcmp(tok, "browser") == 0) {
      memfree(Menulocal.browser);
      f1 = getitok2(&s2, &len, "");
      Menulocal.browser = f1;
    } else if (strcmp(tok, "help_browser") == 0) {
      memfree(Menulocal.help_browser);
      f1 = getitok2(&s2, &len, "");
      Menulocal.help_browser = f1;
    } else if (strcmp(tok, "ext_driver") == 0) {
      f1 = getitok2(&s2, &len, ",");
      f2 = getitok2(&s2, &len, ",");
      if (s2[1] == ',')
	f3 = NULL;
      else
	f3 = getitok2(&s2, &len, ",");
      for (; (s2[0] != '\0') && (strchr(" \t,", s2[0])); s2++);
      f4 = getitok2(&s2, &len, "");
      if (f1 && f2) {
	pnew = (struct extprinter *) memalloc(sizeof(struct extprinter));
	if (pnew == NULL) {
	  memfree(tok);
	  memfree(f1);
	  memfree(f2);
	  memfree(f3);
	  memfree(f4);
	  closeconfig(fp);
	  return 1;
	}
	if (pcur == NULL)
	  Menulocal.extprinterroot = pnew;
	else
	  pcur->next = pnew;
	pcur = pnew;
	pcur->next = NULL;
	pcur->name = f1;
	pcur->driver = f2;
	pcur->ext = f3;
	pcur->option = f4;
      } else {
	memfree(f1);
	memfree(f2);
	memfree(f3);
	memfree(f4);
      }
    } else if (strcmp(tok, "prn_driver") == 0) {
      f1 = getitok2(&s2, &len, ",");
      f2 = getitok2(&s2, &len, ",");
      f3 = getitok2(&s2, &len, ",");
      for (; (s2[0] != '\0') && (strchr(" \t,", s2[0])); s2++);
      f4 = getitok2(&s2, &len, "");
      if (f1 && f2) {
	if ((pnew2 = (struct prnprinter *)
	     memalloc(sizeof(struct prnprinter))) == NULL) {
	  memfree(tok);
	  memfree(f1);
	  memfree(f2);
	  memfree(f3);
	  memfree(f4);
	  closeconfig(fp);
	  return 1;
	}
	if (pcur2 == NULL)
	  Menulocal.prnprinterroot = pnew2;
	else
	  pcur2->next = pnew2;
	pcur2 = pnew2;
	pcur2->next = NULL;
	pcur2->name = f1;
	pcur2->driver = f2;
	pcur2->prn = f3;
	pcur2->option = f4;
      } else {
	memfree(f1);
	memfree(f2);
	memfree(f3);
	memfree(f4);
      }
    } else if (strcmp(tok, "script") == 0) {
      f1 = getitok2(&s2, &len, ",");
      f2 = getitok2(&s2, &len, ",");
      for (; (s2[0] != '\0') && (strchr(" \t,", s2[0])); s2++);
      f3 = getitok2(&s2, &len, "");
      if (f1 && f2) {
	if ((snew =
	     (struct script *) memalloc(sizeof(struct script))) == NULL) {
	  memfree(tok);
	  memfree(f1);
	  memfree(f2);
	  memfree(f3);
	  closeconfig(fp);
	  return 1;
	}
	if (scur == NULL)
	  Menulocal.scriptroot = snew;
	else
	  scur->next = snew;
	scur = snew;
	scur->next = NULL;
	scur->name = f1;
	scur->script = f2;
	scur->option = f3;
      } else {
	memfree(f1);
	memfree(f2);
	memfree(f3);
      }
    } else if (strcmp(tok, "antialias") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0') {
	Mxlocal->antialias = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "viewer_dpi") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Mxlocal->windpi = val;
      memfree(f1);
    } else if (strcmp(tok, "color_depth") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Mxlocal->cdepth = val;
      memfree(f1);
    } else if (strcmp(tok, "viewer_auto_redraw") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0') {
	if (val == 0)
	  Mxlocal->autoredraw = FALSE;
	else
	  Mxlocal->autoredraw = TRUE;
      }
      memfree(f1);
    } else if (strcmp(tok, "viewer_load_file_on_redraw") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0') {
	if (val == 0)
	  Mxlocal->redrawf = FALSE;
	else
	  Mxlocal->redrawf = TRUE;
      }
      memfree(f1);
    } else if (strcmp(tok, "viewer_load_file_data_number") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0') {
	Mxlocal->redrawf_num = val;
      }
      memfree(f1);
    } else if (strcmp(tok, "viewer_show_ruler") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0') {
	if (val == 0)
	  Mxlocal->ruler = FALSE;
	else
	  Mxlocal->ruler = TRUE;
      }
      memfree(f1);
    } else if (strcmp(tok, "viewer_grid") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Mxlocal->grid = val;
      memfree(f1);
    } else if (strcmp(tok, "data_head_lines") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Mxlocal->data_head_lines = val;
      memfree(f1);
    } else if (strcmp(tok, "minus_hyphen") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Mxlocal->minus_hyphen = val;
      memfree(f1);
    }
    memfree(tok);
    memfree(str);
  }
  closeconfig(fp);
  return 0;
}

void
initwindowconfig(void)
{
  Menulocal.fileopen =
    Menulocal.axisopen =
    Menulocal.legendopen  =
    Menulocal.mergeopen =
    Menulocal.dialogopen =
    Menulocal.coordopen = FALSE;

  Menulocal.filex =
    Menulocal.filey =
    Menulocal.fileheight =
    Menulocal.filewidth = CW_USEDEFAULT;

  Menulocal.axisx =
    Menulocal.axisy =
    Menulocal.axisheight =
    Menulocal.axiswidth = CW_USEDEFAULT;

  Menulocal.legendx =
    Menulocal.legendy =
    Menulocal.legendheight =
    Menulocal.legendwidth = CW_USEDEFAULT;

  Menulocal.mergex =
    Menulocal.mergey =
    Menulocal.mergeheight =
    Menulocal.mergewidth = CW_USEDEFAULT;

  Menulocal.dialogx =
    Menulocal.dialogy =
    Menulocal.dialogheight =
    Menulocal.dialogwidth = CW_USEDEFAULT;

  Menulocal.coordx =
    Menulocal.coordy =
    Menulocal.coordheight =
    Menulocal.coordwidth = CW_USEDEFAULT;
}

int
mgtkwindowconfig(void)
{
  FILE *fp;
  char *tok, *str, *s2;
  char *f1, *f2, *f3, *f4, *f5;
  int val;
  char *endptr;
  int len;

  if ((fp = openconfig(MGTKCONF)) == NULL)
    return 0;
  while ((tok = getconfig(fp, &str))) {
    s2 = str;
    if (strcmp(tok, "file_win") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      f5 = getitok2(&s2, &len, " \t,");
      if (f1 && f2 && f3 && f4 && f5) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.filex = val;
	val = strtol(f2, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.filey = val;
	val = strtol(f3, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.filewidth = val;
	val = strtol(f4, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.fileheight = val;
	val = strtol(f5, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.fileopen = val;
      }
      memfree(f1);
      memfree(f2);
      memfree(f3);
      memfree(f4);
      memfree(f5);
    } else if (strcmp(tok, "axis_win") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      f5 = getitok2(&s2, &len, " \t,");
      if (f1 && f2 && f3 && f4 && f5) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.axisx = val;
	val = strtol(f2, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.axisy = val;
	val = strtol(f3, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.axiswidth = val;
	val = strtol(f4, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.axisheight = val;
	val = strtol(f5, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.axisopen = val;
      }
      memfree(f1);
      memfree(f2);
      memfree(f3);
      memfree(f4);
      memfree(f5);
    } else if (strcmp(tok, "legend_win") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      f5 = getitok2(&s2, &len, " \t,");
      if (f1 && f2 && f3 && f4 && f5) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.legendx = val;
	val = strtol(f2, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.legendy = val;
	val = strtol(f3, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.legendwidth = val;
	val = strtol(f4, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.legendheight = val;
	val = strtol(f5, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.legendopen = val;
      }
      memfree(f1);
      memfree(f2);
      memfree(f3);
      memfree(f4);
      memfree(f5);
    } else if (strcmp(tok, "merge_win") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      f5 = getitok2(&s2, &len, " \t,");
      if (f1 && f2 && f3 && f4 && f5) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.mergex = val;
	val = strtol(f2, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.mergey = val;
	val = strtol(f3, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.mergewidth = val;
	val = strtol(f4, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.mergeheight = val;
	val = strtol(f5, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.mergeopen = val;
      }
      memfree(f1);
      memfree(f2);
      memfree(f3);
      memfree(f4);
      memfree(f5);
    } else if (strcmp(tok, "information_win") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      f5 = getitok2(&s2, &len, " \t,");
      if (f1 && f2 && f3 && f4 && f5) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.dialogx = val;
	val = strtol(f2, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.dialogy = val;
	val = strtol(f3, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.dialogwidth = val;
	val = strtol(f4, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.dialogheight = val;
	val = strtol(f5, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.dialogopen = val;
      }
      memfree(f1);
      memfree(f2);
      memfree(f3);
      memfree(f4);
      memfree(f5);
    } else if (strcmp(tok, "coordinate_win") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      f5 = getitok2(&s2, &len, " \t,");
      if (f1 && f2 && f3 && f4 && f5) {
	val = strtol(f1, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.coordx = val;
	val = strtol(f2, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.coordy = val;
	val = strtol(f3, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.coordwidth = val;
	val = strtol(f4, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.coordheight = val;
	val = strtol(f5, &endptr, 10);
	if (endptr[0] == '\0')
	  Menulocal.coordopen = val;
      }
      memfree(f1);
      memfree(f2);
      memfree(f3);
      memfree(f4);
      memfree(f5);
    }
    memfree(tok);
    memfree(str);
  }
  closeconfig(fp);
  return 0;
}

static int
exwinloadconfig(void)
{
  FILE *fp;
  char *tok, *str, *s2;
  char *f1;
  int val;
  char *endptr;
  int len;

  fp = openconfig(G2WINCONF);
  if (fp == NULL)
    return 0;

  while ((tok = getconfig(fp, &str))) {
    s2 = str;
    if (strcmp(tok, "win_dpi") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.exwindpi = val;
      memfree(f1);
    } else if (strcmp(tok, "win_width") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.exwinwidth = val;
      memfree(f1);
    } else if (strcmp(tok, "win_height") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.exwinheight = val;
      memfree(f1);
    } else if (strcmp(tok, "backing_store") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.exwinbackingstore = val;
      memfree(f1);
    } else if (strcmp(tok, "use_external_viewer") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.exwin_use_external = val;
      memfree(f1);
    }
    memfree(tok);
    memfree(str);
  }
  closeconfig(fp);
  return 0;
}

void
menuadddrawrable(struct objlist *parent, struct narray *drawrable)
{
  struct objlist *ocur;
  char *name;

  ocur = chkobjroot();
  while (ocur) {
    if (chkobjparent(ocur) == parent) {
      name = chkobjectname(ocur);
      arrayadd2(drawrable, &name);
      menuadddrawrable(ocur, drawrable);
    }
    ocur = ocur->next;
  }
}

static int
menuinit(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct objlist *robj;
  struct extprinter *pcur, *pdel;
  struct prnprinter *pcur2, *pdel2;
  struct script *scur, *sdel;
  struct gra2cairo_local *local;
  int i, numf, numd;
  char *dum;

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  if (_getobj(obj, "_local", inst, &local)) {
    local = gra2cairo_free(obj, inst);
    memfree(local);
    return 1;
  }

  Mxlocal = (struct mxlocal *) memalloc(sizeof(struct mxlocal));
  if (Mxlocal == NULL) {
    local = gra2cairo_free(obj, inst);
    memfree(local);
    return 1;
  }

  Menulocal.framex = Menulocal.framey = 0;
  Menulocal.menux = Menulocal.menuy
    = Menulocal.menuheight = Menulocal.menuwidth = CW_USEDEFAULT;
  initwindowconfig();
  Menulocal.showtip = TRUE;
  Menulocal.statusb = TRUE;
  Menulocal.movechild = FALSE;
  Menulocal.scriptconsole = FALSE;
  Menulocal.addinconsole = TRUE;
  Menulocal.mouseclick = 500;
  Menulocal.changedirectory = 1;
  Menulocal.editor = NULL;
  Menulocal.browser = NULL;
  Menulocal.help_browser = NULL;
  set_paper_type(21000, 29700);
  Menulocal.LeftMargin = 0;
  Menulocal.TopMargin = 0;
  Menulocal.PaperZoom = 10000;
  Menulocal.exwindpi = DEFAULT_DPI;
  Menulocal.exwinwidth = 0;
  Menulocal.exwinheight = 0;
  Menulocal.exwinbackingstore = FALSE;
  Menulocal.fileopendir = NULL;
  Menulocal.graphloaddir = NULL;
  Menulocal.expand = 1;
  Menulocal.expanddir = (char *) memalloc(3);
  strcpy(Menulocal.expanddir, "./");
  Menulocal.ignorepath = 0;
  Menulocal.expandtofullpath=TRUE;
  Menulocal.savepath = 0;
  Menulocal.savewithdata = 0;
  Menulocal.savewithmerge = 0;
  Menulocal.mathlist = arraynew(sizeof(char *));
  Menulocal.ngpfilelist = arraynew(sizeof(char *));
  Menulocal.ngpdirlist = arraynew(sizeof(char *));
  Menulocal.datafilelist = arraynew(sizeof(char *));
  Menulocal.GRAobj = chkobject("gra");
  Menulocal.extprinterroot = NULL;
  Menulocal.prnprinterroot = NULL;
  Menulocal.scriptroot = NULL;
  Menulocal.hist_size = 1000;
  Menulocal.info_size = 1000;
  Menulocal.bg_r = 0xff;
  Menulocal.bg_g = 0xff;
  Menulocal.bg_b = 0xff;

  arrayinit(&(Menulocal.drawrable), sizeof(char *));
  menuadddrawrable(chkobject("draw"), &(Menulocal.drawrable));

  Mxlocal->windpi = DEFAULT_DPI;
  Mxlocal->autoredraw = TRUE;
  Mxlocal->redrawf = TRUE;
  Mxlocal->redrawf_num = 0xff;
  Mxlocal->ruler = TRUE;
  Mxlocal->grid = 200;
  Mxlocal->cdepth = GTKCOLORDEPTH;
  Mxlocal->backingstore = FALSE;
  Mxlocal->minus_hyphen = TRUE;
  Mxlocal->data_head_lines = 20;
  Mxlocal->local = local;

  if (_putobj(obj, "_gtklocal", inst, Mxlocal))
    goto errexit;

  if (mgtkloadconfig())
    goto errexit;

  if (exwinloadconfig())
    goto errexit;

  Mxlocal->local->antialias = Mxlocal->antialias;
  if (_putobj(obj, "antialias", inst, &(Mxlocal->antialias)))
    goto errexit;

  numf = arraynum(Menulocal.ngpfilelist);
  numd = arraynum(Menulocal.ngpdirlist);
  dum = NULL;

  if (numd > numf) {
    for (i = numf; i < numd; i++) {
      arrayndel2(Menulocal.ngpdirlist, i);
    }
  } else if (numd < numf) {
    for (i = numd; i < numf; i++) {
      arrayadd(Menulocal.ngpdirlist, &dum);
    }
  }

  if (Menulocal.exwindpi < 1)
    Menulocal.exwindpi = DEFAULT_DPI;

  if (Menulocal.exwindpi > DPI_MAX)
    Menulocal.exwindpi = DPI_MAX;

  if (Mxlocal->windpi < 1)
    Mxlocal->windpi = DEFAULT_DPI;

  if (Mxlocal->windpi > DPI_MAX)
    Mxlocal->windpi = DPI_MAX;

  if (Mxlocal->cdepth < 2)
    Mxlocal->cdepth = 2;

  if (_putobj(obj, "dpi", inst, &(Mxlocal->windpi)))
    goto errexit;

  if (_putobj(obj, "data_head_lines", inst, &(Mxlocal->data_head_lines)))
    goto errexit;

  if (_putobj(obj, "auto_redraw", inst, &(Mxlocal->autoredraw)))
    goto errexit;

  if (_putobj(obj, "redraw_flag", inst, &(Mxlocal->redrawf)))
    goto errexit;

  if (_putobj(obj, "redraw_num", inst, &(Mxlocal->redrawf_num)))
    goto errexit;

  if (!chkobjfield(obj, "_output")) {
    Menulocal.output = getobjtblpos(obj, "_output", &robj);
    if (Menulocal.output == -1)
      goto errexit;
    Menulocal.outputobj = robj;
  } else {
    Menulocal.output = -1;
  }

  Menulocal.obj = obj;
  Menulocal.inst = inst;
  Mxlocal->pix = NULL;
  Mxlocal->win = NULL;
  Mxlocal->gc = NULL;
  Mxlocal->lock = 0;

  if (!OpenApplication())
    goto errexit;

  return 0;

errexit:
  memfree(Menulocal.editor);
  memfree(Menulocal.browser);
  memfree(Menulocal.help_browser);
  pcur = Menulocal.extprinterroot;
  while (pcur) {
    pdel = pcur;
    pcur = pcur->next;
    memfree(pdel->name);
    memfree(pdel->driver);
    memfree(pdel->ext);
    memfree(pdel->option);
    memfree(pdel);
  }
  pcur2 = Menulocal.prnprinterroot;
  while (pcur2) {
    pdel2 = pcur2;
    pcur2 = pcur2->next;
    memfree(pdel2->name);
    memfree(pdel2->driver);
    memfree(pdel2->option);
    memfree(pdel2->prn);
    memfree(pdel2);
  }
  scur = Menulocal.scriptroot;
  while (scur) {
    sdel = scur;
    scur = scur->next;
    memfree(sdel->name);
    memfree(sdel->script);
    memfree(sdel->option);
    memfree(sdel);
  }

  if (Mxlocal->pix)
    g_object_unref(Mxlocal->pix);

  memfree(Mxlocal);

  local = gra2cairo_free(obj, inst);
  memfree(local);

  return 1;
}

static int
menudone(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct extprinter *pcur, *pdel;
  struct prnprinter *pcur2, *pdel2;
  struct script *scur, *sdel;

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;
  arraydel2(&(Menulocal.drawrable));
  arrayfree2(Menulocal.mathlist);
  arrayfree2(Menulocal.ngpfilelist);
  arrayfree2(Menulocal.ngpdirlist);
  arrayfree2(Menulocal.datafilelist);
  memfree(Menulocal.editor);
  memfree(Menulocal.browser);
  memfree(Menulocal.help_browser);
  free(Menulocal.fileopendir);
  free(Menulocal.graphloaddir);
  memfree(Menulocal.expanddir);
  pcur = Menulocal.extprinterroot;
  while (pcur) {
    pdel = pcur;
    pcur = pcur->next;
    memfree(pdel->name);
    memfree(pdel->driver);
    memfree(pdel->ext);
    memfree(pdel->option);
    memfree(pdel);
  }
  pcur2 = Menulocal.prnprinterroot;
  while (pcur2) {
    pdel2 = pcur2;
    pcur2 = pcur2->next;
    memfree(pdel2->name);
    memfree(pdel2->driver);
    memfree(pdel2->option);
    memfree(pdel2->prn);
    memfree(pdel2);
  }
  scur = Menulocal.scriptroot;
  while (scur) {
    sdel = scur;
    scur = scur->next;
    memfree(sdel->name);
    memfree(sdel->script);
    memfree(sdel->option);
    memfree(sdel);
  }

  if (Mxlocal->pix)
    g_object_unref(Mxlocal->pix);

  return 0;
}


void
mgtkdisplaydialog(char *str)
{
  DisplayDialog(str);
}


void
mgtkdisplaystatus(char *str)
{
  DisplayStatus(str);
}

int
mgtkputstderr(char *s)
{
  return PutStderr(s);
}

int
mgtkputstdout(char *s)
{
  return PutStdout(s);
}

int
mgtkprintfstderr(char *fmt, ...)
{
  int len;
  char buf[1024];
  va_list ap;

  va_start(ap, fmt);
  len = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  PutStderr(buf);
  return len;
}

int
mgtkprintfstdout(char *fmt, ...)
{
  int len;
  char buf[1024];
  va_list ap;

  va_start(ap, fmt);
  len = vsnprintf(buf, sizeof(buf), fmt, ap);
  va_end(ap);
  PutStdout(buf);
  return len;
}

int
mgtkinterrupt(void)
{
  return ChkInterrupt();
}

int
mgtkinputyn(char *mes)
{
  return InputYN(mes);
}

static int
menumenu(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char *file;

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;
  if (Mxlocal->lock) {
    error(obj, ERRRUN);
    return 1;
  }
  Mxlocal->lock = 1;

  savestdio(&GtkIOSave);

  file = (char *) argv[2];
  application(file);

  loadstdio(&GtkIOSave);
  Mxlocal->lock = 0;
  return 0;
}

static int
mx_evloop(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  ResetEvent();
  return 0;
}

static int
mxredrawflag(struct objlist *obj, char *inst, char *rval, int argc,
	     char **argv)
{
  Mxlocal->redrawf = *(int *) argv[2];
  return 0;
}

static int
mxredraw_num(struct objlist *obj, char *inst, char *rval, int argc,
	     char **argv)
{
  Mxlocal->redrawf_num = *(int *) argv[2];
  return 0;
}

static int
mx_data_head_lines(struct objlist *obj, char *inst, char *rval, int argc,
	     char **argv)
{
  int n;

  n = *(int *) argv[2];

  n = (n < 0) ? 0: n;

  Mxlocal->data_head_lines = n;

  *(int *) argv[2] = n;

  return 0;
}

void
mx_redraw(struct objlist *obj, char *inst)
{
  int n;

  if (Mxlocal->region) {
    mx_clear(Mxlocal->region);
  }

  if (Mxlocal->redrawf) {
    n = Mxlocal->redrawf_num;
  } else {
    n = 0;
  }

  GRAredraw(obj, inst, TRUE, n);
  mxflush(obj, inst, NULL, 0, NULL);
}

void
mx_inslist(struct objlist *obj, char *inst,
	   struct objlist *aobj, char *ainst, char *afield, int addn)
{
  int gc;

  _getobj(obj, "_GC", inst, &gc);
  GRAinslist(gc, aobj, ainst, chkobjectname(aobj), afield, addn);
}

void
mx_dellist(struct objlist *obj, char *inst, int deln)
{
  int gc;

  _getobj(obj, "_GC", inst, &gc);
  GRAdellist(gc, deln);
}

static int
mxredraw(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  mx_redraw(obj, inst);
  return 0;
}

static int
mxautoredraw(struct objlist *obj, char *inst, char *rval,
	     int argc, char **argv)
{
  if (!(Mxlocal->autoredraw) && (*(int *) argv[2]))
    mx_redraw(obj, inst);
  Mxlocal->autoredraw = *(int *) argv[2];
  return 0;
}

static int
mxdpi(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int dpi;

  dpi = abs(*(int *) argv[2]);
  if (dpi < 1)
    dpi = 1;
  if (dpi > DPI_MAX)
    dpi = DPI_MAX;
  Mxlocal->windpi = dpi;
  Mxlocal->local->pixel_dot_x =
      Mxlocal->local->pixel_dot_y =dpi / (DPI_MAX * 1.0);
  *(int *) argv[2] = dpi;

  if (Disp && Mxlocal->win) {
    gdk_window_invalidate_rect(Mxlocal->win, NULL, TRUE);
  }
  return 0;
}

static int
mxflush(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (Mxlocal->local->linetonum && Mxlocal->local->cairo) {
    cairo_stroke(Mxlocal->local->cairo);
    Mxlocal->local->linetonum = 0;
  }

  if (Disp)
    gdk_display_flush(Disp);
  return 0;
}

void
mx_clear(GdkRegion *region)
{
  if (Disp) {
    gint w, h;
    GdkColor color;

    gdk_drawable_get_size(Mxlocal->pix, &w, &h);

    if (region) {
      gdk_gc_set_clip_region(Mxlocal->gc, region);
    } else {
      GdkRectangle rect;

      rect.x = 0;
      rect.y = 0;
      rect.width = w;
      rect.height = h;
      gdk_gc_set_clip_rectangle(Mxlocal->gc, &rect);
    }
    color.red = Menulocal.bg_r * 0xff;
    color.green = Menulocal.bg_g * 0xff;
    color.blue = Menulocal.bg_b * 0xff;
    gdk_gc_set_rgb_fg_color(Mxlocal->gc, &color);
    gdk_draw_rectangle(Mxlocal->pix, Mxlocal->gc, TRUE, 0, 0, w, h);
  }
}

static int
mxclear(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;

  mx_clear(NULL);

  if (Mxlocal->win) {
    gdk_window_invalidate_rect(Mxlocal->win, NULL, TRUE);
  }

return 0;
}

int
mxd2p(int r)
{
  return nround(r * Mxlocal->local->pixel_dot_x);
}

int
mxd2px(int x)
{
  return nround(x * Mxlocal->local->pixel_dot_x + Mxlocal->local->offsetx);
  //  return nround(x * Mxlocal->pixel_dot + Mxlocal->offsetx - Mxlocal->scrollx);
}

int
mxd2py(int y)
{
  return nround(y * Mxlocal->local->pixel_dot_y + Mxlocal->local->offsety);
  //  return nround(y * Mxlocal->pixel_dot + Mxlocal->offsety - Mxlocal->scrolly);
}

int
mxp2d(int r)
{
  return ceil(r / Mxlocal->local->pixel_dot_x);
  //  return nround(r / Mxlocal->pixel_dot);
}

static int
mxfullpathngp(struct objlist *obj, char *inst, char *rval, int argc,
	      char **argv)
{
  char *name, *ngp2;

  name = (char *) argv[2];
  if (name == NULL)
    ngp2 = NULL;
  else
    ngp2 = getbasename(name);
  putobj(Menulocal.obj, "ngp", 0, ngp2);
  return 0;
}

static int
mx_get_focused(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int num, i, id;
  char buf[256], *name, *ptr;
  struct narray *oarray;
  struct Viewer *d;
  struct focuslist **focus;
  

  memfree(*(struct narray **)rval);
  *(char **)rval = NULL;

  d = &(NgraphApp.Viewer);

  num = arraynum(d->focusobj);
  focus = (struct focuslist **) arraydata(d->focusobj);

  if (num < 1)
    return 1;

  oarray = arraynew(sizeof(char *));
  for (i = 0; i < num; i++) {
    inst = chkobjinstoid(focus[i]->obj, focus[i]->oid);
    if (inst) {
      _getobj(focus[i]->obj, "id", inst, &id);
      name = chkobjectname(focus[i]->obj);
      snprintf(buf, sizeof(buf), "%s:%d", name, id);
      ptr = nstrdup(buf);
      if (ptr) {
	arrayadd(oarray, &ptr);
      }
    }
  }

  *(struct narray **)rval = oarray;

  return 0;

}

static int
mx_print(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int show_dialog, select_file, create_window = FALSE, lock;
  GtkWidget *label;

  select_file = * (int *) argv[2];
  show_dialog = * (int *) argv[3];

  if (TopLevel == NULL) {
    create_window = TRUE;
    TopLevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_type_hint(GTK_WINDOW(TopLevel), GDK_WINDOW_TYPE_HINT_DIALOG);
    g_signal_connect(TopLevel, "delete-event", G_CALLBACK(gtk_true), NULL);
    label = gtk_label_new(" Ngraph ");
    gtk_container_add(GTK_CONTAINER(TopLevel), label);
    gtk_widget_show_all(TopLevel);
    ResetEvent();
  }

  lock = Menulock;
  Menulock = FALSE;
  CmOutputPrinter(select_file, show_dialog);
  Menulock = lock;

  if (create_window) {
    gtk_widget_destroy(TopLevel);
    TopLevel = NULL;
    ResetEvent();
  }
  return 0;
}


static struct objtable gtkmenu[] = {
  {"init", NVFUNC, NEXEC, menuinit, NULL, 0},
  {"done", NVFUNC, NEXEC, menudone, NULL, 0},
  {"menu", NVFUNC, NREAD | NEXEC, menumenu, NULL, 0},
  {"ngp", NSTR, NREAD | NWRITE, NULL, NULL, 0},
  {"fullpath_ngp", NSTR, NREAD | NWRITE, mxfullpathngp, NULL, 0},
  {"dpi", NINT, NREAD | NWRITE, mxdpi, NULL, 0},
  {"auto_redraw", NBOOL, NREAD | NWRITE, mxautoredraw, NULL, 0},
  {"redraw_flag", NBOOL, NREAD | NWRITE, mxredrawflag, NULL, 0},
  {"redraw_num", NBOOL, NREAD | NWRITE, mxredraw_num, NULL, 0},
  {"redraw", NVFUNC, NREAD | NEXEC, mxredraw, "", 0},
  {"flush", NVFUNC, NREAD | NEXEC, mxflush, "", 0},
  {"clear", NVFUNC, NREAD | NEXEC, mxclear, "", 0},
  {"focused", NSAFUNC, NREAD | NEXEC, mx_get_focused, NULL, 0},
  {"print", NSAFUNC, NREAD | NEXEC, mx_print, "bi", 0},
  {"data_head_lines", NINT, NREAD | NWRITE, mx_data_head_lines, NULL, 0},
  {"_gtklocal", NPOINTER, 0, NULL, NULL, 0},
  {"_evloop", NVFUNC, 0, mx_evloop, NULL, 0},
};

#define TBLNUM (sizeof(gtkmenu) / sizeof(*gtkmenu))

void *
addmenu()
{
  return addobject(NAME, ALIAS, PARENT, NVERSION, TBLNUM, gtkmenu, ERRNUM,
		   menuerrorlist, NULL, NULL);
}

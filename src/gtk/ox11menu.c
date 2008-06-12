/* 
 * $Id: ox11menu.c,v 1.12 2008/06/12 07:11:45 hito Exp $
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
#include "jstring.h"
#include "nconfig.h"
#include "mathfn.h"
#include "gra.h"
#include "spline.h"

#include "strconv.h"

#include "ogra2x11.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11gui.h"
#include "x11view.h"

#define NAME "menu"
#define ALIAS "winmenu:gtkmenu"
#define PARENT "gra2"
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
  char *endptr, symbol[] = "Sym";
  int len;
  struct fontmap *fcur, *fnew;
  struct extprinter *pcur, *pnew;
  struct prnprinter *pcur2, *pnew2;
  struct script *scur, *snew;

  fp = openconfig(MGTKCONF);
  if (fp == NULL)
    return 0;

  fcur = Mxlocal->fontmaproot;
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
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.framex = val;
    } else if (strcmp(tok, "framey") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      val = strtol(f1, &endptr, 10);
      if (endptr[0] == '\0')
	Menulocal.framey = val;
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
    } else if (strcmp(tok, "font_map") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      for (; (s2[0] != '\0') && (strchr(" \x09,", s2[0])); s2++);
      f4 = getitok2(&s2, &len, "");
      if (f1 && f2 && f3 && f4) {
	if ((fnew = memalloc(sizeof(struct fontmap))) == NULL) {
	  memfree(tok);
	  memfree(f1);
	  memfree(f2);
	  memfree(f3);
	  memfree(f4);
	  closeconfig(fp);
	  return 1;
	}
	if (fcur == NULL)
	  Mxlocal->fontmaproot = fnew;
	else
	  fcur->next = fnew;
	fcur = fnew;
	fcur->next = NULL;
	fcur->fontalias = f1;
	fcur->symbol = ! strncmp(f1, symbol, sizeof(symbol) - 1);
	if (strcmp(f2, "bold") == 0)
	  fcur->type = BOLD;
	else if (strcmp(f2, "italic") == 0)
	  fcur->type = ITALIC;
	else if (strcmp(f2, "bold_italic") == 0)
	  fcur->type = BOLDITALIC;
	else if (strcmp(f2, "oblique") == 0)
	  fcur->type = OBLIQUE;
	else if (strcmp(f2, "bold_oblique") == 0)
	  fcur->type = BOLDOBLIQUE;
	else
	  fcur->type = NORMAL;
	memfree(f2);
	val = strtol(f3, &endptr, 10);
	memfree(f3);
	fcur->twobyte = val;
	fcur->fontname = f4;
      } else {
	memfree(f1);
	memfree(f2);
	memfree(f3);
	memfree(f4);
      }
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
  struct fontmap *fcur, *fdel;
  struct extprinter *pcur, *pdel;
  struct prnprinter *pcur2, *pdel2;
  struct script *scur, *sdel;
  int i, numf, numd;
  char *dum;

  if (_exeparent(obj, (char *) argv[1], inst, rval, argc, argv))
    return 1;

  Mxlocal = (struct mxlocal *) memalloc(sizeof(struct mxlocal));
  if (Mxlocal == NULL)
    return 1;

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
  Menulocal.PaperWidth = 21000;
  Menulocal.PaperHeight = 29700;
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

  Mxlocal->fontmaproot = NULL;
  Mxlocal->windpi = DEFAULT_DPI;
  Mxlocal->autoredraw = TRUE;
  Mxlocal->redrawf = TRUE;
  Mxlocal->redrawf_num = 0xff;
  Mxlocal->ruler = TRUE;
  Mxlocal->grid = 200;
  Mxlocal->cdepth = GTKCOLORDEPTH;
  Mxlocal->backingstore = FALSE;
  Mxlocal->minus_hyphen = TRUE;

  if (_putobj(obj, "_local", inst, Mxlocal))
    goto errexit;

  if (mgtkloadconfig())
    goto errexit;

  if (exwinloadconfig())
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
  fcur = Mxlocal->fontmaproot;
  while (fcur) {
    fdel = fcur;
    fcur = fcur->next;
    memfree(fdel->fontalias);
    memfree(fdel->fontname);
    memfree(fdel);
  }

  if (Mxlocal->pix)
    g_object_unref(Mxlocal->pix);

  memfree(Mxlocal);
  return 1;
}

static int
menudone(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct fontmap *fcur, *fdel;
  struct extprinter *pcur, *pdel;
  struct prnprinter *pcur2, *pdel2;
  struct script *scur, *sdel;
  int i;

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
  fcur = Mxlocal->fontmaproot;
  while (fcur) {
    fdel = fcur;
    fcur = fcur->next;
    memfree(fdel->fontname);
    memfree(fdel->fontalias);
    memfree(fdel);
  }

  for (i = 0; i < Mxlocal->loadfont; i++) {
    if (Mxlocal->font[i].fontalias) {
      memfree(Mxlocal->font[i].fontalias);
      pango_font_description_free(Mxlocal->font[i].font);
      Mxlocal->font[i].fontalias = NULL;
      Mxlocal->font[i].font = NULL;
    }
  }
  Mxlocal->loadfont = 0;

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
  int GC;

  _getobj(obj, "_GC", inst, &GC);
  GRAinslist(GC, aobj, ainst, chkobjectname(aobj), afield, addn);
}

void
mx_dellist(struct objlist *obj, char *inst, int deln)
{
  int GC;

  _getobj(obj, "_GC", inst, &GC);
  GRAdellist(GC, deln);
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
  Mxlocal->pixel_dot = dpi / (DPI_MAX * 1.0);
  *(int *) argv[2] = dpi;
  if (Disp && Mxlocal->win) {
    gdk_window_invalidate_rect(Mxlocal->win, NULL, TRUE);
  }
  return 0;
}

static int
mxflush(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  if (Mxlocal->linetonum != 0) {
    if (Disp) {
      gdk_draw_lines(Mxlocal->pix, Mxlocal->gc,
		     Mxlocal->points, Mxlocal->linetonum);
      Mxlocal->linetonum = 0;
    }
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

  if (Mxlocal->linetonum && Disp) {
    gdk_draw_lines(Mxlocal->pix, Mxlocal->gc, Mxlocal->points, Mxlocal->linetonum);
    Mxlocal->linetonum = 0;
  }

  mx_clear(NULL);

  if (Mxlocal->win) {
    gdk_window_invalidate_rect(Mxlocal->win, NULL, TRUE);
  }

return 0;
}

void
mxsaveGC(GdkGC * gc, GdkDrawable *pix, GdkDrawable * d, int scrollx, int scrolly,
	 struct mxlocal *mxsave, int dpi, GdkRegion * region)
{
  memcpy(mxsave, Mxlocal, sizeof(*mxsave));
  Mxlocal->pix = pix;
  Mxlocal->win = d;
  Mxlocal->gc = gc;
  Mxlocal->scrollx = scrollx;
  Mxlocal->scrolly = scrolly;
  Mxlocal->offsetx = 0;
  Mxlocal->offsety = 0;
  Mxlocal->cpx = 0;
  Mxlocal->cpy = 0;
  Mxlocal->fontalias = NULL;
  /*
  for (i = 0; i < GTKFONTCASH; i++)
    (Mxlocal->font[i]).fontalias = NULL;
  Mxlocal->loadfont = 0;
  Mxlocal->loadfontf = -1;
  */
  Mxlocal->linetonum = 0;
  if (dpi != -1) {
    Mxlocal->windpi = dpi;
    Mxlocal->pixel_dot = Mxlocal->windpi / (DPI_MAX * 1.0);
  } else {
    Mxlocal->windpi = mxsave->windpi;
    Mxlocal->pixel_dot = mxsave->pixel_dot;
  }
  Mxlocal->region = region;
}

void
mxrestoreGC(struct mxlocal *mxsave)
{
  int i;
  struct fontlocal font[GTKFONTCASH];

  if (Mxlocal->linetonum != 0) {
    if (Disp) {
      gdk_draw_lines(Mxlocal->pix, Mxlocal->gc,
		     Mxlocal->points, Mxlocal->linetonum);
      Mxlocal->linetonum = 0;
      gdk_display_flush(Disp);
    }
  }

  if (Mxlocal->region) {
    gdk_region_destroy(Mxlocal->region);
    Mxlocal->region = NULL;
  }

  memfree(Mxlocal->fontalias);

  for (i = 0; i < GTKFONTCASH; i++) {
    font[i] = Mxlocal->font[i];
  }

  memcpy(Mxlocal, mxsave, sizeof(*mxsave));

  for (i = 0; i < GTKFONTCASH; i++) {
    Mxlocal->font[i] = font[i];
  }
}

int
mxd2p(int r)
{
  return nround(r * Mxlocal->pixel_dot);
}

int
mxd2px(int x)
{
  return nround(x * Mxlocal->pixel_dot + Mxlocal->offsetx);
  //  return nround(x * Mxlocal->pixel_dot + Mxlocal->offsetx - Mxlocal->scrollx);
}

int
mxd2py(int y)
{
  return nround(y * Mxlocal->pixel_dot + Mxlocal->offsety);
  //  return nround(y * Mxlocal->pixel_dot + Mxlocal->offsety - Mxlocal->scrolly);
}

int
mxp2d(int r)
{
  return ceil(r / Mxlocal->pixel_dot);
  //  return nround(r / Mxlocal->pixel_dot);
}

static int
calc_color(int c)
{
  c = c * Mxlocal->cdepth / 256;
  if (c >= Mxlocal->cdepth) {
    c = Mxlocal->cdepth - 1;
  } else if (c < 0) {
    c = 0;
  }
  return c * 65535 / (Mxlocal->cdepth - 1);
}

unsigned long
RGB(int R, int G, int B)
{
  GdkColor col;

  col.red = calc_color(R);
  col.green = calc_color(G);
  col.blue = calc_color(B);

  gdk_colormap_alloc_color(&(Mxlocal->cmap), &col, TRUE, TRUE);

  return col.pixel;
}

static int
mxloadfont(char *fontalias, int top)
{
  struct fontlocal font;
  struct fontmap *fcur;
  char *fontname;
  int twobyte = FALSE, type = NORMAL, fontcashfind, i, store, symbol = FALSE;
  PangoFontDescription *pfont;
  PangoStyle style;
  PangoWeight weight;
  static PangoLanguage *lang_ja = NULL, *lang = NULL;

  fontcashfind = -1;


  if (lang == NULL) {
    lang = pango_language_from_string("en-US");
    lang_ja = pango_language_from_string("ja-JP");
  }

  for (i = 0; i < Mxlocal->loadfont; i++) {
    if (strcmp((Mxlocal->font[i]).fontalias, fontalias) == 0) {
      fontcashfind=i;
      break;
    }
  }

  if (fontcashfind != -1) {
    if (top) {
      font = Mxlocal->font[fontcashfind];
      for (i = fontcashfind - 1; i >= 0; i--) {
	Mxlocal->font[i + 1] = Mxlocal->font[i];
      }
      Mxlocal->font[0] = font;
      return 0;
    } else {
      return fontcashfind;
    }
  }

  fontname = NULL;

  for (fcur = Mxlocal->fontmaproot; fcur; fcur=fcur->next) {
    if (strcmp(fontalias, fcur->fontalias) == 0) {
      fontname = fcur->fontname;
      type = fcur->type;
      twobyte = fcur->twobyte;
      symbol = fcur->symbol;
      break;
    }
  }

  pfont = pango_font_description_new();
  pango_font_description_set_family(pfont, fontname);

  switch (type) {
  case ITALIC:
  case BOLDITALIC:
    style = PANGO_STYLE_ITALIC;
    break;
  case OBLIQUE:
  case BOLDOBLIQUE:
    style = PANGO_STYLE_OBLIQUE;
    break;
  default:
    style = PANGO_STYLE_NORMAL;
    break;
  }
  pango_font_description_set_style(pfont, style);

  switch (type) {
  case BOLD:
  case BOLDITALIC:
  case BOLDOBLIQUE:
    weight = PANGO_WEIGHT_BOLD;
    break;
  default:
    weight = PANGO_WEIGHT_NORMAL;
    break;
  }
  pango_font_description_set_weight(pfont, weight);

  if (Mxlocal->loadfont == GTKFONTCASH) {
    i = GTKFONTCASH - 1;

    memfree((Mxlocal->font[i]).fontalias);
    Mxlocal->font[i].fontalias = NULL;

    pango_font_description_free((Mxlocal->font[i]).font);
    Mxlocal->font[i].font = NULL;

    Mxlocal->loadfont--;
  }

  if (top) {
    for (i = Mxlocal->loadfont - 1; i >= 0; i--) {
      Mxlocal->font[i + 1] = Mxlocal->font[i];
    }
    store=0;
  } else {
    store = Mxlocal->loadfont;
  }

  Mxlocal->font[store].fontalias = nstrdup(fontalias);
  if (Mxlocal->font[store].fontalias == NULL) {
    pango_font_description_free(pfont);
    Mxlocal->font[store].font = NULL;
    return -1;
  }

  Mxlocal->font[store].font = pfont;
  Mxlocal->font[store].fonttype = type;
  Mxlocal->font[store].symbol = symbol;

  Mxlocal->loadfont++;

  return store;
}

static GdkColor *
gtkRGB(struct mxlocal *gtklocal, int R, int G, int B)
{
  static GdkColor col;

  R = R * gtklocal->cdepth / 256;
  if (R >= gtklocal->cdepth) {
    R = gtklocal->cdepth - 1;
  } else if (R < 0) {
    R = 0;
  }

  G = G * gtklocal->cdepth / 256;
  if (G >= gtklocal->cdepth) {
    G = gtklocal->cdepth - 1;
  } else if (G < 0) {
    G = 0;
  }

  B = B * gtklocal->cdepth / 256;
  if (B >= gtklocal->cdepth) {
    B = gtklocal->cdepth - 1;
  } else if (B < 0) {
    B = 0;
  }

  col.red = R * 65535 / (gtklocal->cdepth - 1);
  col.green = G * 65535 / (gtklocal->cdepth - 1);
  col.blue = B * 65535 / (gtklocal->cdepth - 1);

  return &col;
}

static void
draw_str(GdkDrawable *pix, char *str, int font, int size, int space, int *fw, int *ah, int *dh)
{
  PangoLayout *layout;
  PangoAttribute *attr;
  PangoAttrList *alist;
  PangoContext *context;
  PangoMatrix matrix = PANGO_MATRIX_INIT;
  PangoLayoutIter *piter;
  int x, y, w, h, width, height, baseline;

  layout = gtk_widget_create_pango_layout(TopLevel, "");
  context = pango_layout_get_context(layout);
  pango_matrix_rotate(&matrix, Mxlocal->fontdir);
  pango_context_set_matrix(context, &matrix);

  alist = pango_attr_list_new();

  attr = pango_attr_size_new_absolute(mxd2p(size) * PANGO_SCALE);
  pango_attr_list_insert(alist, attr);

  attr = pango_attr_letter_spacing_new(mxd2p(space) * PANGO_SCALE);
  pango_attr_list_insert(alist, attr);

  pango_layout_set_font_description(layout, Mxlocal->font[font].font);
  pango_layout_set_attributes(layout, alist);

  pango_layout_set_text(layout, str, -1);

  pango_layout_get_pixel_size(layout, &w, &h);
  piter = pango_layout_get_iter(layout);
  baseline = pango_layout_iter_get_baseline(piter) / PANGO_SCALE;

  x = mxd2px(Mxlocal->cpx);
  y = mxd2py(Mxlocal->cpy);

#if 0
  if (pix) {
    PangoLayoutLine *pline;
    PangoRectangle prect;
    int ascent, descent, s = mxd2p(size);

    pline = pango_layout_get_line_readonly(layout, 0);
    pango_layout_line_get_pixel_extents(pline, &prect, NULL);
    ascent = PANGO_ASCENT(prect);
    descent = PANGO_DESCENT(prect);

    gdk_draw_rectangle(Mxlocal->pix, Mxlocal->gc, FALSE, x, y  - baseline, s, s);

    gdk_gc_set_rgb_fg_color(Mxlocal->gc, &red);
    gdk_draw_rectangle(Mxlocal->pix, Mxlocal->gc, FALSE, x, y - baseline, w, h);
    gdk_draw_line(Mxlocal->pix, Mxlocal->gc, x, y - baseline, x + w, y - baseline);

    gdk_gc_set_rgb_fg_color(Mxlocal->gc, &blue);

    gdk_draw_rectangle(Mxlocal->pix, Mxlocal->gc, FALSE, x + prect.x, y - ascent, prect.width, prect.height);
    gdk_draw_line(Mxlocal->pix, Mxlocal->gc, x + prect.x, y, x + prect.x + prect.width, y);

    gdk_gc_set_rgb_fg_color(Mxlocal->gc, &black);
  }
#endif

  if (Mxlocal->fontdir <= 90) {
    width = Mxlocal->fontcos * w + Mxlocal->fontsin * baseline;
    height = Mxlocal->fontcos * baseline + Mxlocal->fontsin * w;
    x -= Mxlocal->fontsin * baseline;
    y -= height;
  } else if (Mxlocal->fontdir < 180) {
    width = - Mxlocal->fontcos * w + Mxlocal->fontsin * baseline;
    height = - Mxlocal->fontcos * baseline + Mxlocal->fontsin * w;
    x -= width;
    y -= height - Mxlocal->fontsin * baseline;
  } else if (Mxlocal->fontdir <= 270) {
    width = - Mxlocal->fontcos * w - Mxlocal->fontsin * baseline;
    height = - Mxlocal->fontcos * baseline - Mxlocal->fontsin * w;
    x -= width + Mxlocal->fontsin * baseline;
  } else if (Mxlocal->fontdir < 360) {
    width = Mxlocal->fontcos * w - Mxlocal->fontsin * baseline;
    height = Mxlocal->fontcos * baseline - Mxlocal->fontsin * w;
    y += Mxlocal->fontsin * baseline;
  }

  if (fw)
    *fw = w;

  if (ah)
    *ah = baseline;

  if (dh)
    *dh = h - baseline;

  if (pix && str) {
    gdk_draw_layout(pix, Mxlocal->gc, x, y, layout);
    Mxlocal->cpx += mxp2d(w * Mxlocal->fontcos);
    Mxlocal->cpy -= mxp2d(w * Mxlocal->fontsin);
  }

  pango_layout_iter_free(piter);
  pango_attr_list_unref(alist);
  g_object_unref(layout);
}

static int
mx_output(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  char code;
  int *cpar, i, j, x, y, x1, y1, x2, y2, width, l, fontcashsize, fontcashdir;
  char *cstr, *tmp, *tmp2;
  double fontsize, fontspace, fontdir, fontsin, fontcos;
  GdkRectangle rect;
  GdkJoinStyle join;
  GdkCapStyle cap;
  GdkLineStyle style;
  gint8 *dashlist = NULL;
  int arcmode;
  GdkPoint *xpoint;
  GdkRegion *region;

  code = *(char *) (argv[3]);
  cpar = (int *) argv[4];
  cstr = (char *) argv[5];

  if (Disp == NULL || Mxlocal->gc == NULL ||  Mxlocal->pix == NULL)
    return -1;

  if (Mxlocal->linetonum != 0) {
    if ((code != 'T') || (Mxlocal->linetonum >= LINETOLIMIT)) {
      gdk_draw_lines(Mxlocal->pix, Mxlocal->gc,
		     Mxlocal->points, Mxlocal->linetonum);
      Mxlocal->linetonum = 0;
    }
  }
  switch (code) {
  case 'I':
    break;
  case '%': case 'X':
    break;
  case 'E':
    gdk_display_flush(Disp);
    break;
  case 'V':
    Mxlocal->offsetx = mxd2p(cpar[1]);
    Mxlocal->offsety = mxd2p(cpar[2]);
    Mxlocal->cpx = 0;
    Mxlocal->cpy = 0;
    if (cpar[5]) {
      rect.x = mxd2p(cpar[1]);
      rect.y = mxd2p(cpar[2]);
      rect.width = mxd2p(cpar[3]) - rect.x;
      rect.height = mxd2p(cpar[4]) - rect.y;

      region = gdk_region_new();
      gdk_region_union_with_rect(region, &rect);
      if (Mxlocal->region) {
        gdk_region_intersect(region, Mxlocal->region);
      }
      gdk_gc_set_clip_region(Mxlocal->gc, region);
      gdk_region_destroy(region);
    } else {
      if (Mxlocal->region) {
	gdk_gc_set_clip_region(Mxlocal->gc, Mxlocal->region);
      } else {
	rect.x = 0;
	rect.y = 0;
	rect.width = SHRT_MAX;
	rect.height = SHRT_MAX;
	gdk_gc_set_clip_rectangle(Mxlocal->gc, &rect);
      }
    }
    break;
  case 'A':
    if (cpar[1] == 0) {
      style = LineSolid;
    } else {
      style = LineOnOffDash;
      dashlist = memalloc(sizeof(char) * cpar[1]);
      if (dashlist == NULL)
	break;
      for (i = 0; i < cpar[1]; i++) {
	dashlist[i] = mxd2p(cpar[6 + i]);
        if (dashlist[i] <= 0) {
	  dashlist[i]=1;
	}
      }
    }

    width = mxd2p(cpar[2]);

    if (cpar[3] == 2) {
      cap = CapProjecting;
    } else if (cpar[3] == 1) {
      cap = CapRound;
    } else {
      cap=CapButt;
    }

    if (cpar[4] == 2) {
      join = JoinBevel;
    } else if (cpar[4] == 1) {
      join = JoinRound;
    } else {
      join = JoinMiter;
    }
    gdk_gc_set_line_attributes(Mxlocal->gc, width, style, cap, join);

    if (style != LineSolid) {
      gdk_gc_set_dashes(Mxlocal->gc, 0, dashlist, cpar[1]);
    }

    if (cpar[1] != 0) {
      memfree(dashlist);
    }
    break;
  case 'G':
    gdk_gc_set_rgb_fg_color(Mxlocal->gc, gtkRGB(Mxlocal, cpar[1], cpar[2], cpar[3]));
    break;
  case 'M':
    Mxlocal->cpx = cpar[1];
    Mxlocal->cpy = cpar[2];
    break;
  case 'N':
    Mxlocal->cpx += cpar[1];
    Mxlocal->cpy += cpar[2];
    break;
  case 'L':
    gdk_draw_line(Mxlocal->pix, Mxlocal->gc,
              mxd2px(cpar[1]), mxd2py(cpar[2]),
              mxd2px(cpar[3]), mxd2py(cpar[4]));
    break;
  case 'T':
    x = mxd2px(cpar[1]);
    y = mxd2py(cpar[2]);
    if (Mxlocal->linetonum == 0) {
      Mxlocal->points[0].x = mxd2px(Mxlocal->cpx);
      Mxlocal->points[0].y = mxd2py(Mxlocal->cpy);;
      Mxlocal->linetonum++;
    }
    Mxlocal->points[Mxlocal->linetonum].x = x;
    Mxlocal->points[Mxlocal->linetonum].y = y;
    Mxlocal->linetonum++;
    Mxlocal->cpx = cpar[1];
    Mxlocal->cpy = cpar[2];
    break;
  case 'C':
    if (cpar[7]==0) {
      gdk_draw_arc(Mxlocal->pix, Mxlocal->gc,
		   FALSE,
		   mxd2px(cpar[1] - cpar[3]),
		   mxd2py(cpar[2] - cpar[4]),
		   mxd2p(2 * cpar[3]),
		   mxd2p(2 * cpar[4]),
		   (int) cpar[5] * 64 / 100, (int) cpar[6] * 64 / 100);
    } else {
      if ((mxd2p(cpar[3]) < 2) && (mxd2p(cpar[4]) < 2)) {
	gdk_draw_point(Mxlocal->pix, Mxlocal->gc,
                   mxd2px(cpar[1]), mxd2py(cpar[2]));
      } else {
        if (cpar[7] == 1) {
	  arcmode = ArcPieSlice;
	} else {
	  arcmode = ArcChord;
	}
	gdkgc_set_arc_mode(Mxlocal->gc, arcmode);
	gdk_draw_arc(Mxlocal->pix, Mxlocal->gc,
		     TRUE,
		     mxd2px(cpar[1] - cpar[3]),
		     mxd2py(cpar[2] - cpar[4]),
		     mxd2p(2 * cpar[3]), mxd2p(2 * cpar[4]),
		     (int) cpar[5] * 64 / 100, (int) cpar[6] * 64 / 100);
      }
    }
    break;
  case 'B':
    if (cpar[1] <= cpar[3]) {
      x1 = mxd2px(cpar[1]);
      x2 = mxd2p(cpar[3] - cpar[1]);
    } else {
      x1 = mxd2px(cpar[3]);
      x2 = mxd2p(cpar[1] - cpar[3]);
    }
    if (cpar[2] <= cpar[4]) {
      y1 = mxd2py(cpar[2]);
      y2 = mxd2p(cpar[4] - cpar[2]);
    } else {
      y1 = mxd2py(cpar[4]);
      y2 = mxd2p(cpar[2] - cpar[4]);
    }
    if (cpar[5] == 0) {
      gdk_draw_rectangle(Mxlocal->pix, Mxlocal->gc, FALSE, 
			 x1, y1, x2 + 1, y2 + 1);
    } else {
      gdk_draw_rectangle(Mxlocal->pix, Mxlocal->gc, TRUE, x1, y1, x2 + 1, y2 + 1);
    }
    break;
  case 'P': 
    gdk_draw_point(Mxlocal->pix, Mxlocal->gc,
		   mxd2px(cpar[1]), mxd2py(cpar[2]));
    break;
  case 'R': 
    if (cpar[1] == 0) break;
    if ((xpoint = memalloc(sizeof(*xpoint) *cpar[1])) == NULL) break;
    for (i = 0; i < cpar[1]; i++) {
      xpoint[i].x = mxd2px(cpar[i * 2 + 2]);
      xpoint[i].y = mxd2py(cpar[i * 2 + 3]);
    }
    gdk_draw_polygon(Mxlocal->pix, Mxlocal->gc, FALSE, xpoint, cpar[1]);
    memfree(xpoint);
    break;
  case 'D': 
    if (cpar[1] == 0) break;
    if ((xpoint = memalloc(sizeof(*xpoint) *cpar[1])) == NULL) break;
    for (i = 0; i < cpar[1]; i++) {
      xpoint[i].x = mxd2px(cpar[i * 2 + 3]);
      xpoint[i].y = mxd2py(cpar[i * 2 + 4]);
    }
    if (cpar[2] == 1) {
      gdkgc_set_fill_rule(Mxlocal->gc, EvenOddRule);
    } else {
      gdkgc_set_fill_rule(Mxlocal->gc, WindingRule);
    }

    gdk_draw_polygon(Mxlocal->pix, Mxlocal->gc, cpar[2] != 0, xpoint, cpar[1]);
    memfree(xpoint);
    break;
  case 'F':
    memfree(Mxlocal->fontalias);
    Mxlocal->fontalias = nstrdup(cstr);
    break;
  case 'H':
    fontspace = cpar[2] / 72.0 * 25.4;
    Mxlocal->fontspace = fontspace;
    fontsize = cpar[1] / 72.0 * 25.4;
    Mxlocal->fontsize = fontsize;
    fontcashsize = mxd2p(fontsize);
    fontcashdir = cpar[3];
    fontdir = cpar[3] * MPI / 18000.0;
    fontsin = sin(fontdir);
    fontcos = cos(fontdir);
    Mxlocal->fontdir = (cpar[3] % 36000) / 100.0;
    if (Mxlocal->fontdir < 0) {
      Mxlocal->fontdir += 360;
    }
    Mxlocal->fontsin = fontsin;
    Mxlocal->fontcos = fontcos;

    Mxlocal->loadfontf = mxloadfont(Mxlocal->fontalias, TRUE);
    break;
  case 'S':
    if (Mxlocal->loadfontf == -1)
      break;

    tmp = strdup(cstr);
    if (tmp == NULL)
      break;

    l = strlen(cstr);
    for (j = i = 0; i <= l; i++, j++) {
      char c;
      if (cstr[i] == '\\') {
	i++;
        if (cstr[i] == 'x') {
	  i++;
	  c = toupper(cstr[i]);
          if (c >= 'A') {
	    tmp[j] = c - 'A' + 10;
	  } else { 
	    tmp[j] = cstr[i] - '0';
	  }

	  i++;
	  tmp[j] *= 16;
	  c = toupper(cstr[i]);
          if (c >= 'A'){
	    tmp[j] += c - 'A' + 10;
	  } else {
	    tmp[j] += cstr[i] - '0';
	  }
        } else if (cstr[i] != '\0') {
          tmp[j] = cstr[i];
        }
      } else {
        tmp[j] = cstr[i];
      }
    }

    tmp2= iso8859_to_utf8(tmp);
    if (tmp2 == NULL) {
      free(tmp);
      break;
    }

    if (Mxlocal->font[0].symbol) {
      char *ptr;

      ptr = ascii2greece(tmp2);
      if (ptr) {
	free(tmp2);
	tmp2 = ptr;
      }
    }
    draw_str(Mxlocal->pix, tmp2, 0, Mxlocal->fontsize, Mxlocal->fontspace, NULL, NULL, NULL);
    free(tmp2);
    free(tmp);
    break;
  case 'K':
    tmp2 = sjis_to_utf8(cstr);
    if (tmp2 == NULL) 
      break;
    draw_str(Mxlocal->pix, tmp2, 0, Mxlocal->fontsize, Mxlocal->fontspace, NULL, NULL, NULL);
    free(tmp2);
    break;
  default:
    break;
  }
  return 0;
}

static int
mx_charwidth(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct mxlocal *mxlocal;
  char ch[3], *font, *tmp;
  double size;
  //  XChar2b kanji[1];
  int cashpos, width;;
  //  XFontStruct *fontstruct;

  ch[0] = (*(unsigned int *)(argv[3]) & 0xff);
  ch[1] = (*(unsigned int *)(argv[3]) & 0xff00) >> 8;
  ch[2] = '\0';

  size = (*(int *)(argv[4])) / 72.0 * 25.4;
  font = (char *)(argv[5]);

  if (_getobj(obj, "_local", inst, &mxlocal))
    return 1;

  cashpos = mxloadfont(font, FALSE);

  if (cashpos == -1) {
    *(int *) rval = nround(size * 0.600);
    return 0;
  }

  if (ch[1]) {
    tmp = sjis_to_utf8(ch);
    draw_str(NULL, tmp, cashpos, size, 0, &width, NULL, NULL);
    *(int *) rval = mxp2d(width);
    free(tmp);
  } else {
    tmp = iso8859_to_utf8(ch);
    draw_str(NULL, tmp, cashpos, size, 0, &width, NULL, NULL);
    *(int *) rval = mxp2d(width);
    free(tmp);
  }

  return 0;
}

static int
mx_charheight(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct mxlocal *mxlocal;
  char *font;
  double size;
  char *func;
  int height, descent, ascent, cashpos;
  //  XFontStruct *fontstruct;
  struct fontmap *fcur;
  int twobyte;

  func = (char *)argv[1];
  size = (*(int *)(argv[3])) / 72.0 * 25.4;
  font = (char *)(argv[4]);

  if (_getobj(obj, "_local", inst, &mxlocal))
    return 1;

  if (strcmp0(func, "_charascent") == 0) {
    height = TRUE;
  } else {
    height = FALSE;
  }

  fcur = Mxlocal->fontmaproot;
  twobyte = FALSE;

  while (fcur) {
    if (strcmp(font, fcur->fontalias) == 0) {
      twobyte = fcur->twobyte;
      break;
    }
    fcur = fcur->next;
  }

  cashpos = mxloadfont(font, FALSE);

  if (cashpos < 0) {
    if (height) {
      *(int *) rval = nround(size * 0.562);
    } else {
      *(int *) rval = nround(size * 0.250);
    }
  }

  draw_str(NULL, "A", cashpos, size, 0, NULL, &ascent, &descent);

  if (height) {
    *(int *)rval = mxp2d(ascent);
  } else {
    *(int *)rval = mxp2d(descent);
  }

  /*

  if (cashpos != -1) {
    fontstruct = (mxlocal->font[cashpos]).fontstruct;    
    if (fontstruct) {
      if (twobyte) {
        if (height) {
	  *(int *)rval = nround(size * 0.791);
	} else {
	  *(int *)rval = nround(size * 0.250);
	}
      } else {
        if {
	  (height) *(int *)rval = nround(size * 0.562);
	} else {
	  *(int *)rval = nround(size * 0.250);
	}
      }
    } else {
      if (height) {
	*(int *) rval = mxp2d(fontstruct->ascent);
      } else {
	*(int *) rval = mxp2d(fontstruct->descent);
      }
    }
  } else {
    if (height) {
      *(int *) rval = nround(size * 0.562);
    } else {
      *(int *) rval = nround(size * 0.250);
    }
  }
  */
  return 0;
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
  {"_output", NVFUNC, 0, mx_output, NULL, 0},
  {"_charwidth", NIFUNC, 0, mx_charwidth, NULL, 0},
  {"_charascent", NIFUNC, 0, mx_charheight, NULL, 0},
  {"_chardescent", NIFUNC, 0, mx_charheight, NULL, 0},
  {"_evloop", NVFUNC, 0, mx_evloop, NULL, 0},
  {"_local", NPOINTER, 0, NULL, NULL, 0},
};

#define TBLNUM (sizeof(gtkmenu) / sizeof(*gtkmenu))

void *
addmenu()
{
  return addobject(NAME, ALIAS, PARENT, NVERSION, TBLNUM, gtkmenu, ERRNUM,
		   menuerrorlist, NULL, NULL);
}

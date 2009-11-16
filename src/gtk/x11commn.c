/* 
 * $Id: x11commn.c,v 1.55 2009/11/16 09:13:05 hito Exp $
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

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <limits.h>
#include <fcntl.h>
#include <errno.h>

#include "dir_defs.h"
#include "ngraph.h"
#include "object.h"
#include "ioutil.h"
#include "shell.h"
#include "nstring.h"
#include "jnstring.h"
#include "odraw.h"
#include "nconfig.h"

#include "main.h"
#include "x11gui.h"
#include "x11graph.h"
#include "x11dialg.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11commn.h"
#include "x11info.h"

#define MESSAGE_BUF_SIZE 4096
#define COMMENT_BUF_SIZE 1024

static GtkWidget *ProgressDiaog = NULL;
static GtkProgressBar *ProgressBar, *ProgressBar2;
static unsigned int SaveCursor;

static void AddNgpFileList(char *file);
static void ToFullPath(void);
static void ToBasename(void);
static void ToRalativePath(void);

void
OpenGRA(void)
{
  unsigned int j;
  int i, id, otherGC;
  char *device, *name, *name_str = "viewer";
  struct narray *drawrable;

  if ((Menulocal.GRAinst =
       chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid)) != NULL)
    return;

  /* search closed gra object */
  if (chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid) == NULL)
    CloseGRA();

  for (i = chkobjlastinst(Menulocal.GRAobj); i >= 0; i--) {
    getobj(Menulocal.GRAobj, "GC", i, 0, NULL, &otherGC);
    if (otherGC < 0)
      break;
  }

  if (i == -1) {
    /* closed gra object is not found. generate new gra object */

    id = newobj(Menulocal.GRAobj);
    Menulocal.GRAinst = chkobjinst(Menulocal.GRAobj, id);
    _getobj(Menulocal.GRAobj, "oid", Menulocal.GRAinst, &(Menulocal.GRAoid));

    /* set page settings */
    putobj(Menulocal.GRAobj, "paper_width", id, &(Menulocal.PaperWidth));
    putobj(Menulocal.GRAobj, "paper_height", id, &(Menulocal.PaperHeight));
    putobj(Menulocal.GRAobj, "left_margin", id, &(Menulocal.LeftMargin));
    putobj(Menulocal.GRAobj, "top_margin", id, &(Menulocal.TopMargin));
    putobj(Menulocal.GRAobj, "zoom", id, &(Menulocal.PaperZoom));
    CheckPage();
  } else {
    /* find closed gra object */

    id = i;
    Menulocal.GRAinst = chkobjinst(Menulocal.GRAobj, id);
    _getobj(Menulocal.GRAobj, "oid", Menulocal.GRAinst, &(Menulocal.GRAoid));

    /* get page settings */
    CheckPage();
  }

  if (arraynum(&(Menulocal.drawrable)) > 0) {
    drawrable = arraynew(sizeof(char *));
    for (j = 0; j < arraynum(&(Menulocal.drawrable)); j++) {
      arrayadd2(drawrable, (char **) arraynget(&(Menulocal.drawrable), j));
    }
  } else {
    drawrable = NULL;
  }
  putobj(Menulocal.GRAobj, "draw_obj", id, drawrable);
  device = g_strdup("menu:0");
  putobj(Menulocal.GRAobj, "device", id, device);
  name = g_strdup(name_str);
  putobj(Menulocal.GRAobj, "name", id, name);
  getobj(Menulocal.GRAobj, "open", id, 0, NULL, &(Menulocal.GC));
}

void
CheckPage(void)
{
  _getobj(Menulocal.GRAobj, "paper_width", Menulocal.GRAinst,
	  &(Menulocal.PaperWidth));
  _getobj(Menulocal.GRAobj, "paper_height", Menulocal.GRAinst,
	  &(Menulocal.PaperHeight));
  _getobj(Menulocal.GRAobj, "left_margin", Menulocal.GRAinst,
	  &(Menulocal.LeftMargin));
  _getobj(Menulocal.GRAobj, "top_margin", Menulocal.GRAinst,
	  &(Menulocal.TopMargin));
  _getobj(Menulocal.GRAobj, "zoom", Menulocal.GRAinst,
	  &(Menulocal.PaperZoom));

  set_paper_type(Menulocal.PaperWidth, Menulocal.PaperHeight);
}

static int
ValidGRA(void)
{
  int id;
  struct objlist *graobj;
  char *inst;

  if ((graobj = chkobject("gra")) == NULL)
    return -1;
  id = -1;
  if ((inst = chkobjinstoid(graobj, Menulocal.GRAoid)) != NULL)
    _getobj(graobj, "id", inst, &id);
  if (id == -1)
    id = chkobjlastinst(graobj);
  return id;
}

void
CloseGRA(void)
{
  int id;

  if ((Menulocal.GRAinst =
       chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid)) == NULL)
    return;

  if (chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid) != NULL) {
    _getobj(Menulocal.GRAobj, "id", Menulocal.GRAinst, &id);
    exeobj(Menulocal.GRAobj, "close", id, 0, NULL);
    delobj(Menulocal.GRAobj, id);
    Menulocal.GRAinst = NULL;
    Menulocal.GRAoid = -1;
  }
}

void
ChangeGRA(void)
{
  int i, otherGC;

  /* search for closed gra object */
  for (i = chkobjlastinst(Menulocal.GRAobj); i >= 0; i--) {
    getobj(Menulocal.GRAobj, "GC", i, 0, NULL, &otherGC);
    if (otherGC < 0)
      break;
  }

  if (i == -1) {
    /* closed gra is not find. maintain present gra object */
    if ((Menulocal.GRAinst =
	 chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid)) == NULL) {
      ChangePage();
    }
  } else {
    /* use closed gra object */
    ChangePage();
  }

  CheckPage();
}

void
SetPageSettingsToGRA(void)
{
  int i, j, num, otherGC, id;
  struct objlist *obj;
  char *inst;
  struct narray *drawrable;

  if ((obj = chkobject("gra")) == NULL)
    return;

  for (i = chkobjlastinst(obj); i >= 0; i--) {
    getobj(obj, "GC", i, 0, NULL, &otherGC);
    if (otherGC < 0)
      break;
  }

  if (i >= 0) {
    id = i;
    inst = chkobjinst(obj, id);

    putobj(obj, "paper_width", id, &(Menulocal.PaperWidth));
    putobj(obj, "paper_height", id, &(Menulocal.PaperHeight));
    putobj(obj, "left_margin", id, &(Menulocal.LeftMargin));
    putobj(obj, "top_margin", id, &(Menulocal.TopMargin));
    putobj(obj, "zoom", id, &(Menulocal.PaperZoom));

    _getobj(obj, "draw_obj", inst, &drawrable);

    arrayfree2(drawrable);

    drawrable = arraynew(sizeof(char *));

    num = arraynum(&(Menulocal.drawrable));

    for (j = 0; j < num; j++)
      arrayadd2(drawrable, (char **) arraynget(&(Menulocal.drawrable), j));

    _putobj(obj, "draw_obj", inst, &drawrable);
  }
}

void
GetPageSettingsFromGRA(void)
{
  int i, j, num, otherGC, id;
  struct objlist *obj;
  char *inst;
  struct narray *drawrable;

  if ((obj = chkobject("gra")) == NULL)
    return;

  for (i = chkobjlastinst(obj); i >= 0; i--) {
    getobj(obj, "GC", i, 0, NULL, &otherGC);
    if (otherGC < 0)
      break;
  }

  if (i >= 0) {
    id = i;
    inst = chkobjinst(obj, id);
    CheckPage();
    _getobj(obj, "draw_obj", inst, &drawrable);
    arraydel2(&(Menulocal.drawrable));
    num = arraynum(drawrable);

    for (j = 0; j < num; j++) {
      arrayadd2(&(Menulocal.drawrable), (char **) arraynget(drawrable, j));
    }
  }

  ChangeGRA();
}

static void
AxisDel2(int id)
{
  struct objlist *obj, *aobj;
  int i, spc, aid1 = 0, aid2 = 0;
  char *axis, *axis2;
  struct narray iarray;
  int anum;
  char *inst;
  char *axisx, *axisy;
  int findx, findy;

  if ((obj = getobject("axisgrid")) != NULL) {
    for (i = chkobjlastinst(obj); i >= 0; i--) {
      if ((inst = chkobjinst(obj, i)) != NULL) {
	_getobj(obj, "axis_x", inst, &axisx);
	_getobj(obj, "axis_y", inst, &axisy);
	findx = findy = FALSE;
	if (axisx != NULL) {
	  arrayinit(&iarray, sizeof(int));
	  if (!getobjilist(axisx, &aobj, &iarray, FALSE, NULL)) {
	    anum = arraynum(&iarray);
	    if (anum > 0) {
	      aid1 = *(int *) arraylast(&iarray);
	      if (aid1 == id)
		findx = TRUE;
	    }
	  }
	  arraydel(&iarray);
	}
	if (axisy != NULL) {
	  arrayinit(&iarray, sizeof(int));
	  if (!getobjilist(axisy, &aobj, &iarray, FALSE, NULL)) {
	    anum = arraynum(&iarray);
	    if (anum > 0) {
	      aid1 = *(int *) arraylast(&iarray);
	      if (aid1 == id)
		findy = TRUE;
	    }
	  }
	  arraydel(&iarray);
	}
	if (findx || findy)
	  delobj(obj, i);
      }
    }
  }
  if ((obj = getobject("file")) == NULL)
    return;
  for (i = 0; i <= chkobjlastinst(obj); i++) {
    if ((getobj(obj, "axis_x", i, 0, NULL, &axis) >= 0) && (axis != NULL)) {
      arrayinit(&iarray, sizeof(int));
      if (!getobjilist(axis, &aobj, &iarray, FALSE, &spc)) {
	anum = arraynum(&iarray);
	if ((anum > 0) && (spc == 1)) {
	  aid1 = *(int *) arraylast(&iarray);
	  if (aid1 > id)
	    aid1--;
	} else
	  aid1 = -1;
	arraydel(&iarray);
      }
    }
    if ((getobj(obj, "axis_y", i, 0, NULL, &axis) >= 0) && (axis != NULL)) {
      arrayinit(&iarray, sizeof(int));
      if (!getobjilist(axis, &aobj, &iarray, FALSE, &spc)) {
	anum = arraynum(&iarray);
	if ((anum > 0) && (spc == 1)) {
	  aid2 = *(int *) arraylast(&iarray);
	  if (aid2 > id)
	    aid2--;
	} else
	  aid2 = -1;
	arraydel(&iarray);
      }
    }
    if ((aid1 >= 0) && (aid2 >= 0)) {
      if (aid1 == aid2)
	aid2 = aid1 + 1;
    }
    if ((aid1 >= 0) && (aid2 >= 0)) {
      int len;

      len = strlen(chkobjectname(aobj)) + 10;
      axis2 = g_malloc(len);
      if (axis2) {
	snprintf(axis2, len, "%s:%d", chkobjectname(aobj), aid1);
	putobj(obj, "axis_x", i, axis2);
      }
      axis2 = g_malloc(len);
      if (axis2) {
	snprintf(axis2, len, "%s:%d", chkobjectname(aobj), aid2);
	putobj(obj, "axis_y", i, axis2);
      }
    }
  }
}

void
AxisDel(int id)
{
  struct objlist *obj;
  int i, lastinst, *id_array, n;
  char *group, *group2;
  char type;
  char *inst, *inst2;
  char group3[20];

  obj = chkobject("axis");
  if (obj == NULL)
    return;

  inst = chkobjinst(obj, id);
  if (inst == NULL)
    return;

  _getobj(obj, "group", inst, &group);
  if (group == NULL || group[0] == 'a') {
    AxisDel2(id);
    delobj(obj, id);
    return;
  }

  lastinst = chkobjlastinst(obj);
  type = group[0];
  strncpy(group3, group, sizeof(group3) - 1);
  group3[sizeof(group3) - 1] = '\0';

  id_array = g_malloc(sizeof(*id_array) * (lastinst + 1));
  if (id_array == NULL)
    return;

  n = 0;
  for (i = lastinst; i >= 0; i--) {
    inst2 = chkobjinst(obj, i);
    _getobj(obj, "group", inst2, &group2);
    if (group2 && group2[0] == type) {
      if (strcmp(group3 + 2, group2 + 2) == 0) {
	AxisDel2(i);
	id_array[n] = i;
	n++;
      }
    }
  }

  for (i = 0; i < n; i++) {
    delobj(obj, id_array[i]);
  }
  g_free(id_array);
}

static void
axis_move_each_obj(char *axis_str, int i, struct objlist *obj, int id1, int id2)
{
  struct objlist *aobj;
  int spc, aid;
  char *axis, *axis2;
  struct narray iarray;
  int anum, len;

  if (getobj(obj, axis_str, i, 0, NULL, &axis) < 0 || axis == NULL)
    return;

  arrayinit(&iarray, sizeof(int));
  if (getobjilist(axis, &aobj, &iarray, FALSE, &spc))
    return;

  anum = arraynum(&iarray);
  if (anum > 0 && spc == 1) {
    aid = *(int *) arraylast(&iarray);
    if (aid == id1) {
      aid = id2;
    } else {
      if (aid > id1)
	aid--;
      if (aid >= id2)
	aid++;
    }
    len = strlen(chkobjectname(aobj)) + 10;
    axis2 = (char *) g_malloc(len);
    if (axis2) {
      snprintf(axis2, len, "%s:%d", chkobjectname(aobj), aid);
      putobj(obj, axis_str, i, axis2);
    }
  }
  arraydel(&iarray);
}

void
AxisMove(int id1, int id2)
{
  struct objlist *obj;
  int i;

  if ((obj = getobject("file")) == NULL)
    return;

  for (i = 0; i <= chkobjlastinst(obj); i++) {
    axis_move_each_obj("axis_x", i, obj, id1, id2);
    axis_move_each_obj("axis_y", i, obj, id1, id2);
  }
}

static void
AxisNameToGroup(void)		/* this function may exist for compatibility with older version. */
{
  int idx, idy, idu, idr;
  int findX, findY, findU, findR, graphtype;
  int id, id2, num;
  struct objlist *obj;
  char *name, *name2, *gp;
  struct narray group, iarray;
  int a, j, anum;
  char *argv[2];
  int *data;
  char *inst;


  if ((obj = (struct objlist *) chkobject("axis")) == NULL)
    return;
  num = chkobjlastinst(obj);
  arrayinit(&iarray, sizeof(int));
  for (id = 0; id <= num; id++) {
    anum = arraynum(&iarray);
    data = (int *) arraydata(&iarray);
    for (j = 0; j < anum; j++)
      if (data[j] == id)
	break;
    inst = chkobjinst(obj, id);
    _getobj(obj, "name", inst, &name);
    _getobj(obj, "group", inst, &gp);
    if ((j == anum) && (gp[0] == 'a')) {
      findX = findY = findU = findR = FALSE;
      graphtype = -1;
      if (name != NULL) {
	if (strncmp(name, "sectionX", 8) == 0) {
	  graphtype = 0;
	  findX = TRUE;
	  idx = id;
	} else if (strncmp(name, "sectionY", 8) == 0) {
	  graphtype = 0;
	  findY = TRUE;
	  idy = id;
	} else if (strncmp(name, "sectionU", 8) == 0) {
	  graphtype = 0;
	  findU = TRUE;
	  idu = id;
	} else if (strncmp(name, "sectionR", 8) == 0) {
	  graphtype = 0;
	  findR = TRUE;
	  idr = id;
	} else if (strncmp(name, "frameX", 6) == 0) {
	  graphtype = 1;
	  findX = TRUE;
	  idx = id;
	} else if (strncmp(name, "frameY", 6) == 0) {
	  graphtype = 1;
	  findY = TRUE;
	  idy = id;
	} else if (strncmp(name, "frameU", 6) == 0) {
	  graphtype = 1;
	  findU = TRUE;
	  idu = id;
	} else if (strncmp(name, "frameR", 6) == 0) {
	  graphtype = 1;
	  findR = TRUE;
	  idr = id;
	} else if (strncmp(name, "crossX", 6) == 0) {
	  graphtype = 2;
	  findX = TRUE;
	  idx = id;
	} else if (strncmp(name, "crossY", 6) == 0) {
	  graphtype = 2;
	  findY = TRUE;
	  idy = id;
	}
      }
      for (id2 = id + 1; id2 <= num; id2++) {
	for (j = 0; j < anum; j++)
	  if (data[j] == id2)
	    break;
	inst = chkobjinst(obj, id2);
	_getobj(obj, "name", inst, &name2);
	_getobj(obj, "group", inst, &gp);
	if ((j == anum) && (gp[0] == 'a')) {
	  if (graphtype == 0) {
	    if (name2 != NULL) {
	      if ((strncmp(name2, "sectionX", 8) == 0)
		  && (strcmp(name + 8, name2 + 8) == 0)) {
		findX = TRUE;
		idx = id2;
	      } else if ((strncmp(name2, "sectionY", 8) == 0)
			 && (strcmp(name + 8, name2 + 8) == 0)) {
		findY = TRUE;
		idy = id2;
	      } else if ((strncmp(name2, "sectionU", 8) == 0)
			 && (strcmp(name + 8, name2 + 8) == 0)) {
		findU = TRUE;
		idu = id2;
	      } else if ((strncmp(name2, "sectionR", 8) == 0)
			 && (strcmp(name + 8, name2 + 8) == 0)) {
		findR = TRUE;
		idr = id2;
	      }
	    }
	  } else if (graphtype == 1) {
	    if (name2 != NULL) {
	      if ((strncmp(name2, "frameX", 6) == 0)
		  && (strcmp(name + 6, name2 + 6) == 0)) {
		findX = TRUE;
		idx = id2;
	      } else if ((strncmp(name2, "frameY", 6) == 0)
			 && (strcmp(name + 6, name2 + 6) == 0)) {
		findY = TRUE;
		idy = id2;
	      } else if ((strncmp(name2, "frameU", 6) == 0)
			 && (strcmp(name + 6, name2 + 6) == 0)) {
		findU = TRUE;
		idu = id2;
	      } else if ((strncmp(name2, "frameR", 6) == 0)
			 && (strcmp(name + 6, name2 + 6) == 0)) {
		findR = TRUE;
		idr = id2;
	      }
	    }
	  } else if (graphtype == 2) {
	    if (name2 != NULL) {
	      if ((strncmp(name2, "crossX", 6) == 0)
		  && (strcmp(name + 6, name2 + 6) == 0)) {
		findX = TRUE;
		idx = id2;
	      } else if ((strncmp(name2, "crossY", 6) == 0)
			 && (strcmp(name + 6, name2 + 6) == 0)) {
		findY = TRUE;
		idy = id2;
	      }
	    }
	  }
	}
      }
      if ((graphtype == 0) && findX && findY && findU && findR) {
	arrayinit(&group, sizeof(int));
	a = 2;
	arrayadd(&group, &a);
	arrayadd(&group, &idx);
	arrayadd(&group, &idy);
	arrayadd(&group, &idu);
	arrayadd(&group, &idr);
	argv[0] = (char *) &group;
	argv[1] = NULL;
	exeobj(obj, "grouping", id, 1, argv);
	arraydel(&group);
	arrayadd(&iarray, &idx);
	arrayadd(&iarray, &idy);
	arrayadd(&iarray, &idu);
	arrayadd(&iarray, &idr);
      } else if ((graphtype == 1) && findX && findY && findU && findR) {
	arrayinit(&group, sizeof(int));
	a = 1;
	arrayadd(&group, &a);
	arrayadd(&group, &idx);
	arrayadd(&group, &idy);
	arrayadd(&group, &idu);
	arrayadd(&group, &idr);
	argv[0] = (char *) &group;
	argv[1] = NULL;
	exeobj(obj, "grouping", id, 1, argv);
	arraydel(&group);
	arrayadd(&iarray, &idx);
	arrayadd(&iarray, &idy);
	arrayadd(&iarray, &idu);
	arrayadd(&iarray, &idr);
      } else if ((graphtype == 2) && findX && findY) {
	arrayinit(&group, sizeof(int));
	a = 3;
	arrayadd(&group, &a);
	arrayadd(&group, &idx);
	arrayadd(&group, &idy);
	argv[0] = (char *) &group;
	argv[1] = NULL;
	exeobj(obj, "grouping", id, 1, argv);
	arraydel(&group);
	arrayadd(&iarray, &idx);
	arrayadd(&iarray, &idy);
      }
    }
  }
  arraydel(&iarray);
}

void
FitDel(struct objlist *obj, int id)
{
  char *fit;
  struct objlist *fitobj;
  int fitid, idnum;
  struct narray iarray;

  if ((getobj(obj, "fit", id, 0, NULL, &fit) >= 0) && (fit != NULL)) {
    arrayinit(&iarray, sizeof(int));
    if (getobjilist(fit, &fitobj, &iarray, FALSE, NULL) == 0) {
      idnum = arraynum(&iarray);
      if (idnum >= 1) {
	fitid = *(int *) arraylast(&iarray);
	delobj(fitobj, fitid);
      }
    }
    arraydel(&iarray);
  }
}

void
FitCopy(struct objlist *obj, int did, int sid)
{
  char *fit;
  struct objlist *fitobj;
  int fitid;
  struct narray iarray;
  struct narray iarray2;
  int id2, idnum, idnum2;
  char *field[] = {"name", "equation", NULL};
  int fitoid;
  char *inst;

  if ((getobj(obj, "fit", sid, 0, NULL, &fit) >= 0) && (fit != NULL)) {
    arrayinit(&iarray, sizeof(int));
    if (getobjilist(fit, &fitobj, &iarray, FALSE, NULL) == 0) {
      idnum = arraynum(&iarray);
      if (idnum >= 1) {
	fitid = *(int *) arraylast(&iarray);
	if ((getobj(obj, "fit", did, 0, NULL, &fit) >= 0)
	    && (fit != NULL)) {
	  arrayinit(&iarray2, sizeof(int));
	  if (getobjilist(fit, &fitobj, &iarray2, FALSE, NULL) == 0) {
	    idnum2 = arraynum(&iarray2);
	    if (idnum2 >= 1) {
	      id2 = *(int *) arraylast(&iarray2);
	    } else
	      id2 = newobj(fitobj);
	  } else
	    id2 = newobj(fitobj);
	  arraydel(&iarray2);
	} else
	  id2 = newobj(fitobj);
	if (id2 >= 0) {
	  copy_obj_field(fitobj, id2, fitid, field);
	  inst = getobjinst(fitobj, id2);
	  _getobj(fitobj, "oid", inst, &fitoid);
	  if ((fit = mkobjlist(fitobj, NULL, fitoid, NULL, TRUE)) != NULL) {
	    if (putobj(obj, "fit", did, fit) == -1)
	      g_free(fit);
	  }
	}
      }
    }
    arraydel(&iarray);
  }
}

void
FitClear(void)
{
  struct objlist *obj, *fitobj;
  int i, anum, id, hidden;
  char *fit;
  struct narray iarray;

  if (Menulock || GlobalLock)
    return;
  if ((obj = chkobject("file")) == NULL)
    return;
  if ((fitobj = chkobject("fit")) == NULL)
    return;
  for (i = 0; i <= chkobjlastinst(obj); i++) {
    getobj(obj, "fit", i, 0, NULL, &fit);
    if (fit != NULL) {
      arrayinit(&iarray, sizeof(int));
      if (getobjilist(fit, &fitobj, &iarray, FALSE, NULL) == 0) {
	anum = arraynum(&iarray);
	if (anum >= 1) {
	  id = *(int *) arraylast(&iarray);
	  getobj(obj, "hidden", i, 0, NULL, &hidden);
	  if (!hidden)
	    putobj(fitobj, "equation", id, NULL);
	}
      }
      arraydel(&iarray);
    }
  }
}

void
DeleteDrawable(void)
{
  struct objlist *fileobj, *drawobj;
  int i;

  if ((fileobj = chkobject("file")) != NULL) {
    for (i = 0; i <= chkobjlastinst(fileobj); i++)
      FitDel(fileobj, i);
  }
  if ((drawobj = chkobject("draw")) != NULL)
    delchildobj(drawobj);
}

static void
SaveParent(HANDLE hFile, struct objlist *parent, int storedata,
	   int storemerge)
{
  struct objlist *ocur;
  int i, j, instnum;
  char *s;
  char *inst, *fname, *fname2;

  ocur = chkobjroot();
  while (ocur != NULL) {
    if (chkobjparent(ocur) == parent) {
      if ((instnum = chkobjlastinst(ocur)) != -1) {
	for (i = 0; i <= instnum; i++) {
	  getobj(ocur, "save", i, 0, NULL, &s);
	  nwrite(hFile, s, strlen(s));
	  nwrite(hFile, "\n", 1);
	  if (storedata && (ocur == chkobject("file"))) {
	    getobj(ocur, "file", i, 0, NULL, &fname);
	    if (fname != NULL) {
	      for (j = i - 1; j >= 0; j--) {
		getobj(ocur, "file", j, 0, NULL, &fname2);
		if ((fname2 != NULL)
		    && (strcmp0(fname, fname2) == 0))
		  break;
	      }
	      inst = chkobjinst(ocur, i);
	      if (j == -1) {
		while (_exeobj(ocur, "store_data", inst, 0, NULL) == 0) {
		  _getobj(ocur, "store_data", inst, &s);
		  nwrite(hFile, s, strlen(s));
		  nwrite(hFile, "\n", 1);
		}
	      } else {
		while (_exeobj(ocur, "store_dummy", inst, 0, NULL) == 0) {
		  _getobj(ocur, "store_dummy", inst, &s);
		  nwrite(hFile, s, strlen(s));
		  nwrite(hFile, "\n", 1);
		}
	      }
	    }
	  }
	  if (storemerge && (ocur == chkobject("merge"))) {
	    getobj(ocur, "file", i, 0, NULL, &fname);
	    if (fname != NULL) {
	      for (j = i - 1; j >= 0; j--) {
		getobj(ocur, "file", j, 0, NULL, &fname2);
		if ((fname2 != NULL)
		    && (strcmp0(fname, fname2) == 0))
		  break;
	      }
	      inst = chkobjinst(ocur, i);
	      if (j == -1) {
		while (_exeobj(ocur, "store_data", inst, 0, NULL) == 0) {
		  _getobj(ocur, "store_data", inst, &s);
		  nwrite(hFile, s, strlen(s));
		  nwrite(hFile, "\n", 1);
		}
	      } else {
		while (_exeobj(ocur, "store_dummy", inst, 0, NULL) == 0) {
		  _getobj(ocur, "store_dummy", inst, &s);
		  nwrite(hFile, s, strlen(s));
		  nwrite(hFile, "\n", 1);
		}
	      }
	    }
	  }
	}
      }
      SaveParent(hFile, ocur, storedata, storemerge);
    }
    ocur = ocur->next;
  }
}

int
SaveDrawrable(char *name, int storedata, int storemerge)
{
  HANDLE hFile;
  struct objlist *sysobj, *drawobj, *graobj;
  int id, len, error;
  char *arg[2];
  struct narray sarray;
  char *inst, *ver, *sysname, *s, *opt, comment[COMMENT_BUF_SIZE];

  error = FALSE;

  hFile = nopen(name, O_CREAT | O_TRUNC | O_RDWR, NFMODE_NORMAL_FILE);
  if (hFile < 0) {
    error = TRUE;
  } else {
    sysobj = chkobject("system");
    inst = chkobjinst(sysobj, 0);
    _getobj(sysobj, "name", inst, &sysname);
    _getobj(sysobj, "version", inst, &ver);

    len = snprintf(comment, sizeof(comment),
		   "#!ngraph\n#%%creator: %s %s \n#%%version: %s\n",
		   sysname, PLATFORM, ver);
    if (nwrite(hFile, comment, len) != len)
      error = TRUE;

    if ((drawobj = chkobject("draw")) != NULL) {
      SaveParent(hFile, drawobj, storedata, storemerge);
      if ((graobj = chkobject("gra")) != NULL) {
	id = ValidGRA();
	if (id != -1) {
	  arrayinit(&sarray, sizeof(char *));
	  opt = "device";
	  arrayadd(&sarray, &opt);
	  arg[0] = (char *) &sarray;
	  arg[1] = NULL;
	  getobj(graobj, "save", id, 1, arg, &s);
	  arraydel(&sarray);
	  len = strlen(s);
	  if (nwrite(hFile, s, len) != len)
	    error = TRUE;
	} else {
	  error = TRUE;
	}
      }
    }
    nclose(hFile);
  }

  if (error)
    ErrorMessage();

  return error;
}

int
check_overwrite(GtkWidget *parent, const char *filename)
{
  int r;
  char *buf;

  if (filename == NULL || access(filename, W_OK))
    return 0;

  buf = g_strdup_printf(_("`%s'\n\nOverwrite existing file?"), filename);
  r = MessageBox(parent, buf, "Driver", MB_YESNO);
  g_free(buf);

  return r != IDYES;
}

static int
get_save_opt(int *sdata, int *smerge, int *path)
{
  int ret, fnum, mnum, i;
  struct objlist *fobj, *mobj;

  *path = SAVE_PATH_UNCHANGE;
  *sdata = FALSE;
  *smerge = FALSE;

  fobj = chkobject("file");
  mobj = chkobject("merge");

  fnum = (fobj) ? chkobjlastinst(fobj) : -1;
  mnum = (mobj) ? chkobjlastinst(mobj) : -1;

  if (fnum < 0 && mnum < 0)
    return IDCANCEL;

  SaveDialog(&DlgSave, sdata, smerge);
  ret = DialogExecute(TopLevel, &DlgSave);
  if (ret != IDOK)
    return IDCANCEL;

  *path = DlgSave.Path;
  for (i = 0; i <= fnum; i++) {
    putobj(fobj, "save_path", i, path);
  }

  for (i = 0; i <= mnum; i++) {
    putobj(mobj, "save_path", i, path);
  }

  return IDOK;
}

int
GraphSave(int overwrite)
{
  char mes[256];
  int path;
  int sdata, smerge;
  int ret;
  char *ext;
  char *initfil;
  char *file, *prev_wd, *current_wd;

  if (NgraphApp.FileName != NULL) {
    initfil = NgraphApp.FileName;
    if ((ext = getextention(initfil)) != NULL) {
      if ((strcmp0(ext, "PRM") == 0) || (strcmp0(ext, "prm") == 0))
	strcpy(ext, "ngp");
    }
  } else {
    initfil = NULL;
    overwrite = FALSE;
  }
  prev_wd = current_wd = NULL;
  if ((initfil == NULL) || (! overwrite || (access(initfil, 04) == -1))) {
    prev_wd = ngetcwd();
    ret = nGetSaveFileName(TopLevel, _("Save NGP file"), "ngp",
			   &(Menulocal.graphloaddir), initfil,
			   &file, overwrite, Menulocal.changedirectory);
    current_wd = ngetcwd();
    if (prev_wd && current_wd && strcmp(prev_wd, current_wd) == 0) {
      g_free(prev_wd);
      g_free(current_wd);
      prev_wd = NULL;
      current_wd = NULL;
    }
  } else {
    file = g_strdup(initfil);
    if (file == NULL)
      return IDCANCEL;
    ret = IDOK;
  }

  if (ret == IDOK) {
    if (prev_wd && chdir(prev_wd)) {
      ErrorMessage();
    }

    ret = get_save_opt(&sdata, &smerge, &path);
    if (ret == IDOK) {
      snprintf(mes, sizeof(mes), _("Saving `%.128s'."), file);
      SetStatusBar(mes);
      if(SaveDrawrable(file, sdata, smerge)) {
	ret = IDCANCEL;
      } else {
	switch (path) {
	case SAVE_PATH_BASE:
	  ToBasename();
	  break;
	case SAVE_PATH_RELATIVE:
	  ToRalativePath();
	  break;
	case SAVE_PATH_FULL:
	  ToFullPath();
	  break;
	}
	changefilename(file);
	AddNgpFileList(file);
	SetFileName(file);
	reset_graph_modified();
      }
      ResetStatusBar();
    }
    g_free(file);

    if (current_wd && chdir(current_wd)) {
      ErrorMessage();
    }
  }

  g_free(prev_wd);
  g_free(current_wd);

  return ret;
}

static void
change_filename(char * (*func)(char *))
{
  struct objlist *obj;
  int i;
  unsigned int j;
  char *file, *file2, *objname[] = {"file", "merge"};

  for (j = 0; j < sizeof(objname) / sizeof(*objname); j++) {
    obj = chkobject(objname[j]);
    if (obj == NULL)
      continue;

    for (i = 0; i <= chkobjlastinst(obj); i++) {
      getobj(obj, "file", i, 0, NULL, &file);
      if (file == NULL)
	continue;

      file2 = func(file);
      if (file2 == NULL)
	return;

      if (strcmp(file, file2) == 0) {
	g_free(file2);
	continue;
      }

      set_graph_modified();
      putobj(obj, "file", i, file2);
    }
  }
}

static void
ToFullPath(void)
{
  change_filename(getfullpath);
}

static void
ToRalativePath(void)
{
  change_filename(getrelativepath);
}

static char *
get_basename(char *file)
{
  char *ptr;

  ptr = g_path_get_basename(file);
  if (ptr == NULL)
    return NULL;

  return ptr;
}

static void
ToBasename(void)
{
  change_filename(get_basename);
}


void
LoadNgpFile(char *file, int ignorepath, int expand, char *exdir,
	    int console, char *option)
{
  struct objlist *sys;
  char *expanddir;
  struct objlist *obj, *aobj;
  char *name;
  int i, r, newid, allocnow = FALSE, tmp;
  char *s;
  int len;
  char *argv[2];
  struct narray sarray;
  char mes[256];
  int sec;
  char *inst;
  struct objlist *robj;
  int idn;

  sys = chkobject("system");
  if (sys == NULL)
    return;

  expanddir = g_strdup(exdir);

  if (expanddir == NULL)
    return;

  putobj(sys, "expand_dir", 0, expanddir);
  putobj(sys, "expand_file", 0, &expand);

  tmp = FALSE;
  putobj(sys, "ignore_path", 0, &tmp);

  obj = chkobject("shell");
  if (obj == NULL)
    return;

  newid = newobj(obj);
  if (newid < 0)
    return;

  inst = chkobjinst(obj, newid);
  arrayinit(&sarray, sizeof(char *));
  while ((s = getitok2(&option, &len, " \t")) != NULL) {
    if (arrayadd(&sarray, &s) == NULL) {
      g_free(s);
      arraydel2(&sarray);
      return;
    }
  }

  name = g_strdup(file);

  if (name == NULL) {
    arraydel2(&sarray);
    return;
  }

  changefilename(name);

  if (arrayadd(&sarray, &name) == NULL) {
    g_free(name);
    arraydel2(&sarray);
    return;
  }

  DeleteDrawable();

  if (console)
    allocnow = AllocConsole();

  sec = TRUE;
  argv[0] = (char *) &sec;
  argv[1] = NULL;
  _exeobj(obj, "security", inst, 1, argv);

  argv[0] = (char *) &sarray;
  argv[1] = NULL;
  snprintf(mes, sizeof(mes), _("Loading `%.128s'."), name);
  SetStatusBar(mes);

  menu_lock(TRUE);
  idn = getobjtblpos(Menulocal.obj, "_evloop", &robj);
  registerevloop(chkobjectname(Menulocal.obj), "_evloop", robj, idn, Menulocal.inst, NULL);

  r = _exeobj(obj, "shell", inst, 1, argv);

  unregisterevloop(robj, idn, Menulocal.inst);
  menu_lock(FALSE);

  sec = FALSE;
  argv[0] = (char *) &sec;
  argv[1] = NULL;
  _exeobj(obj, "security", inst, 1, argv);

  if (r == 0) {
    if ((aobj = getobject("axis")) != NULL) {
      for (i = 0; i <= chkobjlastinst(aobj); i++)
	exeobj(aobj, "tight", i, 0, NULL);
    }

    if ((aobj = getobject("axisgrid")) != NULL) {
      for (i = 0; i <= chkobjlastinst(aobj); i++)
	exeobj(aobj, "tight", i, 0, NULL);
    }

    SetFileName(file);
    AddNgpFileList(name);
    reset_graph_modified();

    if (ignorepath) {
      ToBasename();
    } else if (Menulocal.expandtofullpath) {
      ToFullPath();
    }
    InfoWinClear();
  }

  AxisNameToGroup();
  ResetStatusBar();
  arraydel2(&sarray);

  if (console)
    FreeConsole(allocnow);

  GetPageSettingsFromGRA();
  UpdateAll();
  delobj(obj, newid);
}

void
LoadPrmFile(char *file)
{
  struct objlist *obj;
  char *name;
  int id;
  char mes[256];

  obj = chkobject("prm");
  if (obj == NULL)
    return;

  id = newobj(obj);
  if (id < 0)
    return;

  name = g_strdup(file);
  if (name == NULL) {
    delobj(obj, id);
    return;
  }
  changefilename(name);
  putobj(obj, "file", id, name);
  PrmDialog(&DlgPrm, obj, id);
  if (DialogExecute(TopLevel, &DlgPrm) == IDOK) {
    snprintf(mes, sizeof(mes), _("Loading `%.128s'."), name);
    SetStatusBar(mes);
    DeleteDrawable();
    exeobj(obj, "load", id, 0, NULL);
    GetPageSettingsFromGRA();
    UpdateAll();
    ResetStatusBar();
    SetFileName(file);
    set_graph_modified();
    InfoWinClear();
  }
  delobj(obj, id);

}

void
FileAutoScale(void)
{
  int anum, room;
  struct objlist *aobj, *aobj2;
  double min, max, inc;
  char *argv2[3];
  char *buf;
  struct objlist *fobj;
  int lastinst;
  int i, j, a, len;
  char *ref;
  struct narray iarray;
  int anum2, aid2;
  char *inst, *group, *refgroup;
  int refother;

  if ((fobj = chkobject("file")) == NULL)
    return;

  lastinst = chkobjlastinst(fobj);
  aobj = chkobject("axis");
  anum = chkobjlastinst(aobj);

  if (lastinst < 0 || aobj == NULL || anum == 0)
    return;

  len = 6 * (lastinst + 1) + 6;
  buf = (char *) g_malloc(len);
  if (buf == NULL) {
    error(NULL, ERRHEAP);
    return;
  }

  j = 0;
  j += snprintf(buf + j, len - j, "file:");
  for (i = 0; i <= lastinst; i++) {
    getobj(fobj, "hidden", i, 0, NULL, &a);
    if (!a)
      j += snprintf(buf + j, len - j, "%d,", i);
  }
  if (buf[j] == ',')
    buf[j] = '\0';
  room = 0;
  argv2[0] = (char *) buf;
  argv2[1] = (char *) &room;
  argv2[2] = NULL;
  for (i = 0; i <= anum; i++) {
    getobj(aobj, "min", i, 0, NULL, &min);
    getobj(aobj, "max", i, 0, NULL, &max);
    getobj(aobj, "inc", i, 0, NULL, &inc);
    getobj(aobj, "group", i, 0, NULL, &group);
    getobj(aobj, "reference", i, 0, NULL, &ref);
    refother = FALSE;
    if (ref != NULL) {
      refother = TRUE;
      arrayinit(&iarray, sizeof(int));
      if (!getobjilist(ref, &aobj2, &iarray, FALSE, NULL)) {
	anum2 = arraynum(&iarray);
	if (anum2 > 0) {
	  aid2 = *(int *) arraylast(&iarray);
	  arraydel(&iarray);
	  inst = getobjinst(aobj2, aid2);
	  if (inst) {
	    _getobj(aobj2, "group", inst, &refgroup);
	    if (refgroup && group && refgroup[0] == group[0]
		&& strcmp(refgroup + 2, group + 2) == 0)
	      refother = FALSE;
	  }
	}
      }
    }
    if (! refother && (min == max || inc == 0)) {
      exeobj(aobj, "auto_scale", i, 2, argv2);
    }
  }
  g_free(buf);
}

void
AdjustAxis(void)
{
  struct objlist *aobj;
  int i, anum;

  if ((aobj = chkobject("axis")) == NULL)
    return;
  anum = chkobjlastinst(aobj);
  for (i = 0; i <= anum; i++)
    exeobj(aobj, "adjust", i, 0, NULL);
}

int
CheckSave(void)
{
  int ret;

  if (get_graph_modified()) {
    ret = MessageBox(TopLevel, _("This graph is modified.\nSave this graph?"),
		     "Modified", MB_YESNOCANCEL);
    if (ret == IDYES) {
      if (GraphSave(TRUE) == IDCANCEL)
	return FALSE;
    } else if (ret != IDNO)
      return FALSE;
  }
  return TRUE;
}

static void
AddNgpFileList(char *file)
{
  char *full_name, *uri;
  GtkRecentData recent_data = {
    NULL,
    AppName,
    "application/x-ngraph",
    AppName,
    "ngraph %f",
    NULL,
    FALSE,
  };

  full_name = getfullpath(file);
  if (full_name == NULL)
    return;

  uri = g_strdup_printf("file://%s", full_name);
  g_free(full_name);
  gtk_recent_manager_add_full(Menulocal.ngpfilelist, uri, &recent_data);
  g_free(uri);
}

void
AddDataFileList(char *file)
{
  int i, j, num;
  char **data;
  char *s;
  struct narray *datafilelist;

  if ((file == NULL) || (file[0] == '\0'))
    return;
  datafilelist = Menulocal.datafilelist;
  num = arraynum(datafilelist);
  data = (char **) arraydata(datafilelist);
  for (i = 0; i < num; i++)
    if (strcmp0(data[i], file) == 0)
      break;
  if (i == num) {
    if (num >= 10)
      arrayndel2(datafilelist, num - 1);
    arrayins2(datafilelist, &file, 0);
  } else {
    s = data[i];
    for (j = i - 1; j >= 0; j--)
      data[j + 1] = data[j];
    data[0] = s;
  }
}

void
SetFileName(char *str)
{
  char *ngp, *name;

  name = g_strdup(str);
  g_free(NgraphApp.FileName);
  if (name == NULL) {
    NgraphApp.FileName = NULL;
    ngp = NULL;
  } else {
    NgraphApp.FileName = getfullpath(name);
    ngp = getfullpath(name);
    g_free(name);
  }
  putobj(Menulocal.obj, "fullpath_ngp", 0, ngp);
}

int
AllocConsole(void)
{
  int allocnow;

  loadstdio(&GtkIOSave);
  allocnow = nallocconsole();
  nforegroundconsole();
  return allocnow;
}

void
FreeConsole(int allocnow)
{
  if (allocnow)
    nfreeconsole();

  putstderr = mgtkputstderr;
  printfstderr = mgtkprintfstderr;
  putstdout = mgtkputstdout;
  printfstdout = mgtkprintfstdout;
  inputyn = mgtkinputyn;
  ndisplaydialog = mgtkdisplaydialog;
  ndisplaystatus = mgtkdisplaystatus;
  ninterrupt = mgtkinterrupt;
}

char *
FileCB(struct objlist *obj, int id)
{
  char *valstr, *file, *s;

  getobj(obj, "file", id, 0, NULL, &file);
  valstr = getbasename(file);
  s = g_strdup_printf("%s", (valstr) ? valstr : "....................");
  if (valstr != NULL) {
    g_free(valstr);
  }

  return s;
}

int
SetFileHidden(void)
{
  struct objlist *fobj;
  int lastinst;
  struct narray farray, ifarray;
  int i, a, num, *array;

  if ((fobj = chkobject("file")) == NULL)
    return 1;
  lastinst = chkobjlastinst(fobj);
  if (lastinst >= 0) {
    arrayinit(&ifarray, sizeof(int));
    for (i = 0; i <= lastinst; i++) {
      getobj(fobj, "hidden", i, 0, NULL, &a);
      if (!a)
	arrayadd(&ifarray, &i);
    }
    SelectDialog(&DlgSelect, fobj, FileCB, (struct narray *) &farray,
		 (struct narray *) &ifarray);
    if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
      a = TRUE;
      for (i = 0; i <= lastinst; i++)
	putobj(fobj, "hidden", i, &a);
      num = arraynum(&farray);
      array = (int *) arraydata(&farray);
      a = FALSE;
      for (i = 0; i < num; i++)
	putobj(fobj, "hidden", array[i], &a);
    } else {
      arraydel(&ifarray);
      arraydel(&farray);
      return 0;
    }
    arraydel(&ifarray);
    arraydel(&farray);
  }
  return 1;
}

int
GetDrawFiles(struct narray *farray)
{
  struct objlist *fobj;
  int lastinst;
  struct narray ifarray;
  int i, a;

  if (farray == NULL)
    return 1;

  fobj = chkobject("file");
  if (fobj == NULL)
    return 1;

  lastinst = chkobjlastinst(fobj);
  if (lastinst < 0)
    return 1;

  arrayinit(&ifarray, sizeof(int));
  for (i = 0; i <= lastinst; i++) {
    getobj(fobj, "hidden", i, 0, NULL, &a);
    if (!a)
      arrayadd(&ifarray, &i);
  }
  SelectDialog(&DlgSelect, fobj, FileCB, farray, &ifarray);
  if (DialogExecute(TopLevel, &DlgSelect) != IDOK) {
    arraydel(&ifarray);
    arraydel(farray);
    return 1;
  }
  arraydel(&ifarray);

  return 0;
}

int
CheckIniFile(void)
{
  int ret;

  ret = writecheckconfig();
  if (ret == 0) {
    MessageBox(TopLevel, _("Ngraph.ini is not found."), "Ngraph.ini", MB_ERROR);
    return FALSE;
  } else if ((ret == -1) || (ret == -3)) {
    MessageBox(TopLevel, _("Ngraph.ini is write protected."), "Ngraph.ini", MB_ERROR);
    return FALSE;
  } else if ((ret == -2) || (ret == 2)) {
    char buf[256];
    snprintf(buf, sizeof(buf), _("Install `Ngraph.ini' to ~/%s ?"), HOME_DIR);
    if (MessageBox(TopLevel, buf, "Ngraph.ini", MB_YESNO) == IDYES) {
      if (!copyconfig()) {
	MessageBox(TopLevel, _("Ngraph.ini could not be copied."), "Ngraph.ini", MB_ERROR);
	return FALSE;
      }
    } else {
      return FALSE;
    }
  }
  return TRUE;
}

void
SaveHistory(void)
{
  struct narray conf;
  char *buf;
  int i, num;
  char **data, data_history[] = "data_history=";

  if (!Menulocal.savehistory)
    return;
  if (!CheckIniFile())
    return;
  arrayinit(&conf, sizeof(char *));

  num = arraynum(Menulocal.datafilelist);
  data = (char **) arraydata(Menulocal.datafilelist);
  for (i = 0; i < num; i++) {
    if (data[i]) {
      buf = g_strdup_printf("%s%s", data_history, data[i]);
      if (buf) {
	arrayadd(&conf, &buf);
      }
    }
  }
  replaceconfig("[x11menu]", &conf);

  arraydel2(&conf);
  arrayinit(&conf, sizeof(char *));
  if (arraynum(Menulocal.datafilelist) == 0) {
    buf = g_strdup(data_history);
    if (buf) {
      arrayadd(&conf, &buf);
    }
  }
  removeconfig("[x11menu]", &conf);
  arraydel2(&conf);
}

void
ProgressDialogSetTitle(char *title)
{
  if (ProgressDiaog)
    gtk_window_set_title(GTK_WINDOW(ProgressDiaog), title);
}

static void
show_progress(int pos, char *msg, double fraction)
{
  GtkProgressBar *bar;

  if (! ProgressDiaog)
    return;

  if (pos) {
    bar = ProgressBar2;
  } else {
    bar = ProgressBar;
  }

  if (fraction < 0) {
    gtk_progress_bar_pulse(bar);
  } else {
    gtk_progress_bar_set_fraction(bar, fraction);
  }

  gtk_progress_bar_set_text(bar, msg);
}

static gboolean
cb_del(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  return TRUE;
}

static void 
stop_btn_clicked(GtkButton *button, gpointer user_data)
{
  NgraphApp.Interrupt = TRUE;
}

void
ProgressDialogCreate(char *title)
{
  GtkWidget *btn, *vbox, *hbox;

  if (TopLevel == NULL)
    return;

  if (ProgressDiaog)
    gtk_widget_destroy(ProgressDiaog);

  SaveCursor = GetCursor();
  SetCursor(GDK_WATCH);

  set_draw_lock(DrawLockDraw);

  ProgressDiaog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  g_signal_connect(ProgressDiaog, "delete-event", G_CALLBACK(cb_del), NULL);
  gtk_window_set_title(GTK_WINDOW(ProgressDiaog), title);

  gtk_window_set_transient_for(GTK_WINDOW(ProgressDiaog), GTK_WINDOW(TopLevel));
  gtk_window_set_modal(GTK_WINDOW(ProgressDiaog), TRUE);
  gtk_window_set_position(GTK_WINDOW(ProgressDiaog), GTK_WIN_POS_CENTER);
  gtk_window_set_type_hint(GTK_WINDOW(ProgressDiaog), GDK_WINDOW_TYPE_HINT_DIALOG);

  vbox = gtk_vbox_new(FALSE, 4);

  ProgressBar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
  gtk_progress_bar_set_ellipsize(ProgressBar, PANGO_ELLIPSIZE_MIDDLE);
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(ProgressBar), FALSE, FALSE, 4);

  ProgressBar2 = GTK_PROGRESS_BAR(gtk_progress_bar_new());
  gtk_progress_bar_set_ellipsize(ProgressBar2, PANGO_ELLIPSIZE_MIDDLE);
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(ProgressBar2), FALSE, FALSE, 4);

  hbox = gtk_hbox_new(FALSE, 4);
  btn = gtk_button_new_from_stock(GTK_STOCK_STOP);
  g_signal_connect(btn, "clicked", G_CALLBACK(stop_btn_clicked), NULL);

  gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 4);

  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
  gtk_container_add(GTK_CONTAINER(ProgressDiaog), vbox);

  gtk_window_set_default_size(GTK_WINDOW(ProgressDiaog), 400, -1);
  gtk_widget_show_all(ProgressDiaog);

  set_progress_func(show_progress);
}

void
ProgressDialogFinalize(void)
{
  if (TopLevel == NULL)
    return;

  SetCursor(SaveCursor);
  set_progress_func(NULL);
  gtk_widget_destroy(ProgressDiaog);
  ProgressDiaog = NULL;
  ProgressBar = NULL;
  set_draw_lock(DrawLockNone);
}

void
ErrorMessage(void)
{
  char *ptr;

  ptr = g_locale_to_utf8(strerror(errno), -1, NULL, NULL, NULL);
  if (ptr) {
    MessageBox(NULL, ptr, _("error"), MB_ERROR);
    g_free(ptr);
  }
}

/*
 * $Id: x11commn.c,v 1.60 2010-03-04 08:30:17 hito Exp $
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
#include "object.h"
#include "ioutil.h"
#include "shell.h"
#include "nstring.h"
#include "odata.h"
#include "odraw.h"
#include "nconfig.h"

#include "init.h"
#include "gtk_widget.h"
#include "x11gui.h"
#include "x11graph.h"
#include "x11dialg.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11commn.h"
#include "x11info.h"
#include "x11file.h"
#include "x11view.h"

#define COMMENT_BUF_SIZE 1024

static GtkWidget *ProgressDialog = NULL;
#if GTK_CHECK_VERSION(4, 0, 0)
#define PROGRESSBAR_N 2
static GtkProgressBar *ProgressBar[PROGRESSBAR_N];
#else
static GtkProgressBar *ProgressBar, *ProgressBar2;
#endif
static unsigned int SaveCursor;

static void AddNgpFileList(const char *file);
static void ToFullPath(void);
static void ToBasename(void);
static void ToRalativePath(void);

void
OpenGRA(void)
{
  int i, id, otherGC;
  char *device, *name, *name_str = "viewer";
  struct narray *drawrable;
  N_VALUE *gra_inst;

  gra_inst = chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid);
  if (gra_inst) {
    return;
  }

  CloseGRA();

  for (i = chkobjlastinst(Menulocal.GRAobj); i >= 0; i--) {
    getobj(Menulocal.GRAobj, "GC", i, 0, NULL, &otherGC);
    if (otherGC < 0)
      break;
  }

  if (i == -1) {
    /* closed gra object is not found. generate new gra object */

    id = newobj(Menulocal.GRAobj);
    gra_inst = chkobjinst(Menulocal.GRAobj, id);
    _getobj(Menulocal.GRAobj, "oid", gra_inst, &(Menulocal.GRAoid));

    /* set page settings */
    putobj(Menulocal.GRAobj, "paper_width", id, &(Menulocal.PaperWidth));
    putobj(Menulocal.GRAobj, "paper_height", id, &(Menulocal.PaperHeight));
    putobj(Menulocal.GRAobj, "left_margin", id, &(Menulocal.LeftMargin));
    putobj(Menulocal.GRAobj, "top_margin", id, &(Menulocal.TopMargin));
    putobj(Menulocal.GRAobj, "zoom", id, &(Menulocal.PaperZoom));
    putobj(Menulocal.GRAobj, "decimalsign", id, &(Menulocal.Decimalsign));
    CheckPage();
  } else {
    /* find closed gra object */

    id = i;
    gra_inst = chkobjinst(Menulocal.GRAobj, id);
    _getobj(Menulocal.GRAobj, "oid", gra_inst, &(Menulocal.GRAoid));

    /* get page settings */
    CheckPage();
  }

  if (arraynum(&(Menulocal.drawrable)) > 0) {
    unsigned int j;
    drawrable = arraynew(sizeof(char *));
    for (j = 0; j < arraynum(&(Menulocal.drawrable)); j++) {
      arrayadd2(drawrable, *(char **)arraynget(&(Menulocal.drawrable), j));
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
  N_VALUE *gra_inst;

  gra_inst = chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid);
  if (gra_inst == NULL) {
    return;
  }

  _getobj(Menulocal.GRAobj, "paper_width", gra_inst,
	  &(Menulocal.PaperWidth));
  _getobj(Menulocal.GRAobj, "paper_height", gra_inst,
	  &(Menulocal.PaperHeight));
  _getobj(Menulocal.GRAobj, "left_margin", gra_inst,
	  &(Menulocal.LeftMargin));
  _getobj(Menulocal.GRAobj, "top_margin", gra_inst,
	  &(Menulocal.TopMargin));
  _getobj(Menulocal.GRAobj, "zoom", gra_inst,
	  &(Menulocal.PaperZoom));
  _getobj(Menulocal.GRAobj, "decimalsign", gra_inst,
	  &(Menulocal.Decimalsign));

  set_paper_type(Menulocal.PaperWidth, Menulocal.PaperHeight);
}

static int
ValidGRA(void)
{
  int id;
  struct objlist *graobj;
  N_VALUE *inst;

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
  N_VALUE *gra_inst;

  gra_inst = chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid);
  if (gra_inst == NULL) {
    return;
  }

  _getobj(Menulocal.GRAobj, "id", gra_inst, &id);
  exeobj(Menulocal.GRAobj, "close", id, 0, NULL);
  delobj(Menulocal.GRAobj, id);
  Menulocal.GRAoid = -1;
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
    if (chkobjinstoid(Menulocal.GRAobj, Menulocal.GRAoid) == NULL) {
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
  int i, otherGC;
  struct objlist *obj;
  struct narray *drawrable;

  if ((obj = chkobject("gra")) == NULL)
    return;

  for (i = chkobjlastinst(obj); i >= 0; i--) {
    getobj(obj, "GC", i, 0, NULL, &otherGC);
    if (otherGC < 0)
      break;
  }

  if (i >= 0) {
    int id, j, num;
    N_VALUE *inst;
    id = i;
    inst = chkobjinst(obj, id);

    putobj(obj, "paper_width", id, &(Menulocal.PaperWidth));
    putobj(obj, "paper_height", id, &(Menulocal.PaperHeight));
    putobj(obj, "left_margin", id, &(Menulocal.LeftMargin));
    putobj(obj, "top_margin", id, &(Menulocal.TopMargin));
    putobj(obj, "zoom", id, &(Menulocal.PaperZoom));
    putobj(obj, "decimalsign", id, &(Menulocal.Decimalsign));

    _getobj(obj, "draw_obj", inst, &drawrable);

    arrayfree2(drawrable);

    drawrable = arraynew(sizeof(char *));

    num = arraynum(&(Menulocal.drawrable));

    for (j = 0; j < num; j++) {
      arrayadd2(drawrable, *(char **) arraynget(&(Menulocal.drawrable), j));
    }

    _putobj(obj, "draw_obj", inst, drawrable);
  }
}

void
GetPageSettingsFromGRA(void)
{
  int i, otherGC;
  struct objlist *obj;
  struct narray *drawrable;

  if ((obj = chkobject("gra")) == NULL)
    return;

  for (i = chkobjlastinst(obj); i >= 0; i--) {
    getobj(obj, "GC", i, 0, NULL, &otherGC);
    if (otherGC < 0)
      break;
  }

  if (i >= 0) {
    int id, num;
    N_VALUE *inst;
    id = i;
    inst = chkobjinst(obj, id);
    CheckPage();
    _getobj(obj, "draw_obj", inst, &drawrable);
    arraydel2(&(Menulocal.drawrable));
    num = arraynum(drawrable);

    if (num == 0) {
      menuadddrawrable(chkobject("draw"), &(Menulocal.drawrable));
    } else {
      struct objlist *dobj;
      int j;
      for (j = 0; j < num; j++) {
        char *name;
	name = arraynget_str(drawrable, j);
	if (name == NULL) {
	  continue;
	}
	dobj = chkobject(name);
	if (dobj == NULL) {
	  continue;
	}
	arrayadd2(&(Menulocal.drawrable), chkobjectname(dobj));
      }
      arrayuniq_all_str(&(Menulocal.drawrable));
    }
  }

  ChangeGRA();
}

static int
get_new_axis_id(struct objlist *obj, struct objlist **aobj, int fid, int id, int a)
{
  int spc, aid = 0;
  char *axis;
  struct narray iarray;
  int anum;

  if (getobj(obj, (a == AXIS_X) ? "axis_x" : "axis_y", fid, 0, NULL, &axis) < 0) {
    return -1;
  }

  if (axis == NULL) {
    return -1;
  }

  arrayinit(&iarray, sizeof(int));
  if (getobjilist(axis, aobj, &iarray, FALSE, &spc)) {
    return -1;
  }

  anum = arraynum(&iarray);
  if (anum > 0 && spc == OBJ_LIST_SPECIFIED_BY_ID) {
    aid = arraylast_int(&iarray);
    if (aid > id) {
      aid--;
    }
  } else {
    aid = -1;
  }
  arraydel(&iarray);

  return aid;
}

static void
AxisDel2(int id)
{
  struct objlist *obj, *aobj;
  int i, aid1;
  char *axis2;

  obj = getobject("axisgrid");
  if (obj) {
    for (i = chkobjlastinst(obj); i >= 0; i--) {
      N_VALUE *inst;
      inst = chkobjinst(obj, i);
      if (inst == NULL) {
	continue;
      }

      aid1 = get_axis_id(obj, inst, &aobj, AXIS_X);
      if (aid1 == id) {
	delobj(obj, i);
	continue;
      }

      aid1 = get_axis_id(obj, inst, &aobj, AXIS_Y);
      if (aid1 == id) {
	delobj(obj, i);
      }
    }
  }

  obj = getobject("data");
  if (obj == NULL) {
    return;
  }
  for (i = 0; i <= chkobjlastinst(obj); i++) {
    int aid2;
    aid1 = get_new_axis_id(obj, &aobj, i, id, AXIS_X);
    aid2 = get_new_axis_id(obj, &aobj, i, id, AXIS_Y);
    if ((aid1 >= 0) && (aid2 >= 0)) {
      if (aid1 == aid2) {
	aid2 = aid1 + 1;
      }

      axis2 = g_strdup_printf("%s:%d", chkobjectname(aobj), aid1);
      if (axis2) {
	putobj(obj, "axis_x", i, axis2);
      }

      axis2 = g_strdup_printf("%s:%d", chkobjectname(aobj), aid2);
      if (axis2) {
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
  N_VALUE *inst;
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
    N_VALUE *inst2;
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
  int spc;
  char *axis;
  struct narray iarray;
  int anum;

  if (getobj(obj, axis_str, i, 0, NULL, &axis) < 0 || axis == NULL)
    return;

  arrayinit(&iarray, sizeof(int));
  if (getobjilist(axis, &aobj, &iarray, FALSE, &spc))
    return;

  anum = arraynum(&iarray);
  if (anum > 0 && spc == OBJ_LIST_SPECIFIED_BY_ID) {
    int aid;
    char *axis2;
    aid = arraylast_int(&iarray);
    if (aid == id1) {
      aid = id2;
    } else {
      if (aid > id1)
	aid--;
      if (aid >= id2)
	aid++;
    }
    axis2 = g_strdup_printf("%s:%d", chkobjectname(aobj), aid);
    if (axis2) {
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

  if ((obj = getobject("data")) == NULL)
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
  int a, j;
  char *argv[2];

  if ((obj = (struct objlist *) chkobject("axis")) == NULL)
    return;
  num = chkobjlastinst(obj);
  arrayinit(&iarray, sizeof(int));
  for (id = 0; id <= num; id++) {
    int anum;
    int *data;
    N_VALUE *inst;
    anum = arraynum(&iarray);
    data = arraydata(&iarray);
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

static void
field_obj_del(struct objlist *obj, int id, const char *field)
{
  char *fit;
  struct objlist *fitobj;
  struct narray iarray;

  if ((getobj(obj, field, id, 0, NULL, &fit) >= 0) && (fit != NULL)) {
    arrayinit(&iarray, sizeof(int));
    if (getobjilist(fit, &fitobj, &iarray, FALSE, NULL) == 0) {
      int idnum, i;
      idnum = arraynum(&iarray);
      arraysort_int(&iarray);
      for (i = idnum - 1; i >= 0; i--) {
        int fitid;
	fitid = arraynget_int(&iarray, i);
	delobj(fitobj, fitid);
      }
    }
    arraydel(&iarray);
  }
}

void
FitDel(struct objlist *obj, int id)
{
  field_obj_del(obj, id, "fit");
}

void
ArrayDel(struct objlist *obj, int id)
{
  field_obj_del(obj, id, "array");
}

void
FitCopy(struct objlist *obj, int did, int sid)
{
  char *fit;
  struct objlist *fitobj;
  struct narray iarray;
  struct narray iarray2;
  int fitoid;

  if ((getobj(obj, "fit", sid, 0, NULL, &fit) >= 0) && (fit != NULL)) {
    arrayinit(&iarray, sizeof(int));
    if (getobjilist(fit, &fitobj, &iarray, FALSE, NULL) == 0) {
      int idnum;
      idnum = arraynum(&iarray);
      if (idnum >= 1) {
        int fitid, id2;
	fitid = arraylast_int(&iarray);
	if ((getobj(obj, "fit", did, 0, NULL, &fit) >= 0)
	    && (fit != NULL)) {
	  arrayinit(&iarray2, sizeof(int));
	  if (getobjilist(fit, &fitobj, &iarray2, FALSE, NULL) == 0) {
            int idnum2;
	    idnum2 = arraynum(&iarray2);
	    if (idnum2 >= 1) {
	      id2 = arraylast_int(&iarray2);
	    } else
	      id2 = newobj(fitobj);
	  } else
	    id2 = newobj(fitobj);
	  arraydel(&iarray2);
	} else {
	  id2 = newobj(fitobj);
        }
	if (id2 >= 0) {
          char *field[] = {"name", "equation", NULL};
          N_VALUE *inst;
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

  if (Menulock || Globallock)
    return;
  if ((obj = chkobject("data")) == NULL)
    return;
  if ((fitobj = chkobject("fit")) == NULL)
    return;
  for (i = 0; i <= chkobjlastinst(obj); i++) {
    getobj(obj, "fit", i, 0, NULL, &fit);
    if (fit == NULL) {
      continue;
    }
    arrayinit(&iarray, sizeof(int));
    if (getobjilist(fit, &fitobj, &iarray, FALSE, NULL) == 0) {
      anum = arraynum(&iarray);
      if (anum >= 1) {
	id = arraylast_int(&iarray);
	getobj(obj, "hidden", i, 0, NULL, &hidden);
	if (! hidden) {
	  putobj(fitobj, "equation", id, NULL);
	}
      }
      arraydel(&iarray);
    }
  }
}

static void
del_darray(struct objlist *data_obj)
{
  char *array;
  int i, j, n, last;
  struct array_prm ary;
  struct narray *id_array;
  struct objlist *darray_obj;

  darray_obj = chkobject("darray");
  if (darray_obj == NULL) {
    return;
  }

  last = chkobjlastinst(data_obj);
  if (last < 0) {
    return;
  }

  id_array = arraynew(sizeof(int));
  if (id_array == NULL) {
    return;
  }

  for (i = 0; i <= last; i++) {
    getobj(data_obj, "array", i, 0, NULL, &array);
    if (array == NULL) {
      continue;
    }
    open_array(array, &ary);
    for (j = 0; j < ary.col_num; j++) {
      arrayadd(id_array, &ary.id[j]);
    }
  }

  arraysort_int(id_array);
  arrayuniq_int(id_array);

  n = arraynum(id_array);
  for (i = 0; i < n; i++) {
    int id;
    id = arraynget_int(id_array, n - 1 - i);
    delobj(darray_obj, id);
  }
  arrayfree(id_array);
}

static void
delete_instances(struct objlist *obj)
{
  int instnum, i;
  instnum = chkobjlastinst(obj);
  if (instnum == -1) {
    return;
  }
  for (i = instnum; i >= 0; i--) {
    delobj(obj, i);
  }
}

void
DeleteDrawable(void)
{
  struct objlist *fileobj, *drawobj, *prmobj;

  fileobj = chkobject("data");
  if (fileobj) {
    int i;
    for (i = 0; i <= chkobjlastinst(fileobj); i++) {
      FitDel(fileobj, i);
    }
    del_darray(fileobj);
  }

  drawobj = chkobject("draw");
  if (drawobj) {
    delchildobj(drawobj);
  }

  prmobj = chkobject("parameter");
  if (prmobj) {
    delete_instances(prmobj);
  }
  set_toolbox_mode(TOOLBOX_MODE_TOOLBAR);
  Menulocal.Decimalsign = get_gra_decimalsign_type(Menulocal.default_decimalsign);
  gra_set_default_decimalsign(Menulocal.Decimalsign);
}

static void
store_file(struct objlist *ocur, int hFile, int i)
{
  char *s, *fname, *fname2;
  int j;
  N_VALUE *inst;

  getobj(ocur, "file", i, 0, NULL, &fname);
  if (fname == NULL) {
    return;
  }

  for (j = i - 1; j >= 0; j--) {
    getobj(ocur, "file", j, 0, NULL, &fname2);
    if ((fname2 != NULL) && (strcmp0(fname, fname2) == 0)) {
      break;
    }
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

static void
save_array(struct objlist *ocur, int hFile, int i, GString *str)
{
  char *s, *array, *array2, iarray_str[] = "iarray:!:push ${darray:!:oid}\n\n";
  int j, k, l, n, id;
  struct array_prm ary, ary2;
  struct narray *id_array;

  getobj(ocur, "array", i, 0, NULL, &array);
  if (array == NULL) {
    return;
  }

  id_array = arraynew(sizeof(int));
  for (j = 0; j < i; j++) {
    getobj(ocur, "array", j, 0, NULL, &array2);
    if (array2 == NULL) {
      continue;
    }
    open_array(array2, &ary2);
    for (k = 0; k < ary2.col_num; k++) {
      n = arraynum(id_array);
      for (l = 0; l < n; l++) {
	id = arraynget_int(id_array, l);
	if (id == ary2.id[k]) {
	  break;
	}
      }
      if (l == n) {
	arrayadd(id_array, &ary2.id[k]);
      }
    }
  }

  open_array(array, &ary);
  for (k = 0; k < ary.col_num; k++) {
    n = arraynum(id_array);
    for (l = 0; l < n; l++) {
      id = arraynget_int(id_array, l);
      if (id == ary2.id[k]) {
	break;
      }
    }
    if (l == n) {
      arrayadd(id_array, &ary.id[k]);
      if (getobj(ary.obj, "save", ary.id[k], 0, NULL, &s) != -1) {
	nwrite(hFile, s, strlen(s));
	nwrite(hFile, "\n", 1);
	nwrite(hFile, iarray_str, sizeof(iarray_str) - 1);
      }
    }
    g_string_append_printf(str, "%s^${iarray:!:get:%d}", (k == 0) ? "darray:\"" : ",", l);
  }
  arrayfree(id_array);
}

static void
save_merge(struct objlist *ocur, int hFile, int storemerge, int i)
{
  char *s;

  getobj(ocur, "save", i, 0, NULL, &s);
  nwrite(hFile, s, strlen(s));
  nwrite(hFile, "\n", 1);
  if (storemerge) {
    store_file(ocur, hFile, i);
  }
}

static void
save_data(struct objlist *ocur, int hFile, int storedata, int i, int *array_data)
{
  char *s;
  int source, argc;
  char *argv2[3], **argv;
  struct narray *array;
  GString *str;

  str = NULL;
  argc = 0;
  argv = NULL;
  array = NULL;
  getobj(ocur, "source", i, 0, NULL, &source);
  switch (source) {
  case DATA_SOURCE_FILE:
    break;
  case DATA_SOURCE_ARRAY:
    array = arraynew(sizeof(char *));
    if (array == NULL) {
      error(NULL, ERRHEAP);
      return;
    }
    s = "array";
    arrayadd(array, &s);
    argv2[0] = (char *) array;
    argv2[1] = NULL;

    argv = argv2;
    argc = 1;

    str = g_string_new("\tdata::array=");
    if (str == NULL) {
      error(NULL, ERRHEAP);
      return;
    }
    if (! *array_data) {
      char iarray_str[] = "new iarray\n\n";
      *array_data = TRUE;
      nwrite(hFile, iarray_str, sizeof(iarray_str) - 1);
    }
    save_array(ocur, hFile, i, str);
    break;
  case DATA_SOURCE_RANGE:
    break;
  }

  getobj(ocur, "save", i, argc, argv, &s);
  nwrite(hFile, s, strlen(s));
  if (storedata) {
    store_file(ocur, hFile, i);
  }

  if (array) {
    arrayfree(array);
  }

  if (str) {
    nwrite(hFile, str->str, str->len);
    nwrite(hFile, "\"\n", 2);
    g_string_free(str, TRUE);
  }
  nwrite(hFile, "\n", 1);
}

static void
save_inst(int hFile, struct objlist *ocur)
{
  int i, instnum;
  char *s;
  instnum = chkobjlastinst(ocur);
  if (instnum == -1) {
    return;
  }
  for (i = 0; i <= instnum; i++) {
    getobj(ocur, "save", i, 0, NULL, &s);
    nwrite(hFile, s, strlen(s));
    nwrite(hFile, "\n", 1);
  }
}

static void
SaveParent(int hFile, struct objlist *parent, int storedata,
	   int storemerge)
{
  struct objlist *ocur, *odata, *omerge;
  int i, instnum, array_data;
  char *s;

  ocur = chkobjroot();
  odata = chkobject("data");
  omerge = chkobject("merge");
  array_data = FALSE;
  while (ocur) {
    if (chkobjparent(ocur) == parent) {
      if ((instnum = chkobjlastinst(ocur)) != -1) {
	for (i = 0; i <= instnum; i++) {
	  if (ocur == odata) {
	    save_data(ocur, hFile, storedata, i, &array_data);
	  } else if (ocur == omerge) {
	    save_merge(ocur, hFile, storemerge, i);
	  } else {
	    getobj(ocur, "save", i, 0, NULL, &s);
	    nwrite(hFile, s, strlen(s));
	    nwrite(hFile, "\n", 1);
	  }
	}
	if (ocur == odata && array_data) {
	  char iarray_str[] = "del iarray:!\n\n";
	  nwrite(hFile, iarray_str, sizeof(iarray_str) -1);
	}
      }
      SaveParent(hFile, ocur, storedata, storemerge);
    }
    ocur = ocur->next;
  }
}

int
SaveDrawrable(char *name, int storedata, int storemerge, int save_decimalsign)
{
  int hFile;
  int error;
  struct narray sarray;
  char *ver, *sysname, *s, *opt;

  error = FALSE;

  hFile = nopen(name, O_CREAT | O_TRUNC | O_RDWR, NFMODE_NORMAL_FILE);

  if (hFile < 0) {
    error = TRUE;
  } else {
    int len;
    struct objlist *sysobj, *drawobj, *prmobj;
    N_VALUE *inst;
    char comment[COMMENT_BUF_SIZE];
    sysobj = chkobject("system");
    inst = chkobjinst(sysobj, 0);
    _getobj(sysobj, "name", inst, &sysname);
    _getobj(sysobj, "version", inst, &ver);

    len = snprintf(comment, sizeof(comment),
		   "#!ngraph\n#%%creator: %s \n#%%version: %s\n",
		   sysname, ver);
    if (nwrite(hFile, comment, len) != len)
      error = TRUE;

    prmobj = chkobject("parameter");
    if (prmobj) {
      save_inst(hFile, prmobj);
    }
    if ((drawobj = chkobject("draw")) != NULL) {
      struct objlist *graobj;
      SaveParent(hFile, drawobj, storedata, storemerge);
      if ((graobj = chkobject("gra")) != NULL) {
        int id;
	id = ValidGRA();
	if (id != -1) {
          char *arg[2];
	  arrayinit(&sarray, sizeof(char *));
	  opt = "device";
	  arrayadd(&sarray, &opt);
	  if (! save_decimalsign) {
	    opt = "decimalsign";
	    arrayadd(&sarray, &opt);
	  }
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

#if GTK_CHECK_VERSION(4, 0, 0)
struct graph_save_data
{
  int storedata, storemerge, path;
  char *file, *current_wd;
  response_cb cb;
};

static int
get_save_opt_response(struct response_callback *cb)
{
  struct objlist *fobj, *mobj;
  int fnum, mnum, path;
  int i;
  struct graph_save_data *save_data;;

  save_data = (struct graph_save_data *) cb->data;

  if (DlgSave.ret != IDOK) {
    return IDCANCEL;
  }

  fobj = chkobject("data");
  mobj = chkobject("merge");

  fnum = chkobjlastinst(fobj) + 1;
  mnum = chkobjlastinst(mobj) + 1;

  path = DlgSave.Path;
  for (i = 0; i < fnum; i++) {
    putobj(fobj, "save_path", i, &path);
  }

  for (i = 0; i < mnum; i++) {
    putobj(mobj, "save_path", i, &path);
  }
  save_data->path = path;
  save_data->storedata = DlgSave.SaveData;
  save_data->storemerge = DlgSave.SaveMerge;
  save_data->cb(cb->return_value, save_data);
  return 0;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
static int
get_save_opt(struct graph_save_data *save_data)
{
  int fnum, mnum, i, src;
  struct objlist *fobj, *mobj;

  fobj = chkobject("data");
  mobj = chkobject("merge");

  fnum = chkobjlastinst(fobj) + 1;
  mnum = chkobjlastinst(mobj) + 1;

  /* there are no instances of data and merge objects */
  if (fnum < 1 && mnum < 1) {
    return IDOK;
  }

  /* check source field of data objects */
  for (i = 0; i < fnum; i++) {
    getobj(fobj, "source", i, 0, NULL, &src);
    if (src ==  DATA_SOURCE_FILE) {
      break;
    }
  }
  if (fnum > 0 && mnum < 1 && i == fnum) {
    return IDOK;
  }

  SaveDialog(&DlgSave, 0, 0);
  /* must be implemented */
  response_callback_add(&DlgSave, get_save_opt_response, NULL, save_data);
  DialogExecute(TopLevel, &DlgSave);
  return IDOK;
}
#else
static int
get_save_opt(int *sdata, int *smerge, int *path)
{
  int ret, fnum, mnum, i, src;
  struct objlist *fobj, *mobj;

  *path = SAVE_PATH_UNCHANGE;
  *sdata = FALSE;
  *smerge = FALSE;

  fobj = chkobject("data");
  mobj = chkobject("merge");

  fnum = chkobjlastinst(fobj) + 1;
  mnum = chkobjlastinst(mobj) + 1;

  /* there are no instances of data and merge objects */
  if (fnum < 1 && mnum < 1) {
    return IDOK;
  }

  /* check source field of data objects */
  for (i = 0; i < fnum; i++) {
    getobj(fobj, "source", i, 0, NULL, &src);
    if (src ==  DATA_SOURCE_FILE) {
      break;
    }
  }
  if (fnum > 0 && mnum < 1 && i == fnum) {
    return IDOK;
  }

  SaveDialog(&DlgSave, sdata, smerge);
  ret = DialogExecute(TopLevel, &DlgSave);
  if (ret != IDOK)
    return IDCANCEL;

  *path = DlgSave.Path;
  for (i = 0; i < fnum; i++) {
    putobj(fobj, "save_path", i, path);
  }

  for (i = 0; i < mnum; i++) {
    putobj(mobj, "save_path", i, path);
  }
  return IDOK;
}
#endif

static void
GraphSave_free(gpointer user_data)
{
  struct graph_save_data *data;
  data = (struct graph_save_data *) user_data;
  g_free(data->file);
  g_free(data->current_wd);
}

static void
GraphSave_response(int ret, gpointer user_data)
{
  struct graph_save_data *d;
  d = (struct graph_save_data *) user_data;

  if (ret == IDOK) {
    char mes[256];
    snprintf(mes, sizeof(mes), _("Saving `%.128s'."), d->file);
    SetStatusBar(mes);
    if(SaveDrawrable(d->file, d->storedata, d->storemerge, TRUE)) {
      ret = IDCANCEL;
    } else {
      switch (d->path) {
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
      changefilename(d->file);
      AddNgpFileList(d->file);
      SetFileName(d->file);
      reset_graph_modified();
    }
    ResetStatusBar();
  }

  if (d->current_wd && nchdir(d->current_wd)) {
    ErrorMessage();
  }
  GraphSave_free(d);
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
GraphSaveSub(char *file, char *prev_wd, char *current_wd)
{
  if (file) {
    struct graph_save_data *save_data;
    if (prev_wd && nchdir(prev_wd)) {
      ErrorMessage();
    }

    save_data = g_malloc0(sizeof(*save_data));
    if (save_data == NULL) {
      return;
    }
    save_data->file = g_strdup(file);
    save_data->current_wd = g_strdup(current_wd);
    save_data->cb = GraphSave_response;
    get_save_opt(save_data);
    g_free(file);
  }

  g_free(current_wd);
  g_free(prev_wd);
}

static void
graph_save_response(char *file, gpointer user_data)
{
  char *prev_wd, *current_wd;
  prev_wd = (char *) user_data;
  current_wd = ngetcwd();
  if (prev_wd && current_wd && strcmp(prev_wd, current_wd) == 0) {
    g_free(prev_wd);
    g_free(current_wd);
    prev_wd = NULL;
    current_wd = NULL;
  }
  GraphSaveSub(file, prev_wd, current_wd);
}

int
GraphSave(int overwrite)
{
  char *initfil;
  int chd;

  if (NgraphApp.FileName != NULL) {
    initfil = NgraphApp.FileName;
  } else {
    initfil = NULL;
    overwrite = FALSE;
  }
  if ((initfil == NULL) || (! overwrite || (naccess(initfil, 04) == -1))) {
    char *prev_wd;
    initfil = (initfil) ? initfil : "untitled.ngp";
    prev_wd = ngetcwd();
    chd = Menulocal.changedirectory;
    nGetSaveFileName(TopLevel, _("Save NGP file"), "ngp",
                     &(Menulocal.graphloaddir), initfil, chd,
                     graph_save_response, prev_wd);
  } else {
    char *file;
    file = g_strdup(initfil);
    if (file) {
      GraphSaveSub(file, NULL, NULL);
    }
  }
  return IDOK;
}
#else
int
GraphSave(int overwrite)
{
  int path;
  int sdata, smerge;
  int ret;
  char *initfil;
  char *file, *prev_wd, *current_wd;
  int chd;

  if (NgraphApp.FileName != NULL) {
    initfil = NgraphApp.FileName;
  } else {
    initfil = NULL;
    overwrite = FALSE;
  }
  prev_wd = current_wd = NULL;
  if ((initfil == NULL) || (! overwrite || (naccess(initfil, 04) == -1))) {
    prev_wd = ngetcwd();
    chd = Menulocal.changedirectory;
    file = nGetSaveFileName(TopLevel, _("Save NGP file"), "ngp",
			   &(Menulocal.graphloaddir), initfil, overwrite, chd);
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

  if (file) {
    if (prev_wd && nchdir(prev_wd)) {
      ErrorMessage();
    }

    ret = get_save_opt(&sdata, &smerge, &path);
    if (ret == IDOK) {
      char mes[256];
      snprintf(mes, sizeof(mes), _("Saving `%.128s'."), file);
      SetStatusBar(mes);
      if(SaveDrawrable(file, sdata, smerge, TRUE)) {
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

    if (current_wd && nchdir(current_wd)) {
      ErrorMessage();
    }
  }

  g_free(prev_wd);
  g_free(current_wd);

  return ret;
}
#endif

static void
change_filename(char * (*func)(const char *))
{
  int i;
  unsigned int j;
  char *file, *file2, *objname[] = {"data", "merge"};

  for (j = 0; j < sizeof(objname) / sizeof(*objname); j++) {
    struct objlist *obj;
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
get_basename(const char *file)
{
  char *ptr;

  ptr = getbasename(file);
  if (ptr == NULL)
    return NULL;

  return ptr;
}

static void
ToBasename(void)
{
  change_filename(get_basename);
}


#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */

struct load_dialog_data
{
  char *file, *cwd;
  int console;
  char *option;
};

static void
load_dialog_cb_free(struct load_dialog_data *data)
{
  g_free(data->file);
  g_free(data->option);
  g_free(data->cwd);
  g_free(data);
}

static void
draw_callback(gpointer user_data)
{
  UpdateAll2(NULL, FALSE);
  menu_clear_undo();
}

static int
LoadNgpFile_response(struct response_callback *cb)
{
  char *file;
  char *option;
  int console;
  struct load_dialog_data *d;
  struct objlist *sys;
  char *expanddir;
  struct objlist *obj;
  char *name;
  int r, newid, allocnow = FALSE, tmp;
  char *s;
  int len;
  char *argv[2];
  struct narray sarray;
  char mes[256];
  int sec;
  N_VALUE *inst;
  struct objlist *robj;
  int idn;
  int loadpath, expand;

  d = (struct load_dialog_data *) cb->data;

  file = d->file;
  console = d->console;
  option = d->option;

  if (DlgLoad.ret != IDOK) {
    goto ErrorExit;
  }
  changefilename(file);

  if (naccess(file, R_OK)) {
    ErrorMessage();
    goto ErrorExit;
  }

  sys = chkobject("system");
  if (sys == NULL) {
    goto ErrorExit;
  }

  loadpath = DlgLoad.loadpath;
  expand = DlgLoad.expand;
  expanddir = DlgLoad.exdir;
  DlgLoad.exdir = NULL;
  if (expanddir == NULL) {
    goto ErrorExit;
  }

  putobj(sys, "expand_dir", 0, expanddir);
  putobj(sys, "expand_file", 0, &expand);

  tmp = FALSE;
  putobj(sys, "ignore_path", 0, &tmp);

  obj = chkobject("shell");
  if (obj == NULL) {
    goto ErrorExit;
  }

  newid = newobj(obj);
  if (newid < 0) {
    goto ErrorExit;
  }

  inst = chkobjinst(obj, newid);
  arrayinit(&sarray, sizeof(char *));
  while ((s = getitok2(&option, &len, " \t")) != NULL) {
    if (arrayadd(&sarray, &s) == NULL) {
      g_free(s);
      arraydel2(&sarray);
      goto ErrorExit;
    }
  }

  name = g_strdup(file);

  if (name == NULL) {
    arraydel2(&sarray);
    goto ErrorExit;
  }

  if (arrayadd(&sarray, &name) == NULL) {
    g_free(name);
    arraydel2(&sarray);
    goto ErrorExit;
  }

  DeleteDrawable();

  if (console) {
    allocnow = allocate_console();
  }

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
    struct objlist *aobj;
    int i;
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

    switch (loadpath) {
    case LOAD_PATH_BASE:
      ToBasename();
      break;
    case LOAD_PATH_FULL:
      ToFullPath();
      break;
    }
    InfoWinClear();
  }

  AxisNameToGroup();
  ResetStatusBar();
  arraydel2(&sarray);

  if (console) {
    free_console(allocnow);
  }

  set_axis_undo_button_sensitivity(FALSE);
  GetPageSettingsFromGRA();
  Draw(FALSE, draw_callback, NULL);
  delobj(obj, newid);
  load_dialog_cb_free(d);

  return 0;

ErrorExit:
  if (d->cwd) {
    nchdir(d->cwd);
  }
  load_dialog_cb_free(d);
  return 1;
}

void
LoadNgpFile(const char *file, int console, const char *option, const char *cwd)
{
  struct load_dialog_data *data;

  data = g_malloc0(sizeof(* data));
  if (data == NULL) {
    return;
  }
  data->file = g_strdup(file);
  data->console = console;
  data->option = g_strdup(option);
  data->cwd = g_strdup(cwd);

  response_callback_add(&DlgLoad, LoadNgpFile_response, NULL, data);
  LoadDialog(&DlgLoad);
  DialogExecute(TopLevel, &DlgLoad);
}
#else
int
LoadNgpFile(char *file, int console, char *option)
{
  struct objlist *sys;
  char *expanddir;
  struct objlist *obj;
  char *name;
  int r, newid, allocnow = FALSE, tmp;
  char *s;
  int len;
  char *argv[2];
  struct narray sarray;
  char mes[256];
  N_VALUE *inst;
  struct objlist *robj;
  int idn;
  int loadpath, expand;

  LoadDialog(&DlgLoad);
  if (DialogExecute(TopLevel, &DlgLoad) != IDOK) {
    return 1;
  }
  changefilename(file);

  if (naccess(file, R_OK)) {
    ErrorMessage();
    return 1;
  }

  sys = chkobject("system");
  if (sys == NULL) {
    return 1;
  }

  loadpath = DlgLoad.loadpath;
  expand = DlgLoad.expand;
  expanddir = DlgLoad.exdir;
  DlgLoad.exdir = NULL;
  if (expanddir == NULL)
    return 1;

  putobj(sys, "expand_dir", 0, expanddir);
  putobj(sys, "expand_file", 0, &expand);

  tmp = FALSE;
  putobj(sys, "ignore_path", 0, &tmp);

  obj = chkobject("shell");
  if (obj == NULL)
    return 1;

  newid = newobj(obj);
  if (newid < 0)
    return 1;

  inst = chkobjinst(obj, newid);
  arrayinit(&sarray, sizeof(char *));
  while ((s = getitok2(&option, &len, " \t")) != NULL) {
    if (arrayadd(&sarray, &s) == NULL) {
      g_free(s);
      arraydel2(&sarray);
      delobj(obj, newid);
      return 1;
    }
  }

  name = g_strdup(file);

  if (name == NULL) {
    arraydel2(&sarray);
    delobj(obj, newid);
    return 1;
  }

  if (arrayadd(&sarray, &name) == NULL) {
    g_free(name);
    arraydel2(&sarray);
    delobj(obj, newid);
    return 1;
  }

  DeleteDrawable();

  if (console) {
    allocnow = allocate_console();
  }

  exeobj(obj, "set_security", newid, 0, NULL);

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

  if (r == 0) {
    struct objlist *aobj;
    int i;
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

    switch (loadpath) {
    case LOAD_PATH_BASE:
      ToBasename();
      break;
    case LOAD_PATH_FULL:
      ToFullPath();
      break;
    }
    InfoWinClear();
  }

  AxisNameToGroup();
  ResetStatusBar();
  arraydel2(&sarray);

  if (console) {
    free_console(allocnow);
  }

  set_axis_undo_button_sensitivity(FALSE);
  GetPageSettingsFromGRA();
  CmViewerDraw(NULL, GINT_TO_POINTER(FALSE));
  UpdateAll2(NULL, FALSE);
  delobj(obj, newid);
  menu_clear_undo();

  return 0;
}
#endif

static int
check_ref_axis(char *ref, const char *group)
{
  int anum, aid;
  char *refgroup;
  struct narray iarray;
  struct objlist *aobj;
  N_VALUE *inst;

  if (ref == NULL) {
    return FALSE;
  }

  arrayinit(&iarray, sizeof(int));
  if (getobjilist(ref, &aobj, &iarray, FALSE, NULL)) {
    arraydel(&iarray);
    return TRUE;
  }

  anum = arraynum(&iarray);
  if (anum < 1) {
    arraydel(&iarray);
    return TRUE;
  }

  aid = arraylast_int(&iarray);
  arraydel(&iarray);
  inst = getobjinst(aobj, aid);
  if (inst == NULL) {
    return TRUE;
  }

  _getobj(aobj, "group", inst, &refgroup);
  if (refgroup && group &&
      refgroup[0] == group[0] &&
      strcmp(refgroup + 2, group + 2) == 0) {
    return FALSE;
  }

  return TRUE;
}

static int
file_auto_scale(char *file_obj, struct objlist *aobj, int anum)
{
  int i;
  double min, max, inc;
  char *group, *ref, *argv[2];

  argv[0] = (char *) file_obj;
  argv[1] = NULL;
  for (i = 0; i <= anum; i++) {
    int refother;
    getobj(aobj, "min", i, 0, NULL, &min);
    getobj(aobj, "max", i, 0, NULL, &max);
    getobj(aobj, "inc", i, 0, NULL, &inc);
    getobj(aobj, "group", i, 0, NULL, &group);
    getobj(aobj, "reference", i, 0, NULL, &ref);
    refother = check_ref_axis(ref, group);
    if (! refother && (min == max || inc == 0)) {
      exeobj(aobj, "auto_scale", i, 1, argv);
    }
  }
  return 0;
}

void
FileAutoScale(void)
{
  int anum;
  struct objlist *aobj;
  char *buf;
  struct objlist *fobj;
  int lastinst;
  int i, hidden;
  GString *str;

  if ((fobj = chkobject("data")) == NULL)
    return;

  lastinst = chkobjlastinst(fobj);
  aobj = chkobject("axis");
  anum = chkobjlastinst(aobj);

  if (lastinst < 0 || aobj == NULL || anum == 0)
    return;

  str = g_string_new("data:");
  if (str == NULL) {
    error(NULL, ERRHEAP);
    return;
  }

  for (i = 0; i <= lastinst; i++) {
    getobj(fobj, "hidden", i, 0, NULL, &hidden);
    if (! hidden) {
      g_string_append_printf(str, "%d,", i);
    }
  }

  i = str->len - 1;
  buf = g_string_free(str, FALSE);
  if (buf[i] != ',') {
    g_free(buf);
    return;
  }
  buf[i] = '\0';

  file_auto_scale(buf, aobj, anum);
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

#if GTK_CHECK_VERSION(4, 0, 0)
struct CheckSave_data {
  response_cb cb;
  gpointer data;
};

static void
CheckSave_response(int ret, gpointer user_data)
{
  struct CheckSave_data *data;
  response_cb cb;
  int r = TRUE;

  data = (struct CheckSave_data *) user_data;
  cb = data->cb;
  if (ret == IDYES) {
    if (GraphSave(TRUE) == IDCANCEL) {
      r = FALSE;
    }
  } else if (ret != IDNO) {
    r = FALSE;
  }
  cb(r, data->data);
  g_free(data);
}

void
CheckSave(response_cb cb, gpointer user_data)
{
  struct CheckSave_data *data;
  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return;
  }
  data->cb = cb;
  data->data = user_data;
  if (get_graph_modified()) {
    response_message_box(TopLevel,
			 _("This graph is modified.\nSave this graph?"),
			 _("Confirm"), RESPONS_YESNOCANCEL,
			 CheckSave_response, data);
  } else {
    CheckSave_response(IDNO, data);
  }
}
#else
int
CheckSave(void)
{
  if (get_graph_modified()) {
    int ret;
    ret = message_box(TopLevel, _("This graph is modified.\nSave this graph?"),
		      _("Confirm"), RESPONS_YESNOCANCEL);
    if (ret == IDYES) {
      if (GraphSave(TRUE) == IDCANCEL)
	return FALSE;
    } else if (ret != IDNO)
      return FALSE;
  }
  return TRUE;
}
#endif

static void
add_hist(const char *file, char *mime)
{
  char *full_name, *uri;
  GtkRecentData recent_data = {
    NULL,
    AppName,
    NULL,
    AppName,
    "ngraph %f",
    NULL,
    FALSE,
  };

  if (! g_utf8_validate(file, -1, NULL)) {
    return;
  }

  full_name = getfullpath(file);
  if (full_name == NULL) {
    return;
  }

  recent_data.mime_type = mime;

  uri = g_filename_to_uri(full_name, NULL, NULL);
  g_free(full_name);

  gtk_recent_manager_add_full(NgraphApp.recent_manager, uri, &recent_data);
  g_free(uri);
}

static void
AddNgpFileList(const char *file)
{
  add_hist(file, NGRAPH_GRAPH_MIME);
}

void
AddDataFileList(const char *file)
{
  add_hist(file, NGRAPH_DATA_MIME);
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
allocate_console(void)
{
  int allocnow;

  loadstdio(&GtkIOSave);
  allocnow = nallocconsole();
  nforegroundconsole();
  return allocnow;
}

void
free_console(int allocnow)
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

static char *
get_plot_cb_str(struct objlist *obj, int id, int source)
{
  char *s;
  const char *str;

  str = get_plot_info_str(obj, id, source);
  if (str == NULL) {
    return g_strdup(FILL_STRING);
  }

  if (source == DATA_SOURCE_FILE) {
    char *valstr;
    valstr = getbasename(str);
    s = g_strdup_printf("%s", (valstr) ? valstr : FILL_STRING);
    if (valstr != NULL) {
      g_free(valstr);
    }
  } else {
    s = g_strdup(str);
  }

  return s;
}

char *
MergeFileCB(struct objlist *obj, int id)
{
  char *file, *bname;

  getobj(obj, "file", id, 0, NULL, &file);
  if (file == NULL) {
    return g_strdup(FILL_STRING);
  }
  bname = getbasename(file);
  if (bname == NULL) {
    return g_strdup(FILL_STRING);
  }
  return bname;
}

char *
FileCB(struct objlist *obj, int id)
{
  int source;

  getobj(obj, "source", id, 0, NULL, &source);
  return get_plot_cb_str(obj, id, source);
}

char *
PlotFileCB(struct objlist *obj, int id)
{
  int source;

  getobj(obj, "source", id, 0, NULL, &source);
  if (source != DATA_SOURCE_FILE) {
    return NULL;
  }
  return get_plot_cb_str(obj, id, source);
}

#if GTK_CHECK_VERSION(4, 0, 0)
/* to be implemented */
struct set_file_hidden_data {
  response_cb cb;
  gpointer user_data;
};

static int
set_file_hidden_response(struct response_callback *cb)
{
  struct SelectDialog *d;
  struct narray *farray, *ifarray;
  int i, a, r;

  d = (struct SelectDialog *) cb->dialog;
  farray = d->sel;
  ifarray = d->isel;
  r = 0;
  if (cb->return_value == IDOK) {
    int num, inum, *array, lastinst;
    struct objlist *fobj;
    fobj = chkobject("data");
    lastinst = chkobjlastinst(fobj);
    a = TRUE;
    for (i = 0; i <= lastinst; i++) {
      putobj(fobj, "hidden", i, &a);
    }
    num = arraynum(farray);
    array = arraydata(farray);
    a = FALSE;
    for (i = 0; i < num; i++) {
      putobj(fobj, "hidden", array[i], &a);
    }

    inum = arraynum(ifarray);
    if (inum != num) {
      set_graph_modified();
    } else {
      for (i = 0; i < num; i++) {
	if (arraynget_int(ifarray, i) != array[i]) {
	  set_graph_modified();
	  break;
	}
      }
    }
    r = 1;
  }

  arrayfree(ifarray);
  arrayfree(farray);

  if (cb->data) {
    struct set_file_hidden_data *data;
    data = (struct set_file_hidden_data *) cb->data;
    if (data->cb) {
      data->cb(r, data->user_data);
    }
    g_free(data);
  }

  return r;
}

int
SetFileHidden(response_cb cb, gpointer user_data)
{
  struct objlist *fobj;
  int lastinst;
  struct narray *farray, *ifarray;
  int i, a;
  struct set_file_hidden_data *data;

  fobj = chkobject("data");
  if (fobj == NULL) {
    return 1;
  }

  lastinst = chkobjlastinst(fobj);
  if (lastinst < 0) {
    return 1;
  }

  data = g_malloc0(sizeof(*data));
  if (data == NULL) {
    return 1;
  }
  data->cb = cb;
  data->user_data = data;

  ifarray = arraynew(sizeof(int));
  if (ifarray == NULL) {
    g_free(data);
    return 1;
  }
  farray = arraynew(sizeof(int));
  if (farray == NULL) {
    g_free(data);
    arrayfree(ifarray);
    return 1;
  }
  for (i = 0; i <= lastinst; i++) {
    getobj(fobj, "hidden", i, 0, NULL, &a);
    if (!a) {
      arrayadd(ifarray, &i);
    }
  }

  SelectDialog(&DlgSelect, fobj, NULL, FileCB, farray, ifarray);
  response_callback_add(&DlgSelect, set_file_hidden_response, NULL, data);
  DialogExecute(TopLevel, &DlgSelect);
  return 0;
}
#else
int
SetFileHidden(void)
{
  struct objlist *fobj;
  int lastinst;
  struct narray farray, ifarray;
  int i, a, r;

  fobj = chkobject("data");
  if (fobj == NULL) {
    return 1;
  }

  lastinst = chkobjlastinst(fobj);
  if (lastinst < 0) {
    return 1;
  }

  arrayinit(&ifarray, sizeof(int));
  for (i = 0; i <= lastinst; i++) {
    getobj(fobj, "hidden", i, 0, NULL, &a);
    if (!a) {
      arrayadd(&ifarray, &i);
    }
  }

  r = 0;
  SelectDialog(&DlgSelect, fobj, NULL, FileCB, &farray, &ifarray);
  if (DialogExecute(TopLevel, &DlgSelect) == IDOK) {
    int num, inum, *array;
    a = TRUE;
    for (i = 0; i <= lastinst; i++) {
      putobj(fobj, "hidden", i, &a);
    }
    num = arraynum(&farray);
    array = arraydata(&farray);
    a = FALSE;
    for (i = 0; i < num; i++) {
      putobj(fobj, "hidden", array[i], &a);
    }

    inum = arraynum(&ifarray);
    if (inum != num) {
      set_graph_modified();
    } else {
      for (i = 0; i < num; i++) {
	if (arraynget_int(&ifarray, i) != array[i]) {
	  set_graph_modified();
	  break;
	}
      }
    }
    r = 1;
  }

  arraydel(&ifarray);
  arraydel(&farray);
  return r;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
struct CheckIniFile_data {
  struct objlist *obj;
  int id, modified;
  obj_response_cb cb;
};

static void
CheckIniFile_response(int ret, gpointer user_data)
{
  struct objlist *obj;
  int id, modified;
  obj_response_cb cb;
  struct CheckIniFile_data *data;

  data = (struct CheckIniFile_data *) user_data;
  id = data->id;
  obj = data->obj;
  cb = data->cb;
  modified = data->modified;
  g_free(data);

  if (ret != IDYES) {
    cb(FALSE, obj, id, modified);
    return;
  }
  if (!copyconfig()) {
    message_box(TopLevel, _("Ngraph.ini could not be copied."), "Ngraph.ini", RESPONS_ERROR);
    cb(FALSE, obj, id, modified);
    return;
  }
  cb(TRUE, obj, id, modified);
}

void
CheckIniFile(obj_response_cb cb, struct objlist *obj, int id, int modified)
{
  int ret;

  ret = writecheckconfig();
  if (ret == 0) {
    message_box(TopLevel, _("Ngraph.ini is not found."), "Ngraph.ini", RESPONS_ERROR);
    cb(FALSE, obj, id, modified);
    return;
  } else if ((ret == -1) || (ret == -3)) {
    message_box(TopLevel, _("Ngraph.ini is write protected."), "Ngraph.ini", RESPONS_ERROR);
    cb(FALSE, obj, id, modified);
    return;
  } else if ((ret == -2) || (ret == 2)) {
    struct CheckIniFile_data *data;
    struct objlist *sys;
    char *homedir, *buf;

    sys = getobject("system");
    if (sys == NULL) {
      cb(FALSE, obj, id, modified);
      return;
    }

    if (getobj(sys, "home_dir", 0, 0, NULL, &homedir) == -1) {
      cb(FALSE, obj, id, modified);
      return;
    }

    data = g_malloc0(sizeof(*data));
    if (data == NULL) {
      cb(FALSE, obj, id, modified);
      return;
    }
    data->cb = cb;
    data->obj = obj;
    data->id = id;
    data->modified = modified;

    buf = g_strdup_printf(_("Install `Ngraph.ini' to %s ?"), homedir);
    response_message_box(TopLevel, buf, "Ngraph.ini", RESPONS_YESNO, CheckIniFile_response, data);
    g_free(buf);
  }
}
#else
int
CheckIniFile(void)
{
  int ret;

  ret = writecheckconfig();
  if (ret == 0) {
    message_box(TopLevel, _("Ngraph.ini is not found."), "Ngraph.ini", RESPONS_ERROR);
    return FALSE;
  } else if ((ret == -1) || (ret == -3)) {
    message_box(TopLevel, _("Ngraph.ini is write protected."), "Ngraph.ini", RESPONS_ERROR);
    return FALSE;
  } else if ((ret == -2) || (ret == 2)) {
    struct objlist *sys;
    char *homedir, *buf;

    sys = getobject("system");
    if (sys == NULL) {
      return FALSE;
    }

    if (getobj(sys, "home_dir", 0, 0, NULL, &homedir) == -1) {
      return FALSE;
    }

    buf = g_strdup_printf(_("Install `Ngraph.ini' to %s ?"), homedir);
    if (message_box(TopLevel, buf, "Ngraph.ini", RESPONS_YESNO) == IDYES) {
      g_free(buf);
      if (!copyconfig()) {
	message_box(TopLevel, _("Ngraph.ini could not be copied."), "Ngraph.ini", RESPONS_ERROR);
	return FALSE;
      }
    } else {
      g_free(buf);
      return FALSE;
    }
  }
  return TRUE;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
struct progress_dialog_data {
  GString *msg[2], *title;
  double fraction[2];
  guint tag;
  GThread *thread;
  progress_func update, finalize;
  int finish;
};
static struct progress_dialog_data * ProgressDialogData = NULL;

void
ProgressDialogSetTitle(const char *title)
{
  if (ProgressDialogData) {
    g_string_assign(ProgressDialogData->title, title);
  }
}

static void
show_progress(int pos, const char *msg, double fraction)
{
  int i;

  if (! ProgressDialog)
    return;

  if (! ProgressDialogData)
    return;

  i = (pos) ? 1 : 0;

  g_string_assign(ProgressDialogData->msg[i], msg);
  ProgressDialogData->fraction[i] = fraction;
}

static void
stop_btn_clicked(GtkButton *button, gpointer user_data)
{
  set_interrupt();
}

static gboolean
progress_dialog_update(gpointer user_data)
{
  int i;
  if (! ProgressDialogData  || ProgressDialogData->finish) {
    g_thread_join(ProgressDialogData->thread);
    if (ProgressDialogData->finalize) {
      ProgressDialogData->finalize(user_data);
    }
    ProgressDialogFinalize();
    return FALSE;
  }
  for (i = 0; i < PROGRESSBAR_N; i++) {
    GtkProgressBar *bar;
    double fraction;
    const char *msg, *title;
    bar = ProgressBar[i];
    fraction = ProgressDialogData->fraction[i];
    msg = ProgressDialogData->msg[i]->str;
    title = ProgressDialogData->title->str;
    if (fraction < 0) {
      gtk_progress_bar_pulse(bar);
    } else {
      gtk_progress_bar_set_fraction(bar, fraction);
    }
    gtk_progress_bar_set_text(bar, msg);
    gtk_window_set_title(GTK_WINDOW(ProgressDialog), title);
  }
  return TRUE;
}

static void
create_progress_dialog(const char *title)
{
  GtkWidget *btn, *vbox, *hbox;

  if (TopLevel == NULL)
    return;

  reset_interrupt();

  SaveCursor = NGetCursor();
  NSetCursor(GDK_WATCH);

  set_draw_lock(DrawLockDraw);

  if (ProgressDialog) {
    ProgressDialogSetTitle(title);
    show_progress(0, "", 0);
    show_progress(1, "", 0);
    set_progress_func(show_progress);
    return;
  }

  ProgressDialog = gtk_window_new();
  g_signal_connect(ProgressDialog, "close_request", G_CALLBACK(gtk_true), NULL);
  gtk_window_set_title(GTK_WINDOW(ProgressDialog), title);

  gtk_window_set_transient_for(GTK_WINDOW(ProgressDialog), GTK_WINDOW(TopLevel));
  gtk_window_set_modal(GTK_WINDOW(ProgressDialog), TRUE);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  ProgressBar[0] = GTK_PROGRESS_BAR(gtk_progress_bar_new());
  gtk_progress_bar_set_ellipsize(ProgressBar[0], PANGO_ELLIPSIZE_MIDDLE);
  gtk_progress_bar_set_show_text(ProgressBar[0], TRUE);
  gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(ProgressBar[0]));

  ProgressBar[1] = GTK_PROGRESS_BAR(gtk_progress_bar_new());
  gtk_progress_bar_set_ellipsize(ProgressBar[1], PANGO_ELLIPSIZE_MIDDLE);
  gtk_progress_bar_set_show_text(ProgressBar[1], TRUE);
  gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(ProgressBar[1]));

  btn = gtk_button_new_with_mnemonic(_("_Stop"));
  set_button_icon(btn, "process-stop");
  g_signal_connect(btn, "clicked", G_CALLBACK(stop_btn_clicked), NULL);
#if USE_HEADER_BAR
  hbox = gtk_header_bar_new();
#if ! GTK_CHECK_VERSION(4, 0, 0)
  gtk_header_bar_set_title(GTK_HEADER_BAR(hbox), title);
#endif
  gtk_header_bar_pack_end(GTK_HEADER_BAR(hbox), btn);
  gtk_window_set_titlebar(GTK_WINDOW(ProgressDialog), hbox);
#else
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 4);
#endif
  gtk_box_append(GTK_BOX(vbox), hbox);
  gtk_window_set_child(GTK_WINDOW(ProgressDialog), vbox);

  gtk_window_set_default_size(GTK_WINDOW(ProgressDialog), 400, -1);

  set_progress_func(show_progress);
}

static gpointer
ProgressDialog_thread(gpointer user_data)
{
  if (ProgressDialogData->update) {
    ProgressDialogData->update(user_data);
  }
  return NULL;
}

void
ProgressDialogFinish(void)
{
  if (ProgressDialogData) {
    ProgressDialogData->finish = TRUE;
  }
}

void
ProgressDialogCreate(char *title, progress_func update, progress_func finalize, gpointer data)
{
  int i;
  ProgressDialogData = g_malloc0(sizeof(*ProgressDialogData));
  if (ProgressDialogData == NULL) {
    return;
  }
  for (i = 0; i < PROGRESSBAR_N; i++) {
    ProgressDialogData->msg[i] = g_string_new("");
    ProgressDialogData->fraction[i] = 0;
  }
    ProgressDialogData->title = g_string_new("");
  ProgressDialogData->update = update;
  ProgressDialogData->finalize = finalize;
  create_progress_dialog(title);
  gtk_widget_show(ProgressDialog);
  ProgressDialogData->thread = g_thread_new(NULL, ProgressDialog_thread, data);

  ProgressDialogData->tag = g_timeout_add(100, progress_dialog_update, data);
}

void
ProgressDialogFinalize(void)
{
  int i;
  if (TopLevel == NULL)
    return;

  gtk_widget_hide(ProgressDialog);
  NSetCursor(SaveCursor);
  set_progress_func(NULL);

  for (i = 0; i < PROGRESSBAR_N; i++) {
    g_string_free(ProgressDialogData->msg[i], TRUE);
  }
  g_string_free(ProgressDialogData->title, TRUE);
  g_free(ProgressDialogData);
  ProgressDialogData = NULL;

  set_draw_lock(DrawLockNone);
}
#else
void
ProgressDialogSetTitle(char *title)
{
  if (ProgressDialog)
    gtk_window_set_title(GTK_WINDOW(ProgressDialog), title);
}

static void
show_progress(int pos, const char *msg, double fraction)
{
  GtkProgressBar *bar;

  if (! ProgressDialog)
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

static void
stop_btn_clicked(GtkButton *button, gpointer user_data)
{
  set_interrupt();
}

void
ProgressDialogCreate(char *title)
{
  GtkWidget *btn, *vbox, *hbox;

  if (TopLevel == NULL)
    return;

  reset_interrupt();

  SaveCursor = NGetCursor();
  NSetCursor(GDK_WATCH);

  set_draw_lock(DrawLockDraw);

  if (ProgressDialog) {
    ProgressDialogSetTitle(title);
    show_progress(0, "", 0);
    show_progress(1, "", 0);
    set_progress_func(show_progress);
#if ! GTK_CHECK_VERSION(4, 0, 0)
    gtk_widget_show_all(ProgressDialog);
#endif
    return;
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  ProgressDialog = gtk_window_new();
#else
  ProgressDialog = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
  g_signal_connect(ProgressDialog, "close_request", G_CALLBACK(gtk_true), NULL);
#else
  g_signal_connect(ProgressDialog, "delete-event", G_CALLBACK(gtk_true), NULL);
#endif
  gtk_window_set_title(GTK_WINDOW(ProgressDialog), title);

  gtk_window_set_transient_for(GTK_WINDOW(ProgressDialog), GTK_WINDOW(TopLevel));
  gtk_window_set_modal(GTK_WINDOW(ProgressDialog), TRUE);
#if ! GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_set_position(GTK_WINDOW(ProgressDialog), GTK_WIN_POS_CENTER);
  gtk_window_set_type_hint(GTK_WINDOW(ProgressDialog), GDK_WINDOW_TYPE_HINT_DIALOG);
#endif

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  ProgressBar = GTK_PROGRESS_BAR(gtk_progress_bar_new());
  gtk_progress_bar_set_ellipsize(ProgressBar, PANGO_ELLIPSIZE_MIDDLE);
  gtk_progress_bar_set_show_text(ProgressBar, TRUE);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(ProgressBar));
#else
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(ProgressBar), FALSE, FALSE, 4);
#endif

  ProgressBar2 = GTK_PROGRESS_BAR(gtk_progress_bar_new());
  gtk_progress_bar_set_ellipsize(ProgressBar2, PANGO_ELLIPSIZE_MIDDLE);
  gtk_progress_bar_set_show_text(ProgressBar2, TRUE);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), GTK_WIDGET(ProgressBar2));
#else
  gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(ProgressBar2), FALSE, FALSE, 4);
#endif

  btn = gtk_button_new_with_mnemonic(_("_Stop"));
  set_button_icon(btn, "process-stop");
  g_signal_connect(btn, "clicked", G_CALLBACK(stop_btn_clicked), NULL);
#if USE_HEADER_BAR
  hbox = gtk_header_bar_new();
#if ! GTK_CHECK_VERSION(4, 0, 0)
  gtk_header_bar_set_title(GTK_HEADER_BAR(hbox), title);
#endif
  gtk_header_bar_pack_end(GTK_HEADER_BAR(hbox), btn);
  gtk_window_set_titlebar(GTK_WINDOW(ProgressDialog), hbox);
#else
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  gtk_box_pack_end(GTK_BOX(hbox), btn, FALSE, FALSE, 4);

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), hbox);
#else
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 4);
#endif
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_set_child(GTK_WINDOW(ProgressDialog), vbox);
#else
  gtk_container_add(GTK_CONTAINER(ProgressDialog), vbox);
#endif

  gtk_window_set_default_size(GTK_WINDOW(ProgressDialog), 400, -1);
#if ! GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show_all(ProgressDialog);
#endif

  set_progress_func(show_progress);
}

void
ProgressDialogFinalize(void)
{
  if (TopLevel == NULL)
    return;

  gtk_widget_hide(ProgressDialog);
  NSetCursor(SaveCursor);
  set_progress_func(NULL);
  set_draw_lock(DrawLockNone);
}
#endif

void
ErrorMessage(void)
{
  const char *s;
  char *ptr;

  s = g_strerror(errno);
  ptr = g_strdup(s);
  message_box(NULL, CHK_STR(ptr), _("error"), RESPONS_ERROR);
  g_free(ptr);
}

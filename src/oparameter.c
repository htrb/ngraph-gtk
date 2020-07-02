/*
 * $Id: oparameter.c,v 1.29 2010-03-04 08:30:17 hito Exp $
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

#include "common.h"
#include "object.h"
#include "oparameter.h"

#define NAME "parameter"
#define PARENT "object"
#define NVERSION  "1.00.00"

char *parameter_type[]={
  N_("SpinButton"),
  N_("ScaleButton"),
  N_("CheckButton"),
  N_("ComboBox"),
  N_("Switch"),
  NULL,
};

static char *parameter_errorlist[] = {
};

#define ERRNUM (sizeof(parameter_errorlist) / sizeof(*parameter_errorlist))

static int
parameter_init(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  double min, max, step, value;
  int checked, type;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  min = 0;
  max = 100;
  step = 1;
  value = min;
  checked = FALSE;
  type = PARAMETER_TYPE_SWITCH;

  _putobj(obj, "min", inst, &min);
  _putobj(obj, "max", inst, &max);
  _putobj(obj, "step", inst, &step);
  _putobj(obj, "value", inst, &value);
  _putobj(obj, "checked", inst, &checked);
  _putobj(obj, "type", inst, &type);

  return 0;
}


static int
parameter_done(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;
  return 0;
}

static struct objtable parameter[] = {
  {"init",      NVFUNC,   NEXEC, parameter_init, NULL, 0},
  {"done",      NVFUNC,   NEXEC, parameter_done, NULL, 0},
  {"next",      NPOINTER, 0,              NULL, NULL, 0},
  {"title",     NSTR,     NREAD | NWRITE, NULL, NULL, 0},
  {"type",      NENUM,    NREAD | NWRITE, NULL, parameter_type, 0},
  {"min",       NDOUBLE,  NREAD | NWRITE, NULL, NULL, 0},
  {"max",       NDOUBLE,  NREAD | NWRITE, NULL, NULL, 0},
  {"step",      NDOUBLE,  NREAD | NWRITE, NULL, NULL, 0},
  {"items",     NSTR,     NREAD | NWRITE, NULL, NULL, 0},
  {"redraw",    NBOOL,    NREAD | NWRITE, NULL, NULL, 0},
  {"checked",   NBOOL,    NREAD | NWRITE, NULL, NULL, 0},
  {"selected",  NINT,     NREAD | NWRITE, NULL, NULL, 0},
  {"value",     NDOUBLE,  NREAD | NWRITE, NULL, NULL, 0},
  {"parameter", NDOUBLE,  NREAD,          NULL, NULL, 0},
};

#define TBLNUM (sizeof(parameter) / sizeof(*parameter))

void *
addparameter(void)
{
  return addobject(NAME, NULL, PARENT, NVERSION, TBLNUM, parameter, ERRNUM,
		   parameter_errorlist, NULL, NULL);
}

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
  N_("Scale"),
  N_("CheckButton"),
  N_("ComboBox"),
  N_("Switch"),
  N_("Transition"),
  NULL,
};

static char *parameter_errorlist[] = {
};

#define ERRNUM (sizeof(parameter_errorlist) / sizeof(*parameter_errorlist))

static void set_parameter(struct objlist *obj, N_VALUE *inst, int type);

static int
parameter_init(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  double min, max, step, value;
  int checked, type, wait;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  min = 0;
  max = 100;
  step = 1;
  value = min;
  wait = 10;
  checked = FALSE;
  type = PARAMETER_TYPE_SPIN;

  _putobj(obj, "min", inst, &min);
  _putobj(obj, "max", inst, &max);
  _putobj(obj, "step", inst, &step);
  _putobj(obj, "start", inst, &min);
  _putobj(obj, "stop", inst, &max);
  _putobj(obj, "value", inst, &value);
  _putobj(obj, "wait", inst, &wait);
  _putobj(obj, "active", inst, &checked);
  _putobj(obj, "type", inst, &type);

  set_parameter(obj, inst, type);
  return 0;
}

static void
set_parameter(struct objlist *obj, N_VALUE *inst, int type)
{
  int checked, selected;
  double value, prm;
  switch (type) {
  case PARAMETER_TYPE_SPIN:
  case PARAMETER_TYPE_SCALE:
    _getobj(obj, "value", inst, &value);
    prm = value;
    break;
  case PARAMETER_TYPE_CHECK:
    _getobj(obj, "active", inst, &checked);
    prm = checked;
    break;
  case PARAMETER_TYPE_COMBO:
  case PARAMETER_TYPE_SWITCH:
    _getobj(obj, "selected", inst, &selected);
    prm = selected;
    break;
  case PARAMETER_TYPE_TRANSITION:
    _getobj(obj, "stop", inst, &value);
    prm = value;
    break;
  default:
    return;
  }
  _putobj(obj, "parameter", inst, &prm);
  return;
}

static int
parameter_type_put(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int type;

  type = *(int *)(argv[2]);
  set_parameter(obj, inst, type);
  return 0;
}

static int
parameter_put_wait(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int wait;

  wait = *(int *) (argv[2]);
  if (wait < OPARAMETER_WAIT_MIN) {
    *(int *)(argv[2]) = OPARAMETER_WAIT_MIN;
  } else if (wait > OPARAMETER_WAIT_MAX) {
    *(int *)(argv[2]) = OPARAMETER_WAIT_MAX;
  }
  return 0;
}

static int
parameter_put_step(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  double step;

  step = arg_to_double(argv, 2);
  if (step <= 0) {
    step = 1;
    *(double *)(argv[2]) = step;
  }
  return 0;
}

static int
parameter_put(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int type, checked, selected;
  double value, prm;
  char *field;

  field = argv[1];
  _getobj(obj, "type", inst, &type);
  switch (type) {
  case PARAMETER_TYPE_SPIN:
  case PARAMETER_TYPE_SCALE:
    if (strcmp(field, "value")) {
      return 0;
    }
    value = arg_to_double(argv, 2);
    prm = value;
    break;
  case PARAMETER_TYPE_SWITCH:
  case PARAMETER_TYPE_CHECK:
    if (strcmp(field, "active")) {
      return 0;
    }
    checked = *(int *)(argv[2]);
    prm = checked;
    break;
  case PARAMETER_TYPE_COMBO:
    if (strcmp(field, "selected")) {
      return 0;
    }
    selected = *(int *)(argv[2]);
    prm = selected;
    break;
  case PARAMETER_TYPE_TRANSITION:
    if (strcmp(field, "stop")) {
      return 0;
    }
    value = arg_to_double(argv, 2);
    prm = value;
    break;
  default:
    return 0;
  }
  _putobj(obj, "parameter", inst, &prm);
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
  {"type",      NENUM,    NREAD | NWRITE, parameter_type_put, parameter_type, 0},
  {"min",       NDOUBLE,  NREAD | NWRITE, NULL, NULL, 0},
  {"max",       NDOUBLE,  NREAD | NWRITE, NULL, NULL, 0},
  {"step",      NDOUBLE,  NREAD | NWRITE, parameter_put_step, NULL, 0},
  {"start",     NDOUBLE,  NREAD | NWRITE, NULL, NULL, 0},
  {"stop",      NDOUBLE,  NREAD | NWRITE, parameter_put, NULL, 0},
  {"wait",      NINT,     NREAD | NWRITE, parameter_put_wait, NULL, 0},
  {"wrap",      NBOOL,    NREAD | NWRITE, NULL, NULL, 0},
  {"loop",      NBOOL,    NREAD | NWRITE, NULL, NULL, 0},
  {"items",     NSTR,     NREAD | NWRITE, NULL, NULL, 0},
  {"redraw",    NBOOL,    NREAD | NWRITE, NULL, NULL, 0},
  {"active",    NBOOL,    NREAD | NWRITE, parameter_put, NULL, 0},
  {"selected",  NINT,     NREAD | NWRITE, parameter_put, NULL, 0},
  {"value",     NDOUBLE,  NREAD | NWRITE, parameter_put, NULL, 0},
  {"parameter", NDOUBLE,  NREAD,          NULL, NULL, 0},
};

#define TBLNUM (sizeof(parameter) / sizeof(*parameter))

void *
addparameter(void)
{
  return addobject(NAME, NULL, PARENT, NVERSION, TBLNUM, parameter, ERRNUM,
		   parameter_errorlist, NULL, NULL);
}

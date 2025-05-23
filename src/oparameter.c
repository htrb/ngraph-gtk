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

#include <math.h>

#include "object.h"
#include "oparameter.h"

#define NAME "parameter"
#define PARENT "object"
#define NVERSION  "1.00.00"

char *parameter_type[]={
  N_("SpinButton"),
  N_("Scale"),
  N_("CheckButton"),
  N_("Switch"),
  N_("ComboBox"),
  N_("Transition"),
  NULL,
};

char *transition_init[]={
  N_("start"),
  N_("stop"),
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
  int checked, type, wait, redraw, transition;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  min = 0;
  max = 100;
  step = 1;
  value = min;
  wait = 10;
  checked = FALSE;
  redraw = TRUE;
  type = PARAMETER_TYPE_SPIN;
  transition = TRANSITION_INIT_STOP;

  _putobj(obj, "min", inst, &min);
  _putobj(obj, "max", inst, &max);
  _putobj(obj, "step", inst, &step);
  _putobj(obj, "start", inst, &min);
  _putobj(obj, "stop", inst, &max);
  _putobj(obj, "value", inst, &value);
  _putobj(obj, "wait", inst, &wait);
  _putobj(obj, "active", inst, &checked);
  _putobj(obj, "redraw", inst, &redraw);
  _putobj(obj, "type", inst, &type);
  _putobj(obj, "transition", inst, &transition);

  set_parameter(obj, inst, type);
  return 0;
}

static void
set_parameter(struct objlist *obj, N_VALUE *inst, int type)
{
  int checked, selected, transition;
  double value, prm;
  const char *field;
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
    _getobj(obj, "transition", inst, &transition);
    field = (transition == TRANSITION_INIT_STOP) ? "stop" : "start";
    _getobj(obj, field, inst, &value);
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
parameter_transition_put(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int transition, type;
  const char *field;
  double value;

  _getobj(obj, "type", inst, &type);
  if (type != PARAMETER_TYPE_TRANSITION) {
    return 0;
  }

  transition = *(int *)(argv[2]);
  field = (transition == TRANSITION_INIT_STOP) ? "stop" : "start";
  _getobj(obj, field, inst, &value);
  _putobj(obj, "parameter", inst, &value);
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
parameter_put_items(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *str;

  str = argv[2];
  if (str == NULL) {
    return 0;
  }

  g_strstrip(str);
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
  int type, checked, selected, transition;
  double value, prm;
  const char *field, *transition_field;

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
    _getobj(obj, "transition", inst, &transition);
    transition_field = (transition == TRANSITION_INIT_STOP) ? "stop" : "start";
    if (strcmp(field, transition_field)) {
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
count_items(const char *str)
{
  char **items;
  int num;
  if (str == NULL) {
    return 0;
  }
  items = g_strsplit(str, "\n", -1);
  if (items == NULL) {
    return 0;
  }
  num = g_strv_length(items);
  g_strfreev(items);
  return num;
}

static int
parameter_set(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int type, num;
  double val, min, max;
  char *items;

  val = arg_to_double(argv, 2);
  _getobj(obj, "type", inst, &type);
  switch (type) {
  case PARAMETER_TYPE_SPIN:
  case PARAMETER_TYPE_SCALE:
    _getobj(obj, "min", inst, &min);
    _getobj(obj, "max", inst, &max);
    if (val < min || val > max) {
      return 1;
    }
    break;
  case PARAMETER_TYPE_SWITCH:
  case PARAMETER_TYPE_CHECK:
    val = (val) ? 1 : 0;
    break;
  case PARAMETER_TYPE_COMBO:
    _getobj(obj, "items", inst, &items);
    num = count_items(items);
    val = ceil(val);
    if (val < 0 || val >= num) {
      return 1;
    }
    break;
  case PARAMETER_TYPE_TRANSITION:
    _getobj(obj, "start", inst, &min);
    _getobj(obj, "stop", inst, &max);
    if (min > max) {
      double tmp;
      tmp = min;
      min = max;
      max = tmp;
    }
    if (val < min || val > max) {
      return 1;
    }
    break;
  default:
    return 1;
  }
  _putobj(obj, "parameter", inst, &val);
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
  {"items",     NSTR,     NREAD | NWRITE, parameter_put_items, NULL, 0},
  {"redraw",    NBOOL,    NREAD | NWRITE, NULL, NULL, 0},
  {"active",    NBOOL,    NREAD | NWRITE, parameter_put, NULL, 0},
  {"selected",  NINT,     NREAD | NWRITE, parameter_put, NULL, 0},
  {"value",     NDOUBLE,  NREAD | NWRITE, parameter_put, NULL, 0},
  {"transition",NENUM,    NREAD | NWRITE, parameter_transition_put, transition_init, 0},
  {"set",       NVFUNC,   NREAD | NEXEC,  parameter_set, "d", 0},
  {"parameter", NDOUBLE,  NREAD,          NULL, NULL, 0},
};

#define TBLNUM (sizeof(parameter) / sizeof(*parameter))

void *
addparameter(void)
{
  return addobject(NAME, NULL, PARENT, NVERSION, TBLNUM, parameter, ERRNUM,
		   parameter_errorlist, NULL, NULL);
}

/*
 * $Id: oroot.h,v 1.3 2009-03-09 05:20:30 hito Exp $
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

#ifndef OROOT_HEADER
#define OROOT_HEADER

enum obj_config_type {
  OBJ_CONFIG_TYPE_NUMERIC,
  OBJ_CONFIG_TYPE_STRING,
  OBJ_CONFIG_TYPE_STYLE,
  OBJ_CONFIG_TYPE_OTHER,
};

struct obj_config {
  char *name;
  enum obj_config_type type;
  int (* load_proc)(struct objlist *, N_VALUE *, char *, char *);
  void (* save_proc)(struct objlist *, N_VALUE *, char *, struct narray *);
  int checked;
};

int obj_load_config(struct objlist *obj, N_VALUE *inst, char *title, NHASH hash, int check);
int obj_save_config(struct objlist *obj, N_VALUE *inst, char *title, struct obj_config *config, unsigned int n, int check);

void obj_save_config_string(struct objlist *obj, N_VALUE *inst, char *field, struct narray *conf);

int oputabs(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);
int oputge1(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);
int oputangle(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);
int oputstyle(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);
int oputcolor(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);
int oputmarktype(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv);

#endif

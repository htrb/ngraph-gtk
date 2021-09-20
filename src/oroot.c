/*
 * $Id: oroot.c,v 1.11 2010-03-04 08:30:16 hito Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <glib.h>
#include "object.h"
#include "nstring.h"
#include "oroot.h"
#include "nconfig.h"

#define NAME "object"
#define PARENT NULL
#define OVERSION "1.00.00"

#define ERRILNAME 100

static char *rooterrorlist[]={
  "illegal instance name",
};

#define ERRNUM (sizeof(rooterrorlist) / sizeof(*rooterrorlist))

int
obj_load_config(struct objlist *obj, N_VALUE *inst, char *title, NHASH hash, int check)
{
  FILE *fp;
  char *tok,*str,*s2;
  char *f1,*f2;
  int val;
  char *endptr;
  int len;
  struct obj_config *cfg;
  struct narray *iarray;

  fp = openconfig(title);
  if (fp == NULL)
    return 0;

  while ((tok = getconfig(fp, &str))) {
    s2 = str;
    if (nhash_get_ptr(hash, tok, (void *) &cfg) == 0) {
      cfg->checked = check;
      switch (cfg->type) {
      case OBJ_CONFIG_TYPE_NUMERIC:
	f1 = getitok2(&s2, &len, " \t,");
        if (f1) {
          val = strtol(f1, &endptr, 10);
          if (endptr[0] == '\0') {
            _putobj(obj, cfg->name, inst, &val);
          }
          g_free(f1);
	}
	break;
      case OBJ_CONFIG_TYPE_STRING:
	f1 = getitok2(&s2, &len, "");
	_getobj(obj, cfg->name, inst, &f2);
	g_free(f2);
	_putobj(obj, cfg->name, inst, f1);
	break;
      case OBJ_CONFIG_TYPE_STYLE:
	iarray = arraynew(sizeof(int));
	if (iarray) {
	  while ((f1 = getitok2(&s2, &len, " \t,")) != NULL) {
	    val = strtol(f1, &endptr, 10);
	    if (endptr[0] == '\0')
	      arrayadd(iarray, &val);
	    g_free(f1);
	  }
	  _putobj(obj, tok, inst, iarray);
	}
	break;
     case OBJ_CONFIG_TYPE_OTHER:
	if (cfg->load_proc) {
	  cfg->load_proc(obj, inst, cfg->name, s2);
	}
	break;
      }
    } else {
      fprintf(stderr, "configuration '%s' in section %s is not used.\n", tok, title);
    }
    g_free(tok);
    g_free(str);
  }
  closeconfig(fp);
  return 0;
}

static void
obj_save_config_numeric(struct objlist *obj, N_VALUE *inst, char *field, struct narray *conf)
{
  char *str;
  int val;

  _getobj(obj, field, inst, &val);
  str = g_strdup_printf("%s=%d", field, val);
  if (str) {
    arrayadd(conf, &str);
  }
}

void
obj_save_config_string(struct objlist *obj, N_VALUE *inst, char *field, struct narray *conf)
{
  char *buf, *val;

  _getobj(obj, field, inst, &val);

  buf = g_strdup_printf("%s=%s", field, CHK_STR(val));
  if (buf) {
    arrayadd(conf, &buf);
  }
}

static void
obj_save_config_line_style(struct objlist *obj, N_VALUE *inst, char *field, struct narray *conf)
{
  char *buf;
  int i, j, num;
  struct narray *iarray;

  _getobj(obj, field, inst, &iarray);
  num = arraynum(iarray);

  buf = g_malloc(strlen(field) + 2 + 20 * num);
  if (buf == NULL)
    return;

  j = 0;
  j += sprintf(buf, "%s=", field);
  for (i = 0; i < num; i++) {
    j += sprintf(buf + j, "%d%s", arraynget_int(iarray, i), (i == num - 1) ? "" : " ");
  }
  arrayadd(conf, &buf);
}

int
obj_save_config(struct objlist *obj, N_VALUE *inst, char *title, struct obj_config *config, unsigned int n, int check)
{
  struct narray conf;
  unsigned int i;

  arrayinit(&conf, sizeof(char *));

  for (i = 0; i < n; i++) {
    switch (config[i].type) {
    case OBJ_CONFIG_TYPE_NUMERIC:
      obj_save_config_numeric(obj, inst, config[i].name, &conf);
      break;
    case OBJ_CONFIG_TYPE_STRING:
      obj_save_config_string(obj, inst, config[i].name, &conf);
      break;
    case OBJ_CONFIG_TYPE_STYLE:
      obj_save_config_line_style(obj, inst, config[i].name, &conf);
      break;
    case OBJ_CONFIG_TYPE_OTHER:
      if (config[i].save_proc) {
	config[i].save_proc(obj, inst, config[i].name, &conf);
      }
      break;
    }
  }
  replaceconfig(title, &conf);
  arraydel2(&conf);
  return 0;
}

static int
oinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int
odone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int
oputname(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  char *arg;
  int i;

  if (argc<3) return 0;
  arg=argv[2];
  if (arg==NULL) return 0;
  if (!isalpha(arg[0]) && (arg[0]!='_')) {
    error2(obj,ERRILNAME,arg);
    return 1;
  }
  for (i=1;arg[i]!='\0';i++)
    if (!isalnum(arg[i]) && (strchr("_",arg[i])==NULL)) {
      error2(obj,ERRILNAME,arg);
      return 1;
    }
  return 0;
}

int
oputabs(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (*(int *)(argv[2])<0) *(int *)argv[2]=0;
  return 0;
}

int
oputge1(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (*(int *)(argv[2])<1) *(int *)(argv[2])=1;
  return 0;
}

int
oputangle(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (*(int *)(argv[2])<0) *(int *)(argv[2])=0;
  else if (*(int *)(argv[2])>36000) *(int *)(argv[2])=36000;
  return 0;
}

int
oputcolor(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (*(int *) (argv[2]) < 0) *(int *)(argv[2]) = 0;
  else if (*(int *)(argv[2]) > 255) *(int *)(argv[2]) = 255;
  return 0;
}

int
oputmarktype(struct objlist *obj,N_VALUE *inst,N_VALUE *rval, int argc,char **argv)
{
  int type;

  type = * (int *) (argv[2]);

  if (type < 0 || type >= MARK_TYPE_NUM) {
    * (int *) (argv[2]) = 0;
  }

  return 0;
}

int
oputstyle(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct narray *array;
  int i, num, *adata;

  array = (struct narray *) argv[2];
  num = arraynum(array);
  adata = arraydata(array);
  if (num == 1) {
    array = arrayadd(array, adata);
    if (array == NULL) {
      return 1;
    }
    num++;
  }
  for (i = 0; i < num; i++) {
    adata[i] = abs(adata[i]);
  }
  return 0;
}

#ifdef COMPILE_UNUSED_FUNCTIONS
static void
ochgobjlist(char **olist)
{
  char *list,*objname,*field;
  char *ids,*ids2;
  int id,len;
  struct objlist *obj;
  char *endptr;
  N_VALUE *inst;

  list=*olist;
  objname=getitok2(&list,&len,":");
  ids=getitok2(&list,&len,":");
  field=list;
  if ((objname==NULL) || (ids==NULL)) {
    g_free(objname);
    g_free(ids);
    g_free(*olist);
    *olist=NULL;
    return;
  }
  obj=chkobject(objname);
  g_free(objname);
  if (ids[0]!='^') {
    g_free(ids);
    return;
  }
  ids2=ids+1;
  id=strtol(ids2,&endptr,0);
  if ((ids2[0]=='\0') || (endptr[0]!='\0')) {
    g_free(ids);
    g_free(*olist);
    *olist=NULL;
    return;
  }
  g_free(ids);
  if ((inst=getobjinstoid(obj,id))==NULL) {
    g_free(*olist);
    *olist=NULL;
    return;
  }
  _getobj(obj,"id",inst,&id);
  g_free(*olist);
  *olist=mkobjlist(obj,NULL,id,field,FALSE);
  return;
}
#endif /* COMPILE_UNUSED_FUNCTIONS */

static int
osave(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct objlist *obj2;
  char *field, *name, *valstr;
  char buf[8], *list;
  int i, j, perm;
  struct narray *array;
  int anum;
  char **adata;
  GString *s;

  g_free(rval->str);
  rval->str = NULL;
  array = (struct narray *) argv[2];
  anum = arraynum(array);
  adata = arraydata(array);
  obj2 = getobject(argv[0]);
  if (obj2 == NULL) {
    return 1;
  }
  if (_getobj(obj2, "name", inst, &name)) {
    return 1;
  }

  s = g_string_sized_new(256);
  if (s == NULL) {
    return 1;
  }

  g_string_append(s, "new ");
  g_string_append(s, argv[0]);
  if (name) {
    g_string_append(s, " name:");
    g_string_append(s, name);
  }
  g_string_append_c(s, '\n');
  for (i = 0; i < chkobjfieldnum(obj2); i++) {
    field = chkobjfieldname(obj2,  i);
    for (j = 0; j < anum; j++) {
      if (strcmp0(field, adata[j]) == 0) {
	break;
      }
    }
    if (j == anum) {
      perm = chkobjperm(obj2, field);
      if ((perm & NREAD) && (perm & NWRITE) && strcmp0(field,  "name") != 0) {
        valstr = NULL;
        g_string_append_c(s, '\t');
        g_string_append(s, argv[0]);
        g_string_append(s, "::");
        g_string_append(s, field);
        g_string_append_c(s, '=');
        if (chkobjfieldtype(obj2, field) == NOBJ) {
          if (_getobj(obj2, field, inst, &list)) {
	    goto errexit;
	  }
          list = chgobjlist(list);
	  valstr = getvaluestr(obj2, field, &list, FALSE, TRUE);
          if (valstr == NULL) {
            goto errexit;
	  }
          g_free(list);
        } else {
          if (_getobj(obj2, field, inst, (void *)buf)) {
	    goto errexit;
	  }
	  valstr = getvaluestr(obj2, field, buf, FALSE, TRUE);
          if (valstr == NULL) {
            goto errexit;
	  }
        }
        g_string_append(s, valstr);
        g_string_append_c(s, '\n');
        g_free(valstr);
      }
    }
  }

  rval->str = g_string_free(s, FALSE);

  return 0;

errexit:
  g_string_free(s, TRUE);
  g_free(valstr);
  return 1;
}


static struct objtable objectroot[] = {
  {"init",NVFUNC,NEXEC,oinit,NULL,0},
  {"done",NVFUNC,NEXEC,odone,NULL,0},
  {"id",NINT,NREAD,NULL,NULL,0},
  {"oid",NINT,NREAD,NULL,NULL,0},
  {"name",NSTR,NREAD|NWRITE,oputname,NULL,0},
  {"save",NSFUNC,NREAD|NEXEC,osave,"sa",0},
};

#define TBLNUM (sizeof(objectroot) / sizeof(*objectroot))

void *
addobjectroot(void)
/* addobjectroot() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,objectroot,ERRNUM,rooterrorlist,NULL,NULL);
}

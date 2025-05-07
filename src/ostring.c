/*
 * $Id: ostring.c,v 1.5 2010-03-04 08:30:16 hito Exp $
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
#include <ctype.h>
#include "object.h"
#include "nstring.h"

#define NAME "string"
#define PARENT "object"
#define OVERSION "1.00.00"

#define ERR_INVALID_UTF8	100
#define ERR_REGEXP		101

static char *stringerrorlist[]={
  "invalid UTF-8 string.",
  "invalid regular expression.",
};

#define ERRNUM (sizeof(stringerrorlist) / sizeof(*stringerrorlist))

static int
stringinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static int
stringdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}

static void
set_length(struct objlist *obj, N_VALUE *inst, char *str)
{
  int byte, len;

  if (str) {
    byte = strlen(str);
    len = g_utf8_strlen(str, -1);
  } else {
    byte = 0;
    len = 0;
  }

  _putobj(obj, "byte", inst, &byte);
  _putobj(obj, "length", inst, &len);
}

static int
string_strip(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *str, *tmp;

  if (rval->str) {
    g_free(rval->str);
  }
  rval->str = NULL;

  if (_getobj(obj, "@", inst, &str)) {
    return 1;
  }

  if (str == NULL || str[0] == '\0') {
    return 0;
  }

  tmp = g_strdup(str);
  if (tmp) {
    g_strstrip(tmp);
  }

  rval->str = tmp;

  return 0;
}

static int
string_set(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *str;

  str = argv[2];

  if (str == NULL) {
    set_length(obj, inst, str);
    return 0;
  }

  if (! g_utf8_validate(str, -1, NULL)) {
    error(obj, ERR_INVALID_UTF8);
    return 1;
  }

  set_length(obj, inst, str);

  return 0;
}

static int
string_upcase(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *str;

  if (rval->str) {
    g_free(rval->str);
  }
  rval->str = NULL;

  if (_getobj(obj, "@", inst, &str)) {
    return 1;
  }

  if (str == NULL) {
    return 0;
  }

  rval->str = g_utf8_strup(str, -1);

  return 0;
}

static int
string_downcase(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *str;

  if (rval->str) {
    g_free(rval->str);
  }
  rval->str = NULL;

  if (_getobj(obj, "@", inst, &str)) {
    return 1;
  }

  if (str == NULL) {
    return 0;
  }

  rval->str = g_utf8_strdown(str, -1);

  return 0;
}

static int
string_reverse(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *str;

  if (rval->str) {
    g_free(rval->str);
  }
  rval->str = NULL;

  if (_getobj(obj, "@", inst, &str)) {
    return 1;
  }

  if (str == NULL) {
    return 0;
  }

  rval->str = g_utf8_strreverse(str, -1);

  return 0;
}

static char *
utf8_string_slice(const char *str, int size, int byte_size, int start, int len)
{
  char *data, *ptr;

  if (str == NULL || size < 1 || len <= 0) {
    return NULL;
  }

  if (start < 0) {
    start = size + start;
  }

  if (start < 0) {
    return NULL;
  }

  if (start >= size) {
    return NULL;
  }

  if (start + len > size) {
    len = size - start;
  }

  data = g_malloc(byte_size + 1);
  if (data == NULL) {
    return NULL;
  }

  ptr = g_utf8_offset_to_pointer(str, start);
  g_utf8_strncpy(data, ptr, len);

  return data;
}

static int
string_slice(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char *str;
  int start, size, byte_size, len;

  start = * (int *) argv[2];
  len = * (int *) argv[3];

  if (rval->str) {
    g_free(rval->str);
  }
  rval->str = NULL;

  if (_getobj(obj, "@", inst, &str)) {
    return 1;
  }

  if (_getobj(obj, "byte", inst, &byte_size)) {
    return 1;
  }

  if (_getobj(obj, "length", inst, &size)) {
    return 1;
  }

  if (str == NULL || len < 1) {
    return 0;
  }

  rval->str = utf8_string_slice(str, size, byte_size, start, len);

  return 0;
}

static int
string_replace(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  GRegex *regexp;
  char *str, *pattern, *replace;

  g_free(rval->str);
  rval->str = NULL;

  pattern = (char *) argv[2];
  replace = (char *) argv[3];
  if (pattern == NULL || pattern[0] == '\0') {
    return 0;
  }

  if (replace == NULL) {
    replace = "";
  }

  if (! g_utf8_validate(pattern, -1, NULL)) {
    error(obj, ERR_INVALID_UTF8);
    return 1;
  }

  if (! g_utf8_validate(replace, -1, NULL)) {
    error(obj, ERR_INVALID_UTF8);
    return 1;
  }

  if (_getobj(obj, "@", inst, &str)) {
    return 1;
  }

  if(str == NULL || str[0] == '\0') {
    return 0;
  }

  regexp = g_regex_new(pattern, 0, 0, NULL);
  if (regexp == NULL) {
    error(obj, ERR_REGEXP);
    return 1;
  }

  rval->str = g_regex_replace(regexp, str, -1, 0, replace, 0, NULL);

  g_regex_unref(regexp);

  return 0;
}

static int
string_index(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int pos, size;
  char *str, *pattern, *ptr, *find;

  rval->i = -1;

  pattern = (char *) argv[2];
  pos = * (int *) argv[3];

  if (pattern == NULL || pattern[0] == '\0') {
    return 1;
  }

  if (_getobj(obj, "length", inst, &size)) {
    return 2;
  }

  if (! g_utf8_validate(pattern, -1, NULL)) {
    error(obj, ERR_INVALID_UTF8);
    return 2;
  }

  if (pos < 0) {
    pos += size;
  }

  if (pos < 0 || pos >= size) {
    return 1;
  }

  if (_getobj(obj, "@", inst, &str)) {
    return 1;
  }

  if(str == NULL || str[0] == '\0') {
    return 1;
  }

  ptr = g_utf8_offset_to_pointer(str, pos);

  find = g_strstr_len(ptr, -1, pattern);
  if (find == NULL) {
    return 1;
  }

  rval->i = g_utf8_pointer_to_offset(str, find);

  return 0;
}

static int
string_rindex(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  int pos, size, len;
  char *str, *pattern, *find;
  const char *ptr;

  rval->i = -1;

  pattern = (char *) argv[2];
  pos = * (int *) argv[3];

  if (pattern == NULL || pattern[0] == '\0') {
    return 1;
  }

  if (_getobj(obj, "length", inst, &size)) {
    return 2;
  }

  if (! g_utf8_validate(pattern, -1, NULL)) {
    error(obj, ERR_INVALID_UTF8);
    return 2;
  }

  if (pos < 0) {
    pos += size;
  }

  if (pos < 0 || pos >= size) {
    return 1;
  }

  if (_getobj(obj, "@", inst, &str)) {
    return 1;
  }

  if(str == NULL || str[0] == '\0') {
    return 1;
  }

  ptr = g_utf8_offset_to_pointer(str, pos);
  len = ptr - str + 1;
  if (len < 1) {
    return 1;
  }

  find = g_strrstr_len(str, len, pattern);
  if (find == NULL) {
    return 1;
  }

  rval->i = g_utf8_pointer_to_offset(str, find);

  return 0;
}

static int
string_match(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  GRegex *regexp;
  char *str, *pattern;

  pattern = (char *) argv[2];

  rval->i = 0;

  if (! g_utf8_validate(pattern, -1, NULL)) {
    error(obj, ERR_INVALID_UTF8);
    return 1;
  }

  _getobj(obj, "@", inst, &str);
  if (str == NULL || str[0] == '\0') {
    return 0;
  }

  regexp = g_regex_new(pattern, 0, 0, NULL);
  if (regexp == NULL) {
    error(obj, ERR_REGEXP);
    return 1;
  }

  rval->i = g_regex_match(regexp, str, 0, NULL);

  g_regex_unref(regexp);

  return 0;
}

static struct objtable ostring[] = {
  {"init",NVFUNC,NEXEC,stringinit,NULL,0},
  {"done",NVFUNC,NEXEC,stringdone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"@",NSTR,NREAD|NWRITE,string_set,NULL,0},
  {"length",NINT,NREAD,NULL,NULL,0},
  {"byte",NINT,NREAD,NULL,NULL,0},
  {"strip",NSFUNC,NREAD|NEXEC,string_strip,"",0},
  {"upcase",NSFUNC,NREAD|NEXEC,string_upcase,"",0},
  {"downcase",NSFUNC,NREAD|NEXEC,string_downcase,"",0},
  {"reverse",NSFUNC,NREAD|NEXEC,string_reverse,"",0},
  {"slice",NSFUNC,NREAD|NEXEC,string_slice,"ii",0},
  {"match",NBFUNC,NREAD|NEXEC,string_match,"s",0},
  {"replace",NSFUNC,NREAD|NEXEC,string_replace,"ss",0},
  {"index",NIFUNC,NREAD|NEXEC,string_index,"si",0},
  {"rindex",NIFUNC,NREAD|NEXEC,string_rindex,"si",0},
};

#define TBLNUM (sizeof(ostring) / sizeof(*ostring))

void *addstring()
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,ostring,ERRNUM,stringerrorlist,NULL,NULL);
}

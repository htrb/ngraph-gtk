#include "common.h"

#include <stdlib.h>
#include <ctype.h>
#include <string.h>

#include "ngraph.h"
#include "object.h"

#define NAME "regexp"
#define PARENT "object"
#define OVERSION "1.00.00"

#define ERR_REGEXP		100
#define ERR_INVALID_UTF8	101

static char *regexperrorlist[]={
  "invalid regular expression.",
  "invalid UTF-8 string.",
};

struct oregexp_local {
  GRegex *regexp;
  struct narray *array;
};

#define ERRNUM (sizeof(regexperrorlist) / sizeof(*regexperrorlist))

static void
del_array_element(struct narray *array)
{
  int i, n;
  char ***data;

  n = arraynum(array);
  data = arraydata(array);

  for (i = 0; i < n; i++) {
    g_strfreev(data[i]);
  }

  arraydel(array);
}

static int 
regexpinit(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct oregexp_local *local;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  local = g_malloc0(sizeof(*local));
  if (local == NULL) {
    return 1;
  }

  local->array = arraynew(sizeof(char **));
  if (local->array == NULL) {
    g_free(local);
    return 1;
  }

  if (_putobj(obj, "_local", inst, local)) {
    g_free(local);
    return 1;
  }

  return 0;
}

static int 
regexpdone(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct oregexp_local *local;

  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;

  _getobj(obj, "_local", inst, &local);

  if (local->regexp) {
    g_regex_unref(local->regexp);
  }

  if (local->array) {
    del_array_element(local->array);
    arrayfree(local->array);
  }

  return 0;
}

static int 
regexp_set(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct oregexp_local *local;
  GRegex *regexp;
  char *str;

  _getobj(obj, "_local", inst, &local);

  str = (char *) argv[2];
  if (str == NULL || str[0] == '\0') {
    if (local->regexp) {
      g_regex_unref(local->regexp);
    }
    local->regexp = NULL;

    del_array_element(local->array);

    return 0;
  }

  if (! g_utf8_validate(str, -1, NULL)) {
    error(obj, ERR_INVALID_UTF8);
    return 1;
  }

  regexp = g_regex_new(str, 0, 0, NULL);
  if (regexp == NULL) {
    error(obj, ERR_REGEXP);
    return 1;
  }

  if (local->regexp) {
    g_regex_unref(local->regexp);
  }

  local->regexp = regexp;

  del_array_element(local->array);

  return 0;
}

static int 
regexp_get(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct oregexp_local *local;
  int i, j, n;
  char ***strvp, **strv;

  i = * (int *) argv[2];
  j = * (int *) argv[3];

  g_free(rval->str);
  rval->str = NULL;

  if (i < 0) {
    return 1;
  }

  _getobj(obj, "_local", inst, &local);

  strvp = arraynget(local->array, i);
  if (strvp == NULL) {
    return 1;
  }

  strv = *strvp;

  for (n = 0; strv[n]; n++) {
    if (n == j) {
      rval->str = g_strdup(strv[n]);
      return 0;
    }
  }

  return 1;
}

static int 
regexp_num(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct oregexp_local *local;

  rval->i = 0;

  _getobj(obj, "_local", inst, &local);
  rval->i = arraynum(local->array);

  return 0;
}

static int 
regexp_match(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct oregexp_local *local;
  GMatchInfo *info;
  int r;
  char *str, **match;

  str = (char *) argv[2];

  rval->i = 0;

  _getobj(obj, "_local", inst, &local);
  if (local->regexp == NULL) {
    return 1;
  }

  del_array_element(local->array);

  if (str == NULL || str[0] == '\0') {
    return 1;
  }

  info = NULL;
  r = g_regex_match(local->regexp, str, 0, &info);
  if (! r) {
    g_match_info_free(info);
    return 1;
  }

  while (g_match_info_matches(info)) {
    match = g_match_info_fetch_all(info);
    arrayadd(local->array, &match);
    g_match_info_next(info, NULL);
  }
  g_match_info_free(info);

  rval->i = arraynum(local->array);

  return 0;
}

static int 
regexp_replace(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv)
{
  struct oregexp_local *local;
  char *str, *replace;

  g_free(rval->str);
  rval->str = NULL;

  str = (char *) argv[2];
  replace = (char *) argv[3];
  if (str == NULL || str[0] == '\0') {
    return 0;
  }

  if (replace == NULL) {
    replace = "";
  }

  if (! g_utf8_validate(str, -1, NULL)) {
    error(obj, ERR_INVALID_UTF8);
    return 1;
  }

  if (! g_utf8_validate(replace, -1, NULL)) {
    error(obj, ERR_INVALID_UTF8);
    return 1;
  }

  _getobj(obj, "_local", inst, &local);
  if (local->regexp == NULL) {
    return 1;
  }

  rval->str = g_regex_replace(local->regexp, str, -1, 0, replace, 0, NULL);

  return 0;
}

int 
regexp_seq(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct oregexp_local *local;
  GString *str;
  int i, n;

  g_free(rval->str);
  rval->str = NULL;

  _getobj(obj, "_local", inst, &local);

  n = arraynum(local->array);
  if (n == 0) {
    return 0;
  }

  str = g_string_sized_new(64);
  if (str == NULL) {
    return 0;
  }

  for (i = 0; i < n; i++) {
    g_string_append_printf(str, "%d%s", i, (i == n - 1) ? "" : " ");
  }

  rval->str = g_string_free(str,  FALSE);

  return 0;
}

int 
regexp_rseq(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct oregexp_local *local;
  GString *str;
  int i, n;

  g_free(rval->str);
  rval->str = NULL;

  _getobj(obj, "_local", inst, &local);

  n = arraynum(local->array);
  if (n == 0) {
    return 0;
  }

  str = g_string_sized_new(64);
  if (str == NULL) {
    return 0;
  }

  for (i = 0; i < n; i++) {
    g_string_append_printf(str, "%d%s", n - i - 1, (i == n - 1) ? "" : " ");
  }

  rval->str = g_string_free(str,  FALSE);

  return 0;
}

static struct objtable oregexp[] = {
  {"init",  NVFUNC,  NEXEC,       regexpinit,  NULL, 0},
  {"done",  NVFUNC,  NEXEC,       regexpdone,  NULL, 0},
  {"next",  NPOINTER,0,           NULL,        NULL, 0},
  {"@",     NSTR,    NREAD|NWRITE,regexp_set,  NULL, 0},
  {"match", NIFUNC,  NREAD|NEXEC, regexp_match,"s",  0},
  {"replace", NSFUNC,NREAD|NEXEC, regexp_replace,"ss",0},
  {"get",   NSFUNC,  NREAD|NEXEC, regexp_get,  "ii", 0},
  {"num",   NIFUNC,  NREAD|NEXEC, regexp_num,  NULL, 0},
  {"seq",   NSFUNC,  NREAD|NEXEC, regexp_seq,  NULL, 0},
  {"rseq",  NSFUNC,  NREAD|NEXEC, regexp_rseq, NULL, 0},
  {"_local",NPOINTER,0,           NULL,        NULL, 0},
};

#define TBLNUM (sizeof(oregexp) / sizeof(*oregexp))

void *
addregexp(void)
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,oregexp,ERRNUM,regexperrorlist,NULL,NULL);
}

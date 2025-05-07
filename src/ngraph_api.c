#include "common.h"

#include <stdlib.h>
#include "gtk/init.h"
#include "ngraph.h"
#include "object.h"
#include "mathfn.h"
#include "shell.h"
#include "osystem.h"

union ngraph_val {
  int i;
  double d;
  char *str;
  struct narray *ary;
};

static struct narray *
allocate_iarray(ngraph_arg *arg)
{
  struct narray *array;
  int i;

  if (arg == NULL) {
    return NULL;
  }

  if (arg->num < 1) {
    return NULL;
  }

  array = arraynew(sizeof(int));
  if (array == NULL) {
    return NULL;
  }

  for (i = 0; i < arg->num; i++) {
    const struct narray *ptr;
    ptr = arrayadd(array, &arg->ary[i].i);
    if (ptr == NULL) {
      arrayfree(array);
      return NULL;
    }
  }

  return array;
}

static struct narray *
allocate_darray(ngraph_arg *arg)
{
  struct narray *array;
  int i;

  if (arg == NULL) {
    return NULL;
  }

  if (arg->num < 1) {
    return NULL;
  }

  array = arraynew(sizeof(double));
  if (array == NULL) {
    return NULL;
  }

  for (i = 0; i < arg->num; i++) {
    const struct narray *ptr;
    ptr = arrayadd(array, &arg->ary[i].d);
    if (ptr == NULL) {
      arrayfree(array);
      return NULL;
    }
  }

  return array;
}

static struct narray *
allocate_sarray(ngraph_arg *arg)
{
  struct narray *array;
  int i;

  if (arg == NULL) {
    return NULL;
  }

  if (arg->num < 1) {
    return NULL;
  }

  array = arraynew(sizeof(char *));
  if (array == NULL) {
    return NULL;
  }

  for (i = 0; i < arg->num; i++) {
    const struct narray *ptr;
    ptr = arrayadd2(array, arg->ary[i].str);
    if (ptr == NULL) {
      arrayfree(array);
      return NULL;
    }
  }

  return array;
}

int
ngraph_object_put(struct objlist *obj, const char *vname, int id, ngraph_value *val)
{
  enum ngraph_object_field_type type;
  int r;
  void *valp;
  struct narray *array;

  r = -1;

  type = chkobjfieldtype(obj, vname);
  switch (type) {
  case NVOID:
#ifdef USE_LABEL
  case NLABEL:
#endif
  case NVFUNC:
    valp = NULL;
    r = putobj(obj, vname, id, valp);
    break;
  case NSTR:
  case NOBJ:
    if (val->str) {
      valp = g_strdup(val->str);
    } else {
      valp = NULL;
    }
    r = putobj(obj, vname, id, valp);
    if (r < 0 && valp) {
      g_free(valp);
    }
    break;
  case NPOINTER:		/* these fields may not be writable */
  case NBFUNC:
  case NIFUNC:
  case NDFUNC:
  case NSFUNC:
  case NIAFUNC:
  case NDAFUNC:
  case NSAFUNC:
    valp = NULL;
    r = putobj(obj, vname, id, valp);
    break;
  case NBOOL:
  case NINT:
  case NENUM:
    valp = &val->i;
    r = putobj(obj, vname, id, valp);
    break;
  case NDOUBLE:
    valp = &val->d;
    r = putobj(obj, vname, id, valp);
    break;
  case NIARRAY:
    array = allocate_iarray(val->ary);
    r = putobj(obj, vname, id, array);
    if (r < 0) {
      arrayfree(array);
    }
    break;
  case NDARRAY:
    array = allocate_darray(val->ary);
    r = putobj(obj, vname, id, array);
    if (r < 0) {
      arrayfree(array);
    }
    break;
  case NSARRAY:
    array = allocate_sarray(val->ary);
    r = putobj(obj, vname, id, array);
    if (r < 0) {
      arrayfree2(array);
    }
    break;
  }

  return r;
}

static const char **
allocate_obj_arg(struct objlist *obj, const char *vname, ngraph_arg *arg)
{
  int i, num;
  const char *arglist;
  const char **ary;

  num = arg->num;
  if (num < 1) {
    /* If the type of the field is NENUM the number of the argument is 0. */
    return NULL;
  }

  arglist = chkobjarglist(obj, vname);
  if (arglist && arglist[0] == '\0') {
    return NULL;
  }

  ary = g_malloc0(sizeof(*ary) * (num + 1));
  if (ary == NULL) {
    return NULL;
  }

  if (arglist == NULL) {
    for (i = 0; i < num; i++) {
      ary[i] = arg->ary[i].str;
    }
  } else {
    int n;
    n = 0;
    for (i = 0; arglist[i]; i++) {
      int is_a;
      if (n >= num) {
	break;
      }

      is_a = (arglist[i + 1]== 'a');

      switch (arglist[i]) {
      case 'b':
      case 'c':
	ary[n] = (char *) &arg->ary[n].i;
	break;
      case 'i':
	if (is_a) {
	  ary[n] = (char *) allocate_iarray(arg->ary[n].ary);
	} else {
	  ary[n] = (char *) &arg->ary[n].i;
	}
	break;
      case 'd':
	if (is_a) {
	  ary[n] = (char *) allocate_darray(arg->ary[n].ary);
	} else {
	  ary[n] = (char *) &arg->ary[n].d;
	}
	break;
      case 's':
	if (is_a) {
	  ary[n] = (char *) allocate_sarray(arg->ary[n].ary);
	} else {
	  ary[n] = arg->ary[n].str;
	}
	break;
      case 'p':
	ary[n] = NULL;
	break;
      case 'o':
	ary[n] = arg->ary[n].str;
	break;
      }

      if (is_a) {
	i++;
      }
      n++;
    }
  }

  return ary;
}

static void
free_obj_arg(const char **ary, struct objlist *obj, const char *vname, const ngraph_arg *arg)
{
  int i, n, num;
  const char *arglist;

  num = arg->num;
  if (num < 1) {
    return;
  }

  arglist = chkobjarglist(obj, vname);
  if (arglist == NULL) {
    return;
  }

  n = 0;
  for (i = 0; arglist[i]; i++) {
    int is_a;
    if (n >= num) {
      break;
    }

    is_a = (arglist[i + 1]== 'a');

    switch (arglist[i]) {
    case 'b':
    case 'c':
      break;
    case 'i':
    case 'd':
      if (is_a) {
	arrayfree((struct narray *) ary[n]);
      }
      break;
    case 's':
      if (is_a) {
	arrayfree2((struct narray *) ary[n]);
      }
      break;
    case 'p':
      break;
    case 'o':
      break;
    }

    if (is_a) {
      i++;
    }
    n++;
  }

  g_free(ary);

  return;
}

int
ngraph_object_get(struct objlist *obj, const char *vname, int id, ngraph_arg *arg, ngraph_returned_value *val)
{
  int r, argc;
  enum ngraph_object_field_type type;
  const char **argv;
  union ngraph_val nval;

  type = chkobjfieldtype(obj, vname);

  argc = arg->num;
  argv = allocate_obj_arg(obj, vname, arg);

  r = getobj(obj, vname, id, argc, (char **) argv, &nval);

  free_obj_arg(argv, obj, vname, arg);
  if (r < 0) {
    return r;
  }

  switch (type) {
  case NVOID:
#ifdef USE_LABEL
  case NLABEL:
#endif
  case NVFUNC:
    break;
  case NSFUNC:
  case NSTR:
  case NOBJ:
    val->str = nval.str;
    break;
  case NPOINTER:		/* these fields may not be writable */
    break;
  case NBFUNC:
  case NIFUNC:
  case NBOOL:
  case NINT:
  case NENUM:
    val->i = nval.i;
    break;
  case NDFUNC:
  case NDOUBLE:
    val->d = nval.d;
    break;
  case NIAFUNC:
  case NIARRAY:
    val->ary.num = arraynum(nval.ary);
    val->ary.data.ia = arraydata(nval.ary);
    break;
  case NDAFUNC:
  case NDARRAY:
    val->ary.num = arraynum(nval.ary);
    val->ary.data.da = arraydata(nval.ary);
    break;
  case NSAFUNC:
  case NSARRAY:
    val->ary.num = arraynum(nval.ary);
    val->ary.data.sa = arraydata(nval.ary);
    break;
  }

  return r;
}

int
ngraph_object_exe(struct objlist *obj, const char *vname, int id, ngraph_arg *arg)
{
  int r, argc;
  enum ngraph_object_field_type type;
  const char **argv;

  type = chkobjfieldtype(obj, vname);
  if (type < NVFUNC) {
    return -1;
  }

  argc = arg->num;
  argv = allocate_obj_arg(obj, vname, arg);

  r = exeobj(obj, vname, id, argc, (char **) argv);

  free_obj_arg(argv, obj, vname, arg);

  return r;
}

struct objlist *
ngraph_get_object(const char *name)
{
  return getobject(name);
}

struct objlist *
ngraph_get_object_instances_by_str(const char *str, int *n, int **ids)
{
  struct narray iarray;
  int *id_ary, anum, i, r;
  const int *adata;
  struct objlist *obj;

  if (n) {
    *n = 0;
  }

  if (ids == NULL) {
    return NULL;
  }

  *ids = NULL;

  if (str == NULL) {
    return NULL;
  }

  arrayinit(&iarray,sizeof(int));
  r = chkobjilist((char *) str, &obj, &iarray, TRUE, NULL);
  if (r) {
    arraydel(&iarray);
    return NULL;
  }

  anum = arraynum(&iarray);
  adata = arraydata(&iarray);

  id_ary = g_malloc(sizeof(*id_ary) * (anum + 1));
  if (id_ary == NULL) {
    arraydel(&iarray);
    return NULL;
  }

  for (i = 0; i < anum; i++) {
    id_ary[i] = adata[i];
  }
  id_ary[i] = -1;
  arraydel(&iarray);

  if (n) {
    *n = anum;
  }

  *ids = id_ary;

  return obj;
}

int
ngraph_object_get_id_by_oid(struct objlist *obj, int oid)
{
  return chkobjoid(obj, oid);
}

int
ngraph_object_move_top(struct objlist *obj, int id)
{
  return movetopobj(obj, id);
}

int
ngraph_object_move_last(struct objlist *obj, int id)
{
  return movelastobj(obj, id);
}

int
ngraph_object_move_up(struct objlist *obj, int id)
{
  return moveupobj(obj, id);
}

int
ngraph_object_move_down(struct objlist *obj, int id)
{
  return movedownobj(obj, id);
}

int
ngraph_object_exchange(struct objlist *obj, int id1, int id2)
{
  return exchobj(obj, id1, id2);
}

int
ngraph_object_copy(struct objlist *obj, int dist, int src)
{
  return copy_obj_field(obj, dist, src, NULL);
}

int
ngraph_object_new(struct objlist *obj)
{
  return newobj(obj);
}

int
ngraph_object_del(struct objlist *obj, int id)
{
  return delobj(obj, id);
}

int
ngraph_object_exist(struct objlist *obj, int id)
{
  int last;

  if (obj == NULL) {
    return -1;
  }

  last = chkobjlastinst(obj);
  if (id < 0 || id > last) {
    return -1;
  }

  return id;
}

const char *
ngraph_get_object_name(struct objlist *obj)
{
  return chkobjectname(obj);
}

const char *
ngraph_get_object_alias(struct objlist *obj)
{
  return chkobjectalias(obj);
}

int
ngraph_get_object_field_num(struct objlist *obj)
{
  return chkobjfieldnum(obj);
}

const char *
ngraph_get_object_field(struct objlist *obj, int i)
{
  return chkobjfieldname(obj, i);
}

int
ngraph_get_object_field_permission(struct objlist *obj, const char *field)
{
  return chkobjperm(obj, field);
}

enum ngraph_object_field_type
ngraph_get_object_field_type(struct objlist *obj, const char *field)
{
  return chkobjfieldtype(obj, field);
}

const char *
ngraph_get_object_field_args(struct objlist *obj, const char *field)
{
  return chkobjarglist(obj, field);
}

struct objlist *
ngraph_get_parent_object(struct objlist *obj)
{
  return chkobjparent(obj);
}

struct objlist *
ngraph_get_root_object(void)
{
  return chkobjroot();
}

const char *
ngraph_get_object_version(const struct objlist *obj)
{
  return chkobjver(obj);
}

int
ngraph_get_object_id(const struct objlist *obj)
{
  return chkobjectid(obj);
}

int
ngraph_get_object_size(const struct objlist *obj)
{
  return chkobjsize(obj);
}

int
ngraph_get_object_current_id(const struct objlist *obj)
{
  return chkobjcurinst(obj);
}

int
ngraph_get_object_last_id(const struct objlist *obj)
{
  return chkobjlastinst(obj);
}

struct objlist *
ngraph_get_next_object(const struct objlist *obj)
{
  return obj->next;
}

struct objlist *
ngraph_get_child_object(const struct objlist *obj)
{
  return obj->child;
}

int
ngraph_puts(const char *s)
{
  return putstdout(s);
}

int
ngraph_err_puts(const char *s)
{
  return putstderr(s);
}

void
ngraph_sleep(double t)
{
  nsleep(t);
}

char *
ngraph_get_init_file(const char *init_file)
{
  char *homedir, *confdir, *inifile;;
  struct objlist *sys;

  if (init_file == NULL) {
    return NULL;
  }

  sys = chkobject("system");
  getobj(sys, "home_dir", 0, 0, NULL, &homedir);
  getobj(sys, "conf_dir", 0, 0, NULL, &confdir);

  inifile = NULL;
  if (findfilename(homedir, CONFTOP, init_file)) {
    inifile = getfilename(homedir, CONFTOP, init_file);
  } else if (findfilename(confdir, CONFTOP, init_file)) {
    inifile = getfilename(confdir, CONFTOP, init_file);
  }

  return inifile;
}

int
ngraph_exec_loginshell(char *loginshell, struct objlist *obj, int id)
{
  int r, allocnow;
  struct objlist *lobj;
  struct narray iarray;
  char *arg;

  if (loginshell == NULL) {
    allocnow = nallocconsole();
    r = exeobj(obj, "shell", id, 0, NULL);
    if (allocnow) {
      nfreeconsole();
    }
  } else {
    arrayinit(&iarray, sizeof(int));
    arg = loginshell;
    if (getobjilist2(&arg, &lobj, &iarray, TRUE)) {
      return -1;
    }
    arraydel(&iarray);
    if (lobj == obj) {
      allocnow = nallocconsole();
    } else {
      allocnow = FALSE;
    }
    r = sexeobj(loginshell);
    if (allocnow) {
      nallocconsole();
    }
  }

  return r;
}

int
ngraph_initialize(int *argc, char ***argv)
{
  return n_initialize(argc, argv);
}

void
ngraph_save_shell_history(void)
{
  n_save_shell_history();
}

void
ngraph_finalize(void)
{
  n_finalize();
}

void *
ngraph_malloc(size_t size)
{
  return g_malloc(size);
}

void
ngraph_free(void *ptr)
{
  return g_free(ptr);
}

char *
ngraph_strdup(const char *str)
{
  return g_strdup(str);
}

int
ngraph_set_exec_func(const char *name, ngraph_plugin_exec func)
{
  return system_set_exec_func(name, func);
}

#include <ruby.h>
#include <ruby/encoding.h>
#include <stdio.h>
#include <ctype.h>

#ifdef HAVE_ALLOCA_H
# include <alloca.h>
#elif defined __GNUC__
# define alloca __builtin_alloca
#elif defined _AIX
# define alloca __alloca
#elif defined _MSC_VER
# include <malloc.h>
# define alloca _alloca
#else
# include <stddef.h>
# ifdef  __cplusplus
extern "C"
# endif
void *alloca (size_t);
#endif

#include "config.h"
#include "../src/ngraph_plugin.h"

#ifdef TRUE
#undef TRUE
#endif
#define TRUE 1

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

static int Initialized = FALSE;
static VALUE NgraphClass, NgraphModule;
static ID Uniq, Argv;

static char *DummyArgv[] = {"ngraph_ruby", NULL};
static char **DummyArgvPtr = DummyArgv;
static int DummyArgc = 1;

#define VAL2INT(val) (NIL_P(val) ? 0 : NUM2INT(val))
#define VAL2DBL(val) (NIL_P(val) ? 0.0 : NUM2DBL(val))
#define VAL2STR(val) (NIL_P(val) ? NULL : StringValueCStr(val))

struct ngraph_instance {
  int id, oid, rcode;
  struct objlist *obj;
};

struct obj_ids {
  int *ids, num;
  struct objlist *obj;
};

static VALUE get_ngraph_obj(const char *name);
static VALUE str2inst_get_ary(VALUE data1);
static VALUE str2inst_ensure(VALUE data2);
static VALUE obj_get(VALUE klass, VALUE id_value, const char *name);

static void
ngraph_object_free(struct ngraph_instance *inst)
{
  free(inst);
}

static VALUE
ngraph_inst_method_equal(VALUE klass1, VALUE klass2)
{
  struct ngraph_instance *inst1, *inst2;

  if (! rb_obj_is_kind_of(klass2, NgraphClass)) {
    return Qfalse;
  }

  Data_Get_Struct(klass1, struct ngraph_instance, inst1);
  Data_Get_Struct(klass2, struct ngraph_instance, inst2);

  if (inst1->obj == inst2->obj &&
      inst1->oid == inst2->oid) {
    return Qtrue;
  }

  return Qfalse;
}

static VALUE
ngraph_inst_method_compare(VALUE klass1, VALUE klass2)
{
  struct ngraph_instance *inst1, *inst2;
  int r;

  if (! rb_obj_is_kind_of(klass2, NgraphClass)) {
    return Qnil;
  }

  Data_Get_Struct(klass1, struct ngraph_instance, inst1);
  Data_Get_Struct(klass2, struct ngraph_instance, inst2);

  if (inst1->obj != inst2->obj) {
    return Qnil;
  }

  if (inst1->oid == inst2->oid) {
    r = 0;
  } else if (inst1->oid > inst2->oid) {
    r = 1;
  } else {
    r = -1;
  }

  return INT2FIX(r);
}

static struct ngraph_instance *
check_id(VALUE klass)
{
  struct ngraph_instance *inst;
  int id, r, last;
  ngraph_arg arg;
  ngraph_returned_value oid;

  Data_Get_Struct(klass, struct ngraph_instance, inst);

  if (inst->id < 0) {
    rb_raise(rb_eArgError, "%s: the instance is already deleted.", rb_obj_classname(klass));
  }

  last = ngraph_plugin_get_obj_last_id(inst->obj);
  if (inst->id <= last) {
    arg.num = 0;
    r = ngraph_plugin_getobj(inst->obj, "oid", inst->id, &arg, &oid);

    if (r >= 0 && inst->oid == oid.i) {
      return inst;
    }

    if (r < 0) {
      inst->id = -1;
      rb_raise(rb_eArgError, "%s: the instance is already deleted.", rb_obj_classname(klass));
    }
  }

  id = ngraph_plugin_get_id_by_oid(inst->obj, inst->oid);
  inst->id = id;

  return inst;
}

static VALUE
ngraph_inst_method_move_up(VALUE klass)
{
  struct ngraph_instance *inst;

  inst = check_id(klass);
  if (inst == NULL) {
    return Qnil;
  }

  inst->id = ngraph_plugin_move_up(inst->obj, inst->id);

  return klass;
}

static VALUE
ngraph_inst_method_move_down(VALUE klass)
{
  struct ngraph_instance *inst;

  inst = check_id(klass);
  if (inst == NULL) {
    return Qnil;
  }

  inst->id = ngraph_plugin_move_down(inst->obj, inst->id);

  return klass;
}

static VALUE
ngraph_inst_method_move_top(VALUE klass)
{
  struct ngraph_instance *inst;

  inst = check_id(klass);
  if (inst == NULL) {
    return Qnil;
  }

  inst->id = ngraph_plugin_move_top(inst->obj, inst->id);

  return klass;
}

static VALUE
ngraph_inst_method_move_last(VALUE klass)
{
  struct ngraph_instance *inst;

  inst = check_id(klass);
  if (inst == NULL) {
    return Qnil;
  }

  inst->id = ngraph_plugin_move_last(inst->obj, inst->id);

  return klass;
}

static int
check_inst_args(VALUE self, VALUE arg, const char *field, struct ngraph_instance **inst1, struct ngraph_instance **inst2)
{
  *inst1 = check_id(self);
  if (*inst1 == NULL) {
    return 1;
  }

  if (! rb_obj_is_kind_of(arg, NgraphClass)) {
    rb_raise(rb_eArgError, "%s#%s: illegal type of the argument (%s).", rb_obj_classname(self), field, rb_obj_classname(arg));
  }

  *inst2 = check_id(arg);
  if (*inst2 == NULL) {
    return 1;
  }

  if ((*inst1)->obj != (*inst2)->obj) {
    rb_raise(rb_eArgError, "%s#%s: illegal type of the argument (%s).", rb_obj_classname(self), field, rb_obj_classname(arg));
  }

  return 0;
}

static VALUE
ngraph_inst_method_exchange(VALUE self, VALUE arg)
{
  struct ngraph_instance *inst1, *inst2;
  int id, r;

  r = check_inst_args(self, arg, "exchange", &inst1, &inst2);
  if (r) {
    return Qnil;
  }

  r = ngraph_plugin_exchange(inst1->obj, inst1->id, inst2->id);
  if (r < 0) {
    return Qnil;
  }

  id = inst1->id;
  inst1->id = inst2->id;
  inst2->id = id;

  return self;
}

static VALUE
ngraph_inst_method_copy(VALUE self, VALUE arg)
{
  struct ngraph_instance *inst1, *inst2;
  int r;

  r = check_inst_args(self, arg, "copy", &inst1, &inst2);
  if (r) {
    return Qnil;
  }

  r = ngraph_plugin_copy(inst1->obj, inst1->id, inst2->id);
  if (r < 0) {
    return Qnil;
  }

  return self;
}

static VALUE
ngraph_inst_method_to_str(VALUE klass)
{
  struct ngraph_instance *inst;
  const char *name;
  VALUE rstr;

  inst = check_id(klass);
  if (inst == NULL) {
    return Qnil;
  }

  name = ngraph_plugin_get_obj_name(inst->obj);
  rstr = rb_sprintf("%s:%d", name, inst->id);

  return rstr;
}

static VALUE
ngraph_inst_method_rcode(VALUE klass)
{
  struct ngraph_instance *inst;

  inst = check_id(klass);
  if (inst == NULL) {
    return Qnil;
  }

  return INT2FIX(inst->rcode);
}

static VALUE
str2inst_get_ary(VALUE data1)
{
  const char *name;
  int i, n;
  VALUE ary, obj, klass;
  struct obj_ids *obj_ids;

  obj_ids = (struct obj_ids *) data1;

  name = ngraph_plugin_get_obj_name(obj_ids->obj);
  klass = get_ngraph_obj(name);
  ary = rb_ary_new2(obj_ids->num);
  n = obj_ids->num;
  for (i = 0; i < n; i++) {
    obj = obj_get(klass, INT2FIX(obj_ids->ids[i]), name);
    rb_ary_push(ary, obj);
  }

  return ary;
}

static VALUE
str2inst_ensure(VALUE data2)
{
  int *ids;

  ids = (int *) data2;
  free(ids);

  return Qnil;
}

static VALUE
obj_get_from_str(VALUE klass, VALUE arg, const char *name)
{
  const char *str;
  int l;
  char *buf;
  struct obj_ids obj_ids;
  VALUE ary;

  str = StringValueCStr(arg);
  l = strlen(str) + strlen(name) + 2;
  buf = alloca(l);
  if (buf == NULL) {
    rb_raise(rb_eSysStackError, "%s: cannot allocate enough memory.", rb_obj_classname(klass));
  }
  snprintf(buf, l, "%s:%s", name, str);
  obj_ids.obj = ngraph_plugin_get_instances_by_str(buf, &obj_ids.num, &obj_ids.ids);
  if (obj_ids.obj == NULL) {
    return rb_ary_new();
  }

  ary = rb_ensure(str2inst_get_ary, (VALUE) &obj_ids, str2inst_ensure, (VALUE) obj_ids.ids);

  return ary;
}

static VALUE
obj_get(VALUE klass, VALUE id_value, const char *name)
{
  struct ngraph_instance *inst;
  struct objlist *nobj;
  ngraph_returned_value oid;
  ngraph_arg arg;
  VALUE new_inst;
  int id, n;

  if (RB_TYPE_P(id_value, T_STRING)) {
    return obj_get_from_str(klass, id_value, name);
  }

  id = NUM2INT(id_value);

  nobj = ngraph_plugin_get_object(name);
  n = ngraph_plugin_get_obj_last_id(nobj);

  if (id < 0) {
    id = n + id + 1;
  }

  if (id < 0 || id > n) {
    return Qnil;
  }

  new_inst = Data_Make_Struct(klass, struct ngraph_instance, NULL, ngraph_object_free, inst);

  inst->obj = nobj;
  inst->id = id;
  inst->rcode = id;
  arg.num = 0;
  ngraph_plugin_getobj(inst->obj, "oid", inst->id, &arg, &oid);
  inst->oid = oid.i;

  return new_inst;
}

static VALUE
obj_field_args(VALUE klass, VALUE field, const char *name)
{
  struct objlist *nobj;
  const char* args;
  int type;

  nobj = ngraph_plugin_get_object(name);
  type = ngraph_plugin_get_obj_field_type(nobj, StringValueCStr(field));
  if (type < NVFUNC) {
    return Qnil;
  }
  args = ngraph_plugin_get_obj_field_args(nobj, StringValueCStr(field));
  if (args == NULL) {
    args = "";
  } else if (args[0] == '\0') {
    args = "void";
  }

  return rb_str_new2(args);
}

static VALUE
obj_field_type(VALUE klass, VALUE field, const char *name)
{
  struct objlist *nobj;
  int type;

  nobj = ngraph_plugin_get_object(name);
  type = ngraph_plugin_get_obj_field_type(nobj, StringValueCStr(field));

  return INT2FIX(type);
}

static VALUE
obj_field_permission(VALUE klass, VALUE field, const char *name)
{
  struct objlist *nobj;
  int perm;

  nobj = ngraph_plugin_get_object(name);
  perm = ngraph_plugin_get_obj_field_permission(nobj, StringValueCStr(field));

  return INT2FIX(perm);
}

static VALUE
get_ngraph_obj(const char *name)
{
  char buf[64];

  strncpy(buf, name, sizeof(buf) - 1);
  buf[sizeof(buf) - 1] = '\0';
  buf[0] = toupper(buf[0]);

  return rb_const_get(NgraphModule, rb_intern(buf));
}

static void 
add_child(VALUE ary, struct objlist *parent, int noinst)
{
  struct objlist *ocur;
  VALUE obj;
  int id;
  const char *name;

  ocur = ngraph_plugin_get_obj_root();
  while (ocur) {
    if (ngraph_plugin_get_obj_parent(ocur) == parent) {
      id = ngraph_plugin_get_obj_last_id(ocur);
      if (id != -1 || ! noinst) {
	name = ngraph_plugin_get_obj_name(ocur);
	obj = get_ngraph_obj(name);
	if (! NIL_P(obj)) {
	  rb_ary_push(ary, obj);
	}
      }
      add_child(ary, ocur, noinst);
    }
    ocur = ngraph_plugin_get_obj_next(ocur);
  }
}

static VALUE
obj_derive(VALUE klass, VALUE arg, const char *name)
{
  struct objlist *obj;
  int noinst, id;
  VALUE robj, ary;

  noinst = RTEST(arg);

  obj = ngraph_plugin_get_object(name);
  if (obj == NULL) {
    return Qnil;
  }

  ary = rb_ary_new();

  id = ngraph_plugin_get_obj_last_id(obj);
  if (id != -1 || ! noinst) {
    robj = get_ngraph_obj(name);
    if (! NIL_P(robj)) {
      rb_ary_push(ary, robj);
    }
  }

  add_child(ary, obj, noinst);

  return ary;
}

static VALUE
obj_each(VALUE klass, const char *name)
{
  struct objlist *nobj;
  int i, n;
  VALUE inst, id, ary;

  nobj = ngraph_plugin_get_object(name);
  n = ngraph_plugin_get_obj_last_id(nobj) + 1;
  if (n < 1) {
    return klass;
  }

  name = ngraph_plugin_get_obj_name(nobj);
  ary = rb_ary_new2(n);
  for (i = 0; i < n; i++) {
    id = INT2FIX(i);
    inst = obj_get(klass, id, name);
    rb_ary_store(ary, i, inst);
  }
  rb_ary_each(ary);

  return klass;
}

static VALUE
obj_move_up(VALUE klass, VALUE arg, const char *name)
{
  struct objlist *nobj;
  int id, r;

  nobj = ngraph_plugin_get_object(name);
  id = NUM2INT(arg);

  r = ngraph_plugin_move_up(nobj, id);
  if (r < 0) {
    return Qnil;
  }

  return klass;
}

static VALUE
obj_move_down(VALUE klass, VALUE arg, const char *name)
{
  struct objlist *nobj;
  int id, r;

  nobj = ngraph_plugin_get_object(name);
  id = NUM2INT(arg);

  r = ngraph_plugin_move_down(nobj, id);
  if (r < 0) {
    return Qnil;
  }

  return klass;
}

static VALUE
obj_move_top(VALUE klass, VALUE arg, const char *name)
{
  struct objlist *nobj;
  int id, r;

  nobj = ngraph_plugin_get_object(name);
  id = NUM2INT(arg);

  r = ngraph_plugin_move_top(nobj, id);
  if (r < 0) {
    return Qnil;
  }

  return klass;
}

static VALUE
obj_move_last(VALUE klass, VALUE arg, const char *name)
{
  struct objlist *nobj;
  int id, r;

  nobj = ngraph_plugin_get_object(name);
  id = NUM2INT(arg);

  r = ngraph_plugin_move_last(nobj, id);
  if (r < 0) {
    return Qnil;
  }

  return klass;
}

static VALUE
obj_exchange(VALUE klass, VALUE arg1, VALUE arg2, const char *name)
{
  struct objlist *nobj;
  int id1, id2, r;

  nobj = ngraph_plugin_get_object(name);
  id1 = NUM2INT(arg1);
  id2 = NUM2INT(arg2);

  r = ngraph_plugin_exchange(nobj, id1, id2);
  if (r < 0) {
    return Qnil;
  }

  return klass;
}

static VALUE
obj_copy(VALUE klass, VALUE arg1, VALUE arg2, const char *name)
{
  struct objlist *nobj;
  int id1, id2, r;

  nobj = ngraph_plugin_get_object(name);
  id1 = NUM2INT(arg1);
  id2 = NUM2INT(arg2);

  r = ngraph_plugin_copy(nobj, id1, id2);
  if (r < 0) {
    return Qnil;
  }

  return klass;
}

static VALUE
obj_size(VALUE klass, const char *name)
{
  struct objlist *nobj;
  int n;

  nobj = ngraph_plugin_get_object(name);
  n = ngraph_plugin_get_obj_last_id(nobj);
  n = (n >= 0) ? n + 1 : 0;

  return INT2FIX(n);
}

static VALUE
obj_exist(VALUE klass, const char *name)
{
  struct objlist *nobj;
  int n;

  nobj = ngraph_plugin_get_object(name);
  n = ngraph_plugin_get_obj_last_id(nobj);

  return (n >= 0) ? Qtrue : Qfalse;
}

static VALUE
obj_del_from_str(VALUE klass, VALUE arg, const char *name)
{
  const char *str;
  int i, l, *ids, n;
  char *buf;
  struct objlist *obj;

  str = StringValueCStr(arg);
  l = strlen(str) + strlen(name) + 2;
  buf = malloc(l);
  if (buf == NULL) {
    rb_raise(rb_eNoMemError, "%s: cannot allocate enough memory.", rb_obj_classname(klass));
  }
  snprintf(buf, l, "%s:%s", name, str);
  obj = ngraph_plugin_get_instances_by_str(buf, &n, &ids);
  free(buf);
  if (obj == NULL) {
    return klass;
  }

  for (i = n - 1;  i >= 0; i--) {
    ngraph_plugin_del(obj, ids[i]);
  }
  free(ids);

  return klass;
}

static VALUE
obj_del(VALUE klass, VALUE id_value, const char *name)
{
  int id, n;
  struct objlist *nobj;

  if (RB_TYPE_P(id_value, T_STRING)) {
    return obj_del_from_str(klass, id_value, name);
  }

  id = NUM2INT(id_value);
  nobj = ngraph_plugin_get_object(name);
  n = ngraph_plugin_get_obj_last_id(nobj);

  if (id < 0) {
    id = n + id + 1;
  }

  if (id < 0 || id > n) {
    return Qnil;
  }

  ngraph_plugin_del(nobj, id);

  return klass;
}

static VALUE
ngraph_inst_method_del(VALUE self)
{
  int id;
  struct ngraph_instance *inst;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  id = inst->id;
  inst->id = -1;
  ngraph_plugin_del(inst->obj, id);

  return INT2FIX(id);
}

static VALUE
obj_new_with_block_body(VALUE arg)
{
  return rb_yield(arg);
}

static VALUE
obj_new_with_block_ensure(VALUE arg)
{
  return ngraph_inst_method_del(arg);
}

static VALUE
obj_new(VALUE klass, const char *name)
{
  VALUE new_inst;
  struct objlist *nobj;
  int r;

  nobj = ngraph_plugin_get_object(name);
  r = ngraph_plugin_new(nobj);
  if (r < 0) {
    return Qnil;
  }

  new_inst = obj_get(klass, INT2FIX(r), name);

  if (RTEST(rb_block_given_p())) {
    new_inst = rb_ensure(obj_new_with_block_body, new_inst, obj_new_with_block_ensure, new_inst);
  }

  return new_inst;
}

static VALUE
obj_current(VALUE klass, const char *name)
{
  VALUE new_inst;
  struct objlist *nobj;
  int id;

  nobj = ngraph_plugin_get_object(name);
  id = ngraph_plugin_get_obj_current_id(nobj);
  if (id < 0) {
    return Qnil;
  }

  new_inst = obj_get(klass, INT2FIX(id), name);

  return new_inst;
}

static VALUE
inst_get_int(VALUE self, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_returned_value num;
  ngraph_arg carg;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  inst->rcode = ngraph_plugin_getobj(inst->obj, field, inst->id, &carg, &num);
  if (inst->rcode < 0) {
    return Qnil;
  }

  return INT2NUM(num.i);
}

static VALUE
inst_get_double(VALUE self, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_returned_value num;
  ngraph_arg carg;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  inst->rcode = ngraph_plugin_getobj(inst->obj, field, inst->id, &carg, &num);
  if (inst->rcode < 0) {
    return Qnil;
  }

  return rb_float_new(num.d);
}

static VALUE
inst_get_bool(VALUE self, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_returned_value num;
  ngraph_arg carg;
  VALUE val;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  inst->rcode = ngraph_plugin_getobj(inst->obj, field, inst->id, &carg, &num);
  if (inst->rcode < 0) {
    return Qnil;
  }

  val = num.i ? Qtrue : Qfalse;

  return val;
}

static VALUE
inst_get_str(VALUE self, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_returned_value str;
  ngraph_arg carg;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  inst->rcode = ngraph_plugin_getobj(inst->obj, field, inst->id, &carg, &str);
  if (inst->rcode < 0) {
    return Qnil;
  }

  if (str.str) {
    return rb_tainted_str_new2(str.str);
  }

  return Qnil;
}

static VALUE
inst_get_obj(VALUE self, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_returned_value str;
  ngraph_arg carg;
  const char *name;
  int id, n, *ids;
  struct objlist *obj;
  VALUE klass;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  inst->rcode = ngraph_plugin_getobj(inst->obj, field, inst->id, &carg, &str);
  if (inst->rcode < 0) {
    return Qnil;
  }

  if (str.str == NULL) {
    return Qnil;
  }

  obj = ngraph_plugin_get_instances_by_str(str.str, &n, &ids);
  if (obj == NULL) {
    return Qnil;
  }

  id = ids[n - 1];
  free(ids);

  name = ngraph_plugin_get_obj_name(obj);
  if (name == NULL) {
    return Qnil;
  }

  klass = get_ngraph_obj(name);

  return obj_get(klass, INT2FIX(id), name);
}

static VALUE
inst_put_int(VALUE self, VALUE arg, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_value num;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  num.i = NUM2INT(arg);
  inst->rcode = ngraph_plugin_putobj(inst->obj, field, inst->id, &num);
  if (inst->rcode < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_double(VALUE self, VALUE arg, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_value num;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  num.d = NUM2DBL(arg);
  inst->rcode = ngraph_plugin_putobj(inst->obj, field, inst->id, &num);
  if (inst->rcode < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_bool(VALUE self, VALUE arg, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_value num;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  num.i = RTEST(arg) ? 1 : 0;
  inst->rcode = ngraph_plugin_putobj(inst->obj, field, inst->id, &num);
  if (inst->rcode < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_enum(VALUE self, VALUE arg, const char *field, int max)
{
  struct ngraph_instance *inst;
  ngraph_value num;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  num.i = NUM2INT(arg);
  if (num.i < 0 || num.i > max) {
    return Qnil;
  }

  inst->rcode = ngraph_plugin_putobj(inst->obj, field, inst->id, &num);
  if (inst->rcode < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_str(VALUE self, VALUE arg, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_value str;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  if (NIL_P(arg)) {
    str.str = NULL;
  } else {
    str.str = StringValueCStr(arg);
  }
  inst->rcode = ngraph_plugin_putobj(inst->obj, field, inst->id, &str);
  if (inst->rcode < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_obj(VALUE self, VALUE arg, const char *field)
{
  struct ngraph_instance *inst1, *inst2;
  ngraph_value str;
  char buf[128];
  const char *name, *ptr;
  int *ids;
  struct objlist *obj;

  switch (TYPE(arg)) {
  case T_NIL:
    ptr = NULL;
    break;
  case T_STRING:
    ptr = StringValueCStr(arg);
    obj = ngraph_plugin_get_instances_by_str(ptr, NULL, &ids);
    if (obj == NULL) {
      rb_raise(rb_eArgError, "%s#%s: illegal instance representation (%s).", rb_obj_classname(self), field, ptr);
    }
    free(ids);
    break;
  default:
    if (! rb_obj_is_kind_of(arg, NgraphClass)) {
      rb_raise(rb_eArgError, "%s#%s: illegal type of the argument (%s).", rb_obj_classname(self), field, rb_obj_classname(arg));
    }

    inst2 = check_id(arg);
    if (inst2 == NULL) {
      return Qnil;
    }

    name = ngraph_plugin_get_obj_name(inst2->obj);
#if 0
    snprintf(buf, sizeof(buf), "%s:%d", name, inst2->id);
#else
    snprintf(buf, sizeof(buf), "%s:^%d", name, inst2->oid); /* shoud instance be tighten? */
#endif
    ptr = buf;
  }

  inst1 = check_id(self);
  if (inst1 == NULL) {
    return Qnil;
  }

  str.str = ptr;
  inst1->rcode = ngraph_plugin_putobj(inst1->obj, field, inst1->id, &str);
  if (inst1->rcode < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_iarray(VALUE self, VALUE arg, const char *field)
{
  VALUE tmpstr;
  struct ngraph_instance *inst;
  ngraph_value ary;
  int num, i;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  if (NIL_P(arg)) {
    num = 0;
  } else {
    if (!RB_TYPE_P(arg, T_ARRAY)) {
      rb_raise(rb_eArgError, "%s#%s: the argument must be an Array", rb_obj_classname(self), field);
    }

    num = RARRAY_LEN(arg);
  }

  ary.ary = NULL;
  if (num > 0) {
    ary.ary = rb_alloc_tmp_buffer(&tmpstr, sizeof(*ary.ary) + sizeof(union array) * num);
    ary.ary->num = num;
    if (ary.ary) {
      for (i = 0; i < num; i++) {
        ary.ary->ary[i].i = NUM2INT(rb_ary_entry(arg, i));
      }
    }
  }

  inst->rcode = ngraph_plugin_putobj(inst->obj, field, inst->id, &ary);

  if (ary.ary) {
    rb_free_tmp_buffer(&tmpstr);
  }

  if (inst->rcode < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_get_iarray(VALUE self, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_returned_value cary;
  ngraph_arg carg;
  VALUE ary;
  int i;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  inst->rcode = ngraph_plugin_getobj(inst->obj, field, inst->id, &carg, &cary);
  if (inst->rcode < 0) {
    return Qnil;
  }

  ary = rb_ary_new2(cary.ary.num);
  for (i = 0; i < cary.ary.num; i++) {
    rb_ary_store(ary, i, INT2NUM(cary.ary.data.ia[i]));
  }

  return ary;
}

static VALUE
inst_put_darray(VALUE self, VALUE arg, const char *field)
{
  VALUE tmpstr;
  struct ngraph_instance *inst;
  ngraph_value ary;
  int num, i;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  if (NIL_P(arg)) {
    num = 0;
  } else {
    if (!RB_TYPE_P(arg, T_ARRAY)) {
      rb_raise(rb_eArgError, "%s#%s: the argument must be an Array", rb_obj_classname(self), field);
    }

    num = RARRAY_LEN(arg);
  }

  ary.ary = NULL;
  if (num > 0) {
    ary.ary = rb_alloc_tmp_buffer(&tmpstr, sizeof(*ary.ary) + sizeof(union array) * num);
    ary.ary->num = num;
    if (ary.ary) {
      for (i = 0; i < num; i++) {
        ary.ary->ary[i].d = NUM2DBL(rb_ary_entry(arg, i));
      }
    }
  }

  inst->rcode = ngraph_plugin_putobj(inst->obj, field, inst->id, &ary);

  if (ary.ary) {
    rb_free_tmp_buffer(&tmpstr);
  }

  if (inst->rcode < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_get_darray(VALUE self, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_returned_value cary;
  ngraph_arg carg;
  VALUE ary;
  int i;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  inst->rcode = ngraph_plugin_getobj(inst->obj, field, inst->id, &carg, &cary);
  if (inst->rcode < 0) {
    return Qnil;
  }

  ary = rb_ary_new2(cary.ary.num);
  for (i = 0; i < cary.ary.num; i++) {
    rb_ary_store(ary, i, rb_float_new(cary.ary.data.da[i]));
  }

  return ary;
}

static VALUE
inst_put_sarray(VALUE self, VALUE arg, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_value ary;
  int num, i;
  VALUE str, tmpstr;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  if (NIL_P(arg)) {
    num = 0;
  } else {
    if (!RB_TYPE_P(arg, T_ARRAY)) {
      rb_raise(rb_eArgError, "%s#%s: the argument must be an Array", rb_obj_classname(self), field);
    }

    num = RARRAY_LEN(arg);
  }

  ary.ary = NULL;
  if (num > 0) {
    ary.ary =  rb_alloc_tmp_buffer(&tmpstr, sizeof(*ary.ary) + sizeof(union array) * num);
    ary.ary->num = num;
    if (ary.ary) {
      for (i = 0; i < num; i++) {
        str = rb_ary_entry(arg, i);
        ary.ary->ary[i].str = StringValueCStr(str);
      }
    }
  }

  inst->rcode = ngraph_plugin_putobj(inst->obj, field, inst->id, &ary);

  if (ary.ary) {
    rb_free_tmp_buffer(&tmpstr);
  }

  if (inst->rcode < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_get_sarray(VALUE self, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_returned_value cary;
  ngraph_arg carg;
  VALUE ary;
  int i;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  inst->rcode = ngraph_plugin_getobj(inst->obj, field, inst->id, &carg, &cary);
  if (inst->rcode < 0) {
    return Qnil;
  }

  ary = rb_ary_new2(cary.ary.num);
  for (i = 0; i < cary.ary.num; i++) {
    rb_ary_store(ary, i, rb_tainted_str_new2(cary.ary.data.sa[i]));
  }

  return ary;
}

static VALUE
inst_exe_void_func(VALUE self, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_arg carg;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  inst->rcode = ngraph_plugin_exeobj(inst->obj, field, inst->id, &carg);
  if (inst->rcode < 0) {
    return Qnil;
  }

  return self;
}

static struct ngraph_array *
allocate_iarray(VALUE self, volatile VALUE *tmpstr, VALUE arg, const char *field)
{
  struct ngraph_array *narray;
  int i, num;

  if (NIL_P(arg)) {
    num = 0;
  } else {
    if (!RB_TYPE_P(arg, T_ARRAY)) {
      rb_raise(rb_eArgError, "%s#%s: the argument must be an Array", rb_obj_classname(self), field);
    }

    num = RARRAY_LEN(arg);
  }

  narray = rb_alloc_tmp_buffer(tmpstr, sizeof(*narray) + sizeof(union array) * num);
  if (narray == NULL) {
    return NULL;
  }

  narray->num = num;
  for (i = 0; i < num; i++) {
    narray->ary[i].i = NUM2INT(rb_ary_entry(arg, i));
  }

  return narray;
}

static struct ngraph_array *
allocate_darray(VALUE self, volatile VALUE *tmpstr, VALUE arg, const char *field)
{
  struct ngraph_array *narray;
  int i, num;

  if (NIL_P(arg)) {
    num = 0;
  } else {
    if (!RB_TYPE_P(arg, T_ARRAY)) {
      rb_raise(rb_eArgError, "%s#%s: the argument must be an Array", rb_obj_classname(self), field);
    }

    num = RARRAY_LEN(arg);
  }

  narray = rb_alloc_tmp_buffer(tmpstr, sizeof(*narray) + sizeof(union array) * num);
  if (narray == NULL) {
    return NULL;
  }

  narray->num = num;
  for (i = 0; i < num; i++) {
    narray->ary[i].d = NUM2DBL(rb_ary_entry(arg, i));
  }

  return narray;
}

static struct ngraph_array *
allocate_sarray(VALUE self, volatile VALUE *tmpstr, VALUE arg, const char *field)
{
  struct ngraph_array *narray;
  int i, num;
  VALUE str;

  if (NIL_P(arg)) {
    num = 0;
  } else {
    if (!RB_TYPE_P(arg, T_ARRAY)) {
      rb_raise(rb_eArgError, "%s#%s: the argument must be an Array", rb_obj_classname(self), field);
    }

    num = RARRAY_LEN(arg);
  }
  narray = rb_alloc_tmp_buffer(tmpstr, sizeof(*narray) + sizeof(union array) * num);

  narray->num = num;
  for (i = 0; i < num; i++) {
    str = rb_ary_entry(arg, i);
    narray->ary[i].str = StringValueCStr(str);
  }

  return narray;
}

static VALUE
create_obj_arg(VALUE args)
{
  int i, n;
  VALUE arg, str, uniq_args;
  struct ngraph_instance *inst;
  struct objlist *obj = NULL;
  const char *name;

  uniq_args = rb_funcall(args, Uniq, 0);
  n = RARRAY_LEN(uniq_args);
  str = rb_str_new2("");
  for (i = 0; i < n; i++) {
    arg = rb_ary_entry(uniq_args, i);
    if (! rb_obj_is_kind_of(arg, NgraphClass)) {
      return Qnil;
    }

    inst = check_id(arg);
    if (inst == NULL) {
      return Qnil;
    }

    if (obj == NULL) {
      obj = inst->obj;
      name = ngraph_plugin_get_obj_name(inst->obj);
      rb_str_cat2(str, name);
    } else if (obj != inst->obj) {
      return Qnil;
    }
    rb_str_catf(str, "%c%d", (i) ? ',' : ':', inst->id);
  }
  return str;
}

static VALUE
obj_func_obj(VALUE self, VALUE argv, const char *field, int type)
{
  VALUE arg, rstr, val;
  int i, n;
  struct ngraph_instance *inst;
  ngraph_arg carg;
  int argc;
  ngraph_returned_value rval;
  const char *str;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  argc = RARRAY_LEN(argv);
  if (argc < 1) {
    str = NULL;
  } else {
    arg = rb_ary_entry(argv, 0);
    if (RB_TYPE_P(arg, T_ARRAY)) {
      if (argc > 1) {
	return Qnil;
      }
      n = RARRAY_LEN(arg);
      if (n < 1) {
	str = NULL;
      } else {
	rstr = create_obj_arg(arg);
	if (NIL_P(rstr)) {
	  return Qnil;
	}
	str = StringValueCStr(rstr);
      }
    } else {
      rstr = create_obj_arg(argv);
      if (NIL_P(rstr)) {
	return Qnil;
      }
      str = StringValueCStr(rstr);
    }
  }

  carg.num = 1;
  carg.ary[0].str = str;
  if (type == NVFUNC) {
    inst->rcode = ngraph_plugin_exeobj(inst->obj, field, inst->id, &carg);
  } else {
    inst->rcode = ngraph_plugin_getobj(inst->obj, field, inst->id, &carg, &rval);
  }
  if (inst->rcode < 0) {
    return Qnil;
  }

  switch (type) {
  case NVFUNC:
    val = self;
    break;
  case NBFUNC:
    val = (rval.i) ? Qtrue : Qfalse;
    break;
#if USE_NCHAR
  case NCFUNC:
#endif
  case NIFUNC:
    val = INT2NUM(rval.i);
    break;
  case NDFUNC:
    val = rb_float_new(rval.d);
    break;
  case NSFUNC:
    if (rval.str == NULL) {
      val = Qnil;
    } else {
      val = rb_tainted_str_new2(rval.str);
    }
    break;
  case NIAFUNC:
    val = rb_ary_new2(rval.ary.num);
    for (i = 0; i < rval.ary.num; i++) {
      rb_ary_store(val, i, INT2NUM(rval.ary.data.ia[i]));
    }
    break;
  case NDAFUNC:
    val = rb_ary_new2(rval.ary.num);
    for (i = 0; i < rval.ary.num; i++) {
      rb_ary_store(val, i, rb_float_new(rval.ary.data.da[i]));
    }
    break;
  case NSAFUNC:
    val = rb_ary_new2(rval.ary.num);
    for (i = 0; i < rval.ary.num; i++) {
      rb_ary_store(val, i, rb_tainted_str_new2(rval.ary.data.sa[i]));
    }
    break;
  default:
    val = Qnil;
  }

  return val;
}

static VALUE
exe_void_func_argv(VALUE self, VALUE argv, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_arg *carg;
  VALUE tmpstr;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }
  carg = allocate_sarray(self, &tmpstr, argv, field);
  inst->rcode = ngraph_plugin_exeobj(inst->obj, field, inst->id, carg);
  rb_free_tmp_buffer(&tmpstr);
  if (inst->rcode < 0) {
    return Qnil;
  }

  return self;
}

static VALUE
get_str_func_argv(VALUE self, VALUE argv, const char *field)
{
  ngraph_returned_value rval;
  struct ngraph_instance *inst;
  ngraph_arg *carg;
  VALUE tmpstr;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }
  carg = allocate_sarray(self, &tmpstr, argv, field);
  inst->rcode = ngraph_plugin_getobj(inst->obj, field, inst->id, carg, &rval);
  rb_free_tmp_buffer(&tmpstr);
  if (inst->rcode < 0) {
    return Qnil;
  }

  return rb_tainted_str_new2(rval.str ? rval.str : "");
}

static void
add_obj_name_const(VALUE klass, struct objlist *nobj, const char *name)
{
  const char *obj_name;
  char str[64];
  VALUE val;

  if (nobj == NULL) {
    val = Qnil;
  } else {
    obj_name = ngraph_plugin_get_obj_name(nobj);
    strncpy(str, obj_name, sizeof(str) - 1);
    str[sizeof(str) - 1] = '\0';
    str[0] = toupper(str[0]);
    val = ID2SYM(rb_intern(str));
  }
  rb_define_const(klass, name, val);
}

static void
add_obj_str_const(VALUE klass, const char *name, const char *str)
{
  VALUE val;

  val = rb_str_new2(str);
  OBJ_FREEZE(val);
  rb_define_const(klass, name, val);
}

static void
add_obj_const(VALUE klass, const char *name)
{
  struct objlist *nobj, *next, *child, *parent;
  const char *str;

  nobj = ngraph_plugin_get_object(name);
  next = ngraph_plugin_get_obj_next(nobj);
  child = ngraph_plugin_get_obj_child(nobj);
  parent = ngraph_plugin_get_obj_parent(nobj);

  str = ngraph_plugin_get_obj_version(nobj);
  add_obj_str_const(klass, "VERSION", str);
  add_obj_str_const(klass, "NAME", name);

  add_obj_name_const(klass, parent, "PARENT");
  add_obj_name_const(klass, next, "NEXT");
  add_obj_name_const(klass, child, "CHILD");
}

static void
setup_obj_common(VALUE obj)
{
  rb_extend_object(obj, rb_mEnumerable);
  rb_include_module(obj, rb_mComparable);
  rb_define_method(obj, "del", ngraph_inst_method_del, 0);
  rb_define_method(obj, "===", ngraph_inst_method_equal, 1);
  rb_define_method(obj, "<=>", ngraph_inst_method_compare, 1);
  rb_define_method(obj, "move_up", ngraph_inst_method_move_up, 0);
  rb_define_method(obj, "move_down", ngraph_inst_method_move_down, 0);
  rb_define_method(obj, "move_top", ngraph_inst_method_move_top, 0);
  rb_define_method(obj, "move_last", ngraph_inst_method_move_last, 0);
  rb_define_method(obj, "exchange", ngraph_inst_method_exchange, 1);
  rb_define_method(obj, "to_s", ngraph_inst_method_to_str, 0);
  rb_define_method(obj, "rcode", ngraph_inst_method_rcode, 0);
  rb_define_method(obj, "copy", ngraph_inst_method_copy, 1);
}

static void
store_field_names(VALUE fields, const char *name)
{
  VALUE field;

  field = rb_str_new2(name);
  OBJ_FREEZE(field);
  rb_ary_push(fields, field);
}

#include "ruby_ngraph.c"

static void
add_common_const(VALUE ngraph_module)
{
  VALUE type, attr;

  type = rb_define_module_under(ngraph_module, "FIELD_TYPE");
#if USE_NCHAR
  rb_define_const(type, "CHAR", INT2FIX(NCHAR));
#endif
  rb_define_const(type, "VOID", INT2FIX(NVOID));
  rb_define_const(type, "BOOL", INT2FIX(NBOOL));
  rb_define_const(type, "INT", INT2FIX(NINT));
  rb_define_const(type, "DOUBLE", INT2FIX(NDOUBLE));
  rb_define_const(type, "STR", INT2FIX(NSTR));
  rb_define_const(type, "POINTER", INT2FIX(NPOINTER));
  rb_define_const(type, "IARRAY", INT2FIX(NIARRAY));
  rb_define_const(type, "DARRAY", INT2FIX(NDARRAY));
  rb_define_const(type, "SARRAY", INT2FIX(NSARRAY));
  rb_define_const(type, "ENUM", INT2FIX(NENUM));
  rb_define_const(type, "OBJ", INT2FIX(NOBJ));
#if USE_LABEL
  rb_define_const(type, "LABEL", INT2FIX(NLABEL));
#endif
  rb_define_const(type, "VFUNC", INT2FIX(NVFUNC));
  rb_define_const(type, "BFUNC", INT2FIX(NBFUNC));
#if USE_NCHAR
  rb_define_const(type, "CFUNC", INT2FIX(NCFUNC));
#endif
  rb_define_const(type, "IFUNC", INT2FIX(NIFUNC));
  rb_define_const(type, "DFUNC", INT2FIX(NDFUNC));
  rb_define_const(type, "SFUNC", INT2FIX(NSFUNC));
  rb_define_const(type, "IAFUNC", INT2FIX(NIAFUNC));
  rb_define_const(type, "DAFUNC", INT2FIX(NDAFUNC));
  rb_define_const(type, "SAFUNC", INT2FIX(NSAFUNC));

  attr = rb_define_module_under(ngraph_module, "FIELD_PERMISSION");
  rb_define_const(attr, "READ", INT2FIX(NREAD));
  rb_define_const(attr, "WRITE", INT2FIX(NWRITE));
  rb_define_const(attr, "EXEC", INT2FIX(NEXEC));
}

static VALUE
ngraph_class_new(VALUE self)
{
  rb_raise(rb_eNotImpError, "%s: creation of an instance is forbidden.", rb_obj_classname(self));
  return Qnil;
}

static VALUE
ngraph_puts(VALUE module, VALUE str)
{
  ngraph_plugin_puts(StringValueCStr(str));
  return Qnil;
}

static VALUE
ngraph_err_puts(VALUE module, VALUE str)
{
  ngraph_plugin_err_puts(StringValueCStr(str));
  return Qnil;
}

static VALUE
ngraph_sleep(VALUE module, VALUE arg)
{
  int t;

  t = NUM2INT(arg);
  ngraph_plugin_sleep(t);

  return Qnil;
}

static VALUE
ngraph_str2inst(VALUE module, VALUE arg)
{
  const char *str;
  VALUE ary;
  struct obj_ids obj_ids;

  str = StringValueCStr(arg);

  obj_ids.obj = ngraph_plugin_get_instances_by_str(str, &obj_ids.num, &obj_ids.ids);
  if (obj_ids.obj == NULL) {
    return Qnil;
  }

  ary = rb_ensure(str2inst_get_ary, (VALUE) &obj_ids, str2inst_ensure, (VALUE) obj_ids.ids);

  return ary;
}

int
ngraph_plugin_open_ruby(struct ngraph_plugin *plugin)
{
  if (Initialized) {
    return 0;
  }

  ruby_sysinit(&DummyArgc, &DummyArgvPtr);
  ruby_init();
  ruby_script("Embedded Ruby on Ngraph");
  ruby_init_loadpath();
  rb_enc_find_index("encdb");	/* http://www.artonx.org/diary/20090206.html */
  Initialized = TRUE;

  NgraphModule = rb_define_module("Ngraph");
  rb_define_singleton_method(NgraphModule, "puts", ngraph_puts, 1);
  rb_define_singleton_method(NgraphModule, "err_puts", ngraph_err_puts, 1);
  rb_define_singleton_method(NgraphModule, "sleep", ngraph_sleep, 1);
  rb_define_singleton_method(NgraphModule, "str2inst", ngraph_str2inst, 1);

  NgraphClass = rb_define_class_under(NgraphModule, "NgraphObject", rb_cObject);
  rb_define_method(NgraphClass, "initialize", ngraph_class_new, 0);
  add_common_const(NgraphModule);
  create_ngraph_classes(NgraphModule, NgraphClass);

  Uniq = rb_intern("uniq");
  Argv = rb_intern("ARGV");

  return 0;
}

int
ngraph_plugin_exec_ruby(struct ngraph_plugin *plugin, int argc, char *argv[])
{
  int state, i;
  VALUE r_argv;

  if (! Initialized) {
    return 1;
  }

  r_argv = rb_const_get(rb_mKernel, Argv);
  rb_ary_clear(r_argv);
  for (i = 2; i < argc; i++) {
    rb_ary_push(r_argv, rb_tainted_str_new2(argv[i]));
  }

  rb_load_protect(rb_str_new2(argv[1]), 1, &state);
  if (state) {
    VALUE errinfo, errstr;
    errinfo = rb_errinfo();
    errstr = rb_obj_as_string(errinfo);

    ngraph_plugin_err_puts(StringValueCStr(errstr));
  }
  rb_gc_start();

  return 0;
}

void
ngraph_plugin_close_ruby(struct ngraph_plugin *plugin)
{
  if (Initialized) {
    ruby_finalize();
    NgraphClass = 0;
    NgraphModule = 0;
  }
}

#include <ruby.h>
#include <ruby/encoding.h>
#include <ctype.h>
#include <ngraph.h>

#ifdef TRUE
#undef TRUE
#endif
#define TRUE 1

#ifdef FALSE
#undef FALSE
#endif
#define FALSE 0

static char *DummyArgv[] = {"ngraph_ruby", NULL};
static char **DummyArgvPtr = DummyArgv;
static int DummyArgc = 1, Initialized = FALSE;
static VALUE NgraphClass, NgraphModule;
static ID Uniq, ExpandPath;

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
static VALUE tainted_utf8_str_new(const char *str);

static VALUE
tainted_utf8_str_new(const char *str)
{
  size_t l;
  VALUE s;

  if (str == NULL) {
    return Qnil;
  }

  l = strlen(str);
  s = rb_enc_str_new(str, l, rb_utf8_encoding());

  return s;
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
  int id, last;
  ngraph_arg arg;
  ngraph_returned_value oid;

  Data_Get_Struct(klass, struct ngraph_instance, inst);

  if (inst->id < 0) {
    rb_raise(rb_eArgError, "%s: the instance is already deleted.", rb_obj_classname(klass));
  }

  last = ngraph_get_object_last_id(inst->obj);
  if (inst->id <= last) {
    int r;
    arg.num = 0;
    r = ngraph_object_get(inst->obj, "oid", inst->id, &arg, &oid);

    if (r >= 0 && inst->oid == oid.i) {
      return inst;
    }

    if (r < 0) {
      inst->id = -1;
      rb_raise(rb_eArgError, "%s: the instance is already deleted.", rb_obj_classname(klass));
    }
  }

  id = ngraph_object_get_id_by_oid(inst->obj, inst->oid);
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

  inst->id = ngraph_object_move_up(inst->obj, inst->id);

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

  inst->id = ngraph_object_move_down(inst->obj, inst->id);

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

  inst->id = ngraph_object_move_top(inst->obj, inst->id);

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

  inst->id = ngraph_object_move_last(inst->obj, inst->id);

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

  r = ngraph_object_exchange(inst1->obj, inst1->id, inst2->id);
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

  r = ngraph_object_copy(inst1->obj, inst1->id, inst2->id);
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

  name = ngraph_get_object_name(inst->obj);
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

  name = ngraph_get_object_name(obj_ids->obj);
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
  ngraph_free(ids);

  return Qnil;
}

static VALUE
obj_get_from_str(VALUE klass, VALUE arg, const char *name)
{
  const char *str;
  struct obj_ids obj_ids;
  VALUE ary, inst;

  str = StringValueCStr(arg);
  inst = rb_sprintf("%s:%s", name, str);
  obj_ids.obj = ngraph_get_object_instances_by_str(StringValueCStr(inst), &obj_ids.num, &obj_ids.ids);
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

  nobj = ngraph_get_object(name);
  n = ngraph_get_object_last_id(nobj);

  if (id < 0) {
    id = n + id + 1;
  }

  if (id < 0 || id > n) {
    return Qnil;
  }

  new_inst = Data_Make_Struct(klass, struct ngraph_instance, 0, -1, inst);

  inst->obj = nobj;
  inst->id = id;
  inst->rcode = id;
  arg.num = 0;
  ngraph_object_get(inst->obj, "oid", inst->id, &arg, &oid);
  inst->oid = oid.i;

  return new_inst;
}

static VALUE
obj_field_args(VALUE klass, VALUE field, const char *name)
{
  struct objlist *nobj;
  const char* args;
  enum ngraph_object_field_type type;

  nobj = ngraph_get_object(name);
  type = ngraph_get_object_field_type(nobj, StringValueCStr(field));
  if (type < NVFUNC) {
    return Qnil;
  }
  args = ngraph_get_object_field_args(nobj, StringValueCStr(field));
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
  enum ngraph_object_field_type type;

  nobj = ngraph_get_object(name);
  type = ngraph_get_object_field_type(nobj, StringValueCStr(field));

  return INT2FIX(type);
}

static VALUE
obj_field_permission(VALUE klass, VALUE field, const char *name)
{
  struct objlist *nobj;
  int perm;

  nobj = ngraph_get_object(name);
  perm = ngraph_get_object_field_permission(nobj, StringValueCStr(field));

  return INT2FIX(perm);
}

static void
set_ngraph_obj_class_name(const char *name, char *buf, int len)
{
  int i;

  buf[0] = toupper(name[0]);
  for (i = 1; i < len - 1; i++) {
    buf[i] = name[i];
    if (name[i] == '\0') {
      break;
    }
  }
  buf[len - 1] = '\0';
}

static VALUE
get_ngraph_obj(const char *name)
{
  char buf[64];

  set_ngraph_obj_class_name(name, buf, sizeof(buf));

  return rb_const_get(NgraphModule, rb_intern(buf));
}

static void
add_child(VALUE ary, struct objlist *parent, int noinst)
{
  struct objlist *ocur;
  VALUE obj;
  int id;
  const char *name;

  ocur = ngraph_get_root_object();
  while (ocur) {
    if (ngraph_get_parent_object(ocur) == parent) {
      id = ngraph_get_object_last_id(ocur);
      if (id != -1 || ! noinst) {
	name = ngraph_get_object_name(ocur);
	obj = get_ngraph_obj(name);
	if (! NIL_P(obj)) {
	  rb_ary_push(ary, obj);
	}
      }
      add_child(ary, ocur, noinst);
    }
    ocur = ngraph_get_next_object(ocur);
  }
}

static VALUE
obj_derive(VALUE klass, VALUE arg, const char *name)
{
  struct objlist *obj;
  int noinst, id;
  VALUE robj, ary;

  noinst = RTEST(arg);

  obj = ngraph_get_object(name);
  if (obj == NULL) {
    return Qnil;
  }

  ary = rb_ary_new();

  id = ngraph_get_object_last_id(obj);
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

  nobj = ngraph_get_object(name);
  n = ngraph_get_object_last_id(nobj) + 1;
  if (n < 1) {
    return klass;
  }

  name = ngraph_get_object_name(nobj);
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

  nobj = ngraph_get_object(name);
  id = NUM2INT(arg);

  r = ngraph_object_move_up(nobj, id);
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

  nobj = ngraph_get_object(name);
  id = NUM2INT(arg);

  r = ngraph_object_move_down(nobj, id);
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

  nobj = ngraph_get_object(name);
  id = NUM2INT(arg);

  r = ngraph_object_move_top(nobj, id);
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

  nobj = ngraph_get_object(name);
  id = NUM2INT(arg);

  r = ngraph_object_move_last(nobj, id);
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

  nobj = ngraph_get_object(name);
  id1 = NUM2INT(arg1);
  id2 = NUM2INT(arg2);

  r = ngraph_object_exchange(nobj, id1, id2);
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

  nobj = ngraph_get_object(name);
  id1 = NUM2INT(arg1);
  id2 = NUM2INT(arg2);

  r = ngraph_object_copy(nobj, id1, id2);
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

  nobj = ngraph_get_object(name);
  n = ngraph_get_object_last_id(nobj);
  n = (n >= 0) ? n + 1 : 0;

  return INT2FIX(n);
}

static VALUE
obj_exist(VALUE klass, const char *name)
{
  struct objlist *nobj;
  int n;

  nobj = ngraph_get_object(name);
  n = ngraph_get_object_last_id(nobj);

  return (n >= 0) ? Qtrue : Qfalse;
}

static VALUE
obj_del_from_str(VALUE klass, VALUE arg, const char *name)
{
  const char *str;
  int i, *ids, n;
  struct objlist *obj;
  VALUE inst;

  str = StringValueCStr(arg);
  inst = rb_sprintf("%s:%s", name, str);
  obj = ngraph_get_object_instances_by_str(StringValueCStr(inst), &n, &ids);
  if (obj == NULL) {
    return klass;
  }

  for (i = n - 1;  i >= 0; i--) {
    ngraph_object_del(obj, ids[i]);
  }
  ngraph_free(ids);

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
  nobj = ngraph_get_object(name);
  n = ngraph_get_object_last_id(nobj);

  if (id < 0) {
    id = n + id + 1;
  }

  if (id < 0 || id > n) {
    return Qnil;
  }

  ngraph_object_del(nobj, id);

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
  ngraph_object_del(inst->obj, id);

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

  nobj = ngraph_get_object(name);
  r = ngraph_object_new(nobj);
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

  nobj = ngraph_get_object(name);
  id = ngraph_get_object_current_id(nobj);
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
  inst->rcode = ngraph_object_get(inst->obj, field, inst->id, &carg, &num);
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
  inst->rcode = ngraph_object_get(inst->obj, field, inst->id, &carg, &num);
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
  inst->rcode = ngraph_object_get(inst->obj, field, inst->id, &carg, &num);
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
  inst->rcode = ngraph_object_get(inst->obj, field, inst->id, &carg, &str);
  if (inst->rcode < 0) {
    return Qnil;
  }

  return tainted_utf8_str_new(str.str);
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
  inst->rcode = ngraph_object_get(inst->obj, field, inst->id, &carg, &str);
  if (inst->rcode < 0) {
    return Qnil;
  }

  if (str.str == NULL) {
    return Qnil;
  }

  obj = ngraph_get_object_instances_by_str(str.str, &n, &ids);
  if (obj == NULL) {
    return Qnil;
  }

  id = ids[n - 1];
  ngraph_free(ids);

  name = ngraph_get_object_name(obj);
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
  inst->rcode = ngraph_object_put(inst->obj, field, inst->id, &num);
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
  inst->rcode = ngraph_object_put(inst->obj, field, inst->id, &num);
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
  inst->rcode = ngraph_object_put(inst->obj, field, inst->id, &num);
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

  inst->rcode = ngraph_object_put(inst->obj, field, inst->id, &num);
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
  inst->rcode = ngraph_object_put(inst->obj, field, inst->id, &str);
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
    obj = ngraph_get_object_instances_by_str(ptr, NULL, &ids);
    if (obj == NULL) {
      rb_raise(rb_eArgError, "%s#%s: illegal instance representation (%s).", rb_obj_classname(self), field, ptr);
    }
    ngraph_free(ids);
    break;
  default:
    if (! rb_obj_is_kind_of(arg, NgraphClass)) {
      rb_raise(rb_eArgError, "%s#%s: illegal type of the argument (%s).", rb_obj_classname(self), field, rb_obj_classname(arg));
    }

    inst2 = check_id(arg);
    if (inst2 == NULL) {
      return Qnil;
    }

    name = ngraph_get_object_name(inst2->obj);
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
  inst1->rcode = ngraph_object_put(inst1->obj, field, inst1->id, &str);
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
  int num;

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
    ary.ary = rb_alloc_tmp_buffer(&tmpstr, sizeof(*ary.ary) + sizeof(ngraph_value) * num);
    ary.ary->num = num;
    if (ary.ary) {
      int i;
      for (i = 0; i < num; i++) {
        ary.ary->ary[i].i = NUM2INT(rb_ary_entry(arg, i));
      }
    }
  }

  inst->rcode = ngraph_object_put(inst->obj, field, inst->id, &ary);

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
  inst->rcode = ngraph_object_get(inst->obj, field, inst->id, &carg, &cary);
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
  int num;

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
    ary.ary = rb_alloc_tmp_buffer(&tmpstr, sizeof(*ary.ary) + sizeof(ngraph_value) * num);
    ary.ary->num = num;
    if (ary.ary) {
      int i;
      for (i = 0; i < num; i++) {
        ary.ary->ary[i].d = NUM2DBL(rb_ary_entry(arg, i));
      }
    }
  }

  inst->rcode = ngraph_object_put(inst->obj, field, inst->id, &ary);

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
  inst->rcode = ngraph_object_get(inst->obj, field, inst->id, &carg, &cary);
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
  int num;
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
    ary.ary =  rb_alloc_tmp_buffer(&tmpstr, sizeof(*ary.ary) + sizeof(ngraph_value) * num);
    ary.ary->num = num;
    if (ary.ary) {
      int i;
      for (i = 0; i < num; i++) {
        str = rb_ary_entry(arg, i);
        ary.ary->ary[i].str = StringValueCStr(str);
      }
    }
  }

  inst->rcode = ngraph_object_put(inst->obj, field, inst->id, &ary);

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
  inst->rcode = ngraph_object_get(inst->obj, field, inst->id, &carg, &cary);
  if (inst->rcode < 0) {
    return Qnil;
  }

  ary = rb_ary_new2(cary.ary.num);
  for (i = 0; i < cary.ary.num; i++) {
    rb_ary_store(ary, i, tainted_utf8_str_new(cary.ary.data.sa[i]));
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
  inst->rcode = ngraph_object_exe(inst->obj, field, inst->id, &carg);
  if (inst->rcode < 0) {
    return Qnil;
  }

  return self;
}

static VALUE
get_array_arg(VALUE self, const char *field, VALUE arg, int *num)
{
  int n;
  VALUE ary, val;

  if (NIL_P(arg)) {
    *num = 0;
    return Qnil;
  }

  if (!RB_TYPE_P(arg, T_ARRAY)) {
    rb_raise(rb_eArgError, "%s#%s: the argument must be an Array", rb_obj_classname(self), field);
  }

  n = RARRAY_LEN(arg);
  ary = arg;

  if (n == 1) {
    val = rb_ary_entry(arg, 0);
    if (RB_TYPE_P(val, T_ARRAY)) {
      ary = val;
      n = RARRAY_LEN(val);
    }
  }

  *num = n;
  return ary;
}

static struct ngraph_array *
allocate_iarray(VALUE self, volatile VALUE *tmpstr, VALUE arg, const char *field)
{
  struct ngraph_array *narray;
  int i, num;
  VALUE ary;

  ary = get_array_arg(self, field, arg, &num);

  narray = rb_alloc_tmp_buffer(tmpstr, sizeof(*narray) + sizeof(ngraph_value) * num);
  if (narray == NULL) {
    return NULL;
  }

  narray->num = num;
  for (i = 0; i < num; i++) {
    narray->ary[i].i = NUM2INT(rb_ary_entry(ary, i));
  }

  return narray;
}

static struct ngraph_array *
allocate_darray(VALUE self, volatile VALUE *tmpstr, VALUE arg, const char *field)
{
  struct ngraph_array *narray;
  int i, num;
  VALUE ary;

  ary = get_array_arg(self, field, arg, &num);

  narray = rb_alloc_tmp_buffer(tmpstr, sizeof(*narray) + sizeof(ngraph_value) * num);
  if (narray == NULL) {
    return NULL;
  }

  narray->num = num;
  for (i = 0; i < num; i++) {
    narray->ary[i].d = NUM2DBL(rb_ary_entry(ary, i));
  }

  return narray;
}

static struct ngraph_array *
allocate_sarray(VALUE self, volatile VALUE *tmpstr, VALUE arg, const char *field)
{
  struct ngraph_array *narray;
  int i, num;
  VALUE str;
  VALUE ary;

  ary = get_array_arg(self, field, arg, &num);

  narray = rb_alloc_tmp_buffer(tmpstr, sizeof(*narray) + sizeof(ngraph_value) * num);
  if (narray == NULL) {
    return NULL;
  }

  narray->num = num;
  for (i = 0; i < num; i++) {
    str = rb_ary_entry(ary, i);
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
      name = ngraph_get_object_name(inst->obj);
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
  int i;
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
      int n;
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
    inst->rcode = ngraph_object_exe(inst->obj, field, inst->id, &carg);
  } else {
    inst->rcode = ngraph_object_get(inst->obj, field, inst->id, &carg, &rval);
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
    val = tainted_utf8_str_new(rval.str);
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
      rb_ary_store(val, i, tainted_utf8_str_new(rval.ary.data.sa[i]));
    }
    break;
  default:
    val = Qnil;
  }

  return val;
}

#if 0
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
  inst->rcode = ngraph_object_get(inst->obj, field, inst->id, carg, &rval);
  rb_free_tmp_buffer(&tmpstr);
  if (inst->rcode < 0) {
    return Qnil;
  }

  return tainted_utf8_str_new(rval.str ? rval.str : "");
}
#endif

static void
add_obj_name_const(VALUE klass, struct objlist *nobj, const char *name)
{
  VALUE val;

  if (nobj == NULL) {
    val = Qnil;
  } else {
    const char *obj_name;
    char str[64];
    obj_name = ngraph_get_object_name(nobj);
    set_ngraph_obj_class_name(obj_name, str, sizeof(str));
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

  nobj = ngraph_get_object(name);
  next = ngraph_get_next_object(nobj);
  child = ngraph_get_child_object(nobj);
  parent = ngraph_get_parent_object(nobj);

  str = ngraph_get_object_version(nobj);
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

#include "ruby_ngraph.h"

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
nputs(VALUE module, VALUE str)
{
  ngraph_puts(StringValueCStr(str));
  return Qnil;
}

static VALUE
nputerr(VALUE module, VALUE str)
{
  ngraph_err_puts(StringValueCStr(str));
  return Qnil;
}

static VALUE
nsleep(VALUE module, VALUE arg)
{
  double t;

  t = NUM2DBL(arg);
  ngraph_sleep(t);

  return Qnil;
}

static VALUE
ngraph_str2inst(VALUE module, VALUE arg)
{
  const char *str;
  VALUE ary;
  struct obj_ids obj_ids;

  str = StringValueCStr(arg);

  obj_ids.obj = ngraph_get_object_instances_by_str(str, &obj_ids.num, &obj_ids.ids);
  if (obj_ids.obj == NULL) {
    return Qnil;
  }

  ary = rb_ensure(str2inst_get_ary, (VALUE) &obj_ids, str2inst_ensure, (VALUE) obj_ids.ids);

  return ary;
}

static VALUE
ngraph_save_hist(VALUE module)
{
  ngraph_save_shell_history();

  return Qnil;
}

static VALUE
ruby_ngraph_init_file(VALUE module, VALUE arg)
{
  char *file;
  const char *init_file;
  size_t len;

  init_file = StringValueCStr(arg);
  file = ngraph_get_init_file(init_file);
  if (file == NULL) {
    return Qnil;
  }

  len = strlen(file);

  return rb_enc_str_new(file, len, rb_utf8_encoding());
}

static VALUE
ruby_ngraph_exec_loginshell(VALUE module, VALUE cmd, VALUE nobj)
{
  int r;
  char *loginshell;
  static struct ngraph_instance *inst;

  if (! rb_obj_is_kind_of(nobj, NgraphClass)) {
    rb_raise(rb_eArgError, "%s: illegal type of the argument (%s).", rb_obj_classname(module), rb_obj_classname(nobj));
  }

  if (NIL_P(cmd)) {
    loginshell = NULL;
  } else {
    size_t len;
    const char *str;
    str = StringValueCStr(cmd);
    len = strlen(str) + 1;
    loginshell = ALLOCA_N(char, len);
    if (loginshell == NULL) {
      rb_raise(rb_eSysStackError, "%s: cannot allocate enough memory.", rb_obj_classname(module));
    }
    strcpy(loginshell, str);
  }

  inst = check_id(nobj);
  r = ngraph_exec_loginshell(loginshell, inst->obj, inst->id);

  return INT2FIX(r);
}

static VALUE
wrap_load_script(VALUE file)
{
  rb_load(file, 1);
  return Qnil;
}

static int
load_script(int argc, char **argv)
{
  VALUE r_argv, fname;
  int state, i;

  if (argc < 1) {
    return 0;
  }

  r_argv = rb_const_get(rb_mKernel, rb_intern("ARGV"));
  rb_ary_clear(r_argv);
  for (i = 1; i < argc; i++) {
    rb_ary_push(r_argv, rb_str_new2(argv[i]));
  }

  fname = rb_funcall(rb_cFile, ExpandPath, 1, rb_str_new2(argv[0]));
  rb_protect(wrap_load_script, fname, &state);
  if (state) {
    VALUE errinfo, errstr, errat;
    const char *cstr;

    errinfo = rb_errinfo();
    errstr = rb_obj_as_string(errinfo);
    cstr = StringValueCStr(errstr);
    if (strcmp(cstr, "exit")) {
      ngraph_err_puts(cstr);
      errat = rb_funcall(errinfo, rb_intern("backtrace"), 0);
      if (! NIL_P(errat)) {
        int n;
	n = RARRAY_LEN(errat);
	for (i = 0; i < n; i ++) {
	  errstr = rb_str_new2("\tfrom ");
	  rb_str_append(errstr, rb_ary_entry(errat, i));
	  ngraph_err_puts(StringValueCStr(errstr));
	}
      }
    }
  }
  rb_gc_start();

  return 0;
}

static VALUE
ruby_ngraph_init(VALUE module, VALUE arg)
{
  char *str;

  if (Initialized) {
    return Qnil;
  }

  if (! NIL_P(arg)) {
    str = strdup(StringValueCStr(arg));
    if (str) {
      DummyArgv[0] = str;
    }
  }

  ngraph_initialize(&DummyArgc, &DummyArgvPtr);
  create_ngraph_classes(module, NgraphClass);

  Initialized = TRUE;

  ngraph_set_exec_func("ruby", load_script);

  return Qnil;
}

void
Init_ngraph(void)
{
  if (Initialized) {
    return;
  }

  Uniq = rb_intern("uniq");
  ExpandPath = rb_intern("expand_path");

  NgraphModule = rb_define_module("Ngraph");
  rb_define_singleton_method(NgraphModule, "puts", nputs, 1);
  rb_define_singleton_method(NgraphModule, "err_puts", nputerr, 1);
  rb_define_singleton_method(NgraphModule, "sleep", nsleep, 1);
  rb_define_singleton_method(NgraphModule, "str2inst", ngraph_str2inst, 1);
  rb_define_singleton_method(NgraphModule, "save_shell_history", ngraph_save_hist, 0);
  rb_define_singleton_method(NgraphModule, "ngraph_initialize", ruby_ngraph_init, 1);
  rb_define_singleton_method(NgraphModule, "get_initialize_file", ruby_ngraph_init_file, 1);
  rb_define_singleton_method(NgraphModule, "execute_loginshell", ruby_ngraph_exec_loginshell, 2);

  NgraphClass = rb_define_class_under(NgraphModule, "NgraphObject", rb_cObject);
  rb_define_method(NgraphClass, "initialize", ngraph_class_new, 0);
  add_common_const(NgraphModule);
}

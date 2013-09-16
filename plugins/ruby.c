#include <ruby.h>
#include <stdio.h>
#include <gmodule.h>

#include "../src/ngraph_plugin_shell.h"

static int Initialized = FALSE;

#define VAL2INT(val) (NIL_P(val) ? 0 : NUM2INT(val))
#define VAL2DBL(val) (NIL_P(val) ? 0.0 : NUM2DBL(val))
#define VAL2STR(val) (NIL_P(val) ? NULL : StringValuePtr(val))

struct ngraph_instance {
  int id, oid;
  struct objlist *obj;
};

static void
ngraph_object_free(struct ngraph_instance *inst)
{
  free(inst);
}

static VALUE
ngraph_inst_method_equal(VALUE klass1, VALUE klass2)
{
  struct ngraph_instance *inst1, *inst2;

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
  int id, r;
  ngraph_arg arg;
  ngraph_returned_value oid;

  Data_Get_Struct(klass, struct ngraph_instance, inst);

  arg.num = 0;
  r = ngraph_plugin_shell_getobj(inst->obj, "oid", inst->id, &arg, &oid);

  if (r >= 0 && inst->oid == oid.i) {
    return inst;
  }

  if (r < 0) {
    inst->id = -1;
    return NULL;
  }

  id = ngraph_plugin_shell_get_id_by_oid(inst->obj, inst->oid);
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

  inst->id = ngraph_plugin_shell_move_up(inst->obj, inst->id);

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

  inst->id = ngraph_plugin_shell_move_down(inst->obj, inst->id);

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

  inst->id = ngraph_plugin_shell_move_top(inst->obj, inst->id);

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

  inst->id = ngraph_plugin_shell_move_last(inst->obj, inst->id);

  return klass;
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

  id = NUM2INT(id_value);

  nobj = ngraph_plugin_shell_get_object(name);
  n = ngraph_plugin_shell_get_obj_last_id(nobj);

  if (id < 0) {
    id = n + id + 1;
  }

  if (id < 0 || id > n) {
    return Qnil;
  }

  new_inst = Data_Make_Struct(klass, struct ngraph_instance, NULL, ngraph_object_free, inst);

  inst->obj = nobj;
  inst->id = id;
  arg.num = 0;
  ngraph_plugin_shell_getobj(inst->obj, "oid", inst->id, &arg, &oid);
  inst->oid = oid.i;

  return new_inst;
}

static VALUE
obj_field_args(VALUE klass, VALUE field, const char *name)
{
  struct objlist *nobj;
  const char* args;
  int type;

  nobj = ngraph_plugin_shell_get_object(name);
  type = ngraph_plugin_shell_get_obj_field_type(nobj, StringValuePtr(field));
  if (type < 20) {
    return Qnil;
  }
  args = ngraph_plugin_shell_get_obj_field_args(nobj, StringValuePtr(field));
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

  nobj = ngraph_plugin_shell_get_object(name);
  type = ngraph_plugin_shell_get_obj_field_type(nobj, StringValuePtr(field));

  return INT2FIX(type);
}

static VALUE
obj_field_permission(VALUE klass, VALUE field, const char *name)
{
  struct objlist *nobj;
  int perm;

  nobj = ngraph_plugin_shell_get_object(name);
  perm = ngraph_plugin_shell_get_obj_field_permission(nobj, StringValuePtr(field));

  return INT2FIX(perm);
}

static VALUE
obj_each(VALUE klass, const char *name)
{
  struct objlist *nobj;
  int i, n;
  VALUE inst, id;

  nobj = ngraph_plugin_shell_get_object(name);
  n = ngraph_plugin_shell_get_obj_last_id(nobj);
  name = ngraph_plugin_shell_get_obj_name(nobj);

  for (i = 0; i <= n; i++) {
    id = INT2FIX(i);
    inst = obj_get(klass, id, name);
    rb_yield(inst);
  }

  return klass;
}

static VALUE
obj_size(VALUE klass, const char *name)
{
  struct objlist *nobj;
  int n;

  nobj = ngraph_plugin_shell_get_object(name);
  n = ngraph_plugin_shell_get_obj_last_id(nobj);

  return INT2FIX(n);
}

static VALUE
obj_del(VALUE klass, VALUE id_value, const char *name)
{
  int id, n;
  struct objlist *nobj;

  id = NUM2INT(id_value);
  nobj = ngraph_plugin_shell_get_object(name);
  n = ngraph_plugin_shell_get_obj_last_id(nobj);

  if (id < 0) {
    id = n + id + 1;
  }

  if (id < 0 || id > n) {
    return Qnil;
  }

  ngraph_plugin_shell_del(nobj, id);

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
  ngraph_plugin_shell_del(inst->obj, id);

  return INT2FIX(id);
}

static VALUE
obj_new(VALUE klass, const char *name)
{
  VALUE new_inst;
  struct objlist *nobj;
  int r;

  nobj = ngraph_plugin_shell_get_object(name);
  r = ngraph_plugin_shell_new(nobj);
  if (r < 0) {
    return Qnil;
  }

  new_inst = obj_get(klass, INT2FIX(r), name);

  return new_inst;
}

static VALUE
inst_get_int(VALUE self, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_returned_value num;
  ngraph_arg carg;
  int r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(inst->obj, field, inst->id, &carg, &num);
  if (r < 0) {
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
  int r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(inst->obj, field, inst->id, &carg, &num);
  if (r < 0) {
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
  int r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(inst->obj, field, inst->id, &carg, &num);
  if (r < 0) {
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
  const char *cstr;
  int r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(inst->obj, field, inst->id, &carg, &str);
  if (r < 0) {
    return Qnil;
  }

  if (str.str == NULL) {
    cstr = "";
  } else {
    cstr = str.str;
  }

  return rb_str_new2(cstr);
}

static VALUE
inst_put_int(VALUE self, VALUE arg, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_value num;
  int r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  num.i = NUM2INT(arg);
  r = ngraph_plugin_shell_putobj(inst->obj, field, inst->id, &num);
  if (r < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_double(VALUE self, VALUE arg, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_value num;
  int r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  num.d = NUM2DBL(arg);
  r = ngraph_plugin_shell_putobj(inst->obj, field, inst->id, &num);
  if (r < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_bool(VALUE self, VALUE arg, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_value num;
  int r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  num.i = RTEST(arg) ? 1 : 0;
  r = ngraph_plugin_shell_putobj(inst->obj, field, inst->id, &num);
  if (r < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_enum(VALUE self, VALUE arg, const char *field, int max)
{
  struct ngraph_instance *inst;
  ngraph_value num;
  int r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  num.i = NUM2INT(arg);
  if (num.i < 0 || num.i > max) {
    return Qnil;
  }

  r = ngraph_plugin_shell_putobj(inst->obj, field, inst->id, &num);
  if (r < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_str(VALUE self, VALUE arg, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_value str;
  int r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  if (NIL_P(arg)) {
    str.str = NULL;
  } else {
    str.str = StringValuePtr(arg);
  }
  r = ngraph_plugin_shell_putobj(inst->obj, field, inst->id, &str);
  if (r < 0) {
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
  int num, i, r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  if (!RB_TYPE_P(arg, T_ARRAY)) {
    rb_raise(rb_eArgError, "arg must be an Array");
  }

  num = RARRAY_LEN(arg);
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

  r = ngraph_plugin_shell_putobj(inst->obj, field, inst->id, &ary);

  if (ary.ary) {
    rb_free_tmp_buffer(&tmpstr);
  }

  if (r < 0) {
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
  int i, r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(inst->obj, field, inst->id, &carg, &cary);
  if (r < 0) {
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
  int num, i, r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  if (!RB_TYPE_P(arg, T_ARRAY)) {
    rb_raise(rb_eArgError, "arg must be an Array");
  }

  num = RARRAY_LEN(arg);
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

  r = ngraph_plugin_shell_putobj(inst->obj, field, inst->id, &ary);

  if (ary.ary) {
    rb_free_tmp_buffer(&tmpstr);
  }

  if (r < 0) {
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
  int i, r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(inst->obj, field, inst->id, &carg, &cary);
  if (r < 0) {
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
  int num, i, r;
  VALUE str, tmpstr;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  if (!RB_TYPE_P(arg, T_ARRAY)) {
    rb_raise(rb_eArgError, "arg must be an Array");
  }

  num = RARRAY_LEN(arg);
  ary.ary = NULL;
  if (num > 0) {
    ary.ary =  rb_alloc_tmp_buffer(&tmpstr, sizeof(*ary.ary) + sizeof(union array) * num);
    ary.ary->num = num;
    if (ary.ary) {
      for (i = 0; i < num; i++) {
        str = rb_ary_entry(arg, i);
        ary.ary->ary[i].str = StringValuePtr(str);
      }
    }
  }

  r = ngraph_plugin_shell_putobj(inst->obj, field, inst->id, &ary);

  if (ary.ary) {
    rb_free_tmp_buffer(&tmpstr);
  }

  if (r < 0) {
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
  int i, r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(inst->obj, field, inst->id, &carg, &cary);
  if (r < 0) {
    return Qnil;
  }

  ary = rb_ary_new2(cary.ary.num);
  for (i = 0; i < cary.ary.num; i++) {
    rb_ary_store(ary, i, rb_str_new2(cary.ary.data.sa[i]));
  }

  return ary;
}

static VALUE
inst_exe_void_func(VALUE self, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_arg carg;
  int r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_exeobj(inst->obj, field, inst->id, &carg);
  if (r < 0) {
    return Qnil;
  }

  return self;
}

static struct ngraph_array *
allocate_iarray(volatile VALUE *tmpstr, VALUE arg)
{
  struct ngraph_array *narray;
  int i, num;

  if (!RB_TYPE_P(arg, T_ARRAY)) {
    rb_raise(rb_eArgError, "arg must be an Array");
  }

  if (NIL_P(arg)) {
    num = 0;
  } else {
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
allocate_darray(volatile VALUE *tmpstr, VALUE arg)
{
  struct ngraph_array *narray;
  int i, num;

  if (!RB_TYPE_P(arg, T_ARRAY)) {
    rb_raise(rb_eArgError, "arg must be an Array");
  }

  if (NIL_P(arg)) {
    num = 0;
  } else {
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
allocate_sarray(volatile VALUE *tmpstr, VALUE arg)
{
  struct ngraph_array *narray;
  int i, num;
  VALUE str;

  if (NIL_P(arg)) {
    num = 0;
  } else {
    if (!RB_TYPE_P(arg, T_ARRAY)) {
      rb_raise(rb_eArgError, "arg must be an Array");
    }

    num = RARRAY_LEN(arg);
  }
  narray = rb_alloc_tmp_buffer(tmpstr, sizeof(*narray) + sizeof(union array) * num);

  narray->num = num;
  for (i = 0; i < num; i++) {
    str = rb_ary_entry(arg, i);
    narray->ary[i].str = StringValuePtr(str);
  }

  return narray;
}

static VALUE
exe_void_func_argv(VALUE self, VALUE argv, const char *field)
{
  struct ngraph_instance *inst;
  ngraph_arg *carg;
  int r;
  VALUE tmpstr;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }
  carg = allocate_sarray(&tmpstr, argv);
  r = ngraph_plugin_shell_exeobj(inst->obj, field, inst->id, carg);
  rb_free_tmp_buffer(&tmpstr);
  if (r < 0) {
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
  int r;
  VALUE tmpstr;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }
  carg = allocate_sarray(&tmpstr, argv);
  r = ngraph_plugin_shell_getobj(inst->obj, field, inst->id, carg, &rval);
  rb_free_tmp_buffer(&tmpstr);
  if (r < 0) {
    return Qnil;
  }

  return rb_str_new2(rval.str ? rval.str : "");
}

static void
add_obj_name_const(VALUE klass, struct objlist *nobj, const char *name)
{
  const char *obj_name;
  VALUE val, str;

  if (nobj == NULL) {
    val = Qnil;
  } else {
    obj_name = ngraph_plugin_shell_get_obj_name(nobj);
    str = rb_str_new2(obj_name);
    rb_funcall(str, rb_to_id(rb_str_new2("capitalize!")), 0);
    val = ID2SYM(rb_to_id(str));
  }
  rb_define_const(klass, name, val);
}

static void
add_obj_const(VALUE klass, const char *name)
{
  struct objlist *nobj, *next, *child, *parent;
  const char *str;
  VALUE val;

  nobj = ngraph_plugin_shell_get_object(name);
  next = ngraph_plugin_shell_get_obj_next(nobj);
  child = ngraph_plugin_shell_get_obj_child(nobj);
  parent = ngraph_plugin_shell_get_obj_parent(nobj);

  str = ngraph_plugin_shell_get_obj_version(nobj);
  val = rb_str_new2(str);
  OBJ_FREEZE(val);
  rb_define_const(klass, "VERSION", val);

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

int
ngraph_plugin_shell_shell_libruby(struct plugin_shell *shell, int argc, char *argv[])
{
  VALUE ngraph_module;
  int state;

  if (! Initialized) {
    ruby_init();
    ruby_script("Embedded Ruby on Ngraph");
    ruby_init_loadpath();
    Initialized = TRUE;

    ngraph_module = rb_define_module("Ngraph");
    add_common_const(ngraph_module);
    create_ngraph_classes(ngraph_module);
  }

  rb_load_protect(rb_str_new2(argv[1]), 1, &state);
  if (state)
  {
    printf("some errors are occurred\n");
  }
  rb_gc_start();
  return 0;
}

void
g_module_unload(GModule *module)
{
  if (Initialized) {
    ruby_finalize();
  }
}

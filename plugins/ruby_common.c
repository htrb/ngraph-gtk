static int Initialized = FALSE;

#define VAL2INT(val) (NIL_P(val) ? 0 : NUM2INT(val))
#define VAL2DBL(val) (NIL_P(val) ? 0.0 : NUM2DBL(val))
#define VAL2STR(val) (NIL_P(val) ? NULL : StringValuePtr(val))

struct ngraph_object {
  int id, oid;
  struct objlist *obj;
};

static void
ngraph_object_free(struct ngraph_object *obj)
{
  free(obj);
}

static VALUE
ngraph_inst_method_equal(VALUE klass1, VALUE klass2)
{
  struct ngraph_object *obj1, *obj2;

  Data_Get_Struct(klass1, struct ngraph_object, obj1);
  Data_Get_Struct(klass2, struct ngraph_object, obj2);

  if (obj1->obj == obj2->obj &&
      obj1->oid == obj2->oid) {
    return Qtrue;
  }

  return Qfalse;
}

static VALUE
ngraph_inst_method_compare(VALUE klass1, VALUE klass2)
{
  struct ngraph_object *obj1, *obj2;
  int r;

  Data_Get_Struct(klass1, struct ngraph_object, obj1);
  Data_Get_Struct(klass2, struct ngraph_object, obj2);

  if (obj1->obj != obj2->obj) {
    return Qnil;
  }

  if (obj1->oid == obj2->oid) {
    r = 0;
  } else if (obj1->oid > obj2->oid) {
    r = 1;
  } else {
    r = -1;
  }

  return INT2FIX(r);
}

static struct ngraph_object *
check_id(VALUE klass)
{
  struct ngraph_object *obj;
  int id, r;
  ngraph_arg arg;
  ngraph_returned_value oid;

  Data_Get_Struct(klass, struct ngraph_object, obj);

  arg.num = 0;
  r = ngraph_plugin_shell_getobj(obj->obj, "oid", obj->id, &arg, &oid);

  if (r >= 0 && obj->oid == oid.i) {
    return obj;
  }

  if (r < 0) {
    obj->id = -1;
    return NULL;
  }

  id = ngraph_plugin_shell_get_id_by_oid(obj->obj, obj->oid);
  obj->id = id;

  return obj;
}

static VALUE
obj_get(VALUE klass, VALUE id_value, const char *name)
{
  struct ngraph_object *obj;
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

  new_inst = Data_Make_Struct(klass, struct ngraph_object, NULL, ngraph_object_free, obj);

  obj->obj = nobj;
  obj->id = id;
  arg.num = 0;
  ngraph_plugin_shell_getobj(obj->obj, "oid", obj->id, &arg, &oid);
  obj->oid = oid.i;

  return new_inst;
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
  struct ngraph_object *obj;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  id = obj->id;
  obj->id = -1;
  ngraph_plugin_shell_del(obj->obj, id);

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
  struct ngraph_object *obj;
  ngraph_returned_value num;
  ngraph_arg carg;
  int r;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(obj->obj, field, obj->id, &carg, &num);
  if (r < 0) {
    return Qnil;
  }

  return INT2NUM(num.i);
}

static VALUE
inst_get_double(VALUE self, const char *field)
{
  struct ngraph_object *obj;
  ngraph_returned_value num;
  ngraph_arg carg;
  int r;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(obj->obj, field, obj->id, &carg, &num);
  if (r < 0) {
    return Qnil;
  }

  return rb_float_new(num.d);
}

static VALUE
inst_get_bool(VALUE self, const char *field)
{
  struct ngraph_object *obj;
  ngraph_returned_value num;
  ngraph_arg carg;
  VALUE val;
  int r;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(obj->obj, field, obj->id, &carg, &num);
  if (r < 0) {
    return Qnil;
  }

  val = num.i ? Qtrue : Qfalse;

  return val;
}

static VALUE
inst_get_str(VALUE self, const char *field)
{
  struct ngraph_object *obj;
  ngraph_returned_value str;
  ngraph_arg carg;
  const char *cstr;
  int r;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(obj->obj, field, obj->id, &carg, &str);
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
  struct ngraph_object *obj;
  ngraph_value num;
  int r;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  num.i = NUM2INT(arg);
  r = ngraph_plugin_shell_putobj(obj->obj, field, obj->id, &num);
  if (r < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_double(VALUE self, VALUE arg, const char *field)
{
  struct ngraph_object *obj;
  ngraph_value num;
  int r;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  num.d = NUM2DBL(arg);
  r = ngraph_plugin_shell_putobj(obj->obj, field, obj->id, &num);
  if (r < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_bool(VALUE self, VALUE arg, const char *field)
{
  struct ngraph_object *obj;
  ngraph_value num;
  int r;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  num.i = RTEST(arg) ? 1 : 0;
  r = ngraph_plugin_shell_putobj(obj->obj, field, obj->id, &num);
  if (r < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_enum(VALUE self, VALUE arg, const char *field, int max)
{
  struct ngraph_object *obj;
  ngraph_value num;
  int r;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  num.i = NUM2INT(arg);
  if (num.i < 0 || num.i > max) {
    return Qnil;
  }

  r = ngraph_plugin_shell_putobj(obj->obj, field, obj->id, &num);
  if (r < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_str(VALUE self, VALUE arg, const char *field)
{
  struct ngraph_object *obj;
  ngraph_value str;
  int r;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  if (NIL_P(arg)) {
    str.str = NULL;
  } else {
    str.str = StringValuePtr(arg);
  }
  r = ngraph_plugin_shell_putobj(obj->obj, field, obj->id, &str);
  if (r < 0) {
    return Qnil;
  }

  return arg;
}

static VALUE
inst_put_iarray(VALUE self, VALUE arg, const char *field)
{
  VALUE tmpstr;
  struct ngraph_object *obj;
  ngraph_value ary;
  int num, i, r;

  obj = check_id(self);
  if (obj == NULL) {
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

  r = ngraph_plugin_shell_putobj(obj->obj, field, obj->id, &ary);

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
  struct ngraph_object *obj;
  ngraph_returned_value cary;
  ngraph_arg carg;
  VALUE ary;
  int i, r;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(obj->obj, field, obj->id, &carg, &cary);
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
  struct ngraph_object *obj;
  ngraph_value ary;
  int num, i, r;

  obj = check_id(self);
  if (obj == NULL) {
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

  r = ngraph_plugin_shell_putobj(obj->obj, field, obj->id, &ary);

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
  struct ngraph_object *obj;
  ngraph_returned_value cary;
  ngraph_arg carg;
  VALUE ary;
  int i, r;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(obj->obj, field, obj->id, &carg, &cary);
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
  struct ngraph_object *obj;
  ngraph_value ary;
  int num, i, r;
  VALUE str, tmpstr;

  obj = check_id(self);
  if (obj == NULL) {
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

  r = ngraph_plugin_shell_putobj(obj->obj, field, obj->id, &ary);

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
  struct ngraph_object *obj;
  ngraph_returned_value cary;
  ngraph_arg carg;
  VALUE ary;
  int i, r;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_getobj(obj->obj, field, obj->id, &carg, &cary);
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
  struct ngraph_object *obj;
  ngraph_arg carg;
  int r;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }

  carg.num = 0;
  r = ngraph_plugin_shell_exeobj(obj->obj, field, obj->id, &carg);
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
  struct ngraph_object *obj;
  ngraph_arg *carg;
  int r;
  VALUE tmpstr;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }
  carg = allocate_sarray(&tmpstr, argv);
  r = ngraph_plugin_shell_exeobj(obj->obj, field, obj->id, carg);
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
  struct ngraph_object *obj;
  ngraph_arg *carg;
  int r;
  VALUE tmpstr;

  obj = check_id(self);
  if (obj == NULL) {
    return Qnil;
  }
  carg = allocate_sarray(&tmpstr, argv);
  r = ngraph_plugin_shell_getobj(obj->obj, field, obj->id, carg, &rval);
  rb_free_tmp_buffer(&tmpstr);
  if (r < 0) {
    return Qnil;
  }

  return rb_str_new2(rval.str ? rval.str : "");
}

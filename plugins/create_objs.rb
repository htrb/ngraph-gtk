#! /usr/bin/ruby

class Field
  attr_reader :name, :func, :argc

  def initialize(name, func, argc)
    @name = name
    @func = func
    @argc = argc
  end
end

class Enum
  attr_reader :module, :enum, :max

  def initialize(name, enum)
    @module = name.capitalize
    @enum = enum.map {|e| e.upcase.gsub(".", "_")}
    @max = enum.size - 1
  end
end

class NgraphObj
  attr_reader :name, :fields
  SINGLETON_METHOD = [
                      ["new", "new", 0],
                      ["[]", "get", 1],
                      ["del", "del", 1],
                      ["each", "each", 0],
                      ["size", "size", 0],
                      ["get_field_args", "field_args", 1],
                      ["get_field_type", "field_type", 1],
                      ["get_field_permission", "field_permission", 1],
                     ]

  FUNC_FIELD_COMMON = <<EOF
  struct ngraph_instance *inst;
  ngraph_arg *carg;
  int r;

  inst = check_id(self);
  if (inst == NULL) {
    return Qnil;
  }
EOF

  FUNC_FIELD_VAL = "  ngraph_returned_value rval;\n"

  FUNC_FIELD_ARY = <<EOF
  ngraph_returned_value rval;
  VALUE ary;
  int i;
EOF

  def initialize(name, cfile, version, parent)
    @name = name
    @fields = []
    @methods = []
    @enum = {}
    @cfile = cfile
    @version = version
    @parent = (parent == "(null)") ? nil : parent
  end

  def add_singleton_method_func(method, func, argc)
    @cfile.puts("static VALUE")
    @cfile.print("#{@name}_#{func}(VALUE klass")
    if (argc > 0)
      @cfile.print(", ")
      argc.times { |i|
        @cfile.print("VALUE arg#{i}")
        if (i == argc - 1)
          @cfile.puts(")\n{")
        else
          @cfile.print(", ")
        end
      }
    else
      @cfile.puts(")\n{")
    end
    @cfile.print("  return obj_#{func}(klass, ")
    argc.times { |i|
      @cfile.print("arg#{i}, ")
    }
    @cfile.puts(%Q!"#{@name}");\n}!);
  end

  def create_obj
    SINGLETON_METHOD.each { |method, func, argc|
      add_singleton_method_func(method, func, argc)
    }
    @cfile.puts <<EOF
static void
create_#{@name}_object(VALUE ngraph_module)
{
  VALUE obj;
  VALUE fields, field;
#{(@enum.size > 0) ? "  VALUE module;" : ""}
  obj = rb_define_class_under(ngraph_module, "#{@name.capitalize}", rb_cObject);
EOF
    SINGLETON_METHOD.each { |method, func, argc|
      @cfile.puts(%Q!  rb_define_singleton_method(obj, "#{method}", #{@name}_#{func}, #{argc});!)
    }
    @cfile.puts <<EOF
  add_obj_const(obj, "#{@name}");
  setup_obj_common(obj);

  fields = rb_ary_new2(#{@fields.size});
  rb_define_const(obj, "FIELDS", fields);
EOF

    @fields.each_with_index { |field, i|
      @cfile.puts(%Q!  field = rb_str_new2("#{field}");!)
      @cfile.puts("  OBJ_FREEZE(field);")
      @cfile.puts("  rb_ary_store(fields, #{i}, field);")
    }
    @cfile.puts("  OBJ_FREEZE(fields);")
    @methods.each { |field|
      @cfile.puts(%Q!  rb_define_method(obj, "#{field.name}", #{field.func}, #{field.argc});!)
    }
    @enum.each { |key, val|
      @cfile.puts(%Q!  module = rb_define_module_under(obj, "#{val.module}");!)
      val.enum.each_with_index { |enum, i|
        @cfile.puts(%Q!  rb_define_const(module, "#{enum}", INT2FIX(#{i}));!)
      }
    }
    @cfile.puts("}")
  end

  def create_rw_field_func(func, rw, type, field)
    if (rw == "put")
      parm = ", VALUE arg"
      arg = ", arg"
    else
      arg = ""
      parm = ""
    end
    @cfile.puts <<EOF
static VALUE
#{func}(VALUE self#{parm})
{
  return inst_#{rw}_#{type}(self#{arg}, "#{field}");
}
EOF
  end

  def create_put_str_func(func, field)
    create_rw_field_func(func, "put", "str", field)
  end

  def create_get_str_func(func, field)
    create_rw_field_func(func, "get", "str", field)
  end

  def create_put_int_func(func, field)
    create_rw_field_func(func, "put", "int", field)
  end

  def create_get_int_func(func, field)
    create_rw_field_func(func, "get", "int", field)
  end

  def create_put_double_func(func, field)
    create_rw_field_func(func, "put", "double", field)
  end

  def create_get_double_func(func, field)
    create_rw_field_func(func, "get", "double", field)
  end

  def create_put_bool_func(func, field)
    create_rw_field_func(func, "put", "bool", field)
  end

  def create_get_bool_func(func, field)
    create_rw_field_func(func, "get", "bool", field)
  end

  def create_put_enum_func(func, field, enum)
    @cfile.puts <<EOF
static VALUE
#{func}(VALUE self, VALUE arg)
{
  return inst_put_enum(self, arg, "#{field}", #{enum.max});
}
EOF
  end

  def add_enum(field, ary)
    return @enum[field] if (@enum[field])
    enum = Enum.new(field, ary)
    @enum[field] = enum
    enum
  end

  def create_get_enum_func(func, field)
    create_rw_field_func(func, "get", "int", field)
  end

  def create_put_int_array_func(func, field)
    create_rw_field_func(func, "put", "iarray", field)
  end

  def create_get_int_array_func(func, field)
    create_rw_field_func(func, "get", "iarray", field)
  end

  def create_put_double_array_func(func, field)
    create_rw_field_func(func, "put", "darray", field)
  end

  def create_get_double_array_func(func, field)
    create_rw_field_func(func, "get", "darray", field)
  end

  def create_put_str_array_func(func, field)
    create_rw_field_func(func, "put", "sarray", field)
  end

  def create_get_str_array_func(func, field)
    create_rw_field_func(func, "get", "sarray", field)
  end

  def field2method(field)
    method = field.gsub("-", "_").gsub("%", "prm")
    method = "value" if (method == "@")
    method
  end

  def add_put_method(ary)
    field = ary[1]
    type = ary[2]
    method = field2method(field)
    func = "#{@name}_field_put_#{method}"
    case type
    when "bool"
      create_put_bool_func(func, field)
    when "int"
      create_put_int_func(func, field)
    when "double"
      create_put_double_func(func, field)
    when "obj"
      create_put_str_func(func, field)
    when "char*"
      create_put_str_func(func, field)
    when "enum("
      n = ary.size - 4
      enum = add_enum(field, ary[3, n])
      create_put_enum_func(func, field, enum)
    when "int[]"
      create_put_int_array_func(func, field)
    when "double[]"
      create_put_double_array_func(func, field)
    when "char*[]"
      create_put_str_array_func(func, field)
    end
    field = Field.new("#{method}=", func, 1)
    @methods.push(field)
  end

  def add_get_method(ary)
    field = ary[1]
    type = ary[2]
    method = field2method(field)
    func = "#{@name}_field_get_#{method}"
    case type
    when "bool"
      create_get_bool_func(func, field)
    when "int"
      create_get_int_func(func, field)
    when "double"
      create_get_double_func(func, field)
    when "obj"
      create_get_str_func(func, field)
    when "char*"
      create_get_str_func(func, field)
    when "enum("
      n = ary.size - 4
      add_enum(field, ary[3, n])
      create_get_enum_func(func, field)
    when "int[]"
      create_get_int_array_func(func, field)
    when "double[]"
      create_get_double_array_func(func, field)
    when "char*[]"
      create_get_str_array_func(func, field)
    end
    field = Field.new("#{method}", func, 0)
    @methods.push(field)
  end

  def create_void_exe_func(func, field)
    @cfile.puts <<EOF
static VALUE
#{func}(VALUE self)
{
  return inst_exe_void_func(self, "#{field}");
}
EOF
  end

  def check_args(ary)
    n = ary.size - 4
    return [-2, []] if (n == 0)
    return [0, []] if (ary[3] == "void")
    return [-1, ary[3, n]]
  end

  def create_func_type(func, args)
    @cfile.print("static VALUE\n#{func}(int argc, VALUE *argv, VALUE self)")
    @cfile.puts("\n{")
    @cfile.puts("  VALUE arg[#{args.size}];")
    @cfile.puts("  VALUE tmpstr;") if (args.find {|s| s[-1] == "]"})
  end

  def create_arguments(args)
    array = false
    @cfile.print(%Q!  rb_scan_args(argc, argv, "0#{args.size}", !)
    args.size.times { |i|
      @cfile.print("arg + #{i}")
      if (i == args.size - 1)
        @cfile.puts(");")
      else
        @cfile.print(", ")
      end
    }
    @cfile.puts("  carg = alloca(sizeof(*carg) + sizeof(union array) * #{args.size});")
    @cfile.puts("  if (carg == NULL) {\n    return Qnil;\n  }")
    @cfile.puts("  carg->num = #{args.size};")
    args.each_with_index { |arg, i|
      case arg
      when "int"
        @cfile.puts("  carg->ary[#{i}].i = VAL2INT(arg[#{i}]);")
      when "double"
        @cfile.puts("  carg->ary[#{i}].d = VAL2DBL(arg[#{i}]);")
      when "char*"
        @cfile.puts("  carg->ary[#{i}].str = VAL2STR(arg[#{i}]);")
      when "bool"
        @cfile.puts("  carg->ary[#{i}].i = RTEST(arg[#{i}]) ? 1 : 0;")
      when "int[]"
        @cfile.puts("  carg->ary[#{i}].ary = allocate_iarray(&tmpstr, arg[#{i}]);")
        array = true
      when "double[]"
        @cfile.puts("  carg->ary[#{i}].ary = allocate_darray(&tmpstr, arg[#{i}]);")
        array = true
      when "char*[]"
        @cfile.puts("  carg->ary[#{i}].ary = allocate_sarray(&tmpstr, arg[#{i}]);")
        array = true
      end
    }
    array
  end

  def create_finalize_arguments(args)
    args.each_with_index { |arg, i|
      case arg
      when "int[]", "double[]", "char*[]"
        @cfile.puts("  if (carg->ary[#{i}].ary) {\n    rb_free_tmp_buffer(&tmpstr);\n  }")
      end
    }
  end

  def create_val_func_with_argv(func, field, comv)
    @cfile.puts("static VALUE\n#{func}(VALUE self, VALUE argv)\n{")
    @cfile.puts("  VALUE tmpstr;")
    @cfile.puts(FUNC_FIELD_VAL)
    @cfile.puts(FUNC_FIELD_COMMON)
    @cfile.puts <<EOF
  carg = allocate_sarray(&tmpstr, argv);
  r = ngraph_plugin_shell_getobj(inst->obj, "#{field}", inst->id, carg, &rval);
  rb_free_tmp_buffer(&tmpstr);
  if (r < 0) {
    return Qnil;
  }

  return #{comv};
}
EOF
  end

  def create_void_func_with_argv(func, field)
    @cfile.puts <<EOF
static VALUE
#{func}(VALUE self, VALUE argv)
{
  return exe_void_func_argv(self, argv, "#{field}");
}
EOF
  end

  def create_array_func_with_argv(func, field, comv)
    @cfile.puts("static VALUE\n#{func}(VALUE self, VALUE argv)\n{")
    @cfile.puts("  VALUE tmpstr;")
    @cfile.puts(FUNC_FIELD_ARY)
    @cfile.puts(FUNC_FIELD_COMMON)
    @cfile.puts <<EOF
  carg = allocate_sarray(&tmpstr, argv);
  r = ngraph_plugin_shell_getobj(inst->obj, "#{field}", inst->id, carg, &rval);
  rb_free_tmp_buffer(&tmpstr);
  if (r < 0) {
    return Qnil;
  }

  ary = rb_ary_new2(rval.ary.num);
  for (i = 0; i < rval.ary.num; i++) {
    rb_ary_store(ary, i, #{comv});
  }

  return ary;
}
EOF
  end

  def create_val_func_with_args(func, field, args, comv)
    create_func_type(func, args)
    @cfile.puts(FUNC_FIELD_VAL)
    @cfile.puts(FUNC_FIELD_COMMON)
    array = create_arguments(args)

    @cfile.puts(%Q!  r = ngraph_plugin_shell_getobj(inst->obj, "#{field}", inst->id, carg, &rval);!)
    @cfile.puts(%Q!  rb_free_tmp_buffer(&tmpstr);!) if (array)
    @cfile.puts <<EOF
  if (r < 0) {
    return Qnil;
  }

  return #{comv};
}
EOF
  end

  def create_void_func_with_args(func, field, args)
    create_func_type(func, args)
    @cfile.puts(FUNC_FIELD_COMMON)
    array = create_arguments(args)

    @cfile.puts(%Q!  r = ngraph_plugin_shell_exeobj(inst->obj, "#{field}", inst->id, carg);!)
    @cfile.puts(%Q!  rb_free_tmp_buffer(&tmpstr);!) if (array)
    @cfile.puts <<EOF
  if (r < 0) {
    return Qnil;
  }

  return self;
}
EOF
  end

  def create_array_func_with_args(func, field, args, comv)
    create_func_type(func, args)
    @cfile.puts(FUNC_FIELD_ARY)
    @cfile.puts(FUNC_FIELD_COMMON)
    array = create_arguments(args)

    @cfile.puts(%Q!  r = ngraph_plugin_shell_getobj(inst->obj, "#{field}", inst->id, carg, &rval);!)
    @cfile.puts(%Q!  rb_free_tmp_buffer(&tmpstr);!) if (array)
    @cfile.puts <<EOF
  if (r < 0) {
    return Qnil;
  }

  ary = rb_ary_new2(rval.ary.num);
  for (i = 0; i < rval.ary.num; i++) {
    rb_ary_store(ary, i, #{comv});
  }

  return ary;
}
EOF
  end

  def create_bool_func_with_args(func, field, args)
    create_val_func_with_args(func, field, args, 'rval.i ? Qtrue : Qfalse')
  end

  def create_int_func_with_args(func, field, args)
    create_val_func_with_args(func, field, args, 'INT2NUM(rval.i)')
  end

  def create_double_func_with_args(func, field, args)
    create_val_func_with_args(func, field, args, 'rb_float_new(rval.d)')
  end

  def create_str_func_with_args(func, field, args)
    create_val_func_with_args(func, field, args, 'rb_str_new2(rval.str ? rval.str : "")')
  end

  def create_bool_func_with_argv(func, field)
    create_val_func_with_argv(func, field, 'rval.i ? Qtrue : Qfalse')
  end

  def create_int_func_with_argv(func, field)
    create_val_func_with_argv(func, field, 'INT2NUM(rval.i)')
  end

  def create_double_func_with_argv(func, field)
    create_val_func_with_argv(func, field, 'rb_float_new(rval.d)')
  end

  def create_str_func_with_argv(func, field)
    @cfile.puts <<EOF
static VALUE
#{func}(VALUE self, VALUE argv)
{
  return get_str_func_argv(self, argv, "#{field}");
}
EOF
  end

  def add_bool_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_bool_func(func, field)
    when -2
      create_bool_func_with_argv(func, field)
    else
      create_bool_func_with_args(func, field, args)
    end
  end

  def add_int_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_int_func(func, field)
    when -2
      create_int_func_with_argv(func, field)
    else
      create_int_func_with_args(func, field, args)
    end
  end

  def add_double_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_double_func(func, field)
    when -2
      create_double_func_with_argv(func, field)
    else
      create_double_func_with_args(func, field, args)
    end
  end

  def add_str_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_str_func(func, field)
    when -2
      create_str_func_with_argv(func, field)
    else
      create_str_func_with_args(func, field, args)
    end
  end

  def add_int_array_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_int_array_func(func, field)
    when -2
      create_array_func_with_argv(func, field, 'INT2NUM(rval.ary.data.ia[i])')
    else
      create_array_func_with_args(func, field, args, 'INT2NUM(rval.ary.data.ia[i])')
    end
  end

  def add_double_array_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_double_array_func(func, field)
    when -2
      create_array_func_with_argv(func, field, 'rb_float_new(rval.ary.data.da[i])')
    else
      create_array_func_with_args(func, field, args, 'rb_float_new(rval.ary.data.da[i])')
    end
  end

  def add_str_array_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_str_array_func(func, field)
    when -2
      create_array_func_with_argv(func, field, 'rb_str_new2(rval.ary.data.sa[i] ? rval.ary.data.sa[i] : "")')
    else
      create_array_func_with_args(func, field, args, 'rb_str_new2(rval.ary.data.sa[i] ? rval.ary.data.sa[i] : "")')
    end
  end

  def add_void_func(func, field, argc, args)
    case (argc)
    when 0
      create_void_exe_func(func, field)
    when -2
      create_void_func_with_argv(func, field)
    else
      create_void_func_with_args(func, field, args)
    end
  end

  def add_func_method(ary)
    field = ary[1]
    type = ary[2]
    method = field2method(field)
    func = "#{@name}_field_#{method}"
    argc, args = check_args(ary)
    case type
    when "bool("
      add_bool_func(func, field, argc, args)
    when "int("
      add_int_func(func, field, argc, args)
    when "double("
      add_double_func(func, field, argc, args)
    when "char*("
      add_str_func(func, field, argc, args)
    when "int[]("
      add_int_array_func(func, field, argc, args)
    when "double[]("
      add_double_array_func(func, field, argc, args)
    when "char*[]("
      add_str_array_func(func, field, argc, args)
    when "void("
      add_void_func(func, field, argc, args)
    end
    field = Field.new("#{method}", func, argc)
    @methods.push(field)
  end

  def add_method(ary)
    case ary[0]
    when "-w-"
      add_put_method(ary)
    when "r--"
      add_get_method(ary)
    when "r-x"
      add_func_method(ary)
    when "rw-"
      add_put_method(ary)
      add_get_method(ary)
    end
  end
end

def create_obj_funcs(file, cfile, name, version, parent)
  obj = NgraphObj.new(name, cfile, version, parent)
  while (true)
    ary = file.gets.chomp.split
    case ary[0]
    when "#"
      break
    when "---"
      # can't access
    when "--x"
      # can't execute
    when "r--"
      obj.add_method(ary)
    when "-w-"
      obj.add_method(ary)
    when "r-x"
      obj.add_method(ary)
    when "rw-"
      obj.add_method(ary)
    else
      next
    end
    obj.fields.push(ary[1])
  end
  obj.create_obj
  obj
end

objs = []
File.open(ARGV[1], "w") { |cfile|
  File.open(ARGV[0], "r") { |file|
    while (l = file.gets)
      str_ary = l.chomp.split
      case (str_ary[0])
      when "object:"
        name = str_ary[1]
        next
      when "version:"
        version = str_ary[1]
        next
      when "parent:"
        parent = str_ary[1]
      else
        next
      end
      obj = create_obj_funcs(file, cfile, name, version, parent)
      objs.push(obj)
    end
  }
  cfile.puts <<EOF
static void
create_ngraph_classes(VALUE ngraph_module)
{
  VALUE objs;

  objs = rb_ary_new2(#{objs.size});
  rb_define_const(ngraph_module, "OBJECTS", objs);
EOF
  objs.each_with_index {|obj, i|
    cfile.puts("  create_#{obj.name}_object(ngraph_module);")
    cfile.puts(%Q!  rb_ary_store(objs, #{i}, ID2SYM(rb_to_id(rb_str_new2("#{obj.name.capitalize}"))));!)
  }
  cfile.puts("  OBJ_FREEZE(objs);")
  cfile.puts("}")
}

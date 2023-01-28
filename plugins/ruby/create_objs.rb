#! /usr/bin/ruby

ADD_QUESTION_MARK_TO_BOOL_FIELD = false

class Field
  attr_reader :name, :func, :argc

  def initialize(name, func, argc)
    @name = name
    @func = func
    @argc = argc
  end
end

class EnumItem
  attr_reader :const, :name, :num
  def initialize(enum, i)
    @const = enum.upcase.gsub(".", "_")
    @num = i
    @name = enum
  end
end

class Enum
  attr_reader :module, :enum, :max

  def initialize(name, enum)
    @module = name.capitalize
    i = 0;
    @enum = []
    enum.each_with_index {|e, i|
      @enum.push(EnumItem.new(e, i))
    }
    @max = enum.size - 1
  end
end

class NgraphObj
  attr_reader :name, :fields
  attr_accessor :abstruct, :singleton
  SINGLETON_METHOD = [
                      ["new", "new", 0],
                      ["[]", "get", 1],
                      ["del", "del", 1],
                      ["each", "each", 0],
                      ["size", "size", 0],
                      ["current", "current", 0],
                     ]

  SINGLETON_METHOD2 = [
                       ["move_up", "move_up", 1],
                       ["move_down", "move_down", 1],
                       ["move_top", "move_top", 1],
                       ["move_last", "move_last", 1],
                       ["exchange", "exchange", 2],
                       ["copy", "copy", 2],
                      ]

  SINGLETON_METHOD3 = [
                       ["exist?", "exist", 0],
                       ["get_field_args", "field_args", 1],
                       ["get_field_type", "field_type", 1],
                       ["get_field_permission", "field_permission", 1],
                       ["derive", "derive", 1],
                      ]

  FUNC_FIELD_COMMON = <<-EOF
	  struct ngraph_instance *inst;

	  inst = check_id(self);
	  if (inst == NULL) {
	    return Qnil;
	  }
	EOF

  FUNC_FIELD_VAL = "  ngraph_returned_value rval;\n"

  FUNC_FIELD_ARY = <<-EOF
	  ngraph_returned_value rval;
	  VALUE ary;
	  int i;
	EOF

  def initialize(name, aliases, cfile, version, parent)
    @name = name
    @fields = []
    @methods = []
    @enum = {}
    @cfile = cfile
    @version = version
    @parent = (parent == "(null)") ? nil : parent
    @abstruct = false
    @singleton = false
    @func = ""
    @alias = if (aliases)
               aliases.split(":").map {|s| s.capitalize}
             else
               []
             end
  end

  def put_indented_str(s)
    @cfile.puts(s.gsub(/^	/, ""))
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
    unless (@abstruct)
      put_indented_str(@func)
      SINGLETON_METHOD.each { |method, func, argc|
        add_singleton_method_func(method, func, argc)
      }
      unless (@singleton)
        SINGLETON_METHOD2.each { |method, func, argc|
          add_singleton_method_func(method, func, argc)
        }
      end
    end
    SINGLETON_METHOD3.each { |method, func, argc|
      add_singleton_method_func(method, func, argc)
    }

    @enum.each { |key, val|
      put_indented_str <<-EOF
	static VALUE
	#{@name}_get_#{val.module}(VALUE klass, VALUE arg)
	{
	  int i;
	  i = VAL2INT(arg);
	  switch (i) {
	EOF
      val.enum.each { |enum|
        @cfile.puts("    case #{enum.num}:")
        @cfile.puts(%Q!      return rb_str_new2("#{enum.name}");!)
      }
      put_indented_str <<-EOF
	  }
	  return Qnil;
	}
	EOF
    }
    put_indented_str <<-EOF
	static void
	create_#{@name}_object(VALUE ngraph_module, VALUE ngraph_class)
	{
	  VALUE obj;
	  VALUE fields;
	#{(@enum.size > 0) ? "  VALUE module;" : ""}
	  obj = rb_define_class_under(ngraph_module, "#{@name.capitalize}", ngraph_class);
	EOF
    @alias.each {|name|
      @cfile.puts(%Q!  rb_define_const(ngraph_module, "#{name}", obj);!)
    }
    unless (@abstruct)
      SINGLETON_METHOD.each { |method, func, argc|
        @cfile.puts(%Q!  rb_define_singleton_method(obj, "#{method}", #{@name}_#{func}, #{argc});!)
      }
      unless (@singleton)
        SINGLETON_METHOD2.each { |method, func, argc|
          @cfile.puts(%Q!  rb_define_singleton_method(obj, "#{method}", #{@name}_#{func}, #{argc});!)
        }
      end
    end
    SINGLETON_METHOD3.each { |method, func, argc|
      @cfile.puts(%Q!  rb_define_singleton_method(obj, "#{method}", #{@name}_#{func}, #{argc});!)
    }
    @cfile.puts("  setup_obj_common(obj);") unless (@abstruct)
    put_indented_str <<-EOF
	  add_obj_const(obj, "#{@name}");

	  fields = rb_ary_new2(#{@fields.size});
	  rb_define_const(obj, "FIELDS", fields);
	EOF

    @fields.each_with_index { |field, i|
      @cfile.puts(%Q!  store_field_names(fields, "#{field}");!)
    }
    @cfile.puts("  OBJ_FREEZE(fields);")
    unless (@abstruct)
      @methods.each { |field|
        @cfile.puts(%Q!  rb_define_method(obj, "#{field.name}", #{field.func}, #{field.argc});!)
      }
    end
    @enum.each { |key, val|
      @cfile.puts(%Q!  module = rb_define_module_under(obj, "#{val.module}");!)
      @cfile.puts(%Q!  rb_extend_object(module, rb_mEnumerable);!)
      @cfile.puts(%Q!  rb_define_singleton_method(module, "[]", #{@name}_get_#{val.module}, 1);!)
      val.enum.each { |enum|
        @cfile.puts(%Q!  rb_define_const(module, "#{enum.const}", INT2FIX(#{enum.num}));!)
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
    @func += <<-EOF
	static VALUE
	#{func}(VALUE self#{parm})
	{
	  return inst_#{rw}_#{type}(self#{arg}, "#{field}");
	}
	EOF
  end

  def create_put_obj_func(func, field)
    create_rw_field_func(func, "put", "obj", field)
  end

  def create_get_obj_func(func, field)
    create_rw_field_func(func, "get", "obj", field)
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
    @func += <<-EOF
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
    method.downcase
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
      create_put_obj_func(func, field)
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
      method += "?" if (ADD_QUESTION_MARK_TO_BOOL_FIELD)
    when "int"
      create_get_int_func(func, field)
    when "double"
      create_get_double_func(func, field)
    when "obj"
      create_get_obj_func(func, field)
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
    @func += <<-EOF
	static VALUE
	#{func}(VALUE self)
	{
	  return inst_exe_void_func(self, "#{field}");
	}
	EOF
  end

  def check_args(ary)
    n = ary.size - 4
    case (n)
    when 0
      [-2, []]
    when 1
      if (ary[3][-1] == "]")
        [-2, ary[3, n]]
      elsif (ary[3] == "void")
        [0, []]
      else
        [-1, ary[3, n]]
      end
    else
      [-1, ary[3, n]]
    end
  end

  def create_func_type(func, args)
    @func += "static VALUE\n#{func}(int argc, VALUE *argv, VALUE self)\n{\n"
    @func += "  VALUE arg[#{args.size}];\n"
    @func += "  VALUE tmpstr;\n" if (args.find {|s| s[-1] == "]"})
    @func += "  ngraph_arg *carg;\n"
  end

  def create_arguments(field, args)
    array = false
    @func += (%Q!  rb_scan_args(argc, argv, "0#{args.size}", !)
    args.size.times { |i|
      @func += ("arg + #{i}")
      if (i == args.size - 1)
        @func += (");\n")
      else
        @func += (", ")
      end
    }
    @func += ("  carg = alloca(sizeof(*carg) + sizeof(ngraph_value) * #{args.size});\n")
    @func += (%Q!  if (carg == NULL) {\n    rb_raise(rb_eSysStackError, "%s: cannot allocate enough memory.", rb_obj_classname(self));\n  }\n!)
    @func += ("  carg->num = #{args.size};\n")
    args.each_with_index { |arg, i|
      case arg
      when "int"
        @func += ("  carg->ary[#{i}].i = VAL2INT(arg[#{i}]);\n")
      when "double"
        @func += ("  carg->ary[#{i}].d = VAL2DBL(arg[#{i}]);\n")
      when "char*"
        @func += ("  carg->ary[#{i}].str = VAL2STR(arg[#{i}]);\n")
      when "bool"
        @func += ("  carg->ary[#{i}].i = RTEST(arg[#{i}]) ? 1 : 0;\n")
      when "int[]"
        @func += (%Q!  carg->ary[#{i}].ary = allocate_iarray(self, &tmpstr, arg[#{i}], "#{field}");\n!)
        array = true
      when "double[]"
        @func += (%Q!  carg->ary[#{i}].ary = allocate_darray(self, &tmpstr, arg[#{i}], "#{field}");\n!)
        array = true
      when "char*[]"
        @func += (%Q!  carg->ary[#{i}].ary = allocate_sarray(self, &tmpstr, arg[#{i}], "#{field}");\n!)
        array = true
      end
    }
    array
  end

  def create_finalize_arguments(args)
    args.each_with_index { |arg, i|
      case arg
      when "int[]", "double[]", "char*[]"
        @func += ("  if (carg->ary[#{i}].ary) {\n    rb_free_tmp_buffer(&tmpstr);\n  }\n")
      end
    }
  end

  def add_arg_array(arg, field)
    unless (arg)
      @func += %Q!  carg = allocate_sarray(self, &tmpstr, argv, "#{field}");\n!
      return
    end

    case (arg)
    when "int[]"
      @func += %Q!  carg.ary[0].ary = allocate_iarray(self, &tmpstr, argv, "#{field}");\n!
    when "double[]"
      @func += %Q!  carg.ary[0].ary = allocate_darray(self, &tmpstr, argv, "#{field}");\n!
    when "char*[]"
      @func += %Q!  carg.ary[0].ary = allocate_sarray(self, &tmpstr, argv, "#{field}");\n!
    else
      @func += %Q!  carg.ary[0].ary = allocate_sarray(self, &tmpstr, argv, "#{field}");\n!
    end
    @func += "  carg.num = 1;\n"
  end

  def create_val_func_with_argv(func, field, args, comv)
    varg = (args.size == 0)
    @func += ("static VALUE\n#{func}(VALUE self, VALUE argv)\n{\n")
    @func += ("  VALUE tmpstr;\n")
    @func += (FUNC_FIELD_VAL)
    @func += "  ngraph_arg #{(varg) ? '*' : ''}carg;\n"
    @func += (FUNC_FIELD_COMMON)
    add_arg_array(args[0], field)
    @func += <<-EOF
	  inst->rcode = ngraph_object_get(inst->obj, "#{field}", inst->id, #{varg ? '' : '&'}carg, &rval);
	  rb_free_tmp_buffer(&tmpstr);
	  if (inst->rcode < 0) {
	    return Qnil;
	  }

	  return #{comv};
	}
	EOF
  end

  def create_void_func_with_argv(func, field, args)
    varg = (args.size == 0)
    @func += <<-EOF
	static VALUE
	#{func}(VALUE self, VALUE argv)
	{
	  struct ngraph_instance *inst;
	  ngraph_arg #{(varg) ? '*' : ''}carg;
	  VALUE tmpstr;

	  inst = check_id(self);
	  if (inst == NULL) {
	    return Qnil;
	  }
	EOF
    add_arg_array(args[0], field)
    @func += <<-EOF
	  inst->rcode = ngraph_object_exe(inst->obj, "#{field}", inst->id, #{varg ? '' : '&'}carg);
	  rb_free_tmp_buffer(&tmpstr);
	  if (inst->rcode < 0) {
	    return Qnil;
	  }

	  return self;
	}
	EOF
  end

  def create_array_func_with_argv(func, field, args, comv)
    varg = (args.size == 0)
    @func += ("static VALUE\n#{func}(VALUE self, VALUE argv)\n{")
    @func += ("  VALUE tmpstr;\n")
    @func += (FUNC_FIELD_ARY)
    @func += "  ngraph_arg #{(varg) ? '*' : ''}carg;\n"
    @func += (FUNC_FIELD_COMMON)
    add_arg_array(args[0], field)
    @func += <<-EOF
	  inst->rcode = ngraph_object_get(inst->obj, "#{field}", inst->id, #{varg ? '' : '&'}carg, &rval);
	  rb_free_tmp_buffer(&tmpstr);
	  if (inst->rcode < 0) {
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

  def create_val_func_with_args(func, field, args, conv)
    create_func_type(func, args)
    @func += (FUNC_FIELD_VAL)
    @func += (FUNC_FIELD_COMMON)
    array = create_arguments(field, args)

    @func += (%Q!  inst->rcode = ngraph_object_get(inst->obj, "#{field}", inst->id, carg, &rval);\n!)
    @func += (%Q!  rb_free_tmp_buffer(&tmpstr);\n!) if (array)
    @func += <<-EOF
	  if (inst->rcode < 0) {
	    return Qnil;
	  }

	  return #{conv};
	}
	EOF
  end

  def create_void_func_with_args(func, field, args)
    create_func_type(func, args)
    @func += (FUNC_FIELD_COMMON)
    array = create_arguments(field, args)

    @func += (%Q!  inst->rcode = ngraph_object_exe(inst->obj, "#{field}", inst->id, carg);\n!)
    @func += (%Q!  rb_free_tmp_buffer(&tmpstr);\n!) if (array)
    @func += <<-EOF
	  if (inst->rcode < 0) {
	    return Qnil;
	  }

	  return self;
	}
	EOF
  end

  def create_array_func_with_args(func, field, args, comv)
    create_func_type(func, args)
    @func += (FUNC_FIELD_ARY)
    @func += (FUNC_FIELD_COMMON)
    array = create_arguments(field, args)

    @func += (%Q!  inst->rcode = ngraph_object_get(inst->obj, "#{field}", inst->id, carg, &rval);\n!)
    @func += (%Q!  rb_free_tmp_buffer(&tmpstr);\n!) if (array)
    @func += <<-EOF
	  if (inst->rcode < 0) {
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
    create_val_func_with_args(func, field, args, 'utf8_str_new(rval.str ? rval.str : "")')
  end

  def create_bool_func_with_argv(func, field, args)
    create_val_func_with_argv(func, field, args, 'rval.i ? Qtrue : Qfalse')
  end

  def create_int_func_with_argv(func, field, args)
    create_val_func_with_argv(func, field, args, 'INT2NUM(rval.i)')
  end

  def create_double_func_with_argv(func, field, args)
    create_val_func_with_argv(func, field, args, 'rb_float_new(rval.d)')
  end

  def create_str_func_with_argv(func, field, args)
    create_val_func_with_argv(func, field, args, 'utf8_str_new(rval.str ? rval.str : "")')
  end

  def add_bool_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_bool_func(func, field)
    when -2
      create_bool_func_with_argv(func, field, args)
    else
      create_bool_func_with_args(func, field, args)
    end
  end

  def add_int_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_int_func(func, field)
    when -2
      create_int_func_with_argv(func, field, args)
    else
      create_int_func_with_args(func, field, args)
    end
  end

  def add_double_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_double_func(func, field)
    when -2
      create_double_func_with_argv(func, field, args)
    else
      create_double_func_with_args(func, field, args)
    end
  end

  def add_str_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_str_func(func, field)
    when -2
      create_str_func_with_argv(func, field, args)
    else
      create_str_func_with_args(func, field, args)
    end
  end

  def add_int_array_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_int_array_func(func, field)
    when -2
      create_array_func_with_argv(func, field, args, 'INT2NUM(rval.ary.data.ia[i])')
    else
      create_array_func_with_args(func, field, args, 'INT2NUM(rval.ary.data.ia[i])')
    end
  end

  def add_double_array_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_double_array_func(func, field)
    when -2
      create_array_func_with_argv(func, field, args, 'rb_float_new(rval.ary.data.da[i])')
    else
      create_array_func_with_args(func, field, args, 'rb_float_new(rval.ary.data.da[i])')
    end
  end

  def add_str_array_func(func, field, argc, args)
    case (argc)
    when 0
      create_get_str_array_func(func, field)
    when -2
      create_array_func_with_argv(func, field, args, 'utf8_str_new(rval.ary.data.sa[i] ? rval.ary.data.sa[i] : "")')
    else
      create_array_func_with_args(func, field, args, 'utf8_str_new(rval.ary.data.sa[i] ? rval.ary.data.sa[i] : "")')
    end
  end

  def add_void_func(func, field, argc, args)
    case (argc)
    when 0
      create_void_exe_func(func, field)
    when -2
      create_void_func_with_argv(func, field, args)
    else
      create_void_func_with_args(func, field, args)
    end
  end

  def add_func_obj(type, func, field, args)
    rtype = case type
            when "bool("
              "NBFUNC"
            when "int("
              "NIFUNC"
            when "char*("
              "NSFUNC"
            when "int[]("
              "NIAFUNC"
            when "double[]("
              "NDAFUNC"
            when "char*[]("
              "NSAFUNC"
            when "void("
              "NVFUNC"
            else
              "NVFUNC"
            end
    @func += <<-EOF
	static VALUE
	#{func}(VALUE self, VALUE argv)
	{
	  return obj_func_obj(self, argv, "#{field}", #{rtype});
	}
	EOF
  end

  def add_func_method(ary)
    field = ary[1]
    type = ary[2]
    method = field2method(field)
    func = "#{@name}_field_#{method}"
    argc, args = check_args(ary)
    if (ary[3] == "obj")
      argc = -2;
      add_func_obj(type, func, field, args)
    else
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
    end
    if (ADD_QUESTION_MARK_TO_BOOL_FIELD)
      method += "?" if (type == "bool(")
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

def create_obj_funcs(file, cfile, name, aliases, version, parent)
  obj = NgraphObj.new(name, aliases, cfile, version, parent)
  singleton = true
  while (true)
    ary = file.gets.chomp.split
    singleton = false if (ary[1] == "next")
    case ary[0]
    when "#"
      break
    when "---"
      obj.abstruct = true if (ary[1] == "init")
    when "--x"
      # can't execute
    when "r--"
      obj.add_method(ary)
    when "-w-"
      #      obj.add_method(ary)
      # only used for backword compatibility.
    when "r-x"
      obj.add_method(ary)
    when "rw-"
      obj.add_method(ary)
    else
      next
    end
    obj.fields.push(ary[1])
  end
  obj.singleton = singleton
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
      when "alias:"
        aliases = str_ary[1]
        next
      when "version:"
        version = str_ary[1]
        next
      when "parent:"
        parent = str_ary[1]
      else
        next
      end
      obj = create_obj_funcs(file, cfile, name, aliases, version, parent)
      objs.push(obj)
    end
  }
  str = <<-EOF
	static void
	create_ngraph_classes(VALUE ngraph_module, VALUE ngraph_class)
	{
	  VALUE objs;

	  objs = rb_ary_new2(#{objs.size});
	  rb_define_const(ngraph_module, "OBJECTS", objs);
	EOF
  cfile.puts(str.gsub(/^	/, ""))
  objs.each_with_index {|obj, i|
    cfile.puts("  create_#{obj.name}_object(ngraph_module, ngraph_class);\n")
    cfile.puts(%Q!  rb_ary_store(objs, #{i}, ID2SYM(rb_intern("#{obj.name.capitalize}")));\n!)
  }
  cfile.puts("  OBJ_FREEZE(objs);\n")
  cfile.puts("}")
}

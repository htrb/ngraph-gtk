# -*- coding: utf-8 -*-

class CompletionInfo
  DataTypeFunc  = 0
  DataTypeConst = 1

  def initialize(func_file, const_file)
    @func_data = []
    @const_data = []
    load_data(DataTypeFunc, func_file, @func_data)
    load_data(DataTypeConst, const_file, @const_data)
  end

  def func_struct(info)
    text = info[0].sub(/\(.+\)/, '()')
    ltext = text.downcase
    info_text = info[1].gsub('"', '\\"')
    %Q!{"#{ltext}", "#{text}", N_("#{info[0]}\\n#{info_text}"), NULL},!
  end

  def const_struct(info)
    info_text = info[1].gsub('"', '\\"')
    %Q!{"#{info[0].downcase}", "#{info[0]}", N_("#{info_text}"), NULL},!
  end

  def load_data(type, file, ary)
    File.open(file, "r:utf-8") { |info_file|
      info_file.each { |l|
        info = l.chomp.split(/\t+/)
        data = if (type == DataTypeFunc)
                 func_struct(info)
               else
                 const_struct(info)
               end
        ary.push(data)
      }
    }
    ary.sort!
  end

  def save_sruct(type, data, c_file)
    c_file.puts("struct completion_info completion_info_#{type}[] = {")
    data.each {|info|
      c_file.puts(info)
    }
    c_file.puts("{NULL, NULL, NULL, NULL}")
    c_file.puts("};")
  end

  def save_data(filename)
    File.open(filename, "w:utf-8") { |c_file|
      c_file.puts('/* -*- Mode: C; coding: utf-8 -*- */')
      c_file.puts('#include "gtk_common.h"')
      c_file.puts('#include "completion_info.h"')
      save_sruct("func", @func_data, c_file)
      save_sruct("const", @const_data, c_file)
    }
  end
end

completion_info = CompletionInfo.new(ARGV[0], ARGV[1])
completion_info.save_data(ARGV[2])

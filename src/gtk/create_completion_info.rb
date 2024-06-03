# -*- coding: utf-8 -*-
# frozen_string_literal: true

# the class to create completion information
class CompletionInfo
  DATA_TYPE_FUNC  = 0
  DATA_TYPE_CONST = 1

  def initialize(func_file, const_file)
    @func_data = []
    @const_data = []
    load_data(DATA_TYPE_FUNC, func_file, @func_data)
    load_data(DATA_TYPE_CONST, const_file, @const_data)
  end

  def func_struct(info)
    function = info[0].sub(/\(.+\)/, '()')
    args = Regexp.last_match(0) ? Regexp.last_match(0).gsub(/[()]/, '') : ''
    func_name = function.split('(')[0]
    lfunc = func_name.downcase
    title = "<b>#{func_name}</b>(<i>#{args}</i>)"
    info_text = info[1].gsub('"', '\\"')
    %!{"#{lfunc}", "#{function}", "#{func_name}", N_("#{title}\\n#{info_text}"), NULL},!
  end

  def const_struct(info)
    info_text = info[1].gsub('"', '\\"')
    %!{"#{info[0].downcase}", "#{info[0]}", NULL, N_("<b>#{info[0]}</b>\\n#{info_text}"), NULL},!
  end

  def load_data(type, file, ary)
    File.open(file, 'r:utf-8') do |info_file|
      info_file.each do |l|
        info = l.chomp.split(/\t+/)
        data = type == DATA_TYPE_FUNC ? func_struct(info) : const_struct(info)
        ary.push(data)
      end
    end
    ary.sort!
  end

  def save_sruct(type, data, c_file)
    c_file.puts("struct completion_info completion_info_#{type}[] = {")
    data.each { |info| c_file.puts(info) }
    c_file.puts('{NULL, NULL, NULL, NULL, NULL}')
    c_file.puts('};')
  end

  def save_data(filename)
    File.open(filename, 'w:utf-8') do |c_file|
      c_file.puts('/* -*- Mode: C; coding: utf-8 -*- */')
      c_file.puts('#include "gtk_common.h"')
      c_file.puts('#include "completion_info.h"')
      save_sruct('func', @func_data, c_file)
      save_sruct('const', @const_data, c_file)
    end
  end
end

completion_info = CompletionInfo.new(ARGV[0], ARGV[1])
completion_info.save_data(ARGV[2])

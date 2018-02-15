# -*- coding: utf-8 -*-

data = []
File.open(ARGV[0], "r:utf-8") { |info_file|
  info_file.each { |l|
    info = l.chomp.split(/\t+/)
    info_text = info[1].gsub('"', '\\"')
    data.push(%Q!{"#{info[0].downcase}", "#{info[0]}", "#{info_text}, NULL"},!)
  }
}

data.sort!
File.open(ARGV[1], "w:utf-8") { |c_file|
  c_file.puts('#include "completion_info.h"')
  c_file.puts("struct completion_info completion_info_const[] = {")
  data.each {|info|
    c_file.puts(info)
  }
  c_file.puts("};")
  c_file.puts("int completion_info_const_num(void) {return sizeof(completion_info_const) / sizeof(*completion_info_const);}")  
}


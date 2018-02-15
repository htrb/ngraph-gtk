# -*- coding: utf-8 -*-

data = []
File.open(ARGV[0], "r") { |info_file|
  info_file.each { |l|
    data.push(l.chomp.split(/\t+/))
  }
}

data.sort!
File.open(ARGV[1], "w") { |c_file|
  c_file.puts('#include "completion_info.h"')
  c_file.puts("struct completion_info completion_info_func[] = {")
  data.each {|info|
    text = info[0].sub(/\(.+\)/, '()')
    info_text = info[1].gsub('"', '\\"')
    c_file.puts(%Q!{"#{text}", "#{info[0]}\\n#{info_text}", NULL},!)
  }
  c_file.puts("};")
  c_file.puts("int completion_info_func_num(void) {return sizeof(completion_info_func) / sizeof(*completion_info_func);}")
}

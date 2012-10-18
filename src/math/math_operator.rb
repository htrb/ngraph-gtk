#! /usr/bin/ruby

N = 256

ope = []
ope_str = [];
IO.foreach(ARGV[0]) { |l|
  l.chomp!
  next if (l.length < 1)
  ope.push(l[0])
  ope_str.push(l.split)
}

ope_str = ope_str.uniq.sort {|a, b| b[0].length - a[0].length}

OPE_PREFIX = "MATH_OPERATOR_TYPE"

File.open("#{ARGV[1]}.h", "w") { |f|
  f.puts <<EOF
#ifndef MATH_SCANNER_OPE_HEADER
#define MATH_SCANNER_OPE_HEADER

enum MATH_OPERATOR_TYPE {
EOF
  ope_str.each {|s|
    f.puts("  #{OPE_PREFIX}_#{s[1]},")
  }
  f.puts("  #{OPE_PREFIX}_UNKNOWN\n};\n\n")

  f.puts <<EOF
int math_scanner_is_ope(int chr);
enum #{OPE_PREFIX} math_scanner_check_ope_str(const char *str, int *len);

#endif
EOF
}

File.open("#{ARGV[1]}.c", "w") { |f|
  f.puts <<EOF
#include "config.h"

#include <string.h>
#include "math_operator.h"

struct ope_str {
  char *ope;
  int len;
  enum #{OPE_PREFIX} type;
};

static struct ope_str OpeStr[] = {
EOF
  ope_str.each {|s|
    f.puts("  {\"#{s[0].gsub('\\', '\\\\\\')}\", #{s[0].length}, #{OPE_PREFIX}_#{s[1]}},")
  }
  f.puts("};\n\n")

  f.puts("static char OpeChar[#{N}] = {")
  N.times {|i|
    c = sprintf("%c", i)
    f.puts(ope.include?(c) ? "  1,  /* #{i.chr} */" : "  0,")
  }
  f.puts("};")

  f.puts <<EOF

int
math_scanner_is_ope(int chr)
{
  if (chr < 0 || chr > (int) (sizeof(OpeChar) / sizeof(*OpeChar)))
    return 0;

  return OpeChar[chr];
}

enum #{OPE_PREFIX}
math_scanner_check_ope_str(const char *str, int *len) {
  unsigned int i;

  for (i = 0; i < sizeof(OpeStr) / sizeof(*OpeStr); i++) {
    if (strncmp(str, OpeStr[i].ope, OpeStr[i].len) == 0) {
      *len = OpeStr[i].len;
      return OpeStr[i].type;
    }
  }

  return #{OPE_PREFIX}_UNKNOWN;
}
EOF
}

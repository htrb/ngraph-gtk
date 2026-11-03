#ifndef _SHELL_ARITHMETIC_HEADER
#define _SHELL_ARITHMETIC_HEADER
typedef struct {
  const char *src;
  struct nshell *nshell;
  int err, skip;
} ShellArithmeticContext;

long long shell_arithmetic_eval(ShellArithmeticContext *ctx);

#endif

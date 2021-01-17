#ifndef OGRA_HEADER
#define OGRA_HEADER

extern char *gra_decimalsign_char[];

enum GRA_DECIMALSIGN_TYPE {
  GRA_DECIMALSIGN_TYPE_PERIOD,
  GRA_DECIMALSIGN_TYPE_COMMA,
};

void gra_set_default_decimalsign(enum GRA_DECIMALSIGN_TYPE decimalsign);

#endif

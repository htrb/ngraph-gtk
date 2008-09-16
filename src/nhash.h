/* 
 * $Id: nhash.h,v 1.1 2008/09/16 08:52:40 hito Exp $
 */

#ifndef NHASH_HEADER
#define NHASH_HEADER

union nval {
  int i;
  void *p;
};

struct nhash {
  char *key;
  union nval val;
  struct nhash *next;
};

typedef struct nhash **NHASH;

NHASH nhash_new(void);
void nhash_free(NHASH hash);
int nhash_set_int(NHASH hash, char *key, int val);
int nhash_set_ptr(NHASH hash, char *key, void *val);
int nhash_get_int(NHASH hash, char *key, int *val);
int nhash_get_ptr(NHASH hash, char *key, void **val);

#endif

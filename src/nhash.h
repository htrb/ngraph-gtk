/* 
 * $Id: nhash.h,v 1.2 2008/09/17 01:54:58 hito Exp $
 */

#ifndef NHASH_HEADER
#define NHASH_HEADER

struct nhash {
  char *key;
  union {
    int i;
    void *p;
  } val;
  struct nhash *l, *r;
};

typedef struct nhash **NHASH;

NHASH nhash_new(void);
void nhash_free(NHASH hash);
int nhash_set_int(NHASH hash, char *key, int val);
int nhash_set_ptr(NHASH hash, char *key, void *val);
int nhash_get_int(NHASH hash, char *key, int *val);
int nhash_get_ptr(NHASH hash, char *key, void **val);

#endif

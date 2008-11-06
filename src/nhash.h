/* 
 * $Id: nhash.h,v 1.3 2008/11/06 05:47:26 hito Exp $
 */

#ifndef NHASH_HEADER
#define NHASH_HEADER

struct nhash {
  char *key;
  union {
    int i;
    void *p;
  } val;
  struct nhash *l, *r, *p;
};

typedef struct nhash **NHASH;

NHASH nhash_new(void);
void nhash_free(NHASH hash);
void nhash_free_with_memfree_ptr(NHASH hash);
int nhash_set_int(NHASH hash, char *key, int val);
int nhash_set_ptr(NHASH hash, char *key, void *val);
int nhash_get_int(NHASH hash, char *key, int *val);
int nhash_get_ptr(NHASH hash, char *key, void **val);
int nhash_each(NHASH hash, int(* func)(struct nhash *, void *), void *data);
void nhash_del(NHASH hash, char *key);

int nhash_set_int_with_hkey(NHASH hash, char *key, int hkey, int val);
int nhash_set_ptr_with_hkey(NHASH hash, char *key, int hkey, void *val);
int nhash_get_int_with_hkey(NHASH hash, char *key, int hkey, int *val);
int nhash_get_ptr_with_hkey(NHASH hash, char *key, int hkey, void **val);
void nhash_del_with_hkey(NHASH hash, char *key, int hkey);

int  nhash_hkey(char *ptr);

#endif

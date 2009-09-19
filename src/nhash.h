/* 
 * $Id: nhash.h,v 1.6 2009/09/19 13:21:44 hito Exp $
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
void nhash_clear(NHASH hash);
int nhash_set_int(NHASH hash, const char *key, int val);
int nhash_set_ptr(NHASH hash, const char *key, void *val);
int nhash_get_int(NHASH hash, const char *key, int *val);
int nhash_get_ptr(NHASH hash, const char *key, void **val);
int nhash_each(NHASH hash, int(* func)(struct nhash *, void *), void *data);
void nhash_del(NHASH hash, const char *key);

int nhash_set_int_with_hkey(NHASH hash, const char *key, int hkey, int val);
int nhash_set_ptr_with_hkey(NHASH hash, const char *key, int hkey, void *val);
int nhash_get_int_with_hkey(NHASH hash, const char *key, int hkey, int *val);
int nhash_get_ptr_with_hkey(NHASH hash, const char *key, int hkey, void **val);
void nhash_del_with_hkey(NHASH hash, const char *key, int hkey);
int nhash_num(NHASH hash);

int  nhash_hkey(const char *ptr);

#endif

/* 
 * $Id: nhash.c,v 1.8 2008/11/16 08:46:18 hito Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "object.h"
#include "nhash.h"

#define HASH_SIZE 67

NHASH
nhash_new(void)
{
  struct nhash **hash;

  hash = calloc(HASH_SIZE, sizeof(struct nhash *));

  return hash;
}

static void
free_hash_list(struct nhash *hash, int free_ptr)
{
  struct nhash *l, *r;

  if (hash == NULL)
    return;

  l = hash->l;
  r = hash->r;
  free(hash->key);
  if (free_ptr) {
    memfree(hash->val.p);
  }
  free(hash);
  free_hash_list(l, free_ptr);
  free_hash_list(r, free_ptr);
}

void
nhash_free(NHASH hash)
{
  int i;

  for (i = 0; i < HASH_SIZE; i++) {
    free_hash_list(hash[i], 0);
  }
}

void
nhash_free_with_memfree_ptr(NHASH hash)
{
  int i;

  for (i = 0; i < HASH_SIZE; i++) {
    free_hash_list(hash[i], 1);
  }
}

int 
nhash_hkey(char *ptr)
{
  unsigned int i, v;

  for (v = i = 0; ptr[i]; i++) {
    v += ptr[i];
  }

  return v % HASH_SIZE;
}

static struct nhash *
create_hash_with_hkey(NHASH hash, char *key, int hkey)
{
  int lr = 0;
  char *k;
  struct nhash *h, *ptr, *prev = NULL;

  ptr = hash[hkey];

  if (ptr == NULL) {
    h = malloc(sizeof(struct nhash));
    if (h == NULL)
      return NULL;
    hash[hkey] = h;
  } else {
    while (ptr) {
      lr = strcmp(key, ptr->key);
      if (lr == 0) {
	return ptr;
      } else if (lr < 0) {
	prev = ptr;
	ptr = ptr->l;
      } else {
	prev = ptr;
	ptr = ptr->r;
      }
    }
    h = malloc(sizeof(struct nhash));
    if (h == NULL) {
      return NULL;
    }
  }

  k = strdup(key);
  if (k == NULL) {
    free(h);
    return NULL;
  }

  if (prev) {
    if (lr < 0) {
      prev->l = h;
    } else {
      prev->r = h;
    }
  }

  h->key = k;
  h->l = NULL;
  h->r = NULL;
  h->p = prev;

  return h;
}

struct nhash *
create_hash(NHASH hash, char *key)
{
  int hkey;
  hkey = nhash_hkey(key);
  return create_hash_with_hkey(hash, key, hkey);
}

int
nhash_set_int(NHASH hash, char *key, int val)
{
  struct nhash *h;

  h = create_hash(hash, key);

  if (h == NULL)
    return 1;

  h->val.i = val;

  return 0;
}

int
nhash_set_ptr(NHASH hash, char *key, void *val)
{
  struct nhash *h;

  h = create_hash(hash, key);

  if (h == NULL)
    return 1;

  h->val.p = val;

  return 0;
}

int
nhash_set_int_with_hkey(NHASH hash, char *key, int hkey, int val)
{
  struct nhash *h;

  h = create_hash_with_hkey(hash, key, hkey);

  if (h == NULL)
    return 1;

  h->val.i = val;

  return 0;
}

int
nhash_set_ptr_with_hkey(NHASH hash, char *key, int hkey, void *val)
{
  struct nhash *h;

  h = create_hash_with_hkey(hash, key, hkey);

  if (h == NULL)
    return 1;

  h->val.p = val;

  return 0;
}

static struct nhash *
nhash_get(NHASH hash, char *key)
{
  struct nhash *ptr;
  int hk, r;

  if (key == NULL)
    return NULL;

  hk = nhash_hkey(key);

  ptr = hash[hk];

  while (ptr) {
    r = strcmp(key, ptr->key);

    if (r < 0) {
      ptr = ptr->l;
    } else if (r > 0) {
      ptr = ptr->r;
    } else {
      return ptr;
    }
  }

  return NULL;
}

static struct nhash *
nhash_get_with_hkey(NHASH hash, char *key, int hk)
{
  struct nhash *ptr;
  int r;

  if (key == NULL)
    return NULL;

  ptr = hash[hk];

  while (ptr) {
    r = strcmp(key, ptr->key);

    if (r < 0) {
      ptr = ptr->l;
    } else if (r > 0) {
      ptr = ptr->r;
    } else {
      return ptr;
    }
  }

  return NULL;
}

int
nhash_get_int(NHASH hash, char *key, int *val)
{
  struct nhash *h;

  h = nhash_get(hash, key);

  if (h == NULL)
    return 1;

  *val = h->val.i;

  return 0;
}

int
nhash_get_int_with_hkey(NHASH hash, char *key, int hkey, int *val)
{
  struct nhash *h;

  h = nhash_get_with_hkey(hash, key, hkey);

  if (h == NULL)
    return 1;

  *val = h->val.i;

  return 0;
}

static void
btree_cat(struct nhash *dest, struct nhash *src)
{
  struct nhash *h;
  int r;

  h = dest;

  while (1) {
    r = strcmp(h->key, src->key);
    if (r < 0) {
      if (h->l == NULL) {
	h->l = src;
	break;
      }
      h = h->l;
    } else if (r > 0) {
      if (h->r == NULL) {
	h->r = src;
	break;
      }
      h = h->r;
    } else {
      /* never reached */
      break;
    }
  }
  src->p = h;
}

static void
nhash_del_sub(NHASH hash, struct nhash *h, int hkey)
{
  struct nhash *p;

  p = h->p;

  if (p == NULL) {
    if (h->l && h->r) {
      btree_cat(h->r, h->l);
      hash[hkey] = h->r;
      h->r->p = NULL;
    } else if (h->l) {
      hash[hkey] = h->l;
      h->l->p = NULL;
    } else if (h->r) {
      hash[hkey] = h->r;
      h->r->p = NULL;
    } else {
      hash[hkey] = NULL;
    }
  } else {
    if (h->l && h->r) {
      btree_cat(h->r, h->l);
      if (p->l == h) {
	p->l = h->r;
      } else {
	p->r = h->r;
      }
      h->r->p = p;
    } else if (h->l) {
      if (p->l == h) {
	p->l = h->l;
      } else {
	p->r = h->l;
      }
      h->l->p = p;
    } else if (h->r) {
      if (p->l == h) {
	p->l = h->r;
      } else {
	p->r = h->r;
      }
      h->r->p = p;
    } else {
      if (p->l == h) {
	p->l = NULL;
      } else {
	p->r = NULL;
      }
    }
  }
  free(h->key);
  free(h);
}

void
nhash_del_with_hkey(NHASH hash, char *key, int hkey)
{
  struct nhash *h;

  h = nhash_get_with_hkey(hash, key, hkey);
  if (h)
    nhash_del_sub(hash, h, hkey);
}

void
nhash_del(NHASH hash, char *key)
{
  struct nhash *h;
  int hkey;

  hkey = nhash_hkey(key);
  h = nhash_get_with_hkey(hash, key, hkey);
  if (h)
    nhash_del_sub(hash, h, hkey);
}

static int
hash_each_sub(struct nhash *h, int(* func)(struct nhash *, void *), void *data)
{
  if (h == NULL)
    return 0;

  if (h->l)
    hash_each_sub(h->l, func, data);

  if (h->r)
    hash_each_sub(h->r, func, data);

  return func(h, data);
}

int
nhash_each(NHASH hash, int(* func)(struct nhash *, void *), void *data)
{
  int i, r;

  if (func == NULL || hash == NULL)
    return 0;

  for (i = 0; i < HASH_SIZE; i++) {
    if (hash[i]) {
      r = hash_each_sub(hash[i], func, data);
      if (r)
	return r;
    }
  }
  return 0;
}

int
nhash_get_ptr(NHASH hash, char *key, void **ptr)
{
  struct nhash *h;

  h = nhash_get(hash, key);

  if (h == NULL) {
    *ptr = NULL;
    return 1;
  }

  *ptr = h->val.p;

  return 0;
}

int
nhash_get_ptr_with_hkey(NHASH hash, char *key, int hkey, void **ptr)
{
  struct nhash *h;

  h = nhash_get_with_hkey(hash, key, hkey);

  if (h == NULL) {
    *ptr = NULL;
    return 1;
  }

  *ptr = h->val.p;

  return 0;
}

static void 
print_hash(struct nhash *h)
{
  if (h == NULL)
    return;

  printf("%s ", h->key);
  print_hash(h->l);
  print_hash(h->r);
}

void
nhash_show(NHASH hash)
{
  int i;

  for (i = 0; i < HASH_SIZE; i++) {
    printf("%2d: ", i);
    print_hash(hash[i]);
    printf("\n");
  }
}

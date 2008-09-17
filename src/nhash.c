/* 
 * $Id: nhash.c,v 1.4 2008/09/17 01:54:58 hito Exp $
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

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
free_hash_list(struct nhash *hash)
{
  struct nhash *l, *r;

  if (hash == NULL)
    return;

  l = hash->l;
  r = hash->r;
  free(hash->key);
  free(hash);
  free_hash_list(l);
  free_hash_list(r);
}

void
nhash_free(NHASH hash)
{
  int i = 0;

  for (i = 0; i < HASH_SIZE; i++) {
    free_hash_list(hash[i]);
  }
}

static int
calc_key_from_str(char *ptr)
{
  unsigned int i, v;

  for (v = i = 0; ptr[i]; i++) {
    v += ptr[i];
  }

  return v % HASH_SIZE;
}

struct nhash *
create_hash(NHASH hash, char *key)
{
  int hkey, lr = 0;
  char *k;
  struct nhash *h, *ptr, *prev = NULL;

  hkey = calc_key_from_str(key);

  ptr = hash[hkey];

  if (ptr == NULL) {
    h = malloc(sizeof(struct nhash));
    if (h == NULL)
      return NULL;
    hash[hkey] = h;
  } else {
    while (ptr) {
      lr = strcmp(key, ptr->key);
      switch (lr) {
      case -1:
	prev = ptr;
	ptr = ptr->l;
	break;
      case 0:
	return ptr;
      case 1:
	prev = ptr;
	ptr = ptr->r;
	break;
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
    if (lr == -1) {
      prev->l = h;
    } else {
      prev->r = h;
    }
  }

  h->key = k;
  h->l = NULL;
  h->r = NULL;

  return h;
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

static struct nhash *
nhash_get(NHASH hash, char *key)
{
  struct nhash *ptr;
  int hk;

  if (key == NULL)
    return NULL;

  hk = calc_key_from_str(key);

  ptr = hash[hk];

  while (ptr) {
    switch (strcmp(key, ptr->key)) {
    case -1:
      ptr = ptr->l;
      break;
    case 0:
      return ptr;
    case 1:
      ptr = ptr->r;
      break;
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
nhash_get_ptr(NHASH hash, char *key, void **ptr)
{
  struct nhash *h;

  h = nhash_get(hash, key);

  if (h == NULL)
    return 1;

  *ptr = h->val.p;

  return 0;
}

static void print_hash(struct nhash *h)
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

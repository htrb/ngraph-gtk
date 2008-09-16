/* 
 * $Id: nhash.c,v 1.2 2008/09/16 08:53:04 hito Exp $
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
  struct nhash *next;

  if (hash == NULL)
    return;

  next = hash->next;
  free(hash->key);
  free(hash);
  free_hash_list(next);
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
calc_key(int v)
{
  return v % HASH_SIZE;
}

static int
calc_key_from_str(char *ptr)
{
  int i, v;

  for (v = i = 0; ptr[i]; i++) {
    v += ptr[i];
  }

  return calc_key(v);
}

struct nhash *
create_hash(NHASH hash, char *key)
{
  int hkey;
  char *k;
  struct nhash *h, *ptr, *prev;

  hkey = calc_key_from_str(key);

  h = malloc(sizeof(struct nhash));
  if (h == NULL)
    return NULL;

  ptr = hash[hkey];

  if (ptr == NULL) {
    h = malloc(sizeof(struct nhash));
    if (h == NULL)
      return NULL;
    hash[hkey] = h;
  } else {
    while (ptr) {
      if (strcmp(key, ptr->key) == 0) {
	return ptr;
      }
      prev = ptr;
      ptr = ptr->next;
    }
    h = malloc(sizeof(struct nhash));
    if (h == NULL)
      return NULL;
    prev->next = h;
  }

  k = strdup(key);
  if (k == NULL) {
    free(h);
    return NULL;
  }

  h->key = k;

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
  h->next = NULL;

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
  h->next = NULL;

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
    if (strcmp(key, ptr->key) == 0) {
      return ptr;
    }
    ptr = ptr->next;
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

void
nhash_show(NHASH hash)
{
  struct nhash *h;
  int i;

  for (i = 0; i < HASH_SIZE; i++) {
    h = hash[i];
    printf("%2d: ", i);
    while (h) {
      printf("%s%s", h->key, (h->next) ? "\t": "");
      h = h->next;
    }
    printf("\n");
  }
}

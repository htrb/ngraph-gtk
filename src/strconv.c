/* 
 * $Id: strconv.c,v 1.2 2010-03-04 08:30:16 hito Exp $
 */

#include "common.h"

#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include "object.h"

unsigned char *
ascii2greece(char unsigned *src)
{
  int i, j, k, slen, dlen, ucode, lcode;
  static unsigned char *u = (unsigned char *) "ΑΒΧΔΕΦΓΗΙϑΚΛΜΝΟΠΘΡΣΤΥϛΩΞΨΖ", *l = (unsigned char *) "αβχδεϕγηιφκλμνοπθρστυϖωξψζ";
  unsigned char *tmp;

  ucode = (u[0] << 8) + u[1];
  lcode = (l[0] << 8) + l[1];

  slen = strlen((char *)src);
  dlen = slen * 2 + 1;

  tmp = g_malloc(dlen);
  if (tmp == NULL)
    return NULL;

  for (i = j = 0; i < slen; i++, j++) {
    if (src[i] >= 'a' && src[i] <= 'z') {
      k = src[i] - 'a';
      tmp[j] = l[k * 2];
      j++;
      tmp[j] = l[k * 2 + 1];
    } else if (src[i] >= 'A' && src[i] <= 'Z') {
      k = src[i] - 'A';
      tmp[j] = u[k * 2];
      j++;
      tmp[j] = u[k * 2 + 1];
    } else {
      tmp[j] = src[i];
    }
  }
  tmp[j] = '\0';

  return tmp;
}

static char *
str2utf8(char *str, char *scode, char *dcode)
{
  GIConv cd;
  size_t l, slen, dlen;
  char *tmp, *ptr;

  if (str == NULL) 
    return NULL;

  cd = g_iconv_open(dcode, scode);
  if (cd == (GIConv)(-1)) {
    g_iconv_close(cd);
    return NULL;
  }

  slen = strlen(str);
  dlen = slen * 6 + 1;

  tmp = g_malloc(dlen);
  if (tmp == NULL) 
    return NULL;

  ptr = tmp;
  l = g_iconv(cd, &str, &slen, &ptr, &dlen);
  if (l == (size_t)(-1)) {
    g_free(tmp);
    tmp = NULL;
  } else {
    *ptr = '\0';
  }

  g_iconv_close(cd);

  return tmp;
}

char *
iso8859_to_utf8(char *src)
{
  return str2utf8(src, "iso-8859-1", "utf-8//TRANSLIT");
}

char *
sjis_to_utf8(char *src)
{
  return str2utf8(src, "CP932", "utf-8//TRANSLIT");
}

char *
utf8_to_sjis(char *src)
{
  return str2utf8(src, "utf-8", "CP932//TRANSLIT");
}


/* 
 * $Id: strconv.c,v 1.2 2010-03-04 08:30:16 hito Exp $
 */

#include "common.h"

#include <string.h>
#include <stdlib.h>
#include <glib.h>

#include "object.h"

gchar *
ascii2greece(const gchar *src)
{
  int i, j, k, c, len, slen, dlen;
  static char *u[] = {"Α", "Β", "Χ", "Δ", "Ε", "Φ", "Γ", "Η", "Ι",
		      "ϑ", "Κ", "Λ", "Μ", "Ν", "Ο", "Π", "Θ", "Ρ",
		      "Σ", "Τ", "Υ", "ϛ", "Ω", "Ξ", "Ψ", "Ζ"};
  static char *l[] = {"α", "β", "χ", "δ", "ε", "ϕ", "γ", "η", "ι",
		      "φ", "κ", "λ", "μ", "ν", "ο", "π", "θ", "ρ",
		      "σ", "τ", "υ", "ϖ", "ω", "ξ", "ψ", "ζ"};
  gchar *tmp, *ch, buf[2];

  slen = strlen((char *)src);
  dlen = slen * 6 + 1;

  tmp = g_malloc(dlen);
  if (tmp == NULL)
    return NULL;

  tmp[0] = '\0';
  for (i = j = 0; i < slen; i++) {
    if (src[i] >= 'a' && src[i] <= 'z') {
      c = src[i] - 'a';
      ch = l[c];
    } else if (src[i] >= 'A' && src[i] <= 'Z') {
      c = src[i] - 'A';
      ch = u[c];
    } else {
      buf[0] = src[i];
      buf[1] = '\0';
      ch = buf;
    }

    len = strlen(ch);
    for (k = 0; k < len; k++) {
      tmp[j++] = ch[k];
    }
    tmp[j] = '\0';
  }

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


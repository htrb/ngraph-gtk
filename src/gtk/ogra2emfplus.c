#include "common.h"

#if WINDOWS
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <unistd.h>
#include <math.h>
#include <cairo/cairo.h>

#include "gtk_common.h"

#include "object.h"
#include "nstring.h"
#include "ioutil.h"
#include "shell.h"
#include "strconv.h"
#include "gra.h"
#include "ogra2cairo.h"
#include "emfplus.h"

#define NAME "gra2emfplus"
#define PARENT "gra2"
#define OVERSION  "1.00.00"

#ifndef MPI
#define MPI 3.14159265358979323846
#endif

#define ERRFOPEN 100
#define ERREMF   101

static char *gra2emfplus_errorlist[]={
  "I/O error: open file",
  "EMF error",
};

#define ERRNUM (sizeof(gra2emfplus_errorlist) / sizeof(*gra2emfplus_errorlist))

#define CHAR_SET_NUM 32

struct gra2emfplus_local {
  struct gdiobj *gdi;
  int r, g, b, x, y;
  int symbol;
  struct font_info font;
  struct narray line;
};

static int
gra2emfplus_init(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2emfplus_local *local = NULL;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  local = g_malloc0(sizeof(struct gra2emfplus_local));
  if (local == NULL){
    goto errexit;
  }

  local->gdi = NULL;
  local->font.font = NULL;

  if (_putobj(obj, "_local", inst, local)) {
    goto errexit;
  }

  arrayinit(&local->line, sizeof(int));

  return 0;

 errexit:
  g_free(local);

  return 1;
}

static int
gra2emfplus_done(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2emfplus_local *local;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  _getobj(obj, "_local", inst, &local);
  if (local == NULL) {
    return 0;
  }

  if (local->gdi) {
    g_clear_pointer (&local->gdi, emfplus_finalize);
  }
  if (local->font.font) {
    g_free (local->font.font);
  }

  arraydel(&local->line);

  return 0;
}

static int
gra2emfplus_flush(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2emfplus_local *local;

  _getobj(obj, "_local", inst, &local);

  if (local->gdi == NULL) {
    return -1;
  }
  emfplus_flush (local->gdi);

  return 0;
}

static void
set_font(struct gra2emfplus_local *local, const char *fontname)
{
  struct compatible_font_info *info;
  struct fontmap *font_map;
  const char *alias;

  g_clear_pointer (&local->font.font, g_free);
  info = gra2cairo_get_compatible_font_info(fontname);
  if (info == NULL) {
    alias = fontname;
    local->symbol = FALSE;
  } else {
    alias = info->name;
    local->font.style = info->style;
    local->symbol = info->symbol;
  }
  font_map = gra2cairo_get_fontmap(alias);
  local->font.font = g_utf8_to_utf16 (font_map->fontname, -1, NULL, NULL, NULL);
}

static void
draw_lines(struct gra2emfplus_local *local)
{
  int n, *data;

  n = arraynum(&local->line) / 2;
  if (n < 2) {
    arrayclear(&local->line);
    return;
  }

  data = arraydata(&local->line);
  emfplus_lines (local->gdi, n, data);
  arrayclear(&local->line);
}

static void
draw_text (struct gra2emfplus_local *local, const char *cstr)
{
  char *tmp;
  gunichar2 *utext;
  tmp = gra2cairo_get_utf8_str(cstr, local->symbol);
  utext = g_utf8_to_utf16 (tmp, -1, NULL, NULL, NULL);
  emfplus_text (local->gdi, &local->x, &local->y, &local->font, utext);
  g_free (utext);
  g_free (tmp);
}

static void
add_line_to(struct gra2emfplus_local *local, int x, int y)
{
  if (arraynum(&local->line) < 1) {
    arrayclear(&local->line);
    arrayadd(&local->line, &local->x);
    arrayadd(&local->line, &local->y);
  }
  local->x = x;
  local->y = y;
  arrayadd(&local->line, &local->x);
  arrayadd(&local->line, &local->y);
}

static int
gra2emfplus_output(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char code, *cstr, *tmp, *fname;
  int *cpar;
  struct gra2emfplus_local *local;
  gunichar2 *utext;

  local = (struct gra2emfplus_local *)argv[2];
  code = *(char *)(argv[3]);
  cpar = (int *)argv[4];
  cstr = argv[5];

  if (code != 'I' && local->gdi == NULL) {
    return 1;
  }

  if (code != 'T') {
    draw_lines(local);
  }

  switch (code) {
  case 'I':
    _getobj(obj, "file", inst, &fname);
    utext = g_utf8_to_utf16 (fname, -1, NULL, NULL, NULL);
    local->gdi = emfplus_init(utext, cpar[3], cpar[4], cpar[5]);
    g_free (utext);
    break;
  case '%': case 'X': case 'Z':
    break;
  case 'E':
    g_clear_pointer (&local->gdi, emfplus_finalize);
    break;
  case 'V':
    emfplus_clip (local->gdi, cpar[1], cpar[2], cpar[3], cpar[4]);
    break;
  case 'A':
    emfplus_line_attribte (local->gdi, cpar[2], cpar[3], cpar[4], cpar[5], cpar[1], cpar + 6);
    break;
  case 'G':
    emfplus_color (local->gdi, cpar[1], cpar[2], cpar[3], cpar[4]);
    break;
  case 'M':
    local->x = cpar[1];
    local->y = cpar[2];
    break;
  case 'N':
    local->x += cpar[1];
    local->y += cpar[2];
    break;
  case 'L':
    emfplus_line (local->gdi, cpar[1], cpar[2], cpar[3], cpar[4]);
    break;
  case 'T':
    add_line_to (local, cpar[1], cpar[2]);
    break;
  case 'C':
    emfplus_arc (local->gdi, cpar[1], cpar[2], cpar[3], cpar[4], cpar[5], cpar[6], cpar[7]);
    break;
  case 'B':
    emfplus_rectangle(local->gdi, cpar[1], cpar[2], cpar[3], cpar[4], cpar[5]);
    break;
  case 'P':
    emfplus_rectangle (local->gdi, cpar[1] - 1, cpar[2] - 1, cpar[1] + 1, cpar[2] + 1, 1);
    break;
  case 'R':
    emfplus_lines (local->gdi, cpar[1], cpar + 2);
    break;
  case 'D':
    emfplus_polygon(local->gdi, cpar[1], cpar + 3, cpar[2]);
    break;
  case 'F':
    set_font(local, cstr);
    break;
  case 'H':
    local->font.space = cpar[2];
    local->font.size = cpar[1];
    local->font.dir = (cpar[3] % 36000);
    if (local->font.dir < 0) {
      local->font.dir += 36000;
    }
    if (cpar[0] > 3) {
      local->font.style = cpar[4];
    }
    break;
  case 'S':
    draw_text (local, cstr);
    break;
  case 'K':
    tmp = sjis_to_utf8(cstr);
    if (tmp) {
      draw_text (local, tmp);
      g_free(tmp);
    }
    break;
  default:
    break;
  }
  return 0;
}

static struct objtable gra2emfplus[] = {
  {"init",    NVFUNC, NEXEC, gra2emfplus_init, NULL, 0},
  {"done",    NVFUNC, NEXEC, gra2emfplus_done, NULL, 0},
  {"next",    NPOINTER, 0, NULL, NULL, 0},
  {"file",    NSTR, NREAD | NWRITE, NULL, NULL,0},
  {"flush",   NVFUNC,NREAD|NEXEC, gra2emfplus_flush,"",0},
  {"_output", NVFUNC, 0, gra2emfplus_output, NULL, 0},
  {"_local",  NPOINTER, 0, NULL, NULL, 0},
};

#define TBLNUM (sizeof(gra2emfplus) / sizeof(*gra2emfplus))

void *
addgra2emfplus(void)
/* addgra2emfplus() returns NULL on error */
{
  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, gra2emfplus, ERRNUM, gra2emfplus_errorlist, NULL, NULL);
}

#endif	/* WINDOWS */

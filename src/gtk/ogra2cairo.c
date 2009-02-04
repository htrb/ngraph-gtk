/* 
 * $Id: ogra2cairo.c,v 1.39 2009/02/04 07:23:06 hito Exp $
 */

#include "gtk_common.h"

#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>
#include <ctype.h>

#include <cairo/cairo.h>

#include "mathfn.h"
#include "ngraph.h"
#include "nstring.h"
#include "object.h"
#include "ioutil.h"
#include "nconfig.h"

#include "strconv.h"

#include "x11gui.h"
#include "ogra2cairo.h"

#define NAME "gra2cairo"
#define PARENT "gra2"
#define OVERSION  "1.00.00"
#define CAIROCONF "[gra2cairo]"

#ifndef M_PI
#define M_PI 3.141592
#endif

char **Gra2CairoErrMsgs = NULL;
int Gra2CairoErrMsgNum = 0;

char *gra2cairo_antialias_type[] = {
  N_("none"),
  N_("default"),
  N_("gray"),
  //  N_("subpixel"),
  NULL,
};

struct gra2cairo_config *Gra2cairoConf = NULL;
static int Instance = 0;
static NHASH FontFace = NULL;
static char *FontFaceStr[] = {
  "normal",
  "bold",
  "italic",
  "bold_italic",
  "oblique",
  "bold_oblique",
};

static int
check_type(int type)
{
  if (type < 0 || type >= sizeof(FontFaceStr) / sizeof(*FontFaceStr)) {
    type = NORMAL;
  }
  return type;
}

static int
mxp2dw(struct gra2cairo_local *local, int r)
{
  return ceil(r / local->pixel_dot_x);
}

static double
mxd2pw(struct gra2cairo_local *local, int r)
{
  return r * local->pixel_dot_x;
}

static int
mxp2dh(struct gra2cairo_local *local, int r)
{
  return ceil(r / local->pixel_dot_y);
}

static double
mxd2ph(struct gra2cairo_local *local, int r)
{
  return r * local->pixel_dot_y;
}

static double
mxd2px(struct gra2cairo_local *local, int x)
{
  return x * local->pixel_dot_x + local->offsetx;
}

static double
mxd2py(struct gra2cairo_local *local, int y)
{
  return y * local->pixel_dot_y + local->offsety;
}

static void
free_font_map(struct fontmap *fcur)
{
  if (fcur == NULL)
    return;

  if (fcur->font) {
    pango_font_description_free(fcur->font);
  }

  memfree(fcur->fontalias);
  memfree(fcur->fontname);

  if (fcur->prev) {
    fcur->prev->next = fcur->next;
  } else {
    Gra2cairoConf->fontmap_list_root = fcur->next;
  }

  if (fcur->next) {
    fcur->next->prev = fcur->prev;
  }

  memfree(fcur);
}

static struct fontmap *
create_font_map(char *fontalias, char *fontname, int type, int twobyte, struct fontmap *fprev)
{
  struct fontmap *fnew;
  char symbol[] = "Sym";

  if (Gra2cairoConf->fontmap == NULL)
    return NULL;

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fnew) == 0) {
    free_font_map(fnew);
  }

  fnew = memalloc(sizeof(struct fontmap));
  if (fnew == NULL) {
    return NULL;
  }

  if (nhash_set_ptr(Gra2cairoConf->fontmap, fontalias, fnew)) {
    memfree(fnew);
    return NULL;
  }

  fnew->fontalias = fontalias;
  fnew->symbol = ! strncmp(fontalias, symbol, sizeof(symbol) - 1);
  fnew->type = type;
  fnew->twobyte = twobyte;
  fnew->fontname = fontname;
  fnew->font = NULL;
  fnew->next = NULL;

  if (fprev) {
    fnew->prev = fprev;
    fprev->next = fnew;
  } else {
    fnew->prev = NULL;
    fnew->next = Gra2cairoConf->fontmap_list_root;
    Gra2cairoConf->fontmap_list_root = fnew;
  }

  Gra2cairoConf->font_num++;

  return fnew;
}

static int
loadconfig(void)
{
  FILE *fp;
  char *tok, *str, *s2;
  char *f1, *f2, *f3, *f4;
  int val;
  char *endptr;
  int len, type;
  struct fontmap *fnew, *fprev;

  fp = openconfig(CAIROCONF);
  if (fp == NULL)
    return 0;

  fprev = NULL;
  while ((tok = getconfig(fp, &str))) {
    s2 = str;
    if (strcmp(tok, "font_map") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      for (; (s2[0] != '\0') && (strchr(" \x09,", s2[0])); s2++);
      f4 = getitok2(&s2, &len, "");
      if (f1 && f2 && f3 && f4) {
	type = NORMAL;
	if (FontFace) {
	  int i;
	  if (nhash_get_int(FontFace, f2, &i) == 0) {
	    type = i;
	  }
	}
	memfree(f2);

	val = strtol(f3, &endptr, 10);
	memfree(f3);

	fnew = create_font_map(f1, f4, type, val, fprev);
	if (fnew == NULL) {
	  memfree(tok);
	  memfree(f1);
	  memfree(f2);
	  memfree(f3);
	  memfree(f4);
	  closeconfig(fp);
	  return 1;
	}

	fprev = fnew;
      } else {
	memfree(f1);
	memfree(f2);
	memfree(f3);
	memfree(f4);
      }
    }
    memfree(tok);
    memfree(str);
  }
  closeconfig(fp);
  return 0;
}

static int
free_fonts_sub(struct nhash *h, void *d)
{
  struct fontmap *fcur;

  fcur = (struct fontmap *) h->val.p;

  free_font_map(fcur);

  return 0;
}

static void
free_fonts(struct gra2cairo_config *conf)
{
  nhash_each(conf->fontmap, free_fonts_sub, NULL);
  conf->font_num = 0;
}

static int
init_conf(void)
{
  Gra2cairoConf = malloc(sizeof(*Gra2cairoConf));
  if (Gra2cairoConf == NULL)
    return 1;

  if (FontFace == NULL) {
    FontFace = nhash_new();
    if (FontFace) {
      nhash_set_int(FontFace, FontFaceStr[BOLD],        BOLD);
      nhash_set_int(FontFace, FontFaceStr[ITALIC],      ITALIC);
      nhash_set_int(FontFace, FontFaceStr[BOLDITALIC],  BOLDITALIC);
      nhash_set_int(FontFace, FontFaceStr[OBLIQUE],     OBLIQUE);
      nhash_set_int(FontFace, FontFaceStr[BOLDOBLIQUE], BOLDOBLIQUE);
    }
  }

  Gra2cairoConf->fontmap = nhash_new();
  if (Gra2cairoConf->fontmap == NULL) {
    free(Gra2cairoConf);
    Gra2cairoConf = NULL;
    return 1;
  }

  Gra2cairoConf->fontmap_list_root = NULL;
  Gra2cairoConf->font_num = 0;

  if (loadconfig()) {
    free_fonts(Gra2cairoConf);
    nhash_free(Gra2cairoConf->fontmap);
    free(Gra2cairoConf);
    Gra2cairoConf = NULL;
    return 1;
  }

  return 0;
}

static void
free_conf(void)
{
  if (Gra2cairoConf == NULL)
    return;

  free_fonts(Gra2cairoConf);
  nhash_free(Gra2cairoConf->fontmap);

  free(Gra2cairoConf);
  Gra2cairoConf = NULL;
}

const char *
gra2cairo_get_font_type_str(int type)
{
  type = check_type(type);

  return FontFaceStr[type];
}

struct fontmap *
gra2cairo_get_fontmap(char *fontalias)
{
  struct fontmap *fnew;

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fnew)) {
    return NULL;
  }
  return fnew;
}

int
gra2cairo_get_fontmap_num(void)
{
  int n = 0;

  if (Gra2cairoConf->fontmap) {
    n = nhash_num(Gra2cairoConf->fontmap);
  }
  return n;
}

void
gra2cairo_remove_fontmap(char *fontalias)
{
  struct fontmap *fnew;

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fnew) == 0) {
    free_font_map(fnew);
    nhash_del(Gra2cairoConf->fontmap, fontalias);
  }
}

void
gra2cairo_update_fontmap(const char *fontalias, const char *fontname, int type, int two_byte)
{
  struct fontmap *fcur;

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fcur))
      return;

  if (strcmp(fontname, fcur->fontname)) {
    memfree(fcur->fontname);
    fcur->fontname = nstrdup(fontname);
  }

  if (fcur->font) {
    pango_font_description_free(fcur->font);
    fcur->font = NULL;
  }
  fcur->type = type;
  fcur->twobyte = two_byte;
}

void
gra2cairo_add_fontmap(const char *fontalias, const char *fontname, int type, int two_byte)
{
  struct fontmap *fnew;
  char *alias, *name;

  alias = nstrdup(fontalias);
  if (alias == NULL)
    return;
  
  name = nstrdup(fontname);
  if (name == NULL) {
    memfree(alias);
    return;
  }

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fnew) == 0) {
    free_font_map(fnew);
  }
  create_font_map(alias, name, type, two_byte, NULL);
}

static int
gra2cairo_init(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{  
  struct gra2cairo_local *local = NULL;
  int antialias = ANTIALIAS_TYPE_DEFAULT;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;

  if (Gra2cairoConf == NULL && init_conf()) {
    goto errexit;
  }

  local = memalloc(sizeof(struct gra2cairo_local));
  if (local == NULL)
    goto errexit;

  if (_putobj(obj, "_local", inst, local))
    goto errexit;

  if (_putobj(obj, "antialias", inst, &antialias))
    goto errexit;

  local->cairo = NULL;
  local->fontalias = NULL;
  local->layout = NULL;
  local->pixel_dot_x = 1;
  local->pixel_dot_y = 1;
  local->linetonum = 0;
  local->text2path = FALSE;
  local->antialias = antialias;
  local->region = NULL;

  Instance++;

  return 0;

 errexit:
  if (Instance == 0)
    free_conf();

  memfree(local);
  return 1;
}

struct gra2cairo_local *
gra2cairo_free(struct objlist *obj, char *inst)
{
  struct gra2cairo_local *local;

  _getobj(obj, "_local", inst, &local);

  if (local == NULL)
    return NULL;

  if (local->cairo) {
    if (local->linetonum) {
      cairo_stroke(local->cairo);
      local->linetonum = 0;
    }
    cairo_destroy(local->cairo);
  }

  if (local->layout) {
    g_object_unref(local->layout);
  }

  if (local->fontalias) {
    memfree(local->fontalias);
  }

  Instance--;
  if (Instance == 0) {
    free_conf();
  }

  return local;
}

static int 
gra2cairo_done(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  gra2cairo_free(obj, inst);

  return 0;
}

int 
gra2cairo_clip_region(struct gra2cairo_local *local, GdkRegion *region)
{
  if (local == NULL || local->cairo == NULL)
    return 1;

  cairo_new_path(local->cairo);
  if (region) {
    gdk_cairo_region(local->cairo, region);
    cairo_clip(local->cairo);
    local->region = region;
  } else {
    cairo_reset_clip(local->cairo);
    local->region = NULL;
  }
  return 0;
}

static double
gra2cairo_get_pixel_dot(struct objlist *obj, char *inst, char **argv)
{
  int dpi;

  dpi = *(int *) argv[2];

  if (dpi < 1)
    dpi = 1;

  if (dpi > DPI_MAX)
    dpi = DPI_MAX;

  *(int *)argv[2] = dpi;

  return dpi / (DPI_MAX * 1.0);
}

static int
gra2cairo_set_dpi(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int dpi;
  struct gra2cairo_local *local;

  dpi = *(int *) argv[2];

  _getobj(obj, "_local", inst, &local);

  _putobj(obj, "dpix", inst, &dpi);
  _putobj(obj, "dpiy", inst, &dpi);

  local->pixel_dot_x =
    local->pixel_dot_y = gra2cairo_get_pixel_dot(obj, inst, argv);

  return 0;
}

static int
gra2cairo_set_dpi_x(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;

  _getobj(obj, "_local", inst, &local);

  local->pixel_dot_x = gra2cairo_get_pixel_dot(obj, inst, argv);

  return 0;
}

static int
gra2cairo_set_dpi_y(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;

  _getobj(obj, "_local", inst, &local);

  local->pixel_dot_y = gra2cairo_get_pixel_dot(obj, inst, argv);

  return 0;
}

void
gra2cairo_set_antialias(struct gra2cairo_local *local, int antialias)
{
  if (local->cairo == NULL)
    return;

  switch (antialias) {
  case ANTIALIAS_TYPE_NONE:
    antialias = CAIRO_ANTIALIAS_NONE;
    break;
  case ANTIALIAS_TYPE_DEFAULT:
    antialias = CAIRO_ANTIALIAS_DEFAULT;
    break;
  case ANTIALIAS_TYPE_GRAY:
    antialias = CAIRO_ANTIALIAS_GRAY;
    break;
  case ANTIALIAS_TYPE_SUBPIXEL:
    antialias = CAIRO_ANTIALIAS_SUBPIXEL;
    break;
  }

  cairo_set_antialias(local->cairo, antialias);
}

static int
set_antialias(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  int antialias;
  struct gra2cairo_local *local;

  antialias = *(int *) argv[2];

  _getobj(obj, "_local", inst, &local);

  local->antialias = antialias;

  gra2cairo_set_antialias(local, antialias);

  return 0;
}

static struct fontmap *
loadfont(char *fontalias)
{
  struct fontmap *fcur;
  PangoFontDescription *pfont;
  PangoStyle style;
  PangoWeight weight;

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fcur))
    return NULL;

  if (fcur->font)
    return fcur;

  pfont = pango_font_description_new();
  pango_font_description_set_family(pfont, fcur->fontname);

  switch (fcur->type) {
  case ITALIC:
  case BOLDITALIC:
    style = PANGO_STYLE_ITALIC;
    break;
  case OBLIQUE:
  case BOLDOBLIQUE:
    style = PANGO_STYLE_OBLIQUE;
    break;
  default:
    style = PANGO_STYLE_NORMAL;
    break;
  }
  pango_font_description_set_style(pfont, style);

  switch (fcur->type) {
  case BOLD:
  case BOLDITALIC:
  case BOLDOBLIQUE:
    weight = PANGO_WEIGHT_BOLD;
    break;
  default:
    weight = PANGO_WEIGHT_NORMAL;
    break;
  }
  pango_font_description_set_weight(pfont, weight);

  fcur->font = pfont;

  return fcur;
}

static void
draw_str(struct gra2cairo_local *local, int draw, char *str, struct fontmap *font, int size, int space, int *fw, int *ah, int *dh)
{
  PangoAttribute *attr;
  PangoAttrList *alist;
  PangoLayoutIter *piter;
  int w, h, baseline;

  if (size == 0) {
    if (fw)
      *fw = 0;

    if (ah)
      *ah = 0;

    if (dh)
      *dh = 0;
    return;
  }

  if (local->layout == NULL) {
    local->layout = pango_cairo_create_layout(local->cairo);
  }

  alist = pango_attr_list_new();
  attr = pango_attr_size_new_absolute(mxd2ph(local, size) * PANGO_SCALE);
  pango_attr_list_insert(alist, attr);

  attr = pango_attr_letter_spacing_new(mxd2ph(local, space) * PANGO_SCALE);
  pango_attr_list_insert(alist, attr);

  pango_layout_set_font_description(local->layout, font->font);
  pango_layout_set_attributes(local->layout, alist);
  pango_attr_list_unref(alist);

  pango_layout_set_text(local->layout, str, -1);

  pango_layout_get_pixel_size(local->layout, &w, &h);
  piter = pango_layout_get_iter(local->layout);
  baseline = pango_layout_iter_get_baseline(piter) / PANGO_SCALE;

  if (fw)
    *fw = w;

  if (ah)
    *ah = baseline;

  if (dh)
    *dh = h - baseline;

  if (draw && str) {
    double x, y;
    double cx, cy;

    x = - local->fontsin * baseline;
    y = - local->fontcos * baseline;

    cairo_get_current_point(local->cairo, &cx, &cy);
    cairo_rel_move_to(local->cairo, x, y);

    cairo_save(local->cairo);
    cairo_rotate(local->cairo, -local->fontdir * G_PI / 180.);
    pango_cairo_update_layout(local->cairo, local->layout);
    if (local->text2path) {
      pango_cairo_layout_path(local->cairo, local->layout);
      cairo_fill(local->cairo);
      cairo_restore(local->cairo);
      cairo_move_to(local->cairo, cx + w * local->fontcos, cy - w * local->fontsin);
    } else {
      pango_cairo_show_layout(local->cairo, local->layout);
      cairo_restore(local->cairo);
      cairo_rel_move_to(local->cairo, w * local->fontcos - x, - w * local->fontsin - y);
    }
  }

  pango_layout_iter_free(piter);
}

static int
check_cairo_status(cairo_t *cairo)
{
  int r;

  r = cairo_status(cairo);
  if (r != CAIRO_STATUS_SUCCESS) {
    return r + 100;
  }
  r = cairo_surface_status(cairo_get_target(cairo));
  if (r != CAIRO_STATUS_SUCCESS) {
    return r + 100;
  }
  return 0;
}

static int 
gra2cairo_flush(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{ 
  struct gra2cairo_local *local;

  _getobj(obj, "_local", inst, &local);

  if (local->cairo == NULL)
    return -1;

  if (local->linetonum) {
    cairo_stroke(local->cairo);
    local->linetonum = 0;
  }

  return check_cairo_status(local->cairo);
}

static int 
gra2cairo_output(struct objlist *obj, char *inst, char *rval, 
                 int argc, char **argv)
{
  char code, *cstr, *tmp, *tmp2;
  int *cpar, i, j, r;
  double x, y, w, h, l, fontcashsize, fontcashdir, fontsize,
    fontspace, fontdir, fontsin, fontcos, a1, a2;
  cairo_line_join_t join;
  cairo_line_cap_t cap;
  double *dashlist = NULL;
  struct gra2cairo_local *local;

  local = (struct gra2cairo_local *)argv[2];
  code = *(char *)(argv[3]);
  cpar = (int *)argv[4];
  cstr = argv[5];

  if (local->cairo == NULL)
    return -1;

  if (local->linetonum && code != 'T') {
    cairo_get_current_point(local->cairo, &x, &y);
    cairo_stroke(local->cairo);
    cairo_move_to(local->cairo, x, y);
    local->linetonum = 0;
  }
  switch (code) {
  case 'I':
    gra2cairo_set_antialias(local, local->antialias);
    local->linetonum = 0;
    r = check_cairo_status(local->cairo);
    if (r) {
      error(obj, r);
      return 1;
    }
    break;
  case '%': case 'X':
    break;
  case 'E':
    r = check_cairo_status(local->cairo);
    if (r) {
      error(obj, r);
      return 1;
    }
    break;
  case 'V':
    local->offsetx = mxd2pw(local, cpar[1]);
    local->offsety = mxd2ph(local, cpar[2]);
    cairo_new_path(local->cairo);
    if (cpar[5]) {
      x = mxd2pw(local, cpar[1]);
      y = mxd2ph(local, cpar[2]);
      w = mxd2pw(local, cpar[3]) - x;
      h = mxd2ph(local, cpar[4]) - y;

      cairo_reset_clip(local->cairo);
      cairo_rectangle(local->cairo, x, y, w, h);
      cairo_clip(local->cairo);
    } else {
      cairo_reset_clip(local->cairo);
    }

    if (local->region)
      gra2cairo_clip_region(local, local->region);

    break;
  case 'A':
    if (cpar[1] == 0) {
      cairo_set_dash(local->cairo, NULL, 0, 0);
    } else {
      dashlist = memalloc(sizeof(* dashlist) * cpar[1]);
      if (dashlist == NULL)
	break;
      for (i = 0; i < cpar[1]; i++) {
	dashlist[i] = mxd2pw(local, cpar[6 + i]);
        if (dashlist[i] <= 0) {
	  dashlist[i] = 1;
	}
      }
      cairo_set_dash(local->cairo, dashlist, cpar[1], 0);
      memfree(dashlist);
    }

    cairo_set_line_width(local->cairo, mxd2pw(local, cpar[2]));

    if (cpar[3] == 2) {
      cap = CAIRO_LINE_CAP_SQUARE;
    } else if (cpar[3] == 1) {
      cap = CAIRO_LINE_CAP_ROUND;
    } else {
      cap = CAIRO_LINE_CAP_BUTT;
    }
    cairo_set_line_cap(local->cairo, cap);

    if (cpar[4] == 2) {
      join = CAIRO_LINE_JOIN_BEVEL;
    } else if (cpar[4] == 1) {
      join = CAIRO_LINE_JOIN_ROUND;
    } else {
      join = CAIRO_LINE_JOIN_MITER;
    }
    cairo_set_line_join(local->cairo, join);
    break;
  case 'G':
    cairo_set_source_rgb(local->cairo, cpar[1] / 255.0, cpar[2] / 255.0, cpar[3] / 255.0);
    break;
  case 'M':
    cairo_move_to(local->cairo, mxd2px(local, cpar[1]), mxd2py(local, cpar[2]));
    break;
  case 'N':
    cairo_rel_move_to(local->cairo, mxd2pw(local, cpar[1]), mxd2ph(local, cpar[2]));
    break;
  case 'L':
    cairo_new_path(local->cairo);
    cairo_move_to(local->cairo, mxd2px(local, cpar[1]), mxd2py(local, cpar[2]));
    cairo_line_to(local->cairo, mxd2px(local, cpar[3]), mxd2py(local, cpar[4]));
    cairo_stroke(local->cairo);
    break;
  case 'T':
    cairo_line_to(local->cairo, mxd2px(local, cpar[1]), mxd2py(local, cpar[2]));
    local->linetonum++;
    break;
  case 'C':
    x = mxd2px(local, cpar[1] - cpar[3]);
    y = mxd2py(local, cpar[2] - cpar[4]);
    w = mxd2pw(local, cpar[3]);
    h = mxd2ph(local, cpar[4]);
    a1 = cpar[5] * (M_PI / 18000.0);
    a2 = cpar[6] * (M_PI / 18000.0) + a1;

    if (w == 0 || h == 0)
      break;

    cairo_new_path(local->cairo);
    cairo_save(local->cairo);
    cairo_translate(local->cairo, x + w, y + h);
    cairo_scale(local->cairo, w, h);
    cairo_arc_negative(local->cairo, 0., 0., 1., -a1, -a2);
    cairo_restore (local->cairo);
    if (cpar[7] == 0) {
      cairo_stroke(local->cairo);
    } else {
      if (cpar[7] == 1) {
	cairo_line_to(local->cairo, x + w, y + h);
      }
      cairo_close_path(local->cairo);
      cairo_fill(local->cairo);
    }
    break;
  case 'B':
    cairo_new_path(local->cairo);
    if (cpar[1] <= cpar[3]) {
      x = mxd2px(local, cpar[1]);
      w = mxd2pw(local, cpar[3] - cpar[1]);
    } else {
      x = mxd2px(local, cpar[3]);
      w = mxd2pw(local, cpar[1] - cpar[3]);
    }

    if (cpar[2] <= cpar[4]) {
      y = mxd2py(local, cpar[2]);
      h = mxd2ph(local, cpar[4] - cpar[2]);
    } else {
      y = mxd2py(local, cpar[4]);
      h = mxd2ph(local, cpar[2] - cpar[4]);
    }
    cairo_rectangle(local->cairo, x, y, w, h);
    if (cpar[5] == 0) {
      cairo_stroke(local->cairo);
    } else {
      cairo_fill(local->cairo);
    }
    break;
  case 'P': 
    cairo_new_path(local->cairo);
    cairo_arc(local->cairo, mxd2px(local, cpar[1]), mxd2py(local, cpar[2]), mxd2pw(local, 1), 0, 2 * M_PI);
    cairo_fill(local->cairo);
    break;
  case 'R': 
    cairo_new_path(local->cairo);
    if (cpar[1] == 0)
      break;

    for (i = 0; i < cpar[1]; i++) {
      cairo_line_to(local->cairo,
		    mxd2px(local, cpar[i * 2 + 2]),
		    mxd2py(local, cpar[i * 2 + 3]));
    }
    cairo_stroke(local->cairo);
    break;
  case 'D': 
    cairo_new_path(local->cairo);

    if (cpar[1] == 0) 
      break;

    for (i = 0; i < cpar[1]; i++) {
      cairo_line_to(local->cairo,
		    mxd2px(local, cpar[i * 2 + 3]),
		    mxd2py(local, cpar[i * 2 + 4]));
    }

    if (cpar[2] == 1) {
      cairo_set_fill_rule(local->cairo, CAIRO_FILL_RULE_EVEN_ODD);
    } else {
      cairo_set_fill_rule(local->cairo, CAIRO_FILL_RULE_WINDING);
    }
    cairo_close_path(local->cairo);
    if (cpar[2]) {
      cairo_fill(local->cairo);
    } else {
      cairo_stroke(local->cairo);
    }
    break;
  case 'F':
    memfree(local->fontalias);
    local->fontalias = nstrdup(cstr);
    break;
  case 'H':
    fontspace = cpar[2] / 72.0 * 25.4;
    local->fontspace = fontspace;
    fontsize = cpar[1] / 72.0 * 25.4;
    local->fontsize = fontsize;
    fontcashsize = mxd2ph(local, fontsize);
    fontcashdir = cpar[3];
    fontdir = cpar[3] * MPI / 18000.0;
    fontsin = sin(fontdir);
    fontcos = cos(fontdir);
    local->fontdir = (cpar[3] % 36000) / 100.0;
    if (local->fontdir < 0) {
      local->fontdir += 360;
    }
    local->fontsin = fontsin;
    local->fontcos = fontcos;

    local->loadfont = loadfont(local->fontalias);
    break;
  case 'S':
    if (local->loadfont == NULL)
      break;

    tmp = strdup(cstr);
    if (tmp == NULL)
      break;

    l = strlen(cstr);
    for (j = i = 0; i <= l; i++, j++) {
      char c;
      if (cstr[i] == '\\') {
	i++;
        if (cstr[i] == 'x') {
	  i++;
	  c = toupper(cstr[i]);
          if (c >= 'A') {
	    tmp[j] = c - 'A' + 10;
	  } else { 
	    tmp[j] = cstr[i] - '0';
	  }

	  i++;
	  tmp[j] *= 16;
	  c = toupper(cstr[i]);
          if (c >= 'A'){
	    tmp[j] += c - 'A' + 10;
	  } else {
	    tmp[j] += cstr[i] - '0';
	  }
        } else if (cstr[i] != '\0') {
          tmp[j] = cstr[i];
        }
      } else {
        tmp[j] = cstr[i];
      }
    }

    tmp2= iso8859_to_utf8(tmp);
    if (tmp2 == NULL) {
      free(tmp);
      break;
    }

    if (local->loadfont->symbol) {
      char *ptr;

      ptr = ascii2greece(tmp2);
      if (ptr) {
	free(tmp2);
	tmp2 = ptr;
      }
    }
    draw_str(local, TRUE, tmp2, local->loadfont, local->fontsize, local->fontspace, NULL, NULL, NULL);
    free(tmp2);
    free(tmp);
    break;
  case 'K':
    if (local->loadfont == NULL)
      break;

    tmp2 = sjis_to_utf8(cstr);
    if (tmp2 == NULL) 
      break;
    draw_str(local, TRUE, tmp2, local->loadfont, local->fontsize, local->fontspace, NULL, NULL, NULL);
    free(tmp2);
    break;
  default:
    break;
  }
  return 0;
}


int
gra2cairo_charwidth(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;
  char ch[3], *font, *tmp;
  double size, dir, s,c ;
  //  XChar2b kanji[1];
  int width;
  struct fontmap *fcur;
  //  XFontStruct *fontstruct;

  ch[0] = (*(unsigned int *)(argv[3]) & 0xff);
  ch[1] = (*(unsigned int *)(argv[3]) & 0xff00) >> 8;
  ch[2] = '\0';

  size = (*(int *)(argv[4])) / 72.0 * 25.4;
  font = (char *)(argv[5]);

  if (size == 0) {
    *(int *) rval = 0;
    return 0;
  }

  if (_getobj(obj, "_local", inst, &local))
    return 1;

  if (local->cairo == NULL)
    return 0;

  fcur = loadfont(font);

  if (fcur == NULL) {
    *(int *) rval = nround(size * 0.600);
    return 0;
  }

  dir = local->fontdir;
  s = local->fontsin;
  c = local->fontcos;

  local->fontdir = 0;
  local->fontsin = 0;
  local->fontcos = 1;

  if (ch[1]) {
    tmp = sjis_to_utf8(ch);
  } else {
    tmp = iso8859_to_utf8(ch);
  }

  draw_str(local, FALSE, tmp, fcur, size, 0, &width, NULL, NULL);
  *(int *) rval = mxp2dw(local, width);
  free(tmp);

  local->fontsin = s;
  local->fontcos = c;
  local->fontdir = dir;

  return 0;
}

int
gra2cairo_charheight(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;
  char *font;
  double size, dir, s, c;
  char *func;
  int height, descent, ascent;
  //  XFontStruct *fontstruct;
  struct fontmap *fcur;
  int twobyte;

  func = (char *)argv[1];
  size = (*(int *)(argv[3])) / 72.0 * 25.4;
  font = (char *)(argv[4]);

  if (size == 0) {
    *(int *) rval = 0;
    return 0;
  }

  if (_getobj(obj, "_local", inst, &local))
    return 1;

  if (local->cairo == NULL)
    return 1;

  if (strcmp0(func, "_charascent") == 0) {
    height = TRUE;
  } else {
    height = FALSE;
  }

  fcur = loadfont(font);

  if (fcur == NULL) {
    if (height) {
      *(int *) rval = nround(size * 0.562);
    } else {
      *(int *) rval = nround(size * 0.250);
    }

    return 0;
  }

  twobyte = fcur->twobyte;

  dir = local->fontdir;
  s = local->fontsin;
  c = local->fontcos;

  local->fontdir = 0;
  local->fontsin = 0;
  local->fontcos = 1;

  draw_str(local, FALSE, "A", fcur, size, 0, NULL, &ascent, &descent);

  if (height) {
    *(int *)rval = mxp2dh(local, ascent);
  } else {
    *(int *)rval = mxp2dh(local, descent);
  }

  /*

  if (cashpos != -1) {
    fontstruct = (mxlocal->font[cashpos]).fontstruct;    
    if (fontstruct) {
      if (twobyte) {
        if (height) {
	  *(int *)rval = nround(size * 0.791);
	} else {
	  *(int *)rval = nround(size * 0.250);
	}
      } else {
        if {
	  (height) *(int *)rval = nround(size * 0.562);
	} else {
	  *(int *)rval = nround(size * 0.250);
	}
      }
    } else {
      if (height) {
	*(int *) rval = mxp2dh(fontstruct->ascent);
      } else {
	*(int *) rval = mxp2dh(fontstruct->descent);
      }
    }
  } else {
    if (height) {
      *(int *) rval = nround(size * 0.562);
    } else {
      *(int *) rval = nround(size * 0.250);
    }
  }
  */

  local->fontsin = s;
  local->fontcos = c;
  local->fontdir = dir;

  return 0;
}

static struct objtable gra2cairo[] = {
  {"init", NVFUNC, NEXEC, gra2cairo_init, NULL, 0}, 
  {"done", NVFUNC, NEXEC, gra2cairo_done, NULL, 0}, 
  {"dpi", NINT, NREAD | NWRITE, gra2cairo_set_dpi, NULL, 0},
  {"dpix", NINT, NREAD | NWRITE, gra2cairo_set_dpi_x, NULL, 0},
  {"dpiy", NINT, NREAD | NWRITE, gra2cairo_set_dpi_y, NULL, 0},
  {"flush",NVFUNC,NREAD|NEXEC,gra2cairo_flush,"",0},
  {"antialias", NENUM, NREAD | NWRITE, set_antialias, gra2cairo_antialias_type, 0},
  {"_output", NVFUNC, 0, gra2cairo_output, NULL, 0}, 
  {"_charwidth", NIFUNC, 0, gra2cairo_charwidth, NULL, 0},
  {"_charascent", NIFUNC, 0, gra2cairo_charheight, NULL, 0},
  {"_chardescent", NIFUNC, 0, gra2cairo_charheight, NULL, 0},
  {"_local", NPOINTER, 0, NULL, NULL, 0}, 
};

#define TBLNUM (sizeof(gra2cairo) / sizeof(*gra2cairo))

void *
addgra2cairo()
/* addgra2cairoile() returns NULL on error */
{
  int errcode[] = {
    CAIRO_STATUS_SUCCESS,
    CAIRO_STATUS_NO_MEMORY,
    CAIRO_STATUS_INVALID_RESTORE,
    CAIRO_STATUS_INVALID_POP_GROUP,
    CAIRO_STATUS_NO_CURRENT_POINT,
    CAIRO_STATUS_INVALID_MATRIX,
    CAIRO_STATUS_INVALID_STATUS,
    CAIRO_STATUS_NULL_POINTER,
    CAIRO_STATUS_INVALID_STRING,
    CAIRO_STATUS_INVALID_PATH_DATA,
    CAIRO_STATUS_READ_ERROR,
    CAIRO_STATUS_WRITE_ERROR,
    CAIRO_STATUS_SURFACE_FINISHED,
    CAIRO_STATUS_SURFACE_TYPE_MISMATCH,
    CAIRO_STATUS_PATTERN_TYPE_MISMATCH,
    CAIRO_STATUS_INVALID_CONTENT,
    CAIRO_STATUS_INVALID_FORMAT,
    CAIRO_STATUS_INVALID_VISUAL,
    CAIRO_STATUS_FILE_NOT_FOUND,
    CAIRO_STATUS_INVALID_DASH,
    CAIRO_STATUS_INVALID_DSC_COMMENT,
    CAIRO_STATUS_INVALID_INDEX,
    CAIRO_STATUS_CLIP_NOT_REPRESENTABLE,
    CAIRO_STATUS_TEMP_FILE_ERROR,
    CAIRO_STATUS_INVALID_STRIDE,
  };
  int i, n;

  n = sizeof(errcode) / sizeof(*errcode);

  Gra2CairoErrMsgs = malloc(sizeof(*Gra2CairoErrMsgs) * n);
  Gra2CairoErrMsgNum = n;

  for (i = 0; i < n; i++) {
    Gra2CairoErrMsgs[i] = strdup(cairo_status_to_string(errcode[i]));
  }

  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, gra2cairo, n, Gra2CairoErrMsgs, NULL, NULL);
}

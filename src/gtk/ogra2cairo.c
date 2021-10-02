/*
 * $Id: ogra2cairo.c,v 1.58 2010-03-04 08:30:17 hito Exp $
 */

#include "gtk_common.h"

#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <ctype.h>

#include "mathfn.h"
#include "object.h"
#include "gra.h"
#include "init.h"
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

#define DEFAULT_FONT "Sans-serif"

#ifndef M_PI
#define M_PI 3.141592
#endif

char **Gra2CairoErrMsgs = NULL;
int Gra2CairoErrMsgNum = 0;
int OldConfigExist = FALSE;	/* for backward compatibility */

char *gra2cairo_antialias_type[] = {
  N_("none"),
  N_("default"),
  N_("gray"),
  //  N_("subpixel"),
  NULL,
};

struct gra2cairo_config *Gra2cairoConf = NULL;
static int Instance = 0;
static NHASH CompatibleFontHash = NULL;

static struct compatible_font_info CompatibleFont[] = {
  {"Times",                GRA_FONT_STYLE_NORMAL,                       FALSE, "Serif"},
  {"TimesBold",            GRA_FONT_STYLE_BOLD,                         FALSE, "Serif"},
  {"TimesItalic",          GRA_FONT_STYLE_ITALIC,                       FALSE, "Serif"},
  {"TimesBoldItalic",      GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Serif"},
  {"Helvetica",            GRA_FONT_STYLE_NORMAL,                       FALSE, "Sans-serif"},
  {"HelveticaBold",        GRA_FONT_STYLE_BOLD,                         FALSE, "Sans-serif"},
  {"HelveticaOblique",     GRA_FONT_STYLE_ITALIC,                       FALSE, "Sans-serif"},
  {"HelveticaItalic",      GRA_FONT_STYLE_ITALIC,                       FALSE, "Sans-serif"},
  {"HelveticaBoldOblique", GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Sans-serif"},
  {"HelveticaBoldItalic",  GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Sans-serif"},
  {"Courier",              GRA_FONT_STYLE_NORMAL,                       FALSE, "Monospace"},
  {"CourierBold",          GRA_FONT_STYLE_BOLD,                         FALSE, "Monospace"},
  {"CourierOblique",       GRA_FONT_STYLE_ITALIC,                       FALSE, "Monospace"},
  {"CourierItalic",        GRA_FONT_STYLE_ITALIC,                       FALSE, "Monospace"},
  {"CourierBoldOblique",   GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Monospace"},
  {"CourierBoldItalic",    GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Monospace"},
  {"Symbol",               GRA_FONT_STYLE_NORMAL,                       TRUE,  "Serif"},
  {"SymbolBold",           GRA_FONT_STYLE_BOLD,                         TRUE,  "Serif"},
  {"SymbolItalic",         GRA_FONT_STYLE_ITALIC,                       TRUE,  "Serif"},
  {"SymbolBoldItalic",     GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, TRUE,  "Serif"},
  {"Mincho",               GRA_FONT_STYLE_NORMAL,                       FALSE, "Serif"},
  {"MinchoBold",           GRA_FONT_STYLE_BOLD,                         FALSE, "Serif"},
  {"MinchoItalic",         GRA_FONT_STYLE_ITALIC,                       FALSE, "Serif"},
  {"MinchoBoldItalic",     GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Serif"},
  {"Gothic",               GRA_FONT_STYLE_NORMAL,                       FALSE, "Sans-serif"},
  {"GothicBold",           GRA_FONT_STYLE_BOLD,                         FALSE, "Sans-serif"},
  {"GothicItalic",         GRA_FONT_STYLE_ITALIC,                       FALSE, "Sans-serif"},
  {"GothicBoldItalic",     GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Sans-serif"},
  {"Tim",                  GRA_FONT_STYLE_NORMAL,                       FALSE, "Serif"},
  {"TimB",                 GRA_FONT_STYLE_BOLD,                         FALSE, "Serif"},
  {"TimI",                 GRA_FONT_STYLE_ITALIC,                       FALSE, "Serif"},
  {"TimBI",                GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Serif"},
  {"Helv",                 GRA_FONT_STYLE_NORMAL,                       FALSE, "Sans-serif"},
  {"HelvB",                GRA_FONT_STYLE_BOLD,                         FALSE, "Sans-serif"},
  {"HelvO",                GRA_FONT_STYLE_ITALIC,                       FALSE, "Sans-serif"},
  {"HelvI",                GRA_FONT_STYLE_ITALIC,                       FALSE, "Sans-serif"},
  {"HelvBO",               GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Sans-serif"},
  {"HelvBI",               GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Sans-serif"},
  {"Cour",                 GRA_FONT_STYLE_NORMAL,                       FALSE, "Monospace"},
  {"CourB",                GRA_FONT_STYLE_BOLD,                         FALSE, "Monospace"},
  {"CourO",                GRA_FONT_STYLE_ITALIC,                       FALSE, "Monospace"},
  {"CourI",                GRA_FONT_STYLE_ITALIC,                       FALSE, "Monospace"},
  {"CourBO",               GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Monospace"},
  {"CourBI",               GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Monospace"},
  {"Sym",                  GRA_FONT_STYLE_NORMAL,                       TRUE,  "Serif"},
  {"SymB",                 GRA_FONT_STYLE_BOLD,                         TRUE,  "Serif"},
  {"SymI",                 GRA_FONT_STYLE_ITALIC,                       TRUE,  "Serif"},
  {"SymBI",                GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, TRUE,  "Serif"},
  {"Min",                  GRA_FONT_STYLE_NORMAL,                       FALSE, "Serif"},
  {"MinB",                 GRA_FONT_STYLE_BOLD,                         FALSE, "Serif"},
  {"MinI",                 GRA_FONT_STYLE_ITALIC,                       FALSE, "Serif"},
  {"MinBI",                GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Serif"},
  {"Goth",                 GRA_FONT_STYLE_NORMAL,                       FALSE, "Sans-serif"},
  {"GothB",                GRA_FONT_STYLE_BOLD,                         FALSE, "Sans-serif"},
  {"GothI",                GRA_FONT_STYLE_ITALIC,                       FALSE, "Sans-serif"},
  {"GothBI",               GRA_FONT_STYLE_BOLD | GRA_FONT_STYLE_ITALIC, FALSE, "Sans-serif"},
};

#define FONT_FACE_NUM ((int) (sizeof(FontFaceStr) / sizeof(*FontFaceStr)))

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
  struct fontmap *cur, *prev;

  if (fcur == NULL)
    return;

  if (fcur->font) {
    pango_font_description_free(fcur->font);
  }

  g_free(fcur->fontalias);
  g_free(fcur->fontname);
  g_free(fcur->alternative);

  prev = NULL;
  cur = Gra2cairoConf->fontmap_list_root;
  while (cur) {
    if (cur == fcur) {
      if (prev == NULL) {
	Gra2cairoConf->fontmap_list_root = cur->next;
      } else {
	prev->next = cur->next;
      }
      break;
    }
    prev = cur;
    cur = cur->next;
  }

  g_free(fcur);
}

static void
add_font_map(struct fontmap *fmap)
{
  struct fontmap *cur;

  if (Gra2cairoConf->fontmap_list_root == NULL) {
    Gra2cairoConf->fontmap_list_root = fmap;
    return;
  }

  cur = Gra2cairoConf->fontmap_list_root;
  while (cur->next) {
    cur = cur->next;
  }
  cur->next = fmap;
}

static struct fontmap *
create_font_map(const char *fontalias, const char *fontname, const char *alternative)
{
  struct fontmap *fnew;

  if (Gra2cairoConf->fontmap == NULL)
    return NULL;

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fnew) == 0) {
    free_font_map(fnew);
  }

  fnew = g_malloc(sizeof(struct fontmap));
  if (fnew == NULL) {
    return NULL;
  }

  if (nhash_set_ptr(Gra2cairoConf->fontmap, fontalias, fnew)) {
    g_free(fnew);
    return NULL;
  }

  fnew->fontalias = g_strdup(fontalias);
  fnew->fontname = g_strdup(fontname);
  fnew->alternative = g_strdup(alternative);
  fnew->font = NULL;
  fnew->next = NULL;
  add_font_map(fnew);
  Gra2cairoConf->font_num++;

  return fnew;
}

void
gra2cairo_save_config(void)
{
  char *buf;
  struct fontmap *fcur;
  struct narray conf;

  arrayinit(&conf, sizeof(char *));

  if (gra2cairo_get_fontmap_num() == 0) {
    buf = g_strdup("font");
    if (buf) {
      arrayadd(&conf, &buf);
      removeconfig(CAIROCONF, &conf);
    }
  } else {
    fcur = Gra2cairoConf->fontmap_list_root;
    while (fcur) {
      buf = g_strdup_printf("font=%s,%s%s%s",
			    fcur->fontalias,
			    fcur->fontname,
			    (fcur->alternative) ? "," : "",
			    CHK_STR(fcur->alternative));
      if (buf) {
	arrayadd(&conf, &buf);
      }
      fcur = fcur->next;
    }
    replaceconfig(CAIROCONF, &conf);
  }

  /* for backward compatibility */
  if (OldConfigExist) {
    buf = g_strdup("font_map");
    if (buf) {
      arrayadd(&conf, &buf);
      removeconfig(CAIROCONF, &conf);
    }
    OldConfigExist = FALSE;
  }

  arraydel2(&conf);
}

static int
loadconfig(void)
{
  FILE *fp;
  char *tok, *str, *s2;
  char *f1, *f2, *f3;
  int len;
  struct fontmap *fnew;

  fp = openconfig(CAIROCONF);
  if (fp == NULL)
    return 0;

  while ((tok = getconfig(fp, &str))) {
    s2 = str;
    if (strcmp(tok, "font") == 0) {
      f1 = getitok2(&s2, &len, ",");
      f2 = getitok2(&s2, &len, ",");
      for (; (s2[0] != '\0') && (strchr(" \x09,", s2[0])); s2++);
      f3 = getitok2(&s2, &len, "");
      if (f1 && f2) {
	fnew = create_font_map(f1, f2, f3);
	g_free(f1);
	g_free(f2);
	if (fnew == NULL) {
	  g_free(tok);
	  closeconfig(fp);
	  return 1;
	}
      } else {
	g_free(f1);
	g_free(f2);
      }
      g_free(f3);
    } else if (strcmp(tok, "font_map") == 0) { /* for backward compatibility */
      char *f4, *endptr;
      int two_byte;

      OldConfigExist = TRUE;
      f1 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      f4 = getitok2(&s2, &len, " \t,");
      two_byte = strtol(f4, &endptr, 10);
      g_free(f3);
      g_free(f4);
      for (; (s2[0] != '\0') && (strchr(" \x09,", s2[0])); s2++);
      f2 = getitok2(&s2, &len, "");
      fnew = NULL;
      if (f1 && f2) {
	struct compatible_font_info *info;
	struct fontmap *tmp;

	info = gra2cairo_get_compatible_font_info(f1);

	if (info) {
	  if (nhash_get_ptr(Gra2cairoConf->fontmap, info->name, (void *) &tmp)) {
	    g_free(f1);
	    f1 = g_strdup(info->name);
	    if (f1) {
	      fnew = create_font_map(f1, f2, NULL);
	    }
	    if (fnew == NULL) {
	      g_free(tok);
	      g_free(f1);
	      g_free(f2);
	      closeconfig(fp);
	      return 1;
	    }
	  } else {
	    if (two_byte) {
	      gra2cairo_set_alternative_font(info->name, f2);
	    }
	  }
	}
      }
      g_free(f1);
      g_free(f2);
    } else {
      fprintf(stderr, "(%s): configuration '%s' in section %s is not used.\n", AppName,tok, CAIROCONF);
    }
    g_free(tok);
    g_free(str);
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
  Gra2cairoConf = g_malloc(sizeof(*Gra2cairoConf));
  if (Gra2cairoConf == NULL)
    return 1;

  Gra2cairoConf->fontmap = nhash_new();
  if (Gra2cairoConf->fontmap == NULL) {
    g_free(Gra2cairoConf);
    Gra2cairoConf = NULL;
    return 1;
  }

  Gra2cairoConf->fontmap_list_root = NULL;
  Gra2cairoConf->font_num = 0;

  if (loadconfig()) {
    free_fonts(Gra2cairoConf);
    nhash_free(Gra2cairoConf->fontmap);
    g_free(Gra2cairoConf);
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

  g_free(Gra2cairoConf);
  Gra2cairoConf = NULL;
}

struct fontmap *
gra2cairo_get_fontmap(const char *fontalias)
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
gra2cairo_remove_fontmap(const char *fontalias)
{
  struct fontmap *fnew;

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fnew) == 0) {
    free_font_map(fnew);
    nhash_del(Gra2cairoConf->fontmap, fontalias);
  }
}

void
gra2cairo_update_fontmap(const char *fontalias, const char *fontname)
{
  struct fontmap *fcur;

  if (fontname == NULL || fontalias == NULL)
    return;

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fcur))
      return;

  if (strcmp(fontname, fcur->fontname)) {
    g_free(fcur->fontname);
    fcur->fontname = g_strdup(fontname);
  }

  if (fcur->font) {
    pango_font_description_free(fcur->font);
    fcur->font = NULL;
  }
}

void
gra2cairo_add_fontmap(const char *fontalias, const char *fontname)
{
  struct fontmap *fnew;

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fnew) == 0) {
    free_font_map(fnew);
  }
  create_font_map(fontalias, fontname, NULL);
}

void
gra2cairo_set_alternative_font(const char *fontalias, const char *fontname)
{
  struct fontmap *fnew;

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fnew)) {
    return;
  }

  g_free(fnew->alternative);
  if (fontname) {
    gchar *ptr;

    ptr = g_strdup(fontname);
    if (ptr) {
      g_strstrip(ptr);
      if (ptr[0]) {
	fnew->alternative = ptr;
      } else {
	fnew->alternative = NULL;
	g_free(ptr);
      }
    }
  } else {
    fnew->alternative = NULL;
  }

  if (fnew->font) {
    pango_font_description_free(fnew->font);
    fnew->font = NULL;
  }
}

static int
gra2cairo_init(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2cairo_local *local = NULL;
  int antialias = ANTIALIAS_TYPE_DEFAULT;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;

  if (Gra2cairoConf == NULL && init_conf()) {
    goto errexit;
  }

  local = g_malloc(sizeof(struct gra2cairo_local));
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
  local->font_style = GRA_FONT_STYLE_NORMAL;
  local->symbol = FALSE;
  local->use_opacity = FALSE;
  local->force_opacity = 0;

  Instance++;

  return 0;

 errexit:
  if (Instance == 0)
    free_conf();

  g_free(local);
  return 1;
}

void
gra2cairo_draw_path(struct gra2cairo_local *local)
{
  if (local->cairo && local->linetonum) {
    double x, y;

    cairo_get_current_point(local->cairo, &x, &y);
    cairo_stroke(local->cairo);
    cairo_move_to(local->cairo, x, y);
    local->linetonum = 0;
  }
}

struct gra2cairo_local *
gra2cairo_free(struct objlist *obj, N_VALUE *inst)
{
  struct gra2cairo_local *local;

  _getobj(obj, "_local", inst, &local);

  if (local == NULL)
    return NULL;

  if (local->cairo) {
    gra2cairo_draw_path(local);
    cairo_destroy(local->cairo);
  }

  if (local->layout) {
    g_object_unref(local->layout);
  }

  if (local->fontalias) {
    g_free(local->fontalias);
  }

  Instance--;
  if (Instance == 0) {
    free_conf();
  }

  return local;
}

static int
gra2cairo_done(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  if (OldConfigExist) {		/* for backward compatibility */
    gra2cairo_save_config();
  }
  gra2cairo_free(obj, inst);

  return 0;
}

int
gra2cairo_clip_region(struct gra2cairo_local *local, cairo_region_t *region)
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
gra2cairo_get_pixel_dot(char **argv)
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
gra2cairo_set_dpi(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int dpi;
  struct gra2cairo_local *local;

  dpi = *(int *) argv[2];

  _getobj(obj, "_local", inst, &local);

  _putobj(obj, "dpix", inst, &dpi);
  _putobj(obj, "dpiy", inst, &dpi);

  local->pixel_dot_x =
    local->pixel_dot_y = gra2cairo_get_pixel_dot(argv);

  return 0;
}

static int
gra2cairo_set_dpi_x(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;

  _getobj(obj, "_local", inst, &local);

  local->pixel_dot_x = gra2cairo_get_pixel_dot(argv);

  return 0;
}

static int
gra2cairo_set_dpi_y(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;

  _getobj(obj, "_local", inst, &local);

  local->pixel_dot_y = gra2cairo_get_pixel_dot(argv);

  return 0;
}

void
set_cairo_antialias(cairo_t *cairo, int antialias)
{
  if (cairo == NULL) {
    return;
  }

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

  cairo_set_antialias(cairo, antialias);
}

void
gra2cairo_set_antialias(struct gra2cairo_local *local, int antialias)
{
  if (local->cairo == NULL)
    return;

  local->antialias = antialias;
  set_cairo_antialias(local->cairo, antialias);
}

static int
set_antialias(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int antialias;
  struct gra2cairo_local *local;

  antialias = *(int *) argv[2];

  _getobj(obj, "_local", inst, &local);

  gra2cairo_set_antialias(local, antialias);

  return 0;
}

struct compatible_font_info *
gra2cairo_get_compatible_font_info(const char *name)
{
  int i;

  if (name == NULL) {
    return NULL;
  }

  if (CompatibleFontHash == NULL) {
    return NULL;
  }

  if (nhash_get_int(CompatibleFontHash, name, &i)) {
    return NULL;
  }

  return &CompatibleFont[i];
}

int
get_font_style(struct objlist *obj, N_VALUE *inst, const char *field, const char *font_field)
{
  int style;
  struct compatible_font_info *compatible;
  char *font;

  _getobj(obj, font_field, inst, &font);
  compatible = gra2cairo_get_compatible_font_info(font);
  if (compatible && compatible->name && ! compatible->symbol) {
    g_free(font);
    font = g_strdup(compatible->name);
    _putobj(obj, font_field, inst, font);
    style = compatible->style;
  } else {
    _getobj(obj, field, inst, &style);
  }
  return style;
}

static struct fontmap *
loadfont(char *fontalias, int font_style, int *symbol)
{
  struct fontmap *fcur;
  PangoFontDescription *pfont;
  PangoStyle style;
  PangoWeight weight;

  *symbol = FALSE;

  if (nhash_get_ptr(Gra2cairoConf->fontmap, fontalias, (void *) &fcur)) {
    int i;

    if (nhash_get_int(CompatibleFontHash, fontalias, &i) == 0) {
      if (nhash_get_ptr(Gra2cairoConf->fontmap, CompatibleFont[i].name, (void *) &fcur) == 0) {
	font_style = CompatibleFont[i].style;
	*symbol = CompatibleFont[i].symbol;
      }
    }
    if (fcur == NULL && nhash_get_ptr(Gra2cairoConf->fontmap, DEFAULT_FONT, (void *) &fcur)) {
      return NULL;
    }
  }

  if (fcur->font) {
    pfont = fcur->font;
  } else {
    gchar *ptr;
    pfont = pango_font_description_new();

    ptr = g_strdup_printf("%s%s%s", fcur->fontname, (fcur->alternative) ? "," : "", CHK_STR(fcur->alternative));
    if (ptr) {
      pango_font_description_set_family(pfont, ptr);
      g_free(ptr);
    } else {
      return NULL;
    }
  }

  if (font_style > 0 && (font_style & GRA_FONT_STYLE_ITALIC)) {
    style = PANGO_STYLE_ITALIC;
  } else {
    style = PANGO_STYLE_NORMAL;
  }
  pango_font_description_set_style(pfont, style);

  if (font_style > 0 && (font_style & GRA_FONT_STYLE_BOLD)) {
    weight = PANGO_WEIGHT_BOLD;
  } else {
    weight = PANGO_WEIGHT_NORMAL;
  }
  pango_font_description_set_weight(pfont, weight);

  fcur->font = pfont;

  return fcur;
}

static void
relative_move(cairo_t *cr, double x, double y)
{
  if (cairo_has_current_point(cr)) {
    cairo_rel_move_to(cr, x, y);
  } else {
    cairo_move_to(cr, x, y);
  }
}

static void
draw_str(struct gra2cairo_local *local, int draw, char *str, struct fontmap *font, int size, int space, int *fw, int *ah, int *dh)
{
  PangoAttribute *attr;
  PangoAttrList *alist;
  PangoLayoutIter *piter;
  int w, h, baseline;

  if (size == 0 || str == NULL) {
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
    relative_move(local->cairo, x, y);

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
      relative_move(local->cairo, w * local->fontcos - x, - w * local->fontsin - y);
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
gra2cairo_flush(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;
  cairo_surface_t *surface;

  _getobj(obj, "_local", inst, &local);

  if (local->cairo == NULL)
    return -1;

  gra2cairo_draw_path(local);

  surface = cairo_get_target(local->cairo);
  if (surface) {
    cairo_surface_flush(surface);
  }


  return check_cairo_status(local->cairo);
}

char *
gra2cairo_get_utf8_str(const char *cstr, int symbol)
{
  char *tmp;
  size_t l, i, j;

  l = strlen(cstr);
  tmp = g_malloc(l * 6 + 1);
  if (tmp == NULL) {
    return NULL;
  }

  for (j = i = 0; i <= l; i++) {
    if (cstr[i] == '\\') {
      if (cstr[i + 1] == 'x' &&
	  g_ascii_isxdigit(cstr[i + 2]) &&
	  g_ascii_isxdigit(cstr[i + 3])) {
	char buf[8];
	int len, k;
	gunichar wc;

	wc = g_ascii_xdigit_value(cstr[i + 2]) * 16 + g_ascii_xdigit_value(cstr[i + 3]);
	len = g_unichar_to_utf8(wc, buf);
	for (k = 0; k < len; k++) {
	  tmp[j++] = buf[k];
	}
	i += 3;
      } else {
	i += 1;
	tmp[j++] = cstr[i];
      }
    } else {
      tmp[j++] = cstr[i];
    }
    tmp[j] = '\0';
  }

  if (symbol) {
    char *ptr;

    ptr = ascii2greece(tmp);
    if (ptr) {
      g_free(tmp);
      tmp = ptr;
    }
  }

  return tmp;
}

static int
gra2cairo_output(struct objlist *obj, N_VALUE *inst, N_VALUE *rval,
                 int argc, char **argv)
{
  char code, *cstr, *tmp;
  int *cpar, i, r, font_style;
  double x, y, w, h, fontsize,
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

  if (code != 'T') {
    gra2cairo_draw_path(local);
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
  case '%': case 'X': case 'Z':
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

    if (local->region) {
      gra2cairo_clip_region(local, local->region);
    }
    break;
  case 'A':
    if (cpar[1] == 0) {
      cairo_set_dash(local->cairo, NULL, 0, 0);
    } else {
      dashlist = g_malloc(sizeof(* dashlist) * cpar[1]);
      if (dashlist == NULL)
	break;
      for (i = 0; i < cpar[1]; i++) {
	dashlist[i] = mxd2pw(local, cpar[6 + i]);
        if (dashlist[i] <= 0) {
	  dashlist[i] = 1;
	}
      }
      cairo_set_dash(local->cairo, dashlist, cpar[1], 0);
      g_free(dashlist);
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
    if (local->force_opacity && cpar[0] > 3) {
      cairo_set_source_rgba(local->cairo,
			    cpar[1] / 255.0,
			    cpar[2] / 255.0,
			    cpar[3] / 255.0,
			    local->force_opacity);
    } else {
    if (local->use_opacity && cpar[0] > 3 && cpar[4] < 255) {
      cairo_set_source_rgba(local->cairo,
			    cpar[1] / 255.0,
			    cpar[2] / 255.0,
			    cpar[3] / 255.0,
			    cpar[4] / 255.0);
    } else {
      cairo_set_source_rgb(local->cairo,
			   cpar[1] / 255.0,
			   cpar[2] / 255.0,
			   cpar[3] / 255.0);
    }
    }
    break;
  case 'M':
    cairo_move_to(local->cairo, mxd2px(local, cpar[1]), mxd2py(local, cpar[2]));
    break;
  case 'N':
    relative_move(local->cairo, mxd2pw(local, cpar[1]), mxd2ph(local, cpar[2]));
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

    if (w == 0 || h == 0 || a1 == a2)
      break;

    cairo_new_path(local->cairo);
    cairo_save(local->cairo);
    cairo_translate(local->cairo, x + w, y + h);
    cairo_scale(local->cairo, w, h);
    cairo_arc_negative(local->cairo, 0., 0., 1., -a1, -a2);
    cairo_restore (local->cairo);
    switch (cpar[7]) {
    case 1:
      cairo_line_to(local->cairo, x + w, y + h);
      /* fall through */
    case 2:
      cairo_close_path(local->cairo);
      cairo_fill(local->cairo);
      break;
    case 3:
      cairo_line_to(local->cairo, x + w, y + h);
      /* fall through */
    case 4:
      cairo_close_path(local->cairo);
      cairo_stroke(local->cairo);
      break;
    default:
      cairo_stroke(local->cairo);
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
    cairo_close_path(local->cairo);

    switch (cpar[2]) {
    case 0:
      cairo_stroke(local->cairo);
      break;
    case 1:
      cairo_set_fill_rule(local->cairo, CAIRO_FILL_RULE_EVEN_ODD);
      cairo_fill(local->cairo);
      break;
    case 2:
      cairo_set_fill_rule(local->cairo, CAIRO_FILL_RULE_WINDING);
      cairo_fill(local->cairo);
      break;
    }
    break;
  case 'F':
    g_free(local->fontalias);
    local->fontalias = g_strdup(cstr);
    break;
  case 'H':
    fontspace = cpar[2] / 72.0 * 25.4;
    local->fontspace = fontspace;
    fontsize = cpar[1] / 72.0 * 25.4;
    local->fontsize = fontsize;
    fontdir = cpar[3] * MPI / 18000.0;
    fontsin = sin(fontdir);
    fontcos = cos(fontdir);
    local->fontdir = (cpar[3] % 36000) / 100.0;
    if (local->fontdir < 0) {
      local->fontdir += 360;
    }
    local->fontsin = fontsin;
    local->fontcos = fontcos;
    font_style = (cpar[0] > 3) ? cpar[4] : -1;
    local->loadfont = loadfont(local->fontalias, font_style, &local->symbol);
    break;
  case 'S':
    if (local->loadfont == NULL)
      break;

    tmp = gra2cairo_get_utf8_str(cstr, local->symbol);
    if (tmp) {
      draw_str(local, TRUE, tmp, local->loadfont, local->fontsize, local->fontspace, NULL, NULL, NULL);
      g_free(tmp);
    }
    break;
  case 'K':
    if (local->loadfont == NULL)
      break;

    tmp = sjis_to_utf8(cstr);
    if (tmp) {
      draw_str(local, TRUE, tmp, local->loadfont, local->fontsize, local->fontspace, NULL, NULL, NULL);
      g_free(tmp);
    }
    break;
  default:
    break;
  }
  return 0;
}


int
gra2cairo_strwidth(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;
  gchar *font, *str;
  double size, dir, s,c ;
  int width, style, symbol;
  struct fontmap *fcur;

  str = argv[3];
  size = ( * (int *) argv[4]) / 72.0 * 25.4;
  font = argv[5];
  style = * (int *) (argv[6]);

  if (size == 0) {
    rval->i = 0;
    return 0;
  }

  if (_getobj(obj, "_local", inst, &local))
    return 1;

  if (local->cairo == NULL)
    return 0;

  fcur = loadfont(font, style, &symbol);

  if (fcur == NULL) {
    rval->i = nround(size * 0.600);
    return 0;
  }

  dir = local->fontdir;
  s = local->fontsin;
  c = local->fontcos;

  local->fontdir = 0;
  local->fontsin = 0;
  local->fontcos = 1;

  if (symbol) {
    char *ptr;
    ptr = ascii2greece(str);
    if (ptr == NULL) {
      return 1;
    }
    str = ptr;
  }

  draw_str(local, FALSE, str, fcur, size, 0, &width, NULL, NULL);

  if (symbol) {
    g_free(str);
  }

  rval->i = mxp2dw(local, width);

  local->fontsin = s;
  local->fontcos = c;
  local->fontdir = dir;

  return 0;
}

int
gra2cairo_charheight(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;
  char *font;
  double size, dir, s, c;
  char *func;
  int height, descent, ascent, style, symbol;
  struct fontmap *fcur;

  func = (char *)argv[1];
  size = (*(int *)(argv[3])) / 72.0 * 25.4;
  font = (char *)(argv[4]);
  style = *(int *) (argv[5]);

  if (size == 0) {
    rval->i = 0;
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

  fcur = loadfont(font, style, &symbol);

  if (fcur == NULL) {
    if (height) {
      rval->i = nround(size * 0.562);
    } else {
      rval->i = nround(size * 0.250);
    }

    return 0;
  }

  dir = local->fontdir;
  s = local->fontsin;
  c = local->fontcos;

  local->fontdir = 0;
  local->fontsin = 0;
  local->fontcos = 1;

  draw_str(local, FALSE, "A", fcur, size, 0, NULL, &ascent, &descent);

  if (height) {
    rval->i = mxp2dh(local, ascent);
  } else {
    rval->i = mxp2dh(local, descent);
  }

  local->fontsin = s;
  local->fontcos = c;
  local->fontdir = dir;

  return 0;
}

static int
use_opacity(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
	      char **argv)
{
  struct gra2cairo_local *local;

  if (_getobj(obj, "_local", inst, &local))
    return 1;

  local->use_opacity = * (int *) argv[2];

  return 0;
}

static int
force_opacity(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc,
	      char **argv)
{
  struct gra2cairo_local *local;
  int a;

  if (_getobj(obj, "_local", inst, &local))
    return 1;

  a = * (int *) argv[2];
  if (a <= 0) {
    local->force_opacity = 0;
  } else if (a < 256) {
    local->force_opacity = a / 255.0;
  }

  return 0;
}

static struct objtable gra2cairo[] = {
  {"init", NVFUNC, 0, gra2cairo_init, NULL, 0},
  {"done", NVFUNC, 0, gra2cairo_done, NULL, 0},
  {"dpi", NINT, NREAD | NWRITE, gra2cairo_set_dpi, NULL, 0},
  {"dpix", NINT, NREAD | NWRITE, gra2cairo_set_dpi_x, NULL, 0},
  {"dpiy", NINT, NREAD | NWRITE, gra2cairo_set_dpi_y, NULL, 0},
  {"flush",NVFUNC,NREAD|NEXEC,gra2cairo_flush,"",0},
  {"antialias", NENUM, NREAD | NWRITE, set_antialias, gra2cairo_antialias_type, 0},
  {"use_opacity", NBOOL, NREAD | NWRITE, use_opacity, NULL,0},
  {"force_opacity", NINT, NREAD | NWRITE, force_opacity, NULL,0},
  {"_output", NVFUNC, 0, gra2cairo_output, NULL, 0},
  {"_strwidth", NIFUNC, 0, gra2cairo_strwidth, NULL, 0},
  {"_charascent", NIFUNC, 0, gra2cairo_charheight, NULL, 0},
  {"_chardescent", NIFUNC, 0, gra2cairo_charheight, NULL, 0},
  {"_local", NPOINTER, 0, NULL, NULL, 0},
};

#define TBLNUM (sizeof(gra2cairo) / sizeof(*gra2cairo))

void *
addgra2cairo()
/* addgra2cairoile() returns NULL on error */
{
  int i;

  if (CompatibleFontHash == NULL) {
    CompatibleFontHash = nhash_new();
    if (CompatibleFontHash == NULL) {
      return NULL;
    }
    for (i = 0; i < (int) (sizeof(CompatibleFont) / sizeof(*CompatibleFont)); i++) {
      nhash_set_int(CompatibleFontHash, CompatibleFont[i].old_name, i);
    }
  }

  if (Gra2CairoErrMsgs == NULL) {
    Gra2CairoErrMsgs = g_malloc(sizeof(*Gra2CairoErrMsgs) * CAIRO_STATUS_LAST_STATUS);
    Gra2CairoErrMsgNum = CAIRO_STATUS_LAST_STATUS;

    for (i = 0; i < CAIRO_STATUS_LAST_STATUS; i++) {
      Gra2CairoErrMsgs[i] = g_strdup(cairo_status_to_string(i));
    }
  }

  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, gra2cairo, Gra2CairoErrMsgNum, Gra2CairoErrMsgs, NULL, NULL);
}

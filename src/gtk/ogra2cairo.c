/* 
 * $Id: ogra2cairo.c,v 1.4 2008/06/26 06:25:52 hito Exp $
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

#define ERRFOPEN 100

#ifndef M_PI
#define M_PI 3.141592
#endif

char *gra2cairo_errorlist[]={
  "I/O error: open file"
};

#define ERRNUM (sizeof(gra2cairo_errorlist) / sizeof(*gra2cairo_errorlist))

static struct gra2cairo_config *Conf = NULL;
static int Instance = 0;


static int
mxp2d(struct gra2cairo_local *local, int r)
{
  return r / local->pixel_dot;
}

static double
mxd2p(struct gra2cairo_local *local, int r)
{
  return r * local->pixel_dot;
}

static double
mxd2px(struct gra2cairo_local *local, int x)
{
  return x * local->pixel_dot + local->offsetx;
}

static double
mxd2py(struct gra2cairo_local *local, int y)
{
  return y * local->pixel_dot + local->offsety;
}

static int
loadconfig(void)
{
  FILE *fp;
  char *tok, *str, *s2;
  char *f1, *f2, *f3, *f4;
  int val;
  char *endptr, symbol[] = "Sym";
  int len;
  struct fontmap *fcur, *fnew;

  fp = openconfig(CAIROCONF);
  if (fp == NULL)
    return 0;

  fcur = Conf->fontmaproot;
  while ((tok = getconfig(fp, &str))) {
    s2 = str;
    if (strcmp(tok, "font_map") == 0) {
      f1 = getitok2(&s2, &len, " \t,");
      f2 = getitok2(&s2, &len, " \t,");
      f3 = getitok2(&s2, &len, " \t,");
      for (; (s2[0] != '\0') && (strchr(" \x09,", s2[0])); s2++);
      f4 = getitok2(&s2, &len, "");
      if (f1 && f2 && f3 && f4) {
	if ((fnew = memalloc(sizeof(struct fontmap))) == NULL) {
	  memfree(tok);
	  memfree(f1);
	  memfree(f2);
	  memfree(f3);
	  memfree(f4);
	  closeconfig(fp);
	  return 1;
	}
	if (fcur == NULL) {
	  Conf->fontmaproot = fnew;
	} else {
	  fcur->next = fnew;
	}
	fcur = fnew;
	fcur->next = NULL;
	fcur->fontalias = f1;
	fcur->symbol = ! strncmp(f1, symbol, sizeof(symbol) - 1);
	if (strcmp(f2, "bold") == 0) {
	  fcur->type = BOLD;
	} else if (strcmp(f2, "italic") == 0) {
	  fcur->type = ITALIC;
	} else if (strcmp(f2, "bold_italic") == 0) {
	  fcur->type = BOLDITALIC;
	} else if (strcmp(f2, "oblique") == 0) {
	  fcur->type = OBLIQUE;
	} else if (strcmp(f2, "bold_oblique") == 0) {
	  fcur->type = BOLDOBLIQUE;
	} else {
	  fcur->type = NORMAL;
	}
	memfree(f2);
	val = strtol(f3, &endptr, 10);
	memfree(f3);
	fcur->twobyte = val;
	fcur->fontname = f4;
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
init_conf(void)
{
  Conf = malloc(sizeof(*Conf));
  if (Conf == NULL)
    return 1;

  Conf->fontmaproot = NULL;
  Conf->loadfont = 0;
  if (loadconfig()) {
    free(Conf);
    return 1;
  }

  return 0;
}

static void
free_conf(void)
{
  int i;

  if (Conf == NULL)
    return;

  for (i = 0; i < Conf->loadfont; i++) {
    if (Conf->font[i].fontalias) {
      memfree(Conf->font[i].fontalias);
      pango_font_description_free(Conf->font[i].font);
      Conf->font[i].fontalias = NULL;
      Conf->font[i].font = NULL;
    }
  }
  Conf->loadfont = 0;

  free(Conf);
  Conf = NULL;
}

static int
gra2cairo_init(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{  
  struct gra2cairo_local *local;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) return 1;

  if (Conf == NULL && init_conf()) {
    goto errexit;
  }

  local = memalloc(sizeof(struct gra2cairo_local));
  if (local == NULL)
    goto errexit;

  if (_putobj(obj, "_local", inst, local))
    goto errexit;

  local->cairo = NULL;
  local->fontalias = NULL;
  local->pixel_dot = 1;
  local->linetonum = 0;

  local->region[0] = local->region[1] =
    local->region[2] = local->region[3] = 0;

  local->region_active = FALSE;
  Instance++;

  return 0;

 errexit:
  free_conf();
  memfree(local);
  return 1;
}

static int 
gra2cairo_done(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  _getobj(obj, "_local", inst, &local);

  if (local->cairo) {
    cairo_destroy(local->cairo);
  }

  Instance--;
  if (Instance == 0) {
    free_conf();
  }

  return 0;
}

static int 
gra2cairo_set_region(struct objlist *obj, char *inst, char *rval, int argc, char **argv)
{
  struct gra2cairo_local *local;
  int i;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  _getobj(obj, "_local", inst, &local);

  if (argc == 4) {
    for (i = 0; i < 2; i++) {
      local->region[i] = mxd2px(local, *(int *) argv[i * 2 + 2]);
      local->region[i] = mxd2py(local, *(int *) argv[i * 2 + 3]);
    }
    local->region_active = TRUE;
  } else {
    for (i = 0; i < 4; i++) {
      local->region[i] = 0;
    }
    local->region_active = FALSE;
  }

  return 0;
}

static void
gra2cairo_set_dpi(struct gra2cairo_local *local, struct objlist *obj, char *inst)
{
  int dpi;

  _getobj(obj, "dpi", inst, &dpi);

  if (dpi < 1)
    dpi = 1;

  if (dpi > DPI_MAX)
    dpi = DPI_MAX;

  local->pixel_dot = dpi / (DPI_MAX * 1.0);
}

static int
loadfont(char *fontalias, int top)
{
  struct fontlocal font;
  struct fontmap *fcur;
  char *fontname;
  int twobyte = FALSE, type = NORMAL, fontcashfind, i, store, symbol = FALSE;
  PangoFontDescription *pfont;
  PangoStyle style;
  PangoWeight weight;
  static PangoLanguage *lang_ja = NULL, *lang = NULL;

  fontcashfind = -1;


  if (lang == NULL) {
    lang = pango_language_from_string("en-US");
    lang_ja = pango_language_from_string("ja-JP");
  }

  for (i = 0; i < Conf->loadfont; i++) {
    if (strcmp((Conf->font[i]).fontalias, fontalias) == 0) {
      fontcashfind=i;
      break;
    }
  }

  if (fontcashfind != -1) {
    if (top) {
      font = Conf->font[fontcashfind];
      for (i = fontcashfind - 1; i >= 0; i--) {
	Conf->font[i + 1] = Conf->font[i];
      }
      Conf->font[0] = font;
      return 0;
    } else {
      return fontcashfind;
    }
  }

  fontname = NULL;

  for (fcur = Conf->fontmaproot; fcur; fcur=fcur->next) {
    if (strcmp(fontalias, fcur->fontalias) == 0) {
      fontname = fcur->fontname;
      type = fcur->type;
      twobyte = fcur->twobyte;
      symbol = fcur->symbol;
      break;
    }
  }

  pfont = pango_font_description_new();
  pango_font_description_set_family(pfont, fontname);

  switch (type) {
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

  switch (type) {
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

  if (Conf->loadfont == CAIRO_FONTCASH) {
    i = CAIRO_FONTCASH - 1;

    memfree((Conf->font[i]).fontalias);
    Conf->font[i].fontalias = NULL;

    pango_font_description_free((Conf->font[i]).font);
    Conf->font[i].font = NULL;

    Conf->loadfont--;
  }

  if (top) {
    for (i = Conf->loadfont - 1; i >= 0; i--) {
      Conf->font[i + 1] = Conf->font[i];
    }
    store=0;
  } else {
    store = Conf->loadfont;
  }

  Conf->font[store].fontalias = nstrdup(fontalias);
  if (Conf->font[store].fontalias == NULL) {
    pango_font_description_free(pfont);
    Conf->font[store].font = NULL;
    return -1;
  }

  Conf->font[store].font = pfont;
  Conf->font[store].fonttype = type;
  Conf->font[store].symbol = symbol;

  Conf->loadfont++;

  return store;
}

static void
draw_str(struct gra2cairo_local *local, int draw, char *str, int font, int size, int space, int *fw, int *ah, int *dh)
{
  PangoLayout *layout;
  PangoAttribute *attr;
  PangoAttrList *alist;
  PangoLayoutIter *piter;
  int w, h, baseline;

  layout = pango_cairo_create_layout(local->cairo);

  alist = pango_attr_list_new();
  attr = pango_attr_size_new_absolute(mxd2p(local, size) * PANGO_SCALE);
  pango_attr_list_insert(alist, attr);

  attr = pango_attr_letter_spacing_new(mxd2p(local, space) * PANGO_SCALE);
  pango_attr_list_insert(alist, attr);

  pango_layout_set_font_description(layout, Conf->font[font].font);
  pango_layout_set_attributes(layout, alist);

  pango_layout_set_text(layout, str, -1);

  pango_layout_get_pixel_size(layout, &w, &h);
  piter = pango_layout_get_iter(layout);
  baseline = pango_layout_iter_get_baseline(piter) / PANGO_SCALE;

#if 0
  if (draw) {
    cairo_save(local->cairo);

    PangoLayoutLine *pline;
    PangoRectangle prect;
    int ascent, descent, s = mxd2p(local, size);

    pline = pango_layout_get_line_readonly(layout, 0);
    pango_layout_line_get_pixel_extents(pline, &prect, NULL);
    ascent = PANGO_ASCENT(prect);
    descent = PANGO_DESCENT(prect);

    cairo_rectangle(local->cairo, x, y  - baseline, s, s);
    cairo_stroke(local->cairo);

    //    gdk_gc_set_rgb_fg_color(Mxlocal->gc, &red);
    cairo_rectangle(local->cairo, x, y  - baseline, w, h);
    cairo_stroke(local->cairo);

    //    gdk_draw_line(Mxlocal->pix, Mxlocal->gc, x, y - baseline, x + w, y - baseline);

    //    gdk_gc_set_rgb_fg_color(Mxlocal->gc, &blue);

    cairo_rectangle(local->cairo, x + prect.x, y - ascent, prect.width, prect.height);
    cairo_stroke(local->cairo);
    //    gdk_draw_line(Mxlocal->pix, Mxlocal->gc, x + prect.x, y, x + prect.x + prect.width, y);

    //    gdk_gc_set_rgb_fg_color(Mxlocal->gc, &black);

    cairo_restore(local->cairo);
  }

#endif

  if (fw)
    *fw = w;

  if (ah)
    *ah = baseline;

  if (dh)
    *dh = h - baseline;

  if (draw && str) {
    double x, y;

    x = - local->fontsin * baseline;
    y = - local->fontcos * baseline;

    cairo_rel_move_to(local->cairo, x, y);
    cairo_save(local->cairo);
    cairo_rotate(local->cairo, -local->fontdir * G_PI / 180.);
    pango_cairo_update_layout(local->cairo, layout);
    pango_cairo_show_layout(local->cairo, layout);
    cairo_restore(local->cairo);
    cairo_rel_move_to(local->cairo, w * local->fontcos - x, - w * local->fontsin - y);
  }

  pango_layout_iter_free(piter);
  pango_attr_list_unref(alist);
  g_object_unref(layout);
}


static int 
gra2cairo_output(struct objlist *obj, char *inst, char *rval, 
                 int argc, char **argv)
{
  char code, *cstr, *tmp, *tmp2;
  int *cpar, i, j;
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

  if (local->linetonum != 0 && code != 'T') {
    cairo_stroke(local->cairo);
    local->linetonum = 0;
  }
  switch (code) {
  case 'I':
    gra2cairo_set_dpi(local, obj, inst);
    local->linetonum = 0;
    break;
  case '%': case 'X':
    break;
  case 'E':
    break;
  case 'V':
    local->offsetx = mxd2p(local, cpar[1]);
    local->offsety = mxd2p(local, cpar[2]);
    local->cpx = 0;
    local->cpy = 0;
    if (cpar[5]) {
      x = mxd2p(local, cpar[1]);
      y = mxd2p(local, cpar[2]);
      w = mxd2p(local, cpar[3]) - x;
      h = mxd2p(local, cpar[4]) - y;

      cairo_new_path(local->cairo);
      cairo_rectangle(local->cairo, x, y, w, h);

      if (local->region_active) {
	cairo_rectangle(local->cairo,
			local->region[0], local->region[1],
			local->region[2], local->region[3]);
      }

      cairo_clip(local->cairo);
    } else {
      if (local->region) {
	cairo_rectangle(local->cairo,
			local->region[0], local->region[1],
			local->region[2], local->region[3]);
      } else {
	cairo_rectangle(local->cairo, 0, 0, SHRT_MAX, SHRT_MAX);
      }
      cairo_clip(local->cairo);
    }
    break;
  case 'A':
    if (cpar[1] == 0) {
      cairo_set_dash(local->cairo, NULL, 0, 0);
    } else {
      dashlist = memalloc(sizeof(* dashlist) * cpar[1]);
      if (dashlist == NULL)
	break;
      for (i = 0; i < cpar[1]; i++) {
	dashlist[i] = mxd2p(local, cpar[6 + i]);
        if (dashlist[i] <= 0) {
	  dashlist[i] = 1;
	}
      }
      cairo_set_dash(local->cairo, dashlist, cpar[1], 0);
      memfree(dashlist);
    }

    cairo_set_line_width(local->cairo, mxd2p(local, cpar[2]));

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
    cairo_move_to(local->cairo, mxd2px(local, cpar[1]), mxd2px(local, cpar[2]));
    break;
  case 'N':
    cairo_rel_move_to(local->cairo, mxd2px(local, cpar[1]), mxd2px(local, cpar[2]));
    break;
  case 'L':
    cairo_new_path(local->cairo);
    cairo_move_to(local->cairo, mxd2px(local, cpar[1]), mxd2px(local, cpar[2]));
    cairo_line_to(local->cairo, mxd2px(local, cpar[3]), mxd2px(local, cpar[4]));
    cairo_stroke(local->cairo);
    break;
  case 'T':
    cairo_line_to(local->cairo, mxd2px(local, cpar[1]), mxd2px(local, cpar[2]));
    local->linetonum++;
    break;
  case 'C':
    cairo_new_path(local->cairo);
    x = mxd2px(local, cpar[1] - cpar[3]);
    y = mxd2py(local, cpar[2] - cpar[4]);
    w = mxd2p(local, cpar[3]);
    h = mxd2p(local, cpar[4]);
    a1 = cpar[5] * (M_PI / 18000.0);
    a2 = cpar[6] * (M_PI / 18000.0) + a1;

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
      w = mxd2p(local, cpar[3] - cpar[1]);
    } else {
      x = mxd2px(local, cpar[3]);
      w = mxd2p(local, cpar[1] - cpar[3]);
    }

    if (cpar[2] <= cpar[4]) {
      y = mxd2py(local, cpar[2]);
      h = mxd2p(local, cpar[4] - cpar[2]);
    } else {
      y = mxd2py(local, cpar[4]);
      h = mxd2p(local, cpar[2] - cpar[4]);
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
    cairo_arc(local->cairo, mxd2px(local, cpar[1]), mxd2py(local, cpar[2]), mxd2p(local, 10), 0, 2 * M_PI);
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
    fontcashsize = mxd2p(local, fontsize);
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

    local->loadfontf = loadfont(local->fontalias, TRUE);
    break;
  case 'S':
    if (local->loadfontf == -1)
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

    if (Conf->font[0].symbol) {
      char *ptr;

      ptr = ascii2greece(tmp2);
      if (ptr) {
	free(tmp2);
	tmp2 = ptr;
      }
    }
    draw_str(local, TRUE, tmp2, 0, local->fontsize, local->fontspace, NULL, NULL, NULL);
    free(tmp2);
    free(tmp);
    break;
  case 'K':
    tmp2 = sjis_to_utf8(cstr);
    if (tmp2 == NULL) 
      break;
    draw_str(local, TRUE, tmp2, 0, local->fontsize, local->fontspace, NULL, NULL, NULL);
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
  int cashpos, width;;
  //  XFontStruct *fontstruct;

  ch[0] = (*(unsigned int *)(argv[3]) & 0xff);
  ch[1] = (*(unsigned int *)(argv[3]) & 0xff00) >> 8;
  ch[2] = '\0';

  size = (*(int *)(argv[4])) / 72.0 * 25.4;
  font = (char *)(argv[5]);

  if (_getobj(obj, "_local", inst, &local))
    return 1;

  cashpos = loadfont(font, FALSE);

  if (cashpos == -1) {
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

  draw_str(local, FALSE, tmp, cashpos, size, 0, &width, NULL, NULL);
  *(int *) rval = mxp2d(local, width);
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
  int height, descent, ascent, cashpos;
  //  XFontStruct *fontstruct;
  struct fontmap *fcur;
  int twobyte;

  func = (char *)argv[1];
  size = (*(int *)(argv[3])) / 72.0 * 25.4;
  font = (char *)(argv[4]);

  if (_getobj(obj, "_local", inst, &local))
    return 1;

  if (strcmp0(func, "_charascent") == 0) {
    height = TRUE;
  } else {
    height = FALSE;
  }

  fcur = Conf->fontmaproot;
  twobyte = FALSE;

  while (fcur) {
    if (strcmp(font, fcur->fontalias) == 0) {
      twobyte = fcur->twobyte;
      break;
    }
    fcur = fcur->next;
  }

  cashpos = loadfont(font, FALSE);

  if (cashpos < 0) {
    if (height) {
      *(int *) rval = nround(size * 0.562);
    } else {
      *(int *) rval = nround(size * 0.250);
    }
  }

  dir = local->fontdir;
  s = local->fontsin;
  c = local->fontcos;

  local->fontdir = 0;
  local->fontsin = 0;
  local->fontcos = 1;

  draw_str(local, FALSE, "A", cashpos, size, 0, NULL, &ascent, &descent);

  if (height) {
    *(int *)rval = mxp2d(local, ascent);
  } else {
    *(int *)rval = mxp2d(local, descent);
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
	*(int *) rval = mxp2d(fontstruct->ascent);
      } else {
	*(int *) rval = mxp2d(fontstruct->descent);
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
  {"next", NPOINTER, 0, NULL, NULL, 0}, 
  {"dpi", NINT, NREAD | NWRITE, NULL, NULL, 0},
  {"region", NVFUNC, NEXEC, gra2cairo_set_region, NULL, 0}, 
  {"_output", NVFUNC, 0, gra2cairo_output, NULL, 0}, 
  {"_local", NPOINTER, 0, NULL, NULL, 0}, 
};

#define TBLNUM (sizeof(gra2cairo) / sizeof(*gra2cairo))

void *
addgra2cairo()
/* addgra2cairoile() returns NULL on error */
{
  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, gra2cairo, ERRNUM, gra2cairo_errorlist, NULL, NULL);
}

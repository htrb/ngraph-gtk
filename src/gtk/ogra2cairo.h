#ifndef _O_GRA2CAIRO_HEADER
#define _O_GRA2CAIRO_HEADER

#include "object.h"

#define CAIRO_FONTCASH 60		/* must be greater than 1 */

extern struct gra2cairo_config *Gra2cairoConf;
extern char *gra2cairo_antialias_type[];

enum antialias_type_id {
  ANTIALIAS_TYPE_NONE,
  ANTIALIAS_TYPE_DEFAULT,
  ANTIALIAS_TYPE_GRAY,
  ANTIALIAS_TYPE_SUBPIXEL,
};

enum cairo_font_type {NORMAL = 0, BOLD, ITALIC, BOLDITALIC, OBLIQUE, BOLDOBLIQUE };
struct fontmap
{
  char *fontalias;
  char *fontname;
  int type, twobyte, symbol;
  struct fontmap *next;
};

struct fontlocal
{
  char *fontalias;
  PangoFontDescription *font;
  int fontsize, fonttype, fontdir, symbol;
};

struct gra2cairo_config {
  struct fontmap *fontmaproot;
  int loadfont;
  char *fontalias;
  struct fontlocal font[CAIRO_FONTCASH];
};

struct gra2cairo_local {
  cairo_t *cairo;
  cairo_font_options_t *font_opt;
  int linetonum, region_active, loadfontf, text2path, antialias;
  char *fontalias;
  double pixel_dot_x,  pixel_dot_y, offsetx, offsety, region[4],
    fontdir, fontcos, fontsin, fontspace, fontsize;
};

int gra2cairo_clip_region(struct gra2cairo_local *local, GdkRegion *region);
int gra2cairo_charwidth(struct objlist *obj, char *inst, char *rval, int argc, char **argv);
int gra2cairo_charheight(struct objlist *obj, char *inst, char *rval, int argc, char **argv);
int gra2cairo_set_region(struct gra2cairo_local *local, int x1, int y1, int x2, int y2);
int gra2cairo_clear_region(struct gra2cairo_local *local);
void gra2cairo_set_antialias(struct gra2cairo_local *local, int antialias);
struct gra2cairo_local *gra2cairo_free(struct objlist *obj, char *inst);

#endif

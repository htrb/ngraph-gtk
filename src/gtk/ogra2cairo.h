#ifndef _O_GRA2CAIRO_HEADER
#define _O_GRA2CAIRO_HEADER

#include "object.h"
#include "nhash.h"

extern struct gra2cairo_config *Gra2cairoConf;
extern char *gra2cairo_antialias_type[], **Gra2CairoErrMsgs;
extern int Gra2CairoErrMsgNum;

enum antialias_type_id {
  ANTIALIAS_TYPE_NONE,
  ANTIALIAS_TYPE_DEFAULT,
  ANTIALIAS_TYPE_GRAY,
  ANTIALIAS_TYPE_SUBPIXEL,
};

struct fontmap
{
  char *fontalias, *fontname, *alternative;
  PangoFontDescription *font;
  struct fontmap *next;
};

struct compatible_font_info {
  char *old_name;
  int style;
  int symbol;
  char *name;
};

struct gra2cairo_config {
  NHASH fontmap;
  struct fontmap *fontmap_list_root;
  int font_num;
};

struct gra2cairo_local {
  cairo_t *cairo;
  PangoLayout *layout;
  int linetonum, text2path, antialias, use_opacity;
  struct fontmap *loadfont;
  char *fontalias;
  int font_style, symbol;
  double pixel_dot_x,  pixel_dot_y, offsetx, offsety,
    fontdir, fontcos, fontsin, fontspace, fontsize;
  GdkRegion *region;
};

int gra2cairo_clip_region(struct gra2cairo_local *local, GdkRegion *region);
int gra2cairo_strwidth(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int gra2cairo_charheight(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int gra2cairo_set_region(struct gra2cairo_local *local, int x1, int y1, int x2, int y2);
int gra2cairo_clear_region(struct gra2cairo_local *local);
void gra2cairo_set_antialias(struct gra2cairo_local *local, int antialias);
struct gra2cairo_local *gra2cairo_free(struct objlist *obj, N_VALUE *inst);
void gra2cairo_update_fontmap(const char *fontalias, const char *fontname);
const char *gra2cairo_get_font_type_str(int type);
struct fontmap *gra2cairo_get_fontmap(char *font_alias);
void gra2cairo_remove_fontmap(char *fontalias);
void gra2cairo_add_fontmap(const char *fontalias, const char *fontname);
int gra2cairo_get_fontmap_num(void);
void gra2cairo_save_config(void);
void gra2cairo_draw_path(struct gra2cairo_local *local);
struct compatible_font_info *gra2cairo_get_compatible_font_info(char *name);
void gra2cairo_set_alternative_font(const char *fontalias, const char *fontname);

#endif

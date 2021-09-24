#ifndef GTK_PRESETTINGS_HEADER
#define GTK_PRESETTINGS_HEADER

#include "object.h"
#include "odraw.h"
#include "x11menu.h"

struct presettings
{
  int line_width, line_style;
  int type, interpolation;
  int mark_type_begin, mark_type_end;
  int mark_type, mark_size;
  enum JOIN_TYPE join;
  enum MARKER_TYPE marker_begin, marker_end;
  int fill, stroke, close_path;
  int r1, g1, b1, a1, r2, g2, b2, a2;
};

GtkWidget *presetting_create_panel(GtkApplication *app);
void presetting_set_obj_field(struct objlist *obj, int id);
void presetting_set_visibility(enum PointerType type);
void presetting_set_parameters(struct Viewer *d);
void presetting_show_focused(void);
void presetting_set_fonts(void);
void presetting_get(struct presettings *setting);

#endif	/* GTK_PRESETTINGS_HEADER */

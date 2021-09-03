#ifndef GTK_PRESETTINGS_HEADER
#define GTK_PRESETTINGS_HEADER

#include "object.h"
#include "x11menu.h"

GtkWidget *presetting_create_panel(GtkApplication *app);
void presetting_set_obj_field(struct objlist *obj, int id);
void presetting_set_visibility(enum PointerType type);
void presetting_set_parameters(struct Viewer *d);
void presetting_show_focused(void);
void presetting_set_fonts(void);

#endif	/* GTK_PRESETTINGS_HEADER */

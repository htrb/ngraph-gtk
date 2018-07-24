#ifndef GTK_PRESETTINGS_HEADER
#define GTK_PRESETTINGS_HEADER

#include "object.h"

GtkWidget *add_setting_panel(GtkApplication *app);
void presetting_set_obj_field(struct objlist *obj, int id);

#endif	/* GTK_PRESETTINGS_HEADER */

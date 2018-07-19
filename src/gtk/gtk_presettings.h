#ifndef GTK_PRESETTINGS_HEADER
#define GTK_PRESETTINGS_HEADER

#include "object.h"

void add_setting_panel(GtkWidget *vbox, GtkApplication *app);
void presetting_set_obj_field(struct objlist *obj, int id);

#endif	/* GTK_PRESETTINGS_HEADER */

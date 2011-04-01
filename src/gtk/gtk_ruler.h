#ifndef GTK_RULER_HEADER
#define GTK_RULER_HEADER

#include <gtk/gtk.h>

GtkWidget *hruler_new(void);
GtkWidget *vruler_new(void);
void nruler_set_range(GtkWidget *ruler, double lower, double upper);
void nruler_set_position(GtkWidget *ruler, double position);

#endif /* GTK_RULER_HEADER */

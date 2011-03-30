#ifndef GTK_RULER_HEADER
#define GTK_RULER_HEADER

#include <gtk/gtk.h>

typedef struct _Nruler {
  GtkWidget *widget;
  int orientation;
  GdkPixmap *backing_store;
  double lower, upper, position;
  double save_l, save_u;
} Nruler;

Nruler *hruler_new(void);
Nruler *vruler_new(void);
void nruler_set_range(Nruler *ruler, double lower, double upper);
void nruler_set_position(Nruler *ruler, double position);

#endif /* GTK_RULER_HEADER */

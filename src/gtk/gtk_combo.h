/*
 * $Id: gtk_combo.h,v 1.1.1.1 2008-05-29 09:37:33 hito Exp $
 */

#ifndef GTKCOMBO_HEADER
#define GTKCOMBO_HEADER

GtkWidget *combo_box_create(void);
GtkWidget *combo_box_entry_create(void);
void combo_box_entry_set_width(GtkWidget *cbox, int width);
void combo_box_entry_set_text(GtkWidget *cbox, char *str);
const char *combo_box_entry_get_text(GtkWidget *cbox);
int combo_box_get_active(GtkWidget *cbox);
char *combo_box_get_active_text(GtkWidget *cbox);
void combo_box_set_active(GtkWidget *cbox, int i);
void combo_box_clear(GtkWidget *cbox);
int combo_box_get_num(GtkWidget *cbox);
void combo_box_append_text(GtkWidget *cbox, const char *str);
int combo_box_get_selected_row(GtkWidget *view, gchar *path, GtkTreeIter *iter, int col);

#endif

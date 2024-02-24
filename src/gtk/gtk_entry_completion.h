/*
 * $Id: gtk_entry_completion.h,v 1.1.1.1 2008-05-29 09:37:33 hito Exp $
 */

void entry_completion_set_entry(GtkTreeModel *model, GtkWidget *entry);
int  entry_completion_save(GtkTreeModel *model, char *file, int size);
GtkTreeModel *entry_completion_load(char *file, int size);
void entry_completion_append(GtkTreeModel *model, const char *str);


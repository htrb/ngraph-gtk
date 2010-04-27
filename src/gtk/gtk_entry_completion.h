/* 
 * $Id: gtk_entry_completion.h,v 1.1.1.1 2008-05-29 09:37:33 hito Exp $
 */

GtkEntryCompletion *entry_completion_create(void);
void entry_completion_set_entry(GtkEntryCompletion *comp, GtkWidget *entry);
int  entry_completion_save(GtkEntryCompletion *comp, char *file, int size);
int  entry_completion_load(GtkEntryCompletion *comp, char *file, int size);
void entry_completion_append(GtkEntryCompletion *comp, const char *str);


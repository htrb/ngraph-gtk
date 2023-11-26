#ifndef GTK_COLUMNVIEW_HEADER
#define GTK_COLUMNVIEW_HEADER

#include "gtk_common.h"

#define NGRAPH_TYPE_INST (ngraph_inst_get_type())
G_DECLARE_FINAL_TYPE (NgraphInst, ngraph_inst, NGRAPH, INST, GObject)

typedef struct _NgraphInst NgraphInst;

struct _NgraphInst {
  GObject parent_instance;
  gchar *name;
  struct objlist *obj;
  int id;
  double x, y;
};

NgraphInst *ngraph_inst_new (const gchar *name, int id, struct objlist *obj);
GtkWidget *columnview_create(gboolean multi);
void columnview_clear(GtkWidget *columnview);
void columnview_create_column(GtkWidget *columnview, const char *header, GCallback setup, GCallback bind, GCallback sort, gpointer user_data);
void columnview_set_active(GtkWidget *columnview, int active);
int columnview_get_active(GtkWidget *columnview);
NgraphInst *columnview_get_active_item(GtkWidget *columnview);
GListStore *columnview_get_list(GtkWidget *columnview);
NgraphInst *list_store_append_ngraph_inst(GListStore *store, const gchar *name, int id, struct objlist *obj);
NgraphInst *columnview_append_ngraph_inst(GtkWidget *columnview, const gchar *name, int id, struct objlist *obj);
void columnview_select_all(GtkWidget *columnview);
void columnview_unselect_all(GtkWidget *columnview);
void columnview_select(GtkWidget *columnview, int i);

#endif

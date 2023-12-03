#ifndef GTK_COLUMNVIEW_HEADER
#define GTK_COLUMNVIEW_HEADER

#include "gtk_common.h"
#include "object.h"

enum N_SELECTION_TYPE {
  N_SELECTION_TYPE_SINGLE,
  N_SELECTION_TYPE_MULTI,
  N_SELECTION_TYPE_NONE,
};

/* NgraphInst Object */
#define NGRAPH_TYPE_INST (ngraph_inst_get_type())
G_DECLARE_FINAL_TYPE (NgraphInst, ngraph_inst, NGRAPH, INST, GObject)

typedef struct _NgraphInst NgraphInst;

struct _NgraphInst {
  GObject parent_instance;
  gchar *name;
  struct objlist *obj;
  int id;
};

NgraphInst *ngraph_inst_new (const gchar *name, int id, struct objlist *obj);


/* NgraphData Object */
#define NGRAPH_TYPE_DATA (ngraph_data_get_type())
G_DECLARE_FINAL_TYPE (NgraphData, ngraph_data, NGRAPH, DATA, GObject)

typedef struct _NgraphData NgraphData;

struct _NgraphData {
  GObject parent_instance;
  int line;
  double x, y;
};

NgraphData *ngraph_data_new (int line, double x, double y);


/* NgraphArray Object */
#define NGRAPH_TYPE_ARRAY (ngraph_array_get_type())
G_DECLARE_FINAL_TYPE (NgraphArray, ngraph_array, NGRAPH, ARRAY, GObject)

typedef struct _NgraphArray NgraphArray;

struct _NgraphArray {
  GObject parent_instance;
  int line;
  struct narray *array;
};

NgraphArray *ngraph_array_new (int line);


GtkWidget *columnview_create(GType item_type, enum N_SELECTION_TYPE type);
void columnview_clear(GtkWidget *columnview);
GtkColumnViewColumn *columnview_create_column(GtkWidget *columnview, const char *header, GCallback setup, GCallback bind, GCallback sort, gpointer user_data, gboolean expand);
void columnview_set_active(GtkWidget *columnview, int active);
int columnview_get_active(GtkWidget *columnview);
GObject *columnview_get_active_item(GtkWidget *columnview);
GListStore *columnview_get_list(GtkWidget *columnview);
NgraphInst *list_store_append_ngraph_inst(GListStore *store, const gchar *name, int id, struct objlist *obj);
NgraphInst *columnview_append_ngraph_inst(GtkWidget *columnview, const gchar *name, int id, struct objlist *obj);
NgraphData *list_store_append_ngraph_data(GListStore *store, int line, double x, double y);
void columnview_select_all(GtkWidget *columnview);
void columnview_unselect_all(GtkWidget *columnview);
void columnview_select(GtkWidget *columnview, int i);
void columnview_remove_selected(GtkWidget *columnview);
GtkSelectionModel *selection_model_create(enum N_SELECTION_TYPE selection_type, GListModel *model);

#endif

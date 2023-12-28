#ifndef GTK_COLUMNVIEW_HEADER
#define GTK_COLUMNVIEW_HEADER

#include "gtk_common.h"
#include "object.h"

enum N_SELECTION_TYPE {
  N_SELECTION_TYPE_SINGLE,
  N_SELECTION_TYPE_MULTI,
  N_SELECTION_TYPE_NONE,
};

/* NInst Object */
#define N_TYPE_INST (n_inst_get_type())
G_DECLARE_FINAL_TYPE (NInst, n_inst, N, INST, GObject)

typedef struct _NInst NInst;

struct _NInst {
  GObject parent_instance;
  gchar *name;
  struct objlist *obj;
  int id;
};

NInst *n_inst_new (const gchar *name, int id, struct objlist *obj);
void n_inst_update (NInst *inst);


/* NData Object */
#define N_TYPE_DATA (n_data_get_type())
G_DECLARE_FINAL_TYPE (NData, n_data, N, DATA, GObject)

typedef struct _NData NData;

struct _NData {
  GObject parent_instance;
  int id, line, data;
  double x, y;
};

NData *n_data_new (int id, int line, double x, double y);

/* NPoint Object */
#define N_TYPE_POINT (n_point_get_type())
G_DECLARE_FINAL_TYPE (NPoint, n_point, N, POINT, GObject)

typedef struct _NPoint NPoint;

struct _NPoint {
  GObject parent_instance;
  int x, y;
};

NPoint *n_point_new (int x, int y);


/* NArray Object */
#define N_TYPE_ARRAY (n_array_get_type())
G_DECLARE_FINAL_TYPE (NArray, n_array, N, ARRAY, GObject)

typedef struct _NArray NArray;

struct _NArray {
  GObject parent_instance;
  int line;
  struct narray *array;
};

NArray *n_array_new (int line);

/* NText Object */
#define N_TYPE_TEXT (n_text_get_type())
G_DECLARE_FINAL_TYPE (NText, n_text, N, TEXT, GObject)

typedef struct _NText NText;

struct _NText {
  GObject parent_instance;
  gchar **text;
  guint size, attribute;
};

NText *n_text_new (gchar **text, guint attribute);
void n_text_set_text (NText *self, char **text, guint attribute);
const char *n_text_get_string (NText *self, guint index);


GtkWidget *columnview_create(GType item_type, enum N_SELECTION_TYPE type);
GtkWidget *columnview_tree_create(GListModel *root, GtkTreeListModelCreateModelFunc create_func, gpointer user_data);
void columnview_clear(GtkWidget *columnview);
GtkColumnViewColumn *columnview_create_column(GtkWidget *columnview, const char *header, GCallback setup, GCallback bind, GCallback sort, gpointer user_data, gboolean expand);
void columnview_set_active(GtkWidget *columnview, int active, gboolean scroll);
int columnview_get_active(GtkWidget *columnview);
GObject *columnview_get_active_item(GtkWidget *columnview);
GListStore *columnview_get_list(GtkWidget *columnview);
NInst *list_store_append_n_inst(GListStore *store, const gchar *name, int id, struct objlist *obj);
NInst *columnview_append_n_inst(GtkWidget *columnview, const gchar *name, int id, struct objlist *obj);
NData *list_store_append_n_data(GListStore *store, int id, int line, double x, double y);
NPoint *list_store_append_n_point(GListStore *store, int x, int y);
NText *list_store_append_n_text(GListStore *store, gchar **test, guint attribute);
void columnview_select_all(GtkWidget *columnview);
void columnview_unselect_all(GtkWidget *columnview);
void columnview_select(GtkWidget *columnview, int i);
void columnview_remove_selected(GtkWidget *columnview);
GtkSelectionModel *selection_model_create(enum N_SELECTION_TYPE selection_type, GListModel *model);
void columnview_set_numeric_sorter(GtkColumnViewColumn *column, GType type, GCallback sort, gpointer user_data);
int columnview_get_n_items(GtkWidget *view);
int selection_model_get_selected(GtkSelectionModel *model);

#endif

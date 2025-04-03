#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "addin_common.h"

#define NAME    "Fitrslt"
#define VERSION "1.00.04"

#define POS_X    50.00
#define POS_Y    50.00

#define POS_INC   1.00
#define POS_MIN   -1000
#define POS_MAX    1000

#define ACCURACY     7
#define DIVISION   100

#define ADD_PLUS FALSE
#define EXPAND    TRUE
#define FRAME     TRUE

#define LINE_BUF_SIZE 8
#define PRM_NUM       10

enum {
  COLUMN_CHECK,
  COLUMN_PRM,
  COLUMN_CAPTION,
  COLUMN_VAL,
};

struct fit_data {
  int file_id;
  char *file;
  int id;
  char *type;
  int poly;
  char *userfunc;
  double prm[PRM_NUM];
};

struct fit_prm {
  GtkWidget *window, *x,*y, *add_plus, *accuracy, *expand, *frame, *shadow, *combo;
  struct font_prm font;
  GtkWidget *caption;
  const char *script;
  struct fit_data *data;
  int posx, posy, fit_num, dark_theme;
};

static GMainLoop *MainLoop;

/* Fitrslt Object */
#define N_TYPE_FITRSLT (n_fitrslt_get_type())
G_DECLARE_FINAL_TYPE (NFitrslt, n_fitrslt, N, FITRSLT, GObject)

typedef struct _NFitrslt NFitrslt;

struct _NFitrslt {
  GObject parent_instance;
  gboolean active;
  gchar *prm, *caption, *result;
};

G_DEFINE_TYPE(NFitrslt, n_fitrslt, G_TYPE_OBJECT)

enum {
  FITRSLT_PROP_ACTIVE = 1,
  FITRSLT_PROP_PRM,
  FITRSLT_PROP_CAPTION,
  FITRSLT_PROP_RESULT
};

static void
n_fitrslt_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  NFitrslt *self = N_FITRSLT (object);
  switch (prop_id) {
  case FITRSLT_PROP_ACTIVE:
    self->active = g_value_get_boolean (value);
    break;
  case FITRSLT_PROP_PRM:
    g_clear_pointer (&self->prm, g_free);
    self->prm = g_strdup(g_value_get_string (value));
    break;
  case FITRSLT_PROP_CAPTION:
    g_clear_pointer (&self->caption, g_free);
    self->caption = g_strdup(g_value_get_string (value));
    break;
  case FITRSLT_PROP_RESULT:
    g_clear_pointer (&self->result, g_free);
    self->result = g_strdup(g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
n_fitrslt_get_property (GObject    *object,
			guint       property_id,
			GValue     *value,
			GParamSpec *pspec)
{
  NFitrslt *self = N_FITRSLT (object);

  switch (property_id) {
  case FITRSLT_PROP_ACTIVE:
    g_value_set_boolean (value, self->active);
    break;
  case FITRSLT_PROP_PRM:
    g_value_set_string (value, self->prm);
    break;
  case FITRSLT_PROP_CAPTION:
    g_value_set_string (value, self->caption);
    break;
  case FITRSLT_PROP_RESULT:
    g_value_set_string (value, self->result);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
n_fitrslt_finalize (GObject *object)
{
  NFitrslt *self = N_FITRSLT (object);

  g_clear_pointer (&self->prm, g_free);
  g_clear_pointer (&self->caption, g_free);
  g_clear_pointer (&self->result, g_free);
  G_OBJECT_CLASS (n_fitrslt_parent_class)->finalize (object);
}

static void
n_fitrslt_class_init (NFitrsltClass * klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = n_fitrslt_finalize;
  object_class->get_property = n_fitrslt_get_property;
  object_class->set_property = n_fitrslt_set_property;

  pspec = g_param_spec_boolean ("active", NULL, NULL, TRUE, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, FITRSLT_PROP_ACTIVE, pspec);

  pspec = g_param_spec_string ("prm", NULL, NULL, NULL, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, FITRSLT_PROP_PRM, pspec);

  pspec = g_param_spec_string ("caption", NULL, NULL, NULL, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, FITRSLT_PROP_CAPTION, pspec);

  pspec = g_param_spec_string ("result", NULL, NULL, NULL, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, FITRSLT_PROP_RESULT, pspec);
}

static void
n_fitrslt_init (NFitrslt * noop)
{
}

static NFitrslt *
n_fitrslt_new (void)
{
  NFitrslt *nobj;

  nobj = g_object_new (N_TYPE_FITRSLT, NULL);
  nobj->active = TRUE;
  nobj->prm = g_strdup ("");
  nobj->caption = g_strdup ("");
  nobj->result = g_strdup ("");

  return nobj;
}

static void
editable_label_changed (GtkEditable* self, GtkListItem *list_item)
{
  const char *str, *label;
  GObject *item;
  NFitrslt *fitrslt;

  item = gtk_list_item_get_item (list_item);
  fitrslt = N_FITRSLT (item);

  label = gtk_list_item_get_accessible_label (list_item);
  str = gtk_editable_get_text (self);
  if (g_strcmp0 (label, "caption") == 0) {
    g_clear_pointer (&fitrslt->caption, g_free);
    fitrslt->caption = g_strdup(str);
  } else if (g_strcmp0 (label, "result") == 0) {
    g_clear_pointer (&fitrslt->result, g_free);
    fitrslt->result = g_strdup(str);
  }
}

static void
setup_column (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
  const char *id;
  id = user_data;
  if (g_strcmp0 (id, "active") == 0) {
    GtkWidget *btn = gtk_check_button_new ();
    gtk_list_item_set_child (list_item, btn);
  } else if (g_strcmp0 (id, "caption") == 0 || g_strcmp0 (id, "result") == 0) {
    GtkWidget *label = gtk_editable_label_new ("");
    gtk_list_item_set_child (list_item, label);
    g_signal_connect (label, "changed", G_CALLBACK (editable_label_changed), list_item);
  } else {
    GtkWidget *label = gtk_label_new (NULL);
    gtk_label_set_xalign (GTK_LABEL (label), 1.0);
    gtk_list_item_set_child (list_item, label);
  }
  gtk_list_item_set_accessible_label (list_item, id);
}

static void
bind_column (GtkSignalListItemFactory *factory, GtkListItem *list_item, const char *prop)
{
  GtkWidget *w = gtk_list_item_get_child (list_item);
  GObject *item = gtk_list_item_get_item (list_item);
  if (g_strcmp0 (prop, "active") == 0) {
    g_object_bind_property (item, prop, w, prop, G_BINDING_BIDIRECTIONAL);
  } else if (g_strcmp0 (prop, "caption") == 0 || g_strcmp0 (prop, "result") == 0) {
    g_object_bind_property (item, prop, w, "text", G_BINDING_SYNC_CREATE);
  } else {
    g_object_bind_property (item, prop, w, "label", G_BINDING_SYNC_CREATE);
  }
}

static GtkWidget *
columnview_column_create(void)
{
  GtkWidget *w;
  static GtkColumnViewColumn *column;

  w = columnview_create(N_TYPE_FITRSLT);
  columnview_create_column(w, "", G_CALLBACK(setup_column), G_CALLBACK(bind_column), "active");
  columnview_create_column(w, "prm", G_CALLBACK(setup_column), G_CALLBACK(bind_column), "prm");

  column = columnview_create_column(w, "caption", G_CALLBACK(setup_column), G_CALLBACK(bind_column), "caption");
  gtk_column_view_column_set_expand(column, TRUE);

  column = columnview_create_column(w, "result", G_CALLBACK(setup_column), G_CALLBACK(bind_column), "result");
  gtk_column_view_column_set_expand(column, TRUE);
  return w;
}

static int
loaddatalist(struct fit_prm *prm, const char *datalist)
{
  FILE *f;
  int i, j, fitnum;

  if (datalist == NULL) {
    return 1;
  }

  f = g_fopen(datalist, "r");
  if (f == NULL) {
    return 1;
  }

  fitnum = fgets_int(f);
  if (fitnum < 1) {
    fclose(f);
    return 1;
  }

  prm->data = g_malloc(sizeof(*prm->data) * fitnum);
  prm->fit_num = fitnum;
  for (i = 0; i < fitnum; i++) {
    char *source, *file, *array;
    prm->data[i].file_id = fgets_int(f);

    source = fgets_str(f);
    file = fgets_str(f);
    array = fgets_str(f);

    prm->data[i].id = fgets_int(f);
    prm->data[i].type = fgets_str(f);
    prm->data[i].poly = fgets_int(f);
    prm->data[i].userfunc = fgets_str(f);
    for (j = 0; j < PRM_NUM; j++) {
      prm->data[i].prm[j] = fgets_double(f);
    }
    if (g_strcmp0(source, "array") == 0) {
      prm->data[i].file = array;
    } else {
      prm->data[i].file = file;
    }
  }
  fclose(f);

  return 0;
}

static void
makescript(FILE *f, struct fit_prm *prm, int gx, int gy, int height, const char *cap, const char *val)
{
  int style, textpt, textspc, textsc, textred, textblue, textgreen, frame;
  const char *font;

  get_font_parameter(&prm->font, &textpt, &textspc, &textsc, &style, &textred, &textblue, &textgreen);
  font = get_selected_font(&prm->font);

  frame = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->frame));

  fprintf(f, "new text\n");
  fprintf(f, "text::text='%s %s'\n", cap, val);
  fprintf(f, "text::x=%d\n", gx);
  fprintf(f, "text::y=%d\n", gy + height);
  fprintf(f, "text::pt=%d\n", textpt);
  fprintf(f, "text::font=%s\n", font);
  fprintf(f, "text::style=%d\n", style);
  fprintf(f, "text::space=%d\n", textspc);
  fprintf(f, "text::script_size=%d\n", textsc);
  fprintf(f, "text::R=%d\n", textred);
  fprintf(f, "text::G=%d\n", textgreen);
  fprintf(f, "text::B=%d\n", textblue);
  if (frame) {
    fprintf(f, "iarray:textbbox:@=${text::bbox}\n");
    fprintf(f, "iarray:textlen:push \"${iarray:textbbox:get:2}-${iarray:textbbox:get:0}\"\n");
  }
  fprintf(f, "menu::modified=true\n");
}

static int
savescript(struct fit_prm *prm)
{
  FILE *f;
  int frame, shadow, height, textpt, gx, gy, posx, posy, h_inc, i;
  GListStore *list;

  if (prm->script == NULL) {
    return 0;
  }

  f = g_fopen(prm->script, "w");
  if (f == NULL) {
    return 1;
  }

  posx = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->x)) * 100;
  posy = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->y)) * 100;

  frame = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->frame));
  if (frame) {
    fprintf(f, "new iarray name:textlen\n");
    fprintf(f, "new iarray name:textbbox\n");
  }

  shadow = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->shadow));
  textpt = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->font.pt)) * 100;
  height = ceil(textpt * 25.4 / 72.0 / 100) * 100;
  gy = posy;

  h_inc = ceil(height * 1.2 / 100) * 100;

  list = columnview_get_list (prm->caption);
  for (i = 0; i < PRM_NUM; i++) {
    NFitrslt *item;
    item = g_list_model_get_item (G_LIST_MODEL (list), i);
    if (item->active) {
      gx = posx;
      makescript(f, prm, gx, gy, height, item->caption, item->result);
      gy += h_inc;
    }
  }

  if (frame) {
    fprintf(f, "iarray:textlen:map 'int(X/100+0.5)*100'\n");
    if (shadow) {
      fprintf(f, "new rectangle\n");
      fprintf(f, "rectangle::x1=%d\n", posx - height / 4);
      fprintf(f, "rectangle::y1=%d\n", posy);
      fprintf(f, "rectangle::x2=%d+${iarray:textlen:max}\n", posx + 3 * height / 4);
      fprintf(f, "rectangle::y2=%d\n", gy + height / 2);
      fprintf(f, "rectangle::fill_R=0\n");
      fprintf(f, "rectangle::fill_G=0\n");
      fprintf(f, "rectangle::fill_B=0\n");
      fprintf(f, "rectangle::fill=true\n");
    }
    fprintf(f, "new rectangle\n");
    fprintf(f, "rectangle::x1=%d\n", posx - height / 2);
    fprintf(f, "rectangle::y1=%d\n", posy - height / 4);
    fprintf(f, "rectangle::x2=%d+${iarray:textlen:max}\n", posx + height / 2);
    fprintf(f, "rectangle::y2=%d\n", gy + height / 4);
    fprintf(f, "rectangle::fill_R=255\n");
    fprintf(f, "rectangle::fill_G=255\n");
    fprintf(f, "rectangle::fill_B=255\n");
    fprintf(f, "rectangle::stroke_R=0\n");
    fprintf(f, "rectangle::stroke_G=0\n");
    fprintf(f, "rectangle::stroke_B=0\n");
    fprintf(f, "rectangle::fill=true\n");
    fprintf(f, "rectangle::stroke=true\n");
    fprintf(f, "del iarray:textlen\n");
    fprintf(f, "del iarray:textbbox\n");
  }

  fclose(f);

  return 0;
}

static GtkWidget *
my_create_spin_button(const char *title, double min, double max, double inc, double init, GtkWidget **hbox)
{
  GtkWidget *w, *label;

  *hbox = gtk_grid_new();
  gtk_widget_set_hexpand(*hbox, FALSE);

  label = gtk_label_new_with_mnemonic(title);

  w = create_spin_button(min, max, inc, init, 0);
  gtk_widget_set_margin_end(w, 4);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
  gtk_widget_set_hexpand(w, TRUE);
  gtk_widget_set_halign(w, GTK_ALIGN_FILL);

  gtk_grid_attach(GTK_GRID(*hbox), label, 0, 0, 1, 1);
  gtk_grid_attach(GTK_GRID(*hbox), w,     1, 0, 1, 1);

  return w;
}

static void
frame_toggled(GtkWidget *togglebutton, gpointer user_data)
{
  int state;
  struct fit_prm *prm;

  prm = (struct fit_prm *) user_data;
  state = gtk_check_button_get_active(GTK_CHECK_BUTTON(togglebutton));
  gtk_widget_set_sensitive(GTK_WIDGET(prm->shadow), state);
}

static GtkWidget *
create_format_frame(struct fit_prm *prm)
{
  GtkWidget *frame, *vbox, *w, *hbox;

  frame = gtk_frame_new("format");

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  w = gtk_check_button_new_with_mnemonic("add _+");
  gtk_box_append(GTK_BOX(vbox), w);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(w), ADD_PLUS);
  prm->add_plus = w;

  w = gtk_check_button_new_with_mnemonic("_Expand");
  gtk_box_append(GTK_BOX(vbox), w);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(w), EXPAND);
  prm->expand = w;

  w = gtk_check_button_new_with_mnemonic("_Frame");
  gtk_box_append(GTK_BOX(vbox), w);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(w), FRAME);
  prm->frame = w;

  w = gtk_check_button_new_with_mnemonic("_Shadow");
  gtk_box_append(GTK_BOX(vbox), w);
  gtk_check_button_set_active(GTK_CHECK_BUTTON(w), FRAME);
  prm->shadow = w;

  g_signal_connect(prm->frame, "toggled", G_CALLBACK(frame_toggled), prm);

  w = my_create_spin_button("_Accuracy:", 1, 15, 1, ACCURACY, &hbox);
  gtk_box_append(GTK_BOX(vbox), hbox);
  prm->accuracy = w;

  gtk_frame_set_child(GTK_FRAME(frame), vbox);

  return frame;
}

static GtkWidget *
create_position_frame(struct fit_prm *prm)
{
  GtkWidget *frame, *w, *table;
  int j;

  frame = gtk_frame_new("position");

  table = gtk_grid_new();

  j = 0;
  w = create_spin_button(POS_MIN, POS_MAX, POS_INC, prm->posx / 100.0, 2);
  add_widget_to_table_sub(table, w, "_X:", TRUE, 0, 1, j++);
  prm->x = w;

  w = create_spin_button(POS_MIN, POS_MAX, POS_INC, prm->posy / 100.0, 2);
  add_widget_to_table_sub(table, w, "_Y:", TRUE, 0, 1, j++);
  prm->y = w;

  gtk_frame_set_child(GTK_FRAME(frame), table);

  return frame;
}

static void
set_parameter(struct fit_prm *prm)
{
  int i, j, accuracy, expand, add_plus, dim[PRM_NUM];
  char buf[LINE_BUF_SIZE], fmt[LINE_BUF_SIZE], *prm_str;
  GListStore *list;

  i = gtk_drop_down_get_selected (GTK_DROP_DOWN (prm->combo));
  if (i < 0) {
    return;
  }

  accuracy = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(prm->accuracy));
  expand = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->expand));
  add_plus = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->add_plus));

  if (strcmp(prm->data[i].type, "poly") == 0) {
    for (j = 0; j < PRM_NUM; j++) {
      dim[j] = (j <= prm->data[i].poly);
    }
  } else if (strcmp(prm->data[i].type, "exp") == 0||
	     strcmp(prm->data[i].type, "log") == 0||
	     strcmp(prm->data[i].type, "pow") == 0) {
    for (j = 0; j < PRM_NUM; j++) {
      dim[j] = (j < 2);
    }
  } else if (prm->data[i].userfunc) {
    for (j = 0; j < PRM_NUM; j++) {
      snprintf(buf, sizeof(buf), "%%%02d", j);
      if (strstr(prm->data[i].userfunc, buf)) {
	dim[j] = TRUE;
      } else {
	dim[j] = FALSE;
      }
    }
  }

  list = columnview_get_list (prm->caption);
  for (j = 0; j < PRM_NUM; j++) {
    char fitprm[64], caption[64];
    NFitrslt *item;
    snprintf(fmt, sizeof(fmt),
	     "%%#%s.%dg",
	     add_plus ? "+" : "",
	     accuracy);
    if (expand) {
      prm_str = g_strdup_printf(fmt, prm->data[i].prm[j]);
    } else {
      prm_str = g_strdup_printf("%%pf{%s %%{data:%d:fit_prm:%d}}",
                                fmt,
                                prm->data[i].file_id,
                                j);
    }
    item = g_list_model_get_item (G_LIST_MODEL (list), j);
    snprintf(fitprm, sizeof(fitprm), "%%%02d", j);
    snprintf(caption, sizeof(caption), "\\%%%02d: ", j);
    g_object_set(item, "active", dim[j], "prm", fitprm, "caption", caption, "result", prm_str, NULL);
    g_free(prm_str);
  }
}

static void
file_changed(struct fit_prm *prm)
{
  set_parameter(prm);
}

struct text_column {
  char *title;
  int editable;
  void (* func)(GtkCellRenderer *, gchar *, gchar *, gpointer);
};

static GtkWidget *
create_caption_frame(struct fit_prm *prm)
{
  GListStore *list;
  GtkWidget *tview, *hbox, *frame;
  int i;

  tview = columnview_column_create ();
  list = columnview_get_list (tview);
  for (i = 0; i < PRM_NUM; i++) {
    NFitrslt *item;
    item = n_fitrslt_new ();
    g_list_store_append (list, item);
    g_object_unref(item);
  }
  frame = gtk_frame_new(NULL);
  gtk_frame_set_child(GTK_FRAME(frame), tview);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  gtk_widget_set_hexpand(frame, TRUE);
  gtk_widget_set_vexpand(frame, TRUE);
  gtk_box_append(GTK_BOX(hbox), frame);

  prm->caption = tview;

  return hbox;
}

static GtkWidget *
create_file_frame(struct fit_prm *prm)
{
  GtkWidget *combo, *hbox, *label;
  char *str, *filename;
  int i;
  GStrvBuilder *strv_builder;
  GStrv strv;

  strv_builder = g_strv_builder_new ();

  for (i = 0; i < prm->fit_num; i++) {
    if (prm->data[i].file == NULL) {
      continue;
    }
    filename = g_path_get_basename(prm->data[i].file);
    str = g_strdup_printf("#%d %s", prm->data[i].file_id, filename);

    g_strv_builder_add (strv_builder, str);

    g_free(str);
    g_free(filename);
  }
  strv = g_strv_builder_end (strv_builder);
  g_strv_builder_unref (strv_builder);

  combo = gtk_drop_down_new_from_strings ((const char * const *) strv);
  g_strfreev (strv);
  prm->combo = combo;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  label = gtk_label_new_with_mnemonic("_Data:");
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), combo);

  gtk_box_append(GTK_BOX(hbox), label);
  gtk_box_append(GTK_BOX(hbox), combo);

  return hbox;
}

static GtkWidget *
create_control(GtkWidget *box, struct fit_prm *prm)
{
  GtkWidget *w, *hbox;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  w = create_format_frame(prm);
  gtk_box_append(GTK_BOX(hbox), w);

  w = create_position_frame(prm);
  gtk_box_append(GTK_BOX(hbox), w);

  w = create_font_frame(&prm->font);
  gtk_box_append(GTK_BOX(hbox), w);

  gtk_box_append(GTK_BOX(box), hbox);

  w = create_file_frame(prm);
  gtk_box_append(GTK_BOX(box), w);

  w = create_caption_frame(prm);
  gtk_box_append(GTK_BOX(box), w);

  return NULL;
}

static void
create_widgets(GtkWidget *vbox, struct fit_prm *prm)
{
  GtkWidget *w;

  w = create_title(NAME " version " VERSION, "fitting results -> legend text");
  gtk_box_append(GTK_BOX(vbox), w);

  w = create_control(vbox, prm);
}

static const char *
get_opt(int argc, char **argv, struct fit_prm *prm)
{
  int i;
  char *data_file = NULL;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-x") == 0) {
      i++;
      if (argv[i]) {
	prm->posx = atoi(argv[i]);
      }
    } else if (strcmp(argv[i], "-y") == 0) {
      i++;
      if (argv[i]) {
	prm->posy = atoi(argv[i]);
      }
    } else if (strcmp(argv[i], "-d") == 0) {
      prm->dark_theme = TRUE;
    } else if (data_file == NULL) {
      data_file = argv[i];
    } else {
      prm->script = argv[i];
    }
  }

  return data_file;
}

static void
dialog_response_cancel(GtkDialog* self, gpointer user_data)
{
  g_main_loop_quit(MainLoop);
}

static void
dialog_response_ok(GtkDialog* self, gpointer user_data)
{
  struct fit_prm *prm;
  prm = user_data;
  savescript(prm);
  dialog_response_cancel(self, user_data);
}

int
main(int argc, char **argv)
{
  GtkWidget *mainwin, *vbox;
  struct fit_prm prm;
  const char *data_file;

  setlocale(LC_ALL, "");
  MainLoop = g_main_loop_new (NULL, FALSE);
  gtk_init();

  prm.posx = POS_X;
  prm.posy = POS_Y;
  prm.dark_theme = FALSE;

  data_file = get_opt(argc, argv, &prm);
  if (data_file == NULL) {
    return 0;
  }

  if (loaddatalist(&prm, data_file)) {
    return 0;
  }

  mainwin = dialog_new (NAME, G_CALLBACK (dialog_response_cancel), G_CALLBACK (dialog_response_ok), &prm);
  g_signal_connect(mainwin, "destroy", G_CALLBACK(dialog_response_cancel), &prm);
  prm.window = mainwin;
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child (GTK_WINDOW (mainwin), vbox);
  create_widgets(vbox, &prm);

  g_signal_connect_swapped(prm.combo, "notify::selected", G_CALLBACK(file_changed), &prm);
  g_signal_connect_swapped(prm.add_plus, "toggled", G_CALLBACK(file_changed), &prm);
  g_signal_connect_swapped(prm.expand, "toggled", G_CALLBACK(file_changed), &prm);
  g_signal_connect_swapped(prm.accuracy, "value-changed", G_CALLBACK(file_changed), &prm);

  gtk_drop_down_set_selected (GTK_DROP_DOWN (prm.combo), 0);
  set_parameter(&prm);

  if (prm.dark_theme) {
    use_dark_theme();
  }
  gtk_window_present (GTK_WINDOW (mainwin));
  g_main_loop_run(MainLoop);

  return 0;
}

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
#define VERSION "1.00.03"

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
  int posx, posy, fit_num;
};

#if GTK_CHECK_VERSION(4, 0, 0)
static GMainLoop *MainLoop;
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
  frame = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->frame));
#else
  frame = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->frame));
#endif

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
  char *cap, *val;
  FILE *f;
  int frame, shadow, height, textpt, gx, gy, posx, posy, h_inc, i, draw;
  GtkTreeModel *model;
  GtkTreeIter iter;

  if (prm->script == NULL) {
    return 0;
  }

  f = g_fopen(prm->script, "w");

  posx = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->x)) * 100;
  posy = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->y)) * 100;

#if GTK_CHECK_VERSION(4, 0, 0)
  frame = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->frame));
#else
  frame = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->frame));
#endif
  if (frame) {
    fprintf(f, "new iarray name:textlen\n");
    fprintf(f, "new iarray name:textbbox\n");
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  shadow = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->shadow));
#else
  shadow = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->shadow));
#endif
  textpt = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->font.pt)) * 100;
  height = ceil(textpt * 25.4 / 72.0 / 100) * 100;
  gy = posy;

  h_inc = ceil(height * 1.2 / 100) * 100;

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(prm->caption));
  if (! gtk_tree_model_get_iter_first(model, &iter)) {
    return 1;
  }
  for (i = 0; i < PRM_NUM; i++) {
    gtk_tree_model_get(model, &iter, COLUMN_CHECK, &draw, COLUMN_CAPTION, &cap, COLUMN_VAL, &val, -1);
    if (draw) {
      gx = posx;
      makescript(f, prm, gx, gy, height, cap, val);
      gy += h_inc;
    }
    g_free(cap);
    g_free(val);
    if (! gtk_tree_model_iter_next(model, &iter)) {
      break;
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
#if ! GTK_CHECK_VERSION(4, 0, 0)
  g_object_set(label, "margin", GINT_TO_POINTER(4), NULL);
#endif

  w = create_spin_button(min, max, inc, init, 0);
#if GTK_CHECK_VERSION(3, 12, 0)
  gtk_widget_set_margin_end(w, 4);
#else
  gtk_widget_set_margin_right(w, 4);
#endif
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
#if GTK_CHECK_VERSION(4, 0, 0)
  state = gtk_check_button_get_active(GTK_CHECK_BUTTON(togglebutton));
#else
  state = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(togglebutton));
#endif
  gtk_widget_set_sensitive(GTK_WIDGET(prm->shadow), state);
}

static GtkWidget *
create_format_frame(struct fit_prm *prm)
{
  GtkWidget *frame, *vbox, *w, *hbox;

  frame = gtk_frame_new("format");

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  w = gtk_check_button_new_with_mnemonic("add _+");
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_check_button_set_active(GTK_CHECK_BUTTON(w), ADD_PLUS);
#else
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), ADD_PLUS);
#endif
  prm->add_plus = w;

  w = gtk_check_button_new_with_mnemonic("_Expand");
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_check_button_set_active(GTK_CHECK_BUTTON(w), EXPAND);
#else
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), EXPAND);
#endif
  prm->expand = w;

  w = gtk_check_button_new_with_mnemonic("_Frame");
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_check_button_set_active(GTK_CHECK_BUTTON(w), FRAME);
#else
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FRAME);
#endif
  prm->frame = w;

  w = gtk_check_button_new_with_mnemonic("_Shadow");
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_check_button_set_active(GTK_CHECK_BUTTON(w), FRAME);
#else
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FRAME);
#endif
  prm->shadow = w;

  g_signal_connect(prm->frame, "toggled", G_CALLBACK(frame_toggled), prm);

  w = my_create_spin_button("_Accuracy:", 1, 15, 1, ACCURACY, &hbox);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), hbox);
#else
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
#endif
  prm->accuracy = w;


#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(frame), vbox);
#else
  gtk_container_add(GTK_CONTAINER(frame), vbox);
#endif

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

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(frame), table);
#else
  gtk_container_add(GTK_CONTAINER(frame), table);
#endif

  return frame;
}

static void
set_parameter(struct fit_prm *prm)
{
  int i, j, accuracy, expand, add_plus, dim[PRM_NUM];
  char buf[LINE_BUF_SIZE], fmt[LINE_BUF_SIZE], *prm_str;
  GtkTreeModel *model;
  GtkTreeIter iter;

  i = gtk_combo_box_get_active(GTK_COMBO_BOX(prm->combo));
  if (i < 0) {
    return;
  }

  accuracy = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(prm->accuracy));
#if GTK_CHECK_VERSION(4, 0, 0)
  expand = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->expand));
#else
  expand = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->expand));
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  add_plus = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->add_plus));
#else
  add_plus = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->add_plus));
#endif

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

  model = gtk_tree_view_get_model(GTK_TREE_VIEW(prm->caption));
  if (! gtk_tree_model_get_iter_first(model, &iter)) {
    return;
  }

  for (j = 0; j < PRM_NUM; j++) {
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
    gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_CHECK, dim[j], COLUMN_VAL, prm_str, -1);
    g_free(prm_str);
    if (! gtk_tree_model_iter_next(model, &iter)) {
      break;
    }
  }
}

static void
file_changed(GtkWidget *widget, gpointer user_data)
{
  struct fit_prm *prm;

  prm = (struct fit_prm *) user_data;
  set_parameter(prm);
}

static void
text_edited(GtkCellRenderer *renderer, gchar *path, gchar *new_text, struct fit_prm *prm, int column)
{
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreeView *view;

  view = GTK_TREE_VIEW(prm->caption);
  model = gtk_tree_view_get_model(view);

  if (! gtk_tree_model_get_iter_from_string(model, &iter, path)) {
    return;
  }

  gtk_list_store_set(GTK_LIST_STORE(model), &iter, column, new_text, -1);
}

static void
caption_edited(GtkCellRenderer *renderer, gchar *path, gchar *new_text, gpointer user_data)
{
  text_edited(renderer, path, new_text, (struct fit_prm *) user_data, COLUMN_CAPTION);
}

static void
value_edited(GtkCellRenderer *renderer, gchar *path, gchar *new_text, gpointer user_data)
{
  text_edited(renderer, path, new_text, (struct fit_prm *) user_data, COLUMN_VAL);
}

static void
caption_toggled(GtkCellRendererToggle *cell_renderer, gchar *path, gpointer user_data)
{
  struct fit_prm *prm;
  GtkTreeView *view;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean v;

  prm = (struct fit_prm *) user_data;
  view = GTK_TREE_VIEW(prm->caption);
  model = gtk_tree_view_get_model(view);

  if (! gtk_tree_model_get_iter_from_string(model, &iter, path)) {
    return;
  }

  gtk_tree_model_get(model, &iter, COLUMN_CHECK, &v, -1);

  v = !v;

  gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_CHECK, v, -1);
}

struct text_column {
  char *title;
  int editable;
  void (* func)(GtkCellRenderer *, gchar *, gchar *, gpointer);
};

static GtkWidget *
create_caption_frame(struct fit_prm *prm)
{
  GtkListStore *list;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  GtkTreeIter iter;
  GtkWidget *tview, *hbox, *frame;
  int i, n;
  struct text_column text_column[] = {
    {"prm",     FALSE, NULL},
    {"caption", TRUE,  caption_edited},
    {"result",  TRUE,  value_edited},
  };
  char buf1[64], buf2[64];

  n = sizeof(text_column) / sizeof(*text_column);

  list = gtk_list_store_new(n + 1, G_TYPE_BOOLEAN, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
  tview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list));
#if ! GTK_CHECK_VERSION(3, 14, 0)
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tview), TRUE);
#endif
  gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(tview), GTK_TREE_VIEW_GRID_LINES_VERTICAL);

  renderer = gtk_cell_renderer_toggle_new();
  col = gtk_tree_view_column_new_with_attributes("", renderer, "active", 0, NULL);
  g_object_set(renderer, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
  g_signal_connect(renderer, "toggled", G_CALLBACK(caption_toggled), prm);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tview), col);

  for (i = 0; i < n; i++) {
    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes(text_column[i].title, renderer, "text", i + 1, "sensitive", 0, NULL);
    if (text_column[i].editable) {
      g_object_set(renderer, "editable", TRUE, NULL);
      g_signal_connect(renderer, "edited", G_CALLBACK(text_column[i].func), prm);
    }
    gtk_tree_view_append_column(GTK_TREE_VIEW(tview), col);
  }

  for (i = 0; i < PRM_NUM; i++) {
    snprintf(buf1, sizeof(buf1), "%%%02d", i);
    snprintf(buf2, sizeof(buf2), "\\%%%02d: ", i);
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter, COLUMN_PRM, buf1, COLUMN_CAPTION, buf2, -1);
  }

  frame = gtk_frame_new(NULL);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(frame), tview);
#else
  gtk_container_add(GTK_CONTAINER(frame), tview);
#endif

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), frame);
#else
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 4);
#endif

  prm->caption = tview;

  return hbox;
}

static GtkWidget *
create_file_frame(struct fit_prm *prm)
{
  GtkListStore  *list;
  GtkTreeIter iter;
  GtkCellRenderer *rend_s;
  GtkWidget *combo, *hbox, *label;
  char *str, *filename;
  int i;

  combo = gtk_combo_box_new();
  list = gtk_list_store_new(1, G_TYPE_STRING);
  gtk_combo_box_set_model(GTK_COMBO_BOX(combo), GTK_TREE_MODEL(list));
  rend_s = gtk_cell_renderer_text_new();
  gtk_cell_layout_pack_start(GTK_CELL_LAYOUT(combo), rend_s, FALSE);
  gtk_cell_layout_add_attribute(GTK_CELL_LAYOUT(combo), rend_s, "text", 0);

  for (i = 0; i < prm->fit_num; i++) {
    if (prm->data[i].file == NULL) {
      continue;
    }
    filename = g_path_get_basename(prm->data[i].file);
    str = g_strdup_printf("#%d %s", prm->data[i].file_id, filename);

    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter, 0, str, -1);

    g_free(str);
    g_free(filename);
  }

  prm->combo = combo;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  label = gtk_label_new_with_mnemonic("_Data:");
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), combo);

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), label);
  gtk_box_append(GTK_BOX(hbox), combo);
#else
  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 4);
#endif

  return hbox;
}

static GtkWidget *
create_control(GtkWidget *box, struct fit_prm *prm)
{
  GtkWidget *w, *hbox;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  w = create_format_frame(prm);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), w);
#else
  gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 4);
#endif

  w = create_position_frame(prm);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), w);
#else
  gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 4);
#endif

  w = create_font_frame(&prm->font);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), w);

  gtk_box_append(GTK_BOX(box), hbox);
#else
  gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 4);

  gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 4);
#endif

  w = create_file_frame(prm);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(box), w);
#else
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 4);
#endif

  w = create_caption_frame(prm);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(box), w);
#else
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 4);
#endif

  return NULL;
}

static void
create_widgets(GtkWidget *vbox, struct fit_prm *prm)
{
  GtkWidget *w;

  w = create_title(NAME " version " VERSION, "fitting results -> legend text");
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
#endif

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
    } else if (data_file == NULL) {
      data_file = argv[i];
    } else {
      prm->script = argv[i];
    }
  }

  return data_file;
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
dialog_response(GtkDialog* self, gint response_id, gpointer user_data)
{
  struct fit_prm *prm;
  prm = user_data;
  if (response_id == GTK_RESPONSE_ACCEPT) {
    savescript(prm);
  }
  gtk_window_destroy(GTK_WINDOW(self));
  g_main_loop_quit(MainLoop);
}
#endif

int
main(int argc, char **argv)
{
  GtkWidget *mainwin;
#if ! GTK_CHECK_VERSION(4, 0, 0)
  gint r;
#endif
  struct fit_prm prm;
  const char *data_file;

  setlocale(LC_ALL, "");
#if GTK_CHECK_VERSION(4, 0, 0)
  MainLoop = g_main_loop_new (NULL, FALSE);
  gtk_init();
#else
  gtk_init(&argc, &argv);
#endif

  prm.posx = POS_X;
  prm.posy = POS_Y;

  data_file = get_opt(argc, argv, &prm);
  if (data_file == NULL) {
    return 0;
  }

  if (loaddatalist(&prm, data_file)) {
    return 0;
  }

  mainwin = gtk_dialog_new_with_buttons(NAME, NULL, 0,
					"_Cancel",
					GTK_RESPONSE_REJECT,
					"_OK",
					GTK_RESPONSE_ACCEPT,
					NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(mainwin), GTK_RESPONSE_ACCEPT);
  prm.window = mainwin;
  create_widgets(gtk_dialog_get_content_area(GTK_DIALOG(mainwin)), &prm);

  g_signal_connect(prm.combo, "changed", G_CALLBACK(file_changed), &prm);
  g_signal_connect(prm.add_plus, "toggled", G_CALLBACK(file_changed), &prm);
  g_signal_connect(prm.expand, "toggled", G_CALLBACK(file_changed), &prm);
  g_signal_connect(prm.accuracy, "value-changed", G_CALLBACK(file_changed), &prm);

  gtk_combo_box_set_active(GTK_COMBO_BOX(prm.combo), 0);


#if GTK_CHECK_VERSION(4, 0, 0)
  g_signal_connect(mainwin, "response", G_CALLBACK(dialog_response), &prm);
  gtk_widget_show(mainwin);
  g_main_loop_run(MainLoop);
#else
  gtk_widget_show_all(mainwin);

  r = gtk_dialog_run(GTK_DIALOG(mainwin));
  if (r == GTK_RESPONSE_ACCEPT) {
    savescript(&prm);
  }
#endif

  return 0;
}

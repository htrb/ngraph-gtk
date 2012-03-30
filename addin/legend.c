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

#define NAME    "Legend"
#define VERSION "1.01.01"

#define POS_X   50.00
#define POS_Y   50.00
#define POS_INC  1.00
#define POS_MIN -1000
#define POS_MAX  1000

#define MIX       TRUE
#define CAPTION   TRUE
#define TYPE      TRUE
#define FRAME     TRUE

#define COLUMN_CHECK   0
#define COLUMN_ID      1
#define COLUMN_CAPTION 5

struct file_data {
  char *file, *math_x, *math_y, *type, *caption, *style;
  int id, x, y, mark, size, width, r, g, b, r2, g2, b2, show, mix;
};

struct caption_widget {
  GtkWidget *check, *caption, *val, *label;
};

struct file_prm {
  GtkWidget *window, *x,*y, *width, *mix, *type, *caption, *frame, *files;
  struct font_prm font;
  const char *script;
  struct file_data *data;
  int posx, posy, w, file_num;
};

static char *
escape_char(const char *src, const char *escape, const char *str)
{
  GString *s;
  const char *ptr;

  if (src == NULL) {
    return g_strdup("");
  }

  if (escape == NULL) {
    return g_strdup(src);
  }

  s = g_string_new("");
  for (ptr = src; *ptr; ptr++) {
    if (strchr(escape, *ptr)) {
      g_string_append(s, str);
    }
    g_string_append_c(s, *ptr);
  }

  return g_string_free(s, FALSE);
}

static int
loaddatalist(struct file_prm *prm, const char *datalist)
{
  FILE *f;
  int i, j, filenum;
  char *hidden, *str, *file;

  if (datalist == NULL) {
    return 1;
  }

  f = g_fopen(datalist, "r");
  if (f == NULL) {
    return 1;
  }

  filenum = fgets_int(f);
  if (filenum < 1) {
    fclose(f);
    return 1;
  }

  prm->data = g_malloc(sizeof(*prm->data) * filenum);
  prm->file_num = filenum;
  for (i = 0; i < filenum; i++) {
    prm->data[i].file = fgets_str(f);
    prm->data[i].id = fgets_int(f);

    hidden = fgets_str(f);
    prm->data[i].show = (g_strcmp0(hidden, "true") == 0) ? FALSE : TRUE;
    g_free(hidden);

    prm->data[i].x = fgets_int(f);
    prm->data[i].y = fgets_int(f);
    prm->data[i].type = fgets_str(f);
    prm->data[i].mark = fgets_int(f);
    prm->data[i].size = fgets_int(f);
    prm->data[i].width = fgets_int(f);
    prm->data[i].style = fgets_str(f);
    prm->data[i].r = fgets_int(f);
    prm->data[i].g = fgets_int(f);
    prm->data[i].b = fgets_int(f);
    prm->data[i].r2 = fgets_int(f);
    prm->data[i].g2 = fgets_int(f);
    prm->data[i].b2 = fgets_int(f);
    prm->data[i].math_x = fgets_str(f);
    prm->data[i].math_y = fgets_str(f);
    prm->data[i].mix = -1;
    if (prm->data[i].file) {
      file = g_path_get_basename(prm->data[i].file);
      str = escape_char(file, "_^@%\\", "\\");
      prm->data[i].caption = str;
      g_free(file);
    } else {
      prm->data[i].caption = NULL;
    }
    for (j = 0; j < i; j++) {
      if ((g_strcmp0(prm->data[i].file, prm->data[j].file) == 0) &&
	  (prm->data[i].x == prm->data[j].x) &&
	  (prm->data[i].y == prm->data[j].y) &&
	  (g_strcmp0(prm->data[i].math_x, prm->data[j].math_x) == 0) &&
	  (g_strcmp0(prm->data[i].math_y, prm->data[j].math_y) == 0)) {
	prm->data[i].mix = j;
	break;
      }
    }
  }
  fclose(f);

  return 0;
}

static void
makescript(FILE *f, struct file_data *data, int gx, int gy, int width, int height)
{
  int h;

  h = ceil(height * 2.0 / 3.0 / 100.0) * 100;

  if ((g_strcmp0(data->type, "line") == 0) ||
      (g_strcmp0(data->type, "polygon") == 0) ||
      (g_strcmp0(data->type, "curve") == 0) ||
      (g_strcmp0(data->type, "diagonal") == 0) ||
      (g_strcmp0(data->type, "errorbar_x") == 0) ||
      (g_strcmp0(data->type, "errorbar_y") == 0) ||
      (g_strcmp0(data->type, "staircase_x") == 0) ||
      (g_strcmp0(data->type, "staircase_y") == 0) ||
      (g_strcmp0(data->type, "fit") == 0)) {
    fprintf(f, "new path type=line\n");
    fprintf(f, "path::points='%d %d %d %d'\n", gx, gy + h, gx + width, gy + h);
    fprintf(f, "path::width=%d\n", data->width);
    if (data->style && data->style[0] != '\0') {
      fprintf(f, "path::style='%s'\n", data->style);
    }
    fprintf(f, "path::stroke_R=%d\n", data->r);
    fprintf(f, "path::stroke_G=%d\n", data->g);
    fprintf(f, "path::stroke_B=%d\n", data->b);
  } else if (g_strcmp0(data->type, "arrow") == 0) {
    fprintf(f, "new path type=line\n");
    fprintf(f, "path::points='%d %d %d %d'\n", gx, gy + h, gx + width, gy + h );
    fprintf(f, "path::width=%d\n", data->width);
    if (data->style && data->style[0] != '\0') {
      fprintf(f, "path::style='%s'\n", data->style);
    }
    fprintf(f, "path::stroke_R=%d\n", data->r);
    fprintf(f, "path::stroke_G=%d\n", data->g);
    fprintf(f, "path::stroke_B=%d\n", data->b);
    fprintf(f, "path::arrow=end\n");
  } else if ((g_strcmp0(data->type, "polygon_solid_fill") == 0) ||
	     (g_strcmp0(data->type, "rectangle") == 0) ||
	     (g_strcmp0(data->type, "rectangle_fill") == 0) ||
	     (g_strcmp0(data->type, "rectangle_solid_fill") == 0) ||
	     (g_strcmp0(data->type, "bar_x") == 0) ||
	     (g_strcmp0(data->type, "bar_y") == 0) ||
	     (g_strcmp0(data->type, "bar_fill_x") == 0) ||
	     (g_strcmp0(data->type, "bar_fill_y") == 0) ||
	     (g_strcmp0(data->type, "bar_solid_fill_x") == 0) ||
	     (g_strcmp0(data->type, "bar_solid_fill_y") == 0)) {
    fprintf(f, "new rectangle\n");
    fprintf(f, "rectangle::x1=%d\n", gx);
    fprintf(f, "rectangle::y1=%d\n", gy + h - height / 2);
    fprintf(f, "rectangle::x2=%d\n", gx + width);
    fprintf(f, "rectangle::y2=%d\n", gy + h + height / 2);
    if ((g_strcmp0(data->type, "rectangle") == 0) ||
	(g_strcmp0(data->type, "bar_x") == 0) ||
	(g_strcmp0(data->type, "bar_y") == 0)) {
      fprintf(f, "rectangle::fill=false\n");
      fprintf(f, "rectangle::stroke_R=%d\n", data->r);
      fprintf(f, "rectangle::stroke_G=%d\n", data->g);
      fprintf(f, "rectangle::stroke_B=%d\n", data->b);
    } else if ((g_strcmp0(data->type, "rectangle_fill") == 0) ||
	       (g_strcmp0(data->type, "bar_fill_x") == 0) ||
	       (g_strcmp0(data->type, "bar_fill_y") == 0)) {
      fprintf(f, "rectangle::fill=true\n");
      fprintf(f, "rectangle::stroke=true\n");
      fprintf(f, "rectangle::fill_R=%d\n", data->r2);
      fprintf(f, "rectangle::fill_G=%d\n", data->g2);
      fprintf(f, "rectangle::fill_B=%d\n", data->b2);
      fprintf(f, "rectangle::stroke_R=%d\n", data->r);
      fprintf(f, "rectangle::stroke_G=%d\n", data->g);
      fprintf(f, "rectangle::stroke_B=%d\n", data->b);
    } else if ((g_strcmp0(data->type, "polygon_solid_fill") == 0) ||
	       (g_strcmp0(data->type, "rectangle_solid_fill") == 0) ||
	       (g_strcmp0(data->type, "bar_solid_fill_x") == 0) ||
	       (g_strcmp0(data->type, "bar_solid_fill_y") == 0)) {;
      fprintf(f, "rectangle::fill=true\n");
      fprintf(f, "rectangle::fill_R=%d\n", data->r);
      fprintf(f, "rectangle::fill_G=%d\n", data->g);
      fprintf(f, "rectangle::fill_B=%d\n", data->b);
    }
    fprintf(f, "rectangle::width=%d\n", data->width);
    if (data->style && data->style[0] != '\0') {
      fprintf(f, "rectangle::style='%s'\n", data->style);
    }
  } else if (g_strcmp0(data->type, "mark") == 0) {
    fprintf(f, "new mark\n");
    fprintf(f, "mark::x=%d\n", gx + width / 2);
    fprintf(f, "mark::y=%d\n", gy + h);
    fprintf(f, "mark::size=%d\n", data->size);
    fprintf(f, "mark::type=%d\n", data->mark);
    fprintf(f, "mark::width=%d\n", data->width);
    if (data->style && data->style[0] != '\0') {
      fprintf(f, "mark::style='%s'\n", data->style);
    }
    fprintf(f, "mark::R=%d\n", data->r);
    fprintf(f, "mark::G=%d\n", data->g);
    fprintf(f, "mark::B=%d\n", data->b);
    fprintf(f, "mark::R2=%d\n", data->r2);
    fprintf(f, "mark::G2=%d\n", data->g2);
    fprintf(f, "mark::B2=%d\n", data->b2);
  }
}

static int
savescript(struct file_prm *prm)
{
  FILE *f;
  int i, j, height, len, gx, gy, posx, posy, pt, spc, script, r, g, b, style, width;
  gboolean type, mix, frame, caption;
  char *str;
  const char *font;

  if (prm->script == NULL) {
    return 1;
  }

  f = g_fopen(prm->script, "w");
  if (f == NULL) {
    return 1;
  }

  type = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->type));
  mix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->mix));
  frame = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->frame));
  caption = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->caption));
  font = get_selected_font(&prm->font);

  get_font_parameter(&prm->font, &pt, &spc, &script, &style, &r, &g, &b);

  width = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->width)) * 100;
  height = ceil(pt * 25.4 / 72.0 / 100.0) * 100;

  posx = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->x)) * 100;
  posy = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->y)) * 100;

  if (frame) {
    fprintf(f, "new iarray name:textlen\n");
    fprintf(f, "new iarray name:textbbox\n");
  }

  len = 0;
  gy = posy;

  for (i = 0; i < prm->file_num; i++) {
    if ((( prm->data[i].mix == -1) || (! mix )) && prm->data[i].show) {
      gx = posx;
      if (type) {
        makescript(f, &prm->data[i], gx, gy, width, height);
	if (mix) {
	  for (j = i + 1; j < prm->file_num; j++) {
	    if (prm->data[j].mix == i && prm->data[j].show) {
	      makescript(f, &prm->data[j], gx, gy, width, height);
	    }
	  }
        }
        len = width + height / 2;
      }
      if (caption) {
        fprintf(f, "new text\n");
        str = escape_char(prm->data[i].caption, "'", "'\"'\"");
        fprintf(f, "text::text='%s'\n", str);
        g_free(str);
        fprintf(f, "text::x=%d\n", gx + len);
        fprintf(f, "text::y=%d\n", gy + height);
        fprintf(f, "text::pt=%d\n", pt);
        fprintf(f, "text::font=%s\n", font);
        fprintf(f, "text::style=%d\n", style);
        fprintf(f, "text::space=%d\n", spc);
        fprintf(f, "text::script_size=%d\n", script);
        fprintf(f, "text::R=%d\n", r);
        fprintf(f, "text::G=%d\n", g);
        fprintf(f, "text::B=%d\n", b);
        if (frame) {
          fprintf(f, "iarray:textbbox:@=${text::bbox}\n");
          fprintf(f, "iarray:textlen:push \"${iarray:textbbox:get:2}-${iarray:textbbox:get:0}\"\n");
        }
      }
      gy = gy + ceil(height * 1.2 / 100.0) * 100;
    }
  }
  if (frame) {
    fprintf(f, "iarray:textlen:map 'int(X/100+0.5)*100'\n");
    fprintf(f, "new rectangle\n");
    fprintf(f, "rectangle::x1=%d\n", posx - height / 4);
    fprintf(f, "rectangle::y1=%d\n", posy);
    fprintf(f, "rectangle::x2=%d+${iarray:textlen:max}\n", posx + len + 3 * height / 4);
    fprintf(f, "rectangle::y2=%d\n", gy + height / 2);
    fprintf(f, "rectangle::fill_R=0\n");
    fprintf(f, "rectangle::fill_G=0\n");
    fprintf(f, "rectangle::fill_B=0\n");
    fprintf(f, "rectangle::fill=true\n");
    fprintf(f, "rectangle::stroke=false\n");
    fprintf(f, "new rectangle\n");
    fprintf(f, "rectangle::x1=%d\n", posx - height / 2);
    fprintf(f, "rectangle::y1=%d\n", posy - height / 4);
    fprintf(f, "rectangle::x2=%d+${iarray:textlen:max}\n", posx + len + height / 2);
    fprintf(f, "rectangle::y2=%d\n", gy + height / 4);
    fprintf(f, "rectangle::fill_R=255\n");
    fprintf(f, "rectangle::fill_G=255\n");
    fprintf(f, "rectangle::fill_B=255\n");
    fprintf(f, "rectangle::stroke_R=0\n");
    fprintf(f, "rectangle::stroke_G=0\n");
    fprintf(f, "rectangle::stroke_B=0\n");
    fprintf(f, "rectangle::fill=true\n");
    fprintf(f, "rectangle::stroke=true\n");
    fprintf(f, "movetop rectangle:!\n");
    fprintf(f, "movetop rectangle:!\n");
    fprintf(f, "del iarray:textlen\n");
    fprintf(f, "del iarray:textbbox\n");
  }
  fprintf(f, "menu::modified=true\n");

  fclose(f);

  return 0;
}

static GtkWidget *
create_option_frame(struct file_prm *prm)
{
  GtkWidget *frame, *vbox, *w;

  frame = gtk_frame_new("option");

  vbox = gtk_vbox_new(FALSE, 4);

  w = gtk_check_button_new_with_mnemonic("_Mix");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), MIX);
  prm->mix = w;

  w = gtk_check_button_new_with_mnemonic("_Type");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TYPE);
  prm->type = w;

  w = gtk_check_button_new_with_mnemonic("_Caption");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), CAPTION);
  prm->caption = w;


  w = gtk_check_button_new_with_mnemonic("_Frame");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FRAME);
  prm->frame = w;


  gtk_container_add(GTK_CONTAINER(frame), vbox);

  return frame;
}

static GtkWidget *
create_geometry_frame(struct file_prm *prm)
{
  GtkWidget *frame, *w, *table;
  int j;

  frame = gtk_frame_new("geometry");

  table = gtk_table_new(1, 2, FALSE);

  j = 0;
  w = gtk_spin_button_new_with_range(POS_MIN, POS_MAX, POS_INC);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(w), 2);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), prm->posx / 100.0);
  add_widget_to_table_sub(table, w, "_X:", TRUE, 0, 1, j++);
  prm->x = w;

  w = gtk_spin_button_new_with_range(POS_MIN, POS_MAX, POS_INC);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(w), 2);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), prm->posy / 100.0);
  add_widget_to_table_sub(table, w, "_Y:", TRUE, 0, 1, j++);
  prm->y = w;

  w = gtk_spin_button_new_with_range(1, 100, 10);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(w), 2);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), prm->w / 100.0);
  add_widget_to_table_sub(table, w, "_Width:", TRUE, 0, 1, j++);
  prm->width = w;

  gtk_container_add(GTK_CONTAINER(frame), table);

  return frame;
}

static void
set_parameter(struct file_prm *prm)
{
  int i, mix;
  char *basename;
  GtkTreeIter iter;
  GtkListStore *list;


  list = GTK_LIST_STORE(gtk_tree_view_get_model(GTK_TREE_VIEW(prm->files)));
  gtk_list_store_clear(list);

  mix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->mix));

  for (i = 0; i < prm->file_num; i++) {
    if (mix && prm->data[i].mix >= 0) {
      continue;
    }
    gtk_list_store_append(list, &iter);

    basename = g_path_get_basename(prm->data[i].file);
    gtk_list_store_set(list, &iter,
		       0, prm->data[i].show,
		       1, prm->data[i].id,
		       2, basename,
		       3, prm->data[i].x,
		       4, prm->data[i].y,
		       5, prm->data[i].caption,
		       -1);
    g_free(basename);
  }
}

static void
set_files(GtkWidget *widget, gpointer user_data)
{
  struct file_prm *prm;

  prm = (struct file_prm *) user_data;
  set_parameter(prm);
}

static void
caption_toggled(GtkCellRendererToggle *cell_renderer, gchar *path, gpointer user_data)
{
  struct file_prm *prm;
  GtkTreeView *view;
  GtkTreeModel *model;
  GtkTreeIter iter;
  gboolean v;
  gint i;

  prm = (struct file_prm *) user_data;
  view = GTK_TREE_VIEW(prm->files);
  model = gtk_tree_view_get_model(view);

  if (! gtk_tree_model_get_iter_from_string(model, &iter, path)) {
    return;
  }

  gtk_tree_model_get(model, &iter, COLUMN_CHECK, &v, -1);
  gtk_tree_model_get(model, &iter, COLUMN_ID, &i, -1);

  v = !v;

  prm->data[i].show = v;

  gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_CHECK, v, -1);
}

static void
caption_edited(GtkCellRenderer *renderer, gchar *path, gchar *new_text, gpointer user_data)
{
  struct file_prm *prm;
  GtkTreeModel *model;
  GtkTreeIter iter;
  GtkTreeView *view;
  gint i;

  prm = (struct file_prm *) user_data;
  view = GTK_TREE_VIEW(prm->files);
  model = gtk_tree_view_get_model(view);

  if (! gtk_tree_model_get_iter_from_string(model, &iter, path)) {
    return;
  }

  gtk_tree_model_get(model, &iter, COLUMN_ID, &i, -1);

  g_free(prm->data[i].caption);
  prm->data[i].caption = g_strdup(new_text);

  gtk_list_store_set(GTK_LIST_STORE(model), &iter, COLUMN_CAPTION, new_text, -1);
}

static GtkWidget *
create_file_frame(struct file_prm *prm)
{
  GtkListStore *list;
  GtkCellRenderer *renderer;
  GtkTreeViewColumn *col;
  GtkWidget *tview, *hbox, *swin, *frame;
  int i, n;
  char *title[] = {"#", "File", "x", "y", "Caption"};

  n = sizeof(title) / sizeof(*title);

  list = gtk_list_store_new(n + 1, G_TYPE_BOOLEAN, G_TYPE_INT, G_TYPE_STRING, G_TYPE_INT, G_TYPE_INT, G_TYPE_STRING);
  tview = gtk_tree_view_new_with_model(GTK_TREE_MODEL(list));
  gtk_widget_set_size_request(GTK_WIDGET(tview), -1, 200);
  gtk_tree_view_set_rules_hint(GTK_TREE_VIEW(tview), TRUE);
  gtk_tree_view_set_grid_lines(GTK_TREE_VIEW(tview), GTK_TREE_VIEW_GRID_LINES_VERTICAL);


  renderer = gtk_cell_renderer_toggle_new();
  col = gtk_tree_view_column_new_with_attributes("", renderer, "active", 0, NULL);
  g_object_set(renderer, "mode", GTK_CELL_RENDERER_MODE_ACTIVATABLE, NULL);
  g_signal_connect(renderer, "toggled", G_CALLBACK(caption_toggled), prm);
  gtk_tree_view_append_column(GTK_TREE_VIEW(tview), col);

  for (i = 0; i < n; i++) {
    renderer = gtk_cell_renderer_text_new();
    col = gtk_tree_view_column_new_with_attributes(title[i], renderer, "text", i + 1, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(tview), col);
  }
  g_object_set(renderer, "editable", TRUE, NULL);
  g_signal_connect(renderer, "edited", G_CALLBACK(caption_edited), prm);

  swin = gtk_scrolled_window_new(NULL, NULL);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  gtk_container_add(GTK_CONTAINER(swin), tview);

  frame = gtk_frame_new(NULL);
  gtk_container_add(GTK_CONTAINER(frame), swin);

  hbox = gtk_hbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 4);

  prm->files = tview;

  return hbox;
}

static GtkWidget *
create_control(GtkWidget *box, struct file_prm *prm)
{
  GtkWidget *w, *hbox;

  hbox = gtk_hbox_new(FALSE, 4);
  w = create_option_frame(prm);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

  w = create_geometry_frame(prm);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

  w = create_font_frame(&prm->font);
  gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 4);

  gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 4);

  w = create_file_frame(prm);
  gtk_box_pack_start(GTK_BOX(box), w, TRUE, TRUE, 4);

  return NULL;
}

static void
create_widgets(GtkWidget *vbox, struct file_prm *prm)
{
  GtkWidget *w;

  w = create_title(NAME " version " VERSION, "automatic legend generator");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

  w = create_control(vbox, prm);
}

static const char *
get_opt(int argc, char **argv, struct file_prm *prm)
{
  int i;
  char *data_file = NULL;

  for (i = 1; i < argc; i++) {
    if (strcmp(argv[i], "-x") == 0) {
      i++;
      prm->posx = atoi(argv[i]);
    } else if (strcmp(argv[i], "-y") == 0) {
      i++;
      prm->posy = atoi(argv[i]);
    } else if (strcmp(argv[i], "-w") == 0) {
      i++;
      prm->w = atoi(argv[i]);
    } else if (data_file == NULL) {
      data_file = argv[i];
    } else {
      prm->script = argv[i];
    }
  }

  return data_file;
}

int
main(int argc, char **argv)
{
  GtkWidget *mainwin;
  gint r;
  struct file_prm prm;
  const char *data_file;

#if GTK_CHECK_VERSION(2, 24, 0)
  setlocale(LC_ALL, "");
#else
  gtk_set_locale();
#endif
  gtk_init(&argc, &argv);

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
					GTK_STOCK_OK,   
					GTK_RESPONSE_ACCEPT,
					GTK_STOCK_CANCEL,   
					GTK_RESPONSE_REJECT,
					NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(mainwin), GTK_RESPONSE_ACCEPT);
  prm.window = mainwin;
  create_widgets(gtk_dialog_get_content_area(GTK_DIALOG(mainwin)), &prm);

  g_signal_connect(prm.mix, "toggled", G_CALLBACK(set_files), &prm);

  set_parameter(&prm);

  gtk_widget_show_all(mainwin);

  r = gtk_dialog_run(GTK_DIALOG(mainwin));
  if (r == GTK_RESPONSE_ACCEPT) {
    savescript(&prm);
  }

  return 0;
}

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
#define VERSION "1.01.02"

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

enum {
  PLOT_SOURCE_FILE,
  PLOT_SOURCE_ARRAY,
  PLOT_SOURCE_FUNC,
};

struct file_data {
  char *file, *array, *math_x, *math_y, *type, *caption, *style;
  int id, source, x, y, mark, size, width, r, g, b, r2, g2, b2, show, mix;
};

struct file_prm {
  GtkWidget *window, *x,*y, *width, *mix, *type, *caption, *frame, *shadow, *files;
  struct font_prm font;
  const char *script;
  struct file_data *data;
  int posx, posy, w, file_num;
};

#if GTK_CHECK_VERSION(4, 0, 0)
static GMainLoop *MainLoop;
#endif

/* Legend Object */
#define N_TYPE_LEGEND (n_legend_get_type())
G_DECLARE_FINAL_TYPE (NLegend, n_legend_, N, LEGEND, GObject)

typedef struct _NLegend NLegend;

struct _NLegend {
  GObject parent_instance;
  struct file_data *data;
  gchar *source;
};

G_DEFINE_TYPE(NLegend, n_legend, G_TYPE_OBJECT)

enum {
  LEGEND_PROP_ACTIVE = 1,
  LEGEND_PROP_ID,
  LEGEND_PROP_X,
  LEGEND_PROP_Y,
  LEGEND_PROP_SOURCE,
  LEGEND_PROP_CAPTION
};

static void
n_legend_set_property (GObject *object, guint prop_id, const GValue *value, GParamSpec *pspec)
{
  NLegend *self = N_LEGEND (object);
  switch (prop_id) {
  case LEGEND_PROP_ACTIVE:
    self->data->show = g_value_get_boolean (value);
    break;
  case LEGEND_PROP_ID:
    self->data->id = g_value_get_int (value);
    break;
  case LEGEND_PROP_X:
    self->data->x = g_value_get_int (value);
    break;
    case LEGEND_PROP_Y:
    self->data->y = g_value_get_int (value);
    break;
  case LEGEND_PROP_CAPTION:
    g_clear_pointer (&self->data->caption, g_free);
    self->data->caption = g_strdup(g_value_get_string (value));
    break;
  case LEGEND_PROP_SOURCE:
    g_clear_pointer (&self->source, g_free);
    self->source = g_strdup(g_value_get_string (value));
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
    break;
  }
}

static void
n_legend_get_property (GObject    *object,
			guint       property_id,
			GValue     *value,
			GParamSpec *pspec)
{
  NLegend *self = N_LEGEND (object);

  switch (property_id) {
  case LEGEND_PROP_ACTIVE:
    g_value_set_boolean (value, self->data->show);
    break;
  case LEGEND_PROP_ID:
    g_value_set_int (value, self->data->id);
    break;
  case LEGEND_PROP_X:
    g_value_set_int (value, self->data->x);
    break;
  case LEGEND_PROP_Y:
    g_value_set_int (value, self->data->y);
    break;
  case LEGEND_PROP_CAPTION:
    g_value_set_string (value, self->data->caption);
    break;
  case LEGEND_PROP_SOURCE:
    g_value_set_string (value, self->source);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, property_id, pspec);
    break;
  }
}

static void
n_legend_finalize (GObject *object)
{
  NLegend *self = N_LEGEND (object);

  g_clear_pointer (&self->source, g_free);
  G_OBJECT_CLASS (n_legend_parent_class)->finalize (object);
}

#define ID_MAX 65535

static void
n_legend_class_init (NLegendClass * klass)
{
  GParamSpec *pspec;
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = n_legend_finalize;
  object_class->get_property = n_legend_get_property;
  object_class->set_property = n_legend_set_property;

  pspec = g_param_spec_boolean ("active", NULL, NULL, TRUE, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, LEGEND_PROP_ACTIVE, pspec);

  pspec = g_param_spec_int ("id", NULL, NULL, 0, ID_MAX, 0, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, LEGEND_PROP_ID, pspec);

  pspec = g_param_spec_int ("x", NULL, NULL, 0, ID_MAX, 0, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, LEGEND_PROP_X, pspec);

  pspec = g_param_spec_int ("y", NULL, NULL, 0, ID_MAX, 0, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, LEGEND_PROP_Y, pspec);

  pspec = g_param_spec_string ("caption", NULL, NULL, NULL, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, LEGEND_PROP_CAPTION, pspec);

  pspec = g_param_spec_string ("source", NULL, NULL, NULL, G_PARAM_READWRITE);
  g_object_class_install_property (object_class, LEGEND_PROP_SOURCE, pspec);
}

static void
n_legend_init (NLegend * noop)
{
}

static NLegend *
n_legend_new (struct file_data *data, const char *source)
{
  NLegend *nobj;
  nobj = g_object_new (N_TYPE_LEGEND, NULL);
  nobj->data = data;
  nobj->source = g_strdup (source);

  return nobj;
}

static void
editable_label_changed (GtkEditable* self, GtkListItem *list_item)
{
  const char *str;
  GObject *item;
  NLegend *legend;

  item = gtk_list_item_get_item (list_item);
  legend = N_LEGEND (item);

  str = gtk_editable_get_text (self);

  g_clear_pointer (&legend->data->caption, g_free);
  legend->data->caption = g_strdup (str);
}

static void
setup_column (GtkSignalListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
  const char *id;
  id = user_data;
  if (g_strcmp0 (id, "active") == 0) {
    GtkWidget *btn = gtk_check_button_new ();
    gtk_list_item_set_child (list_item, btn);
  } else if (g_strcmp0 (id, "caption") == 0) {
    GtkWidget *label = gtk_editable_label_new ("");
    gtk_list_item_set_child (list_item, label);
    g_signal_connect (label, "changed", G_CALLBACK (editable_label_changed), list_item);
  } else {
    GtkWidget *label = gtk_label_new (NULL);
    if (g_strcmp0 (id, "source") == 0) {
      gtk_label_set_xalign (GTK_LABEL (label), 0);
    } else {
      gtk_label_set_xalign (GTK_LABEL (label), 1.0);
    }
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
    g_object_bind_property (item, prop, w, prop, G_BINDING_SYNC_CREATE | G_BINDING_BIDIRECTIONAL);
  } else if (g_strcmp0 (prop, "caption") == 0) {
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

  w = columnview_create(N_TYPE_LEGEND);
  columnview_create_column(w, "", G_CALLBACK(setup_column), G_CALLBACK(bind_column), "active");
  columnview_create_column(w, "#", G_CALLBACK(setup_column), G_CALLBACK(bind_column), "id");
  columnview_create_column(w, "source", G_CALLBACK(setup_column), G_CALLBACK(bind_column), "source");
  columnview_create_column(w, "x", G_CALLBACK(setup_column), G_CALLBACK(bind_column), "x");
  columnview_create_column(w, "y", G_CALLBACK(setup_column), G_CALLBACK(bind_column), "y");

  column = columnview_create_column(w, "caption", G_CALLBACK(setup_column), G_CALLBACK(bind_column), "caption");
  gtk_column_view_column_set_expand(column, TRUE);

  return w;
}

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
  char *str;

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
    char *hidden, *file;
    str = fgets_str(f);
    if (g_strcmp0(str, "array") == 0) {
      prm->data[i].source = PLOT_SOURCE_ARRAY;
    } else if (g_strcmp0(str, "function") == 0) {
      prm->data[i].source = PLOT_SOURCE_FUNC;
    } else {
      prm->data[i].source = PLOT_SOURCE_FILE;
    }
    prm->data[i].file = fgets_str(f);
    prm->data[i].array = fgets_str(f);
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
    str = NULL;
    file = NULL;
    if (prm->data[i].source == PLOT_SOURCE_ARRAY) {
      if (prm->data[i].array) {
	file = g_strdup(prm->data[i].array);
	str = escape_char(file, "_^@%\\", "\\");
      }
    } else if (prm->data[i].source == PLOT_SOURCE_FUNC) {
      file = g_strdup_printf("X=%s; Y=%s",
			     (prm->data[i].math_x[0]) ? prm->data[i].math_x : "X",
			     (prm->data[i].math_y[0]) ? prm->data[i].math_y : "Y");
      str = escape_char(file, "_^@%\\", "\\");
      prm->data[i].type = "line";
    } else {
      if (prm->data[i].file) {
	file = g_path_get_basename(prm->data[i].file);
	str = escape_char(file, "_^@%\\", "\\");
      }
    }
    prm->data[i].caption = str;
    if (file) {
      g_free(file);
    }
    for (j = 0; j < i; j++) {
      if ((prm->data[i].source == prm->data[j].source) &&
	  (g_strcmp0(prm->data[i].file, prm->data[j].file) == 0) &&
	  (g_strcmp0(prm->data[i].array, prm->data[j].array) == 0) &&
	  (prm->data[i].show == prm->data[j].show) &&
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
  } else if (g_strcmp0(data->type, "errorbar_x") == 0) {
    fprintf(f, "new path type=line\n");
    fprintf(f, "path::points='%d %d %d %d'\n", gx + width / 4, gy + h, gx + width * 3 / 4, gy + h);
    fprintf(f, "path::width=%d\n", data->width);
    if (data->style && data->style[0] != '\0') {
      fprintf(f, "path::style='%s'\n", data->style);
    }
    fprintf(f, "path::stroke_R=%d\n", data->r);
    fprintf(f, "path::stroke_G=%d\n", data->g);
    fprintf(f, "path::stroke_B=%d\n", data->b);
    fprintf(f, "path::marker_begin=bar\n");
    fprintf(f, "path::marker_end=bar\n");
    fprintf(f, "path::arrow_width=%d\n", data->size * 5000 / data->width);
  } else if (g_strcmp0(data->type, "errorbar_y") == 0) {
    fprintf(f, "new path type=line\n");
    fprintf(f, "path::points='%d %d %d %d'\n", gx + width / 2, gy + h + h / 2, gx + width / 2, gy + h / 2);
    fprintf(f, "path::width=%d\n", data->width);
    if (data->style && data->style[0] != '\0') {
      fprintf(f, "path::style='%s'\n", data->style);
    }
    fprintf(f, "path::stroke_R=%d\n", data->r);
    fprintf(f, "path::stroke_G=%d\n", data->g);
    fprintf(f, "path::stroke_B=%d\n", data->b);
    fprintf(f, "path::marker_begin=bar\n");
    fprintf(f, "path::marker_end=bar\n");
    fprintf(f, "path::arrow_width=%d\n", data->size * 5000 / data->width);
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
    fprintf(f, "path::marker_end=arrow\n");
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
      fprintf(f, "rectangle::stroke=true\n");
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
      fprintf(f, "rectangle::stroke=false\n");
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
  gboolean type, mix, frame, shadow, caption;
  char *str;
  const char *font;

  if (prm->script == NULL) {
    return 1;
  }

  f = g_fopen(prm->script, "w");
  if (f == NULL) {
    return 1;
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  type = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->type));
#else
  type = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->type));
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  mix = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->mix));
#else
  mix = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->mix));
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  frame = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->frame));
#else
  frame = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->frame));
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  shadow = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->shadow));
#else
  shadow = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->shadow));
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  caption = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->caption));
#else
  caption = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->caption));
#endif
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
    if (((prm->data[i].mix == -1) || (! mix )) && prm->data[i].show) {
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
    if (shadow) {
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
      fprintf(f, "movetop rectangle:!\n");
    }
    fprintf(f, "del iarray:textlen\n");
    fprintf(f, "del iarray:textbbox\n");
  }
  fprintf(f, "menu::modified=true\n");

  fclose(f);

  return 0;
}

static void
frame_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  int state;
  struct file_prm *prm;

  prm = (struct file_prm *) user_data;
  state = gtk_toggle_button_get_active(togglebutton);
  gtk_widget_set_sensitive(GTK_WIDGET(prm->shadow), state);
}

static GtkWidget *
create_option_frame(struct file_prm *prm)
{
  GtkWidget *frame, *vbox, *w;

  frame = gtk_frame_new("option");

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  w = gtk_check_button_new_with_mnemonic("_Mix");
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_check_button_set_active(GTK_CHECK_BUTTON(w), MIX);
#else
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), MIX);
#endif
  prm->mix = w;

  w = gtk_check_button_new_with_mnemonic("_Type");
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_check_button_set_active(GTK_CHECK_BUTTON(w), TYPE);
#else
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), TYPE);
#endif
  prm->type = w;

  w = gtk_check_button_new_with_mnemonic("_Caption");
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_check_button_set_active(GTK_CHECK_BUTTON(w), CAPTION);
#else
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), CAPTION);
#endif
  prm->caption = w;


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

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(frame), vbox);
#else
  gtk_container_add(GTK_CONTAINER(frame), vbox);
#endif

  return frame;
}

static GtkWidget *
create_geometry_frame(struct file_prm *prm)
{
  GtkWidget *frame, *w, *table;
  int j;

  frame = gtk_frame_new("geometry");

  table = gtk_grid_new();

  j = 0;
  w = create_spin_button(POS_MIN, POS_MAX, POS_INC, prm->posx / 100.0, 2);
  add_widget_to_table_sub(table, w, "_X:", TRUE, 0, 1, j++);
  prm->x = w;

  w = create_spin_button(POS_MIN, POS_MAX, POS_INC, prm->posy / 100.0, 2);
  add_widget_to_table_sub(table, w, "_Y:", TRUE, 0, 1, j++);
  prm->y = w;

  w = create_spin_button(1, 100, 10, prm->w / 100.0, 2);
  add_widget_to_table_sub(table, w, "_Width:", TRUE, 0, 1, j++);
  prm->width = w;

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(frame), table);
#else
  gtk_container_add(GTK_CONTAINER(frame), table);
#endif

  return frame;
}

static void
set_parameter(struct file_prm *prm)
{
  GListStore *list;
  int i, mix;
  char *caption;

  list = columnview_get_list (prm->files);
  g_list_store_remove_all (list);

  mix = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->mix));

  for (i = 0; i < prm->file_num; i++) {
    NLegend *item;
    if (mix && prm->data[i].mix >= 0) {
      continue;
    }

    if (prm->data[i].source == PLOT_SOURCE_ARRAY) {
      caption = g_strdup(prm->data[i].array);
    } else if (prm->data[i].source == PLOT_SOURCE_FUNC) {
      caption = g_strdup("function");
    } else {
      caption = g_path_get_basename(prm->data[i].file);
    }
    item = n_legend_new (&prm->data[i], caption);
    g_list_store_append (list, item);
    g_free(caption);
    g_object_unref(item);
  }
}

static void
set_files(GtkWidget *widget, gpointer user_data)
{
  struct file_prm *prm;

  prm = (struct file_prm *) user_data;
  set_parameter(prm);
}

static GtkWidget *
create_file_frame(struct file_prm *prm)
{
  GtkWidget *tview, *hbox, *swin, *frame;

  tview = columnview_column_create ();

#if GTK_CHECK_VERSION(4, 0, 0)
  swin = gtk_scrolled_window_new();
#else
  swin = gtk_scrolled_window_new(NULL, NULL);
#endif
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), tview);
#else
  gtk_container_add(GTK_CONTAINER(swin), tview);
#endif
  gtk_widget_set_size_request(GTK_WIDGET(swin), -1, 300);

  frame = gtk_frame_new(NULL);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(frame), swin);
#else
  gtk_container_add(GTK_CONTAINER(frame), swin);
#endif

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_set_hexpand(frame, TRUE);
  gtk_widget_set_vexpand(frame, TRUE);
  gtk_box_append(GTK_BOX(hbox), frame);
#else
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 4);
#endif

  prm->files = tview;

  return hbox;
}

static GtkWidget *
create_control(GtkWidget *box, struct file_prm *prm)
{
  GtkWidget *w, *hbox;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  w = create_option_frame(prm);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), w);
#else
  gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 4);
#endif

  w = create_geometry_frame(prm);
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
  gtk_box_pack_start(GTK_BOX(box), w, TRUE, TRUE, 4);
#endif

  return NULL;
}

static void
create_widgets(GtkWidget *vbox, struct file_prm *prm)
{
  GtkWidget *w;

  w = create_title(NAME " version " VERSION, "automatic legend generator");
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
#endif

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
      if (argv[i]) {
	prm->posx = atoi(argv[i]);
      }
    } else if (strcmp(argv[i], "-y") == 0) {
      i++;
      if (argv[i]) {
	prm->posy = atoi(argv[i]);
      }
    } else if (strcmp(argv[i], "-w") == 0) {
      i++;
      if (argv[i]) {
	prm->w = atoi(argv[i]);
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
dialog_response_cancel(GtkDialog* self, gpointer user_data)
{
  g_main_loop_quit(MainLoop);
}

static void
dialog_response_ok(GtkDialog* self, gpointer user_data)
{
  struct file_prm *prm;
  prm = user_data;
  savescript(prm);
  dialog_response_cancel(self, user_data);
}
#endif

int
main(int argc, char **argv)
{
  GtkWidget *mainwin, *vbox;
#if ! GTK_CHECK_VERSION(4, 0, 0)
  gint r;
#endif
  struct file_prm prm;
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

  mainwin = dialog_new (NAME, G_CALLBACK (dialog_response_cancel), G_CALLBACK (dialog_response_ok), &prm);
  g_signal_connect(mainwin, "destroy", G_CALLBACK(dialog_response_cancel), &prm);
  prm.window = mainwin;
  vbox = gtk_box_new (GTK_ORIENTATION_VERTICAL, 0);
  gtk_window_set_child (GTK_WINDOW (mainwin), vbox);
  create_widgets(vbox, &prm);

  g_signal_connect(prm.mix, "toggled", G_CALLBACK(set_files), &prm);

  set_parameter(&prm);

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_present (GTK_WINDOW (mainwin));
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

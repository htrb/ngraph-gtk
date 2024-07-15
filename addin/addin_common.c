#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

#include <stdlib.h>
#include <stdio.h>

#include "addin_common.h"

#define LINE_BUF_SIZE 8192

#define FONT_PT       20.00
#define FONT_SCRIPT   70.00
#define FONT_SPACE     0.00

static const char *FontList[] = {"Serif",  "Sans-serif", "Monospace", NULL};

GtkWidget *
create_text_entry(int set_default_action)
{
  GtkWidget *w;

  w = gtk_entry_new();
  if (set_default_action) {
    gtk_entry_set_activates_default(GTK_ENTRY(w), TRUE);
  }

  return w;
}

GtkWidget *
create_spin_button(double min, double max, double inc, double init, int digit)
{
  GtkWidget *w;

  w = gtk_spin_button_new_with_range(min, max, inc);
  gtk_editable_set_alignment(GTK_EDITABLE(w), 1.0);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), init);
  if (digit > 0) {
    gtk_spin_button_set_digits(GTK_SPIN_BUTTON(w), digit);
  }

  return w;
}

GtkWidget *
add_widget_to_table_sub(GtkWidget *table, GtkWidget *w, char *title, int expand, int col, int width, int n)
{
  GtkWidget *label;

  label = NULL;

  if (title) {
    label = gtk_label_new_with_mnemonic(title);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(table), label, col, n, 1, 1);
    col++;
  }

  if (w) {
    if (expand) {
      gtk_widget_set_hexpand(w, TRUE);
      gtk_widget_set_halign(w, GTK_ALIGN_FILL);
    } else {
      gtk_widget_set_halign(w, GTK_ALIGN_START);
    }
    gtk_grid_attach(GTK_GRID(table), w, col, n, width, 1);
  }

  return label;
}

char *
get_text_from_entry(GtkWidget *entry)
{
  const char *tmp;

  tmp = gtk_editable_get_text(GTK_EDITABLE(entry));
  if (tmp == NULL) {
    tmp = "";
  }

  return g_strdup(tmp);
}

GtkWidget *
create_title(const char *name, const char *comment)
{
  GtkWidget *frame, *label;

  frame = gtk_frame_new(name);
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.5);

  label = gtk_label_new(comment);
  gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label, GTK_ALIGN_CENTER);

  gtk_frame_set_child(GTK_FRAME(frame), label);

  return frame;
}

const char *
get_selected_font(struct font_prm *prm)
{
  int font, i;
  i = gtk_drop_down_get_selected(GTK_DROP_DOWN(prm->font));

  font = (i >= 1 && i < (int) (sizeof(FontList) / sizeof(*FontList) - 1)) ? i : 0;

  return FontList[font];
}

void
get_font_parameter(struct font_prm *prm, int *pt, int *spc, int *script, int *style, int *r, int *g, int *b)
{
  int bold, italic;
  const GdkRGBA *color;

  *pt = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->pt)) * 100;
  *script = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->script)) * 100;
  *spc = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->space)) * 100;

  bold = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->bold));
  italic = gtk_check_button_get_active(GTK_CHECK_BUTTON(prm->italic));
  *style = (bold ? 1 : 0) + (italic ? 2 : 0);

  color = gtk_color_dialog_button_get_rgba (GTK_COLOR_DIALOG_BUTTON (prm->color));
  *r = color->red * 255;
  *g = color->green * 255;
  *b = color->blue * 255;
}

GtkWidget *
create_font_frame(struct font_prm *prm)
{
  GtkWidget *frame, *w, *table, *hbox, *vbox;
  unsigned int j;
  GdkRGBA color;
  GtkColorDialog *color_dialog;

  frame = gtk_frame_new("font");

  table = gtk_grid_new();
  j = 0;

  w = gtk_drop_down_new_from_strings (FontList);

  gtk_drop_down_set_selected (GTK_DROP_DOWN (w), 1);
  add_widget_to_table_sub(table, w, "_Font:", TRUE, 0, 1, j++);
  prm->font = w;

  w = create_spin_button(6, 100, 1, FONT_PT, 2);
  add_widget_to_table_sub(table, w, "_Pt:", TRUE, 0, 1, j++);
  prm->pt = w;

  w = create_spin_button(0, 100, 1, FONT_SPACE, 2);
  add_widget_to_table_sub(table, w, "_Space:", TRUE, 0, 1, j++);
  prm->space = w;

  w = create_spin_button(10, 100, 10, FONT_SCRIPT, 2);
  add_widget_to_table_sub(table, w, "_Script:", TRUE, 0, 1, j++);
  prm->script = w;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  gtk_box_append(GTK_BOX(hbox), table);

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  w = gtk_check_button_new_with_mnemonic("_Bold");
  gtk_box_append(GTK_BOX(vbox), w);
  prm->bold = w;

  w = gtk_check_button_new_with_mnemonic("_Italic");
  gtk_box_append(GTK_BOX(vbox), w);
  prm->italic = w;

  color_dialog = gtk_color_dialog_new ();
  w = gtk_color_dialog_button_new(GTK_COLOR_DIALOG (color_dialog));
  gtk_box_append(GTK_BOX(vbox), w);
  prm->color = w;

  color.red = 0;
  color.green = 0;
  color.blue = 0;
  color.alpha = 1;
  gtk_color_dialog_button_set_rgba (GTK_COLOR_DIALOG_BUTTON (prm->color), &color);

  gtk_box_append(GTK_BOX(hbox), vbox);

  gtk_frame_set_child(GTK_FRAME(frame), hbox);

  return frame;
}

int
fgets_int(FILE *fp)
{
  char buf[LINE_BUF_SIZE], *r;
  int val;

  r = fgets(buf, sizeof(buf), fp);
  if (r == NULL) {
    return 0;
  }

  val = atoi(buf);

  return val;
}

double
fgets_double(FILE *fp)
{
  char buf[LINE_BUF_SIZE], *r;
  double val;

  r = fgets(buf, sizeof(buf), fp);
  if (r == NULL) {
    return 0;
  }

  val = atof(buf);

  return val;
}

char *
fgets_str(FILE *fp)
{
  char buf[LINE_BUF_SIZE], *r;
  char *str;

  r = fgets(buf, sizeof(buf), fp);
  if (r == NULL) {
    return NULL;
  }

  g_strchomp(buf);

  str = g_strcompress(buf);

  return str;
}

static gboolean
dialog_escape (GtkWidget* widget, GVariant* args, gpointer user_data)
{
  gtk_window_destroy (GTK_WINDOW (widget));
  return TRUE;
}

GtkWidget *
dialog_new(const char *title, GCallback cancel_cb, GCallback ok_cb, gpointer user_data)
{
  GtkWidget *mainwin, *headerbar, *ok, *cancel, *label;
  GObjectClass *class;
  GtkWidgetClass *widget_class;

  mainwin = gtk_window_new ();
  headerbar = gtk_header_bar_new ();
  label = gtk_label_new (title);
  gtk_header_bar_set_title_widget (GTK_HEADER_BAR (headerbar), label);
  gtk_header_bar_set_show_title_buttons (GTK_HEADER_BAR (headerbar), FALSE);
  gtk_window_set_titlebar (GTK_WINDOW (mainwin), headerbar);

  cancel = gtk_button_new_with_mnemonic ("_Cancel");
  g_signal_connect(cancel, "clicked", G_CALLBACK(cancel_cb), user_data);
  gtk_header_bar_pack_start (GTK_HEADER_BAR (headerbar), cancel);

  ok = gtk_button_new_with_mnemonic ("_Ok");
  gtk_widget_add_css_class (ok, "suggested-action");
  g_signal_connect(ok, "clicked", G_CALLBACK(ok_cb), user_data);
  gtk_header_bar_pack_end (GTK_HEADER_BAR (headerbar), ok);

  gtk_window_set_default_widget (GTK_WINDOW (mainwin), ok);

  class = G_OBJECT_GET_CLASS(mainwin);
  widget_class = GTK_WIDGET_CLASS (class);
  gtk_widget_class_add_binding (widget_class, GDK_KEY_Escape, 0, dialog_escape, NULL);

  return mainwin;
}

GtkWidget *
columnview_create(GType item_type)
{
  GtkWidget *columnview;
  GtkSelectionModel *selection;
  GListModel *model;

  columnview = gtk_column_view_new (NULL);
  gtk_column_view_set_show_column_separators (GTK_COLUMN_VIEW (columnview), TRUE);

  model = G_LIST_MODEL(g_list_store_new (item_type));
  selection = GTK_SELECTION_MODEL(gtk_single_selection_new (model));
  gtk_column_view_set_model (GTK_COLUMN_VIEW (columnview), selection);
  return columnview;
}

GtkColumnViewColumn *
columnview_create_column(GtkWidget *columnview, const char *header, GCallback setup, GCallback bind, gpointer user_data)
{
  GtkColumnViewColumn *column;
  GtkListItemFactory *factory;

  factory = gtk_signal_list_item_factory_new ();
  g_signal_connect (factory, "setup", setup, user_data);
  g_signal_connect (factory, "bind",  bind, user_data);

  column = gtk_column_view_column_new (header, factory);
  gtk_column_view_append_column (GTK_COLUMN_VIEW (columnview), column);

  return column;
}

GListStore *
columnview_get_list(GtkWidget *columnview)
{
  GtkSelectionModel *selection;
  GListModel *model;

  selection = gtk_column_view_get_model (GTK_COLUMN_VIEW (columnview));
  model = gtk_single_selection_get_model (GTK_SINGLE_SELECTION (selection));

  return G_LIST_STORE (model);
}

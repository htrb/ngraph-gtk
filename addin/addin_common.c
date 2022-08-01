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

static char *FontList[] = {"Serif",  "Sans-serif", "Monospace"};

#if GTK_CHECK_VERSION(4, 0, 0)
void
main_loop(void)
{
  while (g_list_model_get_n_items (gtk_window_get_toplevels ()) > 0) {
    g_main_context_iteration (NULL, TRUE);
  }
}
#endif

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
  gtk_entry_set_alignment(GTK_ENTRY(w), 1.0);
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
#if ! GTK_CHECK_VERSION(4, 0, 0)
    g_object_set(label, "margin", GINT_TO_POINTER(4), NULL);
#endif
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
#if ! GTK_CHECK_VERSION(4, 0, 0)
    g_object_set(w, "margin", GINT_TO_POINTER(4), NULL);
#endif
    gtk_grid_attach(GTK_GRID(table), w, col, n, width, 1);
  }

  return label;
}

char *
get_text_from_entry(GtkWidget *entry)
{
  const char *tmp;

#if GTK_CHECK_VERSION(4, 0, 0)
  tmp = gtk_editable_get_text(GTK_EDITABLE(entry));
#else
  tmp = gtk_entry_get_text(GTK_ENTRY(entry));
#endif
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
#if ! GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  g_object_set(frame, "margin", GINT_TO_POINTER(4), NULL);
  g_object_set(frame, "border-width", GINT_TO_POINTER(4), NULL);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.5);
#else
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.5);
#endif

  label = gtk_label_new(comment);
#if ! GTK_CHECK_VERSION(4, 0, 0)
  g_object_set(label, "margin", GINT_TO_POINTER(4), NULL);
#endif
  gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label, GTK_ALIGN_CENTER);

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(frame), label);
#else
  gtk_container_add(GTK_CONTAINER(frame), label);
#endif

  return frame;
}

const char *
get_selected_font(struct font_prm *prm)
{
  int font, i;
  i = gtk_combo_box_get_active(GTK_COMBO_BOX(prm->font));

  font = (i >= 1 && i < (int) (sizeof(FontList) / sizeof(*FontList))) ? i : 0;

  return FontList[font];
}

void
get_font_parameter(struct font_prm *prm, int *pt, int *spc, int *script, int *style, int *r, int *g, int *b)
{
  int bold, italic;
  GdkRGBA color;

  *pt = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->pt)) * 100;
  *script = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->script)) * 100;
  *spc = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->space)) * 100;

  bold = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->bold));
  italic = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->italic));
  *style = (bold ? 1 : 0) + (italic ? 2 : 0);

  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(prm->color), &color);
  *r = color.red * 255;
  *g = color.green * 255;
  *b = color.blue * 255;
}

GtkWidget *
create_font_frame(struct font_prm *prm)
{
  GtkWidget *frame, *w, *table, *hbox, *vbox;
  unsigned int j, i;
  GdkRGBA color;

  frame = gtk_frame_new("font");

  table = gtk_grid_new();
  j = 0;

  w = gtk_combo_box_text_new();
  for (i = 0; i < sizeof(FontList) / sizeof(*FontList); i++) {
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w), FontList[i]);
  }
  gtk_combo_box_set_active(GTK_COMBO_BOX(w), 1);
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
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), table);
#else
  gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 4);
#endif

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  w = gtk_check_button_new_with_mnemonic("_Bold");
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
#endif
  prm->bold = w;

  w = gtk_check_button_new_with_mnemonic("_Italic");
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
#endif
  prm->italic = w;

  w = gtk_color_button_new();
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), w);
#else
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
#endif
  prm->color = w;

  color.red = 0;
  color.green = 0;
  color.blue = 0;
  color.alpha = 1;
  gtk_color_chooser_set_rgba(GTK_COLOR_CHOOSER(prm->color), &color);

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), vbox);
#else
  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(frame), hbox);
#else
  gtk_container_add(GTK_CONTAINER(frame), hbox);
#endif

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

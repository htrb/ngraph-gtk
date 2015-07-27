#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

#include <stdlib.h>
#include <stdio.h>

#include "addin_common.h"

#define LINE_BUF_SIZE 1024

#define FONT_PT       20.00
#define FONT_SCRIPT   70.00
#define FONT_SPACE     0.00

static char *FontList[] = {"Serif",  "Sans-serif", "Monospace"};

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
#if ! GTK_CHECK_VERSION(3, 4, 0)
  GtkWidget *align;
  int x, y;

  g_object_get(table, "n-columns", &x, "n-rows", &y, NULL);

  y = (y > n + 1) ? y : n + 1;
  gtk_table_resize(GTK_TABLE(table), y, x);
#endif

  label = NULL;

  if (title) {
    label = gtk_label_new_with_mnemonic(title);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
#if GTK_CHECK_VERSION(3, 4, 0)
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    g_object_set(label, "margin", GINT_TO_POINTER(4), NULL);
    gtk_grid_attach(GTK_GRID(table), label, col, n, 1, 1);
#else
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, n, n + 1, GTK_FILL, 0, 4, 4);
#endif
    col++;
  }

  if (w) {
#if GTK_CHECK_VERSION(3, 4, 0)
    if (expand) {
      gtk_widget_set_hexpand(w, TRUE);
      gtk_widget_set_halign(w, GTK_ALIGN_FILL);
    } else {
      gtk_widget_set_halign(w, GTK_ALIGN_START);
    }
    g_object_set(w, "margin", GINT_TO_POINTER(4), NULL);
    gtk_grid_attach(GTK_GRID(table), w, col, n, width, 1);
#else
    align = gtk_alignment_new(0, 0.5, (expand) ? 1 : 0, 0);
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, col, col + width, n, n + 1, ((expand) ? GTK_EXPAND : 0) | GTK_FILL, 0, 4, 4);
#endif
  }

  return label;
}

int
warning_dialog(GtkWidget *parent, const char *msg, const char *str)
{
  GtkWidget *dlg;
  int r;

  dlg = gtk_message_dialog_new(GTK_WINDOW(parent), GTK_DIALOG_MODAL, GTK_MESSAGE_WARNING,GTK_BUTTONS_YES_NO, msg, str);
  r = gtk_dialog_run(GTK_DIALOG(dlg));
  gtk_widget_destroy(dlg);

  return r;
}

char *
get_text_from_entry(GtkWidget *entry)
{
  const char *tmp;

  tmp = gtk_entry_get_text(GTK_ENTRY(entry));
  if (tmp == NULL) {
    tmp = "";
  }

  return g_strdup(tmp);
}

GtkWidget *
create_title(const char *name, const char *comment)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  GtkWidget *frame, *label;

  frame = gtk_frame_new(name);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
  g_object_set(frame, "margin", GINT_TO_POINTER(4), NULL);
  g_object_set(frame, "border-width", GINT_TO_POINTER(4), NULL);
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.5);

  label = gtk_label_new(comment);
  g_object_set(label, "margin", GINT_TO_POINTER(4), NULL);
  gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
  gtk_widget_set_valign(label, GTK_ALIGN_CENTER);

  gtk_container_add(GTK_CONTAINER(frame), label);

  return frame;
#else
  GtkWidget *vbox, *frame, *label, *hbox;

  vbox = gtk_vbox_new(FALSE, 4);
  frame = gtk_frame_new(name);
  gtk_frame_set_label_align(GTK_FRAME(frame), 0.5, 0.5);

  label = gtk_label_new(comment);
  gtk_misc_set_alignment(GTK_MISC(label), 0.5, 0.5);
  gtk_box_pack_start(GTK_BOX(vbox), label, FALSE, FALSE, 4);

  gtk_container_add(GTK_CONTAINER(frame), vbox);

  hbox = gtk_hbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 4);

  return hbox;
#endif
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
#if GTK_CHECK_VERSION(3, 4, 0)
  GdkRGBA color;
#else
  GdkColor color;
#endif

  *pt = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->pt)) * 100;
  *script = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->script)) * 100;
  *spc = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->space)) * 100;

  bold = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->bold));
  italic = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->italic));
  *style = (bold ? 1 : 0) + (italic ? 2 : 0);

#if GTK_CHECK_VERSION(3, 4, 0)
  gtk_color_chooser_get_rgba(GTK_COLOR_CHOOSER(prm->color), &color);
  *r = color.red * 255;
  *g = color.green * 255;
  *b = color.blue * 255;
#else
  gtk_color_button_get_color(GTK_COLOR_BUTTON(prm->color), &color);
  *r = color.red >> 8;
  *g = color.green >> 8;
  *b = color.blue >> 8;
#endif
}

GtkWidget *
create_font_frame(struct font_prm *prm)
{
  GtkWidget *frame, *w, *table, *hbox, *vbox;
  unsigned int j, i;

  frame = gtk_frame_new("font");

#if GTK_CHECK_VERSION(3, 4, 0)
  table = gtk_grid_new();
#else
  table = gtk_table_new(1, 2, FALSE);
#endif
  j = 0;

#if GTK_CHECK_VERSION(2, 24, 0)
  w = gtk_combo_box_text_new();
  for (i = 0; i < sizeof(FontList) / sizeof(*FontList); i++) {
    gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(w), FontList[i]);
  }
#else
  w = gtk_combo_box_new_text();
  for (i = 0; i < sizeof(FontList) / sizeof(*FontList); i++) {
    gtk_combo_box_append_text(GTK_COMBO_BOX(w), FontList[i]);
  }
#endif
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

#if GTK_CHECK_VERSION(3, 0, 0)
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
  hbox = gtk_hbox_new(FALSE, 4);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 4);

#if GTK_CHECK_VERSION(3, 0, 0)
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);
#else
  vbox = gtk_vbox_new(FALSE, 4);
#endif

  w = gtk_check_button_new_with_mnemonic("_Bold");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
  prm->bold = w;

  w = gtk_check_button_new_with_mnemonic("_Italic");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
  prm->italic = w;

  w = gtk_color_button_new();
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
  prm->color = w;

  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);

  gtk_container_add(GTK_CONTAINER(frame), hbox);

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

  str = g_strdup(buf);

  return str;
}

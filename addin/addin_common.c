#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>

#include <stdlib.h>
#include <stdio.h>

#define LINE_BUF_SIZE 1024

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
add_widget_to_table_sub(GtkWidget *table, GtkWidget *w, char *title, int expand, int col, int width, int n)
{
  GtkWidget *align, *label;
  int x, y;

  g_object_get(table, "n-columns", &x, "n-rows", &y, NULL);

  y = (y > n + 1) ? y : n + 1;
  gtk_table_resize(GTK_TABLE(table), y, x);

  label = NULL;

  if (title) {
    label = gtk_label_new_with_mnemonic(title);
    gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
    gtk_misc_set_alignment(GTK_MISC(label), 0, 0.5);
    gtk_table_attach(GTK_TABLE(table), label, col, col + 1, n, n + 1, GTK_FILL, 0, 4, 4);
    col++;
  }

  if (w) {
    align = gtk_alignment_new(0, 0.5, (expand) ? 1 : 0, 0);
    gtk_container_add(GTK_CONTAINER(align), w);
    gtk_table_attach(GTK_TABLE(table), align, col, col + width, n, n + 1, ((expand) ? GTK_EXPAND : 0) | GTK_FILL, 0, 4, 4);
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
    return 0;
  }

  g_strchomp(buf);

  str = g_strdup(buf);

  return str;
}

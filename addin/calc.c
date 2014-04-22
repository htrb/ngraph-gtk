#include <locale.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "addin_common.h"

#define NAME    "Calc"
#define VERSION "1.00.03"

#define DATA_FILE "#calc#.dat"
#define MINIMUM   "0.0"
#define MAXIMUM   "1.0"
#define DIVISION  100
#define FORMULA   "X"
#define INCMIN    TRUE
#define INCMAX    TRUE
#define SETDATA   TRUE

#define LINE_BUF_SIZE 1024

struct calc_prm {
  GtkWidget *window, *min,*max, *div, *inc_min, *inc_max, *output,
    *minimum, *maximum, *division, *set_data, *formula;
  const char *script;
};

static void
set_double_to_entry(GtkWidget *entry, double val)
{
  char buf[LINE_BUF_SIZE];

  snprintf(buf, sizeof(buf), "%G", val);
  gtk_entry_set_text(GTK_ENTRY(entry), buf);
}

static double
get_double_from_entry(GtkWidget *entry)
{
  const char *str;
  char *ptr;
  double val;

  str = gtk_entry_get_text(GTK_ENTRY(entry));
  val = g_ascii_strtod(str, &ptr);

  return val;
}

static void
load_settings(struct calc_prm *prm)
{
  double min, max;
  int div, imin, imax, n;
  char *output, *ptr;
  char buf[LINE_BUF_SIZE], tag[LINE_BUF_SIZE];
  char s1[LINE_BUF_SIZE], s2[LINE_BUF_SIZE], s3[LINE_BUF_SIZE];
  FILE *fp;

  output = get_text_from_entry(prm->output);

  fp = g_fopen(output, "r");
  g_free(output);
  if (fp == NULL) {
    return;
  }

  ptr = fgets(buf, sizeof(buf), fp);
  if (ptr == NULL) {
    fclose(fp);
    return;
  }

  n = sscanf(buf, "-s%64s -mx%64s -my%64s", s1, s2, s3);
  if (n != 3) {
    fclose(fp);
    return;
  }

  ptr = fgets(buf, sizeof(buf), fp);
  if (ptr == NULL) {
    fclose(fp);
    return;
  }

  n = sscanf(buf, "%64s %lf %lf %d %d %d", tag, &min, &max, &div, &imin, &imax);
  if (n != 6) {
    fclose(fp);
    return;
  }

  fclose(fp);

  if (strcmp(tag, "%CALC.NSC") != 0) {
    return;
  }

  set_double_to_entry(prm->minimum, min);
  set_double_to_entry(prm->maximum, max);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(prm->division), div);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prm->inc_min), imin);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prm->inc_max), imax);
  gtk_entry_set_text(GTK_ENTRY(prm->formula), s3);
}

static void
load_clicked(GtkButton *button, gpointer user_data)
{
  struct calc_prm *prm;

  prm = (struct calc_prm *) user_data;
  load_settings(prm);
}

static GtkWidget *
create_control(struct calc_prm *prm)
{
  GtkWidget *table, *w;
  int j;

#if GTK_CHECK_VERSION(3, 4, 0)
  table = gtk_grid_new();
#else
  table = gtk_table_new(1, 3, FALSE);
#endif

  j = 0;
  w = create_text_entry(TRUE);
  add_widget_to_table_sub(table, w, "_Output:", TRUE, 0, 2, j++);
  gtk_entry_set_text(GTK_ENTRY(w), DATA_FILE);
  prm->output = w;

  w = create_text_entry(TRUE);
  add_widget_to_table_sub(table, w, "_Formula:", TRUE, 0, 1, j);
  gtk_entry_set_text(GTK_ENTRY(w), FORMULA);
  prm->formula = w;

  w = gtk_button_new_with_mnemonic("_Load");
  add_widget_to_table_sub(table, w, NULL, FALSE, 2, 1, j++);
  g_signal_connect(w, "clicked", G_CALLBACK(load_clicked), prm);


  w = create_text_entry(TRUE);
  add_widget_to_table_sub(table, w, "_Minimum:", TRUE, 0, 1, j);
  gtk_entry_set_text(GTK_ENTRY(w), MINIMUM);
  prm->minimum = w;

  w = gtk_check_button_new_with_mnemonic("_Include min");
  add_widget_to_table_sub(table, w, NULL, FALSE, 2, 1, j++);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), INCMIN);
  prm->inc_min = w;


  w = create_text_entry(TRUE);
  add_widget_to_table_sub(table, w, "_Maximum:", TRUE, 0, 1, j);
  gtk_entry_set_text(GTK_ENTRY(w), MAXIMUM);
  prm->maximum = w;

  w = gtk_check_button_new_with_mnemonic("_Include max");
  add_widget_to_table_sub(table, w, NULL, FALSE, 2, 1, j++);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), INCMAX);
  prm->inc_max = w;

  w = gtk_spin_button_new_with_range(1, 100000, 10);
  add_widget_to_table_sub(table, w, "_Division:", TRUE, 0, 1, j++);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), DIVISION);
  prm->division = w;

  w = gtk_check_button_new_with_mnemonic("open as _Data");
  add_widget_to_table_sub(table, w, NULL, FALSE, 0, 2, j++);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), SETDATA);
  prm->set_data = w;

  return table;
}

static void
create_widgets(GtkWidget *vbox, struct calc_prm *prm)
{
  GtkWidget *w;


  w = create_title(NAME " version " VERSION, "making a data file");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

  w = create_control(prm);
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
}

static void
remove_space_quotation(char *str)
{
  int len, i;

  if (str == NULL) {
    return;
  }

  len = strlen(str);
  for (i = 0; i < len; i++) {
    if (g_ascii_isspace(str[i]) || str[i] == '\'') {
      memmove(str + i, str + i + 1, len - i);
    }
  }
}

static int
save_script(const struct calc_prm *prm)
{
  FILE *fp;
  char *file = NULL, *eqn;

  if (prm->script == NULL) {
    return 0;
  }

  fp = g_fopen(prm->script, "w");
  if (fp == NULL) {
    return 1;
  }


  fprintf(fp, "new file\n");

  file = get_text_from_entry(prm->output);
  fprintf(fp, "file::file=%s\n", file);
  g_free(file);

  fprintf(fp, "file::head_skip=2\n");
  fprintf(fp, "file::type=line\n");

  eqn = get_text_from_entry(prm->formula);
  remove_space_quotation(eqn);
  fprintf(fp, "file::math_y='%s'\n", eqn);
  g_free(eqn);

  fprintf(fp, "menu::modified=true\n");

  fclose(fp);

  return 0;
}

static int
save_data(const struct calc_prm *prm)
{
  double min, max, x;
  int start, end, div, inc_min, inc_max, r, i;
  FILE *fp;
  char *output, *eqn;

  div = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(prm->division));

  inc_min = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->inc_min));
  start = (inc_min) ? 0 : 1;

  inc_max = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->inc_max));
  end = (inc_max) ? div : div - 1;

  output = get_text_from_entry(prm->output);
  if (g_file_test(output, G_FILE_TEST_EXISTS)) {
    r = warning_dialog(prm->window, "Overwrite existing file? (%s)", output);
    if (r == GTK_RESPONSE_NO) {
      g_free(output);
      return -1;
    }
  }

  fp = g_fopen(output, "w");
  if (fp == NULL) {
    g_free(output);
    return 1;
  }

  eqn = get_text_from_entry(prm->formula);
  remove_space_quotation(eqn);
  fprintf(fp, "-s2 -mxX -my%s\n", eqn);
  g_free(eqn);

  min = get_double_from_entry(prm->minimum);
  max = get_double_from_entry(prm->maximum);

  fprintf(fp, "%%CALC.NSC %G %G %d %d %d\n", min, max, div, inc_min, inc_max);

  for (i = start; i <= end; i++) {
    x = min + (max - min) / div * i;
    fprintf(fp, "%G %G\n", x, x);
  }
  fclose(fp);

  g_free(output);

  return 0;
}

int
main(int argc, char **argv)
{
  GtkWidget *mainwin;
  gint r;
  struct calc_prm prm;

  prm.script = (argc < 2) ? NULL : argv[1];

#if GTK_CHECK_VERSION(2, 24, 0)
  setlocale(LC_ALL, "");
#else
  gtk_set_locale();
#endif
  gtk_init(&argc, &argv);

  mainwin = gtk_dialog_new_with_buttons(NAME, NULL, 0,
					"_Cancel",   
					GTK_RESPONSE_REJECT,
					"_OK",   
					GTK_RESPONSE_ACCEPT,
					NULL);
  gtk_dialog_set_default_response(GTK_DIALOG(mainwin), GTK_RESPONSE_ACCEPT);
  prm.window = mainwin;
  create_widgets(gtk_dialog_get_content_area(GTK_DIALOG(mainwin)), &prm);

  load_settings(&prm);
  gtk_widget_show_all(mainwin);

  while (1) {
    r = gtk_dialog_run(GTK_DIALOG(mainwin));
    if (r != GTK_RESPONSE_ACCEPT) {
      return 0;
    }

    r = save_data(&prm);
    if (r == 0) {
      break;
    } else if (r > 0) {
      return 0;
    }
  }

  if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm.set_data))) {
    save_script(&prm);
  }

  return 0;
}

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
#define VERSION "1.00.01"

#define POS_X   50.00
#define POS_Y   50.00
#define POS_INC  1.00
#define ACCURACY 5
#define DIVISION  100
#define FORMULA   "X"
#define ADD_PLUS  FALSE
#define EXPAND    TRUE
#define FRAME     TRUE
#define FONT_PT       20.00
#define FONT_SCRIPT   70.00
#define FONT_SPACE     0.00

#define LINE_BUF_SIZE 1024
#define PRM_NUM       10

#define POS_MIN   -1000
#define POS_MAX    1000

static char *FontList[] = {"Serif",  "Sans-serif", "Monospace"};

struct fit_data {
  int file_id;
  char *file;
  int id;
  char *type;
  int poly;
  char *userfunc;
  double prm[PRM_NUM];
};

struct caption_widget {
  GtkWidget *check, *caption, *val, *label;
};

struct fit_prm {
  GtkWidget *window, *x,*y, *add_plus, *accuracy, *expand, *frame, *combo,
    *font, *font_pt, *font_space, *font_script, *font_color,
    *font_bold, *font_italic;
  struct caption_widget caption[PRM_NUM];
  const char *script;
  struct fit_data *data;
  int posx, posy, fit_num;
};

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
    prm->data[i].file_id = fgets_int(f);
    prm->data[i].file = fgets_str(f);
    prm->data[i].id = fgets_int(f);
    prm->data[i].type = fgets_str(f);
    prm->data[i].poly = fgets_int(f);
    prm->data[i].userfunc = fgets_str(f);
    for (j = 0; j < PRM_NUM; j++) {
      prm->data[i].prm[j] = fgets_double(f);
    }
  }
  fclose(f);

  return 0;
}

static void
makescript(FILE *f, struct fit_prm *prm, int gx, int gy, int height, const char *cap, const char *val)
{
  int style, textpt, textspc, textsc, textred, textblue, textgreen, bold, italic, frame, font;
  GdkColor color;

  textpt = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->font_pt)) * 100;
  textsc = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->font_script)) * 100;
  textspc = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->font_space)) * 100;

  bold = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->font_bold));
  italic = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->font_italic));
  style = (bold ? 1 : 0) + (italic ? 2 : 0);

  gtk_color_button_get_color(GTK_COLOR_BUTTON(prm->font_color), &color);
  textred = color.red >> 8;
  textgreen = color.green >> 8;
  textblue = color.blue >> 8;

  font = gtk_combo_box_get_active(GTK_COMBO_BOX(prm->font));

  frame = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->frame));

  fprintf(f, "new text\n");
  fprintf(f, "text::text='%s %s'\n", cap, val);
  fprintf(f, "text::x=%d\n", gx);
  fprintf(f, "text::y=%d\n", gy + height);
  fprintf(f, "text::pt=%d\n", textpt);
  fprintf(f, "text::font=%s\n", FontList[font]);
  fprintf(f, "text::space=%d\n", textspc);
  fprintf(f, "text::script_size=%d\n", textsc);
  fprintf(f, "text::R=%d\n", textred);
  fprintf(f, "text::G=%d\n", textgreen);
  fprintf(f, "text::B=%d\n", textblue);
  fprintf(f, "text::style=%d\n", style);
  if (frame) {
    fprintf(f, "iarray:textbbox:@=${text::bbox}\n");
    fprintf(f, "int:textlen:@=\"${iarray:textbbox:get:2}-${iarray:textbbox:get:0}\"\n");
    fprintf(f, "if [ \"${int:texttot:@}\" -lt \"${int:textlen:@}\" ]; then\n");
    fprintf(f, "int:texttot:@=${int:textlen:@}\n");
    fprintf(f, "fi\n");
  }
  fprintf(f, "menu::modified=true\n");
}

static int
savescript(struct fit_prm *prm)
{
  char *cap, *val;
  FILE *f;
  int frame, height, textpt, gx, gy, posx, posy, h_inc, i;

  if (prm->script == NULL) {
    return 0;
  }

  f = g_fopen(prm->script, "w");

  posx = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->x)) * 100;
  posy = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->y)) * 100;

  frame = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->frame));
  if (frame) {
    fprintf(f, "new int name:textlen\n");
    fprintf(f, "new int name:texttot\n");
    fprintf(f, "new iarray name:textbbox\n");
  }

  textpt = gtk_spin_button_get_value(GTK_SPIN_BUTTON(prm->font_pt)) * 100;
  height = ceil(textpt * 25.4 / 72.0 / 100) * 100;
  gy = posy;

  h_inc = ceil(height * 1.2 / 100) * 100;

  for (i = 0; i < PRM_NUM; i++) {
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->caption[i].check))) {
      gx = posx;
      cap = get_text_from_entry(prm->caption[i].caption);
      val = get_text_from_entry(prm->caption[i].val);
      makescript(f, prm, gx, gy, height, cap, val);
      gy += h_inc;

      g_free(cap);
      g_free(val);
    }
  }

  if (frame) {
    fprintf(f, "new rectangle\n");
    fprintf(f, "rectangle::x1=%d\n", posx - height / 4);
    fprintf(f, "rectangle::y1=%d\n", posy);
    fprintf(f, "rectangle::x2=%d+${int:texttot:@}\n", posx + 3 * height / 4);
    fprintf(f, "rectangle::y2=%d\n", gy + height / 2);
    fprintf(f, "rectangle::fill_R=0\n");
    fprintf(f, "rectangle::fill_G=0\n");
    fprintf(f, "rectangle::fill_B=0\n");
    fprintf(f, "rectangle::fill=true\n");
    fprintf(f, "new rectangle\n");
    fprintf(f, "rectangle::x1=%d\n", posx - height / 2);
    fprintf(f, "rectangle::y1=%d\n", posy - height / 4);
    fprintf(f, "rectangle::x2=%d+${int:texttot:@}\n", posx + height / 2);
    fprintf(f, "rectangle::y2=%d\n", gy + height / 4);
    fprintf(f, "rectangle::fill_R=255\n");
    fprintf(f, "rectangle::fill_G=255\n");
    fprintf(f, "rectangle::fill_B=255\n");
    fprintf(f, "rectangle::stroke_R=0\n");
    fprintf(f, "rectangle::stroke_G=0\n");
    fprintf(f, "rectangle::stroke_B=0\n");
    fprintf(f, "rectangle::fill=true\n");
    fprintf(f, "rectangle::stroke=true\n");
    fprintf(f, "del int:textlen\n");
    fprintf(f, "del int:texttot\n");
    fprintf(f, "del iarray:textbbox\n");
  }

  fclose(f);

  return 0;
}

static GtkWidget *
create_spin_button(const char *title, double min, double max, double inc, double init, GtkWidget **hbox)
{
  GtkWidget *w, *label;

  *hbox = gtk_hbox_new(FALSE, 4);
  label = gtk_label_new_with_mnemonic(title);
  gtk_box_pack_start(GTK_BOX(*hbox), label, FALSE, FALSE, 4);

  w = gtk_spin_button_new_with_range(min, max, inc);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), init);
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), w);
  gtk_box_pack_start(GTK_BOX(*hbox), w, TRUE, TRUE, 4);

  return w;
}


static GtkWidget *
create_format_frame(struct fit_prm *prm)
{
  GtkWidget *frame, *vbox, *w, *hbox;

  frame = gtk_frame_new("format");

  vbox = gtk_vbox_new(FALSE, 4);

  w = gtk_check_button_new_with_mnemonic("add _+");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), ADD_PLUS);
  prm->add_plus = w;

  w = gtk_check_button_new_with_mnemonic("_Expand");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), EXPAND);
  prm->expand = w;

  w = gtk_check_button_new_with_mnemonic("_Frame");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 2);
  gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(w), FRAME);
  prm->frame = w;


  w = create_spin_button("_Accuracy:", 1, 15, 1, ACCURACY, &hbox);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 2);
  prm->accuracy = w;


  gtk_container_add(GTK_CONTAINER(frame), vbox);

  return frame;
}

static GtkWidget *
create_position_frame(struct fit_prm *prm)
{
  GtkWidget *frame, *w, *table;
  int j;

  frame = gtk_frame_new("position");

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

  gtk_container_add(GTK_CONTAINER(frame), table);

  return frame;
}

static GtkWidget *
create_font_frame(struct fit_prm *prm)
{
  GtkWidget *frame, *w, *table, *hbox, *vbox;
  unsigned int j, i;

  frame = gtk_frame_new("font");

  table = gtk_table_new(1, 2, FALSE);
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

  w = gtk_spin_button_new_with_range(6, 100, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), FONT_PT);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(w), 2);
  add_widget_to_table_sub(table, w, "_Pt:", TRUE, 0, 1, j++);
  prm->font_pt = w;

  w = gtk_spin_button_new_with_range(0, 100, 1);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), FONT_SPACE);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(w), 2);
  add_widget_to_table_sub(table, w, "_Space:", TRUE, 0, 1, j++);
  prm->font_space = w;

  w = gtk_spin_button_new_with_range(10, 100, 10);
  gtk_spin_button_set_value(GTK_SPIN_BUTTON(w), FONT_SCRIPT);
  gtk_spin_button_set_digits(GTK_SPIN_BUTTON(w), 2);
  add_widget_to_table_sub(table, w, "_Script:", TRUE, 0, 1, j++);
  prm->font_script = w;

  hbox = gtk_hbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hbox), table, FALSE, FALSE, 4);

  vbox = gtk_vbox_new(FALSE, 4);

  w = gtk_check_button_new_with_mnemonic("_Bold");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
  prm->font_bold = w;

  w = gtk_check_button_new_with_mnemonic("_Italic");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
  prm->font_italic = w;

  w = gtk_color_button_new();
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 4);
  prm->font_color = w;

  gtk_box_pack_start(GTK_BOX(hbox), vbox, FALSE, FALSE, 4);

  gtk_container_add(GTK_CONTAINER(frame), hbox);

  return frame;
}

static void
caption_toggled(GtkToggleButton *togglebutton, gpointer user_data)
{
  int state;
  struct caption_widget *caption;

  caption = (struct caption_widget *) user_data;

  state = gtk_toggle_button_get_active(togglebutton);

  gtk_widget_set_sensitive(caption->label, state);
  gtk_widget_set_sensitive(caption->caption, state);
  gtk_widget_set_sensitive(caption->val, state);
}

static void
set_parameter(struct fit_prm *prm)
{
  int i, j, accuracy, expand, add_plus, dim[PRM_NUM];
  char buf[LINE_BUF_SIZE], fmt[LINE_BUF_SIZE];

  i = gtk_combo_box_get_active(GTK_COMBO_BOX(prm->combo));

  accuracy = gtk_spin_button_get_value_as_int(GTK_SPIN_BUTTON(prm->accuracy));
  expand = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->expand));
  add_plus = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(prm->add_plus));

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

  for (j = 0; j < PRM_NUM; j++) {
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(prm->caption[j].check), dim[j]);
    snprintf(fmt, sizeof(fmt),
	     "%%#%s.%dg",
	     add_plus ? "+" : "",
	     accuracy);
    if (expand) {
      snprintf(buf, sizeof(buf),
	       fmt,
	       prm->data[i].prm[j]);
    } else {
      snprintf(buf, sizeof(buf),
	       "%%pf{%s %%{fit:%d:%%%02d}}",
	       fmt,
	       prm->data[i].id,
	       j);
    }
    gtk_entry_set_text(GTK_ENTRY(prm->caption[j].val), buf);
  }
}

static void
file_changed(GtkWidget *widget, gpointer user_data)
{
  struct fit_prm *prm;

  prm = (struct fit_prm *) user_data;
  set_parameter(prm);
}

static GtkWidget *
create_caption_frame(struct fit_prm *prm)
{
  GtkWidget *frame, *table, *w, *label, *hbox;
  int i;
  char buf[LINE_BUF_SIZE];
  struct caption_widget *caption;

  frame = gtk_frame_new("caption");

  table = gtk_table_new(1, 4, FALSE);

  for (i = 0; i < PRM_NUM; i++) {
    caption = prm->caption + i;

    snprintf(buf, sizeof(buf), "%%0_%d ", i);
    w = gtk_check_button_new_with_mnemonic(buf);
    g_signal_connect(w, "toggled", G_CALLBACK(caption_toggled), caption);
    add_widget_to_table_sub(table, w, NULL, FALSE, 0, 1, i);
    caption->check = w;

    snprintf(buf, sizeof(buf), "\\%%%02d: ", i);
    w = create_text_entry(TRUE);
    label = add_widget_to_table_sub(table, w, "caption:", TRUE, 1, 1, i);
    gtk_entry_set_text(GTK_ENTRY(w), buf);
    caption->label = label;
    caption->caption = w;

    w = create_text_entry(TRUE);
    add_widget_to_table_sub(table, w, NULL, TRUE, 3, 1, i);
    caption->val = w;

    caption_toggled(GTK_TOGGLE_BUTTON(caption->check), caption);
  }

  gtk_container_add(GTK_CONTAINER(frame), table);

  hbox = gtk_hbox_new(FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hbox), frame, TRUE, TRUE, 4);

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

  hbox = gtk_hbox_new(FALSE, 4);
  label = gtk_label_new_with_mnemonic("_Data:");
  gtk_label_set_mnemonic_widget(GTK_LABEL(label), combo);

  gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 4);
  gtk_box_pack_start(GTK_BOX(hbox), combo, TRUE, TRUE, 4);

  return hbox;
}

static GtkWidget *
create_control(GtkWidget *box, struct fit_prm *prm)
{
  GtkWidget *w, *hbox;

  hbox = gtk_hbox_new(FALSE, 4);
  w = create_format_frame(prm);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

  w = create_position_frame(prm);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

  w = create_font_frame(prm);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);

  gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 4);

  w = create_file_frame(prm);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 4);

  w = create_caption_frame(prm);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 4);

  return NULL;
}

static void
create_widgets(GtkWidget *vbox, struct fit_prm *prm)
{
  GtkWidget *w;

  w = create_title(NAME " version " VERSION, "fitting results ---> legend-text");
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);

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
      prm->posx = atoi(argv[i]);
    } else if (strcmp(argv[i], "-y") == 0) {
      i++;
      prm->posy = atoi(argv[i]);
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
  struct fit_prm prm;
  const char *data_file;

#if GTK_CHECK_VERSION(2, 24, 0)
  setlocale(LC_ALL, "");
#else
  gtk_set_locale();
#endif
  gtk_init(&argc, &argv);

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

  g_signal_connect(prm.combo, "changed", G_CALLBACK(file_changed), &prm);
  g_signal_connect(prm.add_plus, "toggled", G_CALLBACK(file_changed), &prm);
  g_signal_connect(prm.expand, "toggled", G_CALLBACK(file_changed), &prm);
  g_signal_connect(prm.accuracy, "value-changed", G_CALLBACK(file_changed), &prm);

  gtk_combo_box_set_active(GTK_COMBO_BOX(prm.combo), 0);


  gtk_widget_show_all(mainwin);

  r = gtk_dialog_run(GTK_DIALOG(mainwin));
  if (r == GTK_RESPONSE_ACCEPT) {
    savescript(&prm);
  }

  return 0;
}

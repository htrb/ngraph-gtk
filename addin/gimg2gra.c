/**************************************************************************

              gimg2gra Version 0.00 Copyright (C) H.Ito 2002

 **************************************************************************/
#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <errno.h>
#include <limits.h>

#include "addin_common.h"

#define PRGNAME "gimg2gra"
#define VERSION "0.0.0"

#define WIDTH  0
#define HEIGHT 1
#define DPI_MAX 2540

struct AppData {
  GdkPixbuf *im;
  gchar *gra;
  GtkWidget *entry;
  gint dpi;
};

struct Color {
  int r, g, b, a;
};

static struct Color BGCOLOR = {255, 255, 255, 255};
static int DotSize = 36;

struct rectangle{
  unsigned char r, g, b, a;
  int x1, y1, x2, y2;
};
static GtkWidget *App = NULL;

static GMainLoop *MainLoop;

static void create_widgets(GtkWidget *app, struct AppData *app_data, const gchar *img_file);
static void print_error_exit(const gchar *error);
static void set_bgcolor(int r, int g, int b, int a, struct AppData *data);

static void create_entry(GtkWidget *hbox, struct AppData *data);
static void create_buttons(struct AppData *data, GtkWidget *hbox);

static void button_press_event(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer user_data);
static void save_button_clicked(GtkButton *widget, gpointer data);
static void cancel_button_clicked(GtkButton *widget, gpointer data);

static int  rectcmp(const void *tmpa, const void *tmpb);
static void fputcolor(FILE *fp, const struct Color *color);
static int  colorcmp(const struct Color *a, const struct Color *b);
static int  gra_set_dpi(int dpi);
static void gra_set_bgcolor(gint r, gint g, gint b, gint a);
static int  gra_save(GdkPixbuf *im, const char *gra_file);

int
main(int argc, char *argv[])
{
  static gchar *img_file = NULL, *gra_file = NULL;
  struct AppData app_data;

  MainLoop = g_main_loop_new (NULL, FALSE);
  gtk_init();
  App = gtk_window_new();

  if (argc != 4) {
    static char *usage = "Usage: %s resolution image_file gra_file\n";
    const gchar *error;
    error = g_strdup_printf(usage, g_path_get_basename(argv[0]));
    print_error_exit(error);
    g_main_loop_run(MainLoop);
    exit(1);
  }
  gra_file = argv[3];
  img_file = argv[2];
  app_data.dpi = atoi(argv[1]);
  if(gra_file == NULL) {
    gra_file = g_strconcat(g_path_get_basename(img_file), ".gra", NULL);
  }

  app_data.gra = gra_file;
  create_widgets(App, &app_data, img_file);
  g_main_loop_run(MainLoop);

  return 0;
}

static void
create_widgets(GtkWidget *app, struct AppData *app_data, const gchar *img_file)
{
  GtkWidget *w, *vbox, *hbox, *event_box, *scrolled_window;
  GdkPixbuf *pixbuf;
  GError *error;
  GtkGesture *gesture;

  app_data->entry = NULL;

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  error = NULL;
  pixbuf = gdk_pixbuf_new_from_file(img_file, &error);
  if (pixbuf == NULL) {
    print_error_exit(error->message);
  }
  w = gtk_picture_new_for_filename(img_file);
  gtk_picture_set_can_shrink(GTK_PICTURE(w), FALSE);
  gtk_picture_set_content_fit(GTK_PICTURE(w), GTK_CONTENT_FIT_SCALE_DOWN);
  gtk_widget_set_halign(GTK_WIDGET(w), GTK_ALIGN_START);
  gtk_widget_set_valign(GTK_WIDGET(w), GTK_ALIGN_START);
  app_data->im = pixbuf;
  gtk_widget_set_hexpand(w, TRUE);
  gtk_widget_set_vexpand(w, TRUE);
  event_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_append(GTK_BOX(event_box), w);

  gesture = gtk_gesture_click_new();
  gtk_widget_add_controller(w, GTK_EVENT_CONTROLLER(gesture));

  gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 1);
  g_signal_connect(gesture, "pressed", G_CALLBACK(button_press_event), app_data);

  scrolled_window = gtk_scrolled_window_new();
  gtk_widget_set_size_request(scrolled_window, 800, 600);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), event_box);

  gtk_box_append(GTK_BOX(vbox), scrolled_window);
  gtk_box_append(GTK_BOX(vbox), hbox);

  create_buttons(app_data, hbox);

  g_signal_connect_swapped(app, "close-request", G_CALLBACK(g_main_loop_quit), MainLoop);
  gtk_window_set_child(GTK_WINDOW(app), vbox);

  gtk_window_present (GTK_WINDOW (app));
}

static void
dialog_response(GObject* self, GAsyncResult* res, gpointer user_data)
{
  g_main_loop_quit(MainLoop);
}

static void
print_error_exit(const gchar *error)
{
  GtkAlertDialog *dialog;

  dialog = gtk_alert_dialog_new ("Error loading file\n%s", error);
  gtk_alert_dialog_choose (dialog, GTK_WINDOW (App), NULL, dialog_response, NULL);
}

static void
create_buttons(struct AppData *data, GtkWidget *box)
{
  GtkWidget *w, *vbox, *hbox;

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  create_entry(vbox, data);
  gtk_box_append(GTK_BOX(box), vbox);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  w = gtk_button_new_with_label(" OK ");
  g_signal_connect(w, "clicked", G_CALLBACK(save_button_clicked), data);
  gtk_box_append(GTK_BOX(hbox), w);

  w = gtk_button_new_with_label(" Cancel ");
  g_signal_connect(w, "clicked", G_CALLBACK(cancel_button_clicked), NULL);
  gtk_box_append(GTK_BOX(hbox), w);

  gtk_box_append(GTK_BOX(box), hbox);
}

static void
create_entry(GtkWidget *box, struct AppData *data)
{
  GtkWidget *w, *hbox, *entry = NULL;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  w = gtk_label_new("BGCOLOR:");
  gtk_box_append(GTK_BOX(hbox), w);

  entry = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(entry), 10);
  gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
  gtk_box_append(GTK_BOX(hbox), entry);

  data->entry = entry;
  set_bgcolor(255, 255, 255, 255, data);

  gtk_box_append(GTK_BOX(box), hbox);
}

static void
set_bgcolor(int r, int g, int b, int a, struct AppData *data)
{
  static char bgcolor[64];

  gra_set_bgcolor(r, g, b, a);
  sprintf(bgcolor, "#%02x%02x%02x%02x", r, g, b, a);
  if(data->entry != NULL) {
    gtk_editable_set_text(GTK_EDITABLE(data->entry), bgcolor);
  }
}

/************* Event Handler **************/

static void
button_press_event(GtkGestureClick *gesture, gint n_press, gdouble x, gdouble y, gpointer data)
{
  struct AppData *app_data = (struct AppData *) data;
  GdkPixbuf *im;
  int i, w, h, r, g, b, a, rowstride, alpha, bpp;
  guchar *pixels;

  im = app_data->im;
  w = gdk_pixbuf_get_width(im);
  h = gdk_pixbuf_get_height(im);
  rowstride = gdk_pixbuf_get_rowstride(im);
  pixels = gdk_pixbuf_get_pixels(im);
  alpha = gdk_pixbuf_get_has_alpha(im);
  bpp = rowstride / w;
  if(x >= w || y >= h)
    return;

  i = (int) y * rowstride + (int) x * bpp;
  r = pixels[i];
  g = pixels[i + 1];
  b = pixels[i + 2];
  a = (alpha) ? pixels[i + 3] : 255;
  set_bgcolor(r, g, b, a, app_data);
}

static void
save_button_clicked(GtkButton *widget, gpointer data)
{
  struct AppData *app_data = (struct AppData *) data;
  GdkPixbuf *im;
  const gchar *grafile;
  int dotsize, w, h, r;

  im = app_data->im;
  grafile = app_data->gra;
  w = gdk_pixbuf_get_width(im);
  h = gdk_pixbuf_get_height(im);

  dotsize = gra_set_dpi(app_data->dpi);
  r = gra_save(im, grafile);
  printf("%d %d\n", w * dotsize, h * dotsize);
  if (! r) {
    g_main_loop_quit(MainLoop);
  }
}

static void
cancel_button_clicked(GtkButton *widget, gpointer data)
{
  g_main_loop_quit(MainLoop);
}

static void
gra_set_bgcolor(gint r, gint g, gint b, gint a)
{
  if(r >= 0 && r < 256 && g >= 0 && g < 256 && b >= 0 && b < 256 && a >= 0 && a < 256){
    BGCOLOR.r = r;
    BGCOLOR.g = g;
    BGCOLOR.b = b;
    BGCOLOR.a = a;
  }
}

static int
gra_set_dpi(int dpi)
{
  if(dpi > 0 && dpi<= DPI_MAX) {
    DotSize = DPI_MAX / dpi;
  }

  return DotSize;
}

static int
gra_save(GdkPixbuf *im, const char *gra_file)
{
  gint w, h, x, x1, y, i, j, rowstride, alpha, bpp, offset;
  struct Color shape_color, color, color2;
  struct rectangle *rect;
  FILE *fp;
  guchar *pixels;

  if(im == NULL)
    return 1;

  w = gdk_pixbuf_get_width(im);
  h = gdk_pixbuf_get_height(im);

  if(INT_MAX / sizeof(*rect) / w / h < 1 || INT_MAX / w / DotSize < 1 || INT_MAX / h / DotSize < 1){
    const gchar *error;
    error = g_strdup_printf("Too large image.\n");
    print_error_exit(error);
    return 1;
  }
  rect = g_malloc(sizeof(*rect) * w * h);

  if(gra_file == NULL){
    fp = stdout;
  }else if((fp = fopen(gra_file, "wt")) == NULL){
    print_error_exit(g_strerror(errno));
    return 1;
  }

  fprintf(fp, "%%Ngraph GRAF\n"
	  "%%Creator: %s\n"
	  "I,5,0,0,%d,%d,10000\n"
	  "V,5,0,0,%d,%d,1\n"
	  "A,5,0,1,0,0,1000\n",
	  PRGNAME,
	  w * DotSize, h * DotSize,
	  w * DotSize, h * DotSize);

  rowstride = gdk_pixbuf_get_rowstride(im);
  pixels = gdk_pixbuf_get_pixels(im);
  alpha = gdk_pixbuf_get_has_alpha(im);
  bpp = rowstride / w;

  shape_color = BGCOLOR;
  color = BGCOLOR;
  fputcolor(fp, &BGCOLOR);
  fprintf(fp, "B,5,0,0,%d,%d,1\n", w * DotSize, h * DotSize);

  j = 0;
  for(y = 0; y < h; y++){
    offset = y * rowstride;
    for(x = x1 = 0; x < w; x++, offset += bpp){
      color2.r = pixels[offset];
      color2.g = pixels[offset + 1];
      color2.b = pixels[offset + 2];
      color2.a = (alpha) ? pixels[offset + 3] : 255;

      if(colorcmp(&color2, &color)){
	if(x != 0 && colorcmp(&shape_color, &color)){
	  rect[j].r = color.r;
	  rect[j].g = color.g;
	  rect[j].b = color.b;
	  rect[j].a = color.a;
	  rect[j].x1 = x1 * DotSize;
	  rect[j].y1 = y * DotSize;
	  rect[j].x2 = x * DotSize;
	  rect[j].y2 = (y + 1) * DotSize;
	  j++;
	}
	color = color2;
	x1 = x;
      }
      if(x == w - 1 && colorcmp(&shape_color, &color)){
	rect[j].r = color.r;
	rect[j].g = color.g;
	rect[j].b = color.b;
	rect[j].a = color.a;
	rect[j].x1 = x1 * DotSize;
	rect[j].y1 = y * DotSize;
	rect[j].x2 = (x + 1) * DotSize;
	rect[j].y2 = (y + 1) * DotSize;
	j++;
      }
    }
  }

  qsort(rect, j, sizeof(*rect), rectcmp);
  color.r = rect[0].r;
  color.g = rect[0].g;
  color.b = rect[0].b;
  color.a = rect[0].a;
  fputcolor(fp, &color);
  for(i = 0; i < j; i++){
    color2.r = rect[i].r;
    color2.g = rect[i].g;
    color2.b = rect[i].b;
    color2.a = rect[i].a;
    if(colorcmp(&color2, &color)){
      fputcolor(fp, &color2);
      color = color2;
    }
    if (color2.a > 0) {
      fprintf(fp, "B,5,%d,%d,%d,%d,1\n", rect[i].x1, rect[i].y1, rect[i].x2, rect[i].y2);
    }
  }
  fprintf(fp, "E,0\n");

  if(ferror(fp)){
    print_error_exit(g_strerror(errno));
    return 1;
  }

  if(fp != stdout) {
    fclose(fp);
  }

  g_free(rect);
  return 0;
}

static int
colorcmp(const struct Color *a, const struct Color *b)
{
  return ! (a->r == b->r && a->g == b->g && a->b == b->b && a->a == b->a);
}

static int
rectcmp(const void *tmpa, const void *tmpb)
{
  const struct rectangle *a = (struct rectangle *)tmpa,  *b = (struct rectangle *)tmpb;
  int c, d;

  c = (((int)a->r)<<16) + (((int)a->g)<<8) + a->b;
  d = (((int)b->r)<<16) + (((int)b->g)<<8) + b->b;
  return d - c;
}

static void
fputcolor(FILE *fp, const struct Color *color)
{
  if (color->a > 0) {
    fprintf(fp, "G,4,%d,%d,%d,%d\n", color->r, color->g, color->b, color->a);
  }
}

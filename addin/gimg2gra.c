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

static GtkWidget *create_widgets(struct AppData *app_data, const gchar *img_file);
static void print_error_exit(const gchar *error);
static void set_bgcolor(int r, int g, int b, int a, struct AppData *data);

static void create_entry(GdkPixbuf *im, GtkWidget *hbox, struct AppData *data);
static void create_buttons(struct AppData *data, GtkWidget *hbox);

#if ! GTK_CHECK_VERSION(4, 0, 0)
static gboolean delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
static gboolean button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
#endif
static void save_button_clicked(GtkButton *widget, gpointer data);
static void cancel_button_clicked(GtkButton *widget, gpointer data);

static int  rectcmp(const void *tmpa, const void *tmpb);
static void fputcolor(FILE *fp, struct Color *color);
static int  colorcmp(struct Color *a, struct Color *b);
static int  gra_set_dpi(int dpi);
static void gra_set_bgcolor(gint r, gint g, gint b, gint a);
static int  gra_save(GdkPixbuf *im, char *gra_file);

int
main(int argc, char *argv[])
{
  static gchar *img_file = NULL, *gra_file = NULL;
  struct AppData app_data;

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_init();
#else
  gtk_init(&argc, &argv);
#endif
  if (argc != 4) {
    static char *usage = "Usage: %s resolution image_file gra_file\n";
    gchar *error;
    error = g_strdup_printf(usage, g_path_get_basename(argv[0]));
    print_error_exit(error);
  }
  gra_file = argv[3];
  img_file = argv[2];
  app_data.dpi = atoi(argv[1]);
  if(gra_file == NULL) {
    gra_file = g_strconcat(g_path_get_basename(img_file), ".gra", NULL);
  }

  app_data.gra = gra_file;
  App = create_widgets(&app_data, img_file);
#if GTK_CHECK_VERSION(4, 0, 0)
  main_loop();
#else
  gtk_main();
#endif

  return 0;
}

static GtkWidget *
create_widgets(struct AppData *app_data, const gchar *img_file)
{
  GtkWidget *w, *vbox, *hbox, *event_box, *scrolled_window, *app;
  GdkPixbuf *pixbuf;
  GError *error;

  app_data->entry = NULL;

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  error = NULL;
  pixbuf = gdk_pixbuf_new_from_file(img_file, &error);
  if (pixbuf == NULL) {
    print_error_exit(error->message);
  }
  w = gtk_image_new_from_pixbuf(pixbuf);
#if GTK_CHECK_VERSION(4, 0, 0)
  app_data->im = pixbuf;
  event_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_append(GTK_BOX(event_box), w);
#else
  app_data->im = gtk_image_get_pixbuf(GTK_IMAGE(w));
  event_box = gtk_event_box_new();
  gtk_container_add(GTK_CONTAINER(event_box), w);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  g_signal_connect(event_box, "button-press-event", G_CALLBACK(button_press_event), app_data);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  scrolled_window = gtk_scrolled_window_new();
#else
  scrolled_window = gtk_scrolled_window_new(NULL, NULL);
#endif
  gtk_widget_set_size_request(scrolled_window, 800, 600);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(scrolled_window), event_box);
#else
  gtk_container_add(GTK_CONTAINER(scrolled_window), event_box);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), scrolled_window);
  gtk_box_append(GTK_BOX(vbox), hbox);
#else
  gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 5);
#endif

  create_buttons(app_data, hbox);

#if GTK_CHECK_VERSION(4, 0, 0)
  app = gtk_window_new();
#else
  app = gtk_window_new(GTK_WINDOW_TOPLEVEL);
#endif

#if ! GTK_CHECK_VERSION(4, 0, 0)
  g_signal_connect(app, "delete-event", G_CALLBACK(delete_event), NULL);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_set_child(GTK_WINDOW(app), vbox);
#else
  gtk_container_add(GTK_CONTAINER(app), vbox);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(app);
#else
  gtk_widget_show_all(app);
#endif
  return app;
}

#if GTK_CHECK_VERSION(4, 0, 0)
static void
dialog_response(GtkDialog* self, gint response_id, gpointer user_data)
{
  exit(1);
}
#endif

static void
print_error_exit(const gchar *error)
{
  GtkWidget *dialog;

  dialog = gtk_message_dialog_new(GTK_WINDOW(App),
				  GTK_DIALOG_DESTROY_WITH_PARENT,
				  GTK_MESSAGE_ERROR,
				  GTK_BUTTONS_CLOSE,
				  "Error loading file %s",
				  error);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
  g_signal_connect(dialog, "response", G_CALLBACK(dialog_response), NULL);
  gtk_widget_show(dialog);
  main_loop();
#else
  gtk_dialog_run(GTK_DIALOG(dialog));
  gtk_widget_destroy(dialog);
#endif

  exit(1);
}

static void
create_buttons(struct AppData *data, GtkWidget *box)
{
  GtkWidget *w, *vbox, *hbox;
  GdkPixbuf *im;

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);

  im = data->im;

  create_entry(im, vbox, data);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(box), vbox);
#else
  gtk_box_pack_start(GTK_BOX(box), vbox, FALSE, FALSE, 10);
#endif

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  w = gtk_button_new_with_label(" OK ");
  g_signal_connect(w, "clicked", G_CALLBACK(save_button_clicked), data);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), w);
#else
  gtk_box_pack_start(GTK_BOX(hbox), w, TRUE, TRUE, 5);
#endif

  w = gtk_button_new_with_label(" Cancel ");
  g_signal_connect(w, "clicked", G_CALLBACK(cancel_button_clicked), NULL);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), w);

  gtk_box_append(GTK_BOX(box), hbox);
#else
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 5);

  gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 5);
#endif
}

static void
create_entry(GdkPixbuf *im, GtkWidget *box, struct AppData *data)
{
  GtkWidget *w, *hbox, *entry = NULL;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  w = gtk_label_new("BGCOLOR:");
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), w);
#else
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);
#endif

  entry = gtk_entry_new();
  gtk_entry_set_max_length(GTK_ENTRY(entry), 10);
  gtk_editable_set_editable(GTK_EDITABLE(entry), FALSE);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), entry);
#else
  gtk_box_pack_start(GTK_BOX(hbox), entry, FALSE, FALSE, 0);
#endif

  data->entry = entry;
  set_bgcolor(255, 255, 255, 255, data);

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(box), hbox);
#else
  gtk_box_pack_start(GTK_BOX(box), hbox, FALSE, FALSE, 0);
#endif
}

static void
set_bgcolor(int r, int g, int b, int a, struct AppData *data)
{
  static char bgcolor[64];

  gra_set_bgcolor(r, g, b, a);
  sprintf(bgcolor, "#%02x%02x%02x%02x", r, g, b, a);
  if(data->entry != NULL) {
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_editable_set_text(GTK_EDITABLE(data->entry), bgcolor);
#else
    gtk_entry_set_text(GTK_ENTRY(data->entry), bgcolor);
#endif
  }
}

/************* Event Handler **************/
#if ! GTK_CHECK_VERSION(4, 0, 0)
static gboolean
delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
  gtk_main_quit();
  return TRUE;
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
static gboolean
button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
  struct AppData *app_data = (struct AppData *) data;
  GdkPixbuf *im;
  int i, x = event->x,  y = event->y, w, h, r, g, b, a, rowstride, alpha, bpp;
  guchar *pixels;

  im = app_data->im;
  w = gdk_pixbuf_get_width(im);
  h = gdk_pixbuf_get_height(im);
  rowstride = gdk_pixbuf_get_rowstride(im);
  pixels = gdk_pixbuf_get_pixels(im);
  alpha = gdk_pixbuf_get_has_alpha(im);
  bpp = rowstride / w;

  if(x >= w || y >= h)
    return TRUE;

  i = y * rowstride + x * bpp;
  switch(event->button){
  case 1:
    r = pixels[i];
    g = pixels[i + 1];
    b = pixels[i + 2];
    a = (alpha) ? pixels[i + 3] : 255;
    set_bgcolor(r, g, b, a, app_data);
    break;
  }

  return TRUE;
}
#endif

static void
save_button_clicked(GtkButton *widget, gpointer data)
{
  struct AppData *app_data = (struct AppData *) data;
  GdkPixbuf *im;
  gchar *grafile;
  int dotsize, w, h;

  im = app_data->im;
  grafile = app_data->gra;
  w = gdk_pixbuf_get_width(im);
  h = gdk_pixbuf_get_height(im);

  dotsize = gra_set_dpi(app_data->dpi);
  gra_save(im, grafile);
  printf("%d %d\n", w * dotsize, h * dotsize);
#if GTK_CHECK_VERSION(4, 0, 0)
  exit(0);
#else
  gtk_main_quit();
#endif
}

static void
cancel_button_clicked(GtkButton *widget, gpointer data)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  exit(0);
#else
  gtk_main_quit();
#endif
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
gra_save(GdkPixbuf *im, char *gra_file)
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
    gchar *error;
    error = g_strdup_printf("Too large image.\n");
    print_error_exit(error);
  }
  rect = g_malloc(sizeof(*rect) * w * h);

  if(gra_file == NULL){
    fp = stdout;
  }else if((fp = fopen(gra_file, "wt")) == NULL){
    print_error_exit(g_strerror(errno));
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
  }

  if(fp != stdout) {
    fclose(fp);
  }

  g_free(rect);
  return 0;
}

static int
colorcmp(struct Color *a, struct Color *b)
{
  return ! (a->r == b->r && a->g == b->g && a->b == b->b && a->a == b->a);
}

static int
rectcmp(const void *tmpa, const void *tmpb)
{
  struct rectangle *a = (struct rectangle *)tmpa,  *b = (struct rectangle *)tmpb;
  int c, d;

  c = (((int)a->r)<<16) + (((int)a->g)<<8) + a->b;
  d = (((int)b->r)<<16) + (((int)b->g)<<8) + b->b;
  return d - c;
}

static void fputcolor(FILE *fp, struct Color *color)
{
  if (color->a > 0) {
    fprintf(fp, "G,4,%d,%d,%d,%d\n", color->r, color->g, color->b, color->a);
  }
}

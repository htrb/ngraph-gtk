/* --*-coding:utf-8-*-- */
/* 
 * $Id: x11menu.c,v 1.116 2010-04-01 06:08:23 hito Exp $
 */

#include "gtk_common.h"

#include <sys/types.h>
#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <math.h>
#include <time.h>
#include <limits.h>
#include <unistd.h>

#include "ngraph.h"
#include "object.h"
#include "ioutil.h"
#include "shell.h"
#include "nstring.h"
#include "nconfig.h"
#include "mathfn.h"
#include "gra.h"
#include "spline.h"

#include "gtk_entry_completion.h"
#include "gtk_subwin.h"
#include "gtk_widget.h"

#include "main.h"
#include "x11bitmp.h"
#include "x11dialg.h"
#include "ogra2cairo.h"
#include "ogra2gdk.h"
#include "ox11menu.h"
#include "x11menu.h"
#include "x11info.h"
#include "x11cood.h"
#include "x11gui.h"
#include "x11merge.h"
#include "x11commn.h"
#include "x11lgnd.h"
#include "x11axis.h"
#include "x11file.h"
#include "x11graph.h"
#include "x11print.h"
#include "x11opt.h"
#include "x11view.h"

#define TEXT_HISTORY     "text_history"
#define MATH_X_HISTORY   "math_x_history"
#define MATH_Y_HISTORY   "math_y_history"
#define FUNCTION_HISTORY "function_history"
#define KEYMAP_FILE      "accel_map"

#define USE_EXT_DRIVER 0

int Menulock = FALSE, DnDLock = FALSE;
struct NgraphApp NgraphApp;
GtkWidget *TopLevel = NULL;
GtkAccelGroup *AccelGroup = NULL;

static GtkWidget *CurrentWindow = NULL;
static enum {APP_CONTINUE, APP_QUIT, APP_QUIT_FORCE} Hide_window = APP_CONTINUE;
static int Toggle_cb_disable = FALSE, DrawLock = FALSE;
static unsigned int CursorType;
static GtkWidget *ShowFileWin = NULL, *ShowAxisWin = NULL,
  *ShowLegendWin = NULL, *ShowMergeWin = NULL, *ShowCoodinateWin = NULL,
  *ShowInfoWin = NULL, *RecentData = NULL, *SaveMenuItem = NULL,
  *AddinMenu = NULL, *EditCut = NULL,
  *EditCopy = NULL, *EditPaste = NULL, *EditDelete = NULL,
  *RotateCW = NULL, *RotateCCW = NULL, *FlipH = NULL, *FlipV = NULL,
  *EditAlign = NULL, *ToggleStatusBar = NULL, *ToggleScrollbar = NULL,
  *ToggleRuler = NULL, *TogglePToobar = NULL, *ToggleCToobar = NULL,
  *ToggleCrossGauge = NULL, *MathBtn = NULL, *AxisUndoBtn = NULL,
  *AxisUndoMenuItem = NULL, *SaveBtn = NULL;

#if USE_EXT_DRIVER
static GtkWidget *ExtDrvOutMenu = NULL
#endif

static void CmReloadWindowConfig(GtkMenuItem *w, gpointer user_data);
static void script_exec(GtkWidget *w, gpointer client_data);
static void set_widget_visibility(int cross);

GdkCursorType Cursor[] = {
  GDK_LEFT_PTR,
  GDK_XTERM,
  GDK_CROSSHAIR,
  GDK_TOP_LEFT_CORNER,
  GDK_TOP_RIGHT_CORNER,
  GDK_BOTTOM_RIGHT_CORNER,
  GDK_BOTTOM_LEFT_CORNER,
  GDK_TARGET,
  GDK_SIZING,
  GDK_SIZING,
  GDK_WATCH,
  GDK_FLEUR,
  GDK_PENCIL,
  GDK_TCROSS,
};

#define CURSOR_TYPE_NUM (sizeof(Cursor) / sizeof(*Cursor))

struct command_data {
  void (*func)(GtkWidget *, gpointer);
  gchar *label, *tip, *caption;
  const char **xpm;
  const char *stock;
  int type;
  GtkWidget *img;
  GtkWidget **button;
};

static struct command_data Command1_data[] = {
  {
    CmFileWindow,
    N_("Data"),
    "Data Window",
    N_("Activate Data Window"), 
    Filewin_xpm,
    NULL,
    0,
    NULL,
    NULL,
  },
  {
    CmAxisWindow,
    N_("Axis"),
    "Axis Window",
    N_("Activate Axis Window"), 
    Axiswin_xpm,
    NULL,
    0,
    NULL,
    NULL,
  },
  {
    CmLegendWindow,
    N_("Legend"),
    "Legend Window",
    N_("Activate Legend Window"), 
    Legendwin_xpm,
    NULL,
    0,
    NULL,
    NULL,
  },
  {
    CmMergeWindow,
    N_("Merge"),
    "Merge Window",
    N_("Activate Merge Window"), 
    Mergewin_xpm,
    NULL,
    0,
    NULL,
    NULL,
  },
  {
    CmCoordinateWindow,
    N_("Coordinate"),
    "Coordinate Window",
    N_("Activate Coordinate Window"), 
    Coordwin_xpm,
    NULL,
    0,
    NULL,
    NULL,
  },
  {
    CmInformationWindow,
    N_("Information"),
    "Information Window",
    N_("Activate Information Window"), 
    Infowin_xpm,
    NULL,
    0,
    NULL,
    NULL,
  },
  {NULL},
  {
    CmFileOpenB,
    N_("Open"),
    N_("Open Data"),
    N_("Open Data file"), 
    //    Fileopen_xpm,
    NULL,
    GTK_STOCK_FILE,
    0,
    NULL,
    NULL,
  },
  {NULL},
  {
    CmGraphLoadB,
    N_("Load"),
    N_("Load NGP"),
    N_("Load NGP file"), 
    //    Load_xpm,
    NULL,
    GTK_STOCK_OPEN,
    0,
    NULL,
    NULL,
  },
  {
    CmGraphSaveB,
    N_("Save"),
    N_("Save NGP"),
    N_("Save NGP file"),
    //    Save_xpm,
    NULL,
    GTK_STOCK_SAVE,
    0,
    NULL,
    &SaveBtn,
  },
  {NULL},
  {
    CmAxisClear,
    N_("Clear"),
    N_("Clear Scale"),
    N_("Clear Scale"), 
    Scale_xpm,
    NULL,
    0,
    NULL,
    NULL,
  },
  {
    CmViewerDrawB,
    N_("Draw"),
    N_("Draw"),
    N_("Draw on Viewer Window"), 
    Draw_xpm,
    NULL,
    0,
    NULL,
    NULL,
  },
  {
    CmViewerClearB,
    N_("Clear Image"),
    N_("Clear Image"),
    N_("Clear Viewer Window"), 
    //    Clear_xpm,
    NULL,
    GTK_STOCK_CLEAR,
    0,
    NULL,
    NULL,
  },
  {
    CmOutputPrinterB,
    N_("Print"),
    N_("Print"),
    N_("Print"), 
    //    Print_xpm,
    NULL,
    GTK_STOCK_PRINT,
    0,
    NULL,
    NULL,
  },
  {
    CmOutputViewerB,
    N_("Print preview"),
    N_("Print preview"),
    N_("Print preview"), 
    //    Preview_xpm,
    NULL,
    GTK_STOCK_PRINT_PREVIEW,
    0,
    NULL,
    NULL,
  },
  {NULL},
  {
    CmFileWinMath,
    N_("Math"),
    N_("Math Transformation"),
    N_("Set Math Transformation"), 
    Math_xpm,
    NULL,
    0,
    NULL,
    &MathBtn,
  },
  {
    CmAxisWinScaleUndo,
    N_("Undo"),
    N_("Scale Undo"),
    N_("Undo Scale Settings"), 
    //    Scaleundo_xpm,
    NULL,
    GTK_STOCK_UNDO,
    0,
    NULL,
    &AxisUndoBtn,
  },
}; 

static struct command_data Command2_data[] = {
  {
    NULL,
    N_("Point"),
    N_("Pointer"),
    N_("Legend and Axis Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"), 
    Point_xpm,
    NULL,
    PointB,
    NULL,
    NULL,
  },
  {
    NULL,
    N_("Legend"),
    N_("Legend Pointer"),
    N_("Legend Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"),
    Legendpoint_xpm,
    NULL,
    LegendB,
    NULL,
    NULL,
  },
  {
    NULL,
    N_("Axis"),
    N_("Axis Pointer"),
    N_("Axis Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"),
    Axispoint_xpm,
    NULL,
    AxisB,
    NULL,
    NULL,
  },
  {
    NULL,
    N_("Data"),
    N_("Data Pointer"),
    N_("Data Pointer"),
    Datapoint_xpm,
    NULL,
    DataB,
    NULL,
    NULL,
  },
  {NULL},
  {
    NULL,
    N_("Path"),
    N_("Path"),
    N_("New Legend Path (+SHIFT: Fine +CONTROL: snap angle)"), 
    Line_xpm,
    NULL,
    PathB,
    NULL,
    NULL,
  },
  {
    NULL,
    N_("Rectangle"),
    N_("Rectangle"),
    N_("New Legend Rectangle (+SHIFT: Fine +CONTROL: square integer ratio rectangle)"), 
    Rect_xpm,
    NULL,
    RectB,
    NULL,
    NULL,
  },
  {
    NULL,
    N_("Arc"),
    N_("Arc"),
    N_("New Legend Arc (+SHIFT: Fine +CONTROL: circle or integer ratio ellipse)"), 
    Arc_xpm,
    NULL,
    ArcB,
    NULL,
    NULL,
  },
  {
    NULL,
    N_("Mark"),
    N_("Mark"),
    N_("New Legend Mark (+SHIFT: Fine)"), 
    Mark_xpm,
    NULL,
    MarkB,
    NULL,
    NULL,
  },
  {
    NULL,
    N_("Text"),
    N_("Text"),
    N_("New Legend Text (+SHIFT: Fine)"), 
    Text_xpm,
    NULL,
    TextB,
    NULL,
    NULL,
  },
  {
    NULL,
    N_("Gauss"),
    N_("Gaussian"),
    N_("New Legend Gaussian (+SHIFT: Fine +CONTROL: integer ratio)"), 
    Gauss_xpm,
    NULL,
    GaussB,
    NULL,
    NULL,
  },
  {NULL},
  {
    NULL,
    N_("Frame axis"),
    N_("Frame Graph"),
    N_("New Frame Graph (+SHIFT: Fine +CONTROL: integer ratio)"), 
    Frame_xpm,
    NULL,
    FrameB,
    NULL,
    NULL,
  },
  {
    NULL,
    N_("Section axis"),
    N_("Section Graph"),
    N_("New Section Graph (+SHIFT: Fine +CONTROL: integer ratio)"), 
    Section_xpm,
    NULL,
    SectionB,
    NULL,
    NULL,
  },
  {
    NULL,
    N_("Cross axis"),
    N_("Cross Graph"),
    N_("New Cross Graph (+SHIFT: Fine +CONTROL: integer ratio)"), 
    Cross_xpm,
    NULL,
    CrossB,
    NULL,
    NULL,
  },
  {
    NULL,
    N_("Single axis"),
    N_("Single Axis"),
    N_("New Single Axis (+SHIFT: Fine +CONTROL: snap angle)"), 
    Single_xpm,
    NULL,
    SingleB,
    NULL,
    NULL,
  },
  {NULL},
  {
    NULL,
    N_("Trimming"),
    N_("Axis Trimming"),
    N_("Axis Trimming (+SHIFT: Fine)"), 
    Trimming_xpm,
    NULL,
    TrimB,
    NULL,
    NULL,
  },
  {
    NULL,
    N_("Evaluate"),
    N_("Evaluate Data"),
    N_("Evaluate Data Point"), 
    Eval_xpm,
    NULL,
    EvalB,
    NULL,
    NULL,
  },
  {
    NULL,
    N_("Zoom"),
    N_("Viewer Zoom"),
    N_("Viewer Zoom-In (+CONTROL: Zoom-Out +SHIFT: Centering)"), 
    Zoom_xpm,
    NULL,
    ZoomB,
    NULL,
    NULL,
  },
};

#define COMMAND1_NUM (sizeof(Command1_data) / sizeof(*Command1_data))
#define COMMAND2_NUM (sizeof(Command2_data) / sizeof(*Command2_data))

GdkColor white, gray;

void
set_current_window(GtkWidget *w)
{
  CurrentWindow = w;
}

GtkWidget *
get_current_window(void)
{
  return (CurrentWindow) ? CurrentWindow : TopLevel;
}

void
menu_lock(int lock)
{
  static int count = 0;

  if (lock) {
    count++;
  } else {
    count--;
  }

  if (count > 0) {
    Menulock = TRUE;
  } else {
    Menulock = FALSE;
    count = 0;
  }

  if (TopLevel) {
    gtk_widget_set_sensitive(TopLevel, ! Menulock);
  }
}

void
set_axis_undo_button_sensitivity(int state)
{
  gtk_widget_set_sensitive(AxisUndoBtn, state);
}

void 
set_draw_lock(int lock)
{
  DrawLock = lock;
}

static void
kill_signal_handler(int sig)
{
  Hide_window = APP_QUIT;
}

static void
term_signal_handler(int sig)
{
  Hide_window = APP_QUIT_FORCE;
}

static int
AppMainLoop(void)
{
  Hide_window = APP_CONTINUE;
  while (TRUE) {
    NgraphApp.Interrupt = FALSE;
    gtk_main_iteration();
    if (Hide_window != APP_CONTINUE && ! gtk_events_pending()) {
      int state = Hide_window;

      Hide_window = APP_CONTINUE;
      switch (state) {
      case APP_QUIT:
	if (CheckSave()) {
	  return 0;
	}
	break;
      case APP_QUIT_FORCE:
	return 1;
      }
    }
  }
  return 0;
}

void
reset_event(void)
{
  while (gtk_events_pending()) {
    gtk_main_iteration();
  }
}

struct WmStateHint
{
  unsigned long state;
  GtkWindow icon;
};

static gboolean
CloseCallback(GtkWidget *w, GdkEvent  *event, gpointer user_data)
{
  CmGraphQuit();

  return TRUE;
}

void
QuitGUI(void)
{
  if (TopLevel) {
    Hide_window = TRUE;
  }
}

int
find_gra2gdk_inst(char **name, struct objlist **o, N_VALUE **i, struct objlist **ro, int *routput, struct gra2cairo_local **rlocal)
{
  static struct objlist *obj = NULL, *robj = NULL;
  static N_VALUE *inst = NULL;
  static char *oname = "gra2gdk";
  static int pos;
  static struct gra2cairo_local *local = NULL;
  int id;

  if (obj == NULL) {
    obj = chkobject(oname);
    pos = getobjtblpos(obj, "_output", &robj);
  }

  if (obj == NULL)
    return FALSE;

  inst = chkobjinst(obj, 0);
  if (inst == NULL) {
    id = newobj(obj);
    if (id < 0)
      return FALSE;

    inst = chkobjinst(obj, 0);
  }
  _getobj(obj, "_local", inst, &local);

  if (inst == NULL) {
    return FALSE;
  }

  *routput = pos;
  *i = inst;
  *o = obj;
  *ro = robj;
  *name = oname;
  *rlocal = local;

  return TRUE;
}

static GtkWidget * 
create_menu_item(GtkWidget *menu, gchar *label, gboolean use_stock,
		 gchar *accel_path, guint accel_key, GdkModifierType accel_mods,
		 gpointer callback, int call_id)
{
  GtkWidget * menuitem;

  if (label == NULL) {
    menuitem = gtk_separator_menu_item_new();
  } else {
    if (use_stock) {
      menuitem = gtk_image_menu_item_new_from_stock(label, NULL);
    } else {
      menuitem = gtk_menu_item_new_with_mnemonic(label);
    }
    gtk_menu_item_set_accel_path(GTK_MENU_ITEM(menuitem), accel_path);
    g_signal_connect(G_OBJECT(menuitem), "activate", G_CALLBACK(callback), GINT_TO_POINTER(call_id));

    if (accel_key)
      gtk_accel_map_add_entry(accel_path, accel_key, accel_mods);
  }

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));

  return menuitem;
}

static void 
create_graphnewmenu(GtkWidget *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *menu;

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), menu);

  create_menu_item(menu, _("_Frame graph"),   FALSE, "<Ngraph>/Graph/New graph/Frame graph",   0, 0, CmGraphNewMenu, MenuIdGraphNewFrame);
  create_menu_item(menu, _("_Section graph"), FALSE, "<Ngraph>/Graph/New graph/Section graph", 0, 0, CmGraphNewMenu, MenuIdGraphNewSection);
  create_menu_item(menu, _("_Cross graph"),   FALSE, "<Ngraph>/Graph/New graph/Cross graph",   0, 0, CmGraphNewMenu, MenuIdGraphNewCross);
  create_menu_item(menu, _("_All clear"),     FALSE, "<Ngraph>/Graph/New graph/All clear",     0, 0, CmGraphNewMenu, MenuIdGraphAllClear);
}

static void
add_underscore(GString *str)
{
  unsigned int i = 1;

  while (i < str->len) {
    if (str->str[i] == '_') {
      g_string_insert_c(str, i, '_');
      i++;
    }
    i++;
  }
}

struct show_instance_menu_data {
  struct objlist *obj;
  GtkWidget *widget;
};

static void
show_instance_menu_cb(GtkWidget *widget, gpointer user_data)
{
  struct show_instance_menu_data *data;
  int num, i;

  data = (struct show_instance_menu_data *) user_data;

  for (i = 0; data[i].obj; i++) {
    num = chkobjlastinst(data[i].obj);
    gtk_widget_set_sensitive(data[i].widget, num >= 0);
  }
}

static void
hide_instance_menu_cb(GtkWidget *widget, gpointer user_data)
{
  struct show_instance_menu_data *data;
  int i;

  data = (struct show_instance_menu_data *) user_data;

  for (i = 0; data[i].obj; i++) {
    gtk_widget_set_sensitive(data[i].widget, TRUE);
  }
}

static void
set_show_instance_menu_cb(GtkWidget *menu, char *name, struct show_instance_menu_data *data, int n, GCallback show_cb, GCallback hide_cb)
{
  int i;
  struct objlist *obj;

  obj = chkobject(name);
  for (i = 0; i < n - 1; i++) {
    data[i].obj = obj;
  }
  data[i].obj = NULL;
  data[i].widget = NULL;

  if (show_cb == NULL)
    show_cb = G_CALLBACK(show_instance_menu_cb);

  if (hide_cb == NULL)
    hide_cb = G_CALLBACK(hide_instance_menu_cb);

  g_signal_connect(menu, "show", show_cb, data);
  g_signal_connect(menu, "hide", hide_cb, data);
}

static void
show_graph_menu_cb(GtkWidget *w, gpointer user_data)
{
  if (AddinMenu) {
    gtk_widget_set_sensitive(AddinMenu, Menulocal.scriptroot != NULL);
  }

#if USE_EXT_DRIVER
  if (ExtDrvOutMenu) {
    gtk_widget_set_sensitive(ExtDrvOutMenu, Menulocal.extprinterroot != NULL);
  }
#endif
}

static void
create_recent_graph_menu(GtkWidget *parent)
{
  GtkWidget *recent;
  GtkRecentFilter *filter;

  recent = gtk_recent_chooser_menu_new_for_manager(Menulocal.ngpfilelist);

  filter = gtk_recent_filter_new();
  gtk_recent_filter_add_application(filter, AppName);
  gtk_recent_filter_set_name(filter, "NGP file");
  gtk_recent_filter_add_pattern(filter, "*.ngp");

  gtk_recent_chooser_menu_set_show_numbers(GTK_RECENT_CHOOSER_MENU(recent), TRUE);

  gtk_recent_chooser_set_show_tips(GTK_RECENT_CHOOSER(recent), TRUE);
  gtk_recent_chooser_set_show_icons(GTK_RECENT_CHOOSER(recent), FALSE);
  gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER(recent), TRUE);
#ifndef WINDOWS
  gtk_recent_chooser_set_show_not_found(GTK_RECENT_CHOOSER(recent), FALSE);
#endif
  gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(recent), GTK_RECENT_SORT_MRU);
  gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(recent), 10);

  gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(recent), filter);

  g_signal_connect(recent, "item-activated", G_CALLBACK(CmGraphHistory), NULL);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), recent);
}

static void
create_addin_menu(GtkWidget *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *menu, *item;
  struct script *fcur;

  if (parent == NULL)
    return;

  menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(parent));
  if (menu)
    gtk_widget_destroy(menu);

  menu = gtk_menu_new();

  if (accel_group)
    gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);

  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), menu);

  fcur = Menulocal.scriptroot;
  while (fcur) {
    if (fcur->name && fcur->script) {
      item = gtk_menu_item_new_with_mnemonic(fcur->name);
      g_signal_connect(item, "activate", G_CALLBACK(script_exec), fcur);
      gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
      if (Menulocal.showtip && fcur->description) {
	gtk_widget_set_tooltip_text(item, fcur->description);
      }
    }
    fcur = fcur->next;
  }

  gtk_widget_show_all(menu);
}

void
update_addin_menu(void)
{
  create_addin_menu(AddinMenu, AccelGroup);
}

static void 
create_image_outputmenu(GtkWidget *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *menu;

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), menu);

  create_menu_item(menu, _("_GRA file"), FALSE, "<Ngraph>/Graph/Export image/GRA File", 0, 0, CmOutputMenu, MenuIdOutputGRAFile);
  create_menu_item(menu, _("_PS file"),  FALSE, "<Ngraph>/Graph/Export image/PS File",  0, 0, CmOutputMenu, MenuIdOutputPSFile);
  create_menu_item(menu, _("_EPS file"), FALSE, "<Ngraph>/Graph/Export image/EPS File", 0, 0, CmOutputMenu, MenuIdOutputEPSFile);
  create_menu_item(menu, _("P_DF file"), FALSE, "<Ngraph>/Graph/Export image/PDF File", 0, 0, CmOutputMenu, MenuIdOutputPDFFile);
  create_menu_item(menu, _("_SVG file"), FALSE, "<Ngraph>/Graph/Export image/SVG File", 0, 0, CmOutputMenu, MenuIdOutputSVGFile);
  create_menu_item(menu, _("P_NG file"), FALSE, "<Ngraph>/Graph/Export image/PNG File", 0, 0, CmOutputMenu, MenuIdOutputPNGFile);
#ifdef CAIRO_HAS_WIN32_SURFACE
  create_menu_item(menu, _("E_MF file"), FALSE, "<Ngraph>/Graph/Export image/EMF File", 0, 0, CmOutputMenu, MenuIdOutputEMFFile);
#endif	/* CAIRO_HAS_WIN32_SURFACE */
}

static void 
create_graphmenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(_("_Graph"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  g_signal_connect(menu, "show", G_CALLBACK(show_graph_menu_cb), NULL);

  item = gtk_menu_item_new_with_mnemonic(_("_New graph"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  create_graphnewmenu(item, accel_group);

  create_menu_item(menu, _("_Load graph"), FALSE, "<Ngraph>/Graph/Load graph", GDK_r, GDK_CONTROL_MASK, CmGraphMenu, MenuIdGraphLoad);

  item = gtk_menu_item_new_with_mnemonic(_("_Recent graphs"));
  create_recent_graph_menu(item);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);

  SaveMenuItem = create_menu_item(menu, GTK_STOCK_SAVE, TRUE, "<Ngraph>/Graph/Save",  GDK_s, GDK_CONTROL_MASK, CmGraphMenu, MenuIdGraphOverWrite);
  create_menu_item(menu, GTK_STOCK_SAVE_AS, TRUE, "<Ngraph>/Graph/SaveAs",  GDK_s, GDK_CONTROL_MASK | GDK_SHIFT_MASK, CmGraphMenu, MenuIdGraphSave);
  item = gtk_menu_item_new_with_mnemonic(_("_Export image"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  create_image_outputmenu(item, accel_group);
#if USE_EXT_DRIVER
  ExtDrvOutMenu = create_menu_item(menu, _("external _Driver"), FALSE, "<Ngraph>/Graph/External Driver", 0, 0, CmOutputMenu, MenuIdOutputDriver);
#endif
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);

  create_menu_item(menu, _("_Draw order"), FALSE, "<Ngraph>/Graph/Draw order", 0, 0, CmGraphMenu, MenuIdGraphSwitch);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);

#if (GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 14))
  create_menu_item(menu, GTK_STOCK_PAGE_SETUP, TRUE, "<Ngraph>/Graph/Page", 0, 0, CmGraphMenu, MenuIdGraphPage);
#else
  create_menu_item(menu, _("pa_Ge"), FALSE, "<Ngraph>/Graph/Page", 0, 0, CmGraphMenu, MenuIdGraphPage);
#endif
  create_menu_item(menu, GTK_STOCK_PRINT_PREVIEW, TRUE, "<Ngraph>/Graph/Print preview", 0, 0, CmOutputMenu, MenuIdOutputViewer);
  create_menu_item(menu, GTK_STOCK_PRINT, TRUE, "<Ngraph>/Graph/Print",  GDK_p, GDK_CONTROL_MASK, CmGraphMenu, MenuIdOutputDriver);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, _("_Current directory"), FALSE, "<Ngraph>/Graph/Current directory", 0, 0, CmGraphMenu, MenuIdGraphDirectory);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);

  item = gtk_menu_item_new_with_mnemonic(_("_Add-in"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  create_addin_menu(item, accel_group);
  AddinMenu = item;

  create_menu_item(menu, _("_Ngraph shell"), FALSE, "<Ngraph>/Graph/Ngraph shell", 0, 0, CmGraphMenu, MenuIdGraphShell);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, GTK_STOCK_QUIT, TRUE, "<Ngraph>/Graph/Quit",  GDK_q, GDK_CONTROL_MASK, CmGraphMenu, MenuIdGraphQuit);
}

static void
show_edit_menu_cb(GtkWidget *w, gpointer user_data)
{
  int num, type;
  GtkClipboard *clip;
  gboolean state, state2, state3;
  struct objlist *axis = NULL;
  struct FocusObj **focus;


  num = check_focused_obj_type(&NgraphApp.Viewer, &type);

  focus = arraydata(NgraphApp.Viewer.focusobj);

  if (num < 1) {
    state2 = state = FALSE;
  } else if (num == 1 && focus[0]->obj == axis) {
    state = FALSE;
    state2 = TRUE;
  } else {
    state2 = state = TRUE;
  }

  state = (! (type & FOCUS_OBJ_TYPE_AXIS) && (type & (FOCUS_OBJ_TYPE_MERGE | FOCUS_OBJ_TYPE_LEGEND)));
  state2 = (! (type & FOCUS_OBJ_TYPE_MERGE) && (type & (FOCUS_OBJ_TYPE_AXIS | FOCUS_OBJ_TYPE_LEGEND)));
  state3 = (! (type & (FOCUS_OBJ_TYPE_MERGE | FOCUS_OBJ_TYPE_TEXT)) && (type & (FOCUS_OBJ_TYPE_AXIS | FOCUS_OBJ_TYPE_LEGEND)));

  gtk_widget_set_sensitive(EditCut, state);
  gtk_widget_set_sensitive(EditCopy, state);
  gtk_widget_set_sensitive(EditDelete, num > 0);
  gtk_widget_set_sensitive(EditAlign, num > 0);
  gtk_widget_set_sensitive(RotateCW, state2);
  gtk_widget_set_sensitive(RotateCCW, state2);
  gtk_widget_set_sensitive(FlipH, state3);
  gtk_widget_set_sensitive(FlipV, state3);

  clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  state = gtk_clipboard_wait_is_text_available(clip);

  switch (NgraphApp.Viewer.Mode) {
  case PointB:
  case LegendB:
    gtk_widget_set_sensitive(EditPaste, state);
    break;
  default:
    gtk_widget_set_sensitive(EditPaste, FALSE);
  }
}

static void
hide_edit_menu_cb(GtkWidget *w, gpointer user_data)
{
  gtk_widget_set_sensitive(EditCut, TRUE);
  gtk_widget_set_sensitive(EditCopy, TRUE);
  gtk_widget_set_sensitive(EditPaste, TRUE);
  gtk_widget_set_sensitive(EditDelete, TRUE);
  gtk_widget_set_sensitive(EditAlign, TRUE);
  gtk_widget_set_sensitive(RotateCW, TRUE);
  gtk_widget_set_sensitive(RotateCCW, TRUE);
}

static void 
create_alignmenu(GtkWidget *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *menu;

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), menu);

  create_menu_item(menu, _("_Left"),            FALSE, "<Ngraph>/Edit/Align/Frame graph",   0, 0, CmEditMenuCB, MenuIdAlignLeft);
  create_menu_item(menu, _("_Vertical center"), FALSE, "<Ngraph>/Edit/Align/Section graph", 0, 0, CmEditMenuCB, MenuIdAlignVCenter);
  create_menu_item(menu, _("_Right"),           FALSE, "<Ngraph>/Edit/Align/Cross graph",   0, 0, CmEditMenuCB, MenuIdAlignRight);

  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);

  create_menu_item(menu, _("_Top"),               FALSE, "<Ngraph>/Edit/Align/Single Axis", 0, 0, CmEditMenuCB, MenuIdAlignTop);
  create_menu_item(menu, _("_Horizontal center"), FALSE, "<Ngraph>/Edit/Align/Single Axis", 0, 0, CmEditMenuCB, MenuIdAlignHCenter);
  create_menu_item(menu, _("_Bottom"),            FALSE, "<Ngraph>/Edit/Align/Single Axis", 0, 0, CmEditMenuCB, MenuIdAlignHBottom);
}

static void 
create_editmenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(_("_Edit"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  g_signal_connect(menu, "show", G_CALLBACK(show_edit_menu_cb), NULL);
  g_signal_connect(menu, "hide", G_CALLBACK(hide_edit_menu_cb), NULL);

  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  EditCut    = create_menu_item(menu,  GTK_STOCK_CUT,    TRUE, "<Ngraph>/Edit/Cut",
				GDK_x, GDK_CONTROL_MASK, CmEditMenuCB, MenuIdEditCut);
  EditCopy   = create_menu_item(menu,  GTK_STOCK_COPY,   TRUE, "<Ngraph>/Edit/Copy",
				GDK_c, GDK_CONTROL_MASK, CmEditMenuCB, MenuIdEditCopy);
  EditPaste  = create_menu_item(menu,  GTK_STOCK_PASTE,  TRUE, "<Ngraph>/Edit/Paste",
				GDK_v, GDK_CONTROL_MASK, CmEditMenuCB, MenuIdEditPaste);
  EditDelete = create_menu_item(menu,  GTK_STOCK_DELETE, TRUE, "<Ngraph>/Edit/Delete",
				0, 0,                    CmEditMenuCB, MenuIdEditDelete);

  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);

  item = gtk_menu_item_new_with_mnemonic(_("_Align"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  create_alignmenu(item, accel_group);
  EditAlign = item;

  RotateCW  = create_menu_item(menu, _("rotate _90 degree clockwise"), TRUE, "<Ngraph>/Edit/RotateCW",
			       0, 0, CmEditMenuCB, MenuIdEditRotateCW);
  RotateCCW = create_menu_item(menu, _("rotate 9_0 degree counter-clockwise"), TRUE, "<Ngraph>/Edit/RotateCCW",
			       0, 0, CmEditMenuCB, MenuIdEditRotateCCW);

  FlipH  = create_menu_item(menu, _("flip _Horizontally"), TRUE, "<Ngraph>/Edit/FlipHorizontally",
			       0, 0, CmEditMenuCB, MenuIdEditFlipHorizontally);
  FlipV = create_menu_item(menu, _("flip _Vertically"), TRUE, "<Ngraph>/Edit/FlipVertically",
			       0, 0, CmEditMenuCB, MenuIdEditFlipVertically);
}

static void
show_file_menu_cb(GtkWidget *w, gpointer user_data)
{
  GtkWidget *label;
  char **data;
  int num, i;
  static GString *str = NULL;

  num = arraynum(Menulocal.datafilelist);
  data = arraydata(Menulocal.datafilelist);

  if (RecentData)
    gtk_widget_set_sensitive(RecentData, num > 0);

  if (str == NULL) {
    str = g_string_new("");
  }

  for (i = 0; i < MENU_HISTORY_NUM; i++) {
    if (i < num) {
      label = gtk_bin_get_child(GTK_BIN(NgraphApp.fhistory[i]));
      g_string_printf(str, "_%d: %s", i, CHK_STR(data[i]));
      add_underscore(str);
      gtk_label_set_text_with_mnemonic(GTK_LABEL(label), str->str);
      gtk_widget_show(GTK_WIDGET(NgraphApp.fhistory[i]));
    } else {
      gtk_widget_hide(GTK_WIDGET(NgraphApp.fhistory[i]));
    }
  }
}

static void
create_recent_data_menu(GtkWidget *parent, GtkAccelGroup *accel_group)
{
  int i;
  GtkWidget *menu;

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), menu);

  for (i = 0; i < MENU_HISTORY_NUM; i++) {
    NgraphApp.fhistory[i] = gtk_menu_item_new_with_label("");
    g_signal_connect(NgraphApp.fhistory[i], "activate", G_CALLBACK(CmFileHistory), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(NgraphApp.fhistory[i]));
  }
}

static void 
create_filemenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;
  static struct show_instance_menu_data data[6];

  item = gtk_menu_item_new_with_mnemonic(_("_Data"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  g_signal_connect(menu, "show", G_CALLBACK(show_file_menu_cb), NULL);
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  create_menu_item(menu, GTK_STOCK_NEW, TRUE, "<Ngraph>/Data/New", 0, 0, CmFileMenu, MenuIdFileNew);
  create_menu_item(menu, GTK_STOCK_OPEN, TRUE, "<Ngraph>/Data/Open", GDK_o, GDK_CONTROL_MASK, CmFileMenu, MenuIdFileOpen);

  item = gtk_menu_item_new_with_mnemonic(_("_Recent data"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  create_recent_data_menu(item, accel_group);
  RecentData = item;

  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  data[0].widget = create_menu_item(menu, GTK_STOCK_PROPERTIES, TRUE, "<Ngraph>/Data/Property", 0, 0, CmFileMenu, MenuIdFileUpdate);
  data[1].widget = create_menu_item(menu, GTK_STOCK_CLOSE, TRUE, "<Ngraph>/Data/Close", 0, 0, CmFileMenu, MenuIdFileClose);
  data[2].widget = create_menu_item(menu, GTK_STOCK_EDIT, TRUE, "<Ngraph>/Data/Edit", 0, 0, CmFileMenu, MenuIdFileEdit);
  data[3].widget = create_menu_item(menu, _("_Save data"), FALSE, "<Ngraph>/Data/Save data", 0, 0, CmOutputMenu, MenuIdPrintDataFile);
  data[4].widget = create_menu_item(menu, _("_Math Transformation"), FALSE, "<Ngraph>/Data/Math", 0, 0, CmFileMenu, MenuIdFileMath);

  set_show_instance_menu_cb(menu, "file", data, sizeof(data) / sizeof(*data), NULL, NULL);
}

static void 
create_axisaddmenu(GtkWidget *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *menu;

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), menu);

  create_menu_item(menu, _("_Frame graph"), FALSE, "<Ngraph>/Axis/Add/Frame graph", 0, 0, CmAxisAddMenu, MenuIdAxisNewFrame);
  create_menu_item(menu, _("_Section graph"), FALSE, "<Ngraph>/Axis/Add/Section graph", 0, 0, CmAxisAddMenu, MenuIdAxisNewSection);
  create_menu_item(menu, _("_Cross graph"), FALSE, "<Ngraph>/Axis/Add/Cross graph", 0, 0, CmAxisAddMenu, MenuIdAxisNewCross);
  create_menu_item(menu, _("Single _Axis"), FALSE, "<Ngraph>/Axis/Add/Single Axis", 0, 0, CmAxisAddMenu, MenuIdAxisNewSingle);
}

static void 
create_axisgridmenu(GtkWidget *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *menu;
  static struct show_instance_menu_data data[3];

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), menu);

  create_menu_item(menu, GTK_STOCK_NEW, TRUE, "<Ngraph>/Axis/Grid/New", 0, 0, CmGridMenu, MenuIdAxisGridNew);
  data[0].widget = create_menu_item(menu, GTK_STOCK_PROPERTIES, TRUE, "<Ngraph>/Axis/Grid/Property", 0, 0, CmGridMenu, MenuIdAxisGridUpdate);
  data[1].widget = create_menu_item(menu, GTK_STOCK_DELETE, TRUE, "<Ngraph>/Axis/Grid/Delete", 0, 0, CmGridMenu, MenuIdAxisGridDel);

  set_show_instance_menu_cb(menu, "axisgrid", data, sizeof(data) / sizeof(*data), NULL, NULL);
}

static void
show_axis_menu_cb(GtkWidget *widget, gpointer user_data)
{
  static struct objlist *axis = NULL;

  if (axis == NULL)
    axis = chkobject("axis");

  show_instance_menu_cb(widget, user_data);

  if (axis)
    gtk_widget_set_sensitive(AxisUndoMenuItem, check_axis_history(axis));
}

static void
hide_axis_menu_cb(GtkWidget *widget, gpointer user_data)
{
  static struct objlist *axis = NULL;

  if (axis == NULL)
    axis = chkobject("axis");

  hide_instance_menu_cb(widget, user_data);

  if (axis)
    gtk_widget_set_sensitive(AxisUndoMenuItem, TRUE);
}

static void 
create_axismenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;
  static struct show_instance_menu_data data[5];

  item = gtk_menu_item_new_with_mnemonic(_("_Axis"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  create_axisaddmenu(item, accel_group);

  data[0].widget = create_menu_item(menu, GTK_STOCK_PROPERTIES, TRUE, "<Ngraph>/Axis/Property", 0, 0, CmAxisMenu, MenuIdAxisUpdate);
  data[1].widget = create_menu_item(menu, GTK_STOCK_DELETE, TRUE, "<Ngraph>/Axis/Delete", 0, 0, CmAxisMenu, MenuIdAxisDel);
  data[2].widget = create_menu_item(menu, _("Scale _Zoom"), FALSE, "<Ngraph>/Axis/Scale Zoom", 0, 0, CmAxisMenu, MenuIdAxisZoom);
  data[3].widget = create_menu_item(menu, _("Scale _Clear"), FALSE, "<Ngraph>/Axis/Scale Clear", GDK_c, GDK_SHIFT_MASK | GDK_CONTROL_MASK, CmAxisMenu, MenuIdAxisClear);

  AxisUndoMenuItem = create_menu_item(menu, _("Scale _Undo"), FALSE, "<Ngraph>/Axis/Scale Undo", 0, 0, CmAxisMenu, MenuIdAxisUndo);

  item = gtk_menu_item_new_with_mnemonic(_("_Grid"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  create_axisgridmenu(item, accel_group);

  set_show_instance_menu_cb(menu, "axis", data, sizeof(data) / sizeof(*data), G_CALLBACK(show_axis_menu_cb), G_CALLBACK(hide_axis_menu_cb));
}

static GtkWidget *
create_legendsubmenu(GtkWidget *parent, char *label, legend_cb_func func, GtkAccelGroup *accel_group)
{
  GtkWidget *menu, *submenu;
  char buf[256];

  menu = gtk_menu_item_new_with_mnemonic(_(label));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(menu));

  submenu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(submenu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu), submenu);

  if (label[0] == '_')
    label++;

  snprintf(buf, sizeof(buf), "<Ngraph>/Legend/%s/Property", label);
  create_menu_item(submenu, GTK_STOCK_PROPERTIES, TRUE, buf, 0, 0, func, MenuIdLegendUpdate);

  snprintf(buf, sizeof(buf), "<Ngraph>/Legend/%s/Delete", label);
  create_menu_item(submenu, GTK_STOCK_DELETE, TRUE, buf, 0, 0, func, MenuIdLegendDel);

  return menu;
}


static void 
create_legendmenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;
  static struct show_instance_menu_data data[6];

  item = gtk_menu_item_new_with_mnemonic(_("_Legend"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  /* an under score must be placed top of the menu title. */
  data[0].widget = create_legendsubmenu(menu, N_("_Path"), CmLineMenu, accel_group);
  data[0].obj = chkobject("path");

  data[1].widget = create_legendsubmenu(menu, N_("_Rectangle"), CmRectangleMenu, accel_group);
  data[1].obj = chkobject("rectangle");

  data[2].widget = create_legendsubmenu(menu, N_("_Arc"), CmArcMenu, accel_group);
  data[2].obj = chkobject("arc");

  data[3].widget = create_legendsubmenu(menu, N_("_Mark"), CmMarkMenu, accel_group);
  data[3].obj = chkobject("mark");

  data[4].widget = create_legendsubmenu(menu, N_("_Text"), CmTextMenu, accel_group);
  data[4].obj = chkobject("text");

  data[5].obj = NULL;
  data[5].widget = NULL;

  g_signal_connect(menu, "show", G_CALLBACK(show_instance_menu_cb), data);
}

static void 
create_mergemenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;
  static struct show_instance_menu_data data[3];

  item = gtk_menu_item_new_with_mnemonic(_("_Merge"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  create_menu_item(menu, GTK_STOCK_OPEN, TRUE, "<Ngraph>/Merge/Open", 0, 0, CmMergeMenu, MenuIdMergeOpen);
  data[0].widget = create_menu_item(menu, GTK_STOCK_PROPERTIES, TRUE, "<Ngraph>/Merge/Property", 0, 0, CmMergeMenu, MenuIdMergeUpdate);
  data[1].widget = create_menu_item(menu, GTK_STOCK_CLOSE, TRUE, "<Ngraph>/Merge/Close", 0, 0, CmMergeMenu, MenuIdMergeClose);

  set_show_instance_menu_cb(menu, "merge", data, sizeof(data) / sizeof(*data), NULL, NULL);
}

static void 
create_preferencemenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(_("_Preference"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  create_menu_item(menu, _("_Viewer"), FALSE, "<Ngraph>/Preference/Viewer", 0, 0, CmOptionMenu, MenuIdOptionViewer);
  create_menu_item(menu, _("_External viewer"), FALSE, "<Ngraph>/Preference/External Viewer", 0, 0, CmOptionMenu, MenuIdOptionExtViewer);
  create_menu_item(menu, _("_Font aliases"), FALSE, "<Ngraph>/Preference/Font aliases", 0, 0, CmOptionMenu, MenuIdOptionPrefFont);
#if USE_EXT_DRIVER
  create_menu_item(menu, _("External _Driver"), FALSE, "<Ngraph>/Preference/External Driver", 0, 0, CmOptionMenu, MenuIdOptionPrefDriver);
#endif
  create_menu_item(menu, _("_Add-in script"), FALSE, "<Ngraph>/Preference/Addin Script", 0, 0, CmOptionMenu, MenuIdOptionScript);
  create_menu_item(menu, _("_Miscellaneous"), FALSE, "<Ngraph>/Preference/Miscellaneous", 0, 0, CmOptionMenu, MenuIdOptionMisc);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, _("save as default (_Settings)"), FALSE, "<Ngraph>/Preference/save as default (Settings)", 0, 0, CmOptionMenu, MenuIdOptionSaveDefault);
  create_menu_item(menu, _("save as default (_Graph)"), FALSE, "<Ngraph>/Preference/save as default (Graph)", 0, 0, CmOptionMenu, MenuIdOptionSaveNgp);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, _("_Data file default"), FALSE, "<Ngraph>/Preference/Data file default", 0, 0, CmOptionMenu, MenuIdOptionFileDef);
  create_menu_item(menu, _("_Legend text default"), FALSE, "<Ngraph>/Preference/Legend text default", 0, 0, CmOptionMenu, MenuIdOptionTextDef);
}

static void
show_win_menu_cb(GtkWidget *w, gpointer data)
{
  Toggle_cb_disable = TRUE;

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ShowFileWin), 
				 NgraphApp.FileWin.Win && GTK_WIDGET_VISIBLE(NgraphApp.FileWin.Win));

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ShowAxisWin),
				 NgraphApp.AxisWin.Win && GTK_WIDGET_VISIBLE(NgraphApp.AxisWin.Win));
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ShowLegendWin),
				 NgraphApp.LegendWin.Win &&GTK_WIDGET_VISIBLE(NgraphApp.LegendWin.Win));

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ShowMergeWin),
				 NgraphApp.MergeWin.Win && GTK_WIDGET_VISIBLE(NgraphApp.MergeWin.Win));

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ShowCoodinateWin),
				 NgraphApp.CoordWin.Win && GTK_WIDGET_VISIBLE(NgraphApp.CoordWin.Win));

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ShowInfoWin),
				 NgraphApp.InfoWin.Win && GTK_WIDGET_VISIBLE(NgraphApp.InfoWin.Win));

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ToggleStatusBar), Menulocal.statusb);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ToggleRuler), Menulocal.ruler);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ToggleScrollbar), Menulocal.scrollbar);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ToggleCToobar), Menulocal.ctoolbar);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(TogglePToobar), Menulocal.ptoolbar);
  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ToggleCrossGauge), Menulocal.show_cross);

  Toggle_cb_disable = FALSE;
}

static void
toggle_win_cb(GtkWidget *w, gpointer data)
{
  void (*func) (GtkWidget *, gpointer);

  if (Toggle_cb_disable) return;

  func = data;
  func(w, NULL);
}

static GtkWidget * 
create_toggle_menu_item(GtkWidget *menu, gchar *label, gchar *accel_path, guint accel_key,
			GdkModifierType accel_mods, gpointer callback, gpointer data)
{
  GtkWidget * menuitem;

  menuitem = gtk_check_menu_item_new_with_mnemonic(label);
  gtk_menu_item_set_accel_path(GTK_MENU_ITEM(menuitem), accel_path);
  g_signal_connect(G_OBJECT(menuitem), "toggled", G_CALLBACK(callback), data);

  if (accel_key)
    gtk_accel_map_add_entry(accel_path, accel_key, accel_mods);

  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(menuitem));

  return menuitem;
}

static void
toggle_view_cb(GtkWidget *w, gpointer data)
{
  int type, cross;

  if (Toggle_cb_disable)
    return;

  type = (int) data;

  cross = Menulocal.show_cross;

  switch (type) {
  case MenuIdToggleStatusBar:
    Menulocal.statusb = ! Menulocal.statusb;
   break;
  case MenuIdToggleRuler:
    Menulocal.ruler = ! Menulocal.ruler;
    break;
  case MenuIdToggleScrollbar:
    Menulocal.scrollbar = ! Menulocal.scrollbar;
    break;
  case MenuIdToggleCToolbar:
    Menulocal.ctoolbar = ! Menulocal.ctoolbar;
    break;
  case MenuIdTogglePToolbar:
    Menulocal.ptoolbar = ! Menulocal.ptoolbar;
    break;
  case MenuIdToggleCrossGauge:
    cross = ! Menulocal.show_cross;
    break;
  }
  set_widget_visibility(cross);
}

static void
clear_information(GtkMenuItem *w, gpointer user_data)
{
  InfoWinClear();
}

static void 
create_windowmenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(_("_View"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  create_menu_item(menu, _("_Draw"), FALSE, "<Ngraph>/View/Draw", GDK_d, GDK_CONTROL_MASK, CmOutputMenu, MenuIdViewerDraw);
  create_menu_item(menu, GTK_STOCK_CLEAR, TRUE, "<Ngraph>/View/Clear", GDK_e, GDK_CONTROL_MASK, CmOutputMenu, MenuIdViewerClear);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);

  item = create_toggle_menu_item(menu, "_Data Window", "<Ngraph>/View/Data Window", GDK_F3, 0, G_CALLBACK(toggle_win_cb), CmFileWindow);
  ShowFileWin = item;

  item = create_toggle_menu_item(menu, "_Axis Window", "<Ngraph>/View/Axis Window", GDK_F4, 0, G_CALLBACK(toggle_win_cb), CmAxisWindow);
  ShowAxisWin = item;

  item = create_toggle_menu_item(menu, "_Legend Window", "<Ngraph>/View/Legend Window", GDK_F5, 0, G_CALLBACK(toggle_win_cb), CmLegendWindow);
  ShowLegendWin = item;

  item = create_toggle_menu_item(menu, "_Merge Window", "<Ngraph>/View/Merge Window", GDK_F6, 0, G_CALLBACK(toggle_win_cb), CmMergeWindow);
  ShowMergeWin = item;

  item = create_toggle_menu_item(menu, "_Coordinate Window", "<Ngraph>/View/Coordinate Window", GDK_F7, 0, G_CALLBACK(toggle_win_cb), CmCoordinateWindow);
  ShowCoodinateWin = item;

  item = create_toggle_menu_item(menu, "_Information Window", "<Ngraph>/View/Information Window", GDK_F8, 0, G_CALLBACK(toggle_win_cb), CmInformationWindow);
  ShowInfoWin = item;

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = gtk_menu_item_new_with_mnemonic(_("default _Window config"));
  g_signal_connect(item, "activate", G_CALLBACK(CmReloadWindowConfig), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = gtk_menu_item_new_with_mnemonic(_("_Clear information window"));
  g_signal_connect(item, "activate", G_CALLBACK(clear_information), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = create_toggle_menu_item(menu, _("_Status bar"), "<Ngraph>/View/Status bar",
				 0, 0, G_CALLBACK(toggle_view_cb), GINT_TO_POINTER(MenuIdToggleStatusBar));
  ToggleStatusBar = item;

  item = create_toggle_menu_item(menu, _("_Ruler"), "<Ngraph>/View/Ruler",
				 0, 0, G_CALLBACK(toggle_view_cb), GINT_TO_POINTER(MenuIdToggleRuler));
  ToggleRuler = item;

  item = create_toggle_menu_item(menu, _("_Scrollbar"), "<Ngraph>/View/Scrollbar",
				 0, 0, G_CALLBACK(toggle_view_cb), GINT_TO_POINTER(MenuIdToggleScrollbar));
  ToggleScrollbar = item;

  item = create_toggle_menu_item(menu, _("_Command toolbar"), "<Ngraph>/View/Command toolbar",
				 0, 0, G_CALLBACK(toggle_view_cb), GINT_TO_POINTER(MenuIdToggleCToolbar));
  ToggleCToobar = item;

  item = create_toggle_menu_item(menu, _("_Toolbox"), "<Ngraph>/View/Toolbox",
				 0, 0, G_CALLBACK(toggle_view_cb), GINT_TO_POINTER(MenuIdTogglePToolbar));
  TogglePToobar = item;

  item = create_toggle_menu_item(menu, _("cross _Gauge"), "<Ngraph>/View/cross Gauge",
				 GDK_plus, 0, G_CALLBACK(toggle_view_cb), GINT_TO_POINTER(MenuIdToggleCrossGauge));
  ToggleCrossGauge = item;

  g_signal_connect(menu, "show", G_CALLBACK(show_win_menu_cb), NULL);
}

static void 
create_helpmenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(_("_Help"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  create_menu_item(menu, GTK_STOCK_HELP, TRUE, "<Ngraph>/Help/Help", GDK_F1, 0, CmHelpMenu, MenuIdHelpHelp);
  create_menu_item(menu, GTK_STOCK_ABOUT, TRUE, "<Ngraph>/Help/About", 0, 0, CmHelpMenu, MenuIdHelpAbout);
}

static char *
get_home(void)
{
  struct objlist *sysobj;
  N_VALUE *inst;
  char *home;

  sysobj = chkobject("system");
  inst = chkobjinst(sysobj, 0);
  _getobj(sysobj, "home_dir", inst, &home);

  return home;
}

static void
createmenu(GtkMenuBar *parent)
{
  GtkAccelGroup *accel_group;
  char *home, *filename;
 
  accel_group = gtk_accel_group_new();

  create_graphmenu(parent, accel_group);
  create_editmenu(parent, accel_group);
  create_windowmenu(parent, accel_group);
  create_filemenu(parent, accel_group);
  create_axismenu(parent, accel_group);
  create_legendmenu(parent, accel_group);
  create_mergemenu(parent, accel_group);
  create_preferencemenu(parent, accel_group);
  create_helpmenu(parent, accel_group);

  gtk_window_add_accel_group(GTK_WINDOW(TopLevel), accel_group);
  AccelGroup = accel_group;

  home = get_home();
  if (home == NULL)
    return;

  filename = g_strdup_printf("%s/%s", home, KEYMAP_FILE);
  if (naccess(filename, R_OK) == 0) {
    gtk_accel_map_load(filename);
  }
  g_free(filename);
}

static void
createpixmap(int n, struct command_data *data)
{
  GdkPixbuf *pixbuf;
  int i;

  for (i = 0; i < n; i++) {
    if (data[i].label == NULL || data[i].xpm == NULL)
      continue;

    pixbuf = gdk_pixbuf_new_from_xpm_data(data[i].xpm);
    data[i].img = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
  }
}

static void
create_toolbar_pixmap(void)
{
  createpixmap(COMMAND1_NUM, Command1_data);
  createpixmap(COMMAND2_NUM, Command2_data);
}

#define MARK_PIX_SIZE 24
static void
create_markpixmap(GtkWidget *win)
{
  GdkPixmap *pix;
  int gra, i, R, G, B, R2, G2, B2, found, output;
  struct objlist *obj, *robj;
  N_VALUE *inst;
  char *name;
  struct gra2cairo_local *local;

  R = G = B = 0;
  R2 = 0;
  G2 = B2 = 255;

  found = find_gra2gdk_inst(&name, &obj, &inst, &robj, &output, &local);

  for (i = 0; i < MARK_TYPE_NUM; i++) {
    if (! found) {
      pix = NULL;
    } else {
      pix = gra2gdk_create_pixmap(obj, inst, local, win->window,
				  MARK_PIX_SIZE, MARK_PIX_SIZE,
				  255, 255, 255);
      if (pix) {
	gra = _GRAopen("gra2gdk", "_output",
		       robj, inst, output, -1, -1, -1, NULL, local);
	if (gra >= 0) {
	  GRAview(gra, 0, 0, MARK_PIX_SIZE, MARK_PIX_SIZE, 0);
	  GRAlinestyle(gra, 0, NULL, 1, 0, 0, 1000);
	  GRAmark(gra, i,
		  MARK_PIX_SIZE / 2, MARK_PIX_SIZE / 2,
		  MARK_PIX_SIZE - 4,
		  R, G, B, 255,
		  R2, G2, B2, 255);
	}
	_GRAclose(gra);
      }
    }
    NgraphApp.markpix[i] = pix;
  }
}

static void
free_markpixmap(void)
{
  int i;

  for (i = 0; i < MARK_TYPE_NUM; i++) {
    if (NgraphApp.markpix[i])
      g_object_unref(NgraphApp.markpix[i]);
    NgraphApp.markpix[i] = NULL;
  }
}

static void
create_icon(void)
{
  GList *tmp, *list = NULL;
  GdkPixbuf *pixbuf;

  pixbuf = gdk_pixbuf_new_from_xpm_data(Icon_xpm);
  list = g_list_append(list, pixbuf);

  pixbuf = gdk_pixbuf_new_from_xpm_data(Icon_xpm_64);
  list = g_list_append(list, pixbuf);

  gtk_window_set_default_icon_list(list);
  gtk_window_set_icon_list(GTK_WINDOW(TopLevel), list);

  tmp = list;
  while (tmp) {
    g_object_unref(tmp->data);
    tmp = tmp->next;
  }
  g_list_free(list);
}

static int
create_cursor(void)
{
  unsigned int i;

  NgraphApp.cursor = g_malloc(sizeof(GdkCursor *) * CURSOR_TYPE_NUM);
  if (NgraphApp.cursor == NULL)
    return 1;

  for (i = 0; i < CURSOR_TYPE_NUM; i++) {
    NgraphApp.cursor[i] = gdk_cursor_new(Cursor[i]);
  }

  return 0;
}

static void
free_cursor(void)
{
  unsigned int i;

  for (i = 0; i < CURSOR_TYPE_NUM; i++) {
    gdk_cursor_unref(NgraphApp.cursor[i]);
    NgraphApp.cursor[i] = NULL;
  }
  g_free(NgraphApp.cursor);
  NgraphApp.cursor = NULL;
}

static gboolean
tool_button_enter_leave_cb(GtkWidget *w, GdkEventCrossing *e, gpointer data)
{
  char *str;

  str = (char *) data;

  if (e->type == GDK_ENTER_NOTIFY) {
    SetStatusBar(str);
  } else {
    ResetStatusBar();
  }

  return FALSE;
}

static void
createcommand1(GtkToolbar *parent)
{
  GtkToolItem *b;
  unsigned int i;
  struct command_data *cdata;

  for (i = 0; i < COMMAND1_NUM; i++) {
    cdata = &Command1_data[i];
    if (cdata->label) {
      if (cdata->xpm) {
	b = gtk_tool_button_new(cdata->img, _(cdata->label));
      } else {
	b = gtk_tool_button_new_from_stock(cdata->stock);
	gtk_tool_button_set_label(GTK_TOOL_BUTTON(b), _(cdata->label));
      }

      if (cdata->button) {
	*cdata->button = GTK_WIDGET(b);
      }

      if (Menulocal.showtip) {
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(b), _(cdata->tip));
      }

      gtk_tool_item_set_homogeneous(GTK_TOOL_ITEM(b), FALSE);
      g_signal_connect(gtk_bin_get_child(GTK_BIN(b)),
		       "enter-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb),
		       _(cdata->caption));

      g_signal_connect(gtk_bin_get_child(GTK_BIN(b)),
		       "leave-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb), NULL);

      g_signal_connect(b, "clicked", G_CALLBACK(cdata->func), NULL);
    } else {
      b = gtk_separator_tool_item_new();
    }
    gtk_toolbar_insert(parent, GTK_TOOL_ITEM(b), -1);
  }
}

static void
createcommand2(GtkToolbar *parent)
{
  GtkToolItem *b;
  GSList *list = NULL;
  unsigned int i, j;
  struct command_data *cdata;

  j = 0;
  for (i = 0; i < COMMAND2_NUM; i++) {
    cdata = &Command2_data[i];
    if (cdata->label) {
      b = gtk_radio_tool_button_new(list);

      if (cdata->button) {
	*cdata->button = GTK_WIDGET(b);
      }

      if (Menulocal.showtip) {
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(b), _(cdata->tip));
      }

      g_signal_connect(gtk_bin_get_child(GTK_BIN(b)),
		       "enter-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb),
		       _(cdata->caption));
      g_signal_connect(gtk_bin_get_child(GTK_BIN(b)),
		       "leave-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb), NULL);
      g_signal_connect(gtk_bin_get_child(GTK_BIN(b)),
		       "button-press-event",
		       G_CALLBACK(CmViewerButtonPressed), NULL);

      list = gtk_radio_tool_button_get_group(GTK_RADIO_TOOL_BUTTON(b));

      if (cdata->img) {
	gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(b), cdata->img);
      } else {
	gtk_tool_button_set_stock_id(GTK_TOOL_BUTTON(b), cdata->stock);
      }
      gtk_tool_button_set_label(GTK_TOOL_BUTTON(b), _(cdata->label));

      g_signal_connect(b, "clicked", G_CALLBACK(CmViewerButtonArm), GINT_TO_POINTER(cdata->type));

      NgraphApp.viewb[j] = b;
      j++;
    } else {
      b = gtk_separator_tool_item_new();
    }
    gtk_toolbar_insert(parent, GTK_TOOL_ITEM(b), -1);
  }
}

static void
detach_toolbar(GtkHandleBox *handlebox, GtkWidget *widget, gpointer user_data)
{
  gtk_toolbar_set_show_arrow(GTK_TOOLBAR(widget), (int) user_data);
}

static GtkWidget *
create_toolbar(GtkWidget *box, GtkOrientation o, GtkWidget **hbox)
{
  GtkWidget *t, *w;
  GtkPositionType p;

  if (o == GTK_ORIENTATION_HORIZONTAL) {
    p =  GTK_POS_LEFT;
  } else {
    p =  GTK_POS_TOP;
  }

  t = gtk_toolbar_new();
  gtk_toolbar_set_style(GTK_TOOLBAR(t), GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_show_arrow(GTK_TOOLBAR(t), TRUE);
#if (GTK_MAJOR_VERSION > 2 || (GTK_MAJOR_VERSION == 2 && GTK_MINOR_VERSION >= 16))
  gtk_orientable_set_orientation(GTK_ORIENTABLE(t), o);
#else
  gtk_toolbar_set_orientation(GTK_TOOLBAR(t), o);
#endif
  w = gtk_handle_box_new();
  g_signal_connect(w, "child-attached", G_CALLBACK(detach_toolbar), GINT_TO_POINTER(TRUE));
  g_signal_connect(w, "child-detached", G_CALLBACK(detach_toolbar), GINT_TO_POINTER(FALSE));
  gtk_handle_box_set_handle_position(GTK_HANDLE_BOX(w), p);
  gtk_container_add(GTK_CONTAINER(w), t);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  *hbox = w;

  return t;
}

static void
set_widget_visibility(int cross)
{
  if (Menulocal.ruler) {
    gtk_widget_show(NgraphApp.Viewer.HRuler);
    gtk_widget_show(NgraphApp.Viewer.VRuler);
  } else {
    gtk_widget_hide(NgraphApp.Viewer.HRuler);
    gtk_widget_hide(NgraphApp.Viewer.VRuler);
  }

  if (Menulocal.statusb) {
    gtk_widget_show(NgraphApp.Message);
  } else {
    gtk_widget_hide(NgraphApp.Message);
  }

  if (Menulocal.scrollbar) {
    gtk_widget_show(NgraphApp.Viewer.HScroll);
    gtk_widget_show(NgraphApp.Viewer.VScroll);
  } else {
    gtk_widget_hide(NgraphApp.Viewer.HScroll);
    gtk_widget_hide(NgraphApp.Viewer.VScroll);
  }

  if (Menulocal.ctoolbar) {
    gtk_widget_show(NgraphApp.Viewer.CToolbar);
  } else {
    gtk_widget_hide(NgraphApp.Viewer.CToolbar);
  }

  if (Menulocal.ptoolbar) {
    gtk_widget_show(NgraphApp.Viewer.PToolbar);
  } else {
    gtk_widget_hide(NgraphApp.Viewer.PToolbar);
  }

  ViewCross(cross);
}

static GtkWidget *
create_message_box(GtkWidget **label1, GtkWidget **label2)
{
  GtkWidget *frame, *w, *hbox;

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

  hbox = gtk_hbox_new(FALSE, 4);

  w = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.5);
  gtk_label_set_width_chars(GTK_LABEL(w), 16);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
  *label1 = w;

  w = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(w), 0, 0.5);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
  *label2 = w;

  gtk_container_add(GTK_CONTAINER(frame), hbox);

  return frame;
}

static void
setupwindow(void)
{
  GtkWidget *menubar, *command1, *command2, *hbox, *vbox, *table;

  vbox = gtk_vbox_new(FALSE, 0);
  hbox = gtk_hbox_new(FALSE, 0);

  menubar = gtk_menu_bar_new();
  NgraphApp.Viewer.menu = menubar;

  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
  createmenu(GTK_MENU_BAR(menubar));

  command1 = create_toolbar(vbox, GTK_ORIENTATION_HORIZONTAL, &NgraphApp.Viewer.CToolbar);
  command2 = create_toolbar(hbox, GTK_ORIENTATION_VERTICAL, &NgraphApp.Viewer.PToolbar);

  NgraphApp.Viewer.HScroll = gtk_hscrollbar_new(NULL);
  NgraphApp.Viewer.VScroll = gtk_vscrollbar_new(NULL);
  NgraphApp.Viewer.HRuler = gtk_hruler_new();
  NgraphApp.Viewer.VRuler = gtk_vruler_new();
  NgraphApp.Viewer.Win = gtk_drawing_area_new();

  gtk_ruler_set_metric(GTK_RULER(NgraphApp.Viewer.HRuler), GTK_CENTIMETERS);
  gtk_ruler_set_metric(GTK_RULER(NgraphApp.Viewer.VRuler), GTK_CENTIMETERS);

  table = gtk_table_new(3, 3, FALSE);

  gtk_table_attach(GTK_TABLE(table),
		   NgraphApp.Viewer.HRuler,
		   1, 2, 0, 1,
		   GTK_FILL, GTK_FILL,
		   0, 0);

  gtk_table_attach(GTK_TABLE(table),
		   NgraphApp.Viewer.VRuler,
		   0, 1, 1, 2,
		   GTK_FILL, GTK_FILL,
		   0, 0);

  gtk_table_attach(GTK_TABLE(table),
		   NgraphApp.Viewer.HScroll,
		   1, 2, 2, 3,
		   GTK_FILL, GTK_FILL,
		   0, 0);

  gtk_table_attach(GTK_TABLE(table),
		   NgraphApp.Viewer.VScroll,
		   2, 3, 1, 2,
		   GTK_FILL, GTK_FILL,
		   0, 0);

  gtk_table_attach(GTK_TABLE(table),
		   NgraphApp.Viewer.Win,
		   1, 2, 1, 2,
		   GTK_EXPAND | GTK_FILL, GTK_EXPAND | GTK_FILL,
		   0, 0);

  gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);

  NgraphApp.Message = gtk_statusbar_new();
  gtk_box_pack_end(GTK_BOX(NgraphApp.Message),
		   create_message_box(&NgraphApp.Message_pos, &NgraphApp.Message_extra),
		   FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), NgraphApp.Message, FALSE, FALSE, 0);

  NgraphApp.Message1 = gtk_statusbar_get_context_id(GTK_STATUSBAR(NgraphApp.Message), "Message1");

  createcommand1(GTK_TOOLBAR(command1));
  createcommand2(GTK_TOOLBAR(command2));

  set_axis_undo_button_sensitivity(FALSE);

  gtk_container_add(GTK_CONTAINER(TopLevel), vbox);
}

static void
defaultwindowconfig(void)
{
  int w, h;

  w = gdk_screen_get_width(gdk_screen_get_default());
  h = w / 2 * 1.2;

  if (Menulocal.fileopen) {
    if (Menulocal.filewidth == DEFAULT_GEOMETRY)
      Menulocal.filewidth = w / 4;

    if (Menulocal.fileheight == DEFAULT_GEOMETRY)
      Menulocal.fileheight = h / 4;

    if (Menulocal.filex == DEFAULT_GEOMETRY)
      Menulocal.filex = -Menulocal.filewidth - 4;

    if (Menulocal.filey == DEFAULT_GEOMETRY)
      Menulocal.filey = 0;
  }

  if (Menulocal.axisopen) {
    if (Menulocal.axiswidth == DEFAULT_GEOMETRY)
      Menulocal.axiswidth = w / 4;

    if (Menulocal.axisheight == DEFAULT_GEOMETRY)
      Menulocal.axisheight = h / 4;

    if (Menulocal.axisx == DEFAULT_GEOMETRY)
      Menulocal.axisx = -Menulocal.axiswidth - 4;

    if (Menulocal.axisy == DEFAULT_GEOMETRY)
      Menulocal.axisy = Menulocal.fileheight + 4;
  }

  if (Menulocal.coordopen) {
    if (Menulocal.coordwidth == DEFAULT_GEOMETRY)
      Menulocal.coordwidth = w / 4;

    if (Menulocal.coordheight == DEFAULT_GEOMETRY)
      Menulocal.coordheight = h / 4;

    if (Menulocal.coordx == DEFAULT_GEOMETRY)
      Menulocal.coordx = -Menulocal.coordwidth - 4;

    if (Menulocal.coordy == DEFAULT_GEOMETRY)
      Menulocal.coordy = Menulocal.fileheight + Menulocal.axisheight + 8;
  }
}

static void
set_gdk_color(GdkColor *col, int r, int g, int b)
{
  col->red = r << 8;
  col->green = g << 8;
  col->blue = b << 8;
}

static void
load_hist_file(GtkEntryCompletion *list, char *home, char *name)
{
  char *filename;

  filename = g_strdup_printf("%s/%s", home, name);
  entry_completion_load(list, filename, Menulocal.hist_size);
  g_free(filename);
}

static void
save_hist_file(GtkEntryCompletion *list, char *home, char *name)
{
  char *filename;

  filename = g_strdup_printf("%s/%s", home, name);
  entry_completion_save(list, filename, Menulocal.hist_size);
  g_free(filename);
}

static void
load_hist(void)
{
  char *home;

  NgraphApp.legend_text_list = entry_completion_create();
  NgraphApp.x_math_list = entry_completion_create();
  NgraphApp.y_math_list = entry_completion_create();
  NgraphApp.func_list = entry_completion_create();

  home = get_home();
  if (home == NULL)
    return;

  load_hist_file(NgraphApp.legend_text_list, home, TEXT_HISTORY);
  load_hist_file(NgraphApp.x_math_list, home, MATH_X_HISTORY);
  load_hist_file(NgraphApp.y_math_list, home, MATH_Y_HISTORY);
  load_hist_file(NgraphApp.func_list, home, FUNCTION_HISTORY);
}

static void
save_entry_history(void)
{
  char *home;

  home = get_home();
  if (home == NULL)
    return;

  save_hist_file(NgraphApp.legend_text_list, home, TEXT_HISTORY);
  save_hist_file(NgraphApp.x_math_list, home, MATH_X_HISTORY);
  save_hist_file(NgraphApp.y_math_list, home, MATH_Y_HISTORY);
  save_hist_file(NgraphApp.func_list, home, FUNCTION_HISTORY);

  g_object_unref(NgraphApp.legend_text_list);
  g_object_unref(NgraphApp.x_math_list);
  g_object_unref(NgraphApp.y_math_list);
  g_object_unref(NgraphApp.func_list);

  NgraphApp.legend_text_list = NULL;
  NgraphApp.x_math_list = NULL;
  NgraphApp.y_math_list = NULL;
  NgraphApp.func_list = NULL;
}

static void
init_ngraph_app_struct(void)
{
  NgraphApp.Viewer.Win = NULL;
  NgraphApp.Viewer.popup = NULL;

  memset(&NgraphApp.FileWin, 0, sizeof(NgraphApp.FileWin));
  NgraphApp.FileWin.can_focus = FALSE;

  memset(&NgraphApp.AxisWin, 0, sizeof(NgraphApp.AxisWin));
  NgraphApp.AxisWin.can_focus = TRUE;

  memset(&NgraphApp.LegendWin, 0, sizeof(NgraphApp.LegendWin));
  NgraphApp.LegendWin.can_focus = TRUE;

  memset(&NgraphApp.MergeWin, 0, sizeof(NgraphApp.MergeWin));
  NgraphApp.MergeWin.can_focus = TRUE;

  memset(&NgraphApp.InfoWin, 0, sizeof(NgraphApp.InfoWin));
  NgraphApp.InfoWin.can_focus = FALSE;

  memset(&NgraphApp.CoordWin, 0, sizeof(NgraphApp.CoordWin));
  NgraphApp.CoordWin.can_focus = FALSE;

  NgraphApp.legend_text_list = NULL;
  NgraphApp.x_math_list = NULL;
  NgraphApp.y_math_list = NULL;
  NgraphApp.func_list = NULL;

  NgraphApp.Interrupt = FALSE;
}

static void
create_sub_windows(void)
{
  if (Menulocal.dialogopen)
    CmInformationWindow(NULL, NULL);

  if (Menulocal.coordopen)
    CmCoordinateWindow(NULL, NULL);

  if (Menulocal.mergeopen)
    CmMergeWindow(NULL, NULL);

  if (Menulocal.legendopen)
    CmLegendWindow(NULL, NULL);

  if (Menulocal.axisopen)
    CmAxisWindow(NULL, NULL);

  if (Menulocal.fileopen)
    CmFileWindow(NULL, NULL);
}

static void
destroy_sub_windows(void)
{
  if (NgraphApp.FileWin.Win)
    gtk_widget_destroy(NgraphApp.FileWin.Win);

  if (NgraphApp.AxisWin.Win)
    gtk_widget_destroy(NgraphApp.AxisWin.Win);

  if (NgraphApp.LegendWin.Win)
    gtk_widget_destroy(NgraphApp.LegendWin.Win);

  if (NgraphApp.MergeWin.Win)
    gtk_widget_destroy(NgraphApp.MergeWin.Win);

  if (NgraphApp.InfoWin.Win)
    gtk_widget_destroy(NgraphApp.InfoWin.Win);

  if (NgraphApp.CoordWin.Win)
    gtk_widget_destroy(NgraphApp.CoordWin.Win);
}

#ifdef WINDOWS
enum SUB_WINDOW_STATE {
  FILE_WIN_VISIBLE   = 0x01,
  AXIS_WIN_VISIBLE   = 0x02,
  LEGEND_WIN_VISIBLE = 0x04,
  MERGE_WIN_VISIBLE  = 0x08,
  INFO_WIN_VISIBLE   = 0x10,
  COORD_WIN_VISIBLE  = 0x20,
};

static gboolean
change_window_state_cb(GtkWidget *widget, GdkEventWindowState *event, gpointer user_data)
{
  static int window_state = 0;

  if (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) {
    if (NgraphApp.FileWin.Win && GTK_WIDGET_VISIBLE(NgraphApp.FileWin.Win)) {
      window_state |= FILE_WIN_VISIBLE;
      gtk_widget_hide(NgraphApp.FileWin.Win);
    }
    if (NgraphApp.AxisWin.Win && GTK_WIDGET_VISIBLE(NgraphApp.AxisWin.Win)) {
      window_state |= AXIS_WIN_VISIBLE;
      gtk_widget_hide(NgraphApp.AxisWin.Win);
    }
    if (NgraphApp.LegendWin.Win && GTK_WIDGET_VISIBLE(NgraphApp.LegendWin.Win)) {
      window_state |= LEGEND_WIN_VISIBLE;
      gtk_widget_hide(NgraphApp.LegendWin.Win);
    }
    if (NgraphApp.InfoWin.Win && GTK_WIDGET_VISIBLE(NgraphApp.InfoWin.Win)) {
      window_state |= INFO_WIN_VISIBLE;
      gtk_widget_hide(NgraphApp.InfoWin.Win);
    }
    if (NgraphApp.CoordWin.Win && GTK_WIDGET_VISIBLE(NgraphApp.CoordWin.Win)) {
      window_state |= COORD_WIN_VISIBLE;
      gtk_widget_hide(NgraphApp.CoordWin.Win);
    }
  } else if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED) {
    if (NgraphApp.FileWin.Win && (window_state & FILE_WIN_VISIBLE)) {
      gtk_widget_show(NgraphApp.FileWin.Win);
    }
    if (NgraphApp.AxisWin.Win && (window_state & AXIS_WIN_VISIBLE)) {
      gtk_widget_show(NgraphApp.AxisWin.Win);
    }
    if (NgraphApp.LegendWin.Win && (window_state & LEGEND_WIN_VISIBLE)) {
      gtk_widget_show(NgraphApp.LegendWin.Win);
    }
    if (NgraphApp.InfoWin.Win && (window_state & INFO_WIN_VISIBLE)) {
      gtk_widget_show(NgraphApp.InfoWin.Win);
    }
    if (NgraphApp.CoordWin.Win && (window_state & COORD_WIN_VISIBLE)) {
      gtk_widget_show(NgraphApp.CoordWin.Win);
    }
    window_state = 0;
  }

  return FALSE;
}
#endif

void
set_modified_state(int state)
{
  gtk_widget_set_sensitive(SaveMenuItem, state);
  gtk_widget_set_sensitive(SaveBtn, state);
}

int
application(char *file)
{
  int i, terminated;
  struct objlist *aobj;
  int x, y, width, height, w, h;
  GdkScreen *screen;

  if (TopLevel)
    return 1;

  init_ngraph_app_struct();

  screen = gdk_screen_get_default();
  w = gdk_screen_get_width(screen);
  h = gdk_screen_get_height(screen);

  if (Menulocal.menux == DEFAULT_GEOMETRY)
    Menulocal.menux = w * 3 / 8;

  if (Menulocal.menuy == DEFAULT_GEOMETRY)
    Menulocal.menuy = h / 8;

  if (Menulocal.menuwidth == DEFAULT_GEOMETRY)
    Menulocal.menuwidth = w / 2;

  if (Menulocal.menuheight == DEFAULT_GEOMETRY)
    Menulocal.menuheight = h / 1.2;

  x = Menulocal.menux;
  y = Menulocal.menuy;
  width = Menulocal.menuwidth;
  height = Menulocal.menuheight;

  load_hist();

  CurrentWindow = TopLevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(TopLevel), AppName);

  gtk_window_set_default_size(GTK_WINDOW(TopLevel), width, height);
  gtk_window_move(GTK_WINDOW(TopLevel), x, y);

#ifdef WINDOWS
  g_signal_connect(TopLevel, "window-state-event", G_CALLBACK(change_window_state_cb), NULL);
#endif
  g_signal_connect(TopLevel, "delete-event", G_CALLBACK(CloseCallback), NULL);
  g_signal_connect(TopLevel, "destroy-event", G_CALLBACK(CloseCallback), NULL);

  set_gdk_color(&white, 255, 255, 255);
  set_gdk_color(&gray,  0xaa, 0xaa, 0xaa);

  create_icon();

  create_toolbar_pixmap();

  initdialog();

  gtk_widget_show_all(GTK_WIDGET(TopLevel));
  reset_event();
  setupwindow();

  NgraphApp.FileName = NULL;

  gtk_widget_show_all(GTK_WIDGET(TopLevel));
  ViewerWinSetup();
  set_widget_visibility(Menulocal.show_cross);

  create_markpixmap(TopLevel);

  if (create_cursor())
    return 1;

  reset_graph_modified();
  NSetCursor(GDK_LEFT_PTR);

  putstderr = mgtkputstderr;
  printfstderr = mgtkprintfstderr;
  putstdout = mgtkputstdout;
  printfstdout = mgtkprintfstdout;
  ndisplaydialog = mgtkdisplaydialog;
  ndisplaystatus = mgtkdisplaystatus;
  ninterrupt = mgtkinterrupt;
  inputyn = mgtkinputyn;

  if ((aobj = getobject("axis")) != NULL) {
    for (i = 0; i <= chkobjlastinst(aobj); i++)
      exeobj(aobj, "tight", i, 0, NULL);
  }
  if ((aobj = getobject("axisgrid")) != NULL) {
    for (i = 0; i <= chkobjlastinst(aobj); i++)
      exeobj(aobj, "tight", i, 0, NULL);
  }

  if (file != NULL) {
    char *ext;
    ext = getextention(file);
    if (ext) {
      if ((strcmp0(ext, "PRM") == 0) || (strcmp0(ext, "prm") == 0)) {
	LoadPrmFile(file);
      } else if ((strcmp0(ext, "NGP") == 0) || (strcmp0(ext, "ngp") == 0)) {
	LoadNgpFile(file, Menulocal.ignorepath, Menulocal.expand, Menulocal.expanddir, FALSE, NULL);
      }
    }
  }

  CmViewerDrawB(NgraphApp.Viewer.Win, NULL);

#ifndef WINDOWS
  set_signal(SIGINT, 0, kill_signal_handler);
  set_signal(SIGTERM, 0, term_signal_handler);
#endif	/* WINDOWS */

  gtk_widget_show_all(GTK_WIDGET(TopLevel));
  set_widget_visibility(Menulocal.show_cross);

  create_sub_windows();

  terminated = AppMainLoop();

#if 0
  gtk_accel_map_save(KEYMAP_FILE);
#endif

#ifndef WINDOWS
  set_signal(SIGTERM, 0, SIG_DFL);
  set_signal(SIGINT, 0, SIG_DFL);
#endif	/* WINDOWS */

  SaveHistory();
  save_entry_history();
  menu_save_config(SAVE_CONFIG_TYPE_TOGGLE_VIEW |
		   SAVE_CONFIG_TYPE_EXPORT_IMAGE);

  ViewerWinClose();

  destroy_sub_windows();

  g_free(NgraphApp.FileName);
  NgraphApp.FileName = NULL;

  gtk_widget_destroy(TopLevel);
  CurrentWindow = TopLevel = NULL;
  Menulocal.win = NULL;

  free_markpixmap();
  free_cursor();

  reset_event();

  if (terminated) {
    delobj(getobject("system"), 0);
  }

  return 0;
}

void
UpdateAll(void)
{
  ViewerWinUpdate(TRUE); 
  UpdateAll2();
}

void
UpdateAll2(void)
{
  FileWinUpdate(TRUE);
  AxisWinUpdate(TRUE);
  LegendWinUpdate(TRUE);
  MergeWinUpdate(TRUE);
  InfoWinUpdate(TRUE);
  CoordWinUpdate(TRUE);
}

void
ChangePage(void)
{
  CloseGRA();
  OpenGRA();
  CloseGC();
  CloseGRA();
  OpenGRA();
  OpenGC();
  SetScroller();
  ChangeDPI(TRUE);
}

static void
SetStatusBarSub(const char *mes, guint id)
{

  if (NgraphApp.Message) {
    gtk_statusbar_push(GTK_STATUSBAR(NgraphApp.Message), id, mes);
  }
}

static void
ResetStatusBarSub(guint id)
{
  if (NgraphApp.Message) {
    gtk_statusbar_pop(GTK_STATUSBAR(NgraphApp.Message), id);
  }
}

void
SetPoint(struct Viewer *d, int x, int y)
{
  char buf[128];
  struct Point *po;
  unsigned int num;

  //  x += Menulocal.LeftMargin;
  //  y += Menulocal.TopMargin;

  if (NgraphApp.Message && GTK_WIDGET_VISIBLE(NgraphApp.Message)) {
    snprintf(buf, sizeof(buf), "% 6.2f, % 6.2f", x / 100.0, y / 100.0);
    gtk_label_set_text(GTK_LABEL(NgraphApp.Message_pos), buf);

    switch (d->MouseMode) {
    case MOUSECHANGE:
      if (d->Angle >= 0) {
	snprintf(buf, sizeof(buf), "%6.2f°", d->Angle / 100.0);
	gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), buf);
      } else {
	snprintf(buf, sizeof(buf), "(% .2f, % .2f)", d->LineX / 100.0, d->LineY / 100.0);
	gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), buf);
      }
      break;
    case MOUSEZOOM1:
    case MOUSEZOOM2:
    case MOUSEZOOM3:
    case MOUSEZOOM4:
      snprintf(buf, sizeof(buf), "% .2f%%", d->Zoom * 100);
      gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), buf);
      break;
    case MOUSEDRAG:
      snprintf(buf, sizeof(buf), "(% .2f, % .2f)", d->FrameOfsX / 100.0, d->FrameOfsY / 100.0);
      gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), buf);
      break;
    default:
      num =  arraynum(d->points);
      po = (num > 1) ? (* (struct Point **) arraynget(d->points, num - 2)) : NULL;
      if (d->Capture && po) {
	snprintf(buf, sizeof(buf), "(% .2f, % .2f)", (x - po->x) / 100.0, (y - po->y) / 100.0);
	gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), buf);
      } else {
	gtk_label_set_text(GTK_LABEL(NgraphApp.Message_extra), NULL);
      }
    }
  }

  g_object_set(NgraphApp.Viewer.HRuler, "position", N2GTK_RULER_METRIC(x), NULL);
  g_object_set(NgraphApp.Viewer.VRuler, "position", N2GTK_RULER_METRIC(y), NULL);

  CoordWinSetCoord(x, y);
}

void
SetStatusBar(const char *mes)
{
  SetStatusBarSub(mes, NgraphApp.Message1);
}

void
ResetStatusBar(void)
{
  ResetStatusBarSub(NgraphApp.Message1);
}

unsigned int
NGetCursor(void)
{
  return CursorType;
}

void
NSetCursor(unsigned int type)
{
  struct Viewer *d;
  GdkWindow *win;

  if (NgraphApp.cursor == NULL || CursorType == type)
    return;

  d = &(NgraphApp.Viewer);
  win = d->gdk_win;

  CursorType = type;

  switch (type) {
  case GDK_LEFT_PTR:
    gdk_window_set_cursor(win, NgraphApp.cursor[0]);
    break;
  case GDK_XTERM:
    gdk_window_set_cursor(win, NgraphApp.cursor[1]);
    break;
  case GDK_CROSSHAIR:
    gdk_window_set_cursor(win, NgraphApp.cursor[2]);
    break;
  case GDK_TOP_LEFT_CORNER:
    gdk_window_set_cursor(win, NgraphApp.cursor[3]);
    break;
  case GDK_TOP_RIGHT_CORNER:
    gdk_window_set_cursor(win, NgraphApp.cursor[4]);
    break;
  case GDK_BOTTOM_RIGHT_CORNER:
    gdk_window_set_cursor(win, NgraphApp.cursor[5]);
    break;
  case GDK_BOTTOM_LEFT_CORNER:
    gdk_window_set_cursor(win, NgraphApp.cursor[6]);
    break;
  case GDK_TARGET:
    gdk_window_set_cursor(win, NgraphApp.cursor[7]);
    break;
  case GDK_PLUS:
    gdk_window_set_cursor(win, NgraphApp.cursor[8]);
    break;
  case GDK_SIZING:
    gdk_window_set_cursor(win, NgraphApp.cursor[9]);
    break;
  case GDK_WATCH:
    gdk_window_set_cursor(win, NgraphApp.cursor[10]);
    break;
  case GDK_FLEUR:
    gdk_window_set_cursor(win, NgraphApp.cursor[11]);
    break;
  case GDK_PENCIL:
    gdk_window_set_cursor(win, NgraphApp.cursor[12]);
    break;
  case GDK_TCROSS:
    gdk_window_set_cursor(win, NgraphApp.cursor[13]);
    break;
  }
}

void
DisplayDialog(const char *str)
{
  char *ustr;

  if (str == NULL)
    return;

  ustr = g_locale_to_utf8(CHK_STR(str), -1, NULL, NULL, NULL);
  InfoWinDrawInfoText(ustr);
  g_free(ustr);
}

int
PutStdout(const char *s)
{
  gssize len;

  if (s == NULL)
    return 0;

  len = strlen(s);
  DisplayDialog(s);
  return len + 1;
}

int
PutStderr(const char *s)
{
  gssize len;
  gsize rlen, wlen;
  char *ustr;

  if (s == NULL)
    return 0;

  len = strlen(s);
  ustr = g_locale_to_utf8(s, len, &rlen, &wlen, NULL);
  message_box(NULL, ustr, _("Error:"), RESPONS_ERROR);
  g_free(ustr);
  UpdateAll2();
  return len + 1;
}

int
ChkInterrupt(void)
{
#if 0
  GdkEvent *e;
  GtkWidget *w;

  e = gtk_get_current_event();

  if(e == NULL)
    return FALSE;

  w = gtk_get_event_widget(e);
  if (w &&
      (e->type == GDK_BUTTON_PRESS || e->type == GDK_BUTTON_RELEASE)) {
    //  if (w && e->type != GDK_EXPOSE) {
    gtk_propagate_event(w, e);
  }
  gdk_event_free(e);
  if (NgraphApp.Interrupt) {
    NgraphApp.Interrupt = FALSE;
    return TRUE;
  }
#else
  if (DrawLock != DrawLockDraw)
    return FALSE;

  while (gtk_events_pending()) {
    gtk_main_iteration_do(FALSE);
    if (NgraphApp.Interrupt) {
      NgraphApp.Interrupt = FALSE;
      return TRUE;
    }
  }
#endif

  return FALSE;
}

int
InputYN(const char *mes)
{
  int ret;

  ret = message_box((CurrentWindow) ? CurrentWindow : TopLevel, mes, _("Question"), RESPONS_YESNO);
  UpdateAll2();
  return (ret == IDYES) ? TRUE : FALSE;
}

static void
script_exec(GtkWidget *w, gpointer client_data)
{
  char *name, *option, *s, *argv[2], mes[256];
  int newid, allocnow = FALSE, len, idn;
  struct narray sarray;
  struct objlist *robj, *shell;
  struct script *fcur;

  if (Menulock || Globallock || client_data == NULL)
    return;

  shell = chkobject("shell");
  if (shell == NULL)
    return;

  fcur = (struct script *) client_data;
  if (fcur->script == NULL)
    return;

  name = g_strdup(fcur->script);
  if (name == NULL)
    return;

  newid = newobj(shell);
  if (newid < 0) {
    g_free(name);
    return;
  }

  arrayinit(&sarray, sizeof(char *));
  if (arrayadd(&sarray, &name) == NULL) {
    delobj(shell, newid);
    g_free(name);
    arraydel2(&sarray);
    return;
  }

  option = fcur->option;
  while ((s = getitok2(&option, &len, " \t")) != NULL) {
    if (arrayadd(&sarray, &s) == NULL) {
      delobj(shell, newid);
      g_free(s);
      arraydel2(&sarray);
      return;
    }
  }

  if (Menulocal.addinconsole) {
    allocnow = allocate_console();
  }

  snprintf(mes, sizeof(mes), _("Executing `%.128s'."), name);
  SetStatusBar(mes);

  menu_lock(TRUE);

  idn = getobjtblpos(Menulocal.obj, "_evloop", &robj);
  registerevloop(chkobjectname(Menulocal.obj), "_evloop", robj, idn, Menulocal.inst, NULL);
  argv[0] = (char *) &sarray;
  argv[1] = NULL;
  exeobj(shell, "shell", newid, 1, argv);
  unregisterevloop(robj, idn, Menulocal.inst);

  menu_lock(FALSE);

  ResetStatusBar();
  arraydel2(&sarray);

  if (Menulocal.addinconsole) {
    free_console(allocnow);
  }

  GetPageSettingsFromGRA();
  UpdateAll2();

  delobj(shell, newid);
  gdk_window_invalidate_rect(NgraphApp.Viewer.gdk_win, NULL, FALSE);
}

static void
CmReloadWindowConfig(GtkMenuItem *w, gpointer user_data)
{
  gint x, y, w0, h0;

  sub_window_hide(&(NgraphApp.FileWin));
  sub_window_hide(&(NgraphApp.AxisWin));
  sub_window_hide(&(NgraphApp.MergeWin));
  sub_window_hide((struct SubWin *) &(NgraphApp.LegendWin));
  sub_window_hide((struct SubWin *) &(NgraphApp.InfoWin));
  sub_window_hide((struct SubWin *) &(NgraphApp.CoordWin));

  initwindowconfig();
  mgtkwindowconfig();

  gtk_window_get_position(GTK_WINDOW(TopLevel), &x, &y);
  gtk_window_get_size(GTK_WINDOW(TopLevel), &w0, &h0);

  Menulocal.menux = x;
  Menulocal.menuy = y;
  Menulocal.menuwidth = w0;
  Menulocal.menuheight = h0;

  defaultwindowconfig();

  if (Menulocal.dialogopen) {
    CmInformationWindow(NULL, NULL);
    sub_window_set_geometry((struct SubWin *) &(NgraphApp.InfoWin), TRUE);
  }

  if (Menulocal.coordopen) {
    CmCoordinateWindow(NULL, NULL);
    sub_window_set_geometry((struct SubWin *) &(NgraphApp.CoordWin), TRUE);
  }

  if (Menulocal.mergeopen) {
    CmMergeWindow(NULL, NULL);
    sub_window_set_geometry(&(NgraphApp.MergeWin), TRUE);
  }

  if (Menulocal.legendopen) {
    CmLegendWindow(NULL, NULL);
    sub_window_set_geometry((struct SubWin *) &(NgraphApp.LegendWin), TRUE);
  }

  if (Menulocal.axisopen) {
    CmAxisWindow(NULL, NULL);
    sub_window_set_geometry(&(NgraphApp.AxisWin), TRUE);
  }

  if (Menulocal.fileopen) {
    CmFileWindow(NULL, NULL);
    sub_window_set_geometry(&(NgraphApp.FileWin), TRUE);
  }
}


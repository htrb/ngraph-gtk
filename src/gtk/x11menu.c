/* 
 * $Id: x11menu.c,v 1.69 2009/02/24 02:40:40 hito Exp $
 */

#include "gtk_common.h"

#include <sys/types.h>
#include <sys/wait.h>
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
#include "jnstring.h"
#include "nstring.h"
#include "nconfig.h"
#include "mathfn.h"
#include "gra.h"
#include "spline.h"

#include "gtk_entry_completion.h"
#include "gtk_subwin.h"

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
#include "x11scrip.h"
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

int Menulock = FALSE;
struct NgraphApp NgraphApp;
GtkWidget *TopLevel = NULL;
GtkAccelGroup *AccelGroup = NULL;

static int Hide_window = FALSE, Toggle_cb_disable = FALSE, DrawLock = FALSE;
static unsigned int CursorType;
static GtkWidget *ShowFileWin = NULL, *ShowAxisWin = NULL,
  *ShowLegendWin = NULL, *ShowMergeWin = NULL,
  *ShowCoodinateWin = NULL, *ShowInfoWin = NULL, *ShowStatusBar = NULL,
  *RecentGraph = NULL, *RecentData = NULL, *AddinMenu = NULL;

static void CmReloadWindowConfig(GtkMenuItem *w, gpointer user_data);

struct command_data {
  void (*func)(GtkWidget *, gpointer);
  gchar *label, *tip, *caption;
  const char **xpm;
  int type;
  GtkWidget *img;
};

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

static struct command_data Command1_data[] = {
  {
    CmFileWindow,
    N_("Data"),
    "Data Window",
    N_("Activate Data Window"), 
    Filewin_xpm,
  },
  {
    CmAxisWindow,
    N_("Axis"),
    "Axis Window",
    N_("Activate Axis Window"), 
    Axiswin_xpm,
  },
  {
    CmLegendWindow,
    N_("Legend"),
    "Legend Window",
    N_("Activate Legend Window"), 
    Legendwin_xpm,
  },
  {
    CmMergeWindow,
    N_("Merge"),
    "Merge Window",
    N_("Activate Merge Window"), 
    Mergewin_xpm,
  },
  {
    CmCoordinateWindow,
    N_("Coordinate"),
    "Coordinate Window",
    N_("Activate Coordinate Window"), 
    Coordwin_xpm,
  },
  {
    CmInformationWindow,
    N_("Information"),
    "Information Window",
    N_("Activate Information Window"), 
    Infowin_xpm,
  },
  {NULL},
  {
    CmFileOpenB,
    N_("Open"),
    N_("Open Data"),
    N_("Open Data file"), 
    Fileopen_xpm,
  },
  {
    CmGraphLoadB,
    N_("Load"),
    N_("Load NGP"),
    N_("Load NGP file"), 
    Load_xpm,
  },
  {
    CmGraphSaveB,
    N_("Save"),
    N_("Save NGP"),
    N_("Save NGP file "), 
    Save_xpm,
  },
  {
    CmAxisClear,
    N_("Clear"),
    N_("Clear Scale"),
    N_("Clear Scale"), 
    Scale_xpm,
  },
  {
    CmViewerDrawB,
    N_("Draw"),
    N_("Draw"),
    N_("Draw on Viewer Window"), 
    Draw_xpm,
  },
  {
    CmViewerClearB,
    N_("Clear Image"),
    N_("Clear Image"),
    N_("Clear Viewer Window"), 
    Clear_xpm,
  },
  {
    CmOutputPrinterB,
    N_("Print"),
    N_("Print"),
    N_("Print"), 
    Print_xpm,
  },
  {
    CmOutputViewerB,
    N_("Viewer"),
    N_("Ex. Viewer"),
    N_("Open External Viewer"), 
    Preview_xpm,
  },
  {NULL},
  {
    CmFileWinMath,
    N_("Math"),
    N_("Math Transformation"),
    N_("Set Math Transformation"), 
    Math_xpm,
  },
  {
    CmAxisWinScaleUndo,
    N_("Undo"),
    N_("Scale Undo"),
    N_("Undo Scale Settings"), 
    Scaleundo_xpm,
  },
}; 

static struct command_data Command2_data[] = {
  {
    NULL,
    N_("Point"),
    N_("Pointer"),
    N_("Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"), 
    Point_xpm,
    PointB,
  },
  {
    NULL,
    N_("Legend"),
    N_("Legend Pointer"),
    N_("Legend Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"),
    Legendpoint_xpm,
    LegendB,
  },
  {
    NULL,
    N_("Axis"),
    N_("Axis Pointer"),
    N_("Axis Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"),
    Axispoint_xpm,
    AxisB,
  },
  {
    NULL,
    N_("Data"),
    N_("Data Pointer"),
    N_("Data Pointer"),
    Datapoint_xpm,
    DataB,
  },
  {NULL},
  {
    NULL,
    N_("Line"),
    N_("Line"),
    N_("New Legend Line (+SHIFT: Fine +CONTROL: snap angle)"), 
    Line_xpm,
    LineB,
  },
  {
    NULL,
    N_("Curve"),
    N_("Curve"),
    N_("New Legend Curve (+SHIFT: Fine +CONTROL: snap angle)"), 
    Curve_xpm,
    CurveB,

  },
  {
    NULL,
    N_("Polygon"),
    N_("Polygon"),
    N_("New Legend Polygon (+SHIFT: Fine +CONTROL: snap angle)"), 
    Polygon_xpm,
    PolyB,
  },
  {
    NULL,
    N_("Rectangle"),
    N_("Rectangle"),
    N_("New Legend Rectangle (+SHIFT: Fine +CONTROL: square integer ratio rectangle)"), 
    Rect_xpm,
    RectB,
  },
  {
    NULL,
    N_("Arc"),
    N_("Arc"),
    N_("New Legend Arc (+SHIFT: Fine +CONTROL: circle or integer ratio ellipse)"), 
    Arc_xpm,
    ArcB,
  },
  {
    NULL,
    N_("Mark"),
    N_("Mark"),
    N_("New Legend Mark (+SHIFT: Fine)"), 
    Mark_xpm,
    MarkB,
  },
  {
    NULL,
    N_("Text"),
    N_("Text"),
    N_("New Legend Text (+SHIFT: Fine)"), 
    Text_xpm,
    TextB,
  },
  {
    NULL,
    N_("Gauss"),
    N_("Gaussian"),
    N_("New Legend Gaussian (+SHIFT: Fine +CONTROL: integer ratio)"), 
    Gauss_xpm,
    GaussB,
  },
  {
    NULL,
    N_("Frame axis"),
    N_("Frame Graph"),
    N_("New Frame Graph (+SHIFT: Fine +CONTROL: integer ratio)"), 
    Frame_xpm,
    FrameB,
  },
  {
    NULL,
    N_("Section axis"),
    N_("Section Graph"),
    N_("New Section Graph (+SHIFT: Fine +CONTROL: integer ratio)"), 
    Section_xpm,
    SectionB,
  },
  {
    NULL,
    N_("Cross axis"),
    N_("Cross Graph"),
    N_("New Cross Graph (+SHIFT: Fine +CONTROL: integer ratio)"), 
    Cross_xpm,
    CrossB,
  },
  {
    NULL,
    N_("Single axis"),
    N_("Single Axis"),
    N_("New Single Axis (+SHIFT: Fine +CONTROL: snap angle)"), 
    Single_xpm,
    SingleB,
  },
  {NULL},
  {
    NULL,
    N_("Trimming"),
    N_("Axis Trimming"),
    N_("Axis Trimming (+SHIFT: Fine)"), 
    Trimming_xpm,
    TrimB,
  },
  {
    NULL,
    N_("Evaluate"),
    N_("Evaluate Data"),
    N_("Evaluate Data Point"), 
    Eval_xpm,
    EvalB,
  },
  {
    NULL,
    N_("Zoom"),
    N_("Viewer Zoom"),
    N_("Viewer Zoom-In (+CONTROL: Zomm-Out +SHIFT: Centering)"), 
    Zoom_xpm,
    ZoomB,
  },
};

#define COMMAND1_NUM (sizeof(Command1_data) / sizeof(*Command1_data))
#define COMMAND2_NUM (sizeof(Command2_data) / sizeof(*Command2_data))

struct narray ChildList;
int signaltrap = FALSE;

GdkColor white, gray;

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
set_draw_lock(int lock)
{
  DrawLock = lock;
}

static void
kill_signal_handler(int sig)
{
  Hide_window = TRUE;
}

static void
childhandler(int sig)
{
  int i, num;
  pid_t *data;
  struct sigaction act;

  if (!signaltrap)
    return;

  act.sa_handler = SIG_IGN;
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);

  sigaction(sig, &act, NULL);
  data = arraydata(&ChildList);
  num = arraynum(&ChildList);
  for (i = num - 1; i >= 0; i--) {
    if (waitpid(-data[i], NULL, WNOHANG | WUNTRACED) > 0)
      arrayndel(&ChildList, i);
  }
  act.sa_handler = childhandler;
  sigaction(sig, &act, NULL);
}

void
AppMainLoop(void)
{
  Hide_window = FALSE;
  while (TRUE) {
    NgraphApp.Interrupt = FALSE;
    gtk_main_iteration();
    if (Hide_window && ! gtk_events_pending()) {
      Hide_window = FALSE;
      if(CheckSave()) {
	SaveHistory();
	break;
      }
    }
  }
}

void
ResetEvent(void)
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
find_gra2gdk_inst(char **name, struct objlist **o, char **i, struct objlist **ro, int *routput, struct gra2cairo_local **rlocal)
{
  static struct objlist *obj = NULL, *robj = NULL;
  static char *inst = NULL, *oname = "gra2gdk";
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
set_show_instance_menu_cb(GtkWidget *menu, char *name, struct show_instance_menu_data *data, int n)
{
  int i;
  struct objlist *obj;

  obj = chkobject(name);
  for (i = 0; i < n - 1; i++) {
    data[i].obj = obj;
  }
  data[i].obj = NULL;
  data[i].widget = NULL;
  g_signal_connect(menu, "show", G_CALLBACK(show_instance_menu_cb), data);
}

static void
show_graphmwnu_cb(GtkWidget *w, gpointer user_data)
{
  GtkWidget *label;
  char **data;
  int num, i;
  static GString *str = NULL;

  num = arraynum(Menulocal.ngpfilelist);
  data = (char **) arraydata(Menulocal.ngpfilelist);

  if (RecentGraph)
    gtk_widget_set_sensitive(RecentGraph, num > 0);

  if (AddinMenu)
    gtk_widget_set_sensitive(AddinMenu, Menulocal.scriptroot != NULL);

  if (str == NULL)
    str = g_string_new("");

  for (i = 0; i < MENU_HISTORY_NUM; i++) {
    if (i < num) {
      label = gtk_bin_get_child(GTK_BIN(NgraphApp.ghistory[i]));
      g_string_printf(str, "_%d: %s", i, data[i]);
      add_underscore(str);
      gtk_label_set_text_with_mnemonic(GTK_LABEL(label), str->str);
      gtk_widget_show(GTK_WIDGET(NgraphApp.ghistory[i]));
    } else {
      gtk_widget_hide(GTK_WIDGET(NgraphApp.ghistory[i]));
    }
  }
}

static void
create_recent_graph_menu(GtkWidget *parent, GtkAccelGroup *accel_group)
{
  int i;
  GtkWidget *menu;

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), menu);

  for (i = 0; i < MENU_HISTORY_NUM; i++) {
    NgraphApp.ghistory[i] = gtk_menu_item_new_with_label("");
    g_signal_connect(NgraphApp.ghistory[i], "activate", G_CALLBACK(CmGraphHistory),  (gpointer) i);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(NgraphApp.ghistory[i]));
  }
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
      g_signal_connect(item, "activate", G_CALLBACK(CmScriptExec), fcur);
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
create_graphmenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(_("_Graph"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  g_signal_connect(menu, "show", G_CALLBACK(show_graphmwnu_cb), NULL);

  item = gtk_menu_item_new_with_mnemonic(_("_New graph"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  create_graphnewmenu(item, accel_group);

  create_menu_item(menu, _("_Load graph"), FALSE, "<Ngraph>/Graph/Load graph", GDK_r, GDK_CONTROL_MASK, CmGraphMenu, MenuIdGraphLoad);

  item = gtk_menu_item_new_with_mnemonic(_("_Recent graphs"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  create_recent_graph_menu(item, accel_group);
  RecentGraph = item;

  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, GTK_STOCK_SAVE, TRUE, "<Ngraph>/Graph/Save",  GDK_s, GDK_CONTROL_MASK, CmGraphMenu, MenuIdGraphOverWrite);
  create_menu_item(menu, GTK_STOCK_SAVE_AS, TRUE, "<Ngraph>/Graph/SaveAs",  GDK_s, GDK_CONTROL_MASK | GDK_SHIFT_MASK, CmGraphMenu, MenuIdGraphSave);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, _("_Draw order"), FALSE, "<Ngraph>/Graph/Draw order", 0, 0, CmGraphMenu, MenuIdGraphSwitch);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, _("pa_Ge"), FALSE, "<Ngraph>/Graph/Page", 0, 0, CmGraphMenu, MenuIdGraphPage);
  create_menu_item(menu, GTK_STOCK_PRINT, TRUE, "<Ngraph>/Graph/Print",  GDK_p, GDK_CONTROL_MASK, CmGraphMenu, MenuIdOutputDriver);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, _("_Current directory"), FALSE, "<Ngraph>/Graph/Current directory", 0, 0, CmGraphMenu, MenuIdGraphDirectory);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);

  item = gtk_menu_item_new_with_mnemonic(_("_Add-In"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  create_addin_menu(item, accel_group);
  AddinMenu = item;

  create_menu_item(menu, _("_Ngraph shell"), FALSE, "<Ngraph>/Graph/Ngraph shell", 0, 0, CmGraphMenu, MenuIdGraphShell);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, GTK_STOCK_QUIT, TRUE, "<Ngraph>/Graph/Quit",  GDK_q, GDK_CONTROL_MASK, CmGraphMenu, MenuIdGraphQuit);
}

static void
show_filemwnu_cb(GtkWidget *w, gpointer user_data)
{
  GtkWidget *label;
  char **data;
  int num, i;
  static GString *str = NULL;

  num = arraynum(Menulocal.datafilelist);
  data = (char **) arraydata(Menulocal.datafilelist);

  if (RecentData)
    gtk_widget_set_sensitive(RecentData, num > 0);

  if (str == NULL)
    str = g_string_new("");

  for (i = 0; i < MENU_HISTORY_NUM; i++) {
    if (i < num) {
      label = gtk_bin_get_child(GTK_BIN(NgraphApp.fhistory[i]));
      g_string_printf(str, "_%d: %s", i, data[i]);
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
  static struct show_instance_menu_data data[4];

  item = gtk_menu_item_new_with_mnemonic(_("_Data"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  g_signal_connect(menu, "show", G_CALLBACK(show_filemwnu_cb), NULL);
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

  set_show_instance_menu_cb(menu, "file", data, sizeof(data) / sizeof(*data));
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

  set_show_instance_menu_cb(menu, "axisgrid", data, sizeof(data) / sizeof(*data));
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

  item = gtk_menu_item_new_with_mnemonic(_("_Grid"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  create_axisgridmenu(item, accel_group);

  set_show_instance_menu_cb(menu, "axis", data, sizeof(data) / sizeof(*data));
}

static GtkWidget *
create_legendsubmenu(GtkWidget *parent, char *label, legend_cb_func func, GtkAccelGroup *accel_group)
{
  GtkWidget *menu, *submenu;
  char buf[256];

  menu = gtk_menu_item_new_with_mnemonic(label);
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(menu));

  submenu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(submenu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(menu), submenu);

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
  static struct show_instance_menu_data data[8];

  item = gtk_menu_item_new_with_mnemonic(_("_Legend"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  data[0].widget = create_legendsubmenu(menu, _("_Line"), CmLineMenu, accel_group);
  data[0].obj = chkobject("line");

  data[1].widget = create_legendsubmenu(menu, _("_Curve"), CmCurveMenu, accel_group);
  data[1].obj = chkobject("curve");

  data[2].widget = create_legendsubmenu(menu, _("_Polygon"), CmPolygonMenu, accel_group);
  data[2].obj = chkobject("polygon");

  data[3].widget = create_legendsubmenu(menu, _("_Rectangle"), CmRectangleMenu, accel_group);
  data[3].obj = chkobject("rectangle");

  data[4].widget = create_legendsubmenu(menu, _("_Arc"), CmArcMenu, accel_group);
  data[4].obj = chkobject("arc");

  data[5].widget = create_legendsubmenu(menu, _("_Mark"), CmMarkMenu, accel_group);
  data[5].obj = chkobject("mark");

  data[6].widget = create_legendsubmenu(menu, _("_Text"), CmTextMenu, accel_group);
  data[6].obj = chkobject("text");

  data[7].obj = NULL;
  data[7].widget = NULL;

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

  set_show_instance_menu_cb(menu, "merge", data, sizeof(data) / sizeof(*data));
}

static void 
create_image_outputmenu(GtkWidget *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *menu;

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), menu);

  create_menu_item(menu, _("_GRA file"), FALSE, "<Ngraph>/Output/GRA File", 0, 0, CmOutputMenu, MenuIdOutputGRAFile);
  create_menu_item(menu, _("_PS file"),  FALSE, "<Ngraph>/Output/PS File",  0, 0, CmOutputMenu, MenuIdOutputPSFile);
  create_menu_item(menu, _("_EPS file"), FALSE, "<Ngraph>/Output/EPS File", 0, 0, CmOutputMenu, MenuIdOutputEPSFile);
  create_menu_item(menu, _("_PDF file"), FALSE, "<Ngraph>/Output/PDF File", 0, 0, CmOutputMenu, MenuIdOutputPDFFile);
  create_menu_item(menu, _("_SVG file"), FALSE, "<Ngraph>/Output/SVG File", 0, 0, CmOutputMenu, MenuIdOutputSVGFile);
  create_menu_item(menu, _("_PNG file"), FALSE, "<Ngraph>/Output/PNG File", 0, 0, CmOutputMenu, MenuIdOutputPNGFile);
}

static void 
create_outputmenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;
  static struct show_instance_menu_data data[2];

  item = gtk_menu_item_new_with_mnemonic(_("_Output"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  create_menu_item(menu, _("_Draw"), FALSE, "<Ngraph>/Output/Draw", GDK_d, GDK_CONTROL_MASK, CmOutputMenu, MenuIdViewerDraw);
  create_menu_item(menu, GTK_STOCK_CLEAR, TRUE, "<Ngraph>/Output/Clear", GDK_e, GDK_CONTROL_MASK, CmOutputMenu, MenuIdViewerClear);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);

  item = gtk_menu_item_new_with_mnemonic(_("_Export image"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  create_image_outputmenu(item, accel_group);

  create_menu_item(menu, _("external _Viewer"), FALSE, "<Ngraph>/Output/External Viewer", 0, 0, CmOutputMenu, MenuIdOutputViewer);
  create_menu_item(menu, _("external _Driver"), FALSE, "<Ngraph>/Output/External Driver", 0, 0, CmOutputMenu, MenuIdOutputDriver);
  data[0].widget = create_menu_item(menu, _("data _File"), FALSE, "<Ngraph>/Output/Data File", 0, 0, CmOutputMenu, MenuIdPrintDataFile);

  set_show_instance_menu_cb(menu, "file", data, sizeof(data) / sizeof(*data));
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
  create_menu_item(menu, _("External _Driver"), FALSE, "<Ngraph>/Preference/External Driver", 0, 0, CmOptionMenu, MenuIdOptionPrefDriver);
  create_menu_item(menu, _("_Addin script"), FALSE, "<Ngraph>/Preference/Addin Script", 0, 0, CmOptionMenu, MenuIdOptionScript);
  create_menu_item(menu, _("_Miscellaneous"), FALSE, "<Ngraph>/Preference/Miscellaneous", 0, 0, CmOptionMenu, MenuIdOptionMisc);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, _("save as default (_Settings)"), FALSE, "<Ngraph>/Preference/save as default (Settings)", 0, 0, CmOptionMenu, MenuIdOptionSaveDefault);
  create_menu_item(menu, _("save as default (_Graph)"), FALSE, "<Ngraph>/Preference/save as default (Graph)", 0, 0, CmOptionMenu, MenuIdOptionSaveNgp);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, _("_Data file default"), FALSE, "<Ngraph>/Preference/Data file default", 0, 0, CmOptionMenu, MenuIdOptionFileDef);
  create_menu_item(menu, _("_Legend text default"), FALSE, "<Ngraph>/Preference/Legend text default", 0, 0, CmOptionMenu, MenuIdOptionTextDef);
}

static void
show_winmwnu_cb(GtkWidget *w, gpointer data)
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

  gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ShowStatusBar), Menulocal.statusb);

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

static void
toggle_status_bar(GtkWidget *w, gpointer data)
{
  if (Toggle_cb_disable) return;

  if (Menulocal.statusb) {
    Menulocal.statusb = FALSE;
    gtk_widget_hide(NgraphApp.Message);
  } else {
    Menulocal.statusb = TRUE;
    gtk_widget_show(NgraphApp.Message);
  }
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
create_windowmenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(_("_Window"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  item = create_toggle_menu_item(menu, "_Data Window", "<Ngraph>/Window/Data Window", GDK_F3, 0, G_CALLBACK(toggle_win_cb), CmFileWindow);
  ShowFileWin = item;

  item = create_toggle_menu_item(menu, "_Axis Window", "<Ngraph>/Window/Axis Window", GDK_F4, 0, G_CALLBACK(toggle_win_cb), CmAxisWindow);
  ShowAxisWin = item;

  item = create_toggle_menu_item(menu, "_Legend Window", "<Ngraph>/Window/Legend Window", GDK_F5, 0, G_CALLBACK(toggle_win_cb), CmLegendWindow);
  ShowLegendWin = item;

  item = create_toggle_menu_item(menu, "_Merge Window", "<Ngraph>/Window/Merge Window", GDK_F6, 0, G_CALLBACK(toggle_win_cb), CmMergeWindow);
  ShowMergeWin = item;

  item = create_toggle_menu_item(menu, "_Coordinate Window", "<Ngraph>/Window/Coordinate Window", GDK_F7, 0, G_CALLBACK(toggle_win_cb), CmCoordinateWindow);
  ShowCoodinateWin = item;

  item = create_toggle_menu_item(menu, "_Information Window", "<Ngraph>/Window/Information Window", GDK_F8, 0, G_CALLBACK(toggle_win_cb), CmInformationWindow);
  ShowInfoWin = item;

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = gtk_menu_item_new_with_mnemonic(_("default _Window config"));
  g_signal_connect(item, "activate", G_CALLBACK(CmReloadWindowConfig), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = gtk_check_menu_item_new_with_mnemonic(_("_Status bar"));
  g_signal_connect(item, "toggled", G_CALLBACK(toggle_status_bar), NULL);
  ShowStatusBar = item;
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  g_signal_connect(menu, "show", G_CALLBACK(show_winmwnu_cb), NULL);

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
  char *inst, *home;

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
  int len;
 
  accel_group = gtk_accel_group_new();

  g_object_set_data(G_OBJECT(TopLevel->window), "accel_group", accel_group);

  create_graphmenu(parent, accel_group);
  create_filemenu(parent, accel_group);
  create_axismenu(parent, accel_group);
  create_legendmenu(parent, accel_group);
  create_mergemenu(parent, accel_group);
  create_outputmenu(parent, accel_group);
  create_preferencemenu(parent, accel_group);
  create_windowmenu(parent, accel_group);
  create_helpmenu(parent, accel_group);

  gtk_window_add_accel_group(GTK_WINDOW(TopLevel), accel_group);
  AccelGroup = accel_group;

  home = get_home();
  if (home == NULL)
    return;

  len = strlen(home) + strlen(KEYMAP_FILE) + 2;
  filename = malloc(len);

  if (filename == NULL)
    return;

  snprintf(filename, len, "%s/%s", home, KEYMAP_FILE);
  if (access(filename, R_OK) == 0) {
    gtk_accel_map_load(filename);
  }
  free(filename);
}

static void
createpixmap(GtkWidget *win, int n, struct command_data *data)
{
  GdkPixbuf *pixbuf;
  int i;

  for (i = 0; i < n; i++) {
    if (data[i].label == NULL)
      continue;

    pixbuf = gdk_pixbuf_new_from_xpm_data(data[i].xpm);
    data[i].img = gtk_image_new_from_pixbuf(pixbuf);
    g_object_unref(pixbuf);
  }
}

static void
create_toolbar_pixmap(GtkWidget *win)
{
  createpixmap(win, COMMAND1_NUM, Command1_data);
  createpixmap(win, COMMAND2_NUM, Command2_data);
}

#define MARK_PIX_SIZE 24
static void
create_markpixmap(GtkWidget *win)
{
  GdkPixmap *pix;
  int gra, i, R, G, B, R2, G2, B2, found, output;
  struct objlist *obj, *robj;
  char *inst, *name;
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
		       robj, inst, output, -1,
		       -1, -1, NULL, local);
	if (gra >= 0) {
	  GRAview(gra, 0, 0, MARK_PIX_SIZE, MARK_PIX_SIZE, 0);
	  GRAlinestyle(gra, 0, NULL, 1, 0, 0, 1000);
	  GRAmark(gra, i, MARK_PIX_SIZE / 2, MARK_PIX_SIZE / 2, MARK_PIX_SIZE - 4, R, G, B, R2, G2, B2);
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
createicon(GtkWidget *win)
{
  GList *list = NULL;
  GdkPixbuf *pixbuf;

  pixbuf = gdk_pixbuf_new_from_xpm_data(Icon_xpm);
  list = g_list_append(list, pixbuf);

  pixbuf = gdk_pixbuf_new_from_xpm_data(Icon_xpm_64);
  list = g_list_append(list, pixbuf);

  NgraphApp.iconpix = list;
}

static int
create_cursor(void)
{
  unsigned int i;

  NgraphApp.cursor = malloc(sizeof(GdkCursor *) * CURSOR_TYPE_NUM);
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
  free(NgraphApp.cursor);
}

static gboolean
tool_button_enter_leave_cb(GtkWidget *w, GdkEventCrossing *e, gpointer data)
{
  char *str;

  str = (char *) data;

  if (e->type == GDK_ENTER_NOTIFY) {
    SetStatusBarXm(str);
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

  for (i = 0; i < COMMAND1_NUM; i++) {
    if (Command1_data[i].label) {
      b = gtk_tool_button_new(Command1_data[i].img, _(Command1_data[i].label));

      if (Menulocal.showtip)
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(b), _(Command1_data[i].tip));

      g_signal_connect(gtk_bin_get_child(GTK_BIN(b)),
		       "enter-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb),
		       _(Command1_data[i].caption));

      g_signal_connect(gtk_bin_get_child(GTK_BIN(b)),
		       "leave-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb), NULL);

      g_signal_connect(b, "clicked", G_CALLBACK(Command1_data[i].func), NULL);
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

  j = 0;
  for (i = 0; i < COMMAND2_NUM; i++) {
    if (Command2_data[i].label) {
      b = gtk_radio_tool_button_new(list);

      if (Menulocal.showtip)
	gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(b), _(Command2_data[i].tip));

      g_signal_connect(gtk_bin_get_child(GTK_BIN(b)),
		       "enter-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb),
		       _(Command2_data[i].caption));
      g_signal_connect(gtk_bin_get_child(GTK_BIN(b)),
		       "leave-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb), NULL);
      g_signal_connect(gtk_bin_get_child(GTK_BIN(b)),
		       "button-press-event",
		       G_CALLBACK(CmViewerButtonPressed), NULL);

      list = gtk_radio_tool_button_get_group(GTK_RADIO_TOOL_BUTTON(b));

      gtk_tool_button_set_icon_widget(GTK_TOOL_BUTTON(b), Command2_data[i].img);
      gtk_tool_button_set_label(GTK_TOOL_BUTTON(b), _(Command2_data[i].label));

      g_signal_connect(b, "clicked", G_CALLBACK(CmViewerButtonArm), GINT_TO_POINTER(Command2_data[i].type));

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
create_toolbar(GtkWidget *box, GtkOrientation o)
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
  gtk_toolbar_set_orientation(GTK_TOOLBAR(t), o);
  w = gtk_handle_box_new();
  g_signal_connect(w, "child-attached", G_CALLBACK(detach_toolbar), GINT_TO_POINTER(TRUE));
  g_signal_connect(w, "child-detached", G_CALLBACK(detach_toolbar), GINT_TO_POINTER(FALSE));
  gtk_handle_box_set_handle_position(GTK_HANDLE_BOX(w), p);
  gtk_container_add(GTK_CONTAINER(w), t);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  return t;
}

void
set_widget_visibility(void)
{
  if (Mxlocal->ruler) {
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

  command1 = create_toolbar(vbox, GTK_ORIENTATION_HORIZONTAL);
  command2 = create_toolbar(hbox, GTK_ORIENTATION_VERTICAL);

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
  NgraphApp.Message1 = gtk_statusbar_get_context_id(GTK_STATUSBAR(NgraphApp.Message), "Message1");
  NgraphApp.Message2 = gtk_statusbar_get_context_id(GTK_STATUSBAR(NgraphApp.Message), "Message2");
  NgraphApp.Message3 = gtk_statusbar_get_context_id(GTK_STATUSBAR(NgraphApp.Message), "Message3");
  gtk_box_pack_start(GTK_BOX(vbox), NgraphApp.Message, FALSE, FALSE, 0);

  createcommand1(GTK_TOOLBAR(command1));
  createcommand2(GTK_TOOLBAR(command2));

  gtk_container_add(GTK_CONTAINER(TopLevel), vbox);
}

static void
defaultwindowconfig(void)
{
  int w, h;

  w = gdk_screen_get_width(gdk_screen_get_default());
  h = w / 2 * 1.2;

  if (Menulocal.fileopen) {
    if (Menulocal.filewidth == CW_USEDEFAULT)
      Menulocal.filewidth = w / 4;

    if (Menulocal.fileheight == CW_USEDEFAULT)
      Menulocal.fileheight = h / 4;

    if (Menulocal.filex == CW_USEDEFAULT)
      Menulocal.filex = -Menulocal.filewidth - 4;

    if (Menulocal.filey == CW_USEDEFAULT)
      Menulocal.filey = 0;
  }

  if (Menulocal.axisopen) {
    if (Menulocal.axiswidth == CW_USEDEFAULT)
      Menulocal.axiswidth = w / 4;

    if (Menulocal.axisheight == CW_USEDEFAULT)
      Menulocal.axisheight = h / 4;

    if (Menulocal.axisx == CW_USEDEFAULT)
      Menulocal.axisx = -Menulocal.axiswidth - 4;

    if (Menulocal.axisy == CW_USEDEFAULT)
      Menulocal.axisy = Menulocal.fileheight + 4;
  }

  if (Menulocal.coordopen) {
    if (Menulocal.coordwidth == CW_USEDEFAULT)
      Menulocal.coordwidth = w / 4;

    if (Menulocal.coordheight == CW_USEDEFAULT)
      Menulocal.coordheight = h / 4;

    if (Menulocal.coordx == CW_USEDEFAULT)
      Menulocal.coordx = -Menulocal.coordwidth - 4;

    if (Menulocal.coordy == CW_USEDEFAULT)
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
load_hist(void)
{
  char *home, *filename;
  int len;

  NgraphApp.legend_text_list = entry_completion_create();
  NgraphApp.x_math_list = entry_completion_create();
  NgraphApp.y_math_list = entry_completion_create();
  NgraphApp.func_list = entry_completion_create();

  home = get_home();
  if (home == NULL)
    return;

  len = strlen(home) + 64;
  filename = malloc(len);

  if (filename == NULL)
    return;

  snprintf(filename, len, "%s/%s", home, TEXT_HISTORY);
  entry_completion_load(NgraphApp.legend_text_list, filename, Menulocal.hist_size);

  snprintf(filename, len, "%s/%s", home, MATH_X_HISTORY);
  entry_completion_load(NgraphApp.x_math_list, filename, Menulocal.hist_size);

  snprintf(filename, len, "%s/%s", home, MATH_Y_HISTORY);
  entry_completion_load(NgraphApp.y_math_list, filename, Menulocal.hist_size);

  snprintf(filename, len, "%s/%s", home, FUNCTION_HISTORY);
  entry_completion_load(NgraphApp.func_list, filename, Menulocal.hist_size);

  free(filename);
}

static void
save_hist(void)
{
  struct objlist *sysobj;
  char *inst, *home, *filename;
  int len;

  sysobj = chkobject("system");
  inst = chkobjinst(sysobj, 0);
  _getobj(sysobj, "home_dir", inst, &home);

  if (home) {
    len = strlen(home) + 64;
    filename = malloc(len);

    if (filename) {
      snprintf(filename, len, "%s/%s", home, TEXT_HISTORY);
      entry_completion_save(NgraphApp.legend_text_list, filename, Menulocal.hist_size);

      snprintf(filename, len, "%s/%s", home, MATH_X_HISTORY);
      entry_completion_save(NgraphApp.x_math_list, filename, Menulocal.hist_size);

      snprintf(filename, len, "%s/%s", home, MATH_Y_HISTORY);
      entry_completion_save(NgraphApp.y_math_list, filename, Menulocal.hist_size);

      snprintf(filename, len, "%s/%s", home, FUNCTION_HISTORY);
      entry_completion_save(NgraphApp.func_list, filename, Menulocal.hist_size);

      free(filename);
    }
  }

  g_object_unref(NgraphApp.legend_text_list);
  g_object_unref(NgraphApp.x_math_list);
  g_object_unref(NgraphApp.y_math_list);
  g_object_unref(NgraphApp.func_list);

}

static gboolean 
change_window_state_cb(GtkWidget *widget, GdkEventWindowState *event, gpointer user_data)
{
  if (event->new_window_state & GDK_WINDOW_STATE_ICONIFIED) {
    sub_window_minimize(&NgraphApp.FileWin);
    sub_window_minimize(&NgraphApp.AxisWin);
    sub_window_minimize(&NgraphApp.LegendWin);
    sub_window_minimize(&NgraphApp.MergeWin);
    sub_window_minimize(&NgraphApp.InfoWin);
    sub_window_minimize(&NgraphApp.CoordWin);
  } else if (event->changed_mask & GDK_WINDOW_STATE_ICONIFIED) {
    sub_window_restore_state(&NgraphApp.FileWin);
    sub_window_restore_state(&NgraphApp.AxisWin);
    sub_window_restore_state(&NgraphApp.LegendWin);
    sub_window_restore_state(&NgraphApp.MergeWin);
    sub_window_restore_state(&NgraphApp.InfoWin);
    sub_window_restore_state(&NgraphApp.CoordWin);
  }
  return FALSE;
}

static void
init_ngraph_app_struct(void)
{
  NgraphApp.Viewer.Win = NULL;
  NgraphApp.Viewer.popup = NULL;

  NgraphApp.FileWin.Win = NULL;
  NgraphApp.FileWin.popup = NULL;
  NgraphApp.FileWin.popup_item = NULL;
  NgraphApp.FileWin.ev_key = NULL;
  NgraphApp.FileWin.ev_button = NULL;
  NgraphApp.FileWin.window_state = 0;
  NgraphApp.FileWin.can_focus = FALSE;

  NgraphApp.AxisWin.Win = NULL;
  NgraphApp.AxisWin.popup = NULL;
  NgraphApp.AxisWin.popup_item = NULL;
  NgraphApp.AxisWin.ev_key = NULL;
  NgraphApp.AxisWin.ev_button = NULL;
  NgraphApp.AxisWin.window_state = 0;
  NgraphApp.AxisWin.can_focus = TRUE;

  NgraphApp.LegendWin.Win = NULL;
  NgraphApp.LegendWin.popup = NULL;
  NgraphApp.LegendWin.popup_item = NULL;
  NgraphApp.LegendWin.ev_key = NULL;
  NgraphApp.LegendWin.ev_button = NULL;
  NgraphApp.LegendWin.window_state = 0;
  NgraphApp.LegendWin.can_focus = TRUE;

  NgraphApp.MergeWin.Win = NULL;
  NgraphApp.MergeWin.popup = NULL;
  NgraphApp.MergeWin.popup_item = NULL;
  NgraphApp.MergeWin.ev_key = NULL;
  NgraphApp.MergeWin.ev_button = NULL;
  NgraphApp.MergeWin.window_state = 0;
  NgraphApp.MergeWin.can_focus = TRUE;

  NgraphApp.InfoWin.Win = NULL;
  NgraphApp.InfoWin.popup = NULL;
  NgraphApp.InfoWin.popup_item = NULL;
  NgraphApp.InfoWin.str = NULL;
  NgraphApp.InfoWin.ev_key = NULL;
  NgraphApp.InfoWin.ev_button = NULL;
  NgraphApp.InfoWin.window_state = 0;
  NgraphApp.FileWin.can_focus = FALSE;

  NgraphApp.CoordWin.Win = NULL;
  NgraphApp.CoordWin.popup = NULL;
  NgraphApp.CoordWin.popup_item = NULL;
  NgraphApp.CoordWin.str = NULL;
  NgraphApp.CoordWin.ev_key = NULL;
  NgraphApp.CoordWin.ev_button = NULL;
  NgraphApp.CoordWin.window_state = 0;
  NgraphApp.FileWin.can_focus = FALSE;

  NgraphApp.legend_text_list = NULL;
  NgraphApp.x_math_list = NULL;
  NgraphApp.y_math_list = NULL;
  NgraphApp.func_list = NULL;

  NgraphApp.Interrupt = FALSE;
}

void
application(char *file)
{
  int i;
  struct objlist *aobj;
  int x, y, width, height, w, h;
  GdkScreen *screen;
  struct sigaction act;

  if (TopLevel)
    return;

  init_ngraph_app_struct();

  screen = gdk_screen_get_default();
  w = gdk_screen_get_width(screen);
  h = gdk_screen_get_height(screen);

  if (Menulocal.menux == CW_USEDEFAULT)
    Menulocal.menux = w * 3 / 8;

  if (Menulocal.menuy == CW_USEDEFAULT)
    Menulocal.menuy = h / 8;

  if (Menulocal.menuwidth == CW_USEDEFAULT)
    Menulocal.menuwidth = w / 2;

  if (Menulocal.menuheight == CW_USEDEFAULT)
    Menulocal.menuheight = h / 1.2;

  x = Menulocal.menux;
  y = Menulocal.menuy;
  width = Menulocal.menuwidth;
  height = Menulocal.menuheight;

  load_hist();

  TopLevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(TopLevel), AppName);

  gtk_window_set_default_size(GTK_WINDOW(TopLevel), width, height);
  gtk_window_move(GTK_WINDOW(TopLevel), x, y);

  gtk_widget_show_all(GTK_WIDGET(TopLevel));
  ResetEvent();

  //    g_signal_connect(TopLevel, "window-state-event", G_CALLBACK(change_window_state_cb), NULL);
  g_signal_connect(TopLevel, "delete-event", G_CALLBACK(CloseCallback), NULL);
  g_signal_connect(TopLevel, "destroy-event", G_CALLBACK(CloseCallback), NULL);

  set_gdk_color(&white, 255, 255, 255);
  set_gdk_color(&gray,  0xaa, 0xaa, 0xaa);

  createicon(TopLevel);
  gtk_window_set_default_icon_list(NgraphApp.iconpix);
  gtk_window_set_icon_list(GTK_WINDOW(TopLevel), NgraphApp.iconpix);

  create_toolbar_pixmap(TopLevel);

  initdialog();
  setupwindow();

  gtk_widget_show_all(GTK_WIDGET(TopLevel));
  set_widget_visibility();

  NgraphApp.FileName = NULL;

  ResetEvent();

  ViewerWinSetup();

  create_markpixmap(TopLevel);

  if (create_cursor())
    return;

  SetCaption(NULL);
  SetCursor(GDK_LEFT_PTR);

  putstderr = mgtkputstderr;
  printfstderr = mgtkprintfstderr;
  putstdout = mgtkputstdout;
  printfstdout = mgtkprintfstdout;
  ndisplaydialog = mgtkdisplaydialog;
  ndisplaystatus = mgtkdisplaystatus;
  ninterrupt = mgtkinterrupt;
  inputyn = mgtkinputyn;

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

  arrayinit(&ChildList, sizeof(pid_t));
  signaltrap = TRUE;

  act.sa_handler = childhandler;
  act.sa_flags = 0;
  sigemptyset(&act.sa_mask);
  sigaction(SIGCHLD, &act, NULL);

  act.sa_handler = kill_signal_handler;
  act.sa_flags = 0;
  sigaction(SIGINT, &act, NULL);

  ResetEvent();
  gtk_widget_show_all(GTK_WIDGET(TopLevel));
  set_widget_visibility();

  AppMainLoop();

  act.sa_handler = SIG_DFL;
  act.sa_flags = 0;
  sigaction(SIGINT, &act, NULL);

  save_hist();

  signaltrap = FALSE;
  arraydel(&ChildList);

  ViewerWinClose();

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

  memfree(NgraphApp.InfoWin.str);
  NgraphApp.InfoWin.str = NULL;

  memfree(NgraphApp.CoordWin.str);
  NgraphApp.CoordWin.str = NULL;

  memfree(NgraphApp.FileName);
  NgraphApp.FileName = NULL;

  gtk_widget_destroy(TopLevel);
  TopLevel = NULL;

  free_markpixmap();
  free_cursor();

  ResetEvent();
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

void
SetPoint(int x, int y)
{
  char buf[40];

  if (NgraphApp.Message) {
    snprintf(buf, sizeof(buf), "X:%.2f Y:%.2f", x / 100.0, y / 100.0);
    gtk_statusbar_pop(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message3);
    gtk_statusbar_push(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message3, buf);
  }

  g_object_set(NgraphApp.Viewer.HRuler, "position", N2GTK_RULER_METRIC(x), NULL);
  g_object_set(NgraphApp.Viewer.VRuler, "position", N2GTK_RULER_METRIC(y), NULL);
}

void
SetZoom(double zm)
{
  char buf[20];

  if (NgraphApp.Message) {
    snprintf(buf, sizeof(buf), "%.2f%%", zm * 100);
    gtk_statusbar_pop(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message2);
    gtk_statusbar_push(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message2, buf);
  }
}

void
ResetZoom(void)
{
  if (NgraphApp.Message) {
    gtk_statusbar_pop(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message2);
    gtk_statusbar_push(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message2, "");
  }
}

void
SetStatusBar(char *mes)
{
  if (NgraphApp.Message) {
    gtk_statusbar_pop(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message1);
    gtk_statusbar_push(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message1, mes);
  }
}

void
SetStatusBarXm(gchar *s)
{
  if (NgraphApp.Message) {
    gtk_statusbar_pop(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message1);
    gtk_statusbar_push(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message1, s);
  }
}

void
ResetStatusBar(void)
{
  if (NgraphApp.Message) {
    gtk_statusbar_pop(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message1);
    gtk_statusbar_push(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message1, "");
  }
}

void
SetCaption(char *file)
{
  char *fullpath;
  char *buf;
  int len;

  if (file == NULL) {
    fullpath = NULL;
    len = 0;
  } else {
    fullpath = getfullpath(file);
    len = strlen(fullpath);
  }

  len += 10;
  buf = memalloc(len);
  if (buf) {
    snprintf(buf, len, "Ngraph%s%s",
	     (fullpath) ? ":" : "",
	     (fullpath) ? fullpath : "");
    gtk_window_set_title(GTK_WINDOW(TopLevel), buf);
    memfree(buf);
  }
  memfree(fullpath);
}

unsigned int
GetCursor(void)
{
  return CursorType;
}

void
SetCursor(unsigned int type)
{
  struct Viewer *d;
  GdkWindow *win;

  if (NgraphApp.cursor == NULL || CursorType == type)
    return;

  d = &(NgraphApp.Viewer);
  win = d->win;

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
DisplayDialog(char *str)
{
  InfoWinDrawInfoText(str);
}

void
DisplayStatus(char *str)
{
  SetStatusBar(str);
}

int
PutStdout(char *s)
{
  gssize len, rlen, wlen;
  char *ustr;

  len = strlen(s);
  ustr = g_locale_to_utf8(s, len, &rlen, &wlen, NULL);
  DisplayDialog(ustr);
  g_free(ustr);
  return len + 1;
}

int
PutStderr(char *s)
{
  gssize len, rlen, wlen;
  char *ustr;

  len = strlen(s);
  ustr = g_locale_to_utf8(s, len, &rlen, &wlen, NULL);
  MessageBox(TopLevel, ustr, _("Error:"), MB_ERROR);
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
InputYN(char *mes)
{
  int ret;

  ret = MessageBox(TopLevel, mes, _("Question"), MB_YESNO);
  UpdateAll2();
  return (ret == IDYES) ? TRUE : FALSE;
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

  //  ResetEvent();

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


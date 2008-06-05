/* 
 * $Id: x11menu.c,v 1.4 2008/06/05 01:18:37 hito Exp $
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

#include "ngraph.h"
#include "object.h"
#include "ioutil.h"
#include "shell.h"
#include "jstring.h"
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

int Menulock = FALSE;
struct NgraphApp NgraphApp;
GtkWidget *TopLevel = NULL;

// GdkAtom NgraphClose;
// GdkAtom COMPOUND_TEXT;
// char *CloseMessage = "NgraphClose";
// int RegisterWorkProc;
// XtWorkProcId WorkProcId;

static int Hide_window = FALSE, Toggle_cb_disable = FALSE;
static GtkWidget *ShowFileWin = NULL, *ShowAxisWin = NULL,
  *ShowLegendWin = NULL, *ShowMergeWin = NULL,
  *ShowCoodinateWin = NULL, *ShowInfoWin = NULL, *ShowStatusBar = NULL;


static void CmReloadWindowConfig(GtkMenuItem *w, gpointer user_data);
static void do_interrupt(GtkWidget *w, gpointer data);

struct command_data {
  void (*func)(GtkWidget *, gpointer);
  gchar *label, *tip, *caption;
  char **xpm;
  gchar *xbm;
  int type;
  GtkWidget *img;
};

static struct command_data Command1_data[] = {
  {
    CmFileWindow,
    N_("Data"),
    "Data Window",
    N_("Activate Data Window"), 
    Filewin_xpm,
    Filewin_bits
  },
  {
    CmAxisWindow,
    N_("Axis"),
    "Axis Window",
    N_("Activate Axis Window"), 
    Axiswin_xpm,
    Axiswin_bits},
  {
    CmLegendWindow,
    N_("Legend"),
    "Legend Window",
    N_("Activate Legend Window"), 
    Legendwin_xpm,
    Legendwin_bits},
  {
    CmMergeWindow,
    N_("Merge"),
    "Merge Window",
    N_("Activate Merge Window"), 
    Mergewin_xpm,
    Mergewin_bits
  },
  {
    CmCoordinateWindow,
    N_("Coordinate"),
    "Coordinate Window",
    N_("Activate Coordinate Window"), 
    Coordwin_xpm,
    Coordwin_bits
  },
  {
    CmInformationWindow,
    N_("Information"),
    "Information Window",
    N_("Activate Information Window"), 
    Infowin_xpm,
    Infowin_bits
  },
  {NULL},
  {
    CmFileOpenB,
    N_("Open"),
    N_("Open Data"),
    N_("Open Data file"), 
    Fileopen_xpm,
    Fileopen_bits
  },
  {
    CmGraphLoadB,
    N_("Load"),
    N_("Load NGP"),
    N_("Load NGP file"), 
    Load_xpm,
    Load_bits
  },
  {
    CmGraphSaveB,
    N_("Save"),
    N_("Save NGP"),
    N_("Save NGP file "), 
    Save_xpm,
    Save_bits
  },
  {
    CmAxisClear,
    N_("Clear"),
    N_("Clear Scale"),
    N_("Clear Scale"), 
    Scale_xpm,
    Scale_bits
  },
  {
    CmViewerDrawB,
    N_("Draw"),
    N_("Draw"),
    N_("Draw on Viewer Window"), 
    Draw_xpm,
    Draw_bits
  },
  {
    CmViewerClearB,
    N_("Clear"),
    N_("Clear"),
    N_("Clear Viewer Window"), 
    Clear_xpm,
    Clear_bits
  },
  {
    CmOutputDriverB,
    N_("Print"),
    N_("Print"),
    N_("Print"), 
    Print_xpm,
    Print_bits
  },
  {
    CmOutputViewerB,
    N_("Viewer"),
    N_("Ex. Viewer"),
    N_("Open External Viewer"), 
    Preview_xpm,
    Preview_bits
  },
  {NULL},
  {
    CmFileWinMath,
    N_("Math"),
    N_("Math Transformation"),
    N_("Set Math Transformation"), 
    Math_xpm,
    Math_bits
  },
  {
    CmAxisWinScaleUndo,
    N_("Undo"),
    N_("Scale Undo"),
    N_("Undo Scale Settings"), 
    Scaleundo_xpm,
    Scaleundo_bits
  },
  {
    do_interrupt,
    N_("Interrupt"),
    N_("Interrupt"),
    N_("Interrupt Drawing, etc."), 
    Interrupt_xpm,
    Interrupt_bits
  },
}; 

static struct command_data Command2_data[] = {
  {
    NULL,
    N_("Point"),
    N_("Pointer"),
    N_("Pointer (+CONTROL: Horizontal/Vertical +SHIFT: Fine)"), 
    Point_xpm,
    Point_bits,
    PointB,
  },
  {
    NULL,
    N_("Legend"),
    N_("Legend Pointer"),
    N_("Legend Pointer (+CONTROL: Horizontal/Vertical +SHIFT: Fine)"), 
    Legendpoint_xpm,
    Legendpoint_bits,
    LegendB,
  },
  {
    NULL,
    N_("Axis"),
    N_("Axis Pointer"),
    N_("Axis Pointer (+CONTROL: Horizontal/Vertical +SHIFT: Fine)"), 
    Axispoint_xpm,
    Axispoint_bits,
    AxisB,
  },
  {
    NULL,
    N_("Data"),
    N_("Data Pointer"),
    N_("Data Pointer"), 
    Datapoint_xpm,
    Datapoint_bits,
    DataB,
  },
  {NULL},
  {
    NULL,
    N_("Line"),
    N_("Line"),
    N_("New Legend Line (+SHIFT: Fine)"), 
    Line_xpm,
    Line_bits,
    LineB,
  },
  {
    NULL,
    N_("Curve"),
    N_("Curve"),
    N_("New Legend Curve (+SHIFT: Fine)"), 
    Curve_xpm,
    Curve_bits,
    CurveB,

  },
  {
    NULL,
    N_("Polygon"),
    N_("Polygon"),
    N_("New Legend Polygon (+SHIFT: Fine)"), 
    Polygon_xpm,
    Polygon_bits,
    PolyB,
  },
  {
    NULL,
    N_("Rectangle"),
    N_("Rectangle"),
    N_("New Legend Rectangle (+SHIFT: Fine)"), 
    Rect_xpm,
    Rect_bits,
    RectB,
  },
  {
    NULL,
    N_("Arc"),
    N_("Arc"),
    N_("New Legend Arc (+SHIFT: Fine)"), 
    Arc_xpm,
    Arc_bits,
    ArcB,
  },
  {
    NULL,
    N_("Mark"),
    N_("Mark"),
    N_("New Legend Mark (+SHIFT: Fine)"), 
    Mark_xpm,
    Mark_bits,
    MarkB,
  },
  {
    NULL,
    N_("Text"),
    N_("Text"),
    N_("New Legend Text (+SHIFT: Fine)"), 
    Text_xpm,
    Text_bits,
    TextB,
  },
  {
    NULL,
    N_("Gauss"),
    N_("Gaussian"),
    N_("New Legend Gaussian (+SHIFT: Fine)"), 
    Gauss_xpm,
    Gauss_bits,
    GaussB,
  },
  {
    NULL,
    N_("Frame axis"),
    N_("Frame Graph"),
    N_("New Frame Graph (+SHIFT: Fine)"), 
    Frame_xpm,
    Frame_bits,
    FrameB,
  },
  {
    NULL,
    N_("Section axis"),
    N_("Section Graph"),
    N_("New Section Graph (+SHIFT: Fine)"), 
    Section_xpm,
    Section_bits,
    SectionB,
  },
  {
    NULL,
    N_("Cross axis"),
    N_("Cross Graph"),
    N_("New Cross Graph (+SHIFT: Fine)"), 
    Cross_xpm,
    Cross_bits,
    CrossB,
  },
  {
    NULL,
    N_("Single axis"),
    N_("Single Axis"),
    N_("New Single Axis (+SHIFT: Fine)"), 
    Single_xpm,
    Single_bits,
    SingleB,
  },
  {NULL},
  {
    NULL,
    N_("Trimming"),
    N_("Axis Trimming"),
    N_("Axis Trimming (+SHIFT: Fine)"), 
    Trimming_xpm,
    Trimming_bits,
    TrimB,
  },
  {
    NULL,
    N_("Evaluate"),
    N_("Evaluate Data"),
    N_("Evaluate Data Point"), 
    Eval_xpm,
    Eval_bits,
    EvalB,
  },
  {
    NULL,
    N_("Zoom"),
    N_("Viewer Zoom"),
    N_("Viewer Zoom-In (+CONTROL: Zomm-Out +SHIFT: Centering)"), 
    Zoom_xpm,
    Zoom_bits,
    ZoomB,
  },
};

#define COMMAND1_NUM (sizeof(Command1_data) / sizeof(*Command1_data))
#define COMMAND2_NUM (sizeof(Command2_data) / sizeof(*Command2_data))

struct narray ChildList;
int signaltrap = FALSE;

GdkColor black, white, gray, red, blue;

void
childhandler(int sig)
{
  int i, num;
  pid_t *data;

  if (!signaltrap)
    return;
  signal(sig, SIG_IGN);
  data = arraydata(&ChildList);
  num = arraynum(&ChildList);
  for (i = num - 1; i >= 0; i--) {
    if (waitpid(-data[i], NULL, WNOHANG | WUNTRACED) > 0)
      arrayndel(&ChildList, i);
  }
  signal(sig, childhandler);
}

void
AppMainLoop(void)
{
  Hide_window = FALSE;
  while (TRUE) {
    NgraphApp.Interrupt = FALSE;
    gtk_main_iteration();
    if (Hide_window && ! gtk_events_pending()) {
      break;
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
    gtk_widget_hide_all(TopLevel);
    Hide_window = TRUE;
  }
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
show_graphmwnu_cb(GtkWidget *w, gpointer user_data)
{
  GtkWidget *label;
  char **data;
  int num, i;

  num = arraynum(Menulocal.ngpfilelist);
  data = (char **) arraydata(Menulocal.ngpfilelist);
  for (i = 0; i < MENU_HISTORY_NUM; i++) {
    if (i < num) {
      label = gtk_bin_get_child(GTK_BIN(NgraphApp.ghistory[i]));
      gtk_label_set_text(GTK_LABEL(label), data[i]);
      gtk_widget_show(GTK_WIDGET(NgraphApp.ghistory[i]));
    } else {
      gtk_widget_hide(GTK_WIDGET(NgraphApp.ghistory[i]));
    }
  }

}

static void 
create_graphmenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  int i = 1;
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
  create_menu_item(menu, GTK_STOCK_SAVE_AS, TRUE, "<Ngraph>/Graph/SaveAs",  GDK_s, GDK_CONTROL_MASK | GDK_SHIFT_MASK, CmGraphMenu, MenuIdGraphSave);
  create_menu_item(menu, GTK_STOCK_SAVE, TRUE, "<Ngraph>/Graph/Save",  GDK_s, GDK_CONTROL_MASK, CmGraphMenu, MenuIdGraphOverWrite);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, _("pa_Ge"), FALSE, "<Ngraph>/Graph/Page", 0, 0, CmGraphMenu, MenuIdGraphPage);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, _("_Draw order"), FALSE, "<Ngraph>/Graph/Draw order", 0, 0, CmGraphMenu, MenuIdGraphSwitch);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, GTK_STOCK_PRINT, TRUE, "<Ngraph>/Graph/Print",  GDK_p, GDK_CONTROL_MASK, CmGraphMenu, MenuIdOutputDriver);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, _("_Current directory"), FALSE, "<Ngraph>/Graph/Current directory", 0, 0, CmGraphMenu, MenuIdGraphDirectory);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, _("_Add-In"), FALSE, "<Ngraph>/Graph/Add-In", 0, 0, CmGraphMenu, MenuIdScriptExec);
  create_menu_item(menu, _("_Ngraph shell"), FALSE, "<Ngraph>/Graph/Ngraph shell", 0, 0, CmGraphMenu, MenuIdGraphShell);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, GTK_STOCK_QUIT, TRUE, "<Ngraph>/Graph/Quit",  GDK_q, GDK_CONTROL_MASK, CmGraphMenu, MenuIdGraphQuit);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);

  for (i = 0; i < MENU_HISTORY_NUM; i++) {
    NgraphApp.ghistory[i] = gtk_menu_item_new_with_label("");
    g_signal_connect(NgraphApp.ghistory[i], "activate", G_CALLBACK(CmGraphHistory),  (gpointer) i);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(NgraphApp.ghistory[i]));
  }

}

static void
show_filemwnu_cb(GtkWidget *w, gpointer user_data)
{
  GtkWidget *label;
  char **data;
  int num, i;


  num = arraynum(Menulocal.datafilelist);
  data = (char **) arraydata(Menulocal.datafilelist);
  for (i = 0; i < MENU_HISTORY_NUM; i++) {
    if (i < num) {
      label = gtk_bin_get_child(GTK_BIN(NgraphApp.fhistory[i]));
      gtk_label_set_text(GTK_LABEL(label), data[i]);
      gtk_widget_show(GTK_WIDGET(NgraphApp.fhistory[i]));
    } else {
      gtk_widget_hide(GTK_WIDGET(NgraphApp.fhistory[i]));
    }
  }

}

static void 
create_filemenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  int i = 0;
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(_("_Data"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  g_signal_connect(menu, "show", G_CALLBACK(show_filemwnu_cb), NULL);
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  create_menu_item(menu, GTK_STOCK_NEW, TRUE, "<Ngraph>/Data/New", 0, 0, CmFileMenu, MenuIdFileNew);
  create_menu_item(menu, GTK_STOCK_OPEN, TRUE, "<Ngraph>/Data/Open", GDK_o, GDK_CONTROL_MASK, CmFileMenu, MenuIdFileOpen);
  create_menu_item(menu, GTK_STOCK_PROPERTIES, TRUE, "<Ngraph>/Data/Property", 0, 0, CmFileMenu, MenuIdFileUpdate);
  create_menu_item(menu, GTK_STOCK_CLOSE, TRUE, "<Ngraph>/Data/Close", 0, 0, CmFileMenu, MenuIdFileClose);
  create_menu_item(menu, GTK_STOCK_EDIT, TRUE, "<Ngraph>/Data/Close", 0, 0, CmFileMenu, MenuIdFileEdit);

  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);

  for (i = 0; i < MENU_HISTORY_NUM; i++) {
    NgraphApp.fhistory[i] = gtk_menu_item_new_with_label("");
    g_signal_connect(NgraphApp.fhistory[i], "activate", G_CALLBACK(CmFileHistory), NULL);
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(NgraphApp.fhistory[i]));
  }
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

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), menu);

  create_menu_item(menu, GTK_STOCK_NEW, TRUE, "<Ngraph>/Axis/Grid/New", 0, 0, CmGridMenu, MenuIdAxisGridNew);
  create_menu_item(menu, GTK_STOCK_PROPERTIES, TRUE, "<Ngraph>/Axis/Grid/Property", 0, 0, CmGridMenu, MenuIdAxisGridUpdate);
  create_menu_item(menu, GTK_STOCK_DELETE, TRUE, "<Ngraph>/Axis/Grid/Delete", 0, 0, CmGridMenu, MenuIdAxisGridDel);
}

static void 
create_axismenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(_("_Axis"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  item = gtk_image_menu_item_new_from_stock(GTK_STOCK_ADD, NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  create_axisaddmenu(item, accel_group);

  create_menu_item(menu, GTK_STOCK_PROPERTIES, TRUE, "<Ngraph>/Axis/Property", 0, 0, CmAxisMenu, MenuIdAxisUpdate);
  create_menu_item(menu, GTK_STOCK_DELETE, TRUE, "<Ngraph>/Axis/Delete", 0, 0, CmAxisMenu, MenuIdAxisDel);
  create_menu_item(menu, _("Scale _Zoom"), FALSE, "<Ngraph>/Axis/Scale Zoom", 0, 0, CmAxisMenu, MenuIdAxisZoom);
  create_menu_item(menu, _("Scale _Clear"), FALSE, "<Ngraph>/Axis/Scale Clear", 0, 0, CmAxisMenu, MenuIdAxisClear);
  item = gtk_menu_item_new_with_mnemonic(_("_Grid"));
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  create_axisgridmenu(item, accel_group);
}

static void 
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
}


static void 
create_legendmenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(_("_Legend"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  create_legendsubmenu(menu, _("_Line"), CmLineMenu, accel_group);
  create_legendsubmenu(menu, _("_Curve"), CmCurveMenu, accel_group);
  create_legendsubmenu(menu, _("_Polygon"), CmPolygonMenu, accel_group);
  create_legendsubmenu(menu, _("_Rectangle"), CmRectangleMenu, accel_group);
  create_legendsubmenu(menu, _("_Arc"), CmArcMenu, accel_group);
  create_legendsubmenu(menu, _("_Mark"), CmMarkMenu, accel_group);
  create_legendsubmenu(menu, _("_Text"), CmTextMenu, accel_group);
}

static void 
create_mergemenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(_("_Merge"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  create_menu_item(menu, GTK_STOCK_OPEN, TRUE, "<Ngraph>/Merge/Open", 0, 0, CmMergeMenu, MenuIdMergeOpen);
  create_menu_item(menu, GTK_STOCK_PROPERTIES, TRUE, "<Ngraph>/Merge/Property", 0, 0, CmMergeMenu, MenuIdMergeUpdate);
  create_menu_item(menu, GTK_STOCK_CLOSE, TRUE, "<Ngraph>/Merge/Close", 0, 0, CmMergeMenu, MenuIdMergeClose);
}

static void 
create_outputmenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(_("_Output"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  create_menu_item(menu, _("_Draw"), FALSE, "<Ngraph>/Output/Draw", GDK_d, GDK_CONTROL_MASK, CmOutputMenu, MenuIdViewerDraw);
  create_menu_item(menu, GTK_STOCK_CLEAR, TRUE, "<Ngraph>/Output/Clear", GDK_e, GDK_CONTROL_MASK, CmOutputMenu, MenuIdViewerClear);
  create_menu_item(menu, NULL, FALSE, NULL, 0, 0, NULL, 0);
  create_menu_item(menu, _("external _Viewer"), FALSE, "<Ngraph>/Output/External Viewer", 0, 0, CmOutputMenu, MenuIdOutputViewer);
  create_menu_item(menu, _("external _Driver"), FALSE, "<Ngraph>/Output/External Driver", 0, 0, CmOutputMenu, MenuIdOutputDriver);
  create_menu_item(menu, _("_GRA file"), FALSE, "<Ngraph>/Output/GRA File", 0, 0, CmOutputMenu, MenuIdPrintGRAFile);
  create_menu_item(menu, _("data _File"), FALSE, "<Ngraph>/Output/Data File", 0, 0, CmOutputMenu, MenuIdPrintDataFile);
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

static void 
create_windowmenu(GtkMenuBar *parent, GtkAccelGroup *accel_group)
{
  GtkWidget *item, *menu;

  item = gtk_menu_item_new_with_mnemonic(_("_Window"));
  gtk_menu_shell_append(GTK_MENU_SHELL(parent), GTK_WIDGET(item));

  menu = gtk_menu_new();
  gtk_menu_set_accel_group (GTK_MENU(menu), accel_group);
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), menu);

  item = gtk_check_menu_item_new_with_mnemonic("_Data Window");
  g_signal_connect(item, "toggled", G_CALLBACK(toggle_win_cb), CmFileWindow);
  ShowFileWin = item;
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = gtk_check_menu_item_new_with_mnemonic("_Axis Window");
  g_signal_connect(item, "toggled", G_CALLBACK(toggle_win_cb), CmAxisWindow);
  ShowAxisWin = item;
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = gtk_check_menu_item_new_with_mnemonic("_Legend Window");
  g_signal_connect(item, "toggled", G_CALLBACK(toggle_win_cb), CmLegendWindow);
  ShowLegendWin = item;
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = gtk_check_menu_item_new_with_mnemonic("_Merge Window");
  g_signal_connect(item, "toggled", G_CALLBACK(toggle_win_cb), CmMergeWindow);
  ShowMergeWin = item;
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = gtk_check_menu_item_new_with_mnemonic("_Coordinate Window");
  g_signal_connect(item, "toggled", G_CALLBACK(toggle_win_cb), CmCoordinateWindow);
  ShowCoodinateWin = item;
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = gtk_check_menu_item_new_with_mnemonic("_Information Window");
  g_signal_connect(item, "toggled", G_CALLBACK(toggle_win_cb), CmInformationWindow);
  ShowInfoWin = item;
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = gtk_separator_menu_item_new();
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  item = gtk_menu_item_new_with_mnemonic(_("default _Window config"));
  g_signal_connect(item, "activate", G_CALLBACK(CmReloadWindowConfig), NULL);
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));

  /*
  item = gtk_check_menu_item_new_with_mnemonic("_Move child window");
  gtk_menu_shell_append(GTK_MENU_SHELL(menu), GTK_WIDGET(item));
  */

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

static void
createmenu(GtkMenuBar *parent)
{
  GtkAccelGroup *accel_group;
 
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

  gtk_window_add_accel_group (GTK_WINDOW(TopLevel), accel_group);
}

static void
createpixmap(GtkWidget *win, int n, struct command_data *data)
{
  GdkPixmap *pixmap;
  GdkBitmap *bitmap;
  int i;

  for (i = 0; i < n; i++) {
    if (data[i].label == NULL)
      continue;

    bitmap = gdk_bitmap_create_from_data(win->window,
					 data[i].xbm,
					 20, 20);
    pixmap = gdk_pixmap_create_from_xpm_d(win->window, &bitmap, NULL,
					  data[i].xpm);
    data[i].img = gtk_image_new_from_pixmap(pixmap, bitmap);
    g_object_unref(bitmap);
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
  int dpi, gra;
  GdkGC *gc;
  struct mxlocal mxsave;
  int i, R, G, B, R2, G2, B2;
  GdkColor white;

  R = G = B = 0;
  R2 = 0;
  G2 = B2 = 255;
  dpi = MARK_PIX_SIZE * 254;

  white.red = 0xffff;
  white.green = 0xffff;
  white.blue = 0xffff;

  gc = gdk_gc_new(win->window);

  for (i = 0; i < MARK_TYPE_NUM; i++) {
    pix = gdk_pixmap_new(win->window, MARK_PIX_SIZE, MARK_PIX_SIZE, -1);
    gdk_gc_set_rgb_fg_color(gc, &white);
    gdk_draw_rectangle(pix, gc, TRUE, 0, 0, MARK_PIX_SIZE, MARK_PIX_SIZE);
    mxsaveGC(gc, pix, NULL, 0, 0, &mxsave, dpi, NULL);
    gra = _GRAopen(chkobjectname(Menulocal.obj), "_output",
		   Menulocal.outputobj, Menulocal.inst, Menulocal.output, -1,
		   -1, -1, NULL, Mxlocal);
    if (gra >= 0) {
      GRAview(gra, 0, 0, MARK_PIX_SIZE, MARK_PIX_SIZE, 0);
      GRAlinestyle(gra, 0, NULL, 0, 0, 0, 1000);
      GRAmark(gra, i, 5, 5, 8, R, G, B, R2, G2, B2);
    }
    _GRAclose(gra);
    mxrestoreGC(&mxsave);
    NgraphApp.markpix[i] = pix;
  }
  g_object_unref(G_OBJECT(gc));
}

static void
free_markpixmap(void)
{
  int i;

  for (i = 0; i < MARK_TYPE_NUM; i++) {
    g_object_unref(NgraphApp.markpix[i]);
    NgraphApp.markpix[i] = NULL;
  }
}

GdkPixbuf *
create_pixbuf_from_xpm(GtkWidget *win, char **xpm)
{
  GdkPixmap *pixmap;
  GdkPixbuf *pixbuf;

  pixmap = gdk_pixmap_create_from_xpm_d(win->window, NULL, NULL, xpm);
  pixbuf = gdk_pixbuf_get_from_drawable(NULL, pixmap, NULL, 0, 0, 0, 0, -1, -1);
  g_object_unref(G_OBJECT(pixmap));

  return pixbuf;
}

static void
createicon(GtkWidget *win)
{
  GList *list = NULL;
  GdkPixbuf *pixbuf;

  pixbuf = create_pixbuf_from_xpm(win, Icon_xpm);
  list = g_list_append(list, pixbuf);

  pixbuf = create_pixbuf_from_xpm(win, Icon_xpm_64);
  list = g_list_append(list, pixbuf);

  NgraphApp.iconpix = list;
}

static void
create_cursor(void)
{
  GdkCursorType cursor[] = {
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
  };
  int i;

  for (i = 0; i < CURSOR_TYPE_NUM; i++) {
    NgraphApp.cursor[i] = gdk_cursor_new_for_display(Disp, cursor[i]);
  }
}

static void
free_cursor(void)
{
  int i;

  for (i = 0; i < CURSOR_TYPE_NUM; i++) {
    gdk_cursor_unref(NgraphApp.cursor[i]);
    NgraphApp.cursor[i] = NULL;
  }
}
static void
do_interrupt(GtkWidget *w, gpointer data)
{
  NgraphApp.Interrupt = TRUE;
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
  int i;

  for (i = 0; i < COMMAND1_NUM; i++) {
    if (Command1_data[i].label) {
      b = gtk_tool_button_new(Command1_data[i].img, _(Command1_data[i].label));
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
  NgraphApp.interrupt = b;
}

static void
createcommand2(GtkToolbar *parent)
{
  GtkToolItem *b;
  GSList *list = NULL;
  int i, j;

  j = 0;
  for (i = 0; i < COMMAND2_NUM; i++) {
    if (Command2_data[i].label) {
      b = gtk_radio_tool_button_new(list);
      gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(b), _(Command2_data[i].tip));

      g_signal_connect(gtk_bin_get_child(GTK_BIN(b)),
		       "enter-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb),
		       _(Command2_data[i].caption));
      g_signal_connect(gtk_bin_get_child(GTK_BIN(b)),
		       "leave-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb), NULL);

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
setupwindow(void)
{
  GtkWidget *menubar, *command1, *command2, *hbox, *vbox, *table;

  vbox = gtk_vbox_new(FALSE, 0);
  hbox = gtk_hbox_new(FALSE, 0);

  menubar = gtk_menu_bar_new();
  gtk_box_pack_start(GTK_BOX(vbox), menubar, FALSE, FALSE, 0);
  createmenu(GTK_MENU_BAR(menubar));

  command1 = gtk_toolbar_new();
  gtk_toolbar_set_style(GTK_TOOLBAR(command1), GTK_TOOLBAR_ICONS);
  gtk_box_pack_start(GTK_BOX(vbox), command1, FALSE, FALSE, 0);

  command2 = gtk_toolbar_new();
  gtk_toolbar_set_style(GTK_TOOLBAR(command2), GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_orientation(GTK_TOOLBAR(command2), GTK_ORIENTATION_VERTICAL);
  gtk_box_pack_start(GTK_BOX(hbox), command2, FALSE, FALSE, 0);


  NgraphApp.Viewer.HScroll = gtk_hscrollbar_new(NULL);
  NgraphApp.Viewer.VScroll = gtk_vscrollbar_new(NULL);
  NgraphApp.Viewer.Win = gtk_drawing_area_new();

  table = gtk_table_new(2, 2, FALSE);

  gtk_table_attach(GTK_TABLE(table),
		   NgraphApp.Viewer.HScroll,
		   0, 1, 1, 2,
		   GTK_FILL, GTK_FILL,
		   0, 0);

  gtk_table_attach(GTK_TABLE(table),
		   NgraphApp.Viewer.VScroll,
		   1, 2, 0, 1,
		   GTK_FILL, GTK_FILL,
		   0, 0);

  gtk_table_attach(GTK_TABLE(table),
		   NgraphApp.Viewer.Win,
		   0, 1, 0, 1,
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
      Menulocal.filex = -Menulocal.filewidth - Menulocal.framex * 2 - 4;

    if (Menulocal.filey == CW_USEDEFAULT)
      Menulocal.filey = 0;
  }
  if (Menulocal.axisopen) {
    if (Menulocal.axiswidth == CW_USEDEFAULT)
      Menulocal.axiswidth = w / 4;

    if (Menulocal.axisheight == CW_USEDEFAULT)
      Menulocal.axisheight = h / 4;

    if (Menulocal.axisx == CW_USEDEFAULT)
      Menulocal.axisx = -Menulocal.axiswidth - Menulocal.framex * 2 - 4;

    if (Menulocal.axisy == CW_USEDEFAULT)
      Menulocal.axisy = Menulocal.fileheight + Menulocal.framey + 4;
  }
  if (Menulocal.coordopen) {

    if (Menulocal.coordwidth == CW_USEDEFAULT)
      Menulocal.coordwidth = w / 4;

    if (Menulocal.coordheight == CW_USEDEFAULT)
      Menulocal.coordheight = h / 4;

    if (Menulocal.coordx == CW_USEDEFAULT)
      Menulocal.coordx = -Menulocal.coordwidth - Menulocal.framex * 2 - 4;

    if (Menulocal.coordy == CW_USEDEFAULT)
      Menulocal.coordy = Menulocal.fileheight + Menulocal.axisheight
	+ Menulocal.framey * 2 + 8;
  }
}

static void
set_gdk_color(GdkColor *col, int r, int g, int b)
{
  col->red = r * 255;
  col->green = g * 255;
  col->blue = b * 255;
}

static void
load_hist(void)
{
  struct objlist *sysobj;
  char *inst, *home, *filename;
  int len;

  sysobj = chkobject("system");
  inst = chkobjinst(sysobj, 0);
  _getobj(sysobj, "home_dir", inst, &home);

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

  if (home == NULL)
    return;

  len = strlen(home) + 64;
  filename = malloc(len);

  if (filename == NULL)
    return;

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

static gint
escape_drawing_cb(GtkWidget *w, GdkEventKey *e, gpointer data)
{
  if (e->keyval == GDK_Escape)
    do_interrupt(w, data);

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

  NgraphApp.AxisWin.Win = NULL;
  NgraphApp.AxisWin.popup = NULL;
  NgraphApp.AxisWin.popup_item = NULL;
  NgraphApp.AxisWin.ev_key = NULL;
  NgraphApp.AxisWin.ev_button = NULL;
  NgraphApp.AxisWin.window_state = 0;

  NgraphApp.LegendWin.Win = NULL;
  NgraphApp.LegendWin.popup = NULL;
  NgraphApp.LegendWin.popup_item = NULL;
  NgraphApp.LegendWin.ev_key = NULL;
  NgraphApp.LegendWin.ev_button = NULL;
  NgraphApp.LegendWin.window_state = 0;

  NgraphApp.MergeWin.Win = NULL;
  NgraphApp.MergeWin.popup = NULL;
  NgraphApp.MergeWin.popup_item = NULL;
  NgraphApp.MergeWin.ev_key = NULL;
  NgraphApp.MergeWin.ev_button = NULL;
  NgraphApp.MergeWin.window_state = 0;

  NgraphApp.InfoWin.Win = NULL;
  NgraphApp.InfoWin.popup = NULL;
  NgraphApp.InfoWin.popup_item = NULL;
  NgraphApp.InfoWin.str = NULL;
  NgraphApp.InfoWin.ev_key = NULL;
  NgraphApp.InfoWin.ev_button = NULL;
  NgraphApp.InfoWin.window_state = 0;

  NgraphApp.CoordWin.Win = NULL;
  NgraphApp.CoordWin.popup = NULL;
  NgraphApp.CoordWin.popup_item = NULL;
  NgraphApp.CoordWin.str = NULL;
  NgraphApp.CoordWin.ev_key = NULL;
  NgraphApp.CoordWin.ev_button = NULL;
  NgraphApp.CoordWin.window_state = 0;
}

void
application(char *file)
{
  int i;
  struct objlist *aobj;
  int x, y, width, height, w, h, snooper_id;
  GdkScreen *screen;

  if (TopLevel)
    return;

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
    Menulocal.menuheight = w * 1.2 / 2;

  x = Menulocal.menux;
  y = Menulocal.menuy;
  width = Menulocal.menuwidth;
  height = Menulocal.menuheight;

  NgraphApp.legend_text_list = entry_completion_create();
  NgraphApp.x_math_list = entry_completion_create();
  NgraphApp.y_math_list = entry_completion_create();
  NgraphApp.func_list = entry_completion_create();

  load_hist();

  TopLevel = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_window_set_title(GTK_WINDOW(TopLevel), AppName);
  g_set_application_name(AppName);

  gtk_window_set_default_size(GTK_WINDOW(TopLevel), width, height);
  gtk_window_move(GTK_WINDOW(TopLevel), x, y);

  gtk_widget_show_all(GTK_WIDGET(TopLevel));
  ResetEvent();

  //    g_signal_connect(TopLevel, "window-state-event", G_CALLBACK(change_window_state_cb), NULL);
  g_signal_connect(TopLevel, "delete-event", G_CALLBACK(CloseCallback), NULL);
  g_signal_connect(TopLevel, "destroy-event", G_CALLBACK(CloseCallback), NULL);
  //    NgraphClose = gdk_atom_intern_static_string(CloseMessage);

  set_gdk_color(&black, 0,     0,   0);
  set_gdk_color(&white, 255, 255, 255);
  set_gdk_color(&gray,  127, 127, 127);
  set_gdk_color(&red,   255,   0,   0);
  set_gdk_color(&blue,    0,   0, 255);

  createicon(TopLevel);
  gtk_window_set_default_icon_list(NgraphApp.iconpix);
  gtk_window_set_icon_list(GTK_WINDOW(TopLevel), NgraphApp.iconpix);

  create_toolbar_pixmap(TopLevel);

  init_ngraph_app_struct();

  initdialog();
  setupwindow();

  gtk_widget_show_all(GTK_WIDGET(TopLevel));

  NgraphApp.Changed = FALSE;
  NgraphApp.FileName = NULL;

  ResetEvent();

  ViewerWinSetup();

  create_markpixmap(TopLevel);

  create_cursor();
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
  signal(SIGCHLD, childhandler);
  gtk_widget_grab_focus(TopLevel);

  snooper_id = gtk_key_snooper_install(escape_drawing_cb, NULL);
  AppMainLoop();
  gtk_key_snooper_remove(snooper_id);

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

  snprintf(buf, sizeof(buf), "X:%d Y:%d", x, y);
  gtk_statusbar_pop(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message3);
  gtk_statusbar_push(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message3, buf);
}

void
SetZoom(double zm)
{
  char buf[20];

  snprintf(buf, sizeof(buf), "%.2f%%", zm * 100);
  gtk_statusbar_pop(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message2);
  gtk_statusbar_push(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message2, buf);
}

void
ResetZoom(void)
{
  gtk_statusbar_pop(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message2);
  gtk_statusbar_push(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message2, "");
}

void
SetStatusBar(char *mes)
{
  gtk_statusbar_pop(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message1);
  gtk_statusbar_push(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message1, mes);
}

void
SetStatusBarXm(gchar *s)
{
  gtk_statusbar_pop(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message1);
  gtk_statusbar_push(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message1, s);
}

void
ResetStatusBar(void)
{
  gtk_statusbar_pop(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message1);
  gtk_statusbar_push(GTK_STATUSBAR(NgraphApp.Message), NgraphApp.Message1, "");
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
  }
  memfree(fullpath);
}

void
SetCursor(unsigned int type)
{
  struct Viewer *d;
  GdkWindow *win;

  d = &(NgraphApp.Viewer);
  win = d->win;

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
  DisplayDialog(s);
  return strlen(s) + 1;
}

int
PutStderr(char *s)
{
  int len;

  MessageBox(TopLevel, s, _("Error:"), MB_ERROR);
  UpdateAll2();
  len = strlen(s);
  return len + 1;
}

int
ChkInterrupt(void)
{
#if 1
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
    NgraphApp.Interrupt= FALSE;
    return TRUE;
  }
#else
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

  if (NgraphApp.FileWin.Win != NULL)
    CmFileWindow(NULL, NULL);

  if (NgraphApp.AxisWin.Win != NULL)
    CmAxisWindow(NULL, NULL);

  if (NgraphApp.LegendWin.Win != NULL)
    CmLegendWindow(NULL, NULL);

  if (NgraphApp.MergeWin.Win != NULL)
    CmMergeWindow(NULL, NULL);

  if (NgraphApp.InfoWin.Win != NULL)
    CmInformationWindow(NULL, NULL);

  if (NgraphApp.CoordWin.Win != NULL)
    CmCoordinateWindow(NULL, NULL);

  ResetEvent();

  initwindowconfig();
  mgtkwindowconfig();


  gtk_window_get_position(GTK_WINDOW(TopLevel), &x, &y);
  gtk_window_get_size(GTK_WINDOW(TopLevel), &w0, &h0);

  Menulocal.menux = x;
  Menulocal.menuy = y;
  Menulocal.menuwidth = w0;
  Menulocal.menuheight = h0;

  defaultwindowconfig();

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


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

#include "dir_defs.h"

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
#include "gtk_ruler.h"
#include "gtk_presettings.h"

#include "init.h"
#include "osystem.h"
#include "omerge.h"
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
#include "x11parameter.h"
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
#define FIT_HISTORY      "fit_history"
#define FUNC_MATH_HISTORY   "func_math_history"
#define FUNC_FUNC_HISTORY   "func_func_history"

#define USE_EXT_DRIVER 0

#define SIDE_PANE_TAB_ID "side_pane"
int Menulock = FALSE, DnDLock = FALSE;
struct NgraphApp NgraphApp = {0};
GtkWidget *TopLevel = NULL, *DrawButton = NULL;

static GtkWidget *CurrentWindow = NULL, *CToolbar = NULL, *PToolbar = NULL, *SettingPanel = NULL, *ToolBox = NULL;
static enum {APP_CONTINUE, APP_QUIT, APP_QUIT_FORCE} Hide_window = APP_CONTINUE;
static int DrawLock = FALSE;
static unsigned int CursorType;

#if USE_EXT_DRIVER
static GtkWidget *ExtDrvOutMenu = NULL
#endif

struct MenuItem;
struct ToolItem;

static void create_menu(struct MenuItem *item);
static GtkWidget *create_toolbar(struct ToolItem *item, int n, GCallback btn_press_cb);
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
static void CmViewerButtonArm(GtkWidget *action, gpointer client_data);
#else
static void CmViewerButtonArm(GtkToggleToolButton *action, gpointer client_data);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
static char *Cursor[] = {
  "default",
  "text",
  "crosshair",
  "nw-resize",
  "ne-resize",
  "se-resize",
  "sw-resize",
  "all-scroll",
  "zoom-in",
  "zoom-out",
  "wait",
  "move",
  "crosshair",
  "cell",
};
#else
static GdkCursorType Cursor[] = {
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
#endif

#define CURSOR_TYPE_NUM (sizeof(Cursor) / sizeof(*Cursor))

enum FOCUS_TYPE {
  FOCUS_TYPE_1 = 0,
  FOCUS_TYPE_2,
  FOCUS_TYPE_3,
  FOCUS_TYPE_4,
  FOCUS_TYPE_5,
  FOCUS_TYPE_6,
  FOCUS_TYPE_NUM,
};

enum ACTION_TYPE {
  ACTION_TYPE_FILE,
  ACTION_TYPE_PARAMETER,
  ACTION_TYPE_AXIS,
  ACTION_TYPE_GRID,
  ACTION_TYPE_PATH,
  ACTION_TYPE_ARC,
  ACTION_TYPE_RECTANGLE,
  ACTION_TYPE_MARK,
  ACTION_TYPE_TEXT,
  ACTION_TYPE_MERGE,
  ACTION_TYPE_FOCUS_EDIT1,
  ACTION_TYPE_FOCUS_EDIT2,
  ACTION_TYPE_FOCUS_EDIT_PASTE,
  ACTION_TYPE_FOCUS_ROTATE,
  ACTION_TYPE_FOCUS_FLIP,
  ACTION_TYPE_FOCUS_UP,
  ACTION_TYPE_FOCUS_DOWN,
  ACTION_TYPE_AXIS_UNDO,
  ACTION_TYPE_MODIFIED,
  ACTION_TYPE_NONE,
};

struct ActionWidget {
  GAction *action;
  enum ACTION_TYPE type;
};

enum ActionWidgetIndex {
  GraphSaveAction,
  EditRedoAction,
  EditUndoAction,
  EditCutAction,
  EditCopyAction,
  EditRotateCCWAction,
  EditRotateCWAction,
  EditFlipVAction,
  EditFlipHAction,
  EditDeleteAction,
  EditDuplicateAction,
  EditSelectAllAction,
  EditAlignLeftAction,
  EditAlignRightAction,
  EditAlignHCenterAction,
  EditAlignTopAction,
  EditAlignBottomAction,
  EditAlignVCenterAction,
  EditOrderTopAction,
  EditOrderUpAction,
  EditOrderDownAction,
  EditOrderBottomAction,
  EditPasteAction,
  ViewSidebarAction,
  ViewStatusbarAction,
  ViewRulerAction,
  ViewScrollbarAction,
  ViewCommandToolbarAction,
  ViewToolboxAction,
  ViewCrossGaugeAction,
  ViewGridLineAction,
  DataPropertyAction,
  DataCloseAction,
  DataEditAction,
  DataSaveAction,
  DataMathAction,
  ParameterPropertyAction,
  ParameterDeleteAction,
  AxisPropertyAction,
  AxisDeleteAction,
  AxisScaleZoomAction,
  AxisScaleClearAction,
  AxisGridPropertyAction,
  AxisGridDeleteAction,
  AxisScaleUndoAction,
  LegendPathPropertyAction,
  LegendPathDeleteAction,
  LegendRectanglePropertyAction,
  LegendRectangleDeleteAction,
  LegendArcPropertyAction,
  LegendArcDeleteAction,
  LegendMarkPropertyAction,
  LegendMarkDeleteAction,
  LegendTextPropertyAction,
  LegendTextDeleteAction,
  MergePropertyAction,
  MergeCloseAction,
  ActionWidgetNum,
};

struct ActionWidget ActionWidget[ActionWidgetNum];
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
static GtkWidget *PointerModeButtons[PointerModeNum];
#else
static GtkToolItem *PointerModeButtons[PointerModeNum];
#endif
static int DefaultMode = PointerModeBoth;

struct ToolItem {
  enum {
    TOOL_TYPE_NORMAL,
    TOOL_TYPE_DRAW,
    TOOL_TYPE_SAVE,
    TOOL_TYPE_RADIO,
    TOOL_TYPE_RECENT_GRAPH,
    TOOL_TYPE_RECENT_DATA,
    TOOL_TYPE_SEPARATOR,
  } type;
  const char *label;
  const char *tip;
  const char *caption;
  const char *icon;
  GCallback callback;
  int user_data;
  int button;
  const char *action_name;
};

#define PointerModeOffset 10

static struct ToolItem PointerToolbar[] = {
  {
    TOOL_TYPE_RADIO,
    N_("Point"),
    N_("Pointer"),
    N_("Legend and Axis Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"),
    NGRAPH_POINT_ICON,
    G_CALLBACK(CmViewerButtonArm),
    PointB,
    PointerModeBoth + PointerModeOffset,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Legend"),
    N_("Legend Pointer"),
    N_("Legend Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"),
    NGRAPH_LEGENDPOINT_ICON,
    G_CALLBACK(CmViewerButtonArm),
    LegendB,
    PointerModeLegend + PointerModeOffset,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Axis"),
    N_("Axis Pointer"),
    N_("Axis Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"),
    NGRAPH_AXISPOINT_ICON,
    G_CALLBACK(CmViewerButtonArm),
    AxisB,
    PointerModeAxis + PointerModeOffset,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Data"),
    N_("Data Pointer"),
    N_("Data Pointer"),
    NGRAPH_DATAPOINT_ICON,
    G_CALLBACK(CmViewerButtonArm),
    DataB,
    PointerModeData + PointerModeOffset,
  },
  {
    TOOL_TYPE_SEPARATOR,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Path"),
    N_("Path"),
    N_("New Legend Path (+SHIFT: Fine +CONTROL: snap angle)"),
    NGRAPH_LINE_ICON,
    G_CALLBACK(CmViewerButtonArm),
    PathB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Rectangle"),
    N_("Rectangle"),
    N_("New Legend Rectangle (+SHIFT: Fine +CONTROL: square integer ratio rectangle)"),
    NGRAPH_RECT_ICON,
    G_CALLBACK(CmViewerButtonArm),
    RectB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Arc"),
    N_("Arc"),
    N_("New Legend Arc (+SHIFT: Fine +CONTROL: circle or integer ratio ellipse)"),
    NGRAPH_ARC_ICON,
    G_CALLBACK(CmViewerButtonArm),
    ArcB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Mark"),
    N_("Mark"),
    N_("New Legend Mark (+SHIFT: Fine)"),
    NGRAPH_MARK_ICON,
    G_CALLBACK(CmViewerButtonArm),
    MarkB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Text"),
    N_("Text"),
    N_("New Legend Text (+SHIFT: Fine)"),
    NGRAPH_TEXT_ICON,
    G_CALLBACK(CmViewerButtonArm),
    TextB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Gauss"),
    N_("Gaussian"),
    N_("New Legend Gaussian (+SHIFT: Fine +CONTROL: integer ratio)"),
    NGRAPH_GAUSS_ICON,
    G_CALLBACK(CmViewerButtonArm),
    GaussB,
  },
  {
    TOOL_TYPE_SEPARATOR,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Frame axis"),
    N_("Frame Graph"),
    N_("New Frame Graph (+SHIFT: Fine +CONTROL: integer ratio)"),
    NGRAPH_FRAME_ICON,
    G_CALLBACK(CmViewerButtonArm),
    FrameB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Section axis"),
    N_("Section Graph"),
    N_("New Section Graph (+SHIFT: Fine +CONTROL: integer ratio)"),
    NGRAPH_SECTION_ICON,
    G_CALLBACK(CmViewerButtonArm),
    SectionB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Cross axis"),
    N_("Cross Graph"),
    N_("New Cross Graph (+SHIFT: Fine +CONTROL: integer ratio)"),
    NGRAPH_CROSS_ICON,
    G_CALLBACK(CmViewerButtonArm),
    CrossB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Single axis"),
    N_("Single Axis"),
    N_("New Single Axis (+SHIFT: Fine +CONTROL: snap angle)"),
    NGRAPH_SINGLE_ICON,
    G_CALLBACK(CmViewerButtonArm),
    SingleB,
  },
  {
    TOOL_TYPE_SEPARATOR,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Trimming"),
    N_("Axis Trimming"),
    N_("Axis Trimming (+SHIFT: Fine)"),
    NGRAPH_TRIMMING_ICON,
    G_CALLBACK(CmViewerButtonArm),
    TrimB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Evaluate"),
    N_("Evaluate Data"),
    N_("Evaluate Data Point"),
    NGRAPH_EVAL_ICON,
    G_CALLBACK(CmViewerButtonArm),
    EvalB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Zoom"),
    N_("Viewer Zoom"),
    N_("Viewer Zoom-In (+CONTROL: Zoom-Out +SHIFT: Centering)"),
    NGRAPH_ZOOM_ICON,
    G_CALLBACK(CmViewerButtonArm),
    ZoomB,
  },
};

static struct ToolItem CommandToolbar[] = {
  {
    TOOL_TYPE_RECENT_DATA,
    N_("_Add"),
    N_("Add Data"),
    N_("Add Data file"),
    "text-x-generic-symbolic",
    NULL,
    0,
    0,
    "app.DataAddFileAction",
  },
  {
    TOOL_TYPE_SEPARATOR,
    NULL,
  },
  {
    TOOL_TYPE_RECENT_GRAPH,
    N_("_Load graph"),
    N_("Load NGP"),
    N_("Load NGP file"),
    "document-open-symbolic",
    NULL,
    0,
    0,
    "app.GraphLoadAction",
  },
  {
    TOOL_TYPE_SAVE,
    N_("_Save"),
    N_("Save NGP"),
    N_("Save NGP file"),
    "document-save-symbolic",
    NULL,
    0,
    0,
    "app.GraphSaveAction",
  },
  {
    TOOL_TYPE_SEPARATOR,
    NULL,
  },
  {
    TOOL_TYPE_NORMAL,
    N_("Scale _Clear"),
    N_("Clear Scale"),
    N_("Clear Scale"),
    NGRAPH_SCALE_ICON,
    NULL,
    0,
    0,
    "app.AxisScaleClearAction",
  },
  {
    TOOL_TYPE_DRAW,
    N_("_Draw"),
    N_("Draw"),
    N_("Draw on Viewer Window"),
    "ngraph_draw-symbolic",
    NULL,
    0,
    0,
    "app.ViewDrawDirectAction",
  },
  {
    TOOL_TYPE_NORMAL,
    N_("_Print"),
    N_("Print"),
    N_("Print"),
    "document-print-symbolic",
    NULL,
    0,
    0,
    "app.GraphPrintAction",
  },
  {
    TOOL_TYPE_SEPARATOR,
    NULL,
  },
  {
    TOOL_TYPE_NORMAL,
    N_("_Math Transformation"),
    N_("Math Transformation"),
    N_("Set Math Transformation"),
    "accessories-calculator-symbolic",
    NULL,
    0,
    0,
    "app.DataMathAction",
  },
  {
    TOOL_TYPE_NORMAL,
    N_("Scale _Undo"),
    N_("Scale Undo"),
    N_("Undo Scale Settings"),
    "edit-undo-symbolic",
    NULL,
    0,
    0,
    "app.AxisScaleUndoAction",
  },
};

struct MenuItem {
  struct ActionWidget *action;
  const char *action_name;
};

static struct MenuItem MenuAction[] = {
  {
    ActionWidget + MergePropertyAction,
    "MergePropertyAction",
  },
  {
    ActionWidget + MergeCloseAction,
    "MergeCloseAction",
  },
  {
    ActionWidget + LegendTextPropertyAction,
    "LegendTextPropertyAction",
  },
  {
    ActionWidget + LegendTextDeleteAction,
    "LegendTextDeleteAction",
  },
  {
    ActionWidget + LegendMarkPropertyAction,
    "LegendMarkPropertyAction",
  },
  {
    ActionWidget + LegendMarkDeleteAction,
    "LegendMarkDeleteAction",
  },
  {
    ActionWidget + LegendArcPropertyAction,
    "LegendArcPropertyAction",
  },
  {
    ActionWidget + LegendArcDeleteAction,
    "LegendArcDeleteAction",
  },
  {
    ActionWidget + LegendRectanglePropertyAction,
    "LegendRectanglePropertyAction",
  },
  {
    ActionWidget + LegendRectangleDeleteAction,
    "LegendRectangleDeleteAction",
  },
  {
    ActionWidget + LegendPathPropertyAction,
    "LegendPathPropertyAction",
  },
  {
    ActionWidget + LegendPathDeleteAction,
    "LegendPathDeleteAction",
  },
  {
    ActionWidget + AxisGridPropertyAction,
    "AxisGridPropertyAction",
  },
  {
    ActionWidget + AxisGridDeleteAction,
    "AxisGridDeleteAction",
  },
  {
    ActionWidget + AxisPropertyAction,
    "AxisPropertyAction",
  },
  {
    ActionWidget + AxisDeleteAction,
    "AxisDeleteAction",
  },
  {
    ActionWidget + AxisScaleZoomAction,
    "AxisScaleZoomAction",
  },
  {
    ActionWidget + AxisScaleClearAction,
    "AxisScaleClearAction",
  },
  {
    ActionWidget + AxisScaleUndoAction,
    "AxisScaleUndoAction",
  },
  {
    ActionWidget + ParameterPropertyAction,
    "ParameterPropertyAction",
  },
  {
    ActionWidget + ParameterDeleteAction,
    "ParameterDeleteAction",
  },
  {
    ActionWidget + DataPropertyAction,
    "DataPropertyAction",
  },
  {
    ActionWidget + DataCloseAction,
    "DataCloseAction",
  },
  {
    ActionWidget + DataEditAction,
    "DataEditAction",
  },
  {
    ActionWidget + DataSaveAction,
    "DataSaveAction",
  },
  {
    ActionWidget + DataMathAction,
    "DataMathAction",
  },
  {
    ActionWidget + ViewSidebarAction,
    "ViewSidebarAction",
  },
  {
    ActionWidget + ViewStatusbarAction,
    "ViewStatusbarAction",
  },
  {
    ActionWidget + ViewRulerAction,
    "ViewRulerAction",
  },
  {
    ActionWidget + ViewScrollbarAction,
    "ViewScrollbarAction",
  },
  {
    ActionWidget + ViewCommandToolbarAction,
    "ViewCommandToolbarAction",
  },
  {
    ActionWidget + ViewToolboxAction,
    "ViewToolboxAction",
  },
  {
    ActionWidget + ViewCrossGaugeAction,
    "ViewCrossGaugeAction",
  },
  {
    ActionWidget + ViewGridLineAction,
    "ViewGridLineAction",
  },
  {
    ActionWidget + EditOrderTopAction,
    "EditOrderTopAction",
  },
  {
    ActionWidget + EditOrderUpAction,
    "EditOrderUpAction",
  },
  {
    ActionWidget + EditOrderDownAction,
    "EditOrderDownAction",
  },
  {
    ActionWidget + EditOrderBottomAction,
    "EditOrderBottomAction",
  },
  {
    ActionWidget + EditAlignLeftAction,
    "EditAlignLeftAction",
  },
  {
    ActionWidget + EditAlignHCenterAction,
    "EditAlignHCenterAction",
  },
  {
    ActionWidget + EditAlignRightAction,
    "EditAlignRightAction",
  },
  {
    ActionWidget + EditAlignTopAction,
    "EditAlignTopAction",
  },
  {
    ActionWidget + EditAlignVCenterAction,
    "EditAlignVCenterAction",
  },
  {
    ActionWidget + EditAlignBottomAction,
    "EditAlignBottomAction",
  },
  {
    ActionWidget + EditRedoAction,
    "EditRedoAction",
  },
  {
    ActionWidget + EditUndoAction,
    "EditUndoAction",
  },
  {
    ActionWidget + EditCutAction,
    "EditCutAction",
  },
  {
    ActionWidget + EditCopyAction,
    "EditCopyAction",
  },
  {
    ActionWidget + EditPasteAction,
    "EditPasteAction",
  },
  {
    ActionWidget + EditDeleteAction,
    "EditDeleteAction",
  },
  {
    ActionWidget + EditDuplicateAction,
    "EditDuplicateAction",
  },
  {
    ActionWidget + EditSelectAllAction,
    "EditSelectAllAction",
  },
  {
    ActionWidget + EditRotateCWAction,
    "EditRotateCWAction",
  },
  {
    ActionWidget + EditRotateCCWAction,
    "EditRotateCCWAction",
  },
  {
    ActionWidget + EditFlipHAction,
    "EditFlipHAction",
  },
  {
    ActionWidget + EditFlipVAction,
    "EditFlipVAction",
  },
  {
    ActionWidget + GraphSaveAction,
    "GraphSaveAction",
  },
  {
    NULL,
  },
};

void
set_pointer_mode(int id)
{
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
  GtkWidget *button;
#else
  GtkToolItem *button;
#endif

  button = NULL;
  switch (id) {
  case PointerModeDefault:
    button = PointerModeButtons[DefaultMode];
    break;
  case PointerModeFocus:
    button = PointerModeButtons[(DefaultMode == PointerModeData) ? PointerModeBoth : DefaultMode];
    break;
  case PointerModeFocusAxis:
    switch (DefaultMode) {
    case PointerModeLegend:
    case PointerModeData:
      id = PointerModeBoth;
      break;
    default:
      id = DefaultMode;
      break;
    }
    button = PointerModeButtons[id];
    break;
  case PointerModeFocusLegend:
    switch (DefaultMode) {
    case PointerModeAxis:
    case PointerModeData:
      id = PointerModeBoth;
      break;
    default:
      id = DefaultMode;
      break;
    }
    button = PointerModeButtons[id];
    break;
  case PointerModeBoth:
  case PointerModeLegend:
  case PointerModeAxis:
  case PointerModeData:
    button = PointerModeButtons[id];
    break;
  }

  if (button) {
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
#else
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(button), TRUE);
#endif
  }
}

static void
init_action_widget_list(void)
{
  int i;

  for (i = 0; i < ActionWidgetNum; i++) {
    switch (i) {
    case GraphSaveAction:
      ActionWidget[i].type = ACTION_TYPE_MODIFIED;
      break;
    case EditCutAction:
    case EditCopyAction:
      ActionWidget[i].type = ACTION_TYPE_FOCUS_EDIT1;
      break;
    case EditRotateCCWAction:
    case EditRotateCWAction:
      ActionWidget[i].type = ACTION_TYPE_FOCUS_ROTATE;
      break;
    case EditFlipVAction:
    case EditFlipHAction:
      ActionWidget[i].type = ACTION_TYPE_FOCUS_FLIP;
      break;
    case EditDeleteAction:
    case EditDuplicateAction:
    case EditAlignLeftAction:
    case EditAlignRightAction:
    case EditAlignHCenterAction:
    case EditAlignTopAction:
    case EditAlignBottomAction:
    case EditAlignVCenterAction:
      ActionWidget[i].type = ACTION_TYPE_FOCUS_EDIT2;
      break;
    case EditOrderTopAction:
    case EditOrderUpAction:
      ActionWidget[i].type = ACTION_TYPE_FOCUS_UP;
      break;
    case EditOrderDownAction:
    case EditOrderBottomAction:
      ActionWidget[i].type = ACTION_TYPE_FOCUS_DOWN;
      break;
    case EditPasteAction:
      ActionWidget[i].type = ACTION_TYPE_FOCUS_EDIT_PASTE;
      break;
    case DataPropertyAction:
    case DataCloseAction:
    case DataEditAction:
    case DataSaveAction:
    case DataMathAction:
      ActionWidget[i].type = ACTION_TYPE_FILE;
      break;
    case ParameterPropertyAction:
    case ParameterDeleteAction:
      ActionWidget[i].type = ACTION_TYPE_PARAMETER;
      break;
    case AxisPropertyAction:
    case AxisDeleteAction:
    case AxisScaleZoomAction:
    case AxisScaleClearAction:
      ActionWidget[i].type = ACTION_TYPE_AXIS;
      break;
    case AxisGridPropertyAction:
    case AxisGridDeleteAction:
      ActionWidget[i].type = ACTION_TYPE_GRID;
      break;
    case AxisScaleUndoAction:
      ActionWidget[i].type = ACTION_TYPE_AXIS_UNDO;
      break;
    case LegendPathPropertyAction:
    case LegendPathDeleteAction:
      ActionWidget[i].type = ACTION_TYPE_PATH;
      break;
    case LegendRectanglePropertyAction:
    case LegendRectangleDeleteAction:
      ActionWidget[i].type = ACTION_TYPE_RECTANGLE;
      break;
   case LegendArcPropertyAction:
    case LegendArcDeleteAction:
      ActionWidget[i].type = ACTION_TYPE_ARC;
      break;
    case LegendMarkPropertyAction:
    case LegendMarkDeleteAction:
      ActionWidget[i].type = ACTION_TYPE_MARK;
      break;
    case LegendTextPropertyAction:
    case LegendTextDeleteAction:
      ActionWidget[i].type = ACTION_TYPE_TEXT;
      break;
    case MergePropertyAction:
    case MergeCloseAction:
      ActionWidget[i].type = ACTION_TYPE_MERGE;
      break;
    default:
      ActionWidget[i].type = ACTION_TYPE_NONE;
    }
  }
}

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
  GtkWidget *w;
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

#if 0
  gtk_widget_set_sensitive(NgraphApp.Viewer.menu, ! Menulock);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  w = gtk_paned_get_start_child
    (GTK_PANED(NgraphApp.Viewer.main_pane));
#else
  w = gtk_paned_get_child1(GTK_PANED(NgraphApp.Viewer.main_pane));
#endif
  if (w) {
    gtk_widget_set_sensitive(w, ! Menulock);
  }
  gtk_widget_set_sensitive(ToolBox, ! Menulock);
}

static void
set_action_widget_sensitivity(int id, int state)
{
  if (ActionWidget[id].action) {
    g_simple_action_set_enabled(G_SIMPLE_ACTION(ActionWidget[id].action), state);
  }
}

void
set_axis_undo_button_sensitivity(int state)
{
  set_action_widget_sensitivity(AxisScaleUndoAction, state);
}

void
set_draw_lock(int lock)
{
  DrawLock = lock;
}

#if ! WINDOWS
static void
kill_signal_handler(int sig)
{
  if (Menulock || check_paint_lock()) {
    set_interrupt();		/* accept SIGINT */
  } else {
    Hide_window = APP_QUIT;
  }
}

static void
term_signal_handler(int sig)
{
  Hide_window = APP_QUIT_FORCE;
}
#endif	/* WINDOWS */

static int
AppMainLoop(void)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  GMainContext *context;
  context = g_main_context_default();
#endif
  Hide_window = APP_CONTINUE;
  while (TRUE) {
#if GTK_CHECK_VERSION(4, 0, 0)
    g_main_context_iteration(context, TRUE);
#else
    gtk_main_iteration();
#endif
    if (Hide_window != APP_CONTINUE &&
#if GTK_CHECK_VERSION(4, 0, 0)
	! g_main_context_pending(context)
#else
	! gtk_events_pending()
#endif
	) {
      int state = Hide_window;

      Hide_window = APP_CONTINUE;
      switch (state) {
      case APP_QUIT:
	if (CheckSave()) {
	  menu_clear_undo();
	  return 0;
	}
	break;
      case APP_QUIT_FORCE:
	menu_clear_undo();
	return 1;
      }
    }
  }
  return 0;
}

void
reset_event(void)
{
#if GTK_CHECK_VERSION(4, 0, 0)
  GMainContext *context;
  context = g_main_context_default();
  while (g_main_context_pending(context)) {
    g_main_context_iteration(context, TRUE);
  }
#else
  while (gtk_events_pending()) {
    gtk_main_iteration();
  }
#endif
}

static gboolean
CloseCallback(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  CmGraphQuit(NULL, user_data);

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
find_gra2gdk_inst(struct objlist **o, N_VALUE **i, struct objlist **ro, int *routput, struct gra2cairo_local **rlocal)
{
  static struct objlist *obj = NULL, *robj = NULL;
  static int pos;
  N_VALUE *inst = NULL;

  if (obj == NULL) {
    obj = chkobject("gra2gdk");
    pos = getobjtblpos(obj, "_output", &robj);
  }

  if (obj == NULL)
    return FALSE;

  inst = chkobjinst(obj, 0);
  if (inst == NULL) {
    int id;
    id = newobj(obj);
    if (id < 0)
      return FALSE;

    inst = chkobjinst(obj, 0);
  }

  if (inst == NULL) {
    return FALSE;
  }

  _getobj(obj, "_local", inst, rlocal);

  *routput = pos;
  *i = inst;
  *o = obj;
  *ro = robj;

  return TRUE;
}


#define ADDIN_MENU_SECTION_INDEX 6
void
create_addin_menu(void)
{
  GMenuModel *menu;
  GMenu *addin_menu;
  struct script *fcur;
  GMenuItem *item;
  int i, n;
  char buf[1024];

  menu = gtk_application_get_menubar(GtkApp);
  if (menu == NULL) {
    return;
  }

  menu = g_menu_model_get_item_link(G_MENU_MODEL(menu), 0, G_MENU_LINK_SUBMENU);
  if (menu == NULL) {
    return;
  }

  n = g_menu_model_get_n_items(menu);
  if (n < ADDIN_MENU_SECTION_INDEX) {
    return;
  }

  menu = g_menu_model_get_item_link(menu, ADDIN_MENU_SECTION_INDEX, G_MENU_LINK_SECTION);
  if (menu == NULL) {
    return;
  }

  n = g_menu_model_get_n_items(menu);
  if (n > 1) {
    g_menu_remove(G_MENU(menu), 0);
  }

  addin_menu = g_menu_new();
  i = 0;
  fcur = Menulocal.scriptroot;
  while (fcur) {
    if (fcur->name && fcur->script) {
      snprintf(buf, sizeof(buf), "app.GraphAddinAction(%d)", i);
      item = g_menu_item_new(fcur->name, buf);
      g_menu_append_item(addin_menu, item);
      g_object_unref(item);
    }
    fcur = fcur->next;
    i++;
  }

  g_menu_prepend_submenu(G_MENU(menu), _("_Add-in"), G_MENU_MODEL(addin_menu));
}

static void
set_focus_sensitivity_sub(const struct Viewer *d, int insensitive)
{
  int i, num, type, state, up_state, down_state;
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  GtkClipboard *clip;
#endif

  num = check_focused_obj_type(d, &type);

  up_state = down_state = FALSE;
  if (num == 1 && (type & (FOCUS_OBJ_TYPE_LEGEND | FOCUS_OBJ_TYPE_MERGE))) {
    int id, last_id;
    struct FocusObj *focus;

    focus= * (struct FocusObj **) arraynget(d->focusobj, 0);
    id = chkobjoid(focus->obj, focus->oid);
    last_id = chkobjlastinst(focus->obj);

    up_state = (id > 0);
    down_state = (id < last_id);
  }

  for (i = 0; i < ActionWidgetNum; i++) {
    switch (ActionWidget[i].type) {
    case ACTION_TYPE_FOCUS_EDIT1:
      if (insensitive) {
	state = FALSE;
      } else {
	state = (! (type & FOCUS_OBJ_TYPE_AXIS) && (type & (FOCUS_OBJ_TYPE_MERGE | FOCUS_OBJ_TYPE_LEGEND)));
      }
      set_action_widget_sensitivity(i, state);
      break;
    case ACTION_TYPE_FOCUS_EDIT2:
      if (insensitive) {
	state = FALSE;
      } else {
	state = (type & (FOCUS_OBJ_TYPE_AXIS | FOCUS_OBJ_TYPE_LEGEND | FOCUS_OBJ_TYPE_MERGE));
      }
      set_action_widget_sensitivity(i, state);
      break;
    case ACTION_TYPE_FOCUS_EDIT_PASTE:
      if (insensitive) {
	state = FALSE;
      } else {
	switch (d->Mode) {
	case PointB:
	case LegendB:
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
          state = FALSE;
#else
	  clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	  state = gtk_clipboard_wait_is_text_available(clip);
#endif
	  break;
	default:
	  state = FALSE;
	}
      }
      set_action_widget_sensitivity(i, state);
      break;
    case ACTION_TYPE_FOCUS_ROTATE:
      if (insensitive) {
	state = FALSE;
      } else {
	state = (! (type & FOCUS_OBJ_TYPE_MERGE) && (type & (FOCUS_OBJ_TYPE_AXIS | FOCUS_OBJ_TYPE_LEGEND)));
      }
      set_action_widget_sensitivity(i, state);
      break;
    case ACTION_TYPE_FOCUS_FLIP:
      if (insensitive) {
	state = FALSE;
      } else {
	state = (! (type & (FOCUS_OBJ_TYPE_MERGE | FOCUS_OBJ_TYPE_TEXT)) && (type & (FOCUS_OBJ_TYPE_AXIS | FOCUS_OBJ_TYPE_LEGEND)));
      }
      set_action_widget_sensitivity(i, state);
      break;
    case ACTION_TYPE_FOCUS_UP:
      state = up_state;
      set_action_widget_sensitivity(i, state);
      break;
    case ACTION_TYPE_FOCUS_DOWN:
      state = down_state;
      set_action_widget_sensitivity(i, state);
      break;
    default:
      continue;
    }
  }
}

void
set_focus_insensitive(const struct Viewer *d)
{
  set_focus_sensitivity_sub(d, TRUE);
}

void
set_focus_sensitivity(const struct Viewer *d)
{
  set_focus_sensitivity_sub(d, FALSE);
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
create_markpixmap(GtkWidget *win)
{
  cairo_surface_t *pix;
  int gra, i, R, G, B, R2, G2, B2, found, output;
  struct objlist *obj, *robj;
  N_VALUE *inst;
  struct gra2cairo_local *local;
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
  int window = TRUE;
#else
  GdkWindow *window;
#endif

  R = G = B = 0;
  R2 = 0;
  G2 = B2 = 255;

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  window = gtk_widget_get_window(win);
#endif
  found = find_gra2gdk_inst(&obj, &inst, &robj, &output, &local);

  for (i = 0; i < MARK_TYPE_NUM; i++) {
    pix = NULL;
    if (window && found) {
      pix = gra2gdk_create_pixmap(local, MARK_PIX_SIZE, MARK_PIX_SIZE,
				  1.0, 1.0, 1.0);
      if (pix) {
	gra = _GRAopen("gra2gdk", "_output",
		       robj, inst, output, -1, -1, -1, NULL, local);
	if (gra >= 0) {
	  GRAview(gra, 0, 0, MARK_PIX_SIZE, MARK_PIX_SIZE, 0);
	  GRAlinestyle(gra, 0, NULL, 1, GRA_LINE_CAP_BUTT, GRA_LINE_JOIN_MITER, 1000);
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
    if (NgraphApp.markpix[i]) {
      cairo_surface_destroy(NgraphApp.markpix[i]);
    }
    NgraphApp.markpix[i] = NULL;
  }
}

static void
create_icon(void)
{
  GList *list = NULL;
  GdkPixbuf *pixbuf;

  pixbuf = gdk_pixbuf_new_from_resource(NGRAPH_SVG_ICON_FILE, NULL);
  if (pixbuf) {
    list = g_list_append(list, pixbuf);
  }

  pixbuf = gdk_pixbuf_new_from_resource(NGRAPH_ICON_FILE, NULL);
  if (pixbuf) {
    list = g_list_append(list, pixbuf);
  }

  pixbuf = gdk_pixbuf_new_from_resource(NGRAPH_ICON64_FILE, NULL);
  if (pixbuf) {
    list = g_list_append(list, pixbuf);
  }

  pixbuf = gdk_pixbuf_new_from_resource(NGRAPH_ICON128_FILE, NULL);
  if (pixbuf) {
    list = g_list_append(list, pixbuf);
  }

  if (list) {
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
    gtk_window_set_default_icon_list(list);
    gtk_window_set_icon_list(GTK_WINDOW(TopLevel), list);
#endif
    g_list_free_full(list, g_object_unref);
  }
}

static int
create_cursor(void)
{
  unsigned int i;

  NgraphApp.cursor = g_malloc(sizeof(GdkCursor *) * CURSOR_TYPE_NUM);
  if (NgraphApp.cursor == NULL)
    return 1;

  for (i = 0; i < CURSOR_TYPE_NUM; i++) {
#if GTK_CHECK_VERSION(4, 0, 0)
    NgraphApp.cursor[i] = gdk_cursor_new_from_name(Cursor[i], NULL);
#else
    NgraphApp.cursor[i] = gdk_cursor_new_for_display(gdk_display_get_default(), Cursor[i]);
#endif
  }

  return 0;
}

static void
free_cursor(void)
{
  unsigned int i;

  for (i = 0; i < CURSOR_TYPE_NUM; i++) {
    g_object_unref(NgraphApp.cursor[i]);
    NgraphApp.cursor[i] = NULL;
  }
  g_free(NgraphApp.cursor);
  NgraphApp.cursor = NULL;
}

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
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
#endif

static GtkWidget *
create_message_box(GtkWidget **label1, GtkWidget **label2)
{
  GtkWidget *frame, *w, *hbox;

  frame = gtk_frame_new(NULL);
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);
#endif

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  w = gtk_label_new(NULL);
  gtk_widget_set_halign(w, GTK_ALIGN_END);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), w);
#else
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif
  *label1 = w;

  w = gtk_label_new(NULL);
  gtk_widget_set_halign(w, GTK_ALIGN_START);
  gtk_label_set_width_chars(GTK_LABEL(w), 16);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), w);
#else
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
#endif
  *label2 = w;

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_frame_set_child(GTK_FRAME(frame), hbox);
#else
  gtk_container_add(GTK_CONTAINER(frame), hbox);
#endif

  return frame;
}

#define OBJ_ID_KEY "ngraph_object_id"

static void
setup_object_tab(struct SubWin *win, GtkWidget *tab, const char *icon_name, const char *tip)
{
  GtkWidget *icon;
  int obj_id;

  obj_id = chkobjectid(win->data.data->obj);
  g_object_set_data(G_OBJECT(win->Win), OBJ_ID_KEY, GINT_TO_POINTER(obj_id));

#if GTK_CHECK_VERSION(4, 0, 0)
  icon = gtk_image_new_from_icon_name(icon_name);
#else
  icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
#endif
  gtk_widget_set_tooltip_text(icon, tip);

  gtk_notebook_append_page(GTK_NOTEBOOK(tab), win->Win, icon);
  gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(tab), win->Win, TRUE);
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(tab), win->Win, TRUE);
  gtk_notebook_set_menu_label_text(GTK_NOTEBOOK(tab), win->Win, tip);
}

static void
get_pane_position(void)
{
  Menulocal.main_pane_pos = gtk_paned_get_position(GTK_PANED(NgraphApp.Viewer.main_pane));
  Menulocal.side_pane1_pos = gtk_paned_get_position(GTK_PANED(NgraphApp.Viewer.side_pane1));
  Menulocal.side_pane2_pos = gtk_paned_get_position(GTK_PANED(NgraphApp.Viewer.side_pane2));
  Menulocal.side_pane3_pos = gtk_paned_get_position(GTK_PANED(NgraphApp.Viewer.side_pane3));
}

static void
set_pane_position(void)
{
  gtk_paned_set_position(GTK_PANED(NgraphApp.Viewer.main_pane), Menulocal.main_pane_pos);
  gtk_paned_set_position(GTK_PANED(NgraphApp.Viewer.side_pane3), Menulocal.side_pane3_pos);
  gtk_paned_set_position(GTK_PANED(NgraphApp.Viewer.side_pane1), Menulocal.side_pane1_pos);
  gtk_paned_set_position(GTK_PANED(NgraphApp.Viewer.side_pane2), Menulocal.side_pane2_pos);
}

struct obj_tab_info {
  int tab, order;
  int *conf;
  int obj_id;
  const char *obj_name;
  GtkWidget * (* init_func)(struct SubWin *);
  struct SubWin *d;
  const char *icon;
};

static int
tab_info_compare(const void * a, const void * b)
{
  const struct obj_tab_info *info_a, *info_b;

  info_a = (const struct obj_tab_info *) a;
  info_b = (const struct obj_tab_info *) b;

  return info_a->order + info_a->tab * 100 - info_b->order - info_b->tab * 100;
}

static void
init_tab_info(struct obj_tab_info *info, int n)
{
  int i;

  for (i = 0; i < n; i++) {
    int position;
    struct objlist *obj;
    position = *info[i].conf;
    if (position > 99) {
      info[i].tab = 1;
    } else {
      info[i].tab = 0;
    }
    obj = chkobject(info[i].obj_name);
    info[i].obj_id = chkobjectid(obj);
    info[i].order = position % 100;
  }

  qsort(info, n, sizeof(*info), tab_info_compare);
}

static void
save_tab_position_sub(GtkWidget *tab, struct obj_tab_info *tab_info, int offset)
{
  int i, n;

  n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(tab));
  for (i = 0; i < n; i++){
    int obj_id;
    GtkWidget *w;
    w = gtk_notebook_get_nth_page(GTK_NOTEBOOK(tab), i);
    obj_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), OBJ_ID_KEY));
    if (obj_id == tab_info->obj_id) {
      *tab_info->conf = offset + i;
      break;
    }
  }
}

static void
save_tab_position(void)
{
  int i, n;
  struct obj_tab_info tab_info[] = {
    {0, 0, &Menulocal.file_tab,      0, "data"},
    {0, 0, &Menulocal.axis_tab,      0, "axis"},
    {0, 0, &Menulocal.merge_tab,     0, "merge"},
    {0, 0, &Menulocal.path_tab,      0, "path"},
    {0, 0, &Menulocal.rectangle_tab, 0, "rectangle"},
    {0, 0, &Menulocal.arc_tab,       0, "arc"},
    {0, 0, &Menulocal.mark_tab,      0, "mark"},
    {0, 0, &Menulocal.text_tab,      0, "text"},
    {0, 0, &Menulocal.parameter_tab, 0, "parameter"},
  };

  n = sizeof(tab_info) / sizeof(*tab_info);
  init_tab_info(tab_info, n);

  for (i = 0; i < n; i++) {
    GtkWidget *tab;
#if GTK_CHECK_VERSION(4, 0, 0)
    tab = gtk_paned_get_start_child(GTK_PANED(NgraphApp.Viewer.side_pane2));
    save_tab_position_sub(tab, tab_info + i, 0);

    tab = gtk_paned_get_end_child(GTK_PANED(NgraphApp.Viewer.side_pane2));
    save_tab_position_sub(tab, tab_info + i, 100);
#else
    tab = gtk_paned_get_child1(GTK_PANED(NgraphApp.Viewer.side_pane2));
    save_tab_position_sub(tab, tab_info + i, 0);

    tab = gtk_paned_get_child2(GTK_PANED(NgraphApp.Viewer.side_pane2));
    save_tab_position_sub(tab, tab_info + i, 100);
#endif
  }
}

static void
create_object_tabs(void)
{
  int j, tab_n;
  GtkWidget *tab;
  struct obj_tab_info tab_info[] = {
    {0, 0, &Menulocal.file_tab,      0, "data",      create_data_list,  &NgraphApp.FileWin,  NGRAPH_FILEWIN_ICON},
    {0, 0, &Menulocal.axis_tab,      0, "axis",      create_axis_list,  &NgraphApp.AxisWin,  NGRAPH_AXISWIN_ICON},
    {0, 0, &Menulocal.merge_tab,     0, "merge",     create_merge_list, &NgraphApp.MergeWin, NGRAPH_MERGEWIN_ICON},
    {0, 0, &Menulocal.path_tab,      0, "path",      create_path_list,  &NgraphApp.PathWin,  NGRAPH_LINE_ICON},
    {0, 0, &Menulocal.rectangle_tab, 0, "rectangle", create_rect_list,  &NgraphApp.RectWin,  NGRAPH_RECT_ICON},
    {0, 0, &Menulocal.arc_tab,       0, "arc",       create_arc_list,   &NgraphApp.ArcWin,   NGRAPH_ARC_ICON},
    {0, 0, &Menulocal.mark_tab,      0, "mark",      create_mark_list,  &NgraphApp.MarkWin,  NGRAPH_MARK_ICON},
    {0, 0, &Menulocal.text_tab,      0, "text",      create_text_list,  &NgraphApp.TextWin,  NGRAPH_TEXT_ICON},
    {0, 0, &Menulocal.parameter_tab, 0, "parameter", create_parameter_list,  &NgraphApp.ParameterWin,  NGRAPH_PARAMETER_ICON},
  };

  tab_n = sizeof(tab_info) / sizeof(*tab_info);
  init_tab_info(tab_info, tab_n);

  for (j = 0; j < tab_n; j++) {
#if GTK_CHECK_VERSION(4, 0, 0)
    if (tab_info[j].tab > 0) {
      tab = gtk_paned_get_end_child(GTK_PANED(NgraphApp.Viewer.side_pane2));
    } else {
      tab = gtk_paned_get_start_child(GTK_PANED(NgraphApp.Viewer.side_pane2));
    }
#else
    if (tab_info[j].tab > 0) {
      tab = gtk_paned_get_child2(GTK_PANED(NgraphApp.Viewer.side_pane2));
    } else {
      tab = gtk_paned_get_child1(GTK_PANED(NgraphApp.Viewer.side_pane2));
    }
#endif
    tab_info[j].init_func(tab_info[j].d);
    setup_object_tab(tab_info[j].d, tab, tab_info[j].icon, _(tab_info[j].obj_name));
  }

  CoordWinCreate(&NgraphApp.CoordWin);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_paned_set_start_child(GTK_PANED(NgraphApp.Viewer.side_pane3), NgraphApp.CoordWin.Win);
  InfoWinCreate(&NgraphApp.InfoWin);
  gtk_paned_set_end_child(GTK_PANED(NgraphApp.Viewer.side_pane3), NgraphApp.InfoWin.Win);
#else
  gtk_paned_pack1(GTK_PANED(NgraphApp.Viewer.side_pane3), NgraphApp.CoordWin.Win, FALSE, TRUE);
  InfoWinCreate(&NgraphApp.InfoWin);
  gtk_paned_pack2(GTK_PANED(NgraphApp.Viewer.side_pane3), NgraphApp.InfoWin.Win, TRUE, TRUE);
#endif

  set_pane_position();
  if (Menulocal.sidebar) {
    gtk_widget_show(NgraphApp.Viewer.side_pane1);
  }
}

static void
edit_menu_shown(GtkWidget *w, gpointer user_data)
{
  struct Viewer *d;

  d = (struct Viewer *) user_data;

  set_focus_sensitivity(d);
}

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
static void
clipboard_changed(GtkWidget *w, GdkEvent *e, gpointer user_data)
{
  struct Viewer *d;

  d = (struct Viewer *) user_data;

  set_focus_sensitivity(d);
}
#endif

#define USE_APP_HEADER_BAR 0
static void
setup_toolbar(GtkWidget *window)
{
  GtkWidget *w;
#if USE_APP_HEADER_BAR
  GtkWidget *hbar;
#endif
  w = create_toolbar(CommandToolbar, sizeof(CommandToolbar) / sizeof(*CommandToolbar), NULL);
  CToolbar = w;
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  gtk_toolbar_set_style(GTK_TOOLBAR(w), GTK_TOOLBAR_ICONS);
#endif

#if USE_APP_HEADER_BAR
  hbar = gtk_header_bar_new();
#if ! GTK_CHECK_VERSION(4, 0, 0)
  gtk_header_bar_set_title(GTK_HEADER_BAR(hbar), AppName);
#endif
  gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hbar), TRUE);
  gtk_window_set_titlebar(GTK_WINDOW(window), hbar);
#endif
  w = create_toolbar(PointerToolbar, sizeof(PointerToolbar) / sizeof(*PointerToolbar), G_CALLBACK(CmViewerButtonPressed));
  PToolbar = w;
  gtk_orientable_set_orientation(GTK_ORIENTABLE(w), GTK_ORIENTATION_VERTICAL);
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  gtk_toolbar_set_style(GTK_TOOLBAR(w), GTK_TOOLBAR_ICONS);
#endif
}

static void
setupwindow(GtkApplication *app)
{
  GtkWidget *w, *hbox, *hbox2, *vbox2, *table, *hpane1, *hpane2, *vpane1, *vpane2;
#if ! USE_APP_HEADER_BAR
  GtkWidget *vbox;

  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
#endif
  vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);

  create_menu(MenuAction);

  ToolBox = gtk_stack_new();
  SettingPanel = presetting_create_panel(app);
  gtk_stack_add_named(GTK_STACK(ToolBox), CToolbar, "CommandToolbar");
  gtk_stack_add_named(GTK_STACK(ToolBox), SettingPanel, "SettingPanel");
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox2), ToolBox);
  gtk_box_append(GTK_BOX(hbox), PToolbar);
#else
  gtk_box_pack_start(GTK_BOX(vbox2), ToolBox, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), PToolbar, FALSE, FALSE, 0);
#endif

  if (NgraphApp.Viewer.popup) {
    g_signal_connect(NgraphApp.Viewer.popup, "show", G_CALLBACK(edit_menu_shown), &NgraphApp.Viewer);
  }

  NgraphApp.Viewer.HScroll = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, NULL);
  NgraphApp.Viewer.VScroll = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, NULL);
  NgraphApp.Viewer.HRuler = nruler_new(GTK_ORIENTATION_HORIZONTAL);
  NgraphApp.Viewer.VRuler = nruler_new(GTK_ORIENTATION_VERTICAL);
  NgraphApp.Viewer.Win = gtk_drawing_area_new();

  table = gtk_grid_new();

  gtk_widget_set_hexpand(NgraphApp.Viewer.HRuler, TRUE);
  gtk_grid_attach(GTK_GRID(table), NgraphApp.Viewer.HRuler,  1, 0, 1, 1);

  gtk_widget_set_vexpand(NgraphApp.Viewer.VRuler, TRUE);
  gtk_grid_attach(GTK_GRID(table), NgraphApp.Viewer.VRuler,  0, 1, 1, 1);

  gtk_widget_set_hexpand(NgraphApp.Viewer.HScroll, TRUE);
  gtk_grid_attach(GTK_GRID(table), NgraphApp.Viewer.HScroll, 1, 2, 1, 1);

  gtk_widget_set_vexpand(NgraphApp.Viewer.VScroll, TRUE);
  gtk_grid_attach(GTK_GRID(table), NgraphApp.Viewer.VScroll, 2, 1, 1, 1);

  gtk_widget_set_hexpand(NgraphApp.Viewer.Win, TRUE);
  gtk_widget_set_vexpand(NgraphApp.Viewer.Win, TRUE);
  gtk_grid_attach(GTK_GRID(table), NgraphApp.Viewer.Win,     1, 1, 1, 1);

  vpane2 = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  NgraphApp.Viewer.side_pane2 = vpane2;

  w = gtk_notebook_new();
  gtk_notebook_popup_enable(GTK_NOTEBOOK(w));
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w), GTK_POS_LEFT);
  gtk_notebook_set_group_name(GTK_NOTEBOOK(w), SIDE_PANE_TAB_ID);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(w), TRUE);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_paned_set_start_child(GTK_PANED(vpane2), w);
#else
  gtk_paned_add1(GTK_PANED(vpane2), w);
#endif

  w = gtk_notebook_new();
  gtk_notebook_popup_enable(GTK_NOTEBOOK(w));
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w), GTK_POS_LEFT);
  gtk_notebook_set_group_name(GTK_NOTEBOOK(w), SIDE_PANE_TAB_ID);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(w), TRUE);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_paned_set_end_child(GTK_PANED(vpane2), w);
#else
  gtk_paned_add2(GTK_PANED(vpane2), w);
#endif

  hpane2 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  NgraphApp.Viewer.side_pane3 = hpane2;

  vpane1 = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_paned_set_start_child(GTK_PANED(vpane1), vpane2);
  gtk_paned_set_end_child(GTK_PANED(vpane1), hpane2);
#else
  gtk_paned_pack1(GTK_PANED(vpane1), vpane2, TRUE, TRUE);
  gtk_paned_pack2(GTK_PANED(vpane1), hpane2, FALSE, TRUE);
#endif
  NgraphApp.Viewer.side_pane1 = vpane1;

  hpane1 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
#if GTK_CHECK_VERSION(4, 0, 0)
#if USE_APP_HEADER_BAR
  gtk_paned_set_start_child(GTK_PANED(hpane1), hbox);
#else
  gtk_paned_set_start_child(GTK_PANED(hpane1), vbox);
#endif
  gtk_paned_set_end_child(GTK_PANED(hpane1), vpane1);
#else
#if USE_APP_HEADER_BAR
  gtk_paned_add1(GTK_PANED(hpane1), hbox);
#else
  gtk_paned_add1(GTK_PANED(hpane1), vbox);
#endif
  gtk_paned_add2(GTK_PANED(hpane1), vpane1);
#endif
  NgraphApp.Viewer.main_pane = hpane1;

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox), table);
#else
  gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);
#endif
#if ! USE_APP_HEADER_BAR
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox), hbox);
#else
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
#endif
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(hbox2), hpane1);
  gtk_box_append(GTK_BOX(vbox2), hbox2);
#else
  gtk_box_pack_start(GTK_BOX(hbox2), hpane1, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox2), hbox2, TRUE, TRUE, 0);
#endif

  NgraphApp.Message = gtk_statusbar_new();
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
//  gtk_box_append(GTK_BOX(NgraphApp.Message), create_message_box(&NgraphApp.Message_extra, &NgraphApp.Message_pos));
#else
  gtk_box_pack_end(GTK_BOX(NgraphApp.Message),
		   create_message_box(&NgraphApp.Message_extra, &NgraphApp.Message_pos),
		   FALSE, FALSE, 0);
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_box_append(GTK_BOX(vbox2), NgraphApp.Message);
#else
  gtk_box_pack_start(GTK_BOX(vbox2), NgraphApp.Message, FALSE, FALSE, 0);
#endif

  NgraphApp.Message1 = gtk_statusbar_get_context_id(GTK_STATUSBAR(NgraphApp.Message), "Message1");

  set_axis_undo_button_sensitivity(FALSE);

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_window_set_child(GTK_WINDOW(TopLevel), vbox2);
#else
  gtk_container_add(GTK_CONTAINER(TopLevel), vbox2);
#endif

  create_object_tabs();
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
  NgraphApp.fit_list = entry_completion_create();

  home = get_home();
  if (home == NULL)
    return;

  load_hist_file(NgraphApp.legend_text_list, home, TEXT_HISTORY);
  load_hist_file(NgraphApp.x_math_list, home, MATH_X_HISTORY);
  load_hist_file(NgraphApp.y_math_list, home, MATH_Y_HISTORY);
  load_hist_file(NgraphApp.func_list, home, FUNCTION_HISTORY);
  load_hist_file(NgraphApp.fit_list, home, FIT_HISTORY);
}

static void
unref_entry_history(void)
{
  g_object_unref(NgraphApp.legend_text_list);
  g_object_unref(NgraphApp.x_math_list);
  g_object_unref(NgraphApp.y_math_list);
  g_object_unref(NgraphApp.func_list);
  g_object_unref(NgraphApp.fit_list);

  NgraphApp.legend_text_list = NULL;
  NgraphApp.x_math_list = NULL;
  NgraphApp.y_math_list = NULL;
  NgraphApp.func_list = NULL;
  NgraphApp.fit_list = NULL;
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
  save_hist_file(NgraphApp.fit_list, home, FIT_HISTORY);
}

static void
init_ngraph_app_struct(void)
{
  NgraphApp.Viewer.Win = NULL;
  NgraphApp.Viewer.popup = NULL;

  memset(&NgraphApp.FileWin, 0, sizeof(NgraphApp.FileWin));
  NgraphApp.FileWin.type = TypeFileWin;

  memset(&NgraphApp.AxisWin, 0, sizeof(NgraphApp.AxisWin));
  NgraphApp.AxisWin.type = TypeAxisWin;

  memset(&NgraphApp.PathWin, 0, sizeof(NgraphApp.PathWin));
  NgraphApp.PathWin.type = TypePathWin;

  memset(&NgraphApp.RectWin, 0, sizeof(NgraphApp.RectWin));
  NgraphApp.RectWin.type = TypeRectWin;

  memset(&NgraphApp.ArcWin, 0, sizeof(NgraphApp.ArcWin));
  NgraphApp.ArcWin.type = TypeArcWin;

  memset(&NgraphApp.MarkWin, 0, sizeof(NgraphApp.MarkWin));
  NgraphApp.MarkWin.type = TypeMarkWin;

  memset(&NgraphApp.TextWin, 0, sizeof(NgraphApp.TextWin));
  NgraphApp.TextWin.type = TypeTextWin;

  memset(&NgraphApp.MergeWin, 0, sizeof(NgraphApp.MergeWin));
  NgraphApp.MergeWin.type = TypeMergeWin;

  memset(&NgraphApp.ParameterWin, 0, sizeof(NgraphApp.ParameterWin));
  NgraphApp.ParameterWin.type = TypeParameterWin;

  memset(&NgraphApp.InfoWin, 0, sizeof(NgraphApp.InfoWin));
  NgraphApp.InfoWin.type = TypeInfoWin;

  memset(&NgraphApp.CoordWin, 0, sizeof(NgraphApp.CoordWin));
  NgraphApp.CoordWin.type = TypeCoordWin;

  NgraphApp.legend_text_list = NULL;
  NgraphApp.x_math_list = NULL;
  NgraphApp.y_math_list = NULL;
  NgraphApp.func_list = NULL;
  NgraphApp.fit_list = NULL;
}

void
set_modified_state(int state)
{
  set_action_widget_sensitivity(GraphSaveAction, state);
}

int
toggle_view(int type, int state)
{
  static int lock = FALSE;
  GtkWidget *w1 = NULL, *w2 = NULL;

  if (Menulock || Globallock) {
    return FALSE;
  }

  if (lock) {
    return FALSE;
  }

  lock = TRUE;

  switch (type) {
  case MenuIdToggleSidebar:
    Menulocal.sidebar = state;
    w1 = NgraphApp.Viewer.side_pane1;
    break;
  case MenuIdToggleStatusbar:
    Menulocal.statusbar = state;
    w1 = NgraphApp.Message;
   break;
  case MenuIdToggleRuler:
    Menulocal.ruler = state;
    w1 = NgraphApp.Viewer.HRuler;
    w2 = NgraphApp.Viewer.VRuler;
   break;
  case MenuIdToggleScrollbar:
    Menulocal.scrollbar = state;
    w1 = NgraphApp.Viewer.HScroll;
    w2 = NgraphApp.Viewer.VScroll;
    break;
  case MenuIdToggleCToolbar:
    Menulocal.ctoolbar = state;
    w1 = ToolBox;
    break;
  case MenuIdTogglePToolbar:
    Menulocal.ptoolbar = state;
    w1 = PToolbar;
    break;
  case MenuIdToggleCrossGauge:
    ViewCross(state);
    set_toggle_action_widget_state(ViewCrossGaugeAction, state);
    lock = FALSE;
    return TRUE;
    break;
  case MenuIdToggleGridLine:
    Menulocal.show_grid = state;
    set_toggle_action_widget_state(ViewGridLineAction, state);
    update_bg();
    gtk_widget_queue_draw(NgraphApp.Viewer.Win);
    lock = FALSE;
    return TRUE;
    break;
  }

  if (w1) {
    gtk_widget_set_visible(w1, state);
  }
  if (w2) {
    gtk_widget_set_visible(w2, state);
  }

  lock = FALSE;

  return TRUE;
}

void
set_toggle_action_widget_state(int id, int state)
{
  if (ActionWidget[id].action) {
    GVariant *value;

    value = g_variant_new_boolean(state);
    g_action_change_state(ActionWidget[id].action, value);
  }
}

static void
set_widget_visibility(void)
{
  int i, state;

  for (i = 0; i < ActionWidgetNum; i++) {
    switch (i) {
    case ViewSidebarAction:
      state = Menulocal.sidebar;
      break;
    case ViewStatusbarAction:
      state = Menulocal.statusbar;
      break;
    case ViewRulerAction:
      state = Menulocal.ruler;
      break;
    case ViewScrollbarAction:
      state = Menulocal.scrollbar;
      break;
    case ViewCommandToolbarAction:
      state = Menulocal.ctoolbar;
      break;
    case ViewToolboxAction:
      state = Menulocal.ptoolbar;
      break;
    case ViewCrossGaugeAction:
      state = Menulocal.show_cross;
      break;
    case ViewGridLineAction:
      state = Menulocal.show_grid;
      break;
    default:
      continue;
    }
    set_toggle_action_widget_state(i, state);
  }
}

static void
check_instance(struct objlist *obj)
{
  int i;

  for (i = 0; i < ActionWidgetNum; i++) {
    struct objlist *dobj;
    dobj = NULL;
    switch (i) {
    case DataPropertyAction:
    case DataCloseAction:
    case DataEditAction:
    case DataSaveAction:
    case DataMathAction:
      dobj = chkobject("data");
      break;
    case ParameterPropertyAction:
    case ParameterDeleteAction:
      dobj = chkobject("parameter");
      break;
    case AxisPropertyAction:
    case AxisDeleteAction:
    case AxisScaleZoomAction:
    case AxisScaleClearAction:
      dobj = chkobject("axis");
      if (obj == dobj && obj->lastinst < 0) {
	set_axis_undo_button_sensitivity(FALSE);
      }
      break;
    case AxisGridPropertyAction:
    case AxisGridDeleteAction:
      dobj = chkobject("axisgrid");
      break;
    case LegendPathPropertyAction:
    case LegendPathDeleteAction:
      dobj = chkobject("path");
      break;
    case LegendRectanglePropertyAction:
    case LegendRectangleDeleteAction:
      dobj = chkobject("rectangle");
      break;
    case LegendArcPropertyAction:
    case LegendArcDeleteAction:
      dobj = chkobject("arc");
      break;
    case LegendMarkPropertyAction:
    case LegendMarkDeleteAction:
      dobj = chkobject("mark");
      break;
    case LegendTextPropertyAction:
    case LegendTextDeleteAction:
      dobj = chkobject("text");
      break;
    case MergePropertyAction:
    case MergeCloseAction:
      dobj = chkobject("merge");
      break;
    default:
      continue;
    }

    if (obj == dobj) {
      set_action_widget_sensitivity(i, obj->lastinst >= 0);
    }
  }
}

static void
check_exist_instances(struct objlist *parent)
{
  struct objlist *ocur;

  ocur = chkobjroot();
  while (ocur) {
    if (chkobjparent(ocur) == parent) {
      check_instance(ocur);
      check_exist_instances(ocur);
    }
    ocur = ocur->next;
  }
}

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
static gboolean
recent_filter(const GtkRecentFilterInfo *filter_info, gpointer user_data)
{
  int i;

  if (filter_info->mime_type == NULL) {
    return FALSE;
  }

  switch (GPOINTER_TO_INT(user_data)) {
  case RECENT_TYPE_GRAPH:
    if (g_ascii_strcasecmp(filter_info->mime_type, NGRAPH_GRAPH_MIME)) {
      return FALSE;
    }
    break;
  case RECENT_TYPE_DATA:
    if (g_ascii_strncasecmp(filter_info->mime_type, NGRAPH_TEXT_MIME,
			    strlen(NGRAPH_TEXT_MIME))) {
      return FALSE;
    }
    break;
  default:
    return FALSE;
  }

  for (i = 0; filter_info->applications[i]; i++) {
    if (g_strcmp0(AppName, filter_info->applications[i]) == 0) {
      return TRUE;
    }
  }

  return FALSE;
}

static void
create_recent_filter(GtkWidget *w, int type)
{
  GtkRecentFilter *filter;
  GtkRecentChooser *recent;

  recent = GTK_RECENT_CHOOSER(w);

  filter = gtk_recent_filter_new();
  gtk_recent_filter_set_name(filter,
			     (type == RECENT_TYPE_GRAPH) ? "NGP file" : "Data file");
  gtk_recent_filter_add_custom(filter,
			       GTK_RECENT_FILTER_URI |
			       GTK_RECENT_FILTER_MIME_TYPE |
			       GTK_RECENT_FILTER_APPLICATION,
			       recent_filter,
			       GINT_TO_POINTER(type),
			       NULL);

  gtk_recent_chooser_set_filter(recent, filter);
  gtk_recent_chooser_set_show_tips(recent, TRUE);
  gtk_recent_chooser_set_show_icons(recent, FALSE);
  gtk_recent_chooser_set_local_only(recent, TRUE);
#if ! WINDOWS
  gtk_recent_chooser_set_show_not_found(recent, FALSE);
#endif
  gtk_recent_chooser_set_sort_type(recent, GTK_RECENT_SORT_MRU);
}
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
GtkWidget *
create_recent_menu(int type)
{
  return NULL;
}
#else
GtkWidget *
create_recent_menu(int type)
{
  GtkWidget *submenu;

  submenu = gtk_recent_chooser_menu_new_for_manager(NgraphApp.recent_manager);
  create_recent_filter(submenu, type);
  gtk_recent_chooser_menu_set_show_numbers(GTK_RECENT_CHOOSER_MENU(submenu), TRUE);
  gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(submenu), RECENT_CHOOSER_LIMIT);

  switch (type) {
  case RECENT_TYPE_GRAPH:
    g_signal_connect(GTK_RECENT_CHOOSER(submenu), "item-activated", G_CALLBACK(CmGraphHistory), NULL);
    break;
  case RECENT_TYPE_DATA:
    g_signal_connect(GTK_RECENT_CHOOSER(submenu), "item-activated", G_CALLBACK(CmFileHistory), NULL);
    break;
  }

  return submenu;
}
#endif

static GtkWidget*
create_save_menu(void)
{
  GtkWidget *menu;
  GMenu *gmenu;

  gmenu = gtk_application_get_menu_by_id(GtkApp, "save-menu");
  if (gmenu == NULL) {
    return NULL;
  }
#if GTK_CHECK_VERSION(4, 0, 0)
  menu = gtk_popover_menu_new_from_model_full(G_MENU_MODEL(gmenu), GTK_POPOVER_MENU_NESTED);
#else
  menu = gtk_menu_new_from_model(G_MENU_MODEL(gmenu));
  gtk_widget_show_all(menu);
#endif
  return menu;
}

void
draw_notify(int notify)
{
  static int state = FALSE;
  const char *icon_name;
  if (state == notify) {
    return;
  }
  state = notify;
  if (DrawButton == NULL) {
    return;
  }
  icon_name = (state) ? "ngraph_draw-attention-symbolic" : "ngraph_draw-symbolic";
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_button_set_icon_name(GTK_BUTTON(DrawButton), icon_name);
#else
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(DrawButton), icon_name);
#endif
  gtk_widget_queue_draw(DrawButton);
}

static GtkWidget *
create_toolbar(struct ToolItem *item, int n, GCallback btn_press_cb)
{
  int i;
#if GTK_CHECK_VERSION(4, 0, 0)
  GtkWidget *toolbar,  *widget, *group = NULL, *menu;
#else
  GSList *list = NULL;
  GtkToolItem *widget;
  GtkWidget *toolbar, *menu;
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
  toolbar = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
  set_widget_margin_all(toolbar, 4);
#else
  toolbar = gtk_toolbar_new();
#endif
  for (i = 0; i < n; i++) {
#if GTK_CHECK_VERSION(4, 0, 0)
    menu = NULL;
#endif
    switch (item[i].type) {
    case TOOL_TYPE_SEPARATOR:
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
      widget = gtk_frame_new(NULL);
#else
      widget = gtk_separator_tool_item_new();
      gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(widget), TRUE);
#endif
      break;
    case TOOL_TYPE_NORMAL:
#if GTK_CHECK_VERSION(4, 0, 0)
      widget = gtk_button_new_from_icon_name(item[i].icon);
#else
      widget = gtk_tool_button_new(NULL, _(item[i].label));
#endif
      break;
    case TOOL_TYPE_DRAW:
#if GTK_CHECK_VERSION(4, 0, 0)
      widget = gtk_button_new_from_icon_name(item[i].icon);
#else
      widget = gtk_tool_button_new(NULL, _(item[i].label));
#endif
      DrawButton = GTK_WIDGET(widget);
      break;
    case TOOL_TYPE_SAVE:
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
      widget = gtk_button_new_from_icon_name(item[i].icon);
      menu = gtk_menu_button_new();
      gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu), create_save_menu());
      gtk_widget_set_tooltip_text(menu, _("Save menu"));
#else
      widget = gtk_menu_tool_button_new(NULL, _(item[i].label));
      menu = create_save_menu();
      gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(widget), menu);
      gtk_menu_tool_button_set_arrow_tooltip_text(GTK_MENU_TOOL_BUTTON(widget), _("Save menu"));
#endif
      break;
    case TOOL_TYPE_RECENT_GRAPH:
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
      widget = gtk_button_new_from_icon_name(item[i].icon);
      menu = gtk_menu_button_new();
      gtk_widget_set_tooltip_text(menu, _("Recent Graphs"));
#else
      widget = gtk_menu_tool_button_new(NULL, _(item[i].label));
      menu = create_recent_menu(RECENT_TYPE_GRAPH);
      gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(widget), menu);
      gtk_menu_tool_button_set_arrow_tooltip_text(GTK_MENU_TOOL_BUTTON(widget), _("Recent Graphs"));
#endif
      break;
    case TOOL_TYPE_RECENT_DATA:
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
      widget = gtk_button_new_from_icon_name(item[i].icon);
      menu = gtk_menu_button_new();
      gtk_widget_set_tooltip_text(menu, _("Recent Data Files"));
#else
      widget = gtk_menu_tool_button_new(NULL, _(item[i].label));
      menu = create_recent_menu(RECENT_TYPE_DATA);
      gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(widget), menu);
      gtk_menu_tool_button_set_arrow_tooltip_text(GTK_MENU_TOOL_BUTTON(widget), _("Recent Data Files"));
#endif
      break;
    case TOOL_TYPE_RADIO:
#if GTK_CHECK_VERSION(4, 0, 0)
      widget = gtk_toggle_button_new();
      gtk_button_set_icon_name(GTK_BUTTON(widget), item[i].icon);
      if (group) {
        gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(widget), GTK_TOGGLE_BUTTON(group));
      } else {
        group = widget;
      }
      gtk_widget_set_tooltip_text(widget, _(item[i].label));
#else
      widget = gtk_radio_tool_button_new(list);
      list = gtk_radio_tool_button_get_group(GTK_RADIO_TOOL_BUTTON(widget));
      gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _(item[i].label));
#endif
      if (btn_press_cb) {
#if GTK_CHECK_VERSION(4, 0, 0)
	GtkGesture *gesture;

	gesture = gtk_gesture_click_new();
	gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(gesture));

	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
	g_signal_connect(gesture, "pressed", btn_press_cb, NULL);
#else
	g_signal_connect(gtk_bin_get_child(GTK_BIN(widget)), "button-press-event", btn_press_cb, NULL);
#endif
      }
      break;
    default:
      widget = NULL;
    }

    if (widget == NULL) {
      continue;
    }

#if ! GTK_CHECK_VERSION(4, 0, 0)
    if (item[i].icon) {
      gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), item[i].icon);
    }
#endif

    if (item[i].action_name) {
      gtk_actionable_set_action_name(GTK_ACTIONABLE(widget), item[i].action_name);
    } else if (item[i].callback) {
      g_signal_connect(widget, "clicked", G_CALLBACK(item[i].callback), GINT_TO_POINTER(item[i].user_data));
    }

#if ! GTK_CHECK_VERSION(4, 0, 0)
    if (item[i].tip) {
      gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _(item[i].tip));
    }
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
    if (item[i].caption) {
      g_signal_connect(gtk_bin_get_child(GTK_BIN(widget)),
		       "enter-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb),
		       (gpointer) _(item[i].caption));

      g_signal_connect(gtk_bin_get_child(GTK_BIN(widget)),
		       "leave-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb), NULL);
    }
#endif

    if (item[i].button >= PointerModeOffset) {
      int id;
      id = item[i].button - PointerModeOffset;
      PointerModeButtons[id]= widget;
    }

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_widget_set_can_focus(widget, FALSE);
    if (menu) {
      GtkWidget *box;
      box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
      gtk_box_append(GTK_BOX(box), widget);
      gtk_widget_add_css_class(widget, MENUBUTTON_CLASS);
      gtk_box_append(GTK_BOX(box), menu);
      gtk_widget_add_css_class(menu, MENUBUTTON_CLASS);
      gtk_box_append(GTK_BOX(toolbar), box);
    } else {
      gtk_widget_add_css_class(widget, TOOLBUTTON_CLASS);
      gtk_box_append(GTK_BOX(toolbar), widget);
    }
#else
    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(widget), -1);
#endif
  }

  return toolbar;
}

static void
create_menu(struct MenuItem *item)
{
  int i;

  for (i = 0; item[i].action_name; i++) {
    if (item[i].action) {
      item[i].action->action = g_action_map_lookup_action(G_ACTION_MAP(GtkApp), item[i].action_name);
    }
  }
}

static GtkWidget *
create_popup_menu(GtkApplication *app)
{
  GtkWidget *popup;
  GMenu *menu;
  menu = gtk_application_get_menu_by_id(app, "popup-menu");
#if GTK_CHECK_VERSION(4, 0, 0)
  popup = gtk_popover_menu_new_from_model_full(G_MENU_MODEL(menu), GTK_POPOVER_MENU_NESTED);
  gtk_popover_set_has_arrow(GTK_POPOVER(popup), FALSE);
#else
  popup = gtk_menu_new_from_model(G_MENU_MODEL(menu));
#endif
  return popup;
}

static int
create_toplevel_window(void)
{
  int i;
  struct objlist *aobj;
  int x, y, width, height, w, h;
  GdkDisplay *disp;
  GtkWidget *popup;
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  GtkClipboard *clip;
#endif

  NgraphApp.recent_manager = gtk_recent_manager_get_default();

  init_action_widget_list();
  init_ngraph_app_struct();

  w = 800;
  h = 600;
  disp = gdk_display_get_default();
  if (disp) {
    GdkMonitor *monitor;

#if GTK_CHECK_VERSION(4, 0, 0)
    GdkSurface *surface;
    surface = gdk_surface_new_toplevel(disp);
    monitor = gdk_display_get_monitor_at_surface(disp, surface);
    /* g_object_unref(surface); */
#else
    monitor = gdk_display_get_primary_monitor(disp);
#endif
    if (monitor) {
      GdkRectangle rect;

      gdk_monitor_get_geometry(monitor, &rect);
      w = rect.width;
      h = rect.height;
    }
  }

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

  CurrentWindow = TopLevel = gtk_application_window_new(GtkApp);
  gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(TopLevel), TRUE);
  popup = create_popup_menu(GtkApp);
  if (popup) {
    NgraphApp.Viewer.popup = popup;
  }
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  g_signal_connect(clip, "owner-change", G_CALLBACK(clipboard_changed), &NgraphApp.Viewer);
#endif

  gtk_window_set_title(GTK_WINDOW(TopLevel), AppName);
  gtk_window_set_default_size(GTK_WINDOW(TopLevel), width, height);
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  gtk_window_move(GTK_WINDOW(TopLevel), x, y);
#endif

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
  g_signal_connect(TopLevel, "close_request", G_CALLBACK(CloseCallback), NULL);
#else
  g_signal_connect(TopLevel, "delete-event", G_CALLBACK(CloseCallback), NULL);
  g_signal_connect(TopLevel, "destroy-event", G_CALLBACK(CloseCallback), NULL);
#endif

  create_icon();
  initdialog();

  setup_toolbar(TopLevel);
#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(GTK_WIDGET(TopLevel));
#else
  gtk_widget_show_all(GTK_WIDGET(TopLevel));
#endif
  reset_event();
  create_markpixmap(TopLevel);
  setupwindow(GtkApp);
  create_addin_menu();

  NgraphApp.FileName = NULL;
  NgraphApp.Viewer.Mode = PointB;

#if GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show(GTK_WIDGET(TopLevel));
#else
  gtk_widget_show_all(GTK_WIDGET(TopLevel));
#endif
  ViewerWinSetup();

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

  aobj = getobject("axis");
  if (aobj) {
    for (i = 0; i <= chkobjlastinst(aobj); i++) {
      exeobj(aobj, "tight", i, 0, NULL);
    }
  }
  aobj = getobject("axisgrid");
  if (aobj) {
    for (i = 0; i <= chkobjlastinst(aobj); i++) {
      exeobj(aobj, "tight", i, 0, NULL);
    }
  }

#if ! GTK_CHECK_VERSION(4, 0, 0)
  gtk_widget_show_all(GTK_WIDGET(TopLevel));
#endif
  set_widget_visibility();

  set_focus_sensitivity(&NgraphApp.Viewer);
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
#else
  check_exist_instances(chkobject("draw"));
#endif
  check_instance(chkobject("parameter"));

  set_newobj_cb(check_instance);
  set_delobj_cb(check_instance);

  return 0;
}

static void
souce_view_set_search_path(void)
{
  const gchar * const *dirs;
#if GTK_CHECK_VERSION(4, 0, 0)
  const char **new_dirs;
#else
  gchar **new_dirs;
#endif
  gchar *dir;
  int n;
  GtkSourceLanguageManager *lm;

  lm = gtk_source_language_manager_get_default();

  dirs = gtk_source_language_manager_get_search_path(lm);
  dir = g_strdup_printf("%s/%s", NDATADIR, "gtksourceview");
  if (dir == NULL) {
    return;
  }
  if (g_strv_contains(dirs, dir)) {
    g_free(dir);
    return;
  }

  for (n = 0; dirs[n]; n++);
  new_dirs = g_malloc((n + 2) * sizeof(*new_dirs));
  if (new_dirs == NULL) {
    g_free(dir);
    return;
  }

  memcpy(new_dirs, dirs, n * sizeof(*new_dirs));
  new_dirs[n] = dir;
  new_dirs[n + 1] = NULL;
  gtk_source_language_manager_set_search_path(lm, new_dirs);
  g_free(dir);
  g_free(new_dirs);
}

int
application(char *file)
{
  int terminated;

  if (TopLevel) {
    if (gtk_widget_is_visible(TopLevel)) {
      return 1;
    }
    gtk_widget_show(TopLevel);
    OpenGC();
    OpenGRA();
  } else {
    GtkIconTheme *theme;
#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
    GdkDisplay* display;
    display = gdk_display_get_default();
    theme = gtk_icon_theme_get_for_display(display);
#else
    theme = gtk_icon_theme_get_default();
#endif
    gtk_icon_theme_add_resource_path(theme, NGRAPH_ICON_PATH);
    if (create_toplevel_window()) {
      return 1;
    }
  }

  souce_view_set_search_path();

#if ! WINDOWS
  set_signal(SIGINT, 0, kill_signal_handler, NULL);
  set_signal(SIGTERM, 0, term_signal_handler, NULL);
#endif	/* WINDOWS */

  if (file != NULL) {
    char *ext;
    ext = getextention(file);
    if (ext && ((strcmp0(ext, "NGP") == 0) || (strcmp0(ext, "ngp") == 0))) {
      LoadNgpFile(file, FALSE, NULL);
    } else {
      CmViewerDraw(NULL, GINT_TO_POINTER(FALSE));
    }
  } else {
    CmViewerDraw(NULL, GINT_TO_POINTER(FALSE));
  }

  system_set_draw_notify_func(draw_notify);
  reset_event();                /* to set pane position correctly */
  set_pane_position();          /* to set pane position correctly */
  n_application_ready();
  terminated = AppMainLoop();
  system_set_draw_notify_func(NULL);

  if (CheckIniFile()) {
    save_tab_position();
    get_pane_position();
    menu_save_config(SAVE_CONFIG_TYPE_GEOMETRY);
    save_entry_history();
    menu_save_config(SAVE_CONFIG_TYPE_TOGGLE_VIEW |
		     SAVE_CONFIG_TYPE_OTHERS);
  }

  set_newobj_cb(NULL);
  set_delobj_cb(NULL);

#if ! WINDOWS
  set_signal(SIGTERM, 0, SIG_DFL, NULL);
  set_signal(SIGINT, 0, SIG_DFL, NULL);
#endif	/* WINDOWS */

  gtk_widget_hide(TopLevel);
  reset_event();

  CloseGC();
  CloseGRA();

  if (terminated) {
    unref_entry_history();

    ViewerWinClose();

    g_free(NgraphApp.FileName);
    NgraphApp.FileName = NULL;

#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_window_destroy(GTK_WINDOW(TopLevel));
#else
    gtk_widget_destroy(TopLevel);
#endif
    NgraphApp.Viewer.Win = NULL;
    CurrentWindow = TopLevel = PToolbar = CToolbar = ToolBox = NULL;

    free_markpixmap();
    free_cursor();

    reset_event();
    delobj(getobject("system"), 0);
  }

  return 0;
}

void
UpdateAll(char **objects)
{
  UpdateAll2(objects, TRUE);
}

static void
check_update_obj(char **objects,
		 struct obj_list_data *file, int *update_file,
		 struct obj_list_data *axis, int *update_axis,
		 struct obj_list_data *merge, int *update_merge,
		 int *update_axisgrid)
{
  char **ptr;

  if (objects == NULL) {
    *update_file = TRUE;
    *update_axis = TRUE;
    *update_merge = TRUE;
    return;
  }

  *update_file = FALSE;
  *update_axis = FALSE;
  *update_merge = FALSE;

  for (ptr = objects; *ptr; ptr++) {
    struct objlist *obj;
    obj = getobject(*ptr);
    if (obj == file->obj) {
      *update_file = TRUE;
    } else if (obj == axis->obj) {
      *update_axis = TRUE;
    } else if (obj == merge->obj) {
      *update_merge = TRUE;
    } else if (strcmp0(*ptr, "axisgrid") == 0) {
      *update_axisgrid = TRUE;
    }
  }
}

void
UpdateAll2(char **objs, int redraw)
{
  int update_axisgrid, update_axis, update_file, update_merge;

  check_update_obj(objs,
		   NgraphApp.FileWin.data.data, &update_file,
		   NgraphApp.AxisWin.data.data, &update_axis,
		   NgraphApp.MergeWin.data.data, &update_merge,
		   &update_axisgrid);
  if (update_file) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, redraw && ! update_axis);
  }
  if (update_axis) {
    AxisWinUpdate(NgraphApp.AxisWin.data.data, TRUE, redraw);
  } else if (update_axisgrid) {
    update_viewer_axisgrid();
  }
  if (update_merge) {
    MergeWinUpdate(NgraphApp.MergeWin.data.data, TRUE, redraw);
  }
  LegendWinUpdate(objs, TRUE, redraw);
  InfoWinUpdate(TRUE);
  CoordWinUpdate(TRUE);
  ParameterWinUpdate(NgraphApp.ParameterWin.data.data, FALSE, redraw);
  presetting_set_parameters(&NgraphApp.Viewer);
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
  ChangeDPI();
  draw_notify(TRUE);
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
#if ! GTK_CHECK_VERSION(4, 0, 0)
  GdkWindow *win;
#endif
  GdkCursor *cursor;

  if (NgraphApp.Viewer.Win == NULL || NgraphApp.cursor == NULL || CursorType == type)
    return;

#if ! GTK_CHECK_VERSION(4, 0, 0)
  win = gtk_widget_get_window(NgraphApp.Viewer.Win);
  if (win == NULL) {
    return;
  }
#endif

  CursorType = type;
  cursor = NULL;

  switch (type) {
  case GDK_LEFT_PTR:
    cursor = NgraphApp.cursor[0];
    break;
  case GDK_XTERM:
    cursor = NgraphApp.cursor[1];
    break;
  case GDK_CROSSHAIR:
    cursor = NgraphApp.cursor[2];
    break;
  case GDK_TOP_LEFT_CORNER:
    cursor = NgraphApp.cursor[3];
    break;
  case GDK_TOP_RIGHT_CORNER:
    cursor = NgraphApp.cursor[4];
    break;
  case GDK_BOTTOM_RIGHT_CORNER:
    cursor = NgraphApp.cursor[5];
    break;
  case GDK_BOTTOM_LEFT_CORNER:
    cursor = NgraphApp.cursor[6];
    break;
  case GDK_TARGET:
    cursor = NgraphApp.cursor[7];
    break;
  case GDK_PLUS:
    cursor = NgraphApp.cursor[8];
    break;
  case GDK_SIZING:
    cursor = NgraphApp.cursor[9];
    break;
  case GDK_WATCH:
    cursor = NgraphApp.cursor[10];
    break;
  case GDK_FLEUR:
    cursor = NgraphApp.cursor[11];
    break;
  case GDK_PENCIL:
    cursor = NgraphApp.cursor[12];
    break;
  case GDK_TCROSS:
    cursor = NgraphApp.cursor[13];
    break;
  }
  if (cursor) {
#if GTK_CHECK_VERSION(4, 0, 0)
    gtk_widget_set_cursor(NgraphApp.Viewer.Win, cursor);
#else
    gdk_window_set_cursor(win, cursor);
#endif
  }
}

void
DisplayDialog(const char *str)
{
  char *ustr;

  if (str == NULL)
    return;

  ustr = n_locale_to_utf8(CHK_STR(str));
  if (ustr == NULL) {
    return;
  }
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
  size_t len;
  char *ustr;

  if (s == NULL)
    return 0;

  ustr = n_locale_to_utf8(s);
  if (ustr == NULL) {
    return 0;
  }
  message_box(NULL, ustr, _("Error:"), RESPONS_ERROR);
  len = strlen(ustr);
  g_free(ustr);
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
  if (check_interrupt()) {
    return TRUE;
  }
#endif
#if GTK_CHECK_VERSION(4, 0, 0)
  GMainContext *context;
  context = g_main_context_default();
#endif
  if (DrawLock != DrawLockDraw) {
    return check_interrupt();
  }

#if GTK_CHECK_VERSION(4, 0, 0)
  while (g_main_context_pending(context)) {
    g_main_context_iteration(context, TRUE);
    if (check_interrupt()) {
      return TRUE;
    }
  }
#else
  while (gtk_events_pending()) {
    gtk_main_iteration_do(FALSE);
    if (check_interrupt()) {
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

  ret = message_box(get_current_window(), mes, _("Question"), RESPONS_YESNO);
  return (ret == IDYES) ? TRUE : FALSE;
}

void
script_exec(GtkWidget *w, gpointer client_data)
{
  char *name, *option, *s, *argv[2], mes[256];
  int newid, allocnow = FALSE, len, idn;
  struct narray sarray;
  struct objlist *robj, *shell;
  struct script *fcur;

  if (Menulock || Globallock || client_data == NULL) {
    return;
  }

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

  menu_save_undo(UNDO_TYPE_ADDIN, NULL);
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
  UpdateAll(NULL);

  delobj(shell, newid);
  main_window_redraw();
}

static void
#if GTK_CHECK_VERSION(4, 0, 0)
CmViewerButtonArm(GtkWidget *action, gpointer client_data)
#else
CmViewerButtonArm(GtkToggleToolButton *action, gpointer client_data)
#endif
{
  int mode = PointB;
  struct Viewer *d;

  d = &NgraphApp.Viewer;

#if GTK_CHECK_VERSION(4, 0, 0)
  if (! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(action))) {
    return;
  }
#else
  if (! gtk_toggle_tool_button_get_active(action)) {
    return;
  }
#endif

  mode = GPOINTER_TO_INT(client_data);

  UnFocus();

  NgraphApp.Viewer.Mode = mode;
  switch (mode) {
  case PointB:
    DefaultMode = PointerModeBoth;
    NSetCursor(GDK_LEFT_PTR);
    set_toolbox_mode(TOOLBOX_MODE_TOOLBAR);
    break;
  case LegendB:
    DefaultMode = PointerModeLegend;
    NSetCursor(GDK_LEFT_PTR);
    set_toolbox_mode(TOOLBOX_MODE_TOOLBAR);
    break;
  case AxisB:
    DefaultMode = PointerModeAxis;
    NSetCursor(GDK_LEFT_PTR);
    set_toolbox_mode(TOOLBOX_MODE_TOOLBAR);
    break;
  case DataB:
    DefaultMode = PointerModeData;
    NSetCursor(GDK_LEFT_PTR);
    set_toolbox_mode(TOOLBOX_MODE_TOOLBAR);
    break;
  case TrimB:
  case EvalB:
    NSetCursor(GDK_LEFT_PTR);
    set_toolbox_mode(TOOLBOX_MODE_TOOLBAR);
    break;
  case TextB:
    NSetCursor(GDK_XTERM);
    set_toolbox_mode(TOOLBOX_MODE_SETTING_PANEL);
    break;
  case ZoomB:
    NSetCursor(GDK_TARGET);
    set_toolbox_mode(TOOLBOX_MODE_TOOLBAR);
    break;
  default:
    NSetCursor(GDK_PENCIL);
    set_toolbox_mode(TOOLBOX_MODE_SETTING_PANEL);
  }
  NgraphApp.Viewer.Capture = FALSE;
  NgraphApp.Viewer.MouseMode = MOUSENONE;
  presetting_set_visibility(mode);

  if (d->MoveData) {
    move_data_cancel(d, TRUE);
  }

  gtk_widget_queue_draw(d->Win);
}

void
set_toolbox_mode(enum TOOLBOX_MODE mode)
{
  GtkWidget *widget;

  switch (mode) {
  case TOOLBOX_MODE_TOOLBAR:
    switch (NgraphApp.Viewer.Mode) {
    case PointB:
    case LegendB:
    case AxisB:
    case DataB:
    case TrimB:
    case EvalB:
    case ZoomB:
      widget = CToolbar;
      break;
    default:
      widget = SettingPanel;
      break;
    }
    break;
  case TOOLBOX_MODE_SETTING_PANEL:
    presetting_show_focused();
    widget = SettingPanel;
    break;
  }
  gtk_stack_set_visible_child(GTK_STACK(ToolBox), widget);
#if ! GTK_CHECK_VERSION(3, 99, 0)
  gtk_window_set_modal(GTK_WINDOW(TopLevel), widget == SettingPanel); /* for the GtkColorButton (modal GtkColorChooserDialog) */
#endif
}

enum TOOLBOX_MODE
get_toolbox_mode(void)
{
  GtkWidget *widget;

  widget = gtk_stack_get_visible_child(GTK_STACK(ToolBox));
  return (widget == CToolbar) ? TOOLBOX_MODE_TOOLBAR: TOOLBOX_MODE_SETTING_PANEL;
}

#define MODIFIED_TYPE_UNMODIFIED 0
#define MODIFIED_TYPE_DRAWOBJ    1
#define MODIFIED_TYPE_GRAOBJ     2

struct undo_info {
  enum MENU_UNDO_TYPE type;
  char **obj;
  time_t time;
  struct undo_info *next;
  int modified, id;
};
static struct undo_info *UndoInfo = NULL, *RedoInfo = NULL;

static void
iterate_undo_func(struct objlist *parent, UNDO_FUNC func)
{
  struct objlist *obj;
  obj = parent->child;
  while (obj) {
    if (obj->parent != parent) {
      break;
    }
    func(obj);
    if (obj->child) {
      iterate_undo_func(obj, func);
    }
    obj = obj->next;
  }
}

static int
menu_undo_iteration(UNDO_FUNC func, char **objs)
{
  struct objlist *obj;
  int r;

  r = 0;
  if (objs) {
    while (*objs) {
      obj = getobject(*objs);
      if (obj) {
	func(obj);
      }
      objs++;
    }
  } else {
    char *extra_objs[] = {"fit", "parameter", NULL};
    int i;
    obj = getobject("draw");
    if (obj == NULL) {
      return 1;
    }
    iterate_undo_func(obj, func);
    for (i = 0; extra_objs[i]; i++) {
      obj = getobject(extra_objs[i]);
      if (obj == NULL) {
	return 1;
      }
      r = func(obj);;
    }
  }

  return r;
}

static int
menu_check_undo(void)
{
  return UndoInfo ? 1 : 0;
}

static int
menu_check_redo(void)
{
  return RedoInfo ? 1 : 0;
}

static struct undo_info *
undo_info_push(enum MENU_UNDO_TYPE type, char **obj)
{
  static int id = 0;
  struct undo_info *info;
  time_t t;

  t = time(NULL);
  if (UndoInfo &&
      (type == UndoInfo->type) &&
      (t - UndoInfo->time < 2)) {
    UndoInfo->time = t;
    return NULL;
  }
  info = g_malloc(sizeof(*info));
  if (info == NULL) {
    return NULL;
  }
  info->type = type;
  info->obj = g_strdupv(obj);
  info->next = UndoInfo;
  info->time = t;
  info->modified = get_graph_modified();
  info->id = id++;
  UndoInfo = info;
  return info;
}

static struct undo_info *
undo_info_pop(struct undo_info *undo_info)
{
  struct undo_info *info, *next;

  if (undo_info == NULL) {
    return NULL;
  }
  info = undo_info;
  next = info->next;
  g_strfreev(info->obj);
  g_free(info);
  return next;
}

static char *UndoTypeStr[UNDO_TYPE_NUM] = {
  N_("edit"),
  N_("move"),
  N_("rotate"),
  N_("flip"),
  N_("delete object"),
  N_("create object"),
  N_("align"),
  N_("order"),
  N_("duplicate"),
  N_("execute shell"),
  N_("execute add-in"),
  N_("scale clear"),
  N_("scale undo"),
  N_("open file"),
  N_("add range"),
  N_("paste"),
  N_("scale"),
  N_("auto scale"),
  N_("scale trimming"),
  N_("edit"),			/* dummy message */
};

#define EDIT_MENU_INDEX 1
#define UNDO_MENU_SECTION_INDEX 0
static void
set_undo_menu_label(void)
{
  GMenuModel *menu;
  GMenuItem *item;
  int i, n;
  char buf[1024], *label;

  menu = gtk_application_get_menubar(GtkApp);
  if (menu == NULL) {
    return;
  }

  menu = g_menu_model_get_item_link(G_MENU_MODEL(menu), EDIT_MENU_INDEX, G_MENU_LINK_SUBMENU);
  if (menu == NULL) {
    return;
  }

  n = g_menu_model_get_n_items(menu);
  if (n < UNDO_MENU_SECTION_INDEX) {
    return;
  }

  menu = g_menu_model_get_item_link(menu, UNDO_MENU_SECTION_INDEX, G_MENU_LINK_SECTION);
  if (menu == NULL) {
    return;
  }

  n = g_menu_model_get_n_items(menu);
  for (i = 0; i < n; i++) {
    g_menu_remove(G_MENU(menu), 0);
  }

  if (RedoInfo) {
    snprintf(buf, sizeof(buf), _("_Redo: %s"), _(UndoTypeStr[RedoInfo->type]));
    label = buf;
  } else {
    label = _("_Redo");
  }
  item = g_menu_item_new(label, "app.EditRedoAction");
  g_menu_insert_item(G_MENU(menu), 0, item);
  g_object_unref(item);

  if (UndoInfo) {
    snprintf(buf, sizeof(buf), _("_Undo: %s"), _(UndoTypeStr[UndoInfo->type]));
    label = buf;
  } else {
    label = _("_Undo");
  }
  item = g_menu_item_new(label, "app.EditUndoAction");
  g_menu_insert_item(G_MENU(menu), 0, item);
  g_object_unref(item);
}

int
menu_save_undo(enum MENU_UNDO_TYPE type, char **obj)
{
  struct undo_info *info;

  info = undo_info_push(type, obj);
  if (info == NULL) {
    return -1;
  }
  menu_undo_iteration(undo_save, obj);
  while (RedoInfo) {
    RedoInfo = undo_info_pop(RedoInfo);
  }
  set_undo_menu_label();
  set_action_widget_sensitivity(EditUndoAction, menu_check_undo());
  set_action_widget_sensitivity(EditRedoAction, menu_check_redo());
  return info->id;
}

int
menu_save_undo_single(enum MENU_UNDO_TYPE type, char *obj)
{
  char *objs[2];
  objs[0] = obj;
  objs[1] = NULL;
  return menu_save_undo(type, objs);
}


void
menu_delete_undo(int id)
{
  if (UndoInfo == NULL) {
    return;
  }
  if (UndoInfo->id != id) {
    return;
  }
  menu_undo_iteration(undo_delete, UndoInfo->obj);
  UndoInfo = undo_info_pop(UndoInfo);
  set_undo_menu_label();
  if (! menu_check_undo()) {
    set_action_widget_sensitivity(EditUndoAction, FALSE);
  }
}

void
menu_clear_undo(void)
{
  while (UndoInfo) {
    UndoInfo = undo_info_pop(UndoInfo);
  }
  while (RedoInfo) {
    RedoInfo = undo_info_pop(RedoInfo);
  }
  menu_undo_iteration(undo_clear, NULL);
  set_undo_menu_label();
  set_action_widget_sensitivity(EditUndoAction, menu_check_undo());
  set_action_widget_sensitivity(EditRedoAction, menu_check_redo());
  merge_cache_clear();
}

static void
undo_update_widgets(struct undo_info *info)
{
  set_action_widget_sensitivity(EditUndoAction, menu_check_undo());
  set_action_widget_sensitivity(EditRedoAction, menu_check_redo());
  check_exist_instances(chkobject("draw"));
  check_instance(chkobject("parameter"));
  set_axis_undo_button_sensitivity(axis_check_history());
  if (info->obj == NULL) {
    UpdateAll(NULL);
  } else {
    char **ptr, *objs[OBJ_MAX];
    int i, axis, data, axisgrid;
    axis = FALSE;
    data = FALSE;
    axisgrid = FALSE;
    ptr = info->obj;
    i = 0;
    while (ptr[i]) {
      objs[i] = ptr[i];
      if (strcmp(ptr[i], "axis") == 0) {
	axis = TRUE;
      } else if (strcmp(ptr[i], "data") == 0) {
	data = TRUE;
      } else if (strcmp(ptr[i], "axisgrid") == 0) {
	axisgrid = TRUE;
      }
      i++;
    }
    if (axis) {
      if (! data) {
	objs[i] = "data";
	i++;
      }
      if (! axisgrid) {
	objs[i] = "axisgrid";
	i++;
      }
    }
    objs[i] = NULL;
    UpdateAll(objs);
  }
}

static void
graph_modified_sub(int a)
{
  if (Menulocal.obj == NULL)
    return;

  putobj(Menulocal.obj, "modified", 0, &a);
}

static int
undo_check_modified(struct undo_info *info)
{
  int modified_saved, modified_current;
  modified_saved = info->modified;
  modified_current = get_graph_modified();
  if (modified_current) {
    if (! (modified_current & (modified_saved | MODIFIED_TYPE_GRAOBJ))) {
      graph_modified_sub(MODIFIED_TYPE_UNMODIFIED);
    }
  } else {
    if (modified_saved) {
      set_graph_modified();
    }
  }
  return modified_current;
}

static struct undo_info *
menu_undo_common(int *is_modify)
{
  int r, modified;
  struct undo_info *info;

  r = menu_undo_iteration(undo_undo, UndoInfo->obj);
  if (r) {
    return NULL;
  }
  modified = undo_check_modified(UndoInfo);
  info = UndoInfo;
  UndoInfo = info->next;
  if (is_modify) {
    *is_modify = modified;
  }
  return info;
}

void
menu_undo_internal(int id)
{
  struct undo_info *info;
  if (UndoInfo == NULL) {
    return;
  }
  if (UndoInfo->id != id) {
    return;
  }
  info = menu_undo_common(NULL);
  if (info == NULL) {
    return;
  }
  undo_info_pop(info);
  set_action_widget_sensitivity(EditUndoAction, menu_check_undo());
  set_action_widget_sensitivity(EditRedoAction, menu_check_redo());
  set_undo_menu_label();
}

void
menu_undo(void)
{
  int modified;
  struct undo_info *info;
  if (UndoInfo == NULL) {
    return;
  }

  info = menu_undo_common(&modified);
  if (info == NULL) {
    return;
  }
  info->next = RedoInfo;
  info->modified = modified;
  RedoInfo = info;
  undo_update_widgets(info);
  set_undo_menu_label();
}

void
menu_redo(void)
{
  int r, modified;
  struct undo_info *info;
  if (RedoInfo == NULL) {
    return;
  }
  r = menu_undo_iteration(undo_redo, RedoInfo->obj);
  if (r) {
    return;
  }
  modified = undo_check_modified(RedoInfo);
  info = RedoInfo;
  RedoInfo = info->next;
  info->next = UndoInfo;
  info->modified = modified;
  UndoInfo = info;
  undo_update_widgets(info);
  set_undo_menu_label();
}

int
get_graph_modified(void)
{
  return Menulocal.modified;
}

void
set_graph_modified(void)
{
  graph_modified_sub(MODIFIED_TYPE_DRAWOBJ);
}

void
set_graph_modified_gra(void)
{
  graph_modified_sub(MODIFIED_TYPE_GRAOBJ);
}

static void
reset_modified_info(struct undo_info *info)
{
  for (; info; info = info->next) {
      info->modified = 1;
  }
}

void
reset_graph_modified(void)
{
  graph_modified_sub(MODIFIED_TYPE_UNMODIFIED);
  reset_modified_info(UndoInfo);
  reset_modified_info(RedoInfo);
}

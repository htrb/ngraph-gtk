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
#include "gtk_listview.h"

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

static GtkWidget *CurrentWindow = NULL, *CToolbar = NULL, *PToolbar = NULL, *SettingPanel = NULL, *ToolBox = NULL, *RecentGraphMenu = NULL, *RecentDataMenu = NULL;
static unsigned int CursorType;

#if USE_EXT_DRIVER
static GtkWidget *ExtDrvOutMenu = NULL
#endif

struct MenuItem;
struct ToolItem;

static void create_menu(struct MenuItem *item);
static GtkWidget *create_toolbar(struct ToolItem *item, int n, GtkOrientation orientation, GCallback btn_press_cb);
static void CmViewerButtonArm(GtkWidget *action, gpointer client_data);
static void check_exist_instances(struct objlist *parent);
static void check_instance(struct objlist *obj);

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
static GtkWidget *PointerModeButtons[PointerModeNum];
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

struct Accelerator {
  const char *action;
  const char *accel;
};

static struct Accelerator accelerator[] = {
  {
    "app.GraphLoadAction",
    "<Primary>R"
  },
  {
    "app.GraphSaveAction",
    "<Primary>S"
  },
  {
    "app.GraphSaveAsAction",
    "<Primary><Shift>S"
  },
  {
    "app.GraphPrintAction",
    "<Primary>P"
  },
  {
    "app.quit",
    "<Primary>Q"
  },
  {
    "app.EditRedoAction",
    "<Primary>Y"
  },
  {
    "app.EditUndoAction",
    "<Primary>Z"
  },
  {
    "app.EditCutAction",
    "<Primary>X"
  },
  {
    "app.EditCopyAction",
    "<Primary>C"
  },
  {
    "app.EditPasteAction",
    "<Primary>V"
  },
  {
    "app.EditDeleteAction",
    "Delete"
  },
  {
    "app.EditDuplicateAction",
    "Insert"
  },
  {
    "app.EditSelectAllAction",
    "<Primary>A"
  },
  {
    "app.EditOrderTopAction",
    "<Shift>Home"
  },
  {
    "app.EditOrderBottomAction",
    "<Shift>End"
  },
  {
    "app.ViewDrawAction",
    "<Primary>D"
  },
  {
    "app.ViewCrossGaugeAction",
    "<Primary>G"
  },
  {
    "app.DataAddFileAction",
    "<Primary>O"
  },
  {
    "app.AxisScaleClearAction",
    "<Primary><Shift>C"
  },
  {
    "app.help",
    "F1"
  }
};

static void
add_accelerator(GtkApplication *application, struct Accelerator *accel, int n)
{
  int i;
  const char *vaccels[] = {NULL, NULL};

  for (i = 0; i < n; i++) {
    vaccels[0] = accel[i].accel;
    gtk_application_set_accels_for_action(application, accel[i].action, vaccels);
  }
}

void
set_pointer_mode(int id)
{
  GtkWidget *button;

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
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(button), TRUE);
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

  if (! Menulocal.lock) {
    return;
  }

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
  w = gtk_paned_get_start_child(GTK_PANED(NgraphApp.Viewer.main_pane));
  if (w) {
    gtk_widget_set_sensitive(w, ! Menulock);
  }
  if (! Menulock) {
    check_exist_instances(chkobject("draw"));
    check_instance(chkobject("parameter"));
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

static void
main_loop_quit_cb(gpointer client_data)
{
  g_main_loop_quit(main_loop());
}

static void
main_loop_quit(void)
{
  gtk_widget_hide(TopLevel);
  g_idle_add_once(main_loop_quit_cb, NULL);
}

#if ! WINDOWS
static void
kill_signal_handler(int sig)
{
  if (Menulock || check_paint_lock()) {
    set_interrupt();		/* accept SIGINT */
  } else {
    QuitGUI();
  }
}

static void
term_signal_handler(int sig)
{
  main_loop_quit();
}
#endif	/* WINDOWS */

static gboolean
CloseCallback(GtkWidget *w, GdkEvent *event, gpointer user_data)
{
  CmGraphQuit(NULL, user_data);

  return TRUE;
}

static void save_tab_position(void);
static void get_pane_position(void);
static void save_entry_history(void);

static void
check_inifile_response(int response, struct objlist *obj, int id, int modified)
{
  if (response) {
    save_tab_position();
    get_pane_position();
    menu_save_config(SAVE_CONFIG_TYPE_GEOMETRY);
    save_entry_history();
    menu_save_config(SAVE_CONFIG_TYPE_TOGGLE_VIEW |
		     SAVE_CONFIG_TYPE_OTHERS);
  }
  main_loop_quit();
}

static void
QuitGUI_response(int ret, gpointer client_data)
{
  if (ret) {
    CheckIniFile(check_inifile_response, NULL, 0, 0);
  }
}

void
QuitGUI(void)
{
  CheckSave(QuitGUI_response, NULL);
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
  GdkContentFormats *format;
  GdkClipboard *clip;

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
	  clip = gtk_widget_get_clipboard(TopLevel);
	  format = gdk_clipboard_get_formats(clip);
	  state = gdk_content_formats_contain_mime_type(format, "text/plain");
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
    NgraphApp.cursor[i] = gdk_cursor_new_from_name(Cursor[i], NULL);
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

static void
tool_button_enter_cb(GtkEventControllerMotion *self, gdouble x, gdouble y, gpointer user_data)
{
  char *str;

  str = (char *) user_data;
  SetStatusBar(str);
}
static void
tool_button_leave_cb(GtkEventControllerMotion *self, gpointer data)
{
  ResetStatusBar();
}

static GtkWidget *
create_message_box(GtkWidget **label1, GtkWidget **label2)
{
  GtkWidget *frame, *w, *hbox;

  frame = gtk_frame_new(NULL);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  w = gtk_label_new(NULL);
  gtk_widget_set_halign(w, GTK_ALIGN_END);
  gtk_box_append(GTK_BOX(hbox), w);
  *label1 = w;

  w = gtk_label_new(NULL);
  gtk_widget_set_halign(w, GTK_ALIGN_START);
  gtk_label_set_width_chars(GTK_LABEL(w), 16);
  gtk_box_append(GTK_BOX(hbox), w);
  *label2 = w;

  gtk_frame_set_child(GTK_FRAME(frame), hbox);

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

  icon = gtk_image_new_from_icon_name(icon_name);
  gtk_image_set_icon_size(GTK_IMAGE(icon), Menulocal.icon_size);
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
    tab = gtk_paned_get_start_child(GTK_PANED(NgraphApp.Viewer.side_pane2));
    save_tab_position_sub(tab, tab_info + i, 0);

    tab = gtk_paned_get_end_child(GTK_PANED(NgraphApp.Viewer.side_pane2));
    save_tab_position_sub(tab, tab_info + i, 100);
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
    if (tab_info[j].tab > 0) {
      tab = gtk_paned_get_end_child(GTK_PANED(NgraphApp.Viewer.side_pane2));
    } else {
      tab = gtk_paned_get_start_child(GTK_PANED(NgraphApp.Viewer.side_pane2));
    }
    tab_info[j].init_func(tab_info[j].d);
    setup_object_tab(tab_info[j].d, tab, tab_info[j].icon, _(tab_info[j].obj_name));
  }

  CoordWinCreate(&NgraphApp.CoordWin);
  gtk_paned_set_start_child(GTK_PANED(NgraphApp.Viewer.side_pane3), NgraphApp.CoordWin.Win);
  InfoWinCreate(&NgraphApp.InfoWin);
  gtk_paned_set_end_child(GTK_PANED(NgraphApp.Viewer.side_pane3), NgraphApp.InfoWin.Win);

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

static void
clipboard_changed(GdkClipboard* self,  gpointer user_data)
{
  struct Viewer *d;

  d = (struct Viewer *) user_data;

  set_focus_sensitivity(d);
}

#define USE_APP_HEADER_BAR 0
static void
setup_toolbar(GtkWidget *window)
{
  GtkWidget *w;
#if USE_APP_HEADER_BAR
  GtkWidget *hbar;
#endif
  w = create_toolbar(CommandToolbar, sizeof(CommandToolbar) / sizeof(*CommandToolbar), GTK_ORIENTATION_HORIZONTAL, NULL);
  CToolbar = w;

#if USE_APP_HEADER_BAR
  hbar = gtk_header_bar_new();
  gtk_header_bar_set_show_title_buttons(GTK_HEADER_BAR(hbar), TRUE);
  gtk_window_set_titlebar(GTK_WINDOW(window), hbar);
#endif
  w = create_toolbar(PointerToolbar, sizeof(PointerToolbar) / sizeof(*PointerToolbar), GTK_ORIENTATION_VERTICAL, G_CALLBACK(CmViewerButtonPressed));
  PToolbar = w;
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
  gtk_box_append(GTK_BOX(vbox2), ToolBox);
  gtk_box_append(GTK_BOX(hbox), PToolbar);

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
  gtk_paned_set_start_child(GTK_PANED(vpane2), w);

  w = gtk_notebook_new();
  gtk_notebook_popup_enable(GTK_NOTEBOOK(w));
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w), GTK_POS_LEFT);
  gtk_notebook_set_group_name(GTK_NOTEBOOK(w), SIDE_PANE_TAB_ID);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(w), TRUE);
  gtk_paned_set_end_child(GTK_PANED(vpane2), w);

  hpane2 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  NgraphApp.Viewer.side_pane3 = hpane2;

  vpane1 = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_set_start_child(GTK_PANED(vpane1), vpane2);
  gtk_paned_set_end_child(GTK_PANED(vpane1), hpane2);
  NgraphApp.Viewer.side_pane1 = vpane1;

  hpane1 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
#if USE_APP_HEADER_BAR
  gtk_paned_set_start_child(GTK_PANED(hpane1), hbox);
#else
  gtk_paned_set_start_child(GTK_PANED(hpane1), vbox);
#endif
  gtk_paned_set_end_child(GTK_PANED(hpane1), vpane1);
  NgraphApp.Viewer.main_pane = hpane1;

  gtk_box_append(GTK_BOX(hbox), table);
#if ! USE_APP_HEADER_BAR
  gtk_box_append(GTK_BOX(vbox), hbox);
#endif
  gtk_box_append(GTK_BOX(hbox2), hpane1);
  gtk_box_append(GTK_BOX(vbox2), hbox2);

  NgraphApp.Message = gtk_statusbar_new();
  gtk_widget_set_hexpand(NgraphApp.Message, TRUE);
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  gtk_box_append(GTK_BOX(hbox), NgraphApp.Message);
  gtk_box_append(GTK_BOX(hbox), create_message_box(&NgraphApp.Message_extra, &NgraphApp.Message_pos));
  gtk_box_append(GTK_BOX(vbox2), hbox);

  NgraphApp.Message1 = gtk_statusbar_get_context_id(GTK_STATUSBAR(NgraphApp.Message), "Message1");

  set_axis_undo_button_sensitivity(FALSE);

  gtk_window_set_child(GTK_WINDOW(TopLevel), vbox2);

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

  if (Menulock) {
    return;
  }
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

static void
add_recent_menu_item(GtkRecentInfo *info, GtkStringList *string_list, int type)
{
  int local, n;
  const char *uri, *mime, *target_mime;
  char *filename;
  struct stat sb;

  if (! gtk_recent_info_has_application (info, AppName)) {
    return;
  }

  local = gtk_recent_info_is_local(info);
  if (! local) {
    return;
  }

  n = g_list_model_get_n_items(G_LIST_MODEL (string_list));
  if (n >= RECENT_CHOOSER_LIMIT) {
    return;
  }

  uri = gtk_recent_info_get_uri(info);
  mime = gtk_recent_info_get_mime_type(info);
  target_mime = (type == RECENT_TYPE_GRAPH) ? NGRAPH_GRAPH_MIME : NGRAPH_DATA_MIME;
  if (g_ascii_strcasecmp(mime, target_mime)) {
    return;
  }
  filename = g_filename_from_uri(uri, NULL, NULL);
  if (filename == NULL) {
    return;
  }
  if (g_stat(filename, &sb)) {
    g_free(filename);
    return;
  }
  if (! S_ISREG(sb.st_mode)) {
    g_free(filename);
    return;
  }
  gtk_string_list_append (string_list, filename);

  g_free(filename);
}

static void
setup_recent_data(GtkWidget *menu_button, int type)
{
  GtkRecentManager *manager;
  GList *list, *ptr;
  int n;
  GtkPopover *popover;
  GtkWidget *list_view;
  GtkStringList *string_list;
  GtkSingleSelection *model;

  popover = gtk_menu_button_get_popover (GTK_MENU_BUTTON (menu_button));
  list_view = gtk_popover_get_child (GTK_POPOVER (popover));
  model = GTK_SINGLE_SELECTION (gtk_list_view_get_model(GTK_LIST_VIEW (list_view)));
  string_list = GTK_STRING_LIST (gtk_single_selection_get_model(model));

  n = g_list_model_get_n_items (G_LIST_MODEL (string_list));
  gtk_string_list_splice (string_list, 0, n, NULL);

  manager = gtk_recent_manager_get_default();
  list = gtk_recent_manager_get_items(manager);
  for (ptr = list; ptr; ptr = ptr->next) {
    GtkRecentInfo *info;
    info = ptr->data;
    add_recent_menu_item(info, string_list, type);
    gtk_recent_info_unref(info);
  }
  g_list_free(list);

  n = g_list_model_get_n_items (G_LIST_MODEL (string_list));
  gtk_widget_set_sensitive(menu_button, n > 0);
}

static void
bind_recent_item_cb (GtkListItemFactory *factory, GtkListItem *list_item)
{
  GtkWidget *label;
  gpointer item;
  const char *filename;
  char *basename;

  label = gtk_list_item_get_child (list_item);
  item = gtk_list_item_get_item (list_item);
  filename = gtk_string_object_get_string (GTK_STRING_OBJECT (item));
  gtk_widget_set_tooltip_text (label, filename);

  basename = getbasename(filename);
  gtk_label_set_text(GTK_LABEL(label), basename);
  g_free(basename);
}

static void
select_recent_item_cb(GtkListView *self, guint position, gpointer user_data)
{
  GtkWidget *popover;
  GtkStringList *list;
  GtkSingleSelection *model;
  const char *filename;
  int type;

  type = GPOINTER_TO_INT(user_data);
  model = GTK_SINGLE_SELECTION (gtk_list_view_get_model(self));
  list = GTK_STRING_LIST (gtk_single_selection_get_model(model));
  filename = gtk_string_list_get_string (list, position);

  popover = widget_get_grandparent(GTK_WIDGET(self));
  if (G_TYPE_CHECK_INSTANCE_TYPE(popover, GTK_TYPE_POPOVER)) {
    gtk_popover_popdown(GTK_POPOVER(popover));
  }

  switch (type) {
  case RECENT_TYPE_GRAPH:
    graph_dropped(filename);
    break;
  case RECENT_TYPE_DATA:
    load_data(filename);
    break;
  }
}

static void
create_recent_menu(GtkWidget *menu_button, int type)
{
  GtkWidget *popover, *menu;

  menu = listview_create(N_SELECTION_TYPE_SINGLE, NULL, G_CALLBACK (bind_recent_item_cb), NULL);
  gtk_list_view_set_single_click_activate (GTK_LIST_VIEW (menu), TRUE);

  popover = gtk_popover_new();
  gtk_popover_set_child(GTK_POPOVER (popover), menu);
  gtk_menu_button_set_popover (GTK_MENU_BUTTON (menu_button), popover);
  g_signal_connect(menu, "activate", G_CALLBACK(select_recent_item_cb), GINT_TO_POINTER(type));

  setup_recent_data(menu_button, type);
}

static void
recent_manger_changed(GtkRecentManager* self, gpointer user_data)
{
  setup_recent_data(RecentGraphMenu, RECENT_TYPE_GRAPH);
  setup_recent_data(RecentDataMenu, RECENT_TYPE_DATA);
}

static void
remove_menu_model(GtkWidget *button)
{
  GMenuModel *menu;
  menu = gtk_menu_button_get_menu_model(GTK_MENU_BUTTON(button));
  if (menu) {
    gtk_menu_button_set_menu_model(GTK_MENU_BUTTON(button), NULL);
    g_object_unref(menu);
  }
}

static void
finalize_recent_manager(void)
{
  GtkRecentManager *manager;
  manager = gtk_recent_manager_get_default();
  g_signal_handlers_disconnect_by_func(manager, G_CALLBACK(recent_manger_changed), NULL);

  remove_menu_model(RecentGraphMenu);
  remove_menu_model(RecentDataMenu);
}

static void
setup_recent_manager(void)
{
  GtkRecentManager *manager;
  manager = gtk_recent_manager_get_default();
  g_signal_connect(manager, "changed", G_CALLBACK(recent_manger_changed), NULL);
}

static GtkWidget*
create_save_menu(void)
{
  GtkWidget *menu;
  GMenu *gmenu;

  gmenu = gtk_application_get_menu_by_id(GtkApp, "save-menu");
  if (gmenu == NULL) {
    return NULL;
  }
#if USE_NESTED_SUBMENUS
  menu = gtk_popover_menu_new_from_model_full(G_MENU_MODEL(gmenu), POPOVERMEU_FLAG);
#else  /* USE_NESTED_SUBMENUS */
  menu = gtk_popover_menu_new_from_model(G_MENU_MODEL(gmenu));
#endif	/* USE_NESTED_SUBMENUS */
  return menu;
}

void
draw_notify(int notify)
{
  static int state = FALSE;
  const char *icon_name;
  GtkWidget *img;
  if (state == notify) {
    return;
  }
  state = notify;
  if (DrawButton == NULL) {
    return;
  }
  icon_name = (state) ? "ngraph_draw-attention-symbolic" : "ngraph_draw-symbolic";
  img = gtk_image_new_from_icon_name(icon_name);
  gtk_image_set_icon_size(GTK_IMAGE(img), Menulocal.icon_size);
  gtk_button_set_child(GTK_BUTTON(DrawButton), img);
  gtk_widget_queue_draw(DrawButton);
}

static GtkWidget *
create_toolbar(struct ToolItem *item, int n, GtkOrientation orientation, GCallback btn_press_cb)
{
  int i;
  GtkWidget *toolbar,  *widget, *group = NULL, *menu;

  toolbar = gtk_box_new(orientation, 3);
  set_widget_margin_all(toolbar, 4);
  for (i = 0; i < n; i++) {
    menu = NULL;
    switch (item[i].type) {
    case TOOL_TYPE_SEPARATOR:
      widget = gtk_separator_new(orientation);
      break;
    case TOOL_TYPE_NORMAL:
      widget = button_new_with_icon(item[i].icon, FALSE);
      break;
    case TOOL_TYPE_DRAW:
      widget = button_new_with_icon(item[i].icon, FALSE);
      DrawButton = GTK_WIDGET(widget);
      break;
    case TOOL_TYPE_SAVE:
      widget = button_new_with_icon(item[i].icon, FALSE);
      menu = gtk_menu_button_new();
      gtk_menu_button_set_popover(GTK_MENU_BUTTON(menu), create_save_menu());
      gtk_widget_set_tooltip_text(menu, _("Save menu"));
      break;
    case TOOL_TYPE_RECENT_GRAPH:
      widget = button_new_with_icon(item[i].icon, FALSE);
      menu = gtk_menu_button_new();
      gtk_widget_set_tooltip_text(menu, _("Recent Graphs"));
      create_recent_menu(menu, RECENT_TYPE_GRAPH);
      RecentGraphMenu = menu;
      break;
    case TOOL_TYPE_RECENT_DATA:
      widget = button_new_with_icon(item[i].icon, FALSE);
      menu = gtk_menu_button_new();
      gtk_widget_set_tooltip_text(menu, _("Recent Data Files"));
      create_recent_menu(menu, RECENT_TYPE_DATA);
      RecentDataMenu = menu;
      break;
    case TOOL_TYPE_RADIO:
      widget = button_new_with_icon(item[i].icon, TRUE);
      if (group) {
        gtk_toggle_button_set_group(GTK_TOGGLE_BUTTON(widget), GTK_TOGGLE_BUTTON(group));
      } else {
        group = widget;
      }
      if (i == 0) {
	gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget), TRUE);
      }
      if (btn_press_cb) {
	GtkGesture *gesture;

	gesture = gtk_gesture_click_new();
	gtk_widget_add_controller(widget, GTK_EVENT_CONTROLLER(gesture));

	gtk_gesture_single_set_button(GTK_GESTURE_SINGLE(gesture), 0);
	gtk_event_controller_set_propagation_phase(GTK_EVENT_CONTROLLER(gesture), GTK_PHASE_CAPTURE);
	g_signal_connect(gesture, "pressed", btn_press_cb, NULL);
      }
      break;
    default:
      widget = NULL;
    }

    if (widget == NULL) {
      continue;
    }

    if (item[i].action_name) {
      gtk_actionable_set_action_name(GTK_ACTIONABLE(widget), item[i].action_name);
    } else if (item[i].callback) {
      g_signal_connect(widget,
		       (item[i].type == TOOL_TYPE_RADIO) ? "toggled" : "clicked",
		       G_CALLBACK(item[i].callback),
		       GINT_TO_POINTER(item[i].user_data));
    }

    if (item[i].tip) {
      gtk_widget_set_tooltip_text(widget, _(item[i].tip));
    }

    if (item[i].caption) {
      GtkEventController *ev;
      ev = gtk_event_controller_motion_new();
      g_signal_connect(ev,
		       "enter",
		       G_CALLBACK(tool_button_enter_cb),
		       (gpointer) _(item[i].caption));

      g_signal_connect(ev,
		       "leave",
		       G_CALLBACK(tool_button_leave_cb), NULL);
      gtk_widget_add_controller(widget, ev);
    }

    if (item[i].button >= PointerModeOffset) {
      int id;
      id = item[i].button - PointerModeOffset;
      PointerModeButtons[id]= widget;
    }

    gtk_widget_set_can_focus(widget, FALSE);
    if (menu) {
      GtkWidget *box;
      gtk_widget_set_can_focus(menu, FALSE);
      box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
      gtk_box_append(GTK_BOX(box), widget);
      gtk_widget_add_css_class(widget, MENUBUTTON_CLASS);
      gtk_box_append(GTK_BOX(box), menu);
      gtk_widget_add_css_class(menu, MENUBUTTON_CLASS);
      gtk_box_append(GTK_BOX(toolbar), box);
    } else {
      gtk_box_append(GTK_BOX(toolbar), widget);
    }
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
#if USE_NESTED_SUBMENUS
  popup = gtk_popover_menu_new_from_model_full(G_MENU_MODEL(menu), POPOVERMEU_FLAG);
#else  /* USE_NESTED_SUBMENUS */
  popup = gtk_popover_menu_new_from_model(G_MENU_MODEL(menu));
#endif	/* USE_NESTED_SUBMENUS */
  gtk_popover_set_has_arrow(GTK_POPOVER(popup), FALSE);
  return popup;
}

static int
create_toplevel_window(void)
{
  int i;
  struct objlist *aobj;
  int width, height, w, h;
  GdkDisplay *disp;
  GtkWidget *popup;
  GdkClipboard *clip;

  NgraphApp.recent_manager = gtk_recent_manager_get_default();

  init_action_widget_list();
  init_ngraph_app_struct();

  w = 800;
  h = 600;
  disp = gdk_display_get_default();
  if (disp) {
    GdkMonitor *monitor;
    GdkSurface *surface;
    surface = gdk_surface_new_toplevel(disp);
    monitor = gdk_display_get_monitor_at_surface(disp, surface);
    /* g_object_unref(surface); */
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

  width = Menulocal.menuwidth;
  height = Menulocal.menuheight;

  load_hist();

  CurrentWindow = TopLevel = gtk_application_window_new(GtkApp);
  gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(TopLevel), TRUE);
  popup = create_popup_menu(GtkApp);
  if (popup) {
    NgraphApp.Viewer.popup = popup;
  }
  clip = gtk_widget_get_clipboard(TopLevel);
  g_signal_connect(clip, "changed", G_CALLBACK(clipboard_changed), &NgraphApp.Viewer);

  gtk_window_set_title(GTK_WINDOW(TopLevel), AppName);
  gtk_window_set_default_size(GTK_WINDOW(TopLevel), width, height);

  g_signal_connect(TopLevel, "close_request", G_CALLBACK(CloseCallback), NULL);

  create_icon();
  initdialog();

  setup_toolbar(TopLevel);
  gtk_widget_show(GTK_WIDGET(TopLevel));
  setupwindow(GtkApp);
  add_accelerator(GtkApp, accelerator, G_N_ELEMENTS(accelerator));
  create_addin_menu();

  NgraphApp.FileName = NULL;
  NgraphApp.Viewer.Mode = PointB;

  gtk_widget_show(GTK_WIDGET(TopLevel));
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

  set_widget_visibility();

  set_focus_sensitivity(&NgraphApp.Viewer);
  check_exist_instances(chkobject("draw"));
  check_instance(chkobject("parameter"));

  set_newobj_cb(check_instance);
  set_delobj_cb(check_instance);

  return 0;
}

static void
souce_view_set_search_path(void)
{
  const gchar * const *dirs;
  const char **new_dirs;
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

static void
initial_draw(gpointer user_data)
{
  char *file = (char *) user_data;
  if (file) {
    char *ext;
    ext = getextention(file);
    if (ext && ((strcmp0(ext, "NGP") == 0) || (strcmp0(ext, "ngp") == 0))) {
      LoadNgpFile(file, FALSE, NULL, NULL);
    } else {
      CmViewerDraw(NULL, GINT_TO_POINTER(FALSE));
    }
    g_free(file);
  } else {
    CmViewerDraw(NULL, GINT_TO_POINTER(FALSE));
  }
}

int
application(char *file)
{
  int terminated;
  char *file2;

  if (TopLevel) {
    if (gtk_widget_is_visible(TopLevel)) {
      return 1;
    }
    gtk_widget_show(TopLevel);
    OpenGC();
    OpenGRA();
  } else {
    GtkIconTheme *theme;
    GdkDisplay* display;
    display = gdk_display_get_default();
    theme = gtk_icon_theme_get_for_display(display);
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

  file2 = g_strdup(file);
  g_idle_add_once(initial_draw, file2);

  system_set_draw_notify_func(draw_notify);
  set_toolbox_mode(TOOLBOX_MODE_TOOLBAR);
  n_application_ready();
  setup_recent_manager();
  g_main_loop_run(main_loop());
  terminated = FALSE;
  finalize_recent_manager();
  system_set_draw_notify_func(NULL);

  set_newobj_cb(NULL);
  set_delobj_cb(NULL);

#if ! WINDOWS
  set_signal(SIGTERM, 0, SIG_DFL, NULL);
  set_signal(SIGINT, 0, SIG_DFL, NULL);
#endif	/* WINDOWS */

  CloseGC();
  CloseGRA();

  if (terminated) {
    unref_entry_history();

    ViewerWinClose();

    g_free(NgraphApp.FileName);
    NgraphApp.FileName = NULL;

    gtk_window_destroy(GTK_WINDOW(TopLevel));
    NgraphApp.Viewer.Win = NULL;
    CurrentWindow = TopLevel = PToolbar = CToolbar = ToolBox = NULL;

    free_cursor();

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
  GdkCursor *cursor;

  if (NgraphApp.Viewer.Win == NULL || NgraphApp.cursor == NULL || CursorType == type)
    return;

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
    gtk_widget_set_cursor(NgraphApp.Viewer.Win, cursor);
  }
}

static void
display_dialog_main(gpointer user_data)
{
  char *ustr;
  ustr = (char *) user_data;
  InfoWinDrawInfoText(ustr);
  g_free(ustr);
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
  g_idle_add_once(display_dialog_main, ustr);
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

static void
put_stderr_main(gpointer user_data)
{
  char *ustr;
  ustr = (char *) user_data;
  message_box(get_current_window(), ustr, _("Error:"), RESPONS_ERROR);
  g_free(ustr);
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
  len = strlen(ustr);
  g_idle_add_once(put_stderr_main, ustr);
  return len + 1;
}

int
ChkInterrupt(void)
{
  return check_interrupt();
}

struct yn_response_data {
  int wait;
  int response;
  const char *msg;
};

static void
input_yn_cb(int response, gpointer user_data)
{
  struct yn_response_data *data;
  data = (struct yn_response_data *) user_data;
  data->response = response;
  data->wait = FALSE;
}

static void
input_yn_main(gpointer user_data)
{
  struct yn_response_data *data;
  data = (struct yn_response_data *) user_data;
  response_message_box(get_current_window(), data->msg, _("Question"), RESPONS_YESNO, input_yn_cb, data);
}

int
InputYN(const char *mes)
{
  struct yn_response_data response;
  response.wait = TRUE;
  response.response = TRUE;
  response.msg = mes;
  if (is_main_thread()) {
    return TRUE;
  }
  g_idle_add_once(input_yn_main, &response);
  dialog_wait(&response.wait);
  return response.response;
}

#if ! USE_EVENT_LOOP
static void
script_exec_finalize(gpointer user_data)
{
  GThread *thread;

  thread = (GThread *) user_data;

  menu_lock(FALSE);
  ResetStatusBar();

  GetPageSettingsFromGRA();
  UpdateAll(NULL);

  main_window_redraw();
  gtk_widget_set_sensitive(TopLevel, TRUE);
  g_thread_join(thread);
}

static gpointer
script_exec_main(gpointer user_data)
{
  struct objlist *shell;
  struct narray *sarray;
  GThread *thread;

  sarray = (struct narray *) user_data;
  shell = chkobject("shell");
  if (shell) {
    int id;
    id = newobj(shell);
    if (id >= 0) {
      int allocnow;
      char *argv[2];
      if (Menulocal.addinconsole) {
        allocnow = allocate_console();
      }
      argv[0] = (char *) sarray;
      argv[1] = NULL;
      exeobj(shell, "shell", id, 1, argv);

      delobj(shell, id);
      if (Menulocal.addinconsole) {
        free_console(allocnow);
      }
    }
  }
  arrayfree2(sarray);
  thread = g_thread_self();
  g_idle_add_once(script_exec_finalize, thread);
  return NULL;
}

void
script_exec(GtkWidget *w, gpointer client_data)
{
  char *name, *option, *s, mes[256];
  int len;
  struct narray *sarray;
  struct script *fcur;

  if (Menulock || Globallock || client_data == NULL) {
    return;
  }

  fcur = (struct script *) client_data;
  if (fcur->script == NULL)
    return;

  name = g_strdup(fcur->script);
  if (name == NULL)
    return;

  sarray = arraynew(sizeof(char *));
  if (sarray == NULL) {
    g_free(name);
    return;
  }
  if (arrayadd(sarray, &name) == NULL) {
    g_free(name);
    arrayfree2(sarray);
    return;
  }

  option = fcur->option;
  while ((s = getitok2(&option, &len, " \t")) != NULL) {
    if (arrayadd(sarray, &s) == NULL) {
      g_free(s);
      arrayfree2(sarray);
      return;
    }
  }

  snprintf(mes, sizeof(mes), _("Executing `%.128s'."), name);
  SetStatusBar(mes);

  menu_lock(TRUE);
  gtk_widget_set_sensitive(TopLevel, FALSE);

  menu_save_undo(UNDO_TYPE_ADDIN, NULL);
  g_thread_new(NULL, script_exec_main, sarray);
}
#else
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
#endif

static void
CmViewerButtonArm(GtkWidget *action, gpointer client_data)
{
  int mode = PointB;
  struct Viewer *d;

  d = &NgraphApp.Viewer;

  if (! gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(action))) {
    return;
  }

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
    NSetCursor(GDK_PLUS);
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

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
#include "gtk_action.h"
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
#include "x11commn.h"
#include "x11lgnd.h"
#include "x11axis.h"
#include "x11file.h"
#include "x11graph.h"
#include "x11print.h"
#include "x11opt.h"
#include "x11view.h"

#define DUMMY_AXISGRID N_("axisgrid") /* only for translation */
#define DUMMY_SHORTCUT N_("_Keyboard shortcus") /* only for translation */

#define TEXT_HISTORY     "text_history"
#define MATH_X_HISTORY   "math_x_history"
#define MATH_Y_HISTORY   "math_y_history"
#define FUNCTION_HISTORY "function_history"
#define FIT_HISTORY      "fit_history"
#define KEYMAP_FILE      "accel_map"
#define FUNC_MATH_HISTORY   "func_math_history"
#define FUNC_FUNC_HISTORY   "func_func_history"

#define USE_EXT_DRIVER 0

#define SIDE_PANE_TAB_ID "side_pane"
int Menulock = FALSE, DnDLock = FALSE;
struct NgraphApp NgraphApp = {0};
GtkWidget *TopLevel = NULL, *DrawButton = NULL;
GtkAccelGroup *AccelGroup = NULL;

static GtkWidget *CurrentWindow = NULL, *CToolbar = NULL, *PToolbar = NULL, *SettingPanel = NULL, *ToolBox = NULL;
static enum {APP_CONTINUE, APP_QUIT, APP_QUIT_FORCE} Hide_window = APP_CONTINUE;
static int DrawLock = FALSE;
static unsigned int CursorType;
GtkApplication *GtkApp;

#if USE_EXT_DRIVER
static GtkWidget *ExtDrvOutMenu = NULL
#endif

struct MenuItem;
struct ToolItem;

static void create_menu(GtkWidget *w, struct MenuItem *item);
static void create_menu_sub(GtkWidget *parent, struct MenuItem *item, int popup);
static void create_popup(GtkWidget *parent, struct MenuItem *item);
static GtkWidget *create_toolbar(struct ToolItem *item, int n, GCallback btn_press_cb);
static void CmViewerButtonArm(GtkToggleToolButton *action, gpointer client_data);

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

#define CURSOR_TYPE_NUM (sizeof(Cursor) / sizeof(*Cursor))

static void clear_information(void *w, gpointer user_data);
static void toggle_view_cb(GtkCheckMenuItem *action, gpointer data);

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
  GtkWidget *menu, *popup;
  GtkToolItem *tool;
#if USE_GTK_BUILDER
  GAction *action;
#endif
  enum ACTION_TYPE type;
};

enum ActionWidgetIndex {
  GraphSaveAction,
  EditMenuAction,
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
  PopupUpdateAction,
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
  AddinAction,
  PointerModeBoth,
  PointerModeLegend,
  PointerModeAxis,
  PointerModeData,
  ActionWidgetNum,
};

struct ActionWidget ActionWidget[ActionWidgetNum];
static int DefaultMode = PointerModeBoth;

struct ToolItem {
  enum {
    TOOL_TYPE_NORMAL,
    TOOL_TYPE_DRAW,
    TOOL_TYPE_SAVE,
    TOOL_TYPE_TOGGLE,
    TOOL_TYPE_TOGGLE2,
    TOOL_TYPE_RADIO,
    TOOL_TYPE_RECENT_GRAPH,
    TOOL_TYPE_RECENT_DATA,
    TOOL_TYPE_SEPARATOR,
  } type;
  const char *label;
  const char *tip;
  const char *caption;
  const char *icon;
  const char *accel_path;
  guint accel_key;
  GdkModifierType accel_mods;
  struct MenuItem *child;
  GCallback callback;
  int user_data;
  struct ActionWidget *action;
  const char *action_name;
};

static struct ToolItem PointerToolbar[] = {
  {
    TOOL_TYPE_RADIO,
    N_("Point"),
    N_("Pointer"),
    N_("Legend and Axis Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"),
    NGRAPH_POINT_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    PointB,
    ActionWidget + PointerModeBoth,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Legend"),
    N_("Legend Pointer"),
    N_("Legend Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"),
    NGRAPH_LEGENDPOINT_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    LegendB,
    ActionWidget + PointerModeLegend,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Axis"),
    N_("Axis Pointer"),
    N_("Axis Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"),
    NGRAPH_AXISPOINT_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    AxisB,
    ActionWidget + PointerModeAxis,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Data"),
    N_("Data Pointer"),
    N_("Data Pointer"),
    NGRAPH_DATAPOINT_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    DataB,
    ActionWidget + PointerModeData,
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
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    PathB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Rectangle"),
    N_("Rectangle"),
    N_("New Legend Rectangle (+SHIFT: Fine +CONTROL: square integer ratio rectangle)"),
    NGRAPH_RECT_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    RectB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Arc"),
    N_("Arc"),
    N_("New Legend Arc (+SHIFT: Fine +CONTROL: circle or integer ratio ellipse)"),
    NGRAPH_ARC_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    ArcB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Mark"),
    N_("Mark"),
    N_("New Legend Mark (+SHIFT: Fine)"),
    NGRAPH_MARK_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    MarkB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Text"),
    N_("Text"),
    N_("New Legend Text (+SHIFT: Fine)"),
    NGRAPH_TEXT_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    TextB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Gauss"),
    N_("Gaussian"),
    N_("New Legend Gaussian (+SHIFT: Fine +CONTROL: integer ratio)"),
    NGRAPH_GAUSS_ICON,
    NULL,
    0,
    0,
    NULL,
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
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    FrameB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Section axis"),
    N_("Section Graph"),
    N_("New Section Graph (+SHIFT: Fine +CONTROL: integer ratio)"),
    NGRAPH_SECTION_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    SectionB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Cross axis"),
    N_("Cross Graph"),
    N_("New Cross Graph (+SHIFT: Fine +CONTROL: integer ratio)"),
    NGRAPH_CROSS_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    CrossB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Single axis"),
    N_("Single Axis"),
    N_("New Single Axis (+SHIFT: Fine +CONTROL: snap angle)"),
    NGRAPH_SINGLE_ICON,
    NULL,
    0,
    0,
    NULL,
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
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    TrimB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Evaluate"),
    N_("Evaluate Data"),
    N_("Evaluate Data Point"),
    NGRAPH_EVAL_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmViewerButtonArm),
    EvalB,
  },
  {
    TOOL_TYPE_RADIO,
    N_("Zoom"),
    N_("Viewer Zoom"),
    N_("Viewer Zoom-In (+CONTROL: Zoom-Out +SHIFT: Centering)"),
    NGRAPH_ZOOM_ICON,
    NULL,
    0,
    0,
    NULL,
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
    "<Ngraph>/Data/Add",
    GDK_KEY_o,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmFileOpen),
    0,
    NULL,
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
    "<Ngraph>/Graph/Load graph",
    GDK_KEY_r,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmGraphLoad),
    0,
    NULL,
    "app.GraphLoadAction",
  },
  {
    TOOL_TYPE_SAVE,
    N_("_Save"),
    N_("Save NGP"),
    N_("Save NGP file"),
    "document-save-symbolic",
    "<Ngraph>/Graph/Save",
    GDK_KEY_s,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmGraphOverWrite),
    0,
    NULL,
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
    "<Ngraph>/Edit/Copy",
    GDK_KEY_c,
    GDK_SHIFT_MASK | GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmAxisClear),
    0,
    ActionWidget + AxisScaleClearAction,
    "app.AxisScaleClearAction",
  },
  {
    TOOL_TYPE_DRAW,
    N_("_Draw"),
    N_("Draw"),
    N_("Draw on Viewer Window"),
    "ngraph_draw-symbolic",
    "<Ngraph>/View/Draw",
    GDK_KEY_d,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmViewerDraw),
    FALSE,
    NULL,
    "app.ViewDrawDirectAction",
  },
  {
    TOOL_TYPE_NORMAL,
    N_("_Print"),
    N_("Print"),
    N_("Print"),
    "document-print-symbolic",
    "<Ngraph>/Graph/Print",
    GDK_KEY_p,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmOutputPrinterB),
    0,
    NULL,
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
    NULL,
    G_CALLBACK(CmFileMath),
    0,
    ActionWidget + DataMathAction,
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
    NULL,
    G_CALLBACK(CmAxisScaleUndo),
    0,
    ActionWidget + AxisScaleUndoAction,
    "app.AxisScaleUndoAction",
  },
};

struct MenuItem {
  enum {
    MENU_TYPE_NORMAL,
    MENU_TYPE_TOGGLE,
    MENU_TYPE_TOGGLE2,
    MENU_TYPE_RADIO,
    MENU_TYPE_RECENT_GRAPH,
    MENU_TYPE_RECENT_DATA,
    MENU_TYPE_SEPARATOR,
    MENU_TYPE_END,
  } type;
  const char *label;
  const char *tip;
  const char *caption;
  const char *icon;
  const char *accel_path;
  guint accel_key;
  GdkModifierType accel_mods;
  struct MenuItem *child;
  GCallback callback;
  int user_data;
  struct ActionWidget *action;
  const char *action_name;
};

static struct MenuItem HelpMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Help"),
    NULL,
    N_("Show the help document"),
    "help-browser",
    "<Ngraph>/Help/Help",
    GDK_KEY_F1,
    0,
    NULL,
    G_CALLBACK(CmHelpHelp),
    0,
    NULL,
    "help",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Demonstration"),
    NULL,
    NULL,
    "help-demo",
    "<Ngraph>/Help/Demonstration",
    0,
    0,
    NULL,
    G_CALLBACK(CmHelpDemo),
    0,
    NULL,
    "demonstration",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_About"),
    NULL,
    NULL,
    "help-about",
    "<Ngraph>/Help/About",
    0,
    0,
    NULL,
    G_CALLBACK(CmHelpAbout),
    0,
    NULL,
    "about",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem PreferenceMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Viewer"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Preference/Viewer",
    0,
    0,
    NULL,
    G_CALLBACK(CmOptionViewer),
    0,
    NULL,
    "PreferenceViewerAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_External viewer"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Preference/External Viewer",
    0,
    0,
    NULL,
    G_CALLBACK(CmOptionExtViewer),
    0,
    NULL,
    "PreferenceExternalViewerAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Font aliases"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Preference/Font aliases",
    0,
    0,
    NULL,
    G_CALLBACK(CmOptionPrefFont),
    0,
    NULL,
    "PreferenceFontAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Add-in script"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Preference/Addin Script",
    0,
    0,
    NULL,
    G_CALLBACK(CmOptionScript),
    0,
    NULL,
    "PreferenceAddinAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Miscellaneous"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Preference/Miscellaneous",
    0,
    0,
    NULL,
    G_CALLBACK(CmOptionMisc),
    0,
    NULL,
    "PreferenceMiscAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("save as default (_Settings)"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Preference/save as default (Settings)",
    0,
    0,
    NULL,
    G_CALLBACK(CmOptionSaveDefault),
    0,
    NULL,
    "PreferenceSaveSettingAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("save as default (_Graph)"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Preference/save as default (Graph)",
    0,
    0,
    NULL,
    G_CALLBACK(CmOptionSaveNgp),
    0,
    NULL,
    "PreferenceSaveGraphAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Data file default"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Preference/Data file default",
    0,
    0,
    NULL,
    G_CALLBACK(CmOptionFileDef),
    0,
    NULL,
    "PreferenceDataDefaultAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Legend text default"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Preference/Legend text default",
    0,
    0,
    NULL,
    G_CALLBACK(CmOptionTextDef),
    0,
    NULL,
    "PreferenceTextDefaultAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem MergeMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Add"),
    NULL,
    NULL,
    "list-add",
    "<Ngraph>/Merge/Add",
    0,
    0,
    NULL,
    G_CALLBACK(CmMergeOpen),
    0,
    NULL,
    "MergeAddAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Properties"),
    NULL,
    NULL,
    "document-properties",
   "<Ngraph>/Merge/Property",
    0,
    0,
    NULL,
    G_CALLBACK(CmMergeUpdate),
    0,
    ActionWidget + MergePropertyAction,
    "MergePropertyAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Close"),
    NULL,
    NULL,
    "window-close",
    "<Ngraph>/Merge/Close",
    0,
    0,
    NULL,
    G_CALLBACK(CmMergeClose),
    0,
    ActionWidget + MergeCloseAction,
    "MergeCloseAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem LegendTextleMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Properties"),
    NULL,
    NULL,
    "document-properties",
    "<Ngraph>/Legend/Text/Property",
    0,
    0,
    NULL,
    G_CALLBACK(CmTextUpdate),
    0,
    ActionWidget + LegendTextPropertyAction,
    "LegendTextPropertyAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Delete"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Legend/Text/Delete",
    0,
    0,
    NULL,
    G_CALLBACK(CmTextDel),
    0,
    ActionWidget + LegendTextDeleteAction,
    "LegendTextDeleteAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem LegendMarkleMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Properties"),
    NULL,
    NULL,
    "document-properties",
    "<Ngraph>/Legend/Mark/Property",
    0,
    0,
    NULL,
    G_CALLBACK(CmMarkUpdate),
    0,
    ActionWidget + LegendMarkPropertyAction,
    "LegendMarkPropertyAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Delete"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Legend/Mark/Delete",
    0,
    0,
    NULL,
    G_CALLBACK(CmMarkDel),
    0,
    ActionWidget + LegendMarkDeleteAction,
    "LegendMarkDeleteAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem LegendArcMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Properties"),
    NULL,
    NULL,
    "document-properties",
    "<Ngraph>/Legend/Arc/Property",
    0,
    0,
    NULL,
    G_CALLBACK(CmArcUpdate),
    0,
    ActionWidget + LegendArcPropertyAction,
    "LegendArcPropertyAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Delete"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Legend/Arc/Delete",
    0,
    0,
    NULL,
    G_CALLBACK(CmArcDel),
    0,
    ActionWidget + LegendArcDeleteAction,
    "LegendArcDeleteAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem LegendRectangleMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Properties"),
    NULL,
    NULL,
    "document-properties",
    "<Ngraph>/Legend/Rectangle/Property",
    0,
    0,
    NULL,
    G_CALLBACK(CmRectUpdate),
    0,
    ActionWidget + LegendRectanglePropertyAction,
    "LegendRectanglePropertyAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Delete"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Legend/Rectangle/Delete",
    0,
    0,
    NULL,
    G_CALLBACK(CmRectDel),
    0,
    ActionWidget + LegendRectangleDeleteAction,
    "LegendRectangleDeleteAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem LegendPathMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Properties"),
    NULL,
    NULL,
    "document-properties",
    "<Ngraph>/Legend/Path/Property",
    0,
    0,
    NULL,
    G_CALLBACK(CmLineUpdate),
    0,
    ActionWidget + LegendPathPropertyAction,
    "LegendPathPropertyAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Delete"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Legend/Path/Delete",
    0,
    0,
    NULL,
    G_CALLBACK(CmLineDel),
    0,
    ActionWidget + LegendPathDeleteAction,
    "LegendPathDeleteAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem AxisGridMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Add"),
    NULL,
    NULL,
    "list-add",
    "<Ngraph>/Axis/Grid/Add",
    0,
    0,
    NULL,
    G_CALLBACK(CmAxisGridNew),
    0,
    NULL,
    "AxisGridNewAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Properties"),
    NULL,
    NULL,
    "document-properties",
    "<Ngraph>/Axis/Grid/Property",
    0,
    0,
    NULL,
    G_CALLBACK(CmAxisGridUpdate),
    0,
    ActionWidget + AxisGridPropertyAction,
    "AxisGridPropertyAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Delete"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Axis/Grid/Delete",
    0,
    0,
    NULL,
    G_CALLBACK(CmAxisGridDel),
    0,
    ActionWidget + AxisGridDeleteAction,
    "AxisGridDeleteAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem AxisAddMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Frame graph"),
    NULL,
    NULL,
    NGRAPH_FRAME_ICON,
    "<Ngraph>/Axis/Add/Frame graph",
    0,
    0,
    NULL,
    G_CALLBACK(CmAxisNewFrame),
    0,
    NULL,
    "AxisAddFrameAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Section graph"),
    NULL,
    NULL,
    NGRAPH_SECTION_ICON,
    "<Ngraph>/Axis/Add/Section graph",
    0,
    0,
    NULL,
    G_CALLBACK(CmAxisNewSection),
    0,
    NULL,
    "AxisAddSectionAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Cross graph"),
    NULL,
    NULL,
    NGRAPH_CROSS_ICON,
    "<Ngraph>/Axis/Add/Cross graph",
    0,
    0,
    NULL,
    G_CALLBACK(CmAxisNewCross),
    0,
    NULL,
    "AxisAddCrossAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Single _Axis"),
    NULL,
    NULL,
    NGRAPH_SINGLE_ICON,
    "<Ngraph>/Axis/Add/Single axis",
    0,
    0,
    NULL,
    G_CALLBACK(CmAxisNewSingle),
    0,
    NULL,
    "AxisAddSingleAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem AxisMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Add"),
    NULL,
    NULL,
    "list-add",
    NULL,
    0,
    0,
    AxisAddMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Properties"),
    NULL,
    NULL,
    "document-properties",
    "<Ngraph>/Axis/Property",
    0,
    0,
    NULL,
    G_CALLBACK(CmAxisUpdate),
    0,
    ActionWidget + AxisPropertyAction,
    "AxisPropertyAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Delete"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Axis/Delete",
    0,
    0,
    NULL,
    G_CALLBACK(CmAxisDel),
    0,
    ActionWidget + AxisDeleteAction,
    "AxisDeleteAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Scale _Zoom"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Axis/Scale Zoom",
    0,
    0,
    NULL,
    G_CALLBACK(CmAxisZoom),
    0,
    ActionWidget + AxisScaleZoomAction,
    "AxisScaleZoomAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Scale _Clear"),
    N_("Clear Scale"),
    N_("Clear Scale"),
    NGRAPH_SCALE_ICON,
    "<Ngraph>/Axis/Scale Clear",
    GDK_KEY_c,
    GDK_SHIFT_MASK | GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmAxisClear),
    0,
    ActionWidget + AxisScaleClearAction,
    "AxisScaleClearAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Scale _Undo"),
    N_("Scale Undo"),
    N_("Undo Scale Settings"),
    "edit-undo",
    "<Ngraph>/Axis/Scale Undo",
    0,
    0,
    NULL,
    G_CALLBACK(CmAxisScaleUndo),
    0,
    ActionWidget + AxisScaleUndoAction,
    "AxisScaleUndoAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Grid"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    AxisGridMenu,
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem PlotAddMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_File"),
    N_("Add data file"),
    N_("Add data file"),
    "text-x-generic",
    "<Ngraph>/Data/Add/Data",
    GDK_KEY_o,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmFileOpen),
    0,
    NULL,
    "DataAddFileAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Range"),
    N_("Add range plot"),
    NULL,
    NULL,
    "<Ngraph>/Data/Add/Range",
    0,
    0,
    NULL,
    G_CALLBACK(CmRangeAdd),
    0,
    NULL,
    "DataAddRangeAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_RECENT_DATA,
    N_("_Recent files"),
    NULL,
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem DataMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Add"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    PlotAddMenu,
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Properties"),
    NULL,
    NULL,
    "document-properties",
    "<Ngraph>/Data/Property",
    0,
    0,
    NULL,
    G_CALLBACK(CmFileUpdate),
    0,
    ActionWidget + DataPropertyAction,
    "DataPropertyAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Delete"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Data/Close",
    0,
    0,
    NULL,
    G_CALLBACK(CmFileClose),
    0,
    ActionWidget + DataCloseAction,
    "DataCloseAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Edit"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Data/Edit",
    0,
    0,
    NULL,
    G_CALLBACK(CmFileEdit),
    0,
    ActionWidget + DataEditAction,
    "DataEditAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Save data"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Data/Save data",
    0,
    0,
    NULL,
    G_CALLBACK(CmFileSaveData),
    0,
    ActionWidget + DataSaveAction,
    "DataSaveAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Math Transformation"),
    N_("Math Transformation"),
    N_("Set Math Transformation"),
    NGRAPH_MATH_ICON,
    "<Ngraph>/Data/Math",
    0,
    0,
    NULL,
    G_CALLBACK(CmFileMath),
    0,
    ActionWidget + DataMathAction,
    "DataMathAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem ObjectMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Data"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    DataMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Axis"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    AxisMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Path"),
    NULL,
    NULL,
    NGRAPH_LINE_ICON,
    NULL,
    0,
    0,
    LegendPathMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Rectangle"),
    NULL,
    NULL,
    NGRAPH_RECT_ICON,
    NULL,
    0,
    0,
    LegendRectangleMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Arc"),
    NULL,
    NULL,
    NGRAPH_ARC_ICON,
    NULL,
    0,
    0,
    LegendArcMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Mark"),
    NULL,
    NULL,
    NGRAPH_MARK_ICON,
    NULL,
    0,
    0,
    LegendMarkleMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Text"),
    NULL,
    NULL,
    NGRAPH_TEXT_ICON,
    NULL,
    0,
    0,
    LegendTextleMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Merge"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    MergeMenu,
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem ViewMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Draw"),
    N_("Draw"),
    N_("Draw on Viewer Window"),
    NGRAPH_DRAW_ICON,
    "<Ngraph>/View/Draw",
    GDK_KEY_d,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmViewerDraw),
    TRUE,
    NULL,
    "ViewDrawAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Clear information view"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/View/Clear information window",
    0,
    0,
    NULL,
    G_CALLBACK(clear_information),
    0,
    NULL,
    "ViewClearInformationWindowAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_TOGGLE,
    N_("_Sidebar"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/View/Sidebar",
    0,
    0,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleSidebar,
    ActionWidget + ViewSidebarAction,
    "ViewSidebarAction",
  },
  {
    MENU_TYPE_TOGGLE,
    N_("_Statusbar"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/View/Statusbar",
    0,
    0,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleStatusbar,
    ActionWidget + ViewStatusbarAction,
    "ViewStatusbarAction",
  },
  {
    MENU_TYPE_TOGGLE,
    N_("_Ruler"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/View/Ruler",
    0,
    0,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleRuler,
    ActionWidget + ViewRulerAction,
    "ViewRulerAction",
  },
  {
    MENU_TYPE_TOGGLE,
    N_("_Scrollbar"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/View/Scrollbar",
    0,
    0,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleScrollbar,
    ActionWidget + ViewScrollbarAction,
    "ViewScrollbarAction",
  },
  {
    MENU_TYPE_TOGGLE,
    N_("_Command toolbar"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/View/Command toolbar",
    0,
    0,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleCToolbar,
    ActionWidget + ViewCommandToolbarAction,
    "ViewCommandToolbarAction",
  },
  {
    MENU_TYPE_TOGGLE,
    N_("_Toolbox"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/View/Toolbox",
    0,
    0,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdTogglePToolbar,
    ActionWidget + ViewToolboxAction,
    "ViewToolboxAction",
  },
  {
    MENU_TYPE_TOGGLE,
    N_("cross _Gauge"),
    NULL,
    N_("Show cross gauge"),
    NULL,
    "<Ngraph>/View/cross Gauge",
    GDK_KEY_g,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleCrossGauge,
    ActionWidget + ViewCrossGaugeAction,
    "ViewCrossGaugeAction",
  },
  {
    MENU_TYPE_TOGGLE,
    N_("_Grid"),
    NULL,
    N_("Show grid"),
    NULL,
    "<Ngraph>/View/grid Line",
    0,
    0,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleGridLine,
    ActionWidget + ViewGridLineAction,
    "ViewGridLineAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem EditOrderMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Top"),
    NULL,
    N_("Rise selection to top"),
    "go-top",
    "<Ngraph>/Edit/draw Order/Top",
    GDK_KEY_Home,
    GDK_SHIFT_MASK,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditOrderTop,
    ActionWidget + EditOrderTopAction,
    "EditOrderTopAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Up"),
    NULL,
    N_("Rise selection one step"),
    "go-up",
    "<Ngraph>/Edit/draw Order/Up",
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditOrderUp,
    ActionWidget + EditOrderUpAction,
    "EditOrderUpAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Down"),
    NULL,
    N_("Lower selection one step"),
    "go-down",
    "<Ngraph>/Edit/draw Order/Down",
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditOrderDown,
    ActionWidget + EditOrderDownAction,
    "EditOrderDownAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Bottom"),
    NULL,
    N_("Lower selection to bottom"),
    "go-bottom",
    "<Ngraph>/Edit/draw Order/Bottom",
    GDK_KEY_End,
    GDK_SHIFT_MASK,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditOrderBottom,
    ActionWidget + EditOrderBottomAction,
    "EditOrderBottomAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem EditAlignMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("Align _Left"),
    NULL,
    NULL,
    NGRAPH_ALIGN_L_ICON,
    "<Ngraph>/Edit/Align/Left",
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignLeft,
    ActionWidget + EditAlignLeftAction,
    "EditAlignLeftAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Align _Horizontal Center"),
    NULL,
    NULL,
    NGRAPH_ALIGN_HC_ICON,
    "<Ngraph>/Edit/Align/Horizontal center",
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignHCenter,
    ActionWidget + EditAlignHCenterAction,
    "EditAlignHCenterAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Align _Right"),
    NULL,
    NULL,
    NGRAPH_ALIGN_R_ICON,
    "<Ngraph>/Edit/Align/Right",
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignRight,
    ActionWidget + EditAlignRightAction,
    "EditAlignRightAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("Align _Top"),
    NULL,
    NULL,
    NGRAPH_ALIGN_T_ICON,
    "<Ngraph>/Edit/Align/Top",
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignTop,
    ActionWidget + EditAlignTopAction,
    "EditAlignTopAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Align _Vertical Center"),
    NULL,
    NULL,
    NGRAPH_ALIGN_VC_ICON,
    "<Ngraph>/Edit/Align/Vertical center",
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignVCenter,
    ActionWidget + EditAlignVCenterAction,
    "EditAlignVCenterAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Align _Bottom"),
    NULL,
    NULL,
    NGRAPH_ALIGN_B_ICON,
    "<Ngraph>/Edit/Align/Bottom",
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignBottom,
    ActionWidget + EditAlignBottomAction,
    "EditAlignBottomAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem EditMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Redo"),
    NULL,
    N_("Redo previous command"),
    NULL,
    "<Ngraph>/Edit/Redo",
    GDK_KEY_y,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditRedo,
    ActionWidget + EditRedoAction,
    "EditRedoAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Undo"),
    NULL,
    N_("Undo previous command"),
    NULL,
    "<Ngraph>/Edit/Undo",
    GDK_KEY_z,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditUndo,
    ActionWidget + EditUndoAction,
    "EditUndoAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("Cu_t"),
    NULL,
    N_("Cut selected object to clipboard"),
    NULL,
    "<Ngraph>/Edit/Cut",
    GDK_KEY_x,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditCut,
    ActionWidget + EditCutAction,
    "EditCutAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Copy"),
    NULL,
    N_("Copy selected object to clipboard"),
    NULL,
    "<Ngraph>/Edit/Copy",
    GDK_KEY_c,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditCopy,
    ActionWidget + EditCopyAction,
    "EditCopyAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Paste"),
    NULL,
    N_("Paste object from clipboard"),
    NULL,
    "<Ngraph>/Edit/Paste",
    GDK_KEY_v,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditPaste,
    ActionWidget + EditPasteAction,
    "EditPasteAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Delete"),
    NULL,
    N_("Delete the selected object"),
    NULL,
    "<Ngraph>/Edit/Delete",
    GDK_KEY_Delete,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditDelete,
    ActionWidget + EditDeleteAction,
    "EditDeleteAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Duplicate"),
    NULL,
    N_("Duplicate the selected object"),
    NULL,
    "<Ngraph>/Edit/Duplicate",
    GDK_KEY_Insert,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditDuplicate,
    ActionWidget + EditDuplicateAction,
    "EditDuplicateAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Select _All"),
    NULL,
    N_("Select all objects"),
    NULL,
    "<Ngraph>/Edit/SelectAll",
    GDK_KEY_a,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditSelectAll,
    ActionWidget + EditSelectAllAction,
    "EditSelectAllAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("draw _Order"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    EditOrderMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Align"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    EditAlignMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("rotate _90 degree clockwise"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Edit/RotateCW",
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditRotateCW,
    ActionWidget + EditRotateCWAction,
    "EditRotateCWAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("rotate 9_0 degree counter-clockwise"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Edit/RotateCCW",
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditRotateCCW,
    ActionWidget + EditRotateCCWAction,
    "EditRotateCCWAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("flip _Horizontally"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Edit/FlipHorizontally",
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditFlipHorizontally,
    ActionWidget + EditFlipHAction,
    "EditFlipHAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("flip _Vertically"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Edit/FlipVertically",
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditFlipVertically,
    ActionWidget + EditFlipVAction,
    "EditFlipVActiopn",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem GraphNewMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Frame graph"),
    N_("Create frame graph"),
    NULL,
    NGRAPH_FRAME_ICON,
    "<Ngraph>/Graph/New graph/Frame graph",
    0,
    0,
    NULL,
    G_CALLBACK(CmGraphNewMenu),
    MenuIdGraphNewFrame,
    NULL,
    "GraphNewFrameAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Section graph"),
    N_("Create section graph"),
    NULL,
    NGRAPH_SECTION_ICON,
    "<Ngraph>/Graph/New graph/Section graph",
    0,
    0,
    NULL,
    G_CALLBACK(CmGraphNewMenu),
    MenuIdGraphNewSection,
    NULL,
    "GraphNewSectionAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Cross graph"),
    N_("Create cross graph"),
    NULL,
    NGRAPH_CROSS_ICON,
    "<Ngraph>/Graph/New graph/Cross graph",
    0,
    0,
    NULL,
    G_CALLBACK(CmGraphNewMenu),
    MenuIdGraphNewCross,
    NULL,
    "GraphNewCrossAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_All clear"),
    N_("Clear graph"),
    NULL,
    NULL,
    "<Ngraph>/Graph/New graph/All clear",
    0,
    0,
    NULL,
    G_CALLBACK(CmGraphNewMenu),
    MenuIdGraphAllClear,
    NULL,
    "GraphNewClearAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem GraphExportMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_GRA file"),
    N_("Export as GRA file"),
    NULL,
    NULL,
    "<Ngraph>/Graph/Export image/GRA File",
    0,
    0,
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputGRAFile,
    NULL,
    "GraphExportGRAAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_PS file"),
    N_("Export as PostScript file"),
    NULL,
    NULL,
    "<Ngraph>/Graph/Export image/PS File",
    0,
    0,
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputPSFile,
    NULL,
    "GraphExportPSAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_EPS file"),
    N_("Export as Encapsulate PostScript file"),
    NULL,
    NULL,
    "<Ngraph>/Graph/Export image/EPS File",
    0,
    0,
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputEPSFile,
    NULL,
    "GraphExportEPSAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("P_DF file"),
    N_("Export as Portable Document Format"),
    NULL,
    NULL,
    "<Ngraph>/Graph/Export image/PDF File",
    0,
    0,
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputPDFFile,
    NULL,
    "GraphExportPDFAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_SVG file"),
    N_("Export as Scalable Vector Graphics file"),
    NULL,
    NULL,
    "<Ngraph>/Graph/Export image/SVG File",
    0,
    0,
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputSVGFile,
    NULL,
    "GraphExportSVGAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("P_NG file"),
    N_("Export as Portable Network Graphics  file"),
    NULL,
    NULL,
    "<Ngraph>/Graph/Export image/PNG File",
    0,
    0,
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputPNGFile,
    NULL,
    "GraphExportPNGAction",
  },
#if WINDOWS
  {
    MENU_TYPE_NORMAL,
    N_("_EMF file"),
    N_("Export as Windows Enhanced Metafile"),
    NULL,
    NULL,
    "<Ngraph>/Graph/Export image/EMF File",
    0,
    0,
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputEMFFile,
    NULL,
    "GraphExportEMFAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Clipboard (EMF)"),
    N_("Copy to the clipboard as Windows Enhanced Metafile "),
    NULL,
    NULL,
    "<Ngraph>/Graph/Export image/Clipboard",
    0,
    0,
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputEMFClipboard,
    NULL,
    "GraphExportEMFClipboardAction",
  },
#endif	/* WINDOWS */
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem GraphMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_New graph"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    GraphNewMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Load graph"),
    N_("Load NGP"),
    N_("Load a graph (NGP file)"),
    "document-open",
    "<Ngraph>/Graph/Load graph",
    GDK_KEY_r,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmGraphLoad),
    0,
    NULL,
    "GraphLoadAction",
  },
  {
    MENU_TYPE_RECENT_GRAPH,
    N_("_Recent graphs"),
    NULL,
    NULL,
    NULL,
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Save"),
    N_("Save NGP"),
    N_("Save the graph"),
    "document-save",
    "<Ngraph>/Graph/Save",
    GDK_KEY_s,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmGraphOverWrite),
    0,
    ActionWidget + GraphSaveAction,
    "GraphSaveAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Save _As"),
    N_("Save NGP"),
    N_("Save the graph with a new filename"),
    "document-save-as",
    "<Ngraph>/Graph/SaveAs",
    GDK_KEY_s,
    GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    NULL,
    G_CALLBACK(CmGraphSave),
    0,
    NULL,
    "GraphSaveAsAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Export image"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    GraphExportMenu,
  },
  {
    MENU_TYPE_SEPARATOR,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Draw order"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Graph/Draw order",
    0,
    0,
    NULL,
    G_CALLBACK(CmGraphSwitch),
    0,
    NULL,
    "GraphDrawOrderAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("Page Set_up"),
    NULL,
    NULL,
    "document-page-setup",
    "<Ngraph>/Graph/Page",
    0,
    0,
    NULL,
    G_CALLBACK(CmGraphPage),
    0,
    NULL,
    "GraphPageSetupAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Pre_view"),
    N_("Print preview"),
    N_("Print preview"),
    NULL,
    "<Ngraph>/Graph/Print preview",
    0,
    0,
    NULL,
    G_CALLBACK(CmOutputViewerB),
    0,
    NULL,
    "GraphPrintPreviewAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Print"),
    N_("Print"),
    N_("Print the graph"),
    "document-print",
    "<Ngraph>/Graph/Print",
    GDK_KEY_p,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmOutputPrinterB),
    0,
    NULL,
    "GraphPrintAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Current directory"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Graph/Current directory",
    0,
    0,
    NULL,
    G_CALLBACK(CmGraphDirectory),
    0,
    NULL,
    "GraphCurrentDirectoryAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Ngraph shell"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Graph/Ngraph shell",
    0,
    0,
    NULL,
    G_CALLBACK(CmGraphShell),
    0,
    NULL,
    "GraphShellAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Add-in"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    NULL,
    NULL,
    0,
    ActionWidget + AddinAction,
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Quit"),
    NULL,
    N_("Quit the application"),
    "application-exit",
    "<Ngraph>/Graph/Quit",
    GDK_KEY_q,
    GDK_CONTROL_MASK,
    NULL,
    G_CALLBACK(CmGraphQuit),
    0,
    NULL,
    "quit",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem MainMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Graph"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    GraphMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Edit"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    EditMenu,
    NULL,
    0,
    ActionWidget + EditMenuAction,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_View"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    ViewMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Object"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    ObjectMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Preference"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    PreferenceMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Help"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    HelpMenu,
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem SaveMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("_Save"),
    N_("Save NGP"),
    N_("Save the graph"),
    "document-save",
    "<Ngraph>/Graph/Save",
    GDK_KEY_s,
    GDK_CONTROL_MASK,
    NULL,
#if USE_GTK_BUILDER
    NULL,
#else
    G_CALLBACK(CmGraphOverWrite),
#endif
    0,
    ActionWidget + GraphSaveAction,
    "GraphSaveAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Save _As"),
    N_("Save NGP"),
    NULL,
    "document-save-as",
    "<Ngraph>/Graph/SaveAs",
    GDK_KEY_s,
    GDK_CONTROL_MASK | GDK_SHIFT_MASK,
    NULL,
    G_CALLBACK(CmGraphSave),
    0,
    NULL,
    "GraphSaveAsAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Export image"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    GraphExportMenu,
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Save data"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Data/Save data",
    0,
    0,
    NULL,
#if USE_GTK_BUILDER
    NULL,
#else
    G_CALLBACK(CmFileSaveData),
#endif
    0,
    ActionWidget + DataSaveAction,
    "DataSaveAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem PopupRotateMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("rotate _90 degree clockwise"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditRotateCW,
    ActionWidget + EditRotateCWAction,
    "EditRotateCWAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("rotate 9_0 degree counter-clockwise"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditRotateCCW,
    ActionWidget + EditRotateCCWAction,
    "EditRotateCCWAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem PopupFlipMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("flip _Horizontally"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditFlipHorizontally,
    ActionWidget + EditFlipHAction,
    "EditFlipHAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("flip _Vertically"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditFlipVertically,
    ActionWidget + EditFlipVAction,
    "EditFlipVActiopn",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem PopupAlignMenu[] = {
  {
    MENU_TYPE_NORMAL,
    N_("Align _Left"),
    NULL,
    NULL,
    NGRAPH_ALIGN_L_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignLeft,
    ActionWidget + EditAlignLeftAction,
    "EditAlignLeftAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Align _Horizontal Center"),
    NULL,
    NULL,
    NGRAPH_ALIGN_HC_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignHCenter,
    ActionWidget + EditAlignHCenterAction,
    "EditAlignHCenterAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Align _Right"),
    NULL,
    NULL,
    NGRAPH_ALIGN_R_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignRight,
    ActionWidget + EditAlignRightAction,
    "EditAlignRightAction",
  },
  {
    MENU_TYPE_SEPARATOR,
    NULL,
  },
  {
    MENU_TYPE_NORMAL,
    N_("Align _Top"),
    NULL,
    NULL,
    NGRAPH_ALIGN_T_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignTop,
    ActionWidget + EditAlignTopAction,
    "EditAlignTopAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Align _Vertical Center"),
    NULL,
    NULL,
    NGRAPH_ALIGN_VC_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignVCenter,
    ActionWidget + EditAlignVCenterAction,
    "EditAlignVCenterAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("Align _Bottom"),
    NULL,
    NULL,
    NGRAPH_ALIGN_B_ICON,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignBottom,
    ActionWidget + EditAlignBottomAction,
    "EditAlignBottomAction",
  },
  {
    MENU_TYPE_END,
  },
};

static struct MenuItem PopupMenu[] = {
  {
    MENU_TYPE_NORMAL,
    "Cu_t",
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditCut,
    ActionWidget + EditCutAction,
    "EditCutAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Copy"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditCopy,
    ActionWidget + EditCopyAction,
    "EditCopyAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Paste"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditPaste,
    ActionWidget + EditPasteAction,
    "EditPasteAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Delete"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditDelete,
    ActionWidget + EditDeleteAction,
    "EditDeleteAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Duplicate"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditDuplicate,
    ActionWidget + EditDuplicateAction,
    "EditDuplicateAction",
  },
  {
    MENU_TYPE_SEPARATOR,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Properties"),
    NULL,
    NULL,
    NULL,
    "<Ngraph>/Popup/Update",
    0,
    0,
    NULL,
    G_CALLBACK(ViewerUpdateCB),
    0,
    ActionWidget + PopupUpdateAction,
    "PopupUpdateAction",
  },
  {
    MENU_TYPE_SEPARATOR,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Align"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    PopupAlignMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Rotate"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    PopupRotateMenu,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Flip"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    PopupFlipMenu,
  },
  {
    MENU_TYPE_SEPARATOR,
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Top"),
    NULL,
    NULL,
    "go-top",
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditOrderTop,
    ActionWidget + EditOrderTopAction,
    "EditOrderTopAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Up"),
    NULL,
    NULL,
    "go-up",
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditOrderUp,
    ActionWidget + EditOrderUpAction,
    "EditOrderUpAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Down"),
    NULL,
    NULL,
    "go-down",
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditOrderDown,
    ActionWidget + EditOrderDownAction,
    "EditOrderDownAction",
  },
  {
    MENU_TYPE_NORMAL,
    N_("_Bottom"),
    NULL,
    NULL,
    "go-bottom",
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditOrderBottom,
    ActionWidget + EditOrderBottomAction,
    "EditOrderBottomAction",
  },
  {
    MENU_TYPE_SEPARATOR,
  },
  {
    MENU_TYPE_TOGGLE,
    N_("cross _Gauge"),
    NULL,
    NULL,
    NULL,
    NULL,
    0,
    0,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleCrossGauge,
    ActionWidget + ViewCrossGaugeAction,
    "ViewCrossGaugeAction",
  },
  {
    MENU_TYPE_END,
  },
};


void
set_pointer_mode(int id)
{
  if (id < 0) {
    id = DefaultMode;
  }

  switch (id) {
  case PointerModeBoth:
  case PointerModeLegend:
  case PointerModeAxis:
  case PointerModeData:
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(ActionWidget[id].tool), TRUE);
    break;
  }
}

static void
init_action_widget_list(void)
{
  int i;

  for (i = 0; i < ActionWidgetNum; i++) {
    ActionWidget[i].menu = NULL;
    ActionWidget[i].tool = NULL;
    ActionWidget[i].popup = NULL;
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
    case PopupUpdateAction:
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

  if (NgraphApp.Viewer.menu == NULL) {
    return;
  }

  gtk_widget_set_sensitive(NgraphApp.Viewer.menu, ! Menulock);
  w = gtk_paned_get_child1(GTK_PANED(NgraphApp.Viewer.main_pane));
  if (w) {
    gtk_widget_set_sensitive(w, ! Menulock);
  }
}

static void
set_action_widget_sensitivity(int id, int state)
{
#if USE_GTK_BUILDER
  if (ActionWidget[id].action) {
    g_simple_action_set_enabled(G_SIMPLE_ACTION(ActionWidget[id].action), state);
  }
#else
  if (ActionWidget[id].menu) {
    gtk_widget_set_sensitive(ActionWidget[id].menu, state);
  }
  if (ActionWidget[id].tool) {
    gtk_widget_set_sensitive(GTK_WIDGET(ActionWidget[id].tool), state);
  }
  if (ActionWidget[id].popup) {
    gtk_widget_set_sensitive(ActionWidget[id].popup, state);
  }
#endif
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
  Hide_window = APP_CONTINUE;
  while (TRUE) {
    gtk_main_iteration();
    if (Hide_window != APP_CONTINUE && ! gtk_events_pending()) {
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
  while (gtk_events_pending()) {
    gtk_main_iteration();
  }
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


#if USE_GTK_BUILDER
#define ADDIN_MENU_SECTION_INDEX 6
static void
add_addin_menu(void)
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
    }
    fcur = fcur->next;
    i++;
  }

  g_menu_prepend_submenu(G_MENU(menu), _("_Add-in"), G_MENU_MODEL(addin_menu));
}
#endif

void
create_addin_menu(void)
{
  GtkWidget *menu, *item, *parent;
  struct script *fcur;

  parent = ActionWidget[AddinAction].menu;
  if (parent == NULL) {
    return;
  }

  gtk_widget_set_sensitive(parent, Menulocal.scriptroot != NULL);
  if (Menulocal.scriptroot == NULL) {
    return;
  }

  menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(parent));
  if (menu) {
    gtk_widget_destroy(menu);
  }

  menu = gtk_menu_new();

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

#if USE_GTK_BUILDER
  add_addin_menu();
#endif
}

static void
set_focus_sensitivity_sub(const struct Viewer *d, int insensitive)
{
  int i, num, type, state, up_state, down_state;
  GtkClipboard *clip;

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
	  clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	  state = gtk_clipboard_wait_is_text_available(clip);
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

static void
clear_information(void *w, gpointer user_data)
{
  InfoWinClear();
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

#if ! USE_GTK_BUILDER
static void
read_keymap_file(void)
{
  char *home, *filename;

  home = get_home();
  if (home == NULL) {
    filename = g_strdup_printf("%s/%s", home, KEYMAP_FILE);
    if (naccess(filename, R_OK) == 0) {
      gtk_accel_map_load(filename);
      g_free(filename);
      return;
    }
    g_free(filename);
  }

  filename = g_strdup_printf("%s/%s", CONFDIR, KEYMAP_FILE);
  if (naccess(filename, R_OK) == 0) {
    gtk_accel_map_load(filename);
  }
  g_free(filename);
}
#endif

static void
create_markpixmap(GtkWidget *win)
{
  cairo_surface_t *pix;
  int gra, i, R, G, B, R2, G2, B2, found, output;
  struct objlist *obj, *robj;
  N_VALUE *inst;
  struct gra2cairo_local *local;
  GdkWindow *window;

  R = G = B = 0;
  R2 = 0;
  G2 = B2 = 255;

  window = gtk_widget_get_window(win);
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
  GList *tmp, *list = NULL;
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

  if (list) {
    gtk_window_set_default_icon_list(list);
    gtk_window_set_icon_list(GTK_WINDOW(TopLevel), list);

    tmp = list;
    while (tmp) {
      g_object_unref(tmp->data);
      tmp = tmp->next;
    }
    g_list_free(list);
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
    NgraphApp.cursor[i] = gdk_cursor_new_for_display(gdk_display_get_default(), Cursor[i]);
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

static GtkWidget *
create_message_box(GtkWidget **label1, GtkWidget **label2)
{
  GtkWidget *frame, *w, *hbox;

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  w = gtk_label_new(NULL);
  gtk_widget_set_halign(w, GTK_ALIGN_END);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
  *label1 = w;

  w = gtk_label_new(NULL);
  gtk_widget_set_halign(w, GTK_ALIGN_START);
  gtk_label_set_width_chars(GTK_LABEL(w), 16);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
  *label2 = w;

  gtk_container_add(GTK_CONTAINER(frame), hbox);

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

  icon = gtk_image_new_from_icon_name(icon_name, GTK_ICON_SIZE_LARGE_TOOLBAR);
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
  };

  n = sizeof(tab_info) / sizeof(*tab_info);
  init_tab_info(tab_info, n);

  for (i = 0; i < n; i++) {
    GtkWidget *tab;
    tab = gtk_paned_get_child1(GTK_PANED(NgraphApp.Viewer.side_pane2));
    save_tab_position_sub(tab, tab_info + i, 0);

    tab = gtk_paned_get_child2(GTK_PANED(NgraphApp.Viewer.side_pane2));
    save_tab_position_sub(tab, tab_info + i, 100);
  }
}

static void
create_object_tabs(void)
{
  int j, tab_n;
  GtkWidget *tab;
  struct obj_tab_info tab_info[] = {
    {0, 0, &Menulocal.file_tab,      0, "data",      dreate_data_list,  &NgraphApp.FileWin,  NGRAPH_FILEWIN_ICON},
    {0, 0, &Menulocal.axis_tab,      0, "axis",      dreate_axis_list,  &NgraphApp.AxisWin,  NGRAPH_AXISWIN_ICON},
    {0, 0, &Menulocal.merge_tab,     0, "merge",     dreate_merge_list, &NgraphApp.MergeWin, NGRAPH_MERGEWIN_ICON},
    {0, 0, &Menulocal.path_tab,      0, "path",      create_path_list,  &NgraphApp.PathWin,  NGRAPH_LINE_ICON},
    {0, 0, &Menulocal.rectangle_tab, 0, "rectangle", create_rect_list,  &NgraphApp.RectWin,  NGRAPH_RECT_ICON},
    {0, 0, &Menulocal.arc_tab,       0, "arc",       create_arc_list,   &NgraphApp.ArcWin,   NGRAPH_ARC_ICON},
    {0, 0, &Menulocal.mark_tab,      0, "mark",      create_mark_list,  &NgraphApp.MarkWin,  NGRAPH_MARK_ICON},
    {0, 0, &Menulocal.text_tab,      0, "text",      create_text_list,  &NgraphApp.TextWin,  NGRAPH_TEXT_ICON},
  };

  tab_n = sizeof(tab_info) / sizeof(*tab_info);
  init_tab_info(tab_info, tab_n);

  for (j = 0; j < tab_n; j++) {
    if (tab_info[j].tab > 0) {
      tab = gtk_paned_get_child2(GTK_PANED(NgraphApp.Viewer.side_pane2));
    } else {
      tab = gtk_paned_get_child1(GTK_PANED(NgraphApp.Viewer.side_pane2));
    }
    tab_info[j].init_func(tab_info[j].d);
    setup_object_tab(tab_info[j].d, tab, tab_info[j].icon, _(tab_info[j].obj_name));
  }

  CoordWinCreate(&NgraphApp.CoordWin);
  gtk_paned_pack1(GTK_PANED(NgraphApp.Viewer.side_pane3), NgraphApp.CoordWin.Win, FALSE, TRUE);
  InfoWinCreate(&NgraphApp.InfoWin);
  gtk_paned_pack2(GTK_PANED(NgraphApp.Viewer.side_pane3), NgraphApp.InfoWin.Win, TRUE, TRUE);

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

#if USE_GTK_BUILDER
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
  gtk_toolbar_set_style(GTK_TOOLBAR(w), GTK_TOOLBAR_ICONS);

#if USE_APP_HEADER_BAR
  hbar = gtk_header_bar_new();
  gtk_header_bar_set_title(GTK_HEADER_BAR(hbar), AppName);
  gtk_header_bar_set_show_close_button(GTK_HEADER_BAR(hbar), TRUE);
  gtk_window_set_titlebar(GTK_WINDOW(window), hbar);
#endif
  w = create_toolbar(PointerToolbar, sizeof(PointerToolbar) / sizeof(*PointerToolbar), G_CALLBACK(CmViewerButtonPressed));
  PToolbar = w;
  gtk_orientable_set_orientation(GTK_ORIENTABLE(w), GTK_ORIENTATION_VERTICAL);
  gtk_toolbar_set_style(GTK_TOOLBAR(w), GTK_TOOLBAR_ICONS);
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

  w = gtk_menu_bar_new();
  create_menu(w, MainMenu);
#if ! USE_GTK_BUILDER
  gtk_box_pack_start(GTK_BOX(vbox2), w, FALSE, FALSE, 0);
  read_keymap_file();
#endif
  NgraphApp.Viewer.menu = w;

  ToolBox = gtk_stack_new();
  SettingPanel = presetting_create_panel(app);
  gtk_stack_add_named(GTK_STACK(ToolBox), CToolbar, "CommandToolbar");
  gtk_stack_add_named(GTK_STACK(ToolBox), SettingPanel, "SettingPanel");
  gtk_box_pack_start(GTK_BOX(vbox2), ToolBox, FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(hbox), PToolbar, FALSE, FALSE, 0);

  w = gtk_menu_new();
  create_popup(w, PopupMenu);
#if ! USE_GTK_BUILDER
  gtk_menu_set_accel_group(GTK_MENU(w), AccelGroup);
  NgraphApp.Viewer.popup = w;
  gtk_widget_show_all(w);
  g_signal_connect(ActionWidget[EditMenuAction].menu, "show", G_CALLBACK(edit_menu_shown), &NgraphApp.Viewer);
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
  gtk_paned_add1(GTK_PANED(vpane2), w);

  w = gtk_notebook_new();
  gtk_notebook_popup_enable(GTK_NOTEBOOK(w));
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w), GTK_POS_LEFT);
  gtk_notebook_set_group_name(GTK_NOTEBOOK(w), SIDE_PANE_TAB_ID);
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(w), TRUE);
  gtk_paned_add2(GTK_PANED(vpane2), w);

  hpane2 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
  NgraphApp.Viewer.side_pane3 = hpane2;

  vpane1 = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
  gtk_paned_pack1(GTK_PANED(vpane1), vpane2, TRUE, TRUE);
  gtk_paned_pack2(GTK_PANED(vpane1), hpane2, FALSE, TRUE);
  NgraphApp.Viewer.side_pane1 = vpane1;

  hpane1 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
#if USE_APP_HEADER_BAR
  gtk_paned_add1(GTK_PANED(hpane1), hbox);
#else
  gtk_paned_add1(GTK_PANED(hpane1), vbox);
#endif
  gtk_paned_add2(GTK_PANED(hpane1), vpane1);
  NgraphApp.Viewer.main_pane = hpane1;

  gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);
#if ! USE_APP_HEADER_BAR
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
#endif
  gtk_box_pack_start(GTK_BOX(hbox2), hpane1, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox2), hbox2, TRUE, TRUE, 0);

  NgraphApp.Message = gtk_statusbar_new();
  gtk_box_pack_end(GTK_BOX(NgraphApp.Message),
		   create_message_box(&NgraphApp.Message_extra, &NgraphApp.Message_pos),
		   FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox2), NgraphApp.Message, FALSE, FALSE, 0);

  NgraphApp.Message1 = gtk_statusbar_get_context_id(GTK_STATUSBAR(NgraphApp.Message), "Message1");

  set_axis_undo_button_sensitivity(FALSE);

  gtk_container_add(GTK_CONTAINER(TopLevel), vbox2);

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

static void
toggle_view_cb(GtkCheckMenuItem *action, gpointer data)
{
  int type, state;

  if (action == NULL) {
    return;
  }
  type = GPOINTER_TO_INT(data);
  state = gtk_check_menu_item_get_active(action);

  toggle_view(type, state);
}

void
set_toggle_action_widget_state(int id, int state)
{
#if USE_GTK_BUILDER
  if (ActionWidget[id].action) {
    GVariant *value;

    value = g_variant_new_boolean(state);
    g_action_change_state(ActionWidget[id].action, value);
  }
#else
  if (ActionWidget[id].menu) {
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ActionWidget[id].menu), state);
  }
  if (ActionWidget[id].popup) {
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(ActionWidget[id].popup), state);
  }
  if (ActionWidget[id].tool) {
    gtk_toggle_tool_button_set_active(GTK_TOGGLE_TOOL_BUTTON(ActionWidget[id].tool), state);
  }
#endif
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
#if ! USE_GTK_BUILDER
    set_toggle_action_widget_state(i, ! state);
#endif
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

void
show_recent_dialog(int type)
{
#if USE_GTK_BUILDER
  GtkWidget *dialog;
  int res;
  char *title;

  if (Menulock || Globallock) {
    return;
  }

  title = (type == RECENT_TYPE_GRAPH) ? _("Recent Graphs") : _("Recent Data Files");
  dialog = gtk_recent_chooser_dialog_new_for_manager(title,
						     GTK_WINDOW(TopLevel),
						     NgraphApp.recent_manager,
						     _("_Cancel"),
						     GTK_RESPONSE_CANCEL,
						     _("_Open"),
						     GTK_RESPONSE_ACCEPT,
						     NULL);

  create_recent_filter(dialog, type);
  res = gtk_dialog_run (GTK_DIALOG (dialog));
  gtk_widget_hide(dialog);
  if (res == GTK_RESPONSE_ACCEPT) {
    switch (type) {
    case RECENT_TYPE_GRAPH:
      CmGraphHistory(GTK_RECENT_CHOOSER(dialog), NULL);
      break;
    case RECENT_TYPE_DATA:
      CmFileHistory(GTK_RECENT_CHOOSER(dialog), NULL);
      break;
    }
  }

  gtk_widget_destroy(dialog);
#endif
}

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

static GtkWidget*
create_save_menu(void)
{
  GtkWidget *menu;
#if USE_GTK_BUILDER
  int i;
  char *action;
#endif

  menu = gtk_menu_new();
  create_menu_sub(menu, SaveMenu, TRUE);
#if USE_GTK_BUILDER
  for (i = 0; SaveMenu[i].type != MENU_TYPE_END; i++) {
    if (SaveMenu[i].action_name && SaveMenu[i].action) {
      action = g_strdup_printf("app.%s", SaveMenu[i].action_name);
      if (action) {
	gtk_actionable_set_action_name(GTK_ACTIONABLE(SaveMenu[i].action->popup), action);
	g_free(action);
      }
    }
  }
#endif
  gtk_widget_show_all(menu);
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
  gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(DrawButton), icon_name);
  gtk_widget_queue_draw(DrawButton);
}

static GtkWidget *
create_toolbar(struct ToolItem *item, int n, GCallback btn_press_cb)
{
  int i;
  GSList *list;
  GtkToolItem *widget;
  GtkWidget *toolbar, *menu;

  toolbar = gtk_toolbar_new();
  list = NULL;
  for (i = 0; i < n; i++) {
    switch (item[i].type) {
    case TOOL_TYPE_SEPARATOR:
      widget = gtk_separator_tool_item_new();
      gtk_separator_tool_item_set_draw(GTK_SEPARATOR_TOOL_ITEM(widget), TRUE);
      break;
    case TOOL_TYPE_NORMAL:
      widget = gtk_tool_button_new(NULL, _(item[i].label));
      break;
    case TOOL_TYPE_DRAW:
      widget = gtk_tool_button_new(NULL, _(item[i].label));
      DrawButton = GTK_WIDGET(widget);
      break;
    case TOOL_TYPE_SAVE:
      widget = gtk_menu_tool_button_new(NULL, _(item[i].label));
      menu = create_save_menu();
      gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(widget), menu);
      gtk_menu_tool_button_set_arrow_tooltip_text(GTK_MENU_TOOL_BUTTON(widget), _("Save menu"));
      break;
    case TOOL_TYPE_RECENT_GRAPH:
      widget = gtk_menu_tool_button_new(NULL, _(item[i].label));
      menu = create_recent_menu(RECENT_TYPE_GRAPH);
      gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(widget), menu);
      gtk_menu_tool_button_set_arrow_tooltip_text(GTK_MENU_TOOL_BUTTON(widget), _("Recent Graphs"));
      break;
    case TOOL_TYPE_RECENT_DATA:
      widget = gtk_menu_tool_button_new(NULL, _(item[i].label));
      menu = create_recent_menu(RECENT_TYPE_DATA);
      gtk_menu_tool_button_set_menu(GTK_MENU_TOOL_BUTTON(widget), menu);
      gtk_menu_tool_button_set_arrow_tooltip_text(GTK_MENU_TOOL_BUTTON(widget), _("Recent Data Files"));
      break;
    case TOOL_TYPE_TOGGLE:
    case TOOL_TYPE_TOGGLE2:
      widget = gtk_toggle_tool_button_new();
      break;
    case TOOL_TYPE_RADIO:
      widget = gtk_radio_tool_button_new(list);
      list = gtk_radio_tool_button_get_group(GTK_RADIO_TOOL_BUTTON(widget));
      gtk_tool_button_set_label(GTK_TOOL_BUTTON(widget), _(item[i].label));
      if (btn_press_cb) {
	g_signal_connect(gtk_bin_get_child(GTK_BIN(widget)), "button-press-event", btn_press_cb, NULL);
      }
      break;
    default:
      widget = NULL;
    }

    if (widget == NULL) {
      continue;
    }

    if (item[i].icon) {
      gtk_tool_button_set_icon_name(GTK_TOOL_BUTTON(widget), item[i].icon);
    }

#if USE_GTK_BUILDER
    if (item[i].action_name) {
      gtk_actionable_set_action_name(GTK_ACTIONABLE(widget), item[i].action_name);
    } else
#endif
    if (item[i].callback) {
      switch (item[i].type) {
      case TOOL_TYPE_TOGGLE:
	g_signal_connect(widget, "toggled", G_CALLBACK(item[i].callback), GINT_TO_POINTER(item[i].user_data));
	break;
      default:
	g_signal_connect(widget, "clicked", G_CALLBACK(item[i].callback), GINT_TO_POINTER(item[i].user_data));
      }
    }

    if (item[i].tip) {
      gtk_tool_item_set_tooltip_text(GTK_TOOL_ITEM(widget), _(item[i].tip));
    }

    if (item[i].caption) {
      g_signal_connect(gtk_bin_get_child(GTK_BIN(widget)),
		       "enter-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb),
		       _(item[i].caption));

      g_signal_connect(gtk_bin_get_child(GTK_BIN(widget)),
		       "leave-notify-event",
		       G_CALLBACK(tool_button_enter_leave_cb), NULL);
    }

    if (item[i].action) {
      item[i].action->tool = widget;
    }

    gtk_toolbar_insert(GTK_TOOLBAR(toolbar), GTK_TOOL_ITEM(widget), -1);
  }

  return toolbar;
}

static void
create_menu_sub(GtkWidget *parent, struct MenuItem *item, int popup)
{
  int i;
  GtkWidget *menu, *widget, *submenu;

  for (i = 0; item[i].type != MENU_TYPE_END; i++) {
    switch (item[i].type) {
    case MENU_TYPE_SEPARATOR:
      widget = gtk_separator_menu_item_new();
      break;
    case MENU_TYPE_NORMAL:
      widget = gtk_menu_item_new_with_mnemonic(_(item[i].label));
      break;
    case MENU_TYPE_TOGGLE:
    case MENU_TYPE_TOGGLE2:
      widget = gtk_check_menu_item_new_with_mnemonic(_(item[i].label));
      break;
    case MENU_TYPE_RECENT_GRAPH:
      widget = gtk_menu_item_new_with_mnemonic(_(item[i].label));
      submenu = create_recent_menu(RECENT_TYPE_GRAPH);
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(widget), submenu);
      break;
    case MENU_TYPE_RECENT_DATA:
      widget = gtk_menu_item_new_with_mnemonic(_(item[i].label));
      submenu = create_recent_menu(RECENT_TYPE_DATA);
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(widget), submenu);
      break;
    default:
      widget = NULL;
    }

    if (widget == NULL) {
      continue;
    }

    if (item[i].callback) {
      switch (item[i].type) {
      case MENU_TYPE_TOGGLE:
	g_signal_connect(widget, "toggled", G_CALLBACK(item[i].callback), GINT_TO_POINTER(item[i].user_data));
	break;
      default:
	g_signal_connect(widget, "activate", G_CALLBACK(item[i].callback), GINT_TO_POINTER(item[i].user_data));
      }
    }

#if ! USE_GTK_BUILDER
    if (item[i].accel_path) {
      gtk_accel_map_add_entry(item[i].accel_path, item[i].accel_key, item[i].accel_mods);
      gtk_menu_item_set_accel_path(GTK_MENU_ITEM(widget), item[i].accel_path);
    }
#endif

    if (item[i].child) {
      menu = gtk_menu_new();
      gtk_menu_set_accel_group(GTK_MENU(menu), AccelGroup);
      gtk_menu_shell_set_take_focus(GTK_MENU_SHELL(menu), TRUE);
      create_menu_sub(menu, item[i].child, popup);
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(widget), menu);
    }

    if (item[i].action) {
      if (popup) {
	item[i].action->popup = widget;
      } else {
	item[i].action->menu = widget;
      }
    }
#if USE_GTK_BUILDER
    if (item[i].action_name) {
      if (item[i].action) {
	item[i].action->action = g_action_map_lookup_action(G_ACTION_MAP(GtkApp), item[i].action_name);
      }
      if (item[i].accel_key) {
	char *key, accel[128];

	key = gdk_keyval_name(item[i].accel_key);
	if (key) {
          char action[128];
          const char *accels[2];
	  snprintf(accel, sizeof(accel), "%s%s%s",
		   (item[i].accel_mods & GDK_CONTROL_MASK) ? "<Control>" : "",
		   (item[i].accel_mods & GDK_SHIFT_MASK) ? "<Shift>" : "",
		   key);
	  snprintf(action, sizeof(action), "app.%s", item[i].action_name);
	  accels[0] = accel;
	  accels[1] = NULL;
	  gtk_application_set_accels_for_action(GtkApp, action, accels);
	}
      }
    }
#endif

    gtk_menu_shell_append(GTK_MENU_SHELL(parent), widget);
  }
}

static void
create_menu(GtkWidget *parent, struct MenuItem *item)
{
  create_menu_sub(parent, item, FALSE);
}

static void
create_popup(GtkWidget *parent, struct MenuItem *item)
{
  create_menu_sub(parent, item, TRUE);
}

static int
create_toplevel_window(void)
{
  int i;
  struct objlist *aobj;
  int x, y, width, height, w, h;
#if GTK_CHECK_VERSION(3, 22, 0)
  GdkDisplay *disp;
#else
  GdkScreen *screen;
#endif
  GtkWidget *popup;
#if USE_GTK_BUILDER
  GtkClipboard *clip;
#endif	/* USE_GTK_BUILDER */

  NgraphApp.recent_manager = gtk_recent_manager_get_default();

  init_action_widget_list();
  init_ngraph_app_struct();

#if GTK_CHECK_VERSION(3, 22, 0)
  w = 800;
  h = 600;
  disp = gdk_display_get_default();
  if (disp) {
    GdkMonitor *monitor;

    monitor = gdk_display_get_primary_monitor(disp);
    if (monitor) {
      GdkRectangle rect;

      gdk_monitor_get_geometry(monitor, &rect);
      w = rect.width;
      h = rect.height;
    }
  }
#else
  screen = gdk_screen_get_default();
  w = gdk_screen_get_width(screen);
  h = gdk_screen_get_height(screen);
#endif

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

  GtkApp = create_application_window(&popup);
  CurrentWindow = TopLevel = gtk_application_window_new(GtkApp);
  gtk_window_set_modal(GTK_WINDOW(TopLevel), TRUE); /* for the GtkColorButton (modal GtkColorChooserDialog) */
#if USE_GTK_BUILDER
  gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(TopLevel), TRUE);
  if (popup) {
    NgraphApp.Viewer.popup = popup;
  }
  clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  g_signal_connect(clip, "owner-change", G_CALLBACK(clipboard_changed), &NgraphApp.Viewer);
#else	/* USE_GTK_BUILDER */
  gtk_application_window_set_show_menubar(GTK_APPLICATION_WINDOW(TopLevel),FALSE);
#endif	/* USE_GTK_BUILDER */

  gtk_window_set_title(GTK_WINDOW(TopLevel), AppName);
  gtk_window_set_default_size(GTK_WINDOW(TopLevel), width, height);
  gtk_window_move(GTK_WINDOW(TopLevel), x, y);

  if (AccelGroup == NULL) {
    AccelGroup = gtk_accel_group_new();
  }
  gtk_window_add_accel_group(GTK_WINDOW(TopLevel), AccelGroup);

  g_signal_connect(TopLevel, "delete-event", G_CALLBACK(CloseCallback), NULL);
  g_signal_connect(TopLevel, "destroy-event", G_CALLBACK(CloseCallback), NULL);

  create_icon();
  initdialog();

  setup_toolbar(TopLevel);
  gtk_widget_show_all(GTK_WIDGET(TopLevel));
  reset_event();
  create_markpixmap(TopLevel);
  setupwindow(GtkApp);
  create_addin_menu();

  NgraphApp.FileName = NULL;
  NgraphApp.Viewer.Mode = PointB;

  gtk_widget_show_all(GTK_WIDGET(TopLevel));
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

  gtk_widget_show_all(GTK_WIDGET(TopLevel));
  set_widget_visibility();

  set_focus_sensitivity(&NgraphApp.Viewer);
  check_exist_instances(chkobject("draw"));

  set_newobj_cb(check_instance);
  set_delobj_cb(check_instance);

  return 0;
}

static void
souce_view_set_search_path(void)
{
  const gchar * const *dirs;
  gchar **new_dirs;
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
    theme = gtk_icon_theme_get_default();
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

    gtk_widget_destroy(TopLevel);
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
		 struct obj_list_data *merge, int *update_merge)
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
    }
  }
}

void
UpdateAll2(char **objs, int redraw)
{
  int update_axis, update_file, update_merge;

  check_update_obj(objs,
		   NgraphApp.FileWin.data.data, &update_file,
		   NgraphApp.AxisWin.data.data, &update_axis,
		   NgraphApp.MergeWin.data.data, &update_merge);
  if (update_file) {
    FileWinUpdate(NgraphApp.FileWin.data.data, TRUE, redraw && ! update_axis);
  }
  if (update_axis) {
    AxisWinUpdate(NgraphApp.AxisWin.data.data, TRUE, redraw);
  }
  if (update_merge) {
    MergeWinUpdate(NgraphApp.MergeWin.data.data, TRUE, redraw);
  }
  LegendWinUpdate(objs, TRUE, redraw);
  InfoWinUpdate(TRUE);
  CoordWinUpdate(TRUE);
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
  GdkWindow *win;

  if (NgraphApp.Viewer.Win == NULL || NgraphApp.cursor == NULL || CursorType == type)
    return;

  win = gtk_widget_get_window(NgraphApp.Viewer.Win);
  if (win == NULL) {
    return;
  }

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
  char *ustr, *arg[] = {NULL};

  if (s == NULL)
    return 0;

  len = strlen(s);
  ustr = g_locale_to_utf8(s, len, &rlen, &wlen, NULL);
  message_box(NULL, ustr, _("Error:"), RESPONS_ERROR);
  g_free(ustr);
  UpdateAll2(arg, FALSE);
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
#else
  if (DrawLock != DrawLockDraw) {
    return check_interrupt();
  }

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
  char *arg[] = {NULL};

  ret = message_box(get_current_window(), mes, _("Question"), RESPONS_YESNO);
  UpdateAll2(arg, FALSE);
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
CmViewerButtonArm(GtkToggleToolButton *action, gpointer client_data)
{
  int mode = PointB;
  struct Viewer *d;

  d = &NgraphApp.Viewer;

  if (! gtk_toggle_tool_button_get_active(action)) {
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
    obj = getobject("draw");
    if (obj == NULL) {
      return 1;
    }
    iterate_undo_func(obj, func);
    obj = getobject("fit");
    if (obj == NULL) {
      return 1;
    }
    r = func(obj);;
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

#if USE_GTK_BUILDER
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

  if (UndoInfo) {
    snprintf(buf, sizeof(buf), _("_Undo: %s"), _(UndoTypeStr[UndoInfo->type]));
    label = buf;
  } else {
    label = _("_Undo");
  }
  item = g_menu_item_new(label, "app.EditUndoAction");
  g_menu_insert_item(G_MENU(menu), 0, item);
}
#else
static void
set_undo_menu_label(void)
{
  char buf[512], *label;
  if (ActionWidget[EditUndoAction].menu) {
    if (UndoInfo) {
      snprintf(buf, sizeof(buf), _("_Undo: %s"), _(UndoTypeStr[UndoInfo->type]));
      label = buf;
    } else {
      label = _("_Undo");
    }
    gtk_menu_item_set_label(GTK_MENU_ITEM(ActionWidget[EditUndoAction].menu), label);
  }

  if (ActionWidget[EditRedoAction].menu) {
    if (RedoInfo) {
      snprintf(buf, sizeof(buf), _("_Redo: %s"), _(UndoTypeStr[RedoInfo->type]));
      label = buf;
    } else {
      label = _("_Redo");
    }
    gtk_menu_item_set_label(GTK_MENU_ITEM(ActionWidget[EditRedoAction].menu), label);
  }
}
#endif

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

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
#include "gtk_ruler.h"

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
#define FIT_HISTORY      "fit_history"
#define KEYMAP_FILE      "accel_map"
#define UI_FILE          "NgraphUI.xml"

#define USE_EXT_DRIVER 0

#if GTK_CHECK_VERSION(2, 24, 0)
#define SIDE_PANE_TAB_ID "side_pane"
#else
static char *SIDE_PANE_TAB_ID = "side_pane";
#endif
int Menulock = FALSE, DnDLock = FALSE;
struct NgraphApp NgraphApp = {0};
GtkWidget *TopLevel = NULL;
GtkAccelGroup *AccelGroup = NULL;

static GtkActionGroup *ActionGroup = NULL;
static GtkWidget *CurrentWindow = NULL, *CToolbar = NULL, *PToolbar = NULL;
static enum {APP_CONTINUE, APP_QUIT, APP_QUIT_FORCE} Hide_window = APP_CONTINUE;
static int DrawLock = FALSE;
static unsigned int CursorType;
static GtkUIManager *NgraphUi;

#if USE_EXT_DRIVER
static GtkWidget *ExtDrvOutMenu = NULL
#endif

static void CmReloadWindowConfig(GtkAction *w, gpointer user_data);
static void CmToggleSingleWindowMode(GtkToggleAction *action, gpointer client_data);
static void script_exec(GtkWidget *w, gpointer client_data);

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

static void clear_information(GtkAction *w, gpointer user_data);
static void toggle_view_cb(GtkToggleAction *action, gpointer data);

struct actions {
  const char *name;
  GtkAction *action;
  enum {
    FOCUS_TYPE_1 = 0,
    FOCUS_TYPE_2,
    FOCUS_TYPE_3,
    FOCUS_TYPE_4,
    FOCUS_TYPE_5,
    FOCUS_TYPE_6,
    FOCUS_TYPE_NUM,
  } type;
};

static void set_action_sensitivity(struct actions *actions, int *focus_type, int n);

enum {
  RECENT_TYPE_GRAPH,
  RECENT_TYPE_DATA,
};

struct NgraphActionEntry {
  enum {
    ACTION_TYPE_NORMAL,
    ACTION_TYPE_TOGGLE,
    ACTION_TYPE_RADIO,
    ACTION_TYPE_RECENT,
  } type;
  const gchar *name;
  const gchar *stock_id;
  const gchar *label;
  const gchar *tooltip;
  gchar *caption;
  GCallback callback;
  int user_data;
  const char *icon;
  const char *accel_path;
  guint accel_key;
  GdkModifierType accel_mods;
};

static struct NgraphActionEntry ActionEntry[] = {
  {
    ACTION_TYPE_NORMAL,
    "GraphMenuAction",
    NULL,
    N_("_Graph"),
    NULL,
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphNewMenuAction",
    NULL,
    N_("_New graph"),
    NULL,
    NULL,
    NULL,
    0,
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphExportMenuAction",
    NULL,
    N_("_Export image"),
    NULL,
    NULL,
    NULL,
    0,
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphAddinMenuAction",
    NULL,
    N_("_Add-in"),
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphAddinSubMenuAction",
    NULL,
    N_("_Add-in"),
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "EditMenuAction",
    NULL,
    N_("_Edit"),
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "ViewMenuAction",
    NULL,
    N_("_View"),
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "DataMenuAction",
    NULL,
    N_("_Data"),
    NULL,
    NULL,
    NULL
  },
  {
    ACTION_TYPE_NORMAL,
    "AxisMenuAction",
    NULL,
    N_("_Axis"),
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendMenuAction",
    NULL,
    N_("_Legend"),
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "MergeMenuAction",
    NULL,
    N_("_Merge"),
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "PreferenceMenuAction",
    NULL,
    N_("_Preference"),
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "HelpMenuAction",
    NULL,
    N_("_Help"),
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphNewFrameAction",
    NULL,
    N_("_Frame graph"),
    N_("Create frame graph"),
    NULL,
    G_CALLBACK(CmGraphNewMenu),
    MenuIdGraphNewFrame,
    NGRAPH_FRAME_ICON_FILE,
    "<Ngraph>/Graph/New graph/Frame graph",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphNewSectionAction",
    NULL,
    N_("_Section graph"),
    N_("Create section graph"),
    NULL,
    G_CALLBACK(CmGraphNewMenu),
    MenuIdGraphNewSection,
    NGRAPH_SECTION_ICON_FILE,
    "<Ngraph>/Graph/New graph/Section graph",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphNewCrossAction",
    NULL,
    N_("_Cross graph"),
    N_("Create cross graph"),
    NULL,
    G_CALLBACK(CmGraphNewMenu),
    MenuIdGraphNewCross,
    NGRAPH_CROSS_ICON_FILE,
    "<Ngraph>/Graph/New graph/Cross graph",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphNewClearAction",
    NULL,
    N_("_All clear"),
    N_("Clear graph"),
    NULL,
    G_CALLBACK(CmGraphNewMenu),
    MenuIdGraphAllClear,
    NULL,
    "<Ngraph>/Graph/New graph/All clear",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphLoadAction",
    GTK_STOCK_OPEN,
    N_("_Load graph"),
    N_("Load NGP"),
    N_("Load NGP file"),
    G_CALLBACK(CmGraphLoad),
    0,
    NULL,
    "<Ngraph>/Graph/Load graph",
    GDK_KEY_r,
    GDK_CONTROL_MASK
  },
  {
    ACTION_TYPE_RECENT,
    "GraphRecentAction",
    NULL,
    N_("_Recent graphs"),
    NULL,
    NULL,
    NULL,
    RECENT_TYPE_GRAPH,
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphSaveAction",
    GTK_STOCK_SAVE,
    NULL,
    N_("Save NGP"),
    N_("Save NGP file"),
    G_CALLBACK(CmGraphOverWrite),
    0,
    NULL,
    "<Ngraph>/Graph/Save",
    GDK_KEY_s,
    GDK_CONTROL_MASK
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphSaveAsAction",
    GTK_STOCK_SAVE_AS,
    NULL,
    N_("Save NGP"),
    NULL,
    G_CALLBACK(CmGraphSave),
    0,
    NULL,
    "<Ngraph>/Graph/SaveAs",
    GDK_KEY_s,
    GDK_CONTROL_MASK | GDK_SHIFT_MASK
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphExportGRAAction",
    NULL,
    N_("_GRA file"),
    N_("Export as GRA file"),
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputGRAFile,
    NULL,
    "<Ngraph>/Graph/Export image/GRA File",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphExportPSAction",
    NULL,
    N_("_PS file"),
    N_("Export as PostScript file"),
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputPSFile,
    NULL,
    "<Ngraph>/Graph/Export image/PS File",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphExportEPSAction",
    NULL,
    N_("_EPS file"),
    N_("Export as Encapsulate PostScript file"),
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputEPSFile,
    NULL,
    "<Ngraph>/Graph/Export image/EPS File",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphExportPDFAction",
    NULL,
    N_("P_DF file"),
    N_("Export as Portable Document Format"),
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputPDFFile,
    NULL,
    "<Ngraph>/Graph/Export image/PDF File",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphExportSVGAction",
    NULL,
    N_("_SVG file"),
    N_("Export as Scalable Vector Graphics file"),
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputSVGFile,
    NULL,
    "<Ngraph>/Graph/Export image/SVG File",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphExportPNGAction",
    NULL,
    N_("P_NG file"),
    N_("Export as Portable Network Graphics  file"),
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputPNGFile,
    NULL,
    "<Ngraph>/Graph/Export image/PNG File",
  },
#ifdef CAIRO_HAS_WIN32_SURFACE
  {
    ACTION_TYPE_NORMAL,
    "GraphExportCairoEMFAction",
    NULL,
    N_("_EMF file"),
    N_("Export as Windows Enhanced Metafile"),
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputCairoEMFFile,
    NULL,
    "<Ngraph>/Graph/Export image/EMF File (cairo)",
  },
#endif	/* CAIRO_HAS_WIN32_SURFACE */
#ifdef WINDOWS
  {
    ACTION_TYPE_NORMAL,
    "GraphExportEMFFileAction",
    NULL,
    N_("_EMF file"),
    N_("Export as Windows Enhanced Metafile"),
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputEMFFile,
    NULL,
    "<Ngraph>/Graph/Export image/EMF File",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphExportEMFClipboardAction",
    NULL,
    N_("_Clipboard (EMF)"),
    N_("Copy to the clipboard as Windows Enhanced Metafile "),
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputEMFClipboard,
    NULL,
    "<Ngraph>/Graph/Export image/Clipboard",
  },
#endif	/* WINDOWS */
  {
    ACTION_TYPE_NORMAL,
    "GraphDrawOrderAction",
    NULL,
    N_("_Draw order"),
    NULL,
    NULL,
    G_CALLBACK(CmGraphSwitch),
    0,
    NULL,
    "<Ngraph>/Graph/Draw order",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphPageSetupAction",
    GTK_STOCK_PAGE_SETUP,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmGraphPage),
    0,
    NULL,
    "<Ngraph>/Graph/Page",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphPrintPreviewAction",
    GTK_STOCK_PRINT_PREVIEW,
    NULL,
    N_("Print preview"),
    N_("Print preview"),
    G_CALLBACK(CmOutputViewerB),
    0,
    NULL,
    "<Ngraph>/Graph/Print preview",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphPrintAction",
    GTK_STOCK_PRINT,
    NULL,
    N_("Print"),
    N_("Print"),
    G_CALLBACK(CmOutputPrinterB),
    0,
    NULL,
    "<Ngraph>/Graph/Print",
    GDK_KEY_p,
    GDK_CONTROL_MASK,
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphCurrentDirectoryAction",
    NULL,
    N_("_Current directory"),
    NULL,
    NULL,
    G_CALLBACK(CmGraphDirectory),
    0,
    NULL,
    "<Ngraph>/Graph/Current directory",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphShellAction",
    NULL,
    N_("_Ngraph shell"),
    NULL,
    NULL,
    G_CALLBACK(CmGraphShell),
    0,
    NULL,
    "<Ngraph>/Graph/Ngraph shell",
  },
  {
    ACTION_TYPE_NORMAL,
    "GraphQuitAction",
    GTK_STOCK_QUIT,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmGraphQuit),
    0,
    NULL,
    "<Ngraph>/Graph/Quit",
    GDK_KEY_q,
    GDK_CONTROL_MASK,
  },
  {
    ACTION_TYPE_NORMAL,
    "EditCutAction",
    GTK_STOCK_CUT,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditCut,
    NULL,
    "<Ngraph>/Edit/Cut",
    GDK_KEY_x,
    GDK_CONTROL_MASK,
  },
  {
    ACTION_TYPE_NORMAL,
    "EditCopyAction",
    GTK_STOCK_COPY,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditCopy,
    NULL,
    "<Ngraph>/Edit/Copy",
    GDK_KEY_c,
    GDK_CONTROL_MASK,
 },
  {
    ACTION_TYPE_NORMAL,
    "EditPasteAction",
    GTK_STOCK_PASTE,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditPaste,
    NULL,
    "<Ngraph>/Edit/Paste",
    GDK_KEY_v,
    GDK_CONTROL_MASK,
  },
  {
    ACTION_TYPE_NORMAL,
    "EditDeleteAction",
    GTK_STOCK_DELETE,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditDelete,
    NULL,
    "<Ngraph>/Edit/Delete",
    GDK_KEY_Delete,
  },
  {
    ACTION_TYPE_NORMAL,
    "EditDuplicateAction",
    NULL,
    N_("_Duplicate"),
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditDuplicate,
    NULL,
    "<Ngraph>/Edit/Duplicate",
    GDK_KEY_Insert,
  },
  {
    ACTION_TYPE_NORMAL,
    "EditOrderAction",
    NULL,
    N_("draw _Order"),
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "EditOrderTopAction",
    GTK_STOCK_GOTO_TOP,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditOrderTop,
    NULL,
    "<Ngraph>/Edit/draw Order/Top",
    GDK_KEY_Home,
    GDK_SHIFT_MASK,
  },
  {
    ACTION_TYPE_NORMAL,
    "EditOrderUpAction",
    GTK_STOCK_GO_UP,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditOrderUp,
    NULL,
    "<Ngraph>/Edit/draw Order/Up",
  },
  {
    ACTION_TYPE_NORMAL,
    "EditOrderDownAction",
    GTK_STOCK_GO_DOWN,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditOrderDown,
    NULL,
    "<Ngraph>/Edit/draw Order/Down",
  },
  {
    ACTION_TYPE_NORMAL,
    "EditOrderBottomAction",
    GTK_STOCK_GOTO_BOTTOM,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditOrderBottom,
    NULL,
    "<Ngraph>/Edit/draw Order/Bottom",
    GDK_KEY_End,
    GDK_SHIFT_MASK,
  },
  {
    ACTION_TYPE_NORMAL,
    "EditAlignAction",
    NULL,
    N_("_Align"),
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "EditAlignLeftAction",
    NULL,
    N_("_Left"),
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignLeft,
    NGRAPH_ALIGN_L_ICON_FILE,
    "<Ngraph>/Edit/Align/Left",
  },
  {
    ACTION_TYPE_NORMAL,
    "EditAlignRightAction",
    NULL,
    N_("_Right"),
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignRight,
    NGRAPH_ALIGN_R_ICON_FILE,
    "<Ngraph>/Edit/Align/Right",
  },
  {
    ACTION_TYPE_NORMAL,
    "EditAlignVCenterAction",
    NULL,
    N_("_Vertical center"),
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignVCenter,
    NGRAPH_ALIGN_VC_ICON_FILE,
    "<Ngraph>/Edit/Align/Vertical center",
  },
  {
    ACTION_TYPE_NORMAL,
    "EditAlignTopAction",
    NULL,
    N_("_Top"),
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignTop,
    NGRAPH_ALIGN_T_ICON_FILE,
    "<Ngraph>/Edit/Align/Top",
  },
  {
    ACTION_TYPE_NORMAL,
    "EditAlignBottomAction",
    NULL,
    N_("_Bottom"),
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignBottom,
    NGRAPH_ALIGN_B_ICON_FILE,
    "<Ngraph>/Edit/Align/Bottom",
  },
  {
    ACTION_TYPE_NORMAL,
    "EditAlignHCenterAction",
    NULL,
    N_("_Horizontal center"),
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdAlignHCenter,
    NGRAPH_ALIGN_HC_ICON_FILE,
    "<Ngraph>/Edit/Align/Horizontal center",
  },
  {
    ACTION_TYPE_NORMAL,
    "EditRotateAction",
    NULL,
    N_("_Rotate"),
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "EditRotateCWAction",
    NULL,
    N_("rotate _90 degree clockwise"),
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditRotateCW,
    NULL,
    "<Ngraph>/Edit/RotateCW",
  },
  {
    ACTION_TYPE_NORMAL,
    "EditRotateCCWAction",
    NULL,
    N_("rotate 9_0 degree counter-clockwise"),
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditRotateCCW,
    NULL,
    "<Ngraph>/Edit/RotateCCW",
  },
  {
    ACTION_TYPE_NORMAL,
    "EditFlipAction",
    NULL,
    N_("_Flip"),
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "EditFlipHAction",
    NULL,
    N_("flip _Horizontally"),
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditFlipHorizontally,
    NULL,
    "<Ngraph>/Edit/FlipHorizontally",
  },
  {
    ACTION_TYPE_NORMAL,
    "EditFlipVActiopn",
    NULL,
    N_("flip _Vertically"),
    NULL,
    NULL,
    G_CALLBACK(CmEditMenuCB),
    MenuIdEditFlipVertically,
    NULL,
    "<Ngraph>/Edit/FlipVertically",
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewToggleSingleWindowModeAction",
    NULL,
    N_("Single window mode"),
    N_("Single window mode"),
    N_("Toggle single window mode"),
    G_CALLBACK(CmToggleSingleWindowMode),
    0,
    NULL,
    "<Ngraph>/View/Single window mode",
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewToggleDataWindowAction",
    NULL,
    "Data Window",
    "Data Window",
    N_("Activate Data Window"),
    G_CALLBACK(CmFileWindow),
    0,
    NGRAPH_FILEWIN_ICON_FILE,
    "<Ngraph>/View/Data Window",
    GDK_KEY_F3,
    0,
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewToggleAxisWindowAction",
    NULL,
    "Axis Window",
    "Axis Window",
    N_("Activate Axis Window"),
    G_CALLBACK(CmAxisWindow),
    0,
    NGRAPH_AXISWIN_ICON_FILE,
    "<Ngraph>/View/Axis Window",
    GDK_KEY_F4,
    0,
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewToggleLegendWindowAction",
    NULL,
    "Legend Window",
    "Legend Window",
    N_("Activate Legend Window"),
    G_CALLBACK(CmLegendWindow),
    0,
    NGRAPH_LEGENDWIN_ICON_FILE,
    "<Ngraph>/View/Legend Window",
    GDK_KEY_F5,
    0,
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewToggleMergeWindowAction",
    NULL,
    "Merge Window",
    "Merge Window",
    N_("Activate Merge Window"),
    G_CALLBACK(CmMergeWindow),
    0,
    NGRAPH_MERGEWIN_ICON_FILE,
    "<Ngraph>/View/Merge Window",
    GDK_KEY_F6,
    0,
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewToggleCoordinateWindowAction",
    NULL,
    "Coordinate Window",
    "Coordinate Window",
    N_("Activate Coordinate Window"),
    G_CALLBACK(CmCoordinateWindow),
    0,
    NGRAPH_COORDWIN_ICON_FILE,
    "<Ngraph>/View/Coordinate Window",
    GDK_KEY_F7,
    0,
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewToggleInformationWindowAction",
    NULL,
    "Information Window",
    "Information Window",
    N_("Activate Information Window"),
    G_CALLBACK(CmInformationWindow),
    0,
    NGRAPH_INFOWIN_ICON_FILE,
    "<Ngraph>/View/Information Window",
    GDK_KEY_F8,
    0,
  },
  {
    ACTION_TYPE_NORMAL,
    "ViewDrawAction",
    NULL,
    N_("_Draw"),
    N_("Draw"),
    N_("Draw on Viewer Window"),
    G_CALLBACK(CmViewerDraw),
    TRUE,
    NGRAPH_DRAW_ICON_FILE,
    "<Ngraph>/View/Draw",
    GDK_KEY_d,
    GDK_CONTROL_MASK,
  },
  {
    ACTION_TYPE_NORMAL,
    "ViewDrawDirectAction",
    NULL,
    N_("_Draw"),
    N_("Draw"),
    N_("Draw on Viewer Window"),
    G_CALLBACK(CmViewerDraw),
    FALSE,
    NGRAPH_DRAW_ICON_FILE,
    "<Ngraph>/View/DrawDirect",
  },
  {
    ACTION_TYPE_NORMAL,
    "ViewClearAction",
    GTK_STOCK_CLEAR,
    NULL,
    N_("Clear Image"),
    N_("Clear Viewer Window"),
    G_CALLBACK(CmViewerClear),
    0,
    NULL,
    "<Ngraph>/View/Clear",
    GDK_KEY_e,
    GDK_CONTROL_MASK,
  },
  {
    ACTION_TYPE_NORMAL,
    "ViewDefaultWindowConfigAction",
    NULL,
    N_("default _Window config"),
    NULL,
    NULL,
    G_CALLBACK(CmReloadWindowConfig),
    0,
    NULL,
    "<Ngraph>/View/default Window config",
  },
  {
    ACTION_TYPE_NORMAL,
    "ViewClearInformationWindowAction",
    NULL,
    N_("_Clear information view"),
    NULL,
    NULL,
    G_CALLBACK(clear_information),
    0,
    NULL,
    "<Ngraph>/View/Clear information window",
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewSidebarAction",
    NULL,
    N_("_Sidebar"),
    NULL,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleSidebar,
    NULL,
    "<Ngraph>/View/Sidebar",
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewStatusbarAction",
    NULL,
    N_("_Statusbar"),
    NULL,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleStatusbar,
    NULL,
    "<Ngraph>/View/Statusbar",
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewRulerAction",
    NULL,
    N_("_Ruler"),
    NULL,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleRuler,
    NULL,
    "<Ngraph>/View/Ruler",
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewScrollbarAction",
    NULL,
    N_("_Scrollbar"),
    NULL,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleScrollbar,
    NULL,
    "<Ngraph>/View/Scrollbar",
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewCommandToolbarAction",
    NULL,
    N_("_Command toolbar"),
    NULL,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleCToolbar,
    NULL,
    "<Ngraph>/View/Command toolbar",
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewToolboxAction",
    NULL,
    N_("_Toolbox"),
    NULL,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdTogglePToolbar,
    NULL,
    "<Ngraph>/View/Toolbox",
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewCrossGaugeAction",
    NULL,
    N_("cross _Gauge"),
    NULL,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleCrossGauge,
    NULL,
    "<Ngraph>/View/cross Gauge",
    GDK_KEY_g,
    GDK_CONTROL_MASK,
  },
  {
    ACTION_TYPE_NORMAL,
    "DataNewAction",
    NULL,
    N_("_New"),
    NULL,
    NULL,
    G_CALLBACK(CmFileNew),
    0,
    NULL,
    "<Ngraph>/Data/New",
  },
  {
    ACTION_TYPE_NORMAL,
    "DataAddAction",
    GTK_STOCK_FILE,
    N_("_Add"),
    N_("Add Data"),
    N_("Add Data file"),
    G_CALLBACK(CmFileOpen),
    0,
    NULL,
    "<Ngraph>/Data/Add",
    GDK_KEY_o,
    GDK_CONTROL_MASK,
  },
  {
    ACTION_TYPE_RECENT,
    "DataRecentAction",
    NULL,
    N_("_Recent data"),
    NULL,
    NULL,
    NULL,
    RECENT_TYPE_DATA,
  },
  {
    ACTION_TYPE_NORMAL,
    "DataPropertyAction",
    GTK_STOCK_PROPERTIES,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmFileUpdate),
    0,
    NULL,
    "<Ngraph>/Data/Property",
  },
  {
    ACTION_TYPE_NORMAL,
    "DataCloseAction",
    GTK_STOCK_CLOSE,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmFileClose),
    0,
    NULL,
    "<Ngraph>/Data/Close",
  },
  {
    ACTION_TYPE_NORMAL,
    "DataEditAction",
    GTK_STOCK_EDIT,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmFileEdit),
    0,
    NULL,
    "<Ngraph>/Data/Edit",
  },
  {
    ACTION_TYPE_NORMAL,
    "DataSaveAction",
    NULL,
    N_("_Save data"),
    NULL,
    NULL,
    G_CALLBACK(CmFileSaveData),
    0,
    NULL,
    "<Ngraph>/Data/Save data",
  },
  {
    ACTION_TYPE_NORMAL,
    "DataMathAction",
    NULL,
    N_("_Math Transformation"),
    N_("Math Transformation"),
    N_("Set Math Transformation"),
    G_CALLBACK(CmFileMath),
    0,
    NGRAPH_MATH_ICON_FILE,
    "<Ngraph>/Data/Math",
  },
  {
    ACTION_TYPE_NORMAL,
    "AxisAddAction",
    GTK_STOCK_ADD,
    NULL,
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "AxisAddFrameAction",
    NULL,
    N_("_Frame graph"),
    NULL,
    NULL,
    G_CALLBACK(CmAxisNewFrame),
    0,
    NGRAPH_FRAME_ICON_FILE,
    "<Ngraph>/Axis/Add/Frame graph",
  },
  {
    ACTION_TYPE_NORMAL,
    "AxisAddSectionAction",
    NULL,
    N_("_Section graph"),
    NULL,
    NULL,
    G_CALLBACK(CmAxisNewSection),
    0,
    NGRAPH_SECTION_ICON_FILE,
    "<Ngraph>/Axis/Add/Section graph",
  },
  {
    ACTION_TYPE_NORMAL,
    "AxisAddCrossAction",
    NULL,
    N_("_Cross graph"),
    NULL,
    NULL,
    G_CALLBACK(CmAxisNewCross),
    0,
    NGRAPH_CROSS_ICON_FILE,
    "<Ngraph>/Axis/Add/Cross graph",
  },
  {
    ACTION_TYPE_NORMAL,
    "AxisAddSingleAction",
    NULL,
    N_("Single _Axis"),
    NULL,
    NULL,
    G_CALLBACK(CmAxisNewSingle),
    0,
    NGRAPH_SINGLE_ICON_FILE,
    "<Ngraph>/Axis/Add/Single axis",
  },
  {
    ACTION_TYPE_NORMAL,
    "AxisPropertyAction",
    GTK_STOCK_PROPERTIES,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmAxisUpdate),
    0,
    NULL,
    "<Ngraph>/Axis/Property",
  },
  {
    ACTION_TYPE_NORMAL,
    "AxisDeleteAction",
    GTK_STOCK_DELETE,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmAxisDel),
    0,
    NULL,
    "<Ngraph>/Axis/Delete",
  },
  {
    ACTION_TYPE_NORMAL,
    "AxisScaleZoomAction",
    NULL,
    N_("Scale _Zoom"),
    NULL,
    NULL,
    G_CALLBACK(CmAxisZoom),
    0,
    NULL,
    "<Ngraph>/Axis/Scale Zoom",
  },

  {
    ACTION_TYPE_NORMAL,
    "AxisScaleClearAction",
    NULL,
    N_("Scale _Clear"),
    N_("Clear Scale"),
    N_("Clear Scale"),
    G_CALLBACK(CmAxisClear),
    0,
    NGRAPH_SCALE_ICON_FILE,
    "<Ngraph>/Axis/Scale Clear",
    GDK_KEY_c,
    GDK_SHIFT_MASK | GDK_CONTROL_MASK,
  },
  {
    ACTION_TYPE_NORMAL,
    "AxisScaleUndoAction",
    GTK_STOCK_UNDO,
    N_("Scale _Undo"),
    N_("Scale Undo"),
    N_("Undo Scale Settings"),
    G_CALLBACK(CmAxisScaleUndo),
    0,
    NULL,
    "<Ngraph>/Axis/Scale Undo",
  },
  {
    ACTION_TYPE_NORMAL,
    "AxisGridAction",
    NULL,
    N_("_Grid"),
    NULL,
  },
  {
    ACTION_TYPE_NORMAL,
    "AxisGridNewAction",
    GTK_STOCK_ADD,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmAxisGridNew),
    0,
    NULL,
    "<Ngraph>/Axis/Grid/Add",
  },
  {
    ACTION_TYPE_NORMAL,
    "AxisGridPropertyAction",
    GTK_STOCK_PROPERTIES,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmAxisGridUpdate),
    0,
    NULL,
    "<Ngraph>/Axis/Grid/Property",
 },
  {
    ACTION_TYPE_NORMAL,
    "AxisGridDeleteAction",
    GTK_STOCK_DELETE,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmAxisGridDel),
    0,
    NULL,
    "<Ngraph>/Axis/Grid/Delete",
  },
  {
    ACTION_TYPE_NORMAL,
    "MergeAddAction",
    GTK_STOCK_ADD,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmMergeOpen),
    0,
    NULL,
    "<Ngraph>/Merge/Add",
  },
  {
    ACTION_TYPE_NORMAL,
    "MergePropertyAction",
    GTK_STOCK_PROPERTIES,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmMergeUpdate),
    0,
    NULL,
   "<Ngraph>/Merge/Property",
  },
  {
    ACTION_TYPE_NORMAL,
    "MergeCloseAction",
    GTK_STOCK_CLOSE,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmMergeClose),
    0,
    NULL,
    "<Ngraph>/Merge/Close",
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendPathMenuAction",
    NULL,
    N_("_Path"),
    NULL,
    NULL,
    NULL,
    0,
    NGRAPH_LINE_ICON_FILE,
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendRectangleMenuAction",
    NULL,
    N_("_Rectangle"),
    NULL,
    NULL,
    NULL,
    0,
    NGRAPH_RECT_ICON_FILE,
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendArcMenuAction",
    NULL,
    N_("_Arc"),
    NULL,
    NULL,
    NULL,
    0,
    NGRAPH_ARC_ICON_FILE,
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendMarkMenuAction",
    NULL,
    N_("_Mark"),
    NULL,
    NULL,
    NULL,
    0,
    NGRAPH_MARK_ICON_FILE,
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendTextMenuAction",
    NULL,
    N_("_Text"),
    NULL,
    NULL,
    NULL,
    0,
    NGRAPH_TEXT_ICON_FILE,
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendPathPropertyAction",
    GTK_STOCK_PROPERTIES,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmLineUpdate),
    0,
    NULL,
    "<Ngraph>/Legend/Path/Property",
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendPathDeleteAction",
    GTK_STOCK_DELETE,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmLineDel),
    0,
    NULL,
    "<Ngraph>/Legend/Path/Delete",
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendRectanglePropertyAction",
    GTK_STOCK_PROPERTIES,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmRectUpdate),
    0,
    NULL,
    "<Ngraph>/Legend/Rectangle/Property",
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendRectangleDeleteAction",
    GTK_STOCK_DELETE,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmRectDel),
    0,
    NULL,
    "<Ngraph>/Legend/Rectangle/Delete",
  },

  {
    ACTION_TYPE_NORMAL,
    "LegendArcPropertyAction",
    GTK_STOCK_PROPERTIES,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmArcUpdate),
    0,
    NULL,
    "<Ngraph>/Legend/Arc/Property",
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendArcDeleteAction",
    GTK_STOCK_DELETE,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmArcDel),
    0,
    NULL,
    "<Ngraph>/Legend/Arc/Delete",
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendMarkPropertyAction",
    GTK_STOCK_PROPERTIES,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmMarkUpdate),
    0,
    NULL,
    "<Ngraph>/Legend/Mark/Property",
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendMarkDeleteAction",
    GTK_STOCK_DELETE,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmMarkDel),
    0,
    NULL,
    "<Ngraph>/Legend/Mark/Delete",
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendTextPropertyAction",
    GTK_STOCK_PROPERTIES,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmTextUpdate),
    0,
    NULL,
    "<Ngraph>/Legend/Text/Property",
  },
  {
    ACTION_TYPE_NORMAL,
    "LegendTextDeleteAction",
    GTK_STOCK_DELETE,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmTextDel),
    0,
    NULL,
    "<Ngraph>/Legend/Text/Delete",
  },
  {
    ACTION_TYPE_NORMAL,
    "PreferenceViewerAction",
    NULL,
    N_("_Viewer"),
    NULL,
    NULL,
    G_CALLBACK(CmOptionViewer),
    0,
    NULL,
    "<Ngraph>/Preference/Viewer",
  },
  {
    ACTION_TYPE_NORMAL,
    "PreferenceExternalViewerAction",
    NULL,
    N_("_External viewer"),
    NULL,
    NULL,
    G_CALLBACK(CmOptionExtViewer),
    0,
    NULL,
    "<Ngraph>/Preference/External Viewer",
  },
  {
    ACTION_TYPE_NORMAL,
    "PreferenceFontAction",
    NULL,
    N_("_Font aliases"),
    NULL,
    NULL,
    G_CALLBACK(CmOptionPrefFont),
    0,
    NULL,
    "<Ngraph>/Preference/Font aliases",
  },
  {
    ACTION_TYPE_NORMAL,
    "PreferenceAddinAction",
    NULL,
    N_("_Add-in script"),
    NULL,
    NULL,
    G_CALLBACK(CmOptionScript),
    0,
    NULL,
    "<Ngraph>/Preference/Addin Script",
  },
  {
    ACTION_TYPE_NORMAL,
    "PreferenceMiscAction",
    NULL,
    N_("_Miscellaneous"),
    NULL,
    NULL,
    G_CALLBACK(CmOptionMisc),
    0,
    NULL,
    "<Ngraph>/Preference/Miscellaneous",
  },
  {
    ACTION_TYPE_NORMAL,
    "PreferenceSaveSettingAction",
    NULL,
    N_("save as default (_Settings)"),
    NULL,
    NULL,
    G_CALLBACK(CmOptionSaveDefault),
    0,
    NULL,
    "<Ngraph>/Preference/save as default (Settings)",
  },
  {
    ACTION_TYPE_NORMAL,
    "PreferenceSaveGraphAction",
    NULL,
    N_("save as default (_Graph)"),
    NULL,
    NULL,
    G_CALLBACK(CmOptionSaveNgp),
    0,
    NULL,
    "<Ngraph>/Preference/save as default (Graph)",
  },
  {
    ACTION_TYPE_NORMAL,
    "PreferenceDataDefaultAction",
    NULL,
    N_("_Data file default"),
    NULL,
    NULL,
    G_CALLBACK(CmOptionFileDef),
    0,
    NULL,
    "<Ngraph>/Preference/Data file default",
  },
  {
    ACTION_TYPE_NORMAL,
    "PreferenceTextDefaultAction",
    NULL,
    N_("_Legend text default"),
    NULL,
    NULL,
    G_CALLBACK(CmOptionTextDef),
    0,
    NULL,
    "<Ngraph>/Preference/Legend text default",
  },
  {
    ACTION_TYPE_NORMAL,
    "HelpHelpAction",
    GTK_STOCK_HELP,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmHelpHelp),
    0,
    NULL,
    "<Ngraph>/Help/Help",
    GDK_KEY_F1,
    0,
  },
  {
    ACTION_TYPE_NORMAL,
    "HelpAboutAction",
    GTK_STOCK_ABOUT,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmHelpAbout),
    0,
    NULL,
    "<Ngraph>/Help/About",
  },
  {
    ACTION_TYPE_NORMAL,
    "PopupUpdateAction",
    GTK_STOCK_PROPERTIES,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(ViewerUpdateCB),
    0,
    NULL,
    "<Ngraph>/Popup/Update",
    GDK_KEY_Return,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxPointAction",
    NULL,
    N_("Point"),
    N_("Pointer"),
    N_("Legend and Axis Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"),
    G_CALLBACK(CmViewerButtonArm),
    PointB,
    NGRAPH_POINT_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxLegendAction",
    NULL,
    N_("Legend"),
    N_("Legend Pointer"),
    N_("Legend Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"),
    G_CALLBACK(CmViewerButtonArm),
    LegendB,
    NGRAPH_LEGENDPOINT_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxAxisAction",
    NULL,
    N_("Axis"),
    N_("Axis Pointer"),
    N_("Axis Pointer (+SHIFT: Multi select / +CONTROL: Horizontal/Vertical +SHIFT: Fine)"),
    G_CALLBACK(CmViewerButtonArm),
    AxisB,
    NGRAPH_AXISPOINT_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxDataAction",
    NULL,
    N_("Data"),
    N_("Data Pointer"),
    N_("Data Pointer"),
    G_CALLBACK(CmViewerButtonArm),
    DataB,
    NGRAPH_DATAPOINT_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxPathAction",
    NULL,
    N_("Path"),
    N_("Path"),
    N_("New Legend Path (+SHIFT: Fine +CONTROL: snap angle)"),
    G_CALLBACK(CmViewerButtonArm),
    PathB,
    NGRAPH_LINE_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxRectangleAction",
    NULL,
    N_("Rectangle"),
    N_("Rectangle"),
    N_("New Legend Rectangle (+SHIFT: Fine +CONTROL: square integer ratio rectangle)"),
    G_CALLBACK(CmViewerButtonArm),
    RectB,
    NGRAPH_RECT_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxArcAction",
    NULL,
    N_("Arc"),
    N_("Arc"),
    N_("New Legend Arc (+SHIFT: Fine +CONTROL: circle or integer ratio ellipse)"),
    G_CALLBACK(CmViewerButtonArm),
    ArcB,
    NGRAPH_ARC_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxMarkAction",
    NULL,
    N_("Mark"),
    N_("Mark"),
    N_("New Legend Mark (+SHIFT: Fine)"),
    G_CALLBACK(CmViewerButtonArm),
    MarkB,
    NGRAPH_MARK_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxTextAction",
    NULL,
    N_("Text"),
    N_("Text"),
    N_("New Legend Text (+SHIFT: Fine)"),
    G_CALLBACK(CmViewerButtonArm),
    TextB,
    NGRAPH_TEXT_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxGaussAction",
    NULL,
    N_("Gauss"),
    N_("Gaussian"),
    N_("New Legend Gaussian (+SHIFT: Fine +CONTROL: integer ratio)"),
    G_CALLBACK(CmViewerButtonArm),
    GaussB,
    NGRAPH_GAUSS_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxFrameAction",
    NULL,
    N_("Frame axis"),
    N_("Frame Graph"),
    N_("New Frame Graph (+SHIFT: Fine +CONTROL: integer ratio)"),
    G_CALLBACK(CmViewerButtonArm),
    FrameB,
    NGRAPH_FRAME_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxSectionAction",
    NULL,
    N_("Section axis"),
    N_("Section Graph"),
    N_("New Section Graph (+SHIFT: Fine +CONTROL: integer ratio)"),
    G_CALLBACK(CmViewerButtonArm),
    SectionB,
    NGRAPH_SECTION_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxCrossAction",
    NULL,
    N_("Cross axis"),
    N_("Cross Graph"),
    N_("New Cross Graph (+SHIFT: Fine +CONTROL: integer ratio)"),
    G_CALLBACK(CmViewerButtonArm),
    CrossB,
    NGRAPH_CROSS_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxSingleAction",
    NULL,
    N_("Single axis"),
    N_("Single Axis"),
    N_("New Single Axis (+SHIFT: Fine +CONTROL: snap angle)"),
    G_CALLBACK(CmViewerButtonArm),
    SingleB,
    NGRAPH_SINGLE_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxTrimmingAction",
    NULL,
    N_("Trimming"),
    N_("Axis Trimming"),
    N_("Axis Trimming (+SHIFT: Fine)"),
    G_CALLBACK(CmViewerButtonArm),
    TrimB,
    NGRAPH_TRIMMING_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxEvaluateAction",
    NULL,
    N_("Evaluate"),
    N_("Evaluate Data"),
    N_("Evaluate Data Point"),
    G_CALLBACK(CmViewerButtonArm),
    EvalB,
    NGRAPH_EVAL_ICON_FILE,
  },
  {
    ACTION_TYPE_RADIO,
    "ToolboxZoomAction",
    NULL,
    N_("Zoom"),
    N_("Viewer Zoom"),
    N_("Viewer Zoom-In (+CONTROL: Zoom-Out +SHIFT: Centering)"),
    G_CALLBACK(CmViewerButtonArm),
    ZoomB,
    NGRAPH_ZOOM_ICON_FILE,
  },
};

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

void
set_axis_undo_button_sensitivity(int state)
{
  static GtkAction *axis_undo = NULL;

  if (axis_undo == NULL){
    axis_undo = gtk_action_group_get_action(ActionGroup, "AxisScaleUndoAction");
  }
  gtk_action_set_sensitive(axis_undo, state);
}

void
set_draw_lock(int lock)
{
  DrawLock = lock;
}

#ifndef WINDOWS
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
#endif	/* WINDOWS */

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
  int id;

  if (obj == NULL) {
    obj = chkobject("gra2gdk");
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

void
create_addin_menu(void)
{
  GtkWidget *menu, *item, *parent;
  struct script *fcur;

  parent = gtk_ui_manager_get_widget(NgraphUi, "/MenuBar/GraphMenu/GraphAddin");
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
}

static void
set_action_sensitivity(struct actions *actions, int *focus_type, int n)
{
  int i;

  for (i = 0; i < n; i++) {
    gtk_action_set_sensitive(actions[i].action, focus_type[actions[i].type]);
  }
}

static void
set_focus_sensitivity_sub(const struct Viewer *d, int insensitive)
{
  int num, type;
  GtkClipboard *clip;
  gboolean state;
  struct actions actions[] = {
    {"EditCutAction"		, NULL, FOCUS_TYPE_1},
    {"EditCopyAction"		, NULL, FOCUS_TYPE_1},
    {"EditRotateCCWAction"	, NULL, FOCUS_TYPE_2},
    {"EditRotateCWAction"	, NULL, FOCUS_TYPE_2},
    {"EditFlipVActiopn"		, NULL, FOCUS_TYPE_3},
    {"EditFlipHAction"		, NULL, FOCUS_TYPE_3},
    {"EditDeleteAction"		, NULL, FOCUS_TYPE_4},
    {"EditDuplicateAction"	, NULL, FOCUS_TYPE_4},
    {"EditAlignLeftAction"	, NULL, FOCUS_TYPE_4},
    {"EditAlignRightAction"	, NULL, FOCUS_TYPE_4},
    {"EditAlignHCenterAction"	, NULL, FOCUS_TYPE_4},
    {"EditAlignTopAction"	, NULL, FOCUS_TYPE_4},
    {"EditAlignBottomAction"	, NULL, FOCUS_TYPE_4},
    {"EditAlignVCenterAction"	, NULL, FOCUS_TYPE_4},
    {"EditOrderTopAction"	, NULL, FOCUS_TYPE_5},
    {"EditOrderUpAction"	, NULL, FOCUS_TYPE_5},
    {"EditOrderDownAction"	, NULL, FOCUS_TYPE_6},
    {"EditOrderBottomAction"	, NULL, FOCUS_TYPE_6},

    {"PopupUpdateAction"	, NULL, FOCUS_TYPE_4},
  };
  struct actions edit_paste_action = {"EditPasteAction", NULL};
  int focus_type[FOCUS_TYPE_NUM], i, n;

  n = sizeof(actions) / sizeof(*actions);

  if (actions[0].action == NULL) {
    for (i = 0; i < n; i++) {
      actions[i].action = gtk_action_group_get_action(ActionGroup, actions[i].name);
    }
    edit_paste_action.action = gtk_action_group_get_action(ActionGroup, edit_paste_action.name);
  }

  num = check_focused_obj_type(d, &type);

  if (insensitive) {
    focus_type[FOCUS_TYPE_1] = FALSE;
    focus_type[FOCUS_TYPE_2] = FALSE;
    focus_type[FOCUS_TYPE_3] = FALSE;
    focus_type[FOCUS_TYPE_4] = FALSE;
    focus_type[FOCUS_TYPE_5] = FALSE;
    focus_type[FOCUS_TYPE_6] = FALSE;
    set_action_sensitivity(actions, focus_type, n);

    gtk_action_set_sensitive(edit_paste_action.action, FALSE);
    return;
  }

  focus_type[FOCUS_TYPE_1] = (! (type & FOCUS_OBJ_TYPE_AXIS) && (type & (FOCUS_OBJ_TYPE_MERGE | FOCUS_OBJ_TYPE_LEGEND)));
  focus_type[FOCUS_TYPE_2] = (! (type & FOCUS_OBJ_TYPE_MERGE) && (type & (FOCUS_OBJ_TYPE_AXIS | FOCUS_OBJ_TYPE_LEGEND)));
  focus_type[FOCUS_TYPE_3] = (! (type & (FOCUS_OBJ_TYPE_MERGE | FOCUS_OBJ_TYPE_TEXT)) && (type & (FOCUS_OBJ_TYPE_AXIS | FOCUS_OBJ_TYPE_LEGEND)));
  focus_type[FOCUS_TYPE_4] = (num > 0);
  focus_type[FOCUS_TYPE_5] = FALSE;
  focus_type[FOCUS_TYPE_6] = FALSE;

  if (num == 1 && (type & (FOCUS_OBJ_TYPE_LEGEND | FOCUS_OBJ_TYPE_MERGE))) {
    int id, last_id;
    struct FocusObj *focus;

    focus= * (struct FocusObj **) arraynget(d->focusobj, 0);
    id = chkobjoid(focus->obj, focus->oid);
    last_id = chkobjlastinst(focus->obj);

    focus_type[FOCUS_TYPE_5] = (id > 0);
    focus_type[FOCUS_TYPE_6] = (id < last_id);
  }

  set_action_sensitivity(actions, focus_type, n);

  clip = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
  state = gtk_clipboard_wait_is_text_available(clip);

  switch (d->Mode) {
  case PointB:
  case LegendB:
    gtk_action_set_sensitive(edit_paste_action.action, state);
    break;
  default:
    gtk_action_set_sensitive(edit_paste_action.action, FALSE);
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
clear_information(GtkAction *w, gpointer user_data)
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

static int
create_ui_from_file(const gchar *ui_file)
{
  GError *err;
  char *home, *filename;
  int id;

  home = get_home();
  if (home) {
    filename = g_strdup_printf("%s/%s", home, ui_file);
    if (naccess(filename, R_OK) == 0) {
      id = gtk_ui_manager_add_ui_from_file(NgraphUi, filename, &err);
      g_free(filename);
      return id;
    }
    g_free(filename);
  }

  filename = g_strdup_printf("%s/%s", CONFDIR, ui_file);
  if (naccess(filename, R_OK) == 0) {
    id = gtk_ui_manager_add_ui_from_file(NgraphUi, filename, &err);
    g_free(filename);
    return id;
  }
  g_free(filename);

  return -1;
}

static void
create_markpixmap(GtkWidget *win)
{
#if GTK_CHECK_VERSION(3, 0, 0)
  cairo_surface_t *pix;
#else
  GdkPixmap *pix;
#endif
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
#if GTK_CHECK_VERSION(3, 0, 0)
      pix = gra2gdk_create_pixmap(local, MARK_PIX_SIZE, MARK_PIX_SIZE,
				  1.0, 1.0, 1.0);
#else
      pix = gra2gdk_create_pixmap(local, window,
				  MARK_PIX_SIZE, MARK_PIX_SIZE,
				  1.0, 1.0, 1.0);
#endif
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
#if GTK_CHECK_VERSION(3, 0, 0)
      cairo_surface_destroy(NgraphApp.markpix[i]);
#else
      g_object_unref(NgraphApp.markpix[i]);
#endif
    }
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
#if GTK_CHECK_VERSION(3, 0,0)
    g_object_unref(NgraphApp.cursor[i]);
#else
    gdk_cursor_unref(NgraphApp.cursor[i]);
#endif
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

#if ! GTK_CHECK_VERSION(3, 4, 0)
static void
detach_toolbar(GtkHandleBox *handlebox, GtkWidget *widget, gpointer user_data)
{
  gtk_toolbar_set_show_arrow(GTK_TOOLBAR(widget), GPOINTER_TO_INT(user_data));
}

static GtkWidget *
create_toolbar_box(GtkWidget *box, GtkWidget *t, GtkOrientation o)
{
  GtkWidget *w;
  GtkPositionType p;

  if (t == NULL) {
    return NULL;
  }

  if (o == GTK_ORIENTATION_HORIZONTAL) {
    p =  GTK_POS_LEFT;
  } else {
    p =  GTK_POS_TOP;
  }

  gtk_toolbar_set_style(GTK_TOOLBAR(t), GTK_TOOLBAR_ICONS);
  gtk_toolbar_set_show_arrow(GTK_TOOLBAR(t), TRUE);
  gtk_orientable_set_orientation(GTK_ORIENTABLE(t), o);
  w = gtk_handle_box_new();
  g_signal_connect(w, "child-attached", G_CALLBACK(detach_toolbar), GINT_TO_POINTER(TRUE));
  g_signal_connect(w, "child-detached", G_CALLBACK(detach_toolbar), GINT_TO_POINTER(FALSE));
  gtk_handle_box_set_handle_position(GTK_HANDLE_BOX(w), p);
  gtk_container_add(GTK_CONTAINER(w), t);
  gtk_box_pack_start(GTK_BOX(box), w, FALSE, FALSE, 0);

  return w;
}
#endif

static GtkWidget *
create_message_box(GtkWidget **label1, GtkWidget **label2)
{
  GtkWidget *frame, *w, *hbox;

  frame = gtk_frame_new(NULL);
  gtk_frame_set_shadow_type(GTK_FRAME(frame), GTK_SHADOW_IN);

#if GTK_CHECK_VERSION(3, 0, 0)
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);
#else
  hbox = gtk_hbox_new(FALSE, 4);
#endif

  w = gtk_label_new(NULL);
#if GTK_CHECK_VERSION(3, 4, 0)
  gtk_widget_set_halign(w, GTK_ALIGN_END);
#else
  gtk_misc_set_alignment(GTK_MISC(w), 1.0, 0.5);
#endif
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
  *label1 = w;

  w = gtk_label_new(NULL);
#if GTK_CHECK_VERSION(3, 4, 0)
  gtk_widget_set_halign(w, GTK_ALIGN_START);
#else
  gtk_misc_set_alignment(GTK_MISC(w), 0, 0.5);
#endif
  gtk_label_set_width_chars(GTK_LABEL(w), 16);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
  *label2 = w;

  gtk_container_add(GTK_CONTAINER(frame), hbox);

  return frame;
}

static void
set_toolbar_caption(GtkToolItem *item)
{
  GtkAction *action;
  const gchar *caption;

  action = gtk_activatable_get_related_action(GTK_ACTIVATABLE(item));
  if (action == NULL) {
    return;
  }

  caption = g_object_get_data(G_OBJECT(action), "caption");
  if (caption == NULL) {
    return;
  }

  g_signal_connect(gtk_bin_get_child(GTK_BIN(item)),
		   "enter-notify-event",
		   G_CALLBACK(tool_button_enter_leave_cb),
		   _(caption));

  g_signal_connect(gtk_bin_get_child(GTK_BIN(item)),
		   "leave-notify-event",
		   G_CALLBACK(tool_button_enter_leave_cb), NULL);
}

static void
set_btn_press_cb(GtkToolItem *item, GCallback btn_press_func)
{
  GtkAction *action;

  action = gtk_activatable_get_related_action(GTK_ACTIVATABLE(item));
  if (action == NULL) {
    return;
  }

  g_signal_connect(gtk_bin_get_child(GTK_BIN(item)),
		   "button-press-event",
		   btn_press_func, NULL);
}

static GtkWidget *
get_toolbar(GtkUIManager *ui, const gchar *path, GCallback btn_press_func)
{
  GtkWidget *w;
  GtkToolItem *item;
  int n, i;

  w = gtk_ui_manager_get_widget(ui, path);
  if (w == NULL) {
    return NULL;
  }

  n = gtk_toolbar_get_n_items(GTK_TOOLBAR(w));
  for (i = 0; i < n; i++) {
    item = gtk_toolbar_get_nth_item(GTK_TOOLBAR(w), i);
    set_toolbar_caption(item);

    if (btn_press_func) {
      set_btn_press_cb(item, btn_press_func);
    }
  }

  return w;
}

static void
set_window_action_visibility(int visibility)
{
  char *name1[] = {
    "ViewToggleDataWindowAction",
    "ViewToggleAxisWindowAction",
    "ViewToggleLegendWindowAction",
    "ViewToggleMergeWindowAction",
    "ViewToggleInformationWindowAction",
    "ViewToggleCoordinateWindowAction",
    "ViewDefaultWindowConfigAction",
  };
  char *name2[] = {
    "ViewSidebarAction",
  };
  GtkAction *action;
  unsigned int i;

  for (i = 0; i < sizeof(name1) / sizeof(*name1); i++) {
    action = gtk_action_group_get_action(ActionGroup, name1[i]);
    gtk_action_set_visible(action, visibility);
  }

  for (i = 0; i < sizeof(name2) / sizeof(*name2); i++) {
    action = gtk_action_group_get_action(ActionGroup, name2[i]);
    gtk_action_set_visible(action, ! visibility);
  }
}

#define OBJ_ID_KEY "ngraph_object_id"

static void
window_to_tab(struct SubWin *win, GtkWidget *tab, const char *icon_file, const char *tip)
{
  GtkWidget *w, *icon, *dialog;
  int obj_id;

  obj_id = chkobjectid(win->data.data->obj);
  dialog = win->Win;

  w = gtk_bin_get_child(GTK_BIN(dialog));
  g_object_ref(w);
  g_object_set_data(G_OBJECT(w), OBJ_ID_KEY, GINT_TO_POINTER(obj_id));
  gtk_container_remove(GTK_CONTAINER(dialog), w);

  icon = gtk_image_new_from_file(icon_file);
  gtk_widget_set_tooltip_text(icon, tip);

  gtk_notebook_append_page(GTK_NOTEBOOK(tab), w, icon);
  gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(tab), w, TRUE);
  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(tab), w, TRUE);
  gtk_notebook_set_menu_label_text(GTK_NOTEBOOK(tab), w, tip);
}

static void
tab_to_window(GtkWidget *dialog, GtkWidget *tab, int page)
{
  GtkWidget *w;

  w = gtk_notebook_get_nth_page(GTK_NOTEBOOK(tab), page);
  g_object_ref(w);		/* FIXME: can avoid to call of the function? */
  gtk_notebook_remove_page(GTK_NOTEBOOK(tab), page);
  gtk_container_add(GTK_CONTAINER(dialog), w);
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
  int i, position;
  struct objlist *obj;

  for (i = 0; i < n; i++) {
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
  int i, n, obj_id;
  GtkWidget *w;

  n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(tab));
  for (i = 0; i < n; i++){
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
  GtkWidget *tab;
  struct obj_tab_info tab_info[] = {
    {0, 0, &Menulocal.file_tab,      0, "file"},
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
    tab = gtk_paned_get_child1(GTK_PANED(NgraphApp.Viewer.side_pane2));
    save_tab_position_sub(tab, tab_info + i, 0);

    tab = gtk_paned_get_child2(GTK_PANED(NgraphApp.Viewer.side_pane2));
    save_tab_position_sub(tab, tab_info + i, 100);
  }
}

static void
multi_to_single(void)
{
  int i, j, n, tab_n, obj_id, height, width;
  struct obj_list_data *obj_data;
  GtkWidget *legend_tab, *icon, *w, *tab;
  struct obj_tab_info tab_info[] = {
    {0, 0, &Menulocal.file_tab,      0, "file"},
    {0, 0, &Menulocal.axis_tab,      0, "axis"},
    {0, 0, &Menulocal.merge_tab,     0, "merge"},
    {0, 0, &Menulocal.path_tab,      0, "path"},
    {0, 0, &Menulocal.rectangle_tab, 0, "rectangle"},
    {0, 0, &Menulocal.arc_tab,       0, "arc"},
    {0, 0, &Menulocal.mark_tab,      0, "mark"},
    {0, 0, &Menulocal.text_tab,      0, "text"},
  };

  tab_n = sizeof(tab_info) / sizeof(*tab_info);
  init_tab_info(tab_info, tab_n);

  legend_tab =  gtk_bin_get_child(GTK_BIN(NgraphApp.LegendWin.Win));

  for (j = 0; j < tab_n; j++) {
    if (tab_info[j].tab > 0) {
      tab = gtk_paned_get_child2(GTK_PANED(NgraphApp.Viewer.side_pane2));
    } else {
      tab = gtk_paned_get_child1(GTK_PANED(NgraphApp.Viewer.side_pane2));
    }
    if (strcmp(tab_info[j].obj_name, "file") == 0) {
#ifdef WINDOWS
      char *str;
      str = g_strdup_printf("%s%s", PIXMAPDIR, NGRAPH_FILEWIN_ICON_FILE);
      window_to_tab(&NgraphApp.FileWin, tab, str, _("data"));
      g_free(str);
#else
      window_to_tab(&NgraphApp.FileWin, tab, NGRAPH_FILEWIN_ICON_FILE, _("data"));
#endif
    } else if (strcmp(tab_info[j].obj_name, "axis") == 0) {
#ifdef WINDOWS
      char *str;
      str = g_strdup_printf("%s%s", PIXMAPDIR, NGRAPH_AXISWIN_ICON_FILE);
      window_to_tab(&NgraphApp.AxisWin, tab, str, _(tab_info[j].obj_name));
      g_free(str);
#else
      window_to_tab(&NgraphApp.AxisWin, tab, NGRAPH_AXISWIN_ICON_FILE, _(tab_info[j].obj_name));
#endif
    } else if (strcmp(tab_info[j].obj_name, "merge") == 0) {
#ifdef WINDOWS
      char *str;
      str = g_strdup_printf("%s%s", PIXMAPDIR, NGRAPH_MERGEWIN_ICON_FILE);
      window_to_tab(&NgraphApp.MergeWin, tab, str, _(tab_info[j].obj_name));
      g_free(str);
#else
      window_to_tab(&NgraphApp.MergeWin, tab, NGRAPH_MERGEWIN_ICON_FILE, _(tab_info[j].obj_name));
#endif
    } else {
      n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(legend_tab));
      for (i = 0; i < n; i++) {
	w = gtk_notebook_get_nth_page(GTK_NOTEBOOK(legend_tab), i);
	icon = gtk_notebook_get_tab_label(GTK_NOTEBOOK(legend_tab), w);
	obj_data = g_object_get_data(G_OBJECT(icon), "ngraph_object_data");
	obj_id = chkobjectid(obj_data->obj);
	if (obj_id == tab_info[j].obj_id) {
	  g_object_ref(w);
	  g_object_ref(icon);
	  g_object_set_data(G_OBJECT(w), OBJ_ID_KEY, GINT_TO_POINTER(obj_id));
	  gtk_notebook_remove_page(GTK_NOTEBOOK(legend_tab), i);
	  gtk_notebook_append_page(GTK_NOTEBOOK(tab), w, icon);
	  gtk_notebook_set_tab_detachable(GTK_NOTEBOOK(tab), w, TRUE);
	  gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(tab), w, TRUE);
	  gtk_notebook_set_menu_label_text(GTK_NOTEBOOK(tab), w, _(tab_info[j].obj_name));
	  break;
	}
      }
    }
  }

  w =  gtk_bin_get_child(GTK_BIN(NgraphApp.CoordWin.Win));
  g_object_ref(w);
  gtk_container_remove(GTK_CONTAINER(NgraphApp.CoordWin.Win), w);
  gtk_paned_pack1(GTK_PANED(NgraphApp.Viewer.side_pane3), w, FALSE, TRUE);

  w =  gtk_bin_get_child(GTK_BIN(NgraphApp.InfoWin.Win));
  g_object_ref(w);
  gtk_container_remove(GTK_CONTAINER(NgraphApp.InfoWin.Win), w);
  gtk_paned_pack2(GTK_PANED(NgraphApp.Viewer.side_pane3), w, TRUE, TRUE);

  set_window_action_visibility(FALSE);
  set_pane_position();

  if (Menulocal.sidebar) {
    gtk_widget_show(NgraphApp.Viewer.side_pane1);
  }

  window_action_set_active(TypeFileWin, FALSE);
  window_action_set_active(TypeAxisWin, FALSE);
  window_action_set_active(TypeLegendWin, FALSE);
  window_action_set_active(TypeInfoWin, FALSE);
  window_action_set_active(TypeCoordWin, FALSE);

  if (! Menulocal.single_window_mode) {
    gtk_window_get_size(GTK_WINDOW(TopLevel), &width, &height);
    if (Menulocal.filewidth > 0) {
      width += Menulocal.filewidth;
    }
    gtk_window_resize(GTK_WINDOW(TopLevel), width, height);
  }

  Menulocal.single_window_mode = TRUE;
}

static void
check_move_widget(GtkWidget *tab2)
{
  int i, n, obj_id;
  GtkWidget *w;

  n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(tab2));
  for (i = n - 1; i >= 0; i--) {
    w = gtk_notebook_get_nth_page(GTK_NOTEBOOK(tab2), i);
    obj_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), OBJ_ID_KEY));
    if (obj_id == chkobjectid(NgraphApp.MergeWin.data.data->obj)) {
      tab_to_window(NgraphApp.MergeWin.Win, tab2, i);
    } else if (obj_id == chkobjectid(NgraphApp.AxisWin.data.data->obj)) {
      tab_to_window(NgraphApp.AxisWin.Win, tab2, i);
    } else if (obj_id == chkobjectid(NgraphApp.FileWin.data.data->obj)) {
      tab_to_window(NgraphApp.FileWin.Win, tab2, i);
    }
  }
}

static void
check_move_legend_widget(GtkWidget *tab, GtkWidget *tab2, int obj_id)
{
  int i, n, obj_id2;
  GtkWidget *w, *icon;

  n = gtk_notebook_get_n_pages(GTK_NOTEBOOK(tab2));
  for (i = 0; i < n; i++) {
    w = gtk_notebook_get_nth_page(GTK_NOTEBOOK(tab2), i);
    obj_id2 = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(w), OBJ_ID_KEY));
    if (obj_id2 == obj_id) {
      icon = gtk_notebook_get_tab_label(GTK_NOTEBOOK(tab2), w);
      g_object_ref(w);
      g_object_ref(icon);
      gtk_notebook_remove_page(GTK_NOTEBOOK(tab2), i);
      gtk_notebook_append_page(GTK_NOTEBOOK(tab), w, icon);
      break;
    }
  }
}

static void
single_to_multi(void)
{
  int obj_id, width, height;
  GtkWidget *tab, *w, *tab2, *tab3;
  struct obj_list_data *obj_data;

  save_tab_position();

  set_window_action_visibility(TRUE);
  get_pane_position();

  tab2 = gtk_paned_get_child2(GTK_PANED(NgraphApp.Viewer.side_pane2));
  check_move_widget(tab2);

  tab2 = gtk_paned_get_child1(GTK_PANED(NgraphApp.Viewer.side_pane2));
  check_move_widget(tab2);

  obj_data = NgraphApp.LegendWin.data.data;
  tab =  gtk_bin_get_child(GTK_BIN(NgraphApp.LegendWin.Win));
  tab2 = gtk_paned_get_child2(GTK_PANED(NgraphApp.Viewer.side_pane2));
  tab3 = gtk_paned_get_child1(GTK_PANED(NgraphApp.Viewer.side_pane2));
  for (; obj_data; obj_data = obj_data->next) {
    obj_id = chkobjectid(obj_data->obj);
    check_move_legend_widget(tab, tab2, obj_id);
    check_move_legend_widget(tab, tab3, obj_id);
  }

  w = gtk_paned_get_child1(GTK_PANED(NgraphApp.Viewer.side_pane3));
  g_object_ref(w);
  gtk_container_remove(GTK_CONTAINER(NgraphApp.Viewer.side_pane3), w);
  gtk_container_add(GTK_CONTAINER(NgraphApp.CoordWin.Win), w);

  w = gtk_paned_get_child2(GTK_PANED(NgraphApp.Viewer.side_pane3));
  g_object_ref(w);
  gtk_container_remove(GTK_CONTAINER(NgraphApp.Viewer.side_pane3), w);
  gtk_container_add(GTK_CONTAINER(NgraphApp.InfoWin.Win), w);

  gtk_widget_hide(NgraphApp.Viewer.side_pane1);

  if (Menulocal.single_window_mode) {
    gtk_window_get_size(GTK_WINDOW(TopLevel), &width, &height);
    if (Menulocal.filewidth > 0) {
      width -= Menulocal.filewidth;
    }
    gtk_window_resize(GTK_WINDOW(TopLevel), width, height);
  }

  CmReloadWindowConfig(NULL, NULL);

  Menulocal.single_window_mode = FALSE;
}

static void
edit_menu_shown(GtkWidget *w, gpointer user_data)
{
  struct Viewer *d;

  d = (struct Viewer *) user_data;

  set_focus_sensitivity(d);
}

static void
setupwindow(void)
{
  GtkWidget *w, *hbox, *hbox2, *vbox, *vbox2, *table, *hpane1, *hpane2, *vpane1, *vpane2, *item;

#if GTK_CHECK_VERSION(3, 0, 0)
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  vbox2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
  hbox2 = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
#else
  vbox = gtk_vbox_new(FALSE, 0);
  vbox2 = gtk_vbox_new(FALSE, 0);
  hbox = gtk_hbox_new(FALSE, 0);
  hbox2 = gtk_hbox_new(FALSE, 0);
#endif

  w = gtk_ui_manager_get_widget(NgraphUi, "/MenuBar");
  if (w) {
    gtk_box_pack_start(GTK_BOX(vbox2), w, FALSE, FALSE, 0);
  }
  read_keymap_file();
  NgraphApp.Viewer.menu = w;

  w = get_toolbar(NgraphUi, "/CommandToolBar", NULL);
#if GTK_CHECK_VERSION(3, 4, 0)
  CToolbar = w;
  gtk_toolbar_set_style(GTK_TOOLBAR(w), GTK_TOOLBAR_ICONS);
  gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
#else
  CToolbar = create_toolbar_box(vbox, w, GTK_ORIENTATION_HORIZONTAL);
#endif

  w = get_toolbar(NgraphUi, "/ToolBox", G_CALLBACK(CmViewerButtonPressed));
#if GTK_CHECK_VERSION(3, 4, 0)
  PToolbar = w;
  gtk_orientable_set_orientation(GTK_ORIENTABLE(w), GTK_ORIENTATION_VERTICAL);
  gtk_toolbar_set_style(GTK_TOOLBAR(w), GTK_TOOLBAR_ICONS);
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 0);
#else
  PToolbar = create_toolbar_box(hbox, w, GTK_ORIENTATION_VERTICAL);
#endif

  NgraphApp.Viewer.popup = gtk_ui_manager_get_widget(NgraphUi, "/ViewerPopup");
  g_signal_connect(NgraphApp.Viewer.popup, "show", G_CALLBACK(edit_menu_shown), &NgraphApp.Viewer);

  item = gtk_ui_manager_get_widget(NgraphUi, "/MenuBar/EditMenu");
  if (item) {
    w = gtk_menu_item_get_submenu(GTK_MENU_ITEM(item));
    if (w) {
      g_signal_connect(w, "show", G_CALLBACK(edit_menu_shown), &NgraphApp.Viewer);
    }
  }
#if GTK_CHECK_VERSION(3, 2, 0)
  NgraphApp.Viewer.HScroll = gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, NULL);
  NgraphApp.Viewer.VScroll = gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, NULL);
#else
  NgraphApp.Viewer.HScroll = gtk_hscrollbar_new(NULL);
  NgraphApp.Viewer.VScroll = gtk_vscrollbar_new(NULL);
#endif
  NgraphApp.Viewer.HRuler = nruler_new(GTK_ORIENTATION_HORIZONTAL);
  NgraphApp.Viewer.VRuler = nruler_new(GTK_ORIENTATION_VERTICAL);
  NgraphApp.Viewer.Win = gtk_drawing_area_new();

#if GTK_CHECK_VERSION(3, 4, 0)
  table = gtk_grid_new();
#else
  table = gtk_table_new(3, 3, FALSE);
#endif

#if GTK_CHECK_VERSION(3, 4, 0)
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
#else
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
#endif

#if GTK_CHECK_VERSION(3, 2, 0)
  vpane2 = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
#else
  vpane2 = gtk_vpaned_new();
#endif
  NgraphApp.Viewer.side_pane2 = vpane2;

  w = gtk_notebook_new();
  gtk_notebook_popup_enable(GTK_NOTEBOOK(w));
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w), GTK_POS_LEFT);
#if GTK_CHECK_VERSION(2, 24, 0)
  gtk_notebook_set_group_name(GTK_NOTEBOOK(w), SIDE_PANE_TAB_ID);
#else
  gtk_notebook_set_group(GTK_NOTEBOOK(w), SIDE_PANE_TAB_ID);
#endif
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(w), TRUE);
  gtk_paned_add1(GTK_PANED(vpane2), w);

  w = gtk_notebook_new();
  gtk_notebook_popup_enable(GTK_NOTEBOOK(w));
  gtk_notebook_set_tab_pos(GTK_NOTEBOOK(w), GTK_POS_LEFT);
#if GTK_CHECK_VERSION(2, 24, 0)
  gtk_notebook_set_group_name(GTK_NOTEBOOK(w), SIDE_PANE_TAB_ID);
#else
  gtk_notebook_set_group(GTK_NOTEBOOK(w), SIDE_PANE_TAB_ID);
#endif
  gtk_notebook_set_scrollable(GTK_NOTEBOOK(w), TRUE);
  gtk_paned_add2(GTK_PANED(vpane2), w);

#if GTK_CHECK_VERSION(3, 2, 0)
  hpane2 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
#else
  hpane2 = gtk_hpaned_new();
#endif
  NgraphApp.Viewer.side_pane3 = hpane2;

#if GTK_CHECK_VERSION(3, 2, 0)
  vpane1 = gtk_paned_new(GTK_ORIENTATION_VERTICAL);
#else
  vpane1 = gtk_vpaned_new();
#endif
  gtk_paned_pack1(GTK_PANED(vpane1), vpane2, TRUE, TRUE);
  gtk_paned_pack2(GTK_PANED(vpane1), hpane2, FALSE, TRUE);
  NgraphApp.Viewer.side_pane1 = vpane1;

#if GTK_CHECK_VERSION(3, 2, 0)
  hpane1 = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
#else
  hpane1 = gtk_hpaned_new();
#endif
  gtk_paned_add1(GTK_PANED(hpane1), vbox);
  gtk_paned_add2(GTK_PANED(hpane1), vpane1);
  NgraphApp.Viewer.main_pane = hpane1;

  gtk_box_pack_start(GTK_BOX(hbox), table, TRUE, TRUE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), hbox, TRUE, TRUE, 0);
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
init_ngraph_app_struct(void)
{
  NgraphApp.Viewer.Win = NULL;
  NgraphApp.Viewer.popup = NULL;

  memset(&NgraphApp.FileWin, 0, sizeof(NgraphApp.FileWin));
  NgraphApp.FileWin.type = TypeFileWin;

  memset(&NgraphApp.AxisWin, 0, sizeof(NgraphApp.AxisWin));
  NgraphApp.AxisWin.type = TypeAxisWin;

  memset(&NgraphApp.LegendWin, 0, sizeof(NgraphApp.LegendWin));
  NgraphApp.LegendWin.type = TypeLegendWin;

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

  NgraphApp.Interrupt = FALSE;
}

static void
create_sub_windows(void)
{
  CmInformationWindow(NULL, NULL);
  CmCoordinateWindow(NULL, NULL);
  CmMergeWindow(NULL, NULL);
  CmLegendWindow(NULL, NULL);
  CmAxisWindow(NULL, NULL);
  CmFileWindow(NULL, NULL);

  if (Menulocal.single_window_mode) {
    return;
  }

  if (Menulocal.dialogopen) {
    window_action_set_active(TypeInfoWin, TRUE);
  }

  if (Menulocal.coordopen) {
    window_action_set_active(TypeCoordWin, TRUE);
  }

  if (Menulocal.mergeopen) {
    window_action_set_active(TypeMergeWin, TRUE);
  }

  if (Menulocal.legendopen) {
    window_action_set_active(TypeLegendWin, TRUE);
  }

  if (Menulocal.axisopen) {
    window_action_set_active(TypeAxisWin, TRUE);
  }

  if (Menulocal.fileopen) {
    window_action_set_active(TypeFileWin, TRUE);
  }
}

static void
destroy_sub_windows(void)
{
  if (NgraphApp.FileWin.Win) {
    window_action_set_active(TypeFileWin, FALSE);
    gtk_widget_destroy(NgraphApp.FileWin.Win);
  }

  if (NgraphApp.AxisWin.Win) {
    window_action_set_active(TypeAxisWin, FALSE);
    gtk_widget_destroy(NgraphApp.AxisWin.Win);
  }

  if (NgraphApp.LegendWin.Win) {
    window_action_set_active(TypeLegendWin, FALSE);
    gtk_widget_destroy(NgraphApp.LegendWin.Win);
  }

  if (NgraphApp.MergeWin.Win) {
    window_action_set_active(TypeMergeWin, FALSE);
    gtk_widget_destroy(NgraphApp.MergeWin.Win);
  }

  if (NgraphApp.InfoWin.Win) {
    window_action_set_active(TypeInfoWin, FALSE);
    gtk_widget_destroy(NgraphApp.InfoWin.Win);
  }

  if (NgraphApp.CoordWin.Win) {
    window_action_set_active(TypeCoordWin, FALSE);
    gtk_widget_destroy(NgraphApp.CoordWin.Win);
  }
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
  static GtkAction *save_action = NULL;

  if (save_action == NULL){
    save_action = gtk_action_group_get_action(ActionGroup, "GraphSaveAction");
  }
  gtk_action_set_sensitive(save_action, state);
}

static void
toggle_view_cb(GtkToggleAction *action, gpointer data)
{
  int type, state;
  GtkWidget *w1 = NULL, *w2 = NULL;

  type = GPOINTER_TO_INT(data);

  if (action == NULL) {
    return;
  }
  state = gtk_toggle_action_get_active(action);

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
    w1 = CToolbar;
    break;
  case MenuIdTogglePToolbar:
    Menulocal.ptoolbar = state;
    w1 = PToolbar;
    break;
  case MenuIdToggleCrossGauge:
    ViewCross(state);
    break;
  }

  if (type == MenuIdToggleCrossGauge) {
    return;
  }

  if (state) {
    if (w1) {
      gtk_widget_show(w1);
    }
    if (w2) {
      gtk_widget_show(w2);
    }
  } else {
    if (w1) {
      gtk_widget_hide(w1);
    }
    if (w2) {
      gtk_widget_hide(w2);
    }
  }
}

static void
set_widget_visibility(void)
{
  unsigned int i;
  struct {
    int *state;
    const char *name;
  } id_array[] = {
    {&Menulocal.sidebar,    "ViewSidebarAction"},
    {&Menulocal.statusbar,  "ViewStatusbarAction"},
    {&Menulocal.ruler,      "ViewRulerAction"},
    {&Menulocal.scrollbar,  "ViewScrollbarAction"},
    {&Menulocal.ctoolbar,   "ViewCommandToolbarAction"},
    {&Menulocal.ptoolbar,   "ViewToolboxAction"},
    {&Menulocal.show_cross, "ViewCrossGaugeAction"},
  };
  GtkToggleAction *action;

  for (i = 0; i < sizeof (id_array) / sizeof(*id_array); i++) {
    action = GTK_TOGGLE_ACTION(gtk_action_group_get_action(ActionGroup, id_array[i].name));
    if (action) {
      gtk_toggle_action_set_active(action, *id_array[i].state);
      gtk_toggle_action_toggled(action);
    }
  }
}

static void
check_instance(struct objlist *obj)
{
  unsigned int i;
  struct {
    const char *name;
    GtkAction *action;
    const char *objname;
    struct objlist *obj;
  } actions[] = {
    {"DataPropertyAction", NULL, "file", NULL},
    {"DataCloseAction", NULL, "file", NULL},
    {"DataEditAction", NULL, "file", NULL},
    {"DataSaveAction", NULL, "file", NULL},
    {"DataMathAction", NULL, "file", NULL},

    {"AxisPropertyAction", NULL, "axis", NULL},
    {"AxisDeleteAction", NULL, "axis", NULL},
    {"AxisScaleZoomAction", NULL, "axis", NULL},
    {"AxisScaleClearAction", NULL, "axis", NULL},

    {"AxisGridPropertyAction", NULL, "axisgrid", NULL},
    {"AxisGridDeleteAction", NULL, "axisgrid", NULL},

    {"LegendPathPropertyAction", NULL, "path", NULL},
    {"LegendPathDeleteAction", NULL, "path", NULL},

    {"LegendRectanglePropertyAction", NULL, "rectangle", NULL},
    {"LegendRectangleDeleteAction", NULL, "rectangle", NULL},

    {"LegendArcPropertyAction", NULL, "arc", NULL},
    {"LegendArcDeleteAction", NULL, "arc", NULL},

    {"LegendMarkPropertyAction", NULL, "mark", NULL},
    {"LegendMarkDeleteAction", NULL, "mark", NULL},

    {"LegendTextPropertyAction", NULL, "text", NULL},
    {"LegendTextDeleteAction", NULL, "text", NULL},

    {"MergePropertyAction", NULL, "merge", NULL},
    {"MergeCloseAction", NULL, "merge", NULL},
  };

  if (actions[0].action == NULL) {
    for (i = 0; i < sizeof(actions) / sizeof(*actions); i++) {
      actions[i].action = gtk_action_group_get_action(ActionGroup, actions[i].name);
      actions[i].obj = chkobject(actions[i].objname);
    }
  }

  for (i = 0; i < sizeof(actions) / sizeof(*actions); i++) {
    if (actions[i].obj == obj) {
      gtk_action_set_sensitive(actions[i].action, obj->lastinst >= 0);
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
set_toggle_action(const char *name, int state)
{
  GtkAction *action;

  if (name == NULL) {
    return;
  }
  action = gtk_action_group_get_action(ActionGroup, name);

  if (action) {
    if (state < 0) {
      int active;
      active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
      gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), ! active);
    } else {
      gtk_toggle_action_set_active(GTK_TOGGLE_ACTION(action), state);
    }
  }
}

void
window_action_set_active(enum SubWinType type, int state)
{
  char *name;

  switch (type) {
  case TypeFileWin:
    name = "ViewToggleDataWindowAction";
    break;
  case TypeAxisWin:
    name = "ViewToggleAxisWindowAction";
    break;
  case TypeLegendWin:
    name = "ViewToggleLegendWindowAction";
    break;
  case TypeMergeWin:
    name = "ViewToggleMergeWindowAction";
    break;
  case TypeInfoWin:
    name = "ViewToggleInformationWindowAction";
    break;
  case TypeCoordWin:
    name = "ViewToggleCoordinateWindowAction";
    break;
  default:
    name = NULL;
  }

  set_toggle_action(name, state);
}

void
window_action_toggle(enum SubWinType type)
{
  window_action_set_active(type, -1);
}

gboolean
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

static GtkActionGroup *
create_action_group(struct NgraphActionEntry *entry, int n)
{
  int i, radio_index = 0;
  GSList *group = NULL;
  GtkRecentFilter *filter;
  GtkActionGroup *action_group;

  NgraphUi = gtk_ui_manager_new();
  action_group = gtk_action_group_new("Ngraph");

  for (i = 0; i < n; i++) {
    GtkAction *action;
    switch (entry[i].type) {
    case ACTION_TYPE_NORMAL:
      action = gtk_action_new(entry[i].name,
			      _(entry[i].label),
			      _(entry[i].tooltip),
			      entry[i].stock_id);
      break;
    case ACTION_TYPE_TOGGLE:
      action = GTK_ACTION(gtk_toggle_action_new(entry[i].name,
						_(entry[i].label),
						_(entry[i].tooltip),
						entry[i].stock_id));
      break;
    case ACTION_TYPE_RADIO:
      action = GTK_ACTION(gtk_radio_action_new(entry[i].name,
					       _(entry[i].label),
					       _(entry[i].tooltip),
					       entry[i].stock_id,
					       radio_index++));
      gtk_radio_action_set_group(GTK_RADIO_ACTION(action), group);
      group = gtk_radio_action_get_group(GTK_RADIO_ACTION(action));
      NgraphApp.viewb = GTK_RADIO_ACTION(action);
      break;
    case ACTION_TYPE_RECENT:
      action = gtk_recent_action_new_for_manager(entry[i].name,
						 _(entry[i].label),
						 _(entry[i].tooltip),
						 entry[i].stock_id,
						 Menulocal.ngpfilelist);

      filter = gtk_recent_filter_new();
      gtk_recent_filter_add_custom(filter,
				   GTK_RECENT_FILTER_URI |
				   GTK_RECENT_FILTER_MIME_TYPE |
				   GTK_RECENT_FILTER_APPLICATION,
				   recent_filter,
				   GINT_TO_POINTER(entry[i].user_data),
				   NULL);

      gtk_recent_action_set_show_numbers(GTK_RECENT_ACTION(action), TRUE);

      gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(action), filter);
      gtk_recent_chooser_set_filter(GTK_RECENT_CHOOSER(action), filter);
      gtk_recent_chooser_set_show_tips(GTK_RECENT_CHOOSER(action), TRUE);
      gtk_recent_chooser_set_show_icons(GTK_RECENT_CHOOSER(action), FALSE);
      gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER(action), TRUE);
#ifndef WINDOWS
      gtk_recent_chooser_set_show_not_found(GTK_RECENT_CHOOSER(action), FALSE);
#endif
      gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(action), GTK_RECENT_SORT_MRU);
      gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(action), 10);

      switch (entry[i].user_data) {
      case RECENT_TYPE_GRAPH:
	g_signal_connect(GTK_RECENT_CHOOSER(action), "item-activated", G_CALLBACK(CmGraphHistory), NULL);
	break;
      case RECENT_TYPE_DATA:
	g_signal_connect(GTK_RECENT_CHOOSER(action), "item-activated", G_CALLBACK(CmFileHistory), NULL);
	break;
      }
      break;
    default:
      action = NULL;
    }

    if (action == NULL) {
      continue;
    }

    if (entry[i].callback) {
      switch (entry[i].type) {
      case ACTION_TYPE_TOGGLE:
	g_signal_connect(action, "toggled", G_CALLBACK(entry[i].callback), GINT_TO_POINTER(entry[i].user_data));
	break;
      default:
	g_signal_connect(action, "activate", G_CALLBACK(entry[i].callback), GINT_TO_POINTER(entry[i].user_data));
      }
    }

    if (entry[i].icon) {
      GIcon *icon;
#ifdef WINDOWS
      char *str;
      str = g_strdup_printf("%s%s", PIXMAPDIR, entry[i].icon);
      icon = g_icon_new_for_string(str, NULL);
      g_free(str);
#else
      icon = g_icon_new_for_string(entry[i].icon, NULL);
#endif
      if (icon) {
	gtk_action_set_gicon(action, icon);
      }
    }

    if (entry[i].accel_path) {
      gtk_action_set_accel_path(action, entry[i].accel_path);

      if (entry[i].accel_key) {
	gtk_accel_map_add_entry(entry[i].accel_path, entry[i].accel_key, entry[i].accel_mods);
      }
    }

    if (entry[i].caption) {
      g_object_set_data(G_OBJECT(action), "caption", entry[i].caption);
    }

    gtk_action_group_add_action(action_group, action);
  }
  gtk_action_group_set_translation_domain(action_group, NULL);

  gtk_ui_manager_insert_action_group(NgraphUi, action_group, 0);

  return action_group;
}

void
show_ui_definition(void)
{
  if (NgraphUi) {
    printfstdout("%s", gtk_ui_manager_get_ui(NgraphUi));
  }
}

int
application(char *file)
{
  int i, terminated, ui_id;
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
#if GTK_CHECK_VERSION(3, 0, 0)
  gtk_window_set_has_resize_grip(GTK_WINDOW(TopLevel), TRUE);
#endif
  gtk_window_set_default_size(GTK_WINDOW(TopLevel), width, height);
  gtk_window_move(GTK_WINDOW(TopLevel), x, y);

  if (ActionGroup == NULL) {
    ActionGroup = create_action_group(ActionEntry, sizeof(ActionEntry) / sizeof(*ActionEntry));
  }

  ui_id = create_ui_from_file(UI_FILE);
  gtk_ui_manager_ensure_update(NgraphUi);
  AccelGroup = gtk_ui_manager_get_accel_group(NgraphUi);
  gtk_window_add_accel_group(GTK_WINDOW(TopLevel), AccelGroup);
  create_addin_menu();

#ifdef WINDOWS
  g_signal_connect(TopLevel, "window-state-event", G_CALLBACK(change_window_state_cb), NULL);
#endif
  g_signal_connect(TopLevel, "delete-event", G_CALLBACK(CloseCallback), NULL);
  g_signal_connect(TopLevel, "destroy-event", G_CALLBACK(CloseCallback), NULL);

  set_gdk_color(&white, 255, 255, 255);
  set_gdk_color(&gray,  0xaa, 0xaa, 0xaa);

  create_icon();
  initdialog();

  gtk_widget_show_all(GTK_WIDGET(TopLevel));
  reset_event();
  setupwindow();

  NgraphApp.FileName = NULL;

  gtk_widget_show_all(GTK_WIDGET(TopLevel));
  ViewerWinSetup();
  set_widget_visibility();

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

  if (file != NULL) {
    char *ext;
    ext = getextention(file);
    if (ext) {
      if ((strcmp0(ext, "PRM") == 0) || (strcmp0(ext, "prm") == 0)) {
	LoadPrmFile(file);
      } else if ((strcmp0(ext, "NGP") == 0) || (strcmp0(ext, "ngp") == 0)) {
	LoadNgpFile(file, Menulocal.loadpath, Menulocal.expand, Menulocal.expanddir, FALSE, NULL);
      }
    }
  }

  CmViewerDraw(NULL, GINT_TO_POINTER(FALSE));

#ifndef WINDOWS
  set_signal(SIGINT, 0, kill_signal_handler);
  set_signal(SIGTERM, 0, term_signal_handler);
#endif	/* WINDOWS */

  gtk_widget_show_all(GTK_WIDGET(TopLevel));
  set_widget_visibility();

  create_sub_windows();

  set_focus_sensitivity(&NgraphApp.Viewer);
  check_exist_instances(chkobject("draw"));

  set_newobj_cb(check_instance);
  set_delobj_cb(check_instance);

  if (Menulocal.single_window_mode) {
    GtkAction *action;
    int active;
    action = gtk_action_group_get_action(ActionGroup, "ViewToggleSingleWindowModeAction");
    active = gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(action));
    if (active) {
      multi_to_single();
    } else {
      set_toggle_action("ViewToggleSingleWindowModeAction", TRUE);
    }
  } else {
    gtk_widget_hide(NgraphApp.Viewer.side_pane1);
  }

  terminated = AppMainLoop();

  if (CheckIniFile()) {
    if (Menulocal.single_window_mode) {
      save_tab_position();
      get_pane_position();
      menu_save_config(SAVE_CONFIG_TYPE_GEOMETRY);
    }
    save_entry_history();
    menu_save_config(SAVE_CONFIG_TYPE_TOGGLE_VIEW |
		     SAVE_CONFIG_TYPE_OTHERS);
  }

  set_newobj_cb(NULL);
  set_delobj_cb(NULL);

  gtk_ui_manager_remove_ui(NgraphUi, ui_id);

#ifndef WINDOWS
  set_signal(SIGTERM, 0, SIG_DFL);
  set_signal(SIGINT, 0, SIG_DFL);
#endif	/* WINDOWS */

  ViewerWinClose();

  destroy_sub_windows();

  g_free(NgraphApp.FileName);
  NgraphApp.FileName = NULL;

  gtk_widget_destroy(TopLevel);
  NgraphApp.Viewer.Win = NULL;
  CurrentWindow = TopLevel = PToolbar = CToolbar = NULL;

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
  ViewerWinUpdate();
  UpdateAll2();
}

void
UpdateAll2(void)
{
  FileWinUpdate(NgraphApp.FileWin.data.data, TRUE);
  AxisWinUpdate(NgraphApp.AxisWin.data.data, TRUE);
  LegendWinUpdate(TRUE);
  MergeWinUpdate(NgraphApp.MergeWin.data.data, TRUE);
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
  ChangeDPI();
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
  main_window_redraw();
}

static void
CmReloadWindowConfig(GtkAction *w, gpointer user_data)
{
  gint x, y, w0, h0;

  window_action_set_active(TypeInfoWin, FALSE);
  window_action_set_active(TypeCoordWin, FALSE);
  window_action_set_active(TypeMergeWin, FALSE);
  window_action_set_active(TypeLegendWin, FALSE);
  window_action_set_active(TypeMergeWin, FALSE);
  window_action_set_active(TypeAxisWin, FALSE);
  window_action_set_active(TypeFileWin, FALSE);

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
    window_action_set_active(TypeInfoWin, TRUE);
    sub_window_set_geometry(&(NgraphApp.InfoWin), TRUE);
  }

  if (Menulocal.coordopen) {
    window_action_set_active(TypeCoordWin, TRUE);
    sub_window_set_geometry(&(NgraphApp.CoordWin), TRUE);
  }

  if (Menulocal.mergeopen) {
    window_action_set_active(TypeMergeWin, TRUE);
    sub_window_set_geometry(&(NgraphApp.MergeWin), TRUE);
  }

  if (Menulocal.legendopen) {
    window_action_set_active(TypeLegendWin, TRUE);
    sub_window_set_geometry(&(NgraphApp.LegendWin), TRUE);
  }

  if (Menulocal.axisopen) {
    window_action_set_active(TypeAxisWin, TRUE);
    sub_window_set_geometry(&(NgraphApp.AxisWin), TRUE);
  }

  if (Menulocal.fileopen) {
    window_action_set_active(TypeFileWin, TRUE);
    sub_window_set_geometry(&(NgraphApp.FileWin), TRUE);
  }
}

static void
CmToggleSingleWindowMode(GtkToggleAction *action, gpointer client_data)
{
  int state;

  if (action) {
    state = gtk_toggle_action_get_active(action);
  } else {
    state = TRUE;
  }

  if (state) {
    multi_to_single();
  } else {
    single_to_multi();
  }
}

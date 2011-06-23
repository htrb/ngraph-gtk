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
#define KEYMAP_FILE      "accel_map"
#define UI_FILE          "NgraphUI.xml"

#define USE_EXT_DRIVER 0

int Menulock = FALSE, DnDLock = FALSE;
struct NgraphApp NgraphApp;
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
    "ngraph_frame.png",
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
    "ngraph_section.png",
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
    "ngraph_cross.png",
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
    GDK_r,
    GDK_CONTROL_MASK
  },
  {
    ACTION_TYPE_RECENT,
    "GraphRecentAction",
    NULL,
    N_("_Recent graphs"),
    NULL,
    NULL,
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
    GDK_s,
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
    GDK_s,
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
    "GraphExportEMFAction",
    NULL,
    N_("_EMF file"),
    N_("Export as Windows Enhanced Metafile"), 
    NULL,
    G_CALLBACK(CmOutputMenu),
    MenuIdOutputEMFFile,
    NULL,
    "<Ngraph>/Graph/Export image/EMF File",
  },
#endif	/* CAIRO_HAS_WIN32_SURFACE */

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
    GDK_p,
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
    GDK_q,
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
    GDK_x,
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
    GDK_c,
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
    GDK_v,
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
    "ngraph_align_l.png",
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
    "ngraph_align_r.png",
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
    "ngraph_align_vc.png",
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
    "ngraph_align_t.png",
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
    "ngraph_align_b.png",
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
    "ngraph_align_hc.png",
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
    "ViewToggleDataWindowAction",
    NULL,
    "Data Window",
    "Data Window",
    N_("Activate Data Window"), 
    G_CALLBACK(CmFileWindow),
    0,
    "ngraph_filewin.png",
    "<Ngraph>/View/Data Window",
    GDK_F3,
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
    "ngraph_axiswin.png",
    "<Ngraph>/View/Axis Window",
    GDK_F4,
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
    "ngraph_legendwin.png",
    "<Ngraph>/View/Legend Window",
    GDK_F5,
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
    "ngraph_mergewin.png",
    "<Ngraph>/View/Merge Window",
    GDK_F6,
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
    "ngraph_coordwin.png",
    "<Ngraph>/View/Coordinate Window",
    GDK_F7,
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
    "ngraph_infowin.png",
    "<Ngraph>/View/Information Window",
    GDK_F8,
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
    "ngraph_draw.png",
    "<Ngraph>/View/Draw",
    GDK_d,
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
    "ngraph_draw.png",
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
    GDK_e,
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
    N_("_Clear information window"),
    NULL,
    NULL,
    G_CALLBACK(clear_information),
    0,
    NULL,
    "<Ngraph>/View/Clear information window",
  },
  {
    ACTION_TYPE_TOGGLE,
    "ViewStatusbarAction",
    NULL,
    N_("_Status bar"),
    NULL,
    NULL,
    G_CALLBACK(toggle_view_cb),
    MenuIdToggleStatusBar,
    NULL,
    "<Ngraph>/View/Status bar",
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
    GDK_plus,
    0,
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
    "DataOpenAction",
    GTK_STOCK_FILE,
    N_("_Open"),
    N_("Open Data"),
    N_("Open Data file"),
    G_CALLBACK(CmFileOpen),
    0,
    NULL,
    "<Ngraph>/Data/Open",
    GDK_o,
    GDK_CONTROL_MASK,
  },
  {
    ACTION_TYPE_NORMAL,
    "DataRecentAction",
    NULL,
    N_("_Recent data"),
    NULL,
    NULL,
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
    "ngraph_math.png",
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
    "ngraph_frame.png",
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
    "ngraph_section.png",
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
    "ngraph_cross.png",
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
    "ngraph_single.png",
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
    "ngraph_scale.png",
    "<Ngraph>/Axis/Scale Clear",
    GDK_c,
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
    GTK_STOCK_NEW,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmAxisGridNew),
    0,
    NULL,
    "<Ngraph>/Axis/Grid/New",
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
    "MergeOpenAction",
    GTK_STOCK_OPEN,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(CmMergeOpen),
    0,
    NULL,
    "<Ngraph>/Merge/Open", 
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
    "ngraph_line.png",
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
    "ngraph_rect.png",
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
    "ngraph_arc.png",
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
    "ngraph_mark.png",
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
    "ngraph_text.png",
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
    GDK_F1,
    0,
  },
  {
    ACTION_TYPE_NORMAL,
    "HelpAboputAction",
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
    G_CALLBACK(ViewerPopupMenu),
    VIEW_UPDATE,
  },
  {
    ACTION_TYPE_NORMAL,
    "PopupUpAction",
    GTK_STOCK_GO_UP,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(ViewerPopupMenu),
    VIEW_UP,
  },
  {
    ACTION_TYPE_NORMAL,
    "PopupDownAction",
    GTK_STOCK_GO_DOWN,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(ViewerPopupMenu),
    VIEW_DOWN,
  },
  {
    ACTION_TYPE_NORMAL,
    "PopupTopAction",
    GTK_STOCK_GOTO_TOP,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(ViewerPopupMenu),
    VIEW_TOP,
  },
  {
    ACTION_TYPE_NORMAL,
    "PopupBottomAction",
    GTK_STOCK_GOTO_BOTTOM,
    NULL,
    NULL,
    NULL,
    G_CALLBACK(ViewerPopupMenu),
    VIEW_LAST,
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
    "ngraph_point.png",
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
    "ngraph_legendpoint.png",
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
    "ngraph_axispoint.png",
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
    "ngraph_datapoint.png",
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
    "ngraph_line.png",
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
    "ngraph_rect.png",
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
    "ngraph_arc.png",
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
    "ngraph_mark.png",
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
    "ngraph_text.png",
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
    "ngraph_gauss.png",
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
    "ngraph_frame.png",
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
    "ngraph_section.png",
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
    "ngraph_cross.png",
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
    "ngraph_single.png",
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
    "ngraph_trimming.png",
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
    "ngraph_eval.png",
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
    "ngraph_zoom.png",
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

void
set_focus_sensitivity(const struct Viewer *d)
{
  int num, type;
  GtkClipboard *clip;
  gboolean state;
  unsigned int i;
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
  } actions[] = {
    {"EditCutAction"		, NULL, FOCUS_TYPE_1},
    {"EditCopyAction"		, NULL, FOCUS_TYPE_1},
    {"EditRotateCCWAction"	, NULL, FOCUS_TYPE_2},
    {"EditRotateCWAction"	, NULL, FOCUS_TYPE_2},
    {"EditFlipVActiopn"		, NULL, FOCUS_TYPE_3},
    {"EditFlipHAction"		, NULL, FOCUS_TYPE_3},
    {"EditDeleteAction"		, NULL, FOCUS_TYPE_4},
    {"EditAlignLeftAction"	, NULL, FOCUS_TYPE_4},
    {"EditAlignRightAction"	, NULL, FOCUS_TYPE_4},
    {"EditAlignHCenterAction"	, NULL, FOCUS_TYPE_4},
    {"EditAlignTopAction"	, NULL, FOCUS_TYPE_4},
    {"EditAlignBottomAction"	, NULL, FOCUS_TYPE_4},
    {"EditAlignVCenterAction"	, NULL, FOCUS_TYPE_4},

    {"PopupUpdateAction"	, NULL, FOCUS_TYPE_4},
    {"PopupTopAction"		, NULL, FOCUS_TYPE_5},
    {"PopupUpAction"		, NULL, FOCUS_TYPE_5},
    {"PopupDownAction"		, NULL, FOCUS_TYPE_6},
    {"PopupBottomAction"	, NULL, FOCUS_TYPE_6},
  };
  struct actions edit_paste_action = {"EditPasteAction", NULL};
  int focus_type[FOCUS_TYPE_NUM];

  if (actions[0].action == NULL) {
    for (i = 0; i < sizeof(actions) / sizeof(*actions); i++) {
      actions[i].action = gtk_action_group_get_action(ActionGroup, actions[i].name);
    }
    edit_paste_action.action = gtk_action_group_get_action(ActionGroup, edit_paste_action.name);
  }

  num = check_focused_obj_type(d, &type);

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

  for (i = 0; i < sizeof(actions) / sizeof(*actions); i++) {
    gtk_action_set_sensitive(actions[i].action, focus_type[actions[i].type]);
  }

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

void
create_recent_data_menu(void)
{
  int i, num;
  char **data, *basename;;
  GtkWidget *menu, *item, *parent;
  static GString *str = NULL;

  parent = gtk_ui_manager_get_widget(NgraphUi, "/MenuBar/DataMenu/DataRecent");
  if (parent == NULL) {
    return;
  }

  menu = gtk_menu_item_get_submenu(GTK_MENU_ITEM(parent));
  if (menu) {
    gtk_widget_destroy(menu);
  }

  num = arraynum(Menulocal.datafilelist);
  data = arraydata(Menulocal.datafilelist);

  gtk_widget_set_sensitive(parent, num > 0);
  if (num < 1) {
    return;
  }

  menu = gtk_menu_new();
  gtk_menu_item_set_submenu(GTK_MENU_ITEM(parent), menu);

  if (str == NULL) {
    str = g_string_new("");
  }

  for (i = 0; i < num; i++) {
    basename = g_path_get_basename(CHK_STR(data[i]));
    g_string_printf(str, "_%d: %s", i, basename);
    g_free(basename);
    add_underscore(str);
    item = gtk_menu_item_new_with_mnemonic(str->str);
    gtk_widget_set_tooltip_text(item, CHK_STR(data[i]));
    g_signal_connect(item, "activate", G_CALLBACK(CmFileHistory), GINT_TO_POINTER(i));
    gtk_menu_shell_append(GTK_MENU_SHELL(menu), item);
  }

  gtk_widget_show_all(menu);
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



#define MARK_PIX_SIZE 24
static void
create_markpixmap(GtkWidget *win)
{
  GdkPixmap *pix;
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
      pix = gra2gdk_create_pixmap(obj, inst, local, window,
				  MARK_PIX_SIZE, MARK_PIX_SIZE,
				  1.0, 1.0, 1.0);
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
#if GTK_CHECK_VERSION(2, 16, 0)
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

  return w;
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
  gtk_box_pack_start(GTK_BOX(hbox), w, FALSE, FALSE, 4);
  *label1 = w;

  w = gtk_label_new(NULL);
  gtk_misc_set_alignment(GTK_MISC(w), 0, 0.5);
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
setupwindow(void)
{
  GtkWidget *w, *hbox, *vbox, *table;

  vbox = gtk_vbox_new(FALSE, 0);
  hbox = gtk_hbox_new(FALSE, 0);

  w = gtk_menu_bar_new();
  NgraphApp.Viewer.menu = w;

  w = gtk_ui_manager_get_widget(NgraphUi, "/MenuBar");
  if (w) {
    gtk_box_pack_start(GTK_BOX(vbox), w, FALSE, FALSE, 0);
  }
  read_keymap_file();

  w = get_toolbar(NgraphUi, "/CommandToolBar", NULL);
  CToolbar = create_toolbar_box(vbox, w, GTK_ORIENTATION_HORIZONTAL);

  w = get_toolbar(NgraphUi, "/ToolBox", G_CALLBACK(CmViewerButtonPressed));
  PToolbar = create_toolbar_box(hbox, w, GTK_ORIENTATION_VERTICAL);

  NgraphApp.Viewer.popup = gtk_ui_manager_get_widget(NgraphUi, "/ViewerPopup");
  NgraphApp.Viewer.HScroll = gtk_hscrollbar_new(NULL);
  NgraphApp.Viewer.VScroll = gtk_vscrollbar_new(NULL);
  NgraphApp.Viewer.HRuler = hruler_new();
  NgraphApp.Viewer.VRuler = vruler_new();
  NgraphApp.Viewer.Win = gtk_drawing_area_new();

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
		   create_message_box(&NgraphApp.Message_extra, &NgraphApp.Message_pos),
		   FALSE, FALSE, 0);
  gtk_box_pack_start(GTK_BOX(vbox), NgraphApp.Message, FALSE, FALSE, 0);

  NgraphApp.Message1 = gtk_statusbar_get_context_id(GTK_STATUSBAR(NgraphApp.Message), "Message1");

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
  NgraphApp.FileWin.type = TypeFileWin;

  memset(&NgraphApp.AxisWin, 0, sizeof(NgraphApp.AxisWin));
  NgraphApp.AxisWin.can_focus = TRUE;
  NgraphApp.AxisWin.type = TypeAxisWin;

  memset(&NgraphApp.LegendWin, 0, sizeof(NgraphApp.LegendWin));
  NgraphApp.LegendWin.can_focus = TRUE;
  NgraphApp.LegendWin.type = TypeLegendWin;

  memset(&NgraphApp.MergeWin, 0, sizeof(NgraphApp.MergeWin));
  NgraphApp.MergeWin.can_focus = TRUE;
  NgraphApp.MergeWin.type = TypeMergeWin;

  memset(&NgraphApp.InfoWin, 0, sizeof(NgraphApp.InfoWin));
  NgraphApp.InfoWin.can_focus = FALSE;
  NgraphApp.InfoWin.type = TypeInfoWin;

  memset(&NgraphApp.CoordWin, 0, sizeof(NgraphApp.CoordWin));
  NgraphApp.CoordWin.can_focus = FALSE;
  NgraphApp.CoordWin.type = TypeCoordWin;

  NgraphApp.legend_text_list = NULL;
  NgraphApp.x_math_list = NULL;
  NgraphApp.y_math_list = NULL;
  NgraphApp.func_list = NULL;

  NgraphApp.Interrupt = FALSE;
}

static void
create_sub_windows(void)
{
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
  case MenuIdToggleStatusBar:
    Menulocal.statusb = state;
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
    {&Menulocal.statusb,    "ViewStatusbarAction"},
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
  struct actions {
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

void
window_action_set_active(enum SubWinType type, int state)
{
  char *name;
  GtkAction *action;

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
window_action_toggle(enum SubWinType type)
{
  window_action_set_active(type, -1);
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
      gtk_recent_filter_add_application(filter, AppName);
      gtk_recent_filter_set_name(filter, "NGP file");
      gtk_recent_filter_add_pattern(filter, "*.ngp");

      gtk_recent_action_set_show_numbers(GTK_RECENT_ACTION(action), TRUE);
      gtk_recent_chooser_set_show_tips(GTK_RECENT_CHOOSER(action), TRUE);
      gtk_recent_chooser_set_show_icons(GTK_RECENT_CHOOSER(action), FALSE);
      gtk_recent_chooser_set_local_only(GTK_RECENT_CHOOSER(action), TRUE);
#ifndef WINDOWS
      gtk_recent_chooser_set_show_not_found(GTK_RECENT_CHOOSER(action), FALSE);
#endif
      gtk_recent_chooser_set_sort_type(GTK_RECENT_CHOOSER(action), GTK_RECENT_SORT_MRU);
      gtk_recent_chooser_set_limit(GTK_RECENT_CHOOSER(action), 10);
      g_signal_connect(GTK_RECENT_CHOOSER(action), "item-activated", G_CALLBACK(CmGraphHistory), NULL);

      gtk_recent_chooser_add_filter(GTK_RECENT_CHOOSER(action), filter);
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
      char *file;

      file = g_strdup_printf("%s%c%s", PIXMAPDIR, DIRSEP, entry[i].icon);
      icon = g_icon_new_for_string(file, NULL);
      g_free(file);
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

static void
SaveHistory(void)
{
  struct narray conf;
  char *buf;
  int i, num;
  char **data, data_history[] = "data_history=";

  if (!Menulocal.savehistory)
    return;
  if (!CheckIniFile())
    return;
  arrayinit(&conf, sizeof(char *));

  num = arraynum(Menulocal.datafilelist);
  data = arraydata(Menulocal.datafilelist);
  for (i = 0; i < num; i++) {
    if (data[i]) {
      buf = g_strdup_printf("%s%s", data_history, data[i]);
      if (buf) {
	arrayadd(&conf, &buf);
      }
    }
  }
  replaceconfig("[x11menu]", &conf);

  arraydel2(&conf);
  arrayinit(&conf, sizeof(char *));
  if (arraynum(Menulocal.datafilelist) == 0) {
    buf = g_strdup(data_history);
    if (buf) {
      arrayadd(&conf, &buf);
    }
  }
  removeconfig("[x11menu]", &conf);
  arraydel2(&conf);
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
  create_recent_data_menu();

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
	LoadNgpFile(file, Menulocal.ignorepath, Menulocal.expand, Menulocal.expanddir, FALSE, NULL);
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

  terminated = AppMainLoop();

  set_newobj_cb(NULL);
  set_delobj_cb(NULL);

  gtk_ui_manager_remove_ui(NgraphUi, ui_id);

#ifndef WINDOWS
  set_signal(SIGTERM, 0, SIG_DFL);
  set_signal(SIGINT, 0, SIG_DFL);
#endif	/* WINDOWS */

  SaveHistory();
  save_entry_history();
  menu_save_config(SAVE_CONFIG_TYPE_TOGGLE_VIEW |
		   SAVE_CONFIG_TYPE_OTHERS);

  ViewerWinClose();

  destroy_sub_windows();

  g_free(NgraphApp.FileName);
  NgraphApp.FileName = NULL;

  gtk_widget_destroy(TopLevel);
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

  if (NgraphApp.cursor == NULL || CursorType == type)
    return;

  win = NgraphApp.Viewer.gdk_win;

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
    sub_window_set_geometry((struct SubWin *) &(NgraphApp.InfoWin), TRUE);
  }

  if (Menulocal.coordopen) {
    window_action_set_active(TypeCoordWin, TRUE);
    sub_window_set_geometry((struct SubWin *) &(NgraphApp.CoordWin), TRUE);
  }

  if (Menulocal.mergeopen) {
    window_action_set_active(TypeMergeWin, TRUE);
    sub_window_set_geometry(&(NgraphApp.MergeWin), TRUE);
  }

  if (Menulocal.legendopen) {
    window_action_set_active(TypeLegendWin, TRUE);
    sub_window_set_geometry((struct SubWin *) &(NgraphApp.LegendWin), TRUE);
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

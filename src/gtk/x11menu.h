/*
 * $Id: x11menu.h,v 1.48 2010-03-04 08:30:17 hito Exp $
 *
 * This file is part of "Ngraph for GTK".
 *
 * Copyright (C) 2002,  Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 *
 * "Ngraph for GTK" is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License,  or (at your option) any later version.
 *
 * "Ngraph for GTK" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not,  write to the Free Software
 * Foundation,  Inc.,  59 Temple Place - Suite 330,  Boston,  MA  02111-1307,  USA.
 *
 */

#ifndef GTK_MENU_HEADER
#define GTK_MENU_HEADER

#include "common.h"
#include "gtk_liststore.h"
#include "ogra2cairo.h"

#define N2GTK_RULER_METRIC(v) ((v) / 100.0 * 72.0 / 25.4 * 10)

#define MARK_PIX_SIZE 24

enum MenuID {
  MenuIdGraphNewFrame,
  MenuIdGraphNewSection,
  MenuIdGraphNewCross,
  MenuIdGraphAllClear,
  MenuIdEditRedo,
  MenuIdEditUndo,
  MenuIdEditCut,
  MenuIdEditCopy,
  MenuIdEditPaste,
  MenuIdEditDelete,
  MenuIdEditDuplicate,
  MenuIdEditSelectAll,
  MenuIdAlignLeft,
  MenuIdAlignVCenter,
  MenuIdAlignRight,
  MenuIdAlignTop,
  MenuIdAlignHCenter,
  MenuIdAlignBottom,
  MenuIdEditRotateCW,
  MenuIdEditRotateCCW,
  MenuIdEditFlipHorizontally,
  MenuIdEditFlipVertically,
  MenuIdOutputGRAFile,
  MenuIdOutputPSFile,
  MenuIdOutputEPSFile,
  MenuIdOutputPNGFile,
  MenuIdOutputCairoEMFFile,
  MenuIdOutputEMFFile,
  MenuIdOutputEMFClipboard,
  MenuIdOutputPDFFile,
  MenuIdOutputSVGFile,
  MenuIdToggleSidebar,
  MenuIdToggleStatusbar,
  MenuIdToggleRuler,
  MenuIdToggleScrollbar,
  MenuIdToggleCToolbar,
  MenuIdTogglePToolbar,
  MenuIdToggleCrossGauge,
  MenuIdToggleGridLine,
  MenuIdEditOrderTop,
  MenuIdEditOrderUp,
  MenuIdEditOrderDown,
  MenuIdEditOrderBottom,
};

enum {
  RECENT_TYPE_GRAPH,
  RECENT_TYPE_DATA,
};

enum DrawLockVal {DrawLockNone, DrawLockDraw, DrawLockExpose};

enum PointerType {
  PointB   = 0x000001,
  LegendB  = 0x000002,
  PathB    = 0x000004,
  RectB    = 0x000008,
  ArcB     = 0x000010,
  MarkB    = 0x000020,
  TextB    = 0x000040,
  GaussB   = 0x000080,
  AxisB    = 0x000100,
  TrimB    = 0x000200,
  FrameB   = 0x000400,
  SectionB = 0x000800,
  CrossB   = 0x001000,
  SingleB  = 0x002000,
  DataB    = 0x004000,
  EvalB    = 0x008000,
  ZoomB    = 0x010000,
};

#define POINT_TYPE_POINT (PointB | LegendB | AxisB)
#define POINT_TYPE_DRAW1 (ArcB | RectB | GaussB | FrameB | SectionB | CrossB)
#define POINT_TYPE_DRAW2 (PathB | SingleB)
#define POINT_TYPE_DRAW3 (TextB | MarkB)
#define POINT_TYPE_DRAW_ALL (POINT_TYPE_DRAW1 | POINT_TYPE_DRAW2 | POINT_TYPE_DRAW3)
#define POINT_TYPE_TRIM  (TrimB | DataB | EvalB)

enum _n_line_type {
  N_LINE_TYPE_SOLID,
  N_LINE_TYPE_DOT,
};

enum MouseMode {
  MOUSENONE,
  MOUSEPOINT,
  MOUSEDRAG,
  MOUSEZOOM1,
  MOUSEZOOM2,
  MOUSEZOOM3,
  MOUSEZOOM4,
  MOUSECHANGE,
  MOUSESCROLLE,
};

enum PointerMode {
  PointerModeBoth,
  PointerModeLegend,
  PointerModeAxis,
  PointerModeData,
  PointerModeNum,
  PointerModeDefault,
  PointerModeFocus,
  PointerModeFocusAxis,
  PointerModeFocusLegend,
};

enum pop_up_menu_item_type {
  POP_UP_MENU_ITEM_TYPE_NORMAL,
  POP_UP_MENU_ITEM_TYPE_CHECK,
  POP_UP_MENU_ITEM_TYPE_MENU,
  POP_UP_MENU_ITEM_TYPE_SEPARATOR,
  POP_UP_MENU_ITEM_TYPE_RECENT_GRAPH,
  POP_UP_MENU_ITEM_TYPE_RECENT_DATA,
  POP_UP_MENU_ITEM_TYPE_END,
};

struct ScrollPrm {
  double x, y;
};

struct ZoomPrm {
  int x, y, dpi;
  int focused;
  double scale;
};

struct DragPrm {
  int x, y;
  int active;
  double vx, vy;
};

struct DecelerationPrm
{
  guint id;
  guint64 start;
};

#define VIEWER_POPUP_ITEM_NUM 14
struct Viewer
{
  GtkWidget *Win;
  GtkWidget *VScroll, *HScroll, *popup, *VRuler, *HRuler, *side_pane1, *side_pane2, *side_pane3, *main_pane;
  int ShowFrame, ShowLine, ShowRect;
  int Capture, MoveData, KeyMask;
  enum MouseMode MouseMode;
  enum PointerType Mode;
  struct narray *focusobj, *points;
  int FrameOfsX, FrameOfsY;
  int MouseX1, MouseY1, MouseX2, MouseY2, MouseDX, MouseDY;
  int RefX1, RefY1, RefX2, RefY2, ChangePoint;
  int LineX, LineY, Angle;
  int cx, cy;
  int ignoreredraw;
  double vscroll, hscroll, ZoomX, ZoomY, CrossX, CrossY;
  struct ZoomPrm zoom_prm;
  struct DragPrm drag_prm;
  struct DecelerationPrm deceleration_prm;
  struct ScrollPrm scroll_prm;
};

enum SubWinType {
  TypeFileWin,
  TypeAxisWin,
  TypePathWin,
  TypeRectWin,
  TypeArcWin,
  TypeMarkWin,
  TypeTextWin,
  TypeMergeWin,
  TypeParameterWin,
  TypeCoordWin,
  TypeInfoWin,
};

struct SubWin;

struct obj_list_data
{
  GtkWidget *popup, **popup_item;
  GtkWidget *text;
  int select, can_focus;
  void (* update)(struct obj_list_data *data, int, int);
  void (* delete)(struct obj_list_data *data, int);
  void (* setup_dialog)(struct obj_list_data *data, int id, int user_data);
  int (* undo_save)(int type);
  void *dialog;
  gboolean (* ev_key) (GtkWidget *, guint, GdkModifierType, gpointer);
  struct objlist *obj;
  struct SubWin *parent;
  n_list_store *list;
  int list_col_num;
};

typedef void (* sub_window_state_func) (struct SubWin *d, int state);

struct SubWin
{
  enum SubWinType type;
  GtkWidget *Win;
  union {
    struct obj_list_data *data;
    GtkWidget *text;
  } data;
};

#define MENU_HISTORY_NUM 10

struct NgraphApp
{
  char *FileName;
  GtkWidget *Message, *Message_pos, *Message_extra;
  gint Message1;
  GtkRecentManager *recent_manager;
  GtkEntryCompletion *legend_text_list, *x_math_list, *y_math_list, *func_list, *fit_list;
  cairo_surface_t *markpix[MARK_TYPE_NUM];
  GdkCursor **cursor;
  struct Viewer Viewer;
  struct SubWin FileWin;
  struct SubWin AxisWin;
  struct SubWin PathWin;
  struct SubWin RectWin;
  struct SubWin ArcWin;
  struct SubWin MarkWin;
  struct SubWin TextWin;
  struct SubWin MergeWin;
  struct SubWin ParameterWin;
  struct SubWin CoordWin;
  struct SubWin InfoWin;
};


extern int Menulock, DnDLock;
extern struct NgraphApp NgraphApp;
extern GtkWidget *TopLevel;
extern GdkColor white, gray;
extern GtkAccelGroup *AccelGroup;
extern GtkApplication *GtkApp;

enum MENU_UNDO_TYPE {
  UNDO_TYPE_EDIT,
  UNDO_TYPE_MOVE,
  UNDO_TYPE_ROTATE,
  UNDO_TYPE_FLIP,
  UNDO_TYPE_DELETE,
  UNDO_TYPE_CREATE,
  UNDO_TYPE_ALIGN,
  UNDO_TYPE_ORDER,
  UNDO_TYPE_COPY,
  UNDO_TYPE_SHLL,
  UNDO_TYPE_ADDIN,
  UNDO_TYPE_CLEAR_SCALE,
  UNDO_TYPE_UNDO_SCALE,
  UNDO_TYPE_OPEN_FILE,
  UNDO_TYPE_ADD_RANGE,
  UNDO_TYPE_PASTE,
  UNDO_TYPE_ZOOM,
  UNDO_TYPE_AUTOSCALE,
  UNDO_TYPE_TRIMMING,
  UNDO_TYPE_DUMMY,
  UNDO_TYPE_NUM,
};

struct EventLoopInfo {
  int type;
  guint32 time;
};

enum RerawFlag {
  DRAW_NONE = 0,
  DRAW_REDRAW = 1,
  DRAW_NOTIFY = 2,
  DRAW_AXIS_ONLY = 4,
};

enum TOOLBOX_MODE {
  TOOLBOX_MODE_TOOLBAR,
  TOOLBOX_MODE_SETTING_PANEL,
};

void set_toolbox_mode(enum TOOLBOX_MODE mode);
enum TOOLBOX_MODE get_toolbox_mode(void);

int application(char *file);

void set_current_window(GtkWidget *w);
GtkWidget *get_current_window(void);
GtkWidget *create_recent_menu(int type);
void UpdateAll(char **objects);
void UpdateAll2(char **objects, int redraw);
void ChangePage(void);
void NSetCursor(unsigned int type);
unsigned int NGetCursor(void);
void reset_event(void);
void SetStatusBar(const char *mes);
void ResetStatusBar(void);
int PutStderr(const char *s);
int PutStdout(const char *s);
void DisplayDialog(const char *str);
int ChkInterrupt(void);
int InputYN(const char *mes);
void QuitGUI(void);
void menu_lock(int lock);
void set_draw_lock(int lock);
int find_gra2gdk_inst(struct objlist **o, N_VALUE **i, struct objlist **ro, int *routput, struct gra2cairo_local **rlocal);
void set_axis_undo_button_sensitivity(int state);
void set_modified_state(int state);
void set_focus_insensitive(const struct Viewer *d);
void set_focus_sensitivity(const struct Viewer *d);
void create_addin_menu(void);
void create_recent_data_menu(void);
void set_pointer_mode(int id);
void set_toggle_action_widget_state(int id, int state);
void script_exec(GtkWidget *w, gpointer client_data);
int toggle_view(int type, int state);
void CmToggleSingleWindowMode(GtkCheckMenuItem *action, gpointer client_data);
void CmReloadWindowConfig(void *w, gpointer user_data);
void show_recent_dialog(int type);
int menu_save_undo(enum MENU_UNDO_TYPE type, char **obj);
int menu_save_undo_single(enum MENU_UNDO_TYPE type, char *obj);
void menu_delete_undo(int id);
void menu_clear_undo(void);
void menu_undo_internal(int id);
void menu_undo(void);
void menu_redo(void);
int get_graph_modified(void);
void set_graph_modified(void);
void set_graph_modified_gra(void);
void reset_graph_modified(void);
void draw_notify(int notify);

#endif

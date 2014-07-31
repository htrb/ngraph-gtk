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
  MenuIdEditCut,
  MenuIdEditCopy,
  MenuIdEditPaste,
  MenuIdEditDelete,
  MenuIdEditDuplicate,
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

enum pop_up_menu_item_type {
  POP_UP_MENU_ITEM_TYPE_NORMAL,
  POP_UP_MENU_ITEM_TYPE_CHECK,
  POP_UP_MENU_ITEM_TYPE_MENU,
  POP_UP_MENU_ITEM_TYPE_SEPARATOR,
  POP_UP_MENU_ITEM_TYPE_RECENT_GRAPH,
  POP_UP_MENU_ITEM_TYPE_RECENT_DATA,
  POP_UP_MENU_ITEM_TYPE_END,
};

#define VIEWER_POPUP_ITEM_NUM 14
struct Viewer
{
  GtkWidget *Win;
  GtkWidget *menu, *VScroll, *HScroll, *popup, *VRuler, *HRuler, *side_pane1, *side_pane2, *side_pane3, *main_pane;
  int ShowFrame, ShowLine, ShowRect;
  int Capture, MoveData, KeyMask;
  enum MouseMode MouseMode;
  enum PointerType Mode;
  struct narray *focusobj, *points;
  int FrameOfsX, FrameOfsY;
  int MouseX1, MouseY1, MouseX2, MouseY2, MouseDX, MouseDY;
  int RefX1, RefY1, RefX2, RefY2, ChangePoint;
  int LineX, LineY, CrossX, CrossY, Angle;
  int allclear;
  int cx, cy;
  int ignoreredraw;
  double vscroll, hscroll, Zoom;
};

enum subwin_state {
  SUBWIN_STATE_SHOW,
  SUBWIN_STATE_HIDE,
  SUBWIN_STATE_TOGGLE,
};

enum SubWinType {
  TypeFileWin,
  TypeAxisWin,
  TypeLegendWin,
  TypeMergeWin,
  TypeCoordWin,
  TypeInfoWin,
};

struct SubWin;

struct obj_list_data
{
  GtkWidget *popup, **popup_item;
  GtkWidget *text;
  int select, can_focus;
  void (* update)(struct obj_list_data *data, int);
  void (* delete)(struct obj_list_data *data, int);
  void (* setup_dialog)(struct obj_list_data *data, int id, int user_data);
  void *dialog;
  gboolean (* ev_key) (GtkWidget *, GdkEvent *, gpointer);
  struct objlist *obj;
  struct SubWin *parent;
  n_list_store *list;
  int list_col_num;
  struct obj_list_data *next;
};

struct SubWin;

typedef void (* sub_window_state_func) (struct SubWin *d, int state);

struct SubWin
{
  enum SubWinType type;
  GtkWidget *Win;
  GdkWindowState window_state;
  int visible, action_widget_id;
  union {
    struct obj_list_data *data;
    GtkWidget *text;
  } data;
  sub_window_state_func state_func;
};

#define MENU_HISTORY_NUM 10

struct NgraphApp
{
  int Interrupt;
  char *FileName;
  GtkWidget *Message, *Message_pos, *Message_extra;
  gint Message1;
  GtkWidget *ghistory[MENU_HISTORY_NUM], *fhistory[MENU_HISTORY_NUM];
  GtkEntryCompletion *legend_text_list, *x_math_list, *y_math_list, *func_list, *fit_list;
  GtkRadioAction *viewb;
#if GTK_CHECK_VERSION(3, 0, 0)
  cairo_surface_t *markpix[MARK_TYPE_NUM];
#else
  GdkPixmap *markpix[MARK_TYPE_NUM];
#endif
  GdkCursor **cursor;
  struct Viewer Viewer;
  struct SubWin FileWin;
  struct SubWin AxisWin;
  struct SubWin LegendWin;
  struct SubWin MergeWin;
  struct SubWin CoordWin;
  struct SubWin InfoWin;
};


extern int Menulock, DnDLock;
extern struct NgraphApp NgraphApp;
extern GtkWidget *TopLevel;
extern GdkColor white, gray;
extern GtkAccelGroup *AccelGroup;

int application(char *file);

void set_current_window(GtkWidget *w);
GtkWidget *get_current_window(void);
GtkWidget *create_recent_menu(int type);
void UpdateAll(void);
void UpdateAll2(void);
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
void set_subwindow_state(enum SubWinType id, enum subwin_state state);

#endif

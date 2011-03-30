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

#include "gtk_ruler.h"
#include "x11dialg.h"
#include "ogra2cairo.h"

#define N2GTK_RULER_METRIC(v) ((v) / 100.0 * 72.0 / 25.4 * 10)

enum MenuID {
  MenuIdGraphLoad,
  MenuIdGraphSave,
  MenuIdGraphOverWrite,
  MenuIdGraphPage,
  MenuIdGraphSwitch,
  MenuIdOutputDriver,
  MenuIdGraphDirectory,
  MenuIdGraphShell,
  MenuIdGraphQuit,
  MenuIdGraphNewFrame,
  MenuIdGraphNewSection,
  MenuIdGraphNewCross,
  MenuIdGraphAllClear,
  MenuIdEditCut,
  MenuIdEditCopy,
  MenuIdEditPaste,
  MenuIdEditDelete,
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
  MenuIdFileNew,
  MenuIdFileOpen,
  MenuIdFileUpdate,
  MenuIdFileClose,
  MenuIdFileEdit,
  MenuIdFileMath,
  MenuIdAxisUpdate,
  MenuIdAxisDel,
  MenuIdAxisZoom,
  MenuIdAxisClear,
  MenuIdAxisUndo,
  MenuIdAxisNewFrame,
  MenuIdAxisNewSection,
  MenuIdAxisNewCross,
  MenuIdAxisNewSingle,
  MenuIdAxisGridNew,
  MenuIdAxisGridUpdate,
  MenuIdAxisGridDel,
  MenuIdLegendUpdate,
  MenuIdLegendDel,
  MenuIdMergeOpen,
  MenuIdMergeUpdate,
  MenuIdMergeClose,
  MenuIdViewerDraw,
  MenuIdViewerClear,
  MenuIdOutputViewer,
  MenuIdOutputGRAFile,
  MenuIdOutputPSFile,
  MenuIdOutputEPSFile,
  MenuIdOutputPNGFile,
  MenuIdOutputEMFFile,
  MenuIdOutputPDFFile,
  MenuIdOutputSVGFile,
  MenuIdPrintDataFile,
  MenuIdOptionViewer,
  MenuIdOptionExtViewer,
  MenuIdOptionPrefFont,
  MenuIdOptionPrefDriver,
  MenuIdOptionScript,
  MenuIdOptionMisc,
  MenuIdOptionSaveDefault,
  MenuIdOptionSaveNgp,
  MenuIdOptionFileDef,
  MenuIdOptionTextDef,
  MenuIdToggleStatusBar,
  MenuIdToggleRuler,
  MenuIdToggleScrollbar,
  MenuIdToggleCToolbar,
  MenuIdTogglePToolbar,
  MenuIdToggleCrossGauge,
  MenuIdHelpAbout,
  MenuIdHelpHelp,
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

enum MouseMode {
  MOUSENONE,
  MOUSEPOINT,
  MOUSEDRAG,
  MOUSEZOOM1,
  MOUSEZOOM2,
  MOUSEZOOM3,
  MOUSEZOOM4,
  MOUSECHANGE,
};

enum pop_up_menu_item_type {
  POP_UP_MENU_ITEM_TYPE_NORMAL,
  POP_UP_MENU_ITEM_TYPE_CHECK,
  POP_UP_MENU_ITEM_TYPE_MENU,
  POP_UP_MENU_ITEM_TYPE_SEPARATOR,
};

#define VIEWER_POPUP_ITEM_NUM 14
struct Viewer
{
  GtkWidget *Win;
  GdkWindow *gdk_win;
  GtkWidget *menu, *VScroll, *HScroll, *popup;
  Nruler *VRuler, *HRuler;
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
  double vscroll, hscroll, vupper, hupper, Zoom;
};

enum SubWinType {
  TypeFileWin,
  TypeAxisWin,
  TypeLegendWin,
  TypeMergeWin,
  TypeInfoWin,
  TypeCoordWin,
};

#define SUBWIN_PROTOTYPE enum SubWinType type;				\
  GtkWidget *Win, *popup, **popup_item;					\
  GdkWindowState window_state;						\
  GObject *text;							\
  int select, num, can_focus;						\
  void (* update)(int);							\
  void (* delete)(int);							\
  void (* setup_dialog)(void *data, struct objlist *obj, int id, int sub_id); \
  void *dialog;								\
  gboolean (* ev_key) (GtkWidget *, GdkEvent *, gpointer);		\
  gboolean (* ev_button) (GtkWidget *, GdkEventButton *, gpointer);	\


struct SubWin
{
  SUBWIN_PROTOTYPE;
  struct objlist *obj;
};


#define LEGENDNUM 5

struct LegendWin
{
  SUBWIN_PROTOTYPE;
  struct objlist *obj[LEGENDNUM];
  /* Private member */
  int legend_type;
  int legend[LEGENDNUM];
};

#define MENU_HISTORY_NUM 10

struct NgraphApp
{
  int Interrupt;
  char *FileName;
  GtkWidget *Message, *Message_pos, *Message_extra;
  gint Message1;
  GtkWidget *ghistory[MENU_HISTORY_NUM], *fhistory[MENU_HISTORY_NUM];
  GtkEntryCompletion *legend_text_list, *x_math_list, *y_math_list, *func_list;
  GtkRadioAction *viewb;
  GdkPixmap *markpix[MARK_TYPE_NUM];
  GdkCursor **cursor;
  struct Viewer Viewer;
  struct SubWin FileWin;
  struct SubWin AxisWin;
  struct LegendWin LegendWin;
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
void AxisWinUpdate(int clear);
void UpdateAll(void);
void UpdateAll2(void);
void ChangePage(void);
void NSetCursor(unsigned int type);
unsigned int NGetCursor(void);
void SetPoint(struct Viewer *d, int x, int y);
void SetZoom(double zm);
void ResetZoom(void);
void reset_event(void);
void WaitForMap(void);
void GetWMFrame(void);
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
int check_focused_obj_type(const struct Viewer *d, int *type);
void set_axis_undo_button_sensitivity(int state);
void set_modified_state(int state);
void set_focus_sensitivity(const struct Viewer *d);
void window_action_set_active(enum SubWinType type, int state);
void window_action_toggle(enum SubWinType type);
void show_ui_definition(void);
int activate_action(const char *name);
void create_addin_menu(void);
void create_recent_data_menu(void);

#endif

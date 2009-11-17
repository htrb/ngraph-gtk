/* 
 * $Id: x11menu.h,v 1.46 2009/11/17 06:41:50 hito Exp $
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
  MenuIdAlignHBottom,
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
  LineB    = 0x000004,
  CurveB   = 0x000008,
  PolyB    = 0x000010,
  RectB    = 0x000020,
  ArcB     = 0x000040,
  MarkB    = 0x000080,
  TextB    = 0x000100,
  GaussB   = 0x000200,
  AxisB    = 0x000400,
  TrimB    = 0x000800,
  FrameB   = 0x001000,
  SectionB = 0x002000,
  CrossB   = 0x004000,
  SingleB  = 0x008000,
  DataB    = 0x010000,
  EvalB    = 0x020000,
  ZoomB    = 0x040000,
};

#define POINT_TYPE_POINT (PointB | LegendB | AxisB)
#define POINT_TYPE_DRAW1 (ArcB | RectB | GaussB | FrameB | SectionB | CrossB)
#define POINT_TYPE_DRAW2 (CurveB | LineB | PolyB | SingleB)
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
  GdkWindow *win;
  GtkWidget *menu, *VScroll, *HScroll, *VRuler, *HRuler,
    *PToolbar, *CToolbar, *popup, *popup_item[VIEWER_POPUP_ITEM_NUM];
  int ShowFrame, ShowLine, ShowRect;
  int Capture, MoveData;
  enum MouseMode MouseMode;
  enum PointerType Mode;
  struct narray *focusobj, *points;
  int FrameOfsX, FrameOfsY;
  int MouseX1, MouseY1, MouseX2, MouseY2, MouseDX, MouseDY;
  int RefX1, RefY1, RefX2, RefY2, ChangePoint;
  int LineX, LineY, CrossX, CrossY, Angle;
  int allclear;
  int cx, cy, width, height;
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

#define SUBWIN_PROTOTYPE enum SubWinType type;\
  GtkWidget *Win, *swin, *popup, **popup_item;\
  GdkWindowState window_state;\
  GObject *text;\
  int select, num, can_focus;\
  void (* update)(int);\
  void (* setup_dialog)(void *data, struct objlist *obj, int id, int sub_id);\
  void *dialog;\
  gboolean (* ev_key) (GtkWidget *, GdkEvent *, gpointer);\
  gboolean (* ev_button) (GtkWidget *, GdkEventButton *, gpointer);\


struct SubWin
{
  SUBWIN_PROTOTYPE;
  struct objlist *obj;
};


#define LEGENDNUM 7

struct LegendWin
{
  SUBWIN_PROTOTYPE;
  struct objlist *obj[LEGENDNUM];
  /* Private member */
  int legend_type;
  int legend[LEGENDNUM];
};

struct InfoWin
{
  SUBWIN_PROTOTYPE;
  struct objlist *obj;
  /* Private member */
  char *str;
};

#define MENU_HISTORY_NUM 10

struct NgraphApp
{
  int Interrupt;
  char *FileName;
  GtkWidget *Message;
  guint Message1, Message2;
  GtkWidget *ghistory[MENU_HISTORY_NUM], *fhistory[MENU_HISTORY_NUM];
  GtkEntryCompletion *legend_text_list, *x_math_list, *y_math_list, *func_list;
  GtkToolItem *viewb[20];
  GdkPixmap *markpix[MARK_TYPE_NUM];
  GList *iconpix;
  GdkCursor **cursor;
  struct Viewer Viewer;
  struct SubWin FileWin;
  struct SubWin AxisWin;
  struct LegendWin LegendWin;
  struct SubWin MergeWin;
  struct InfoWin CoordWin;
  struct InfoWin InfoWin;
};


extern int Menulock;
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
void SetCursor(unsigned int type);
unsigned int GetCursor(void);
void SetPoint(struct Viewer *d, int x, int y);
void SetZoom(double zm);
void ResetZoom(void);
void ResetEvent(void);
void WaitForMap(void);
void GetWMFrame(void);
void SetStatusBar(char *mes);
void SetStatusBarXm(gchar * s);
void ResetStatusBar(void);
int PutStderr(char *s);
int PutStdout(char *s);
void DisplayDialog(char *str);
int ChkInterrupt(void);
int InputYN(char *mes);
void QuitGUI(void);
void menu_lock(int lock);
void set_draw_lock(int lock);
int find_gra2gdk_inst(char **name, struct objlist **o, char **i, struct objlist **ro, int *routput, struct gra2cairo_local **rlocal);
void update_addin_menu(void);
int check_focused_obj_type(struct Viewer *d, int *type);
void set_axis_undo_button_sensitivity(int state);

#endif

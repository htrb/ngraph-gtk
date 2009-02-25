/* 
 * $Id: x11menu.h,v 1.27 2009/02/25 09:40:58 hito Exp $
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

#define N2GTK_RULER_METRIC(v) (v / 100.0 * 72.0 / 25.4 * 10)

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
  MenuIdFileNew,
  MenuIdFileOpen,
  MenuIdFileUpdate,
  MenuIdFileClose,
  MenuIdFileEdit,
  MenuIdAxisUpdate,
  MenuIdAxisDel,
  MenuIdAxisZoom,
  MenuIdAxisClear,
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
#define POINT_TYPE_DRAW_ALL (POINT_TYPE_DRAW1 | POINT_TYPE_DRAW2)
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

#define VIEWER_POPUP_ITEM_NUM 9
struct Viewer
{
  GtkWidget *Win;
  GdkWindow *win;
  GtkWidget *menu, *VScroll, *HScroll, *VRuler, *HRuler, *popup, *popup_item[VIEWER_POPUP_ITEM_NUM];
  int ShowFrame, ShowLine, ShowRect, ShowCross;
  int Capture, MoveData;
  enum MouseMode MouseMode;
  enum PointerType Mode;
  struct narray *focusobj, *points;
  int FrameOfsX, FrameOfsY;
  int MouseX1, MouseY1, MouseX2, MouseY2, MouseDX, MouseDY;
  int RefX1, RefY1, RefX2, RefY2, ChangePoint;
  int LineX, LineY, CrossX, CrossY;
  int allclear;
  int cx, cy, width, height;
  int ignoreredraw;
  double vscroll, hscroll, vupper, hupper;
};

enum SubWinType {
  TypeFileWin,
  TypeAxisWin,
  TypeLegendWin,
  TypeMergeWin,
  TypeInfoWin,
  TypeCoordWin,
};

struct SubWin
{
  enum SubWinType type;
  GtkWidget *Win, *swin, *popup, **popup_item;
  GdkWindowState window_state;
  GObject *text;
  int select, num, can_focus;
  void (* update)(int);
  void (* setup_dialog)(void *data, struct objlist *obj, int id, int sub_id);
  void *dialog;
  struct objlist *obj;
  gboolean (* ev_key) (GtkWidget *, GdkEvent *, gpointer);
  gboolean (* ev_button) (GtkWidget *, GdkEventButton *, gpointer);
};


#define LEGENDNUM 7

struct LegendWin
{
  enum SubWinType type;
  GtkWidget *Win, *swin, *popup, **popup_item;
  GdkWindowState window_state;
  GObject *text;
  int select, num, can_focus;
  void (* update)(int);
  void (* setup_dialog)(void *data, struct objlist *obj, int id, int sub_id);
  void *dialog;
  struct objlist *obj[LEGENDNUM];
  gboolean (* ev_key) (GtkWidget *, GdkEvent *, gpointer);
  gboolean (* ev_button) (GtkWidget *, GdkEventButton *, gpointer);
  /* Private member */
  int legend_type;
  int legend[LEGENDNUM];
};

struct InfoWin
{
  enum SubWinType type;
  GtkWidget *Win, *swin, *popup, **popup_item;
  GdkWindowState window_state;
  GObject *text;
  int select, num, can_focus;
  void (* update)(int);
  void (* setup_dialog)(void *data, struct objlist *obj, int id, int sub_id);
  void *dialog;
  struct objlist *obj;
  gboolean (* ev_key) (GtkWidget *, GdkEvent *, gpointer);
  gboolean (* ev_button) (GtkWidget *, GdkEventButton *, gpointer);
  /* Private member */
  char *str;
};

struct CoordWin
{
  enum SubWinType type;
  GtkWidget *Win, *swin, *popup, **popup_item;
  GdkWindowState window_state;
  GObject *text;
  int select, num, can_focus;
  void (* update)(int);
  void (* setup_dialog)(void *data, struct objlist *obj, int id, int sub_id);
  void *dialog;
  struct objlist *obj;
  gboolean (* ev_key) (GtkWidget *, GdkEvent *, gpointer);
  gboolean (* ev_button) (GtkWidget *, GdkEventButton *, gpointer);
  /* Private member */
  char *str;
};

#define MENU_HISTORY_NUM 10

struct NgraphApp
{
  int Interrupt;
  char *FileName;
  GtkWidget *Message;
  guint Message1, Message2, Message3;
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
  struct CoordWin CoordWin;
  struct InfoWin InfoWin;
};


extern int Menulock;
extern struct NgraphApp NgraphApp;
extern int FWidth, FHeight;
extern GtkWidget *TopLevel;
extern struct narray ChildList;
extern GdkDisplay *Disp;
extern GdkColor black, white, gray;
extern GtkAccelGroup *AccelGroup;

void application(char *file);

void AxisWinUpdate(int clear);
void UpdateAll();
void UpdateAll2();
void ChangePage();
void SetCaption(char *file);
void SetCursor(unsigned int type);
unsigned int GetCursor(void);
void SetPoint(int x, int y);
void SetZoom(double zm);
void ResetZoom();
void ResetEvent();
void WaitForMap();
void GetWMFrame();
void SetStatusBar(char *mes);
void SetStatusBarXm(gchar * s);
void ResetStatusBar();
int PutStderr(char *s);
int PutStdout(char *s);
void DisplayStatus(char *str);
void DisplayDialog(char *str);
int ChkInterrupt();
int InputYN(char *mes);
void QuitGUI(void);
void menu_lock(int lock);
void set_draw_lock(int lock);
int find_gra2gdk_inst(char **name, struct objlist **o, char **i, struct objlist **ro, int *routput, struct gra2cairo_local **rlocal);
void update_addin_menu(void);
void set_widget_visibility(void);

#endif

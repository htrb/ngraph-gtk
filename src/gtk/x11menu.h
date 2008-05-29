/* 
 * $Id: x11menu.h,v 1.2 2008/05/29 10:42:48 hito Exp $
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

enum MenuID {
  MenuIdGraphLoad,
  MenuIdGraphSave,
  MenuIdGraphOverWrite,
  MenuIdGraphPage,
  MenuIdGraphSwitch,
  MenuIdOutputDriver,
  MenuIdGraphDirectory,
  MenuIdScriptExec,
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
  MenuIdPrintGRAFile,
  MenuIdPrintDataFile,
  MenuIdOptionViewer,
  MenuIdOptionExtViewer,
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

#define VIEWER_POPUP_ITEM_NUM 6
struct Viewer
{
  GtkWidget *Win;
  GdkWindow *win;
  GtkWidget *VScroll, *HScroll, *popup, *popup_item[VIEWER_POPUP_ITEM_NUM];
  int ShowFrame, ShowLine, ShowRect, ShowCross;
  int Mode, Capture, MoveData, MouseMode;
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
  int select, num;
  void (* update)(int);
  void (* setup_dialog)(struct DialogType *data, struct objlist *obj, int id, int sub_id);
  struct DialogType *dialog;
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
  int select, num;
  void (* update)(int);
  void (* setup_dialog)(struct DialogType *data, struct objlist *obj, int id, int sub_id);
  struct DialogType *dialog;
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
  int select, num;
  void (* update)(int);
  void (* setup_dialog)(struct DialogType *data, struct objlist *obj, int id, int sub_id);
  struct DialogType *dialog;
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
  int select, num;
  void (* update)(int);
  void (* setup_dialog)(struct DialogType *data, struct objlist *obj, int id, int sub_id);
  struct DialogType *dialog;
  struct objlist *obj;
  gboolean (* ev_key) (GtkWidget *, GdkEvent *, gpointer);
  gboolean (* ev_button) (GtkWidget *, GdkEventButton *, gpointer);
  /* Private member */
  char *str;
};

union SubWindow
{
  struct SubWin subwin;
  struct LegendWin legend;
  struct InfoWin info;
  struct CoordWin coord;
};


#define MARK_TYPE_NUM 90
#define MENU_HISTORY_NUM 10

enum PointerType {
  PointB,
  LegendB,
  LineB,
  CurveB,
  PolyB,
  RectB,
  ArcB,
  MarkB,
  TextB,
  GaussB,
  AxisB,
  TrimB,
  FrameB,
  SectionB,
  CrossB,
  SingleB,
  DataB,
  EvalB,
  ZoomB,
};

struct NgraphApp
{
  int Interrupt;
  int Changed;
  char *FileName;
  GtkWidget *Message;
  guint Message1, Message2, Message3;
  GtkWidget *ghistory[MENU_HISTORY_NUM], *fhistory[MENU_HISTORY_NUM];
  GtkEntryCompletion *legend_text_list, *x_math_list, *y_math_list, *func_list;
  GtkToolItem *interrupt, *viewb[19];
  GdkPixmap *markpix[MARK_TYPE_NUM];
  GList *iconpix;
  GdkCursor *cursor[11];
  struct Viewer Viewer;
  struct SubWin FileWin;
  struct SubWin AxisWin;
  struct LegendWin LegendWin;
  struct SubWin MergeWin;
  struct CoordWin CoordWin;
  struct InfoWin InfoWin;
  //  union SubWindow FileWin, AxisWin, LegendWin, MergeWin, CoordWin, InfoWin;
};

extern struct NgraphApp NgraphApp;
extern int FWidth, FHeight;
extern GtkWidget *TopLevel;
extern struct narray ChildList;
extern GdkDisplay *Disp;
extern GdkColor black, white, gray, red, blue;

void application(char *file);

void AxisWinUpdate(int clear);
void UpdateAll();
void UpdateAll2();
void ChangePage();
void SetCaption(char *file);
void SetCursor(unsigned int type);
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
GdkPixbuf *create_pixbuf_from_xpm(GtkWidget *win, char **xpm);
void QuitGUI(void);

#endif

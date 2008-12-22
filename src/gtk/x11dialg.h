/* 
 * $Id: x11dialg.h,v 1.19 2008/12/22 06:30:54 hito Exp $
 * 
 * This file is part of "Ngraph for X11".
 * 
 * Copyright (C) 2002, Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 * 
 * "Ngraph for X11" is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * "Ngraph for X11" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 */

#ifndef GTK_DIALOG_HEADER
#define GTK_DIALOG_HEADER

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "object.h"

struct line_style {
  char *name, *list;
  int num;
};

extern struct line_style FwLineStyle[];
extern char *FwNumStyle[];
extern int FwNumStyleNum;
extern gint8 Dashes[];
extern int DashesNum;

#define N_RESPONSE_ALL 1

void initdialog();
int CopyClick(GtkWidget *parent, struct objlist *obj, int Id,
	      char *(*callback) (struct objlist *, int));
int SetObjFieldFromWidget(GtkWidget *w, struct objlist *Obj, int Id, char *field);
void SetWidgetFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field);
int SetObjFieldFromText(GtkWidget *w, struct objlist *Obj, int Id, char *field);
void SetTextFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field);
int SetObjPointsFromText(GtkWidget *w, struct objlist *Obj, int Id, char *field);
void SetTextFromObjPoints(GtkWidget *w, struct objlist *Obj, int Id, char *field);
int SetObjFieldFromSpin(GtkWidget *w, struct objlist *Obj, int Id, char *field);
void SetSpinFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field);
int SetObjFieldFromToggle(GtkWidget *w, struct objlist *Obj, int Id, char *field);
void SetToggleFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field);
int SetObjFieldFromStyle(GtkWidget *w, struct objlist *Obj, int Id, char *field);
void SetStyleFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field);
int SetObjFieldFromList(GtkWidget *w, struct objlist *Obj, int Id, char *field);
void SetListFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field);
int SetObjAxisFieldFromWidget(GtkWidget *w, struct objlist *obj, int id, char *field);
void SetComboList(GtkWidget *w, char **list, int num);
void SetComboList2(GtkWidget *w, char **list, int num);
GtkWidget *GetComboBoxList(GtkWidget *w);
GtkWidget *GetComboBoxText(GtkWidget *w);
void SetComboBoxVisibleItemCount(GtkWidget *w, int count);
int get_radio_index(GSList *top);
void SetFontListFromObj(GtkWidget *w, struct objlist *obj, int id, char *name, int jfont);
void SetObjFieldFromFontList(GtkWidget *w, struct objlist *obj, int id, char *name, int jfont);
void set_color(GtkWidget *w, struct objlist *obj, int id, char *prefix);
void set_color2(GtkWidget *w, struct objlist *obj, int id);
int putobj_color(GtkWidget *w, struct objlist *obj, int id, char *prefix);
int putobj_color2(GtkWidget *w, struct objlist *obj, int id);

struct DialogType
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
};

struct MarkDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *toggle[90];
  int Type, cb_respond;
};
void MarkDialog(struct MarkDialog *data, int type);

struct FileDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *file, *load_settings, *fit, *fitid, *xcol, *xaxis, *ycol, *yaxis,
    *type, *mark_btn, *curve, *col1, *col2, *clip, *style, *size, *miter, *join,
    *comment_box, *fit_box, *button_box, *width;
  GtkTextBuffer *comment;
  struct objlist *Obj;
  int Id;
  struct MarkDialog mark;
  int R, G, B, R2, G2, B2;
};

void FileDialog(void *data, struct objlist *obj, int id, int candel);

struct EvalDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *list;
  struct narray *sel;
  struct objlist *Obj;
  int Num;
};
void EvalDialog(struct EvalDialog *data,
		struct objlist *obj, int num, struct narray *iarray);

#define MATH_FNC_NUM 5
struct MathDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *list, *func[MATH_FNC_NUM];
  struct objlist *Obj;
  int Mode;
};
void MathDialog(struct MathDialog *data, struct objlist *obj);

struct MathTextDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *list, *label;
  char *Text, *math;
  int Mode;
};
void MathTextDialog(struct MathTextDialog *data, char *text, int mode);

#define FIT_PARM_NUM 10

struct FitDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *type, *through_point, *x, *y, *dim, *weight,
    *min, *max, *div, *interpolation, *formula, *converge,
    *derivatives, *p[FIT_PARM_NUM], *d[FIT_PARM_NUM];
  struct objlist *Obj;
  int Id;
  int Lastid;
};
void FitDialog(struct FitDialog *data, struct objlist *obj, int id);

struct FitLoadDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *list;
  struct objlist *Obj;
  int Sid;
  int sel;
};
void FitLoadDialog(struct FitLoadDialog *data, struct objlist *obj, int sid);

struct FitSaveDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *profile;
  struct objlist *Obj;
  int Sid;
  char *Profile;
};
void FitSaveDialog(struct FitSaveDialog *data, struct objlist *obj, int sid);

struct FileMoveDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *line, *x, *y, *list;
  struct objlist *Obj;
  int Id, changed;
};
void FileMoveDialog(struct FileMoveDialog *data, struct objlist *obj, int id);

struct FileMaskDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *line, *list;
  struct objlist *Obj;
  int Id, changed;
};
void FileMaskDialog(struct FileMaskDialog *data, struct objlist *obj, int id);

struct FileLoadDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *headskip, *readstep, *finalline, *remark, *ifs, *csv;
  struct objlist *Obj;
  int Id;
};
void FileLoadDialog(struct FileLoadDialog *data, struct objlist *obj, int id);

struct FileMathDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *xsmooth, *xmath, *ysmooth, *ymath, *fmath, *gmath, *hmath;
  struct objlist *Obj;
  int Id;
};
void FileMathDialog(struct FileMathDialog *data, struct objlist *obj, int id);

struct SectionDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *x, *y, *w, *h, *xid, *yid, *rid, *uid, *gid,
    *width, *height, *xaxis, *yaxis, *uaxis, *raxis, *grid;
  int Section;
  int X, Y, LenX, LenY, X0, Y0, LenX0, LenY0;
  int IDX, IDY, IDU, IDR, *IDG;
  struct objlist *Obj, *Obj2;
  int MaxX, MaxY;
};
void SectionDialog(struct SectionDialog *data,
		   int x, int y, int lenx, int leny,
		   struct objlist *obj, int idx, int idy, int idu, int idr,
		   struct objlist *obj2, int *idg, int section);

struct CrossDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *x, *y, *w, *h, *xid, *yid, *width, *height, *xaxis, *yaxis;
  int X, Y, LenX, LenY, X0, Y0, LenX0, LenY0;
  int IDX, IDY;
  struct objlist *Obj;
  int MaxX, MaxY;
};

void CrossDialog(struct CrossDialog *data,
		 int x, int y, int lenx, int leny,
		 struct objlist *obj, int idx, int idy);

struct AxisDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *min, *max, *inc, *div, *scale, *ref, *clear;
  struct objlist *Obj;
  int Id;
  int CanDel;
};

void AxisDialog(void *data, struct objlist *obj, int id, int candel);

struct AxisBaseDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *style, *width, *color, *arrow, *arrowlen, *arrowwid,
    *wave, *wavelen, *wavewid, *baseline;
  struct objlist *Obj;
  int Id;
  int R, G, B;
};
void AxisBaseDialog(struct AxisBaseDialog *data, struct objlist *obj, int id);

struct AxisPosDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *x, *y, *len, *direction, *adjust, *adjustpos;
  struct objlist *Obj;
  int Id;
};
void AxisPosDialog(struct AxisPosDialog *data, struct objlist *obj, int id);

struct AxisFontDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *space, *pt, *script, *font, *jfont, *color;
  struct objlist *Obj;
  int Id;
  int R, G, B;
};
void AxisFontDialog(struct AxisFontDialog *data, struct objlist *obj, int id);

#define GAUGE_STYLE_NUM 3
struct GaugeDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  struct objlist *Obj;
  GtkWidget *length[GAUGE_STYLE_NUM], *width[GAUGE_STYLE_NUM],
    *gauge, *min, *max, *style, *color;
  int Id;
  int R, G, B;
};
void GaugeDialog(struct GaugeDialog *data, struct objlist *obj, int id);

#define GRID_DIALOG_STYLE_NUM 3

struct GridDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  struct objlist *Obj;
  GtkWidget *style[GRID_DIALOG_STYLE_NUM], *width[GRID_DIALOG_STYLE_NUM],
    *axisx, *axisy, *background, *color, *bcolor;
  int Id;
  int R, G, B, R2, G2, B2;
};
void GridDialog(struct GridDialog *data, struct objlist *obj, int id);

struct ZoomDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *zoom_entry;
  int zoom;
};
void ZoomDialog(struct ZoomDialog *data);

struct NumDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *num, *begin, *ster, *numnum, *head, *fraction, *add_plus, *tail,
    *align, *direction, *shiftp, *shiftn, *log_power, *no_zero, *norm, *step;
  struct objlist *Obj;
  int Id;
};
void NumDialog(struct NumDialog *data, struct objlist *obj, int id);

struct MergeDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *file, *topmargin, *leftmargin, *zoom, *greeksymbol;
  struct objlist *Obj;
  int Id;
};
void MergeDialog(void *data, struct objlist *obj, int id, int Sub_id);

struct LegendDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  char *(* prop_cb) (struct objlist *obj, int id);
  GtkWidget *style, *points, *interpolation, *width, *miter, *join,
    *color,*color2, *x, *y, *x1, *y1, *x2, *y2, *rx, *ry, *angle1, *angle2,
    *pieslice, *fill, *fill_rule, *frame, *arrow, *arrow_length, *arrow_width,
    *size, *type, *view, *text, *pt, *space, *script_size, *direction, *raw, *font, *jfont;
  struct objlist *Obj;
  int Id;
  int R, G, B, R2, G2, B2, wid, ang;
  struct MarkDialog mark;
  GdkPixmap *arrow_pixmap;
};

void LegendCurveDialog(struct LegendDialog *data,
		       struct objlist *obj, int id);
void LegendPolyDialog(struct LegendDialog *data,
		      struct objlist *obj, int id);
void LegendArrowDialog(struct LegendDialog *data,
		       struct objlist *obj, int id);
void LegendRectDialog(struct LegendDialog *data,
		      struct objlist *obj, int id);
void LegendArcDialog(struct LegendDialog *data,
		     struct objlist *obj, int id);
void LegendMarkDialog(struct LegendDialog *data,
		      struct objlist *obj, int id);
void LegendTextDialog(struct LegendDialog *data,
		      struct objlist *obj, int id);

struct LegendGaussDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GSList *func_list, *dir_list;
  GtkWidget *style, *div, *color, *width, *miter, *join, *sch, *scv, *view;
  int R, G, B;
  struct objlist *Obj;
  int Id;
  int Minx, Miny, Wdx, Wdy;
  int Div, Dir;
  int Mode;
  double Position, Param;
  int alloc;
};
void LegendGaussDialog(struct LegendGaussDialog *data,
		       struct objlist *obj, int id,
		       int minx, int miny, int wdx, int wdy);

struct PageDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *leftmargin, *topmargin, *paperwidth, *paperheight, *paperzoom, *paper;
};
void PageDialog(struct PageDialog *data);

struct SwitchDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *drawlist, *objlist, *top, *up, *down, *bottom, *del, *ins, *add;
  int btn_lock;
  struct narray drawrable;
  struct narray idrawrable;
};
void SwitchDialog(struct SwitchDialog *data);

struct DirectoryDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *dir;
};
void DirectoryDialog(struct DirectoryDialog *data);

struct LoadDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *expand_file, *ignore_path, *dir;
  int expand;
  char *exdir;
  int ignorepath;
  int Id;
};
void LoadDialog(struct LoadDialog *data);

struct PrmDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *ignore_path, *greek_symbol;
  struct objlist *Obj;
  int Id;
};
void PrmDialog(struct PrmDialog *data, struct objlist *obj, int id);

struct SaveDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *path, *include_data, *include_merge;
  int Path;
  int *SaveData, *SaveMerge;
};
void SaveDialog(struct SaveDialog *data, int *sdata, int *smerge);

struct DriverDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *driver, *file, *option;
  struct objlist *Obj;
  int Id;
};
void DriverDialog(struct DriverDialog *data, struct objlist *obj, int id);

struct PrintDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *driver, *print, *option;
  struct objlist *Obj;
  int Id;
};
void PrintDialog(struct PrintDialog *data, struct objlist *obj, int id);

struct OutputDataDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *div_entry;
  int div;
};
void OutputDataDialog(struct OutputDataDialog *data, int div);

struct ScriptDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  char *execscript;
  GtkWidget *list, *entry;
  char option[256];
};
void ScriptDialog(struct ScriptDialog *data);

struct DefaultDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *geometry, *child_geometry, *viewer, *external_driver, *addin_script, *misc, *external_viewer;
};
void DefaultDialog(struct DefaultDialog *data);

struct SetScriptDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *name, *script, *option;
  struct script *Script;
};
void SetScriptDialog(struct SetScriptDialog *data, struct script *sc);

struct PrefScriptDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *list;
};
void PrefScriptDialog(struct PrefScriptDialog *data);

struct SetDriverDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *name, *driver, *option, *ext;
  struct extprinter *Driver;
};
void SetDriverDialog(struct SetDriverDialog *data, struct extprinter *prn);

struct PrefDriverDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *list;
};
void PrefDriverDialog(struct PrefDriverDialog *data);

struct MiscDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *editor, *directory, *history, *path, *datafile,
    *expand, *expanddir, *ignorepath, *mergefile, *bgcol,
    *hist_size, *info_size, *preserve_width, *data_head_lines;
  struct objlist *Obj;
  int Id;
};
void MiscDialog(struct MiscDialog *data, struct objlist *obj, int id);

struct ExViewerDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *dpi, *width, *height, *use_external;
  struct objlist *Obj;
  int Id;
};
void ExViewerDialog(struct ExViewerDialog *data);

struct ViewerDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *dpi, *ruler, *redraw, *loadfile, *grid, *data_num, *antialias;
  struct objlist *Obj;
  int Id;
  int Clear;
  int dpis;
};
void ViewerDialog(struct ViewerDialog *data, struct objlist *obj, int id);

struct SelectDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  struct narray *sel, *isel;
  struct objlist *Obj;
  char *Field;
  GtkWidget *list;
  char *(*cb) (struct objlist * obj, int id);
};
void SelectDialog(struct SelectDialog *data,
		  struct objlist *obj,
		  char *(*callback) (struct objlist * obj, int id),
		  struct narray *array, struct narray *iarray);

struct CopyDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  struct objlist *Obj;
  int Id;
  int sel;
  GtkWidget *list;
  char *(*cb) (struct objlist * obj, int id);
};

void CopyDialog(struct CopyDialog *data,
		struct objlist *obj, int id,
		char *(*callback) (struct objlist * obj, int id));

struct OutputImageDialog
{
  GtkWidget *parent, *widget;
  GtkVBox *vbox;
  int ret, show_buttons, show_cancel;
  char *resource;
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);
  void (*CloseWindow) (GtkWidget *w, void *data);
  /****** local member *******/
  GtkWidget *version, *t2p, *dpi, *vlabel, *dlabel;
  int Version, text2path, Dpi, DlgType;
};
void OutputImageDialog(struct OutputImageDialog *data, int type);



extern struct FileDialog DlgFile;
extern struct FileDialog DlgFileDef;
extern struct EvalDialog DlgEval;
extern struct MathDialog DlgMath;
extern struct MathTextDialog DlgMathText;
extern struct FitDialog DlgFit;
extern struct FitLoadDialog DlgFitLoad;
extern struct FitSaveDialog DlgFitSave;
extern struct FileMoveDialog DlgFileMove;
extern struct FileMaskDialog DlgFileMask;
extern struct FileLoadDialog DlgFileLoad;
extern struct FileMathDialog DlgFileMath;
extern struct SectionDialog DlgSection;
extern struct CrossDialog DlgCross;
extern struct AxisDialog DlgAxis;
extern struct GridDialog DlgGrid;
extern struct ZoomDialog DlgZoom;
extern struct AxisBaseDialog DlgAxisBase;
extern struct AxisPosDialog DlgAxisPos;
extern struct NumDialog DlgNum;
extern struct AxisFontDialog DlgAxisFont;
extern struct GaugeDialog DlgGauge;
extern struct MergeDialog DlgMerge;
extern struct LegendDialog DlgLegendCurve;
extern struct LegendDialog DlgLegendPoly;
extern struct LegendDialog DlgLegendArrow;
extern struct LegendDialog DlgLegendRect;
extern struct LegendDialog DlgLegendArc;
extern struct LegendDialog DlgLegendMark;
extern struct LegendDialog DlgLegendText;
extern struct LegendDialog DlgLegendTextDef;
extern struct LegendGaussDialog DlgLegendGauss;
extern struct PageDialog DlgPage;
extern struct SwitchDialog DlgSwitch;
extern struct DirectoryDialog DlgDirectory;
extern struct LoadDialog DlgLoad;
extern struct PrmDialog DlgPrm;
extern struct SaveDialog DlgSave;
extern struct DriverDialog DlgDriver;
extern struct PrintDialog DlgPrinter;
extern struct OutputDataDialog DlgOutputData;
extern struct ScriptDialog DlgScript;
extern struct DefaultDialog DlgDefault;
extern struct SetScriptDialog DlgSetScript;
extern struct PrefScriptDialog DlgPrefScript;
extern struct SetDriverDialog DlgSetDriver;
extern struct PrefDriverDialog DlgPrefDriver;
extern struct MiscDialog DlgMisc;
extern struct ExViewerDialog DlgExViewer;
extern struct ViewerDialog DlgViewer;
extern struct SelectDialog DlgSelect;
extern struct CopyDialog DlgCopy;
extern struct OutputImageDialog DlgImageOut;

#endif


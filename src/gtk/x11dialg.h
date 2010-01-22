/* 
 * $Id: x11dialg.h,v 1.53 2010/01/22 02:02:23 hito Exp $
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

#define MARK_TYPE_NUM 90
#define MATH_FNC_NUM 5

struct line_style {
  char *name, *list;
  int num;
};

extern struct line_style FwLineStyle[];
extern char *FwNumStyle[];
extern int FwNumStyleNum;

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
int chk_sputobjfield(struct objlist *obj, int id, char *field, char *str);
const char *get_style_string(struct objlist *obj, int id, char *field);

#define DIALOG_PROTOTYPE GtkWidget *parent, *widget, *focus;\
  GtkVBox *vbox;\
  int ret, show_buttons, show_cancel;\
  char *resource;\
  const char *ok_button;\
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);\
  void (*CloseWindow) (GtkWidget *w, void *data);\


struct DialogType
{
  DIALOG_PROTOTYPE;
};

struct MarkDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *toggle[MARK_TYPE_NUM];
  int Type, cb_respond;
};
void MarkDialog(struct MarkDialog *data, int type);

struct FileMath
{
  GtkWidget *xsmooth, *x, *ysmooth, *y, *f, *g, *h;
  int tab_id;
};

struct FileLoad
{
  GtkWidget *headskip, *readstep, *finalline, *remark, *ifs, *csv;
  int tab_id;
};

struct FileMask
{
  GtkWidget *line, *list;
  int changed, tab_id;
};

struct FileMove
{
  GtkWidget *line, *x, *y, *list;
  int changed, tab_id;
};

struct FileDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *file, *load_settings, *fit, *xcol, *xaxis, *ycol, *yaxis,
    *type, *mark_btn, *mark_label, *curve, *curve_label, *col1, *col2, *col2_label,
    *clip, *style, *size, *size_label, *miter, *miter_label, *join, *join_label,
    *comment_box, *file_box, *fit_table, *width, *apply_all, *comment_view;
  GtkNotebook *tab;
  GtkTextBuffer *comment;
  GtkTextTag *comment_num_tag;
  struct objlist *Obj;
  int Id;
  struct MarkDialog mark;
  int R, G, B, R2, G2, B2, multi_open, fit_row, tab_active;
  struct FileMath math;
  struct FileLoad load;
  struct FileMask mask;
  struct FileMove move;
};

void FileDialog(void *data, struct objlist *obj, int id, int candel);

struct EvalDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *list;
  struct narray *sel;
  struct objlist *Obj;
  int Num;
};
void EvalDialog(struct EvalDialog *data,
		struct objlist *obj, int num, struct narray *iarray);

struct MathDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *list, *func[MATH_FNC_NUM];
  struct objlist *Obj;
  int Mode;
};
void MathDialog(struct MathDialog *data, struct objlist *obj);

struct MathTextDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *list, *label, *tree;
  GList *id_list;
  struct objlist *Obj;
  char *Text;
  int Mode;
};
void MathTextDialog(struct MathTextDialog *data, char *text, int mode, struct objlist *obj, GList *list, GtkWidget *tree);

#define FIT_PARM_NUM 10

struct FitDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *type, *through_point, *x, *y, *dim, *weight,
    *min, *max, *div, *interpolation, *formula, *converge,
    *derivatives, *p[FIT_PARM_NUM], *d[FIT_PARM_NUM], *d_label[FIT_PARM_NUM], *through_box,
    *dim_label, *usr_def_frame, *func_label;
  struct objlist *Obj;
  int Id;
  int Lastid;
};
void FitDialog(struct FitDialog *data, struct objlist *obj, int id);

struct FitLoadDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *list;
  struct objlist *Obj;
  int Sid;
  int sel;
};
void FitLoadDialog(struct FitLoadDialog *data, struct objlist *obj, int sid);

struct FitSaveDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *profile;
  struct objlist *Obj;
  int Sid;
  char *Profile;
};
void FitSaveDialog(struct FitSaveDialog *data, struct objlist *obj, int sid);

struct SectionDialog
{
  DIALOG_PROTOTYPE;
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
  DIALOG_PROTOTYPE;
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


struct AxisBase
{
  GtkWidget *style, *width, *color, *arrow, *arrowlen, *arrowwid,
    *wave, *wavelen, *wavewid, *baseline;
  int R, G, B, tab_id;
};

struct AxisPos
{
  GtkWidget *x, *y, *len, *direction, *adjust, *adjustpos;
  int tab_id;
};

struct AxisFont
{
  GtkWidget *space, *pt, *script, *font, *jfont, *color;
  int R, G, B, tab_id;
};

struct AxisNumbering
{
  GtkWidget *num, *begin, *ster, *numnum, *head, *fraction, *add_plus, *tail,
    *align, *direction, *shiftp, *shiftn, *log_power, *no_zero, *norm, *step;
  int tab_id;
};

#define GAUGE_STYLE_NUM 3
struct AxisGauge
{
  GtkWidget *length[GAUGE_STYLE_NUM], *width[GAUGE_STYLE_NUM],
    *gauge, *min, *max, *style, *color;
  int R, G, B, tab_id;
};

struct AxisDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *min, *max, *inc, *div, *scale, *ref, *clear;
  GtkNotebook *tab;
  GtkWidget *del_btn;
  struct objlist *Obj;
  int Id, CanDel, tab_active;
  struct AxisBase base;
  struct AxisFont font;
  struct AxisPos position;
  struct AxisGauge gauge;
  struct AxisNumbering numbering;
};
void AxisDialog(void *data, struct objlist *obj, int id, int candel);

#define GRID_DIALOG_STYLE_NUM 3

struct GridDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  struct objlist *Obj;
  GtkWidget *style[GRID_DIALOG_STYLE_NUM], *width[GRID_DIALOG_STYLE_NUM],
    *axisx, *axisy, *background, *color, *bcolor, *bclabel;
  int Id;
  int R, G, B, R2, G2, B2;
};
void GridDialog(struct GridDialog *data, struct objlist *obj, int id);

struct ZoomDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *zoom_entry;
  int zoom;
};
void ZoomDialog(struct ZoomDialog *data);

struct MergeDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *file, *topmargin, *leftmargin, *zoom, *greeksymbol;
  struct objlist *Obj;
  int Id;
};
void MergeDialog(void *data, struct objlist *obj, int id, int Sub_id);

struct LegendDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  char *(* prop_cb) (struct objlist *obj, int id);
  GtkWidget *style, *points, *interpolation, *width, *miter, *join,
    *color,*color2, *x, *y, *x1, *y1, *x2, *y2, *rx, *ry, *angle1, *angle2,
    *pieslice, *fill, *fill_rule, *frame, *arrow, *arrow_length, *arrow_width,
    *size, *type, *view, *text, *pt, *space, *script_size, *direction, *raw, *font, *jfont,
    *color2_label;
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
  DIALOG_PROTOTYPE;
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
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *leftmargin, *topmargin, *paperwidth, *paperheight, *paperzoom, *paper;
};
void PageDialog(struct PageDialog *data);

struct SwitchDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *drawlist, *objlist, *top, *up, *down, *bottom, *del, *ins, *add;
  int btn_lock;
  struct narray drawrable;
  struct narray idrawrable;
};
void SwitchDialog(struct SwitchDialog *data);

struct DirectoryDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *dir;
};
void DirectoryDialog(struct DirectoryDialog *data);

struct LoadDialog
{
  DIALOG_PROTOTYPE;
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
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *ignore_path, *greek_symbol;
  struct objlist *Obj;
  int Id;
};
void PrmDialog(struct PrmDialog *data, struct objlist *obj, int id);

struct SaveDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *path, *include_data, *include_merge;
  int Path;
  int *SaveData, *SaveMerge;
};
void SaveDialog(struct SaveDialog *data, int *sdata, int *smerge);

struct DriverDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *driver, *file, *option;
  char *ext;
  struct objlist *Obj;
  int Id;
};
void DriverDialog(struct DriverDialog *data, struct objlist *obj, int id);

struct PrintDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *driver, *print, *option;
  struct objlist *Obj;
  int Id;
};
void PrintDialog(struct PrintDialog *data, struct objlist *obj, int id);

struct OutputDataDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *div_entry;
  int div;
};
void OutputDataDialog(struct OutputDataDialog *data, int div);

struct DefaultDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *geometry, *child_geometry, *viewer, *external_driver, *addin_script, *misc, *external_viewer, *fonts;
};
void DefaultDialog(struct DefaultDialog *data);

struct SetScriptDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *name, *script, *option, *description;
  struct script *Script;
};
void SetScriptDialog(struct SetScriptDialog *data, struct script *sc);

struct PrefScriptDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *list, *update_b, *del_b, *up_b, *down_b;
};
void PrefScriptDialog(struct PrefScriptDialog *data);

struct SetDriverDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *name, *driver, *option, *ext;
  struct extprinter *Driver;
};
void SetDriverDialog(struct SetDriverDialog *data, struct extprinter *prn);

struct PrefDriverDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *list, *update_b, *del_b, *up_b, *down_b;
};
void PrefDriverDialog(struct PrefDriverDialog *data);

struct PrefFontDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *list, *alias, *two_byte, *update_b, *del_b, *up_b, *down_b;
};
void PrefFontDialog(struct PrefFontDialog *data);

struct MiscDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *editor, *directory, *history, *path, *datafile,
    *expand, *expanddir, *ignorepath, *mergefile, *coordwin_font, *infowin_font,
    *file_preview_font, *hist_size, *info_size, *data_head_lines;
  struct objlist *Obj;
  int Id;
};
void MiscDialog(struct MiscDialog *data, struct objlist *obj, int id);

struct ExViewerDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *dpi, *width, *height, *use_external;
  struct objlist *Obj;
  int Id;
};
void ExViewerDialog(struct ExViewerDialog *data);

struct ViewerDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *dpi, *loadfile, *grid, *data_num, *antialias, *fftype,
    *bgcol, *preserve_width;
  struct objlist *Obj;
  int Id;
  int Clear;
  int dpis;
};
void ViewerDialog(struct ViewerDialog *data, struct objlist *obj, int id);

struct SelectDialog
{
  DIALOG_PROTOTYPE;
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
  DIALOG_PROTOTYPE;
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
  DIALOG_PROTOTYPE;
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
extern struct DefaultDialog DlgDefault;
extern struct SetScriptDialog DlgSetScript;
extern struct PrefScriptDialog DlgPrefScript;
extern struct SetDriverDialog DlgSetDriver;
extern struct PrefDriverDialog DlgPrefDriver;
extern struct PrefFontDialog DlgPrefFont;
extern struct MiscDialog DlgMisc;
extern struct ExViewerDialog DlgExViewer;
extern struct ViewerDialog DlgViewer;
extern struct SelectDialog DlgSelect;
extern struct CopyDialog DlgCopy;
extern struct OutputImageDialog DlgImageOut;

#endif


/*
 * $Id: x11dialg.h,v 1.57 2010-02-03 01:18:12 hito Exp $
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

#include "common.h"

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "object.h"
#include "ofit.h"
#include "ogra2cairo.h"
#include "x11menu.h"

#define MATH_FNC_NUM 5
#define AXIS_SELECTION_LIMIT 6

#define LINE_STYLE_ELEMENT_MAX 6
struct line_style {
  char *name, *list;
  int nlist[LINE_STYLE_ELEMENT_MAX];
  int num;
};

extern struct line_style FwLineStyle[];
extern char *FwNumStyle[];
extern int FwNumStyleNum;

enum AXIS_COMBO_BOX_FLAGS {
  AXIS_COMBO_BOX_NONE = 0,
  AXIS_COMBO_BOX_USE_OID = 1,
  AXIS_COMBO_BOX_ADD_NONE = 2,
};

#define N_RESPONSE_ALL 1

void initdialog(void);
void CopyClick(GtkWidget *parent, struct objlist *obj, int Id,
	       char *(*callback) (struct objlist *, int),
               response_cb response_cb, gpointer user_data);
int SetObjFieldFromWidget(GtkWidget *w, struct objlist *Obj, int Id, char *field);
void SetWidgetFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field);
int SetObjPointsFromText(GtkWidget *w, struct objlist *Obj, int Id, char *field);
void SetTextFromObjPoints(GtkWidget *w, struct objlist *Obj, int Id, char *field);
int SetObjFieldFromStyle(GtkWidget *w, struct objlist *Obj, int Id, char *field);
void SetStyleFromObjField(GtkWidget *w, struct objlist *Obj, int Id, char *field);
GtkWidget *axis_combo_box_create(int flags);
void axis_combo_box_setup(GtkWidget *cbox, struct objlist *obj, int id, const char *field);
int SetObjAxisFieldFromWidget(GtkWidget *w, struct objlist *obj, int id, char *field);
struct compatible_font_info *SetFontListFromObj(GtkWidget *w, struct objlist *obj, int id, const char *name);
void SetObjFieldFromFontList(GtkWidget *w, struct objlist *obj, int id, char *name);
void set_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id, char *prefix);
void set_color2(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id);
void set_fill_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id);
void set_stroke_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id);
int putobj_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id, const char *prefix);
int putobj_color2(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id);
int putobj_fill_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id);
int putobj_stroke_color(GtkWidget *w, GtkWidget *aw, struct objlist *obj, int id);
int chk_sputobjfield(struct objlist *obj, int id, char *field, char *str);
int get_style_index(struct objlist *obj, int id, char *field);
const char *get_style_string(struct objlist *obj, int id, char *field);

struct DialogType;
struct response_callback;

typedef void (* response_callback_func) (struct response_callback *);
typedef void (* response_callback_free_func) (struct response_callback *);

struct response_callback
{
  response_callback_func cb;
  response_callback_free_func free;
  gpointer data;
  int return_value;
  struct DialogType *dialog;
};

void response_callback_add(void *dialog, response_callback_func cb, response_callback_free_func free, gpointer data);

#define DIALOG_PROTOTYPE GtkWidget *parent, *widget, *focus;            \
  GtkBox *vbox;                                                         \
  struct response_callback *response_cb;                                \
  int ret, show_cancel;                                                 \
  char *resource, *ok_button;                                           \
  void (*SetupWindow) (GtkWidget *w, void *data, int makewidget);       \
  void (*CloseWindow) (GtkWidget *w, void *data);                       \
  int lockstate, menulock, modified;                                    \
  GtkWidget *win_ptr, *ok;						\


struct DialogType
{
  DIALOG_PROTOTYPE;
};

struct FileMath
{
  GtkWidget *xsmooth, *x, *ysmooth, *averaging_type, *y, *f, *g, *h, *text_x, *text_y, *text_f, *text_g, *text_h;
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
    *type, *mark_btn, *curve, *col1, *col2, *alpha1, *alpha2, *math_input_tab,
    *clip, *style, *size, *miter, *join, *min, *max, *div,
    *comment_box, *file_box, *fit_table, *width, *comment_view, *comment_table;
  GtkNotebook *tab, *math_tab;
  struct objlist *Obj;
  int Id, source, math_page;
  int multi_open, fit_row, initialized;
  struct FileMath math;
  struct FileLoad load;
  struct FileMask mask;
  struct FileMove move;
  char *head_lines;
};

void FileDialog(struct obj_list_data *data, int id, int multi);

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
  GtkWidget *list, *text, *tree, *input_tab;
  struct objlist *Obj;
  char *Text;
  int Mode, page;
};
void MathTextDialog(struct MathTextDialog *data, char *text, int mode, struct objlist *obj, GtkWidget *tree);

#define FIT_PARM_NUM FIT_DIMENSION_MAX

struct FitDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *type, *through_point, *x, *y, *dim, *weight,
    *min, *max, *div, *interpolation, *formula, *converge,
    *derivatives, *p[FIT_PARM_NUM], *d[FIT_PARM_NUM], *through_box,
    *usr_def_frame, *usr_def_prm_tbl, *func_label;
  struct objlist *Obj;
  int Id;
  int Lastid;
};
void FitDialog(struct FitDialog *data, struct objlist *obj, int id);

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
  int IDX, IDY, IDU, IDR, IDG;
  struct objlist *Obj, *Obj2;
  int MaxX, MaxY;
};
void SectionDialog(struct SectionDialog *data,
		   int x, int y, int lenx, int leny,
		   struct objlist *obj, int idx, int idy, int idu, int idr,
		   struct objlist *obj2, int idg, int section);

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
  GtkWidget *style, *width, *color, *alpha, *arrow, *arrowlen, *arrowwid,
    *wave, *wavelen, *wavewid, *baseline;
  int tab_id;
};

struct AxisPos
{
  GtkWidget *x, *y, *len, *direction, *adjust, *adjustpos;
  int tab_id;
};

struct AxisFont
{
  GtkWidget *space, *pt, *script, *font, *color, *alpha, *font_bold, *font_italic;
  int tab_id;
};

struct AxisNumbering
{
  GtkWidget *num, *begin, *ster, *numnum, *head, *fraction, *add_plus, *tail,
    *date_format, *align, *direction, *shiftp, *shiftn,
    *log_power, *no_zero, *norm, *step, *math;
  int tab_id;
};

#define GAUGE_STYLE_NUM 3
struct AxisGauge
{
  GtkWidget *length[GAUGE_STYLE_NUM], *width[GAUGE_STYLE_NUM],
    *gauge, *min, *max, *style, *color, *alpha;
  int tab_id;
};

struct AxisDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *min, *max, *inc, *div, *scale, *ref, *clear, *margin;
  GtkNotebook *tab;
  struct objlist *Obj;
  int Id;
  struct AxisBase base;
  struct AxisFont font;
  struct AxisPos position;
  struct AxisGauge gauge;
  struct AxisNumbering numbering;
};
void AxisDialog(struct obj_list_data *data, int id, int sub_id);

#define GRID_DIALOG_STYLE_NUM 3

struct GridDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  struct objlist *Obj;
  GtkWidget *style[GRID_DIALOG_STYLE_NUM], *width[GRID_DIALOG_STYLE_NUM],
    *axisx, *axisy, *background, *color, *bcolor, *alpha, *balpha, *draw_x, *draw_y;
  int Id;
  int R, G, B, R2, G2, B2, A;
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
  GtkWidget *file, *topmargin, *leftmargin, *zoom_x, *zoom_y, *link;
  struct objlist *Obj;
  int Id;
};
void MergeDialog(struct obj_list_data *data, int id, int Sub_id);

struct ParameterDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *type, *start, *stop, *transition, *transition_step, *min, *max, *step, *wait, *checked, *selected, *value, *items, *redraw, *title, *stack, *wrap;
  struct objlist *Obj;
  int Id;
};
void ParameterDialog(struct obj_list_data *data, int id, int Sub_id);

struct LegendDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  char *(* prop_cb) (struct objlist *obj, int id);
  GtkWidget *path_type, *style, *points, *interpolation, *width,
    *miter, *join, *color, *color2, *alpha, *alpha2, *stroke_color, *stroke_alpha, *fill_color, *fill_alpha,
    *x, *y, *x1, *y1, *x2, *y2, *rx, *ry, *angle1, *angle2,
    *pieslice, *close_path, *stroke, *fill, *fill_rule,
    *marker_begin, *marker_end, *arrow_length, *arrow_width,
    *size, *type, *view, *text, *pt, *mark_type_begin, *mark_type_end,
    *space, *script_size, *direction, *raw, *font, *font_bold,
    *font_italic;
  struct objlist *Obj;
  int Id;
  int R, G, B, R2, G2, B2, fill_R, fill_G, fill_B, A, A2, fill_A, wid, ang;
  cairo_surface_t *arrow_pixmap;
};

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
  GtkWidget *style, *div, *color, *alpha, *width, *miter, *join, *sch, *scv, *view;
  int R, G, B, A;
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
  GtkWidget *leftmargin, *topmargin, *paperwidth, *paperheight, *paperzoom, *paper, *decimalsign, *portrait, *landscape;
  int new_graph;
};
void PageDialog(struct PageDialog *data, int new_graph);

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
  GtkWidget *dir, *dir_label;
};
void DirectoryDialog(struct DirectoryDialog *data);

struct LoadDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *expand_file, *load_path, *dir;
  int expand;
  char *exdir, *file;
  int loadpath;
  int Id;
};
void LoadDialog(struct LoadDialog *data, const char *file);

struct SaveDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *path, *include_data, *include_merge;
  int Path;
  int SaveData, SaveMerge;
};
void SaveDialog(struct SaveDialog *data);

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
  GtkWidget *viewer, *addin_script, *misc, *external_viewer, *fonts;
};
void DefaultDialog(struct DefaultDialog *data);

struct SetScriptDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *addins, *name, *script, *option, *description;
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

struct PrefFontDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *list, *update_b, *del_b, *up_b, *down_b;
};
void PrefFontDialog(struct PrefFontDialog *data);

struct FontSettingDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *list, *alias, *font_b, *add_b, *del_b, *up_b, *down_b;
  const gchar *alias_str, *font_str, *alternative_str;
  int is_update;
};
void FontSettingDialog(struct FontSettingDialog *d, const char *alias, const char *font, const char *alternative);

struct MiscDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *editor, *path, *datafile,
    *expand, *expanddir, *loadpath, *mergefile, *coordwin_font, *infowin_font,
    *file_preview_font, *hist_size, *info_size, *data_head_lines, *help_browser,
    *browser, *select_data, *use_custom_palette, *use_dark_theme, *source_style,
    *decimalsign, *icon_size;
  GtkWidget **palette;
  GtkWidget *directory;
  struct objlist *Obj;
  struct narray tmp_palette;
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
  const char *title;
  char *(*cb) (struct objlist * obj, int id);
};
void SelectDialog(struct SelectDialog *data,
		  struct objlist *obj,
		  const char *title,
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
  const char *title;
  char *(*cb) (struct objlist * obj, int id);
  response_cb rcb;
};

void CopyDialog(struct CopyDialog *data,
		struct objlist *obj, int id,
		const char *title,
		char *(*callback) (struct objlist * obj, int id));

struct OutputImageDialog
{
  DIALOG_PROTOTYPE;
  /****** local member *******/
  GtkWidget *version, *t2p, *dpi, *use_opacity;
  int Version, text2path, Dpi, DlgType, UseOpacity;
};



extern struct FileDialog DlgFile;
extern struct FileDialog DlgRange;
extern struct FileDialog DlgArray;
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
extern struct GridDialog DlgGridDef;
extern struct ZoomDialog DlgZoom;
extern struct AxisBaseDialog DlgAxisBase;
extern struct AxisPosDialog DlgAxisPos;
extern struct NumDialog DlgNum;
extern struct AxisFontDialog DlgAxisFont;
extern struct GaugeDialog DlgGauge;
extern struct MergeDialog DlgMerge;
extern struct ParameterDialog DlgParameter;
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
extern struct SaveDialog DlgSave;
extern struct OutputDataDialog DlgOutputData;
extern struct DefaultDialog DlgDefault;
extern struct SetScriptDialog DlgSetScript;
extern struct PrefScriptDialog DlgPrefScript;
extern struct PrefFontDialog DlgPrefFont;
extern struct FontSettingDialog DlgFontSetting;
extern struct MiscDialog DlgMisc;
extern struct ExViewerDialog DlgExViewer;
extern struct ViewerDialog DlgViewer;
extern struct SelectDialog DlgSelect;
extern struct CopyDialog DlgCopy;
extern struct OutputImageDialog DlgImageOut;

#endif

#include "common.h"

#ifdef WINDOWS
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <unistd.h>
#include <math.h>
#include <cairo/cairo.h>

#include "gtk_common.h"

#include "ngraph.h"
#include "object.h"
#include "nstring.h"
#include "ioutil.h"
#include "shell.h"
#include "strconv.h"
#include "gra.h"
#include "ogra2cairo.h"

#define NAME "gra2emf"
#define PARENT "gra2"
#define OVERSION  "1.00.00"

#define DEFAULT_FONT "Sans-serif"

#ifndef MPI
#define MPI 3.141592
#endif

#define ERRFOPEN 100

static char *gra2emf_errorlist[]={
  "I/O error: open file"
};

#define ERRNUM (sizeof(gra2emf_errorlist) / sizeof(*gra2emf_errorlist))

struct gra2emf_local {
  HDC hdc;
  int r, g, b, x, y, offsetx, offsety;
  char *fontalias;
  int font_style, symbol, line_join, line_cap, line_width, line_style_num;
  double fontdir, fontcos, fontsin, fontspace, fontsize;
  DWORD *line_style;
};

static int
gra2emf_init(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2emf_local *local = NULL;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  local = g_malloc0(sizeof(struct gra2emf_local));
  if (local == NULL){
    goto errexit;
  }

  if (_putobj(obj, "_local", inst, local))
    goto errexit;

  return 0;

 errexit:
  g_free(local);

  return 1;
}

static int
gra2emf_done(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2emf_local *local;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  _getobj(obj, "_local", inst, &local);
  if (local == NULL) {
    return 0;
  }

  if (local->fontalias) {
    g_free(local->fontalias);
    local->fontalias = NULL;
  }

  return 0;
}

static HDC
open_emf(void)
{
  HDC hdc;
  XFORM xform = {1, 0, 0, -1, 0, 0};

  hdc = CreateEnhMetaFile(NULL, NULL, NULL, NULL);
  if (hdc == NULL) {
    return NULL;
  }

  SetGraphicsMode(hdc, GM_ADVANCED);
  SetMapMode(hdc, MM_HIMETRIC);
  SetWorldTransform(hdc, &xform);
  SetBkMode(hdc, TRANSPARENT);
  SetArcDirection(hdc, AD_COUNTERCLOCKWISE);
  StartPage(hdc);

  return hdc;
}

static int
close_emf(HDC hdc, const char *fname)
{
  HENHMETAFILE emf;
  int r;

  r = 1;

  EndPage(hdc);
  emf = CloseEnhMetaFile(hdc);
  if (emf == NULL) {
    return 1;
  }

  if (fname) {
    HENHMETAFILE emf2;
    emf2 = CopyEnhMetaFile(emf, fname);
    if (emf2) {
      DeleteEnhMetaFile(emf2);
      r = 0;
    }
  } else {
    if (OpenClipboard(NULL)) {
      EmptyClipboard();
      SetClipboardData(CF_ENHMETAFILE, emf);
      CloseClipboard();
      r = 0;
    }
  }
  DeleteEnhMetaFile(emf);
  DeleteDC(hdc);
  local->emf = NULL;

  return r;
}

static HFONT
select_font(struct gra2emf_local *local)
{
  LOGFONTW id_font;
  gunichar2 *ustr;

  id_font.lfHeight = - local->fontsize;
  id_font.lfWidth = 0;
  id_font.lfEscapement = local->fontdir * 10;
  id_font.lfOrientation = local->fontdir * 10;
  id_font.lfUnderline = 0;
  id_font.lfStrikeOut = 0;
  id_font.lfWeight = (local->font_style & GRA_FONT_STYLE_BOLD) ? FW_BOLD : FW_NORMAL;
  id_font.lfItalic = (local->font_style & GRA_FONT_STYLE_ITALIC) ? TRUE : FALSE;
  if (g_ascii_strncasecmp(local->fontalias, "Sans-serif", 5) == 0) {
    id_font.lfPitchAndFamily = (VARIABLE_PITCH | FF_SWISS);
    wcscpy(id_font.lfFaceName, L"arial");
  } else if (g_ascii_strncasecmp(local->fontalias, "Serif", 5) == 0) {
    id_font.lfPitchAndFamily = (VARIABLE_PITCH | FF_ROMAN);
    wcscpy(id_font.lfFaceName, L"times new roman");
  } else if (g_ascii_strncasecmp(local->fontalias, "Monospace", 7) == 0) {
    id_font.lfPitchAndFamily = (FIXED_PITCH | FF_MODERN);
    wcscpy(id_font.lfFaceName, L"courier new");
  } else {
    id_font.lfPitchAndFamily = (VARIABLE_PITCH | FF_SWISS);
    wcscpy(id_font.lfFaceName, L"arial");
  }

  id_font.lfCharSet = SHIFTJIS_CHARSET;
  id_font.lfCharSet = OEM_CHARSET;
  id_font.lfCharSet = ANSI_CHARSET;
  id_font.lfCharSet = DEFAULT_CHARSET;

  id_font.lfOutPrecision = OUT_DEFAULT_PRECIS;
  id_font.lfClipPrecision = CLIP_STROKE_PRECIS;
  id_font.lfQuality = PROOF_QUALITY;

#if 0
  ustr = g_utf8_to_utf16(local->loadfont->fontname, -1, NULL, &len, NULL);
  wcsncpy(id_font.lfFaceName, ustr, LF_FACESIZE - 1);
  gfree(ustr);
#endif
  id_font.lfFaceName[LF_FACESIZE - 1] = L'\0';
  return CreateFontIndirectW(&id_font);
}

static void
draw_str(struct gra2emf_local *local, const char *str)
{
  double cx, cy, red, green, blue, alpha, ratio;
  gunichar2 *ustr;
  glong len;
  HDC hdc;
  int r, g, b, space, ret;
  char *utf8_str;
  HFONT font, old_font;
  SIZE str_size;

  hdc = local->hdc;

  SetTextAlign(hdc, TA_BASELINE);
  SetTextCharacterExtra(hdc, local->fontspace);
  SetTextColor(hdc, RGB(local->r, local->g, local->b));
  font = select_font(local);
  old_font = SelectObject(hdc, font);

  SetBkMode(hdc, TRANSPARENT);

  utf8_str = gra2cairo_get_utf8_str(str, local->symbol);
  ustr = g_utf8_to_utf16(utf8_str, -1, NULL, &len, NULL);
  g_free(utf8_str);

  ExtTextOutW(hdc,
	      local->x, local->y,
	      0,
	      NULL,
	      ustr, len,
	      NULL);

  GetTextExtentPoint32W(hdc, ustr, len, &str_size);

  SelectObject(hdc, old_font);
  DeleteObject(font);

  local->x += str_size.cx * local->fontcos;
  local->y -= str_size.cx * local->fontsin;

  g_free(ustr);
}

static int
gra2emf_flush(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2emf_local *local;

  _getobj(obj, "_local", inst, &local);

  if (local->hdc == NULL) {
    return -1;
  }

  return 0;
}

static void
create_pen(struct gra2emf_local *local)
{
  HPEN old;
  DWORD *line_style, line_attr;
  LOGBRUSH log_brush;
  HPEN pen;

  line_attr = PS_GEOMETRIC;
  switch (local->line_cap) {
  case 2:
    line_attr |= PS_ENDCAP_SQUARE;
    break;
  case 1:
    line_attr |= PS_ENDCAP_ROUND;
    break;
  default:
    line_attr |= PS_ENDCAP_FLAT;
  }

  switch (local->line_join) {
  case 2:
    line_attr |= PS_JOIN_BEVEL;
    break;
  case 1:
    line_attr |= PS_JOIN_ROUND;
    break;
  default:
    line_attr |= PS_JOIN_MITER;
  }

  if (local->line_style) {
    line_attr |= PS_USERSTYLE;
  } else {
    line_attr |= PS_SOLID;
  }
  log_brush.lbStyle = BS_SOLID;
  log_brush.lbColor = RGB(local->r, local->g, local->b);
  pen = ExtCreatePen(line_attr, local->line_width, &log_brush, local->line_style_num, local->line_style);
  old = SelectObject(local->hdc, pen);
  DeleteObject(old);
}

static void
draw_arc(struct gra2emf_local *local, int x, int y, int w, int h, int start, int angle, int style)
{
  double a1, a2;

  if (angle == 0) {
    return;
  }

  if (angle % 36000 == 0 && (style == 1 || style == 2)) {
    Ellipse(local->hdc,
	    x - w, y - h,
	    x + w, y + h);
    return;
  }

  a1 = start * (MPI / 18000.0);
  a2 = angle * (MPI / 18000.0) + a1;

  switch (style) {
  case 1:
    Pie(local->hdc,
	x - w, y - h,
	x + w, y + h,
	x + w * cos(a1),
	y - h * sin(a1),
	x + w * cos(a2),
	y - h * sin(a2)
	);
    break;
  case 2:
    Chord(local->hdc,
	  x - w, y - h,
	  x + w, y + h,
	  x + w * cos(a1),
	  y - h * sin(a1),
	  x + w * cos(a2),
	  y - h * sin(a2));
    break;
  case 3:
    MoveToEx(local->hdc, x, y, NULL);
    BeginPath(local->hdc);
    ArcTo(local->hdc,
	  x - w, y - h,
	  x + w, y + h,
	  x + w * cos(a1),
	  y - h * sin(a1),
	  x + w * cos(a2),
	  y - h * sin(a2)
	  );
    CloseFigure(local->hdc);
    EndPath(local->hdc);
    StrokePath(local->hdc);
    MoveToEx(local->hdc, local->x, local->y, NULL);
    break;
  case 4:
    BeginPath(local->hdc);
    Arc(local->hdc,
	x - w, y - h,
	x + w, y + h,
	x + w * cos(a1),
	y - h * sin(a1),
	x + w * cos(a2),
	y - h * sin(a2)
	);
    CloseFigure(local->hdc);
    EndPath(local->hdc);
    StrokePath(local->hdc);
    MoveToEx(local->hdc, local->x, local->y, NULL);
    break;
  default: 
    Arc(local->hdc,
	x - w, y - h,
	x + w, y + h,
	x + w * cos(a1),
	y - h * sin(a1),
	x + w * cos(a2),
	y - h * sin(a2)
	);
    break;
  }
}

static int
gra2emf_output(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char code, *cstr, *tmp, *fname;
  int *cpar, i, r;
  double x, y, w, h, fontsize,
    fontspace, fontdir, fontsin, fontcos;
  struct gra2emf_local *local;
  HRGN hrgn;
  HBRUSH brush, old_brush;
  DWORD *line_style;
  POINT lpoint[2];

  local = (struct gra2emf_local *)argv[2];
  code = *(char *)(argv[3]);
  cpar = (int *)argv[4];
  cstr = argv[5];

  switch (code) {
  case 'I':
    local->hdc = open_emf();
    break;
  case '%': case 'X':
    break;
  case 'E':
    if (local->hdc == NULL) {
      //	error2(obj, ERRFOPEN, fname);
    }
    _getobj(obj, "file", inst, &fname);
    if (fname) {
      fname = g_locale_from_utf8(fname, -1, NULL, NULL, NULL);
    }
    r = close_emf(local->hdc, fname);
    if (fname) {
      g_free(fname);
    }
    if (r) {
      //	error2(obj, ERRFOPEN, (fname) ? fname : "clipboard
      return 1;
    }
    break;
  case 'V':
    local->offsetx = cpar[1];
    local->offsety = cpar[2];
    SelectClipRgn(local->hdc, NULL);
    if (cpar[5]) {
      hrgn = CreateRectRgn(cpar[1], cpar[2], cpar[3], cpar[4]);
      SelectClipRgn(local->hdc, hrgn);
      DeleteObject(hrgn);
    }
    break;
  case 'A':
    if (local->line_style) {
      g_free(local->line_style);
    }
    local->line_style = NULL;
    local->line_style_num = 0;
    if (cpar[1]) {
      local->line_style = g_malloc(sizeof(* line_style) * cpar[1]);
      if (local->line_style == NULL) {
	break;
      }
      for (i = 0; i < cpar[1]; i++) {
	local->line_style[i] = cpar[6 + i];
      }
      local->line_style_num = cpar[1];
    }
    local->line_width = cpar[2];
    local->line_cap = cpar[3];
    local->line_join = cpar[4];
    create_pen(local);
    break;
  case 'G':
    local->r = cpar[1];
    local->g = cpar[2];
    local->b = cpar[3];
    brush = CreateSolidBrush(RGB(local->r, local->g, local->b));
    old_brush = SelectObject(local->hdc, brush);
    DeleteObject(old_brush);
    create_pen(local);
    break;
  case 'M':
    local->x = cpar[1] + local->offsetx;
    local->y = cpar[2] + local->offsety;
    MoveToEx(local->hdc, local->x, local->y, NULL);
    break;
  case 'N':
    local->x += cpar[1];
    local->y += cpar[2];
    MoveToEx(local->hdc, local->x, local->y, NULL);
    break;
  case 'L':
    lpoint[0].x = cpar[1] + local->offsetx;
    lpoint[0].y = cpar[2] + local->offsety;
    lpoint[1].x = cpar[3] + local->offsetx;
    lpoint[1].y = cpar[4] + local->offsety;
    Polyline(local->hdc, lpoint, 2);
 break;
  case 'T':
    local->x = cpar[1] + local->offsetx;
    local->y = cpar[2] + local->offsety;
    LineTo(local->hdc, local->x, local->y);
    break;
  case 'C':
    x = cpar[1] + local->offsetx;
    y = cpar[2] + local->offsety;
    w = cpar[3];
    h = cpar[4];
    draw_arc(local, x, y, w, h, cpar[5], cpar[6], cpar[7]);
    break;
  case 'B':
    BeginPath(local->hdc);
    MoveToEx(local->hdc, cpar[1] + local->offsetx, cpar[2] + local->offsety, NULL);
    LineTo(local->hdc,
	   cpar[1] + local->offsetx,
	   cpar[4] + local->offsety);
    LineTo(local->hdc,
	   cpar[3] + local->offsetx,
	   cpar[4] + local->offsety);
    LineTo(local->hdc,
	   cpar[3] + local->offsetx,
	   cpar[2] + local->offsety);
    CloseFigure(local->hdc);
    EndPath(local->hdc);
    if (cpar[5]) {
      FillPath(local->hdc);
    } else {
      StrokePath(local->hdc);
    }
    MoveToEx(local->hdc, local->x, local->y, NULL);
    break;
  case 'P':
    SetPixel(local->hdc,
	     cpar[1] + local->offsetx, cpar[2] + local->offsety,
	     RGB(local->r, local->g, local->b));
    break;
  case 'R':
    if (cpar[1] == 0)
      break;

    MoveToEx(local->hdc, cpar[2] + local->offsetx, cpar[3] + local->offsety, NULL);
    for (i = 1; i < cpar[1]; i++) {
      LineTo(local->hdc,
	     cpar[i * 2 + 0] + local->offsetx,
	     cpar[i * 2 + 2] + local->offsety);
    }
    MoveToEx(local->hdc, local->x, local->y, NULL);
    break;
  case 'D':
    if (cpar[1] == 0)
      break;

    BeginPath(local->hdc);
    MoveToEx(local->hdc, cpar[3] + local->offsetx, cpar[4] + local->offsety, NULL); 
    for (i = 1; i < cpar[1]; i++) {
      LineTo(local->hdc,
	     cpar[i * 2 + 1] + local->offsetx,
	     cpar[i * 2 + 2] + local->offsety);
    }
    CloseFigure(local->hdc);
    EndPath(local->hdc);
    switch (cpar[2]) {
    case 0:
      StrokePath(local->hdc);
      break;
    case 1:
      SetPolyFillMode(local->hdc, ALTERNATE);
      FillPath(local->hdc);
      break;
    case 2:
      SetPolyFillMode(local->hdc, WINDING);
      FillPath(local->hdc);
      break;
    }
    MoveToEx(local->hdc, local->x, local->y, NULL);
    break;
  case 'F':
    g_free(local->fontalias);
    local->fontalias = g_strdup(cstr);
    break;
  case 'H':
    fontspace = cpar[2] / 72.0 * 25.4;
    local->fontspace = fontspace;
    fontsize = cpar[1] / 72.0 * 25.4;
    local->fontsize = fontsize;
    fontdir = cpar[3] * MPI / 18000.0;
    fontsin = sin(fontdir);
    fontcos = cos(fontdir);
    local->fontdir = (cpar[3] % 36000) / 100.0;
    if (local->fontdir < 0) {
      local->fontdir += 360;
    }
    local->fontsin = fontsin;
    local->fontcos = fontcos;
    local->font_style = (cpar[0] > 3) ? cpar[4] : 0;
    break;
  case 'S':
    draw_str(local, argv[5]);
    break;
  case 'K':
    tmp = sjis_to_utf8(cstr);
    if (tmp) {
      draw_str(local, tmp);
      g_free(tmp);
    }
    break;
  default:
    break;
  }
  return 0;
}

  static struct objtable gra2emf[] = {
  {"init",    NVFUNC, NEXEC, gra2emf_init, NULL, 0},
  {"done",    NVFUNC, NEXEC, gra2emf_done, NULL, 0},
  {"next",    NPOINTER, 0, NULL, NULL, 0},
  {"file",    NSTR, NREAD | NWRITE, NULL, NULL,0},
  {"flush",   NVFUNC,NREAD|NEXEC, gra2emf_flush,"",0},
  {"_output", NVFUNC, 0, gra2emf_output, NULL, 0},
  {"_local",  NPOINTER, 0, NULL, NULL, 0},
};

#define TBLNUM (sizeof(gra2emf) / sizeof(*gra2emf))

void *
addgra2emf(void)
/* addgra2emfile() returns NULL on error */
{
  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, gra2emf, ERRNUM, gra2emf_errorlist, NULL, NULL);
}

#endif	/* WINDOWS */

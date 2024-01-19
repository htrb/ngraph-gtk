#include "common.h"

#if WINDOWS
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include "emfplus.h"

using namespace Gdiplus;

#define DPI 360.0
#define UNIT_FACTOR (2540.0 / DPI)
#define UNIT_CONV(v) ((v) / UNIT_FACTOR)
#define UNIT_RCONV(v) ((v) * UNIT_FACTOR)

struct gdiobj {
  Bitmap *bitmap;
  Graphics *bitmap_graphics, *graphics;
  HDC hdc;
  Metafile *metafile;
  Pen *pen;
  SolidBrush *brush;
  ULONG_PTR gdiplusToken;
  WCHAR *tmp_file;
};

static WCHAR *
get_temp_filename (void)
{
  DWORD dwRetVal = 0;
  UINT uRetVal   = 0;

  WCHAR *lpTempFileName;
  WCHAR lpTempPathBuffer[MAX_PATH + 1];

  dwRetVal = GetTempPathW(MAX_PATH, lpTempPathBuffer);
  if (dwRetVal > MAX_PATH || (dwRetVal == 0)) {
    return NULL;
  }

  lpTempFileName = new WCHAR[MAX_PATH + 1];
  if (lpTempFileName == NULL) {
    return NULL;
  }

  uRetVal = GetTempFileNameW(lpTempPathBuffer, // directory for tmp files
                             L"NGP",           // temp file name prefix
                             0,                // create unique name
                             lpTempFileName);  // buffer for name
  if (uRetVal == 0) {
    delete[] lpTempFileName;
    return NULL;
  }
  return lpTempFileName;
}

struct gdiobj *
emfplus_init (const wchar_t *filename)
{
  ULONG_PTR gdiplusToken;
  GdiplusStartupInput gdiplusStartupInput;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

  Bitmap *bitmap = new Bitmap(1, 1);
  Graphics *g = new Graphics(bitmap);
  HDC hDC = g->GetHDC();
  WCHAR *tmp_file;
  Metafile *metafile;
  if (filename == NULL) {
    tmp_file = get_temp_filename ();
    if (tmp_file == NULL) {
      delete g;
      delete bitmap;
      GdiplusShutdown(gdiplusToken);
      return NULL;
    }
    metafile = new Metafile(tmp_file, hDC, EmfTypeEmfPlusOnly, NULL);
  } else {
    tmp_file = NULL;
    metafile = new Metafile(filename, hDC, EmfTypeEmfPlusOnly, NULL);
  }
  Graphics *graphics = new Graphics(metafile);
  Pen *pen = new Pen(Color(255, 0, 0, 0));
  SolidBrush *brush = new SolidBrush(Color(255, 0, 0, 0));

  graphics->SetPageUnit (UnitPixel);
  graphics->SetSmoothingMode(SmoothingModeHighQuality);
  graphics->SetTextRenderingHint(TextRenderingHintAntiAlias);

  struct gdiobj *gdi = new struct gdiobj;

  gdi->bitmap = bitmap;
  gdi->bitmap_graphics = g;
  gdi->metafile = metafile;
  gdi->pen = pen;
  gdi->brush = brush;
  gdi->graphics = graphics;
  gdi->hdc = hDC;
  gdi->gdiplusToken = gdiplusToken;
  gdi->tmp_file = tmp_file;

  return gdi;
}

void
emfplus_flush (struct gdiobj *gdi)
{
  gdi->graphics->Flush(FlushIntentionSync);
}

static void
set_clipboard (const WCHAR *file)
{
  Metafile metaFile (file);
  HENHMETAFILE hMetaFile = metaFile.GetHENHMETAFILE();
  if (OpenClipboard(NULL)) {
    EmptyClipboard();
    SetClipboardData(CF_ENHMETAFILE, hMetaFile);
    CloseClipboard();
  }
}

void
emfplus_finalize (struct gdiobj *gdi)
{
  if (gdi == NULL) {
    return;
  }

  gdi->graphics->Flush(FlushIntentionSync);
  delete gdi->brush;
  delete gdi->pen;
  delete gdi->graphics;
  delete gdi->metafile;
  delete gdi->bitmap_graphics;
  delete gdi->bitmap;
  if (gdi->tmp_file) {
    set_clipboard (gdi->tmp_file);
    DeleteFileW (gdi->tmp_file);
    delete[] gdi->tmp_file;
  }
  GdiplusShutdown(gdi->gdiplusToken);
  delete gdi;
}

void
emfplus_line (struct gdiobj *gdi, int ix1, int iy1, int ix2, int iy2)
{
  REAL x1, x2, y1, y2;
  x1 = UNIT_CONV (ix1);
  x2 = UNIT_CONV (ix2);
  y1 = UNIT_CONV (iy1);
  y2 = UNIT_CONV (iy2);
  gdi->graphics->DrawLine(gdi->pen, x1, y1, x2, y2);
}

// Surrogate pairs, combined strings and ligatures are not considered.
static void
draw_test (struct gdiobj *gdi, PointF &pointF, Font *font, const StringFormat* pStringFormat, int ispace, const wchar_t *text)
{
  double space;

  space =  UNIT_CONV (ispace / 72.0 * 25.4);
  for (int i = 0; text[i]; i++) {
    RectF boundingBox;
    gdi->graphics->MeasureString (text + i, 1, font, pointF, pStringFormat, &boundingBox);
    gdi->graphics->DrawString (text + i, 1, font, pointF, pStringFormat, gdi->brush);
    pointF.X = boundingBox.GetRight();
    if (text[i + 1]) {
      pointF.X += space;
    }
  }
}

void
emfplus_text (struct gdiobj *gdi, int *px, int *py, struct font_info *fontinfo, const wchar_t *text)
{
  LOGFONTW id_font;
  PointF pointF (0, 0);
  FontFamily fontfamily (fontinfo->font);
  Font font (&fontfamily, fontinfo->size / 100.0 / 72.0 * DPI, fontinfo->style, UnitPixel);
  RectF boundingBox;
  const StringFormat* pStringFormat = StringFormat::GenericTypographic();
  double x, y, w, h, a;

  gdi->graphics->MeasureString (text, -1, &font, pointF, pStringFormat, &boundingBox);
  w = boundingBox.GetRight () - boundingBox.GetLeft ();
  h = boundingBox.GetTop () - boundingBox.GetBottom ();
  int descent, ascent;
  descent = fontfamily.GetCellDescent (fontinfo->style);
  ascent = fontfamily.GetCellAscent (fontinfo->style);
  x = UNIT_CONV (*px);
  y = UNIT_CONV (*py);

  GraphicsState state = gdi->graphics->Save();
  gdi->graphics->TranslateTransform (x, y);
  gdi->graphics->RotateTransform (-fontinfo->dir / 100.0);
  pointF.Y = h * ascent / (ascent + descent);
  if (fontinfo->space) {
    draw_test (gdi, pointF, &font, pStringFormat, fontinfo->space, text);
    w = pointF.X;
  } else {
    gdi->graphics->DrawString (text, -1, &font, pointF, pStringFormat, gdi->brush);
  }
  gdi->graphics->Restore(state);
  a = M_PI * fontinfo->dir / 18000.0;
  x += w * cos (a);
  y -= w * sin (a);
  *px = UNIT_RCONV (x);
  *py = UNIT_RCONV (y);
}

void
emfplus_line_attribte (struct gdiobj *gdi, int width, int cap, int join, int miter, int n, int *style)
{
  LineCap endCap;
  LineJoin lineJoin;

  switch (cap) {
  case 0:
    endCap = LineCapFlat;
    break;
  case 1:
    endCap = LineCapRound;
    break;
  case 2:
    endCap = LineCapTriangle;
    break;
  }
  switch (join) {
  case 0:
    lineJoin = LineJoinMiterClipped;
    break;
  case 1:
    lineJoin = LineJoinRound;
    break;
  case 2:
    lineJoin = LineJoinBevel;
    break;
  }

  gdi->pen->SetWidth (UNIT_CONV (width));
  gdi->pen->SetEndCap (endCap);
  gdi->pen->SetStartCap (endCap);
  gdi->pen->SetLineJoin (lineJoin);
  gdi->pen->SetMiterLimit (miter / 100.0);
  if (n == 0) {
    gdi->pen->SetDashStyle (DashStyleSolid);
  } else {
    REAL dash[n];
    for (int i = 0; i < n; i++) {
      dash[i] = style[i] * 1.0 / width;
    }
    gdi->pen->SetDashStyle (DashStyleCustom);
    gdi->pen->SetDashPattern (dash, n);
  }
}

void
emfplus_color (struct gdiobj *gdi, int r, int g, int b, int a)
{
  Color color (a, r, g, b);
  gdi->pen->SetColor (color);
  gdi->brush->SetColor (color);
}

void
emfplus_rectangle (struct gdiobj *gdi, int x1, int y1, int x2, int y2, int fill)
{
  REAL x, y, w, h;
  x = UNIT_CONV ((x1 < x2) ? x1 : x2);
  y = UNIT_CONV ((y1 < y2) ? y1 : y2);
  w = UNIT_CONV (abs (x2 - x1));
  h = UNIT_CONV (abs (y2 - y1));
  if (fill) {
    gdi->graphics->FillRectangle (gdi->brush, x, y, w, h);
  } else {
    LineJoin lineJoin;
    lineJoin = gdi->pen->GetLineJoin ();
    gdi->pen->SetLineJoin (LineJoinMiter);
    gdi->graphics->DrawRectangle (gdi->pen, x, y, w, h);
    gdi->pen->SetLineJoin (lineJoin);
  }
}

void
emfplus_polygon (struct gdiobj *gdi, int n, const int *xy, int fill)
{
  PointF points[n];
  for (int i = 0; i < n; i++) {
    points[i].X = UNIT_CONV (xy[i * 2]);
    points[i].Y = UNIT_CONV (xy[i * 2 + 1]);
  }
  switch (fill) {
  case 0:
    gdi->graphics->DrawPolygon (gdi->pen, points, n);
    break;
  case 1:
    gdi->graphics->FillPolygon (gdi->brush, points, n, FillModeAlternate);
    break;
  case 2:
    gdi->graphics->FillPolygon (gdi->brush, points, n, FillModeWinding);
    break;
  }
}

void
emfplus_lines (struct gdiobj *gdi, int n, const int *xy)
{
  PointF points[n];
  for (int i = 0; i < n; i++) {
    points[i].X = UNIT_CONV (xy[i * 2]);
    points[i].Y = UNIT_CONV (xy[i * 2 + 1]);
  }
  gdi->graphics->DrawLines (gdi->pen, points, n);
}

void
emfplus_arc (struct gdiobj *gdi, int ix, int iy, int iw, int ih, int start, int angle, int style)
{
  REAL a1, a2, x, y, w, h;
  GraphicsPath path;

  if (angle == 0) {
    return;
  }

  x = UNIT_CONV (ix - iw);
  y = UNIT_CONV (iy - ih);
  h = UNIT_CONV (ih) * 2.0;
  w = UNIT_CONV (iw) * 2.0;
  if (angle % 36000 == 0) {
    if (style == 1 || style == 2) {
      gdi->graphics->FillEllipse(gdi->brush, x, y, w, h);
    } else {
      gdi->graphics->DrawEllipse(gdi->pen, x, y, w, h);
    }
    return;
  }

  a1 = - start / 100.0;
  a2 = - angle / 100.0;

  switch (style) {
  case 1:
    gdi->graphics->FillPie (gdi->brush, x, y, w, h, a1, a2);
    break;
  case 2:
    path.AddArc (x, y, w, h, a1, a2);
    path.CloseFigure ();
    gdi->graphics->FillPath (gdi->brush, &path);
    break;
  case 3:
    gdi->graphics->DrawPie (gdi->pen, x, y, w, h, a1, a2);
    break;
  case 4:
    path.AddArc (x, y, w, h, a1, a2);
    path.CloseFigure ();
    gdi->graphics->DrawPath (gdi->pen, &path);
    break;
  default:
    gdi->graphics->DrawArc (gdi->pen, x, y, w, h, a1, a2);
    break;
  }
}

void
emfplus_clip (struct gdiobj *gdi, int x1, int y1, int x2, int y2)
{
  double x, y, w, h;
  gdi->graphics->ResetTransform ();
  x = UNIT_CONV ((x1 < x2) ? x1 : x2);
  y = UNIT_CONV ((y1 < y2) ? y1 : y2);
  w = UNIT_CONV (abs (x2 - x1));
  h = UNIT_CONV (abs (y2 - y1));
  gdi->graphics->ResetClip ();
  gdi->graphics->SetClip (RectF(x, y, w, h), CombineModeReplace);
  gdi->graphics->TranslateTransform (x, y);
}
#endif

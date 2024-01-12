#include "common.h"

#if WINDOWS
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include "emfplus.h"

using namespace Gdiplus;

struct gdiobj {
  Bitmap *bitmap;
  Graphics *bitmap_graphics, *graphics;
  HDC hdc;
  Metafile *metafile;
  Pen *pen;
  SolidBrush *brush;
  double scale;
  ULONG_PTR gdiplusToken;
  GraphicsState state;
};

#define EMF_PAGE_SCALE 0.579
struct gdiobj *
emfplus_init (const wchar_t *filename, int margin_left, int margin_top, int width, int height, int iscale)
{
  double scale;
  ULONG_PTR gdiplusToken;
  GdiplusStartupInput gdiplusStartupInput;
  GdiplusStartup(&gdiplusToken, &gdiplusStartupInput, NULL);

  Bitmap *bitmap = new Bitmap(1, 1);
  Graphics *g = new Graphics(bitmap);
  HDC hDC = g->GetHDC();
  RectF rect(0, 0, width * EMF_PAGE_SCALE / 100.0, height * EMF_PAGE_SCALE / 100.0);
  Metafile *metafile = new Metafile(filename, hDC, rect, MetafileFrameUnitMillimeter, EmfTypeEmfPlusOnly, NULL);
  Graphics *graphics = new Graphics(metafile);
  Pen *pen = new Pen(Color(255, 0, 0, 0));
  SolidBrush *brush = new SolidBrush(Color(255, 0, 0, 0));

  graphics->SetPageUnit (UnitMillimeter);
  graphics->SetSmoothingMode(SmoothingModeHighQuality);
  graphics->SetTextRenderingHint(TextRenderingHintAntiAlias);

  graphics->TranslateTransform (margin_left / 100.0, margin_top / 100.0);
  scale = iscale / 10000.0;
  graphics->ScaleTransform (scale, scale);

  struct gdiobj *gdi = new struct gdiobj;

  gdi->bitmap = bitmap;
  gdi->bitmap_graphics = g;
  gdi->metafile = metafile;
  gdi->pen = pen;
  gdi->brush = brush;
  gdi->graphics = graphics;
  gdi->hdc = hDC;
  gdi->gdiplusToken = gdiplusToken;
  gdi->scale = scale;
  gdi->state = graphics->Save();

  return gdi;
}

void
emfplus_flush (struct gdiobj *gdi)
{
  gdi->graphics->Flush(FlushIntentionSync);
}

void
emfplus_finalize (struct gdiobj *gdi)
{
  if (gdi == NULL) {
    return;
  }

  gdi->graphics->Flush(FlushIntentionSync);
  gdi->graphics->Restore(gdi->state);
  delete gdi->brush;
  delete gdi->pen;
  delete gdi->graphics;
  delete gdi->metafile;
  delete gdi->bitmap_graphics;
  delete gdi->bitmap;
  GdiplusShutdown(gdi->gdiplusToken);
  delete gdi;
}

void
emfplus_line (struct gdiobj *gdi, int ix1, int iy1, int ix2, int iy2)
{
  REAL x1, x2, y1, y2;
  x1 = ix1 / 100.0;
  x2 = ix2 / 100.0;
  y1 = iy1 / 100.0;
  y2 = iy2 / 100.0;
  gdi->graphics->DrawLine(gdi->pen, x1, y1, x2, y2);
}

void
emfplus_text (struct gdiobj *gdi, int *px, int *py, struct font_info *fontinfo, const wchar_t *text)
{
  LOGFONTW id_font;
  PointF pointF (0, 0);
  FontFamily fontfamily (fontinfo->font);
  Font font (&fontfamily, fontinfo->size / 100.0, fontinfo->style, UnitPoint);
  RectF boundingBox;
  const StringFormat* pStringFormat = StringFormat::GenericTypographic();
  double x, y, w, h, a;

  gdi->graphics->MeasureString (text, -1, &font, pointF, pStringFormat, &boundingBox);
  w = boundingBox.GetLeft () - boundingBox.GetRight ();
  h = boundingBox.GetTop () - boundingBox.GetBottom ();
  int descent, ascent;
  descent = fontfamily.GetCellDescent (fontinfo->style);
  ascent = fontfamily.GetCellAscent (fontinfo->style);
  x = *px / 100.0;
  y = *py / 100.0;

  GraphicsState state = gdi->graphics->Save();
  gdi->graphics->TranslateTransform (x, y);
  gdi->graphics->RotateTransform (-fontinfo->dir / 100.0);
  pointF.Y = h * ascent / (ascent + descent);
  gdi->graphics->DrawString (text, -1, &font, pointF, pStringFormat, gdi->brush);
  gdi->graphics->Restore(state);
  a = M_PI * fontinfo->dir / 18000.0;
  x -= w * cos (a);
  y += w * sin (a);
  *px = x * 100;
  *py = y * 100;
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

  gdi->pen->SetWidth (width / 100.0);
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
  x = ((x1 < x2) ? x1 : x2) / 100.0;
  y = ((y1 < y2) ? y1 : y2) / 100.0;
  w = abs (x2 - x1) / 100.0;
  h = abs (y2 - y1) / 100.0;
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
    points[i].X = xy[i * 2] / 100.0;
    points[i].Y = xy[i * 2 + 1] / 100.0;
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
    points[i].X = xy[i * 2] / 100.0;
    points[i].Y = xy[i * 2 + 1] / 100.0;
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

  x = (ix - iw) / 100.0;
  y = (iy - ih) / 100.0;
  h = ih / 50.0;
  w = iw / 50.0;
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
  gdi->graphics->Restore(gdi->state);
  gdi->state = gdi->graphics->Save();
  x = ((x1 < x2) ? x1 : x2) / 100.0;
  y = ((y1 < y2) ? y1 : y2) / 100.0;
  w = abs (x2 - x1) / 100.0;
  h = abs (y2 - y1) / 100.0;
  gdi->graphics->ResetClip ();
  gdi->graphics->SetClip (RectF(x, y, w, h), CombineModeReplace);
  gdi->graphics->TranslateTransform (x, y);
}
#endif

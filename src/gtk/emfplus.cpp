#include "common.h"

#if WINDOWS
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <dwrite.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <string>
#include <vector>

#pragma comment(lib, "gdiplus.lib")
#pragma comment(lib, "dwrite.lib")

using Microsoft::WRL::ComPtr;

#include "emfplus.h"

using namespace Gdiplus;

#define DPI 360.0
#define UNIT_FACTOR (2540.0 / DPI)
#define UNIT_CONV(v) ((v) / UNIT_FACTOR)
#define UNIT_RCONV(v) ((v) * UNIT_FACTOR)

struct gdiobj {
  Bitmap *bitmap;
  Graphics *bitmap_graphics, *graphics;
  Metafile *metafile;
  Pen *pen;
  SolidBrush *brush;
  ULONG_PTR gdiplusToken;
  WCHAR *tmp_file;
  ComPtr<IDWriteFactory> dwriteFactory;
  ComPtr<IDWriteFontCollection> sysFontCollection;
  bool dwriteReady;
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
      g->ReleaseHDC(hDC);
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
  g->ReleaseHDC(hDC);
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
  gdi->gdiplusToken = gdiplusToken;
  gdi->tmp_file = tmp_file;

  gdi->dwriteReady = SUCCEEDED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), reinterpret_cast<IUnknown**>(gdi->dwriteFactory.GetAddressOf())));
  if (gdi->dwriteReady) {
    gdi->dwriteReady = SUCCEEDED(gdi->dwriteFactory->GetSystemFontCollection(&gdi->sysFontCollection));
  }

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
  ULONG_PTR gdiplusToken = gdi->gdiplusToken;
  delete gdi;
  GdiplusShutdown(gdiplusToken);
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

// ヘルパー: UTF-8 を UTF-16 (std::wstring) に変換
std::wstring Utf8ToUtf16(const char* utf8Str) {
  if (!utf8Str || *utf8Str == '\0') return L"";
  int len = MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, nullptr, 0);
  if (len <= 0) return L"";
  std::wstring wstr(len - 1, L'\0');
  MultiByteToWideChar(CP_UTF8, 0, utf8Str, -1, &wstr[0], len);
  return wstr;
}

// ヘルパー: UTF-16 文字列からサロゲートペアを考慮して Unicode コードポイント(UTF-32)配列を抽出
std::vector<UINT32> ExtractCodePoints(const std::wstring& utf16Text) {
  std::vector<UINT32> codePoints;
  codePoints.reserve(utf16Text.length());

  for (size_t i = 0; i < utf16Text.length(); ++i) {
    wchar_t ch = utf16Text[i];
    // 上位サロゲート (0xD800 - 0xDBFF) のチェック
    if (ch >= 0xD800 && ch <= 0xDBFF && (i + 1) < utf16Text.length()) {
      wchar_t low = utf16Text[i + 1];
      // 下位サロゲート (0xDC00 - 0xDFFF) との合成
      if (low >= 0xDC00 && low <= 0xDFFF) {
	UINT32 cp = 0x10000 + (((static_cast<UINT32>(ch) - 0xD800) << 10) | (static_cast<UINT32>(low) - 0xDC00));
	codePoints.push_back(cp);
	++i; // 2要素消費したので進める
	continue;
      }
    }
    codePoints.push_back(static_cast<UINT32>(ch));
  }
  return codePoints;
}

// DirectWrite を用いて指定フォントがすべてのコードポイントを収録しているか判定
bool CanFontRenderAllCodePoints(
				IDWriteFontCollection* fontCollection,
				const std::wstring& fontFamilyName,
				DWRITE_FONT_WEIGHT weight,
				DWRITE_FONT_STYLE style,
				const std::vector<UINT32>& codePoints)
{
  if (!fontCollection || fontFamilyName.empty()) return false;

  UINT32 index = 0;
  BOOL exists = FALSE;
  if (FAILED(fontCollection->FindFamilyName(fontFamilyName.c_str(), &index, &exists)) || !exists) {
    return false; // フォント自体がシステムに存在しない
  }

  ComPtr<IDWriteFontFamily> family;
  if (FAILED(fontCollection->GetFontFamily(index, &family))) return false;

  ComPtr<IDWriteFont> font;
  if (FAILED(family->GetFirstMatchingFont(weight, DWRITE_FONT_STRETCH_NORMAL, style, &font))) {
    return false;
  }

  // 1文字ずつグリフが存在するかチェック
  for (UINT32 cp : codePoints) {
    // 制御文字（改行やタブ等）はグリフチェックから除外
    if (cp < 0x20) continue;

    BOOL hasCharacter = FALSE;
    if (FAILED(font->HasCharacter(cp, &hasCharacter)) || !hasCharacter) {
      return false; // 1文字でも未収録があれば false
    }
  }
  return true;
}

// GDI+ の FontStyle を DirectWrite の Weight / Style に変換
void ConvertGdiPlusStyleToDWrite(Gdiplus::FontStyle gdiStyle, DWRITE_FONT_WEIGHT& weight, DWRITE_FONT_STYLE& style) {
  weight = (gdiStyle & Gdiplus::FontStyleBold) ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
  style = (gdiStyle & Gdiplus::FontStyleItalic) ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;
}

/**
 * 文字列描画関数
 * 
 * @param graphics          GDI+ Graphics オブジェクト
 * @param textUtf16         描画する UTF-16 文字列
 * @param primaryFontName   第1候補フォント名 (UTF-16)
 * @param fallbackFontsUtf8 NULL終端された代替フォント名(UTF-8)の配列 (例: {"Segoe UI Emoji", "Yu Gothic UI", nullptr})
 * @param fontSize          フォントサイズ
 * @param fontStyle         GDI+ フォントスタイル (FontStyleRegular 等)
 * @param brush             描画ブラシ
 * @param layoutRect        描画領域
 * @param stringFormat      文字列フォーマット (省略可)
 */
static void
DrawStringWithFallback(
			    struct gdiobj *gdi,
			    Gdiplus::Graphics *graphics,
			    const std::wstring& textUtf16,
			    const std::wstring& primaryFontName,
			    const char* const* fallbackFontsUtf8,
			    Gdiplus::REAL fontSize,
			    Gdiplus::FontStyle fontStyle,
			    const Gdiplus::Brush* brush,
			    const Gdiplus::RectF& layoutRect,
			    const Gdiplus::StringFormat* stringFormat = nullptr)
{
  if (textUtf16.empty() || !brush) return;

  std::wstring selectedFontName = primaryFontName;

  if (gdi->dwriteReady && gdi->sysFontCollection) {
    std::vector<UINT32> codePoints = ExtractCodePoints(textUtf16);
    DWRITE_FONT_WEIGHT dwWeight;
    DWRITE_FONT_STYLE dwStyle;
    ConvertGdiPlusStyleToDWrite(fontStyle, dwWeight, dwStyle);

    // 2. 第1候補フォントで全文字描画可能かチェック
    if (!CanFontRenderAllCodePoints(gdi->sysFontCollection.Get(), primaryFontName, dwWeight, dwStyle, codePoints)) {
      // 3. 不可の場合、代替フォントリスト(UTF-8, NULL終端配列)を順にチェック
      if (fallbackFontsUtf8) {
	for (size_t i = 0; fallbackFontsUtf8[i] != nullptr; ++i) {
	  std::wstring candidate = Utf8ToUtf16(fallbackFontsUtf8[i]);
	  if (CanFontRenderAllCodePoints(gdi->sysFontCollection.Get(), candidate, dwWeight, dwStyle, codePoints)) {
	    selectedFontName = candidate;
	    break; // 描画可能なフォントが見つかったら確定
	  }
	}
      }
      // 4. すべて不可だった場合は selectedFontName == primaryFontName のまま進み、OSのフォールバックに任せる
    }
  }

  // 5. 決定したフォントで GDI+ 描画を実行
  Gdiplus::FontFamily family(selectedFontName.c_str());
  Gdiplus::Font font(&family, fontSize, fontStyle, Gdiplus::UnitPixel);

  // 万が一 primaryFontName すらシステムに存在しない場合は、GenericSansSerif でOSに任せる
  if (font.GetLastStatus() != Gdiplus::Ok) {
    Gdiplus::Font fallbackGdiFont(Gdiplus::FontFamily::GenericSansSerif(), fontSize, fontStyle, Gdiplus::UnitPixel);
    graphics->DrawString(textUtf16.c_str(), -1, &fallbackGdiFont, layoutRect, stringFormat, brush);
  } else {
    graphics->DrawString(textUtf16.c_str(), -1, &font, layoutRect, stringFormat, brush);
  }
}

static void
draw_test (struct gdiobj *gdi, struct font_info *fontinfo, PointF &pointF, Font *font, const StringFormat* pStringFormat, int ispace, const wchar_t *text)
{
  double space;

  space =  UNIT_CONV (ispace / 72.0 * 25.4);
  for (int i = 0; text[i]; i++) {
    RectF boundingBox;
    gdi->graphics->MeasureString (text + i, 1, font, pointF, pStringFormat, &boundingBox);
    DrawStringWithFallback(gdi, gdi->graphics, text, fontinfo->font, fontinfo->alternative, fontinfo->size / 100.0 / 72.0 * DPI, (Gdiplus::FontStyle)fontinfo->style, gdi->brush, boundingBox);
    pointF.X = boundingBox.GetRight();
    if (text[i + 1]) {
      pointF.X += space;
    }
  }
}

void
emfplus_text (struct gdiobj *gdi, int *px, int *py, struct font_info *fontinfo, const wchar_t *text)
{
  PointF pointF (0, 0);
  FontFamily fontfamily (fontinfo->font);
  REAL fontSize = (REAL)(fontinfo->size / 100.0 / 72.0 * DPI);
  Font font (&fontfamily, fontSize, fontinfo->style, UnitPixel);
  RectF boundingBox;
  const StringFormat* pStringFormat = StringFormat::GenericTypographic();
  double x, y, w, h, a;

  gdi->graphics->MeasureString (text, -1, &font, pointF, pStringFormat, &boundingBox);
  w = boundingBox.GetRight () - boundingBox.GetLeft ();
  h = boundingBox.GetTop () - boundingBox.GetBottom ();

  int descent = fontfamily.GetCellDescent (fontinfo->style);
  int ascent = fontfamily.GetCellAscent (fontinfo->style);

  x = UNIT_CONV (*px);
  y = UNIT_CONV (*py);

  GraphicsState state = gdi->graphics->Save();
  gdi->graphics->TranslateTransform (x, y);
  gdi->graphics->RotateTransform (-fontinfo->dir / 100.0);
  pointF.Y = (REAL)(h * ascent / (ascent + descent));
  if (fontinfo->space) {
    draw_test (gdi, fontinfo, pointF, &font, pStringFormat, fontinfo->space, text);
    w = pointF.X;
  } else {
    DrawStringWithFallback(gdi, gdi->graphics, text, fontinfo->font, fontinfo->alternative, fontSize, (Gdiplus::FontStyle)fontinfo->style, gdi->brush, boundingBox);
  }
  gdi->graphics->Restore(state);

  a = M_PI * fontinfo->dir / 18000.0;
  x += w * cos (a);
  y -= w * sin (a);
  *px = UNIT_RCONV (x);
  *py = UNIT_RCONV (y);
}

void
emfplus_line_attribte (struct gdiobj *gdi, int width, int cap, int join, int miter, int n, const int *style)
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

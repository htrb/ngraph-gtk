#include "common.h"

#if WINDOWS
#include <windows.h>
#include <objidl.h>
#include <gdiplus.h>
#include <dwrite.h>
#include <wrl/client.h> // Microsoft::WRL::ComPtr
#include <string>
#include <vector>
#include <pango/pango.h>
#include <glib.h>
#include <cstring>

#include "emfplus.h"

using namespace Gdiplus;
using Microsoft::WRL::ComPtr;

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


// 1つの表示単位（書記素クラスター）
struct GraphemeCluster {
  std::wstring utf16;   // GDI+描画用のUTF-16文字列（例: L"ที่", L"👨‍👩‍👧‍👦"）
  UINT32 primaryCp;     // DirectWriteフォント判定用の代表コードポイント
};

// 同じフォントが連続する描画単位（ラン）
struct TextRun {
  std::wstring fontName;               // 適用するフォント名(UTF-16)
  std::vector<GraphemeCluster> items;  // このフォントで描画するクラスター群
};

// ヘルパー: UTF-8 文字列を UTF-16 (std::wstring) に変換
static std::wstring
Utf8ToUtf16(const char* utf8Str, gssize len = -1) {
  if (!utf8Str || (len == 0) || (len == -1 && *utf8Str == '\0')) return L"";
  glong u16Len = 0;
  gunichar2* u16 = g_utf8_to_utf16(utf8Str, len, nullptr, &u16Len, nullptr);
  if (!u16) return L"";
  std::wstring result(reinterpret_cast<wchar_t*>(u16), u16Len);
  g_free(u16);
  return result;
}

// 1. UTF-8 文字列を Pango を使って書記素クラスター単位に分解する
static std::vector<GraphemeCluster>
SplitUtf8IntoGraphemes(const char* textUtf8) {
  std::vector<GraphemeCluster> clusters;
  if (!textUtf8 || *textUtf8 == '\0') return clusters;

  glong byteLen = static_cast<glong>(std::strlen(textUtf8));
  glong numChars = g_utf8_strlen(textUtf8, byteLen);
  if (numChars <= 0) return clusters;

  // Pango で文字列全体の論理属性 (PangoLogAttr) を一括取得
  std::vector<PangoLogAttr> attrs(numChars + 1);
  pango_get_log_attrs(textUtf8, static_cast<int>(byteLen), -1, nullptr, attrs.data(), static_cast<int>(attrs.size()));

  const gchar* p = textUtf8;
  const gchar* clusterStart = textUtf8;
  gunichar currentPrimaryCp = 0;

  for (glong i = 0; i < numChars; ++i) {
    const gchar* next = g_utf8_next_char(p);
    gunichar cp = g_utf8_get_char(p);

    // 新しい書記素クラスターの開始位置（is_cursor_position == TRUE）で区切る
    if (i > 0 && attrs[i].is_cursor_position) {
      clusters.push_back({
	  Utf8ToUtf16(clusterStart, p - clusterStart),
	  static_cast<UINT32>(currentPrimaryCp)
	});
      clusterStart = p;
      currentPrimaryCp = 0;
    }

    if (currentPrimaryCp == 0) {
      currentPrimaryCp = cp;
    }
    p = next;
  }

  // 最後のクラスターを追加
  if (clusterStart < p) {
    clusters.push_back({
	Utf8ToUtf16(clusterStart, p - clusterStart),
	static_cast<UINT32>(currentPrimaryCp)
      });
  }

  return clusters;
}

// 2. 1つの代表コードポイントに対して最適なフォント名を解決する
static std::wstring
ResolveFontForCodePoint(
				     IDWriteFontCollection* fontCollection,
				     const std::wstring& primaryFontName,
				     const std::vector<std::wstring>& fallbackFonts,
				     DWRITE_FONT_WEIGHT weight,
				     DWRITE_FONT_STYLE style,
				     UINT32 cp)
{
  if (cp <= 0x20 || !fontCollection) {
    return primaryFontName;
  }

  auto hasGlyph = [&](const std::wstring& fontName) -> bool {
    if (fontName.empty()) return false;
    UINT32 index = 0;
    BOOL exists = FALSE;
    if (FAILED(fontCollection->FindFamilyName(fontName.c_str(), &index, &exists)) || !exists) {
      return false;
    }
    ComPtr<IDWriteFontFamily> family;
    if (FAILED(fontCollection->GetFontFamily(index, &family))) return false;

    ComPtr<IDWriteFont> font;
    if (FAILED(family->GetFirstMatchingFont(weight, DWRITE_FONT_STRETCH_NORMAL, style, &font))) {
      return false;
    }
    BOOL hasChar = FALSE;
    return SUCCEEDED(font->HasCharacter(cp, &hasChar)) && hasChar;
  };

  if (hasGlyph(primaryFontName)) return primaryFontName;

  for (const auto& fbFont : fallbackFonts) {
    if (hasGlyph(fbFont)) return fbFont;
  }

  return primaryFontName;
}

// フォントのアセント(px)を計算
static Gdiplus::REAL
GetFontAscentPx(const Gdiplus::FontFamily& family, Gdiplus::FontStyle style, Gdiplus::REAL fontSize) {
  UINT16 emHeight = family.GetEmHeight(style);
  if (emHeight == 0) return 0.0f;
  UINT16 cellAscent = family.GetCellAscent(style);
  return fontSize * (static_cast<Gdiplus::REAL>(cellAscent) / static_cast<Gdiplus::REAL>(emHeight));
}

/**
 * UTF-8文字列を直接受け取り、書記素クラスター単位のフォント解決・ベースライン揃え・文字間隔調整を行う描画関数
 * 
 * @param gdi               初期化済みの gdiobj 構造体
 * @param textUtf8          描画する UTF-8 文字列 (NULL終端)
 * @param primaryFontUtf8   第1候補フォント名 (UTF-8, NULL終端)
 * @param fallbackFontsUtf8 NULL終端された代替フォント名(UTF-8)の配列
 * @param fontSize          フォントサイズ (px)
 * @param fontStyle         GDI+ フォントスタイル
 * @param startPoint        描画開始座標 (左上)
 * @param charSpacingPx     文字間隔の追加スペース (px)。0.0f で標準間隔
 */
static void
emfplus_draw_string_utf8(
			 struct gdiobj* gdi,
			 const char* textUtf8,
			 const char* primaryFontUtf8,
			 const char* const* fallbackFontsUtf8,
			 Gdiplus::REAL fontSize,
			 Gdiplus::FontStyle fontStyle,
			 const Gdiplus::PointF& startPoint,
			 Gdiplus::REAL charSpacingPx = 0.0f)
{
  if (!gdi || !gdi->graphics || !gdi->brush || !textUtf8 || *textUtf8 == '\0') return;

  // 1. Pango で UTF-8 文字列を書記素クラスターに分解
  std::vector<GraphemeCluster> clusters = SplitUtf8IntoGraphemes(textUtf8);
  if (clusters.empty()) return;

  // 2. フォント名を UTF-16 に変換
  std::wstring primaryFontName = Utf8ToUtf16(primaryFontUtf8);
  std::vector<std::wstring> fallbackFonts;
  if (fallbackFontsUtf8) {
    for (size_t i = 0; fallbackFontsUtf8[i] != nullptr; ++i) {
      std::wstring fb = Utf8ToUtf16(fallbackFontsUtf8[i]);
      if (!fb.empty()) {
	fallbackFonts.push_back(std::move(fb));
      }
    }
  }

  DWRITE_FONT_WEIGHT dwWeight = (fontStyle & Gdiplus::FontStyleBold) ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
  DWRITE_FONT_STYLE dwStyle = (fontStyle & Gdiplus::FontStyleItalic) ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;

  // 3. クラスターごとにフォントを解決し、同じフォントが連続するランにまとめる
  std::vector<TextRun> runs;
  IDWriteFontCollection* fontCollection = gdi->dwriteReady ? gdi->sysFontCollection.Get() : nullptr;

  for (auto& cl : clusters) {
    std::wstring targetFont = ResolveFontForCodePoint(
						      fontCollection, primaryFontName, fallbackFonts, dwWeight, dwStyle, cl.primaryCp
						      );
    if (!runs.empty() && runs.back().fontName == targetFont) {
      runs.back().items.push_back(std::move(cl));
    } else {
      TextRun newRun;
      newRun.fontName = std::move(targetFont);
      newRun.items.push_back(std::move(cl));
      runs.push_back(std::move(newRun));
    }
  }

  // 4. 計測・描画用の StringFormat を準備
  Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
  INT flags = format.GetFormatFlags();
  flags |= Gdiplus::StringFormatFlagsMeasureTrailingSpaces;
  flags |= Gdiplus::StringFormatFlagsNoClip;
  flags &= ~Gdiplus::StringFormatFlagsLineLimit;
  format.SetFormatFlags(flags);

  // 5. 基準となる第1候補フォントのアセントを算出
  Gdiplus::FontFamily primaryFamily(primaryFontName.c_str());
  Gdiplus::REAL baseAscent = (primaryFamily.GetLastStatus() == Gdiplus::Ok)
    ? GetFontAscentPx(primaryFamily, fontStyle, fontSize)
    : GetFontAscentPx(*Gdiplus::FontFamily::GenericSansSerif(), fontStyle, fontSize);

  Gdiplus::REAL currentX = startPoint.X;

  // 6. ランごとにベースラインを揃えながら描画
  for (const auto& run : runs) {
    Gdiplus::FontFamily family(run.fontName.c_str());
    Gdiplus::Font font(&family, fontSize, fontStyle, Gdiplus::UnitPixel);

    const Gdiplus::Font* activeFont = &font;
    const Gdiplus::FontFamily* activeFamily = &family;

    Gdiplus::Font fallbackGdiFont(Gdiplus::FontFamily::GenericSansSerif(), fontSize, fontStyle, Gdiplus::UnitPixel);
    if (font.GetLastStatus() != Gdiplus::Ok) {
      activeFont = &fallbackGdiFont;
      activeFamily = Gdiplus::FontFamily::GenericSansSerif();
    }

    Gdiplus::REAL runAscent = GetFontAscentPx(*activeFamily, fontStyle, fontSize);
    Gdiplus::REAL adjustedY = startPoint.Y + (baseAscent - runAscent);

    if (charSpacingPx == 0.0f) {
      // 【標準モード】ラン内のクラスターを結合して一括描画（高速・合字やカーニングも維持）
      std::wstring combinedText;
      for (const auto& item : run.items) {
	combinedText += item.utf16;
      }
      Gdiplus::PointF pos(currentX, adjustedY);
      gdi->graphics->DrawString(combinedText.c_str(), -1, activeFont, pos, &format, gdi->brush);

      Gdiplus::RectF boundRect;
      gdi->graphics->MeasureString(combinedText.c_str(), -1, activeFont, pos, &format, &boundRect);
      currentX += boundRect.Width;
    } else {
      // 【文字間隔調整モード】すでに書記素クラスター単位に分かれているのでそのまま1要素ずつ描画
      for (const auto& item : run.items) {
	Gdiplus::PointF pos(currentX, adjustedY);
	gdi->graphics->DrawString(item.utf16.c_str(), -1, activeFont, pos, &format, gdi->brush);

	Gdiplus::RectF boundRect;
	gdi->graphics->MeasureString(item.utf16.c_str(), -1, activeFont, pos, &format, &boundRect);
	currentX += boundRect.Width + charSpacingPx;
      }
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
  Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
  double x, y, w, h, a;
  double space =  UNIT_CONV (fontinfo->space / 72.0 * 25.4);

  format.SetFormatFlags(format.GetFormatFlags() | Gdiplus::StringFormatFlagsNoClip);
  gdi->graphics->MeasureString (text, -1, &font, pointF, &format, &boundingBox);
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
  emfplus_draw_string_runs(gdi, text, fontinfo->font, fontinfo->alternative, fontSize, (Gdiplus::FontStyle)fontinfo->style, pointF, space);
  emfplus_draw_string_utf8(gdi, text, fontinfo->font, fontinfo->alternative, fontSize, (Gdiplus::FontStyle)fontinfo->style, pointF, space);
  gdi->graphics->Restore(state);

  w = pointF.X;
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

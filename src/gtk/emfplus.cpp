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

// 分割された文字列の塊（ラン）
struct TextRun {
  std::wstring text;     // この区間のUTF-16文字列
  std::wstring fontName; // この区間に適用するフォント名
};

// 異体字セレクタ (Variation Selector) や結合文字など、前の文字のフォントに追従すべきコードポイントか判定
bool
IsVariationSelectorOrJoiner(UINT32 cp) {
  return (cp >= 0xFE00 && cp <= 0xFE0F) ||     // 異体字セレクタ (絵文字スタイル等)
    (cp >= 0xE0100 && cp <= 0xE01EF) ||   // 漢字異体字セレクタ (IVS)
    (cp == 0x200D);                       // ゼロ幅接合子 (ZWJ: 結合絵文字用)
}

// 1つのコードポイントに対して最適なフォント名を解決する
std::wstring
ResolveFontForCodePoint(
			IDWriteFontCollection* fontCollection,
			const std::wstring& primaryFontName,
			const std::vector<std::wstring>& fallbackFonts,
			DWRITE_FONT_WEIGHT weight,
			DWRITE_FONT_STYLE style,
			UINT32 cp)
{
  // 制御文字（スペースや改行等）は第1候補フォントをそのまま使う
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

  // 1. 第1候補フォントで描画可能か
  if (hasGlyph(primaryFontName)) {
    return primaryFontName;
  }

  // 2. 代替フォントリストを先頭から順にチェック
  for (const auto& fbFont : fallbackFonts) {
    if (hasGlyph(fbFont)) {
      return fbFont;
    }
  }

  // 3. すべて不可なら第1候補を返し、OSのフォールバックに任せる
  return primaryFontName;
}

// UTF-16文字列を解析し、最適なフォントごとのラン(TextRun)に分割する
std::vector<TextRun> SplitTextIntoFontRuns(
					   IDWriteFontCollection* fontCollection,
					   const std::wstring& textUtf16,
					   const std::wstring& primaryFontName,
					   const char* const* fallbackFontsUtf8,
					   Gdiplus::FontStyle fontStyle)
{
  std::vector<TextRun> runs;
  if (textUtf16.empty()) return runs;

  // 代替フォントリストをUTF-16に変換してキャッシュ
  std::vector<std::wstring> fallbackFonts;
  if (fallbackFontsUtf8) {
    for (size_t i = 0; fallbackFontsUtf8[i] != nullptr; ++i) {
      int len = MultiByteToWideChar(CP_UTF8, 0, fallbackFontsUtf8[i], -1, nullptr, 0);
      if (len > 1) {
	std::wstring wstr(len - 1, L'\0');
	MultiByteToWideChar(CP_UTF8, 0, fallbackFontsUtf8[i], -1, &wstr[0], len);
	fallbackFonts.push_back(std::move(wstr));
      }
    }
  }

  DWRITE_FONT_WEIGHT dwWeight = (fontStyle & Gdiplus::FontStyleBold) ? DWRITE_FONT_WEIGHT_BOLD : DWRITE_FONT_WEIGHT_NORMAL;
  DWRITE_FONT_STYLE dwStyle = (fontStyle & Gdiplus::FontStyleItalic) ? DWRITE_FONT_STYLE_ITALIC : DWRITE_FONT_STYLE_NORMAL;

  for (size_t i = 0; i < textUtf16.length(); ++i) {
    wchar_t ch = textUtf16[i];
    UINT32 cp = static_cast<UINT32>(ch);
    size_t charLen = 1;

    // サロゲートペアの検出と合成
    if (ch >= 0xD800 && ch <= 0xDBFF && (i + 1) < textUtf16.length()) {
      wchar_t low = textUtf16[i + 1];
      if (low >= 0xDC00 && low <= 0xDFFF) {
	cp = 0x10000 + (((static_cast<UINT32>(ch) - 0xD800) << 10) | (static_cast<UINT32>(low) - 0xDC00));
	charLen = 2;
      }
    }

    std::wstring utf16Char = textUtf16.substr(i, charLen);
    std::wstring targetFont;

    // 異体字セレクタやZWJは直前の文字と同じランに結合する
    if (!runs.empty() && IsVariationSelectorOrJoiner(cp)) {
      targetFont = runs.back().fontName;
    } else {
      targetFont = ResolveFontForCodePoint(fontCollection, primaryFontName, fallbackFonts, dwWeight, dwStyle, cp);
    }

    // 直前のランと同じフォントなら文字列を連結、異なるフォントなら新しいランを作成
    if (!runs.empty() && runs.back().fontName == targetFont) {
      runs.back().text += utf16Char;
    } else {
      runs.push_back({ utf16Char, targetFont });
    }

    if (charLen == 2) {
      ++i; // サロゲートペアの2要素目をスキップ
    }
  }

  return runs;
}

// 指定された FontFamily とスタイルにおけるアセント（上端からベースラインまでのピクセル数）を計算
Gdiplus::REAL
GetFontAscentPx(const Gdiplus::FontFamily& family, Gdiplus::FontStyle style, Gdiplus::REAL fontSize) {
  UINT16 emHeight = family.GetEmHeight(style);
  if (emHeight == 0) return 0.0f;
  UINT16 cellAscent = family.GetCellAscent(style);
  return fontSize * (static_cast<Gdiplus::REAL>(cellAscent) / static_cast<Gdiplus::REAL>(emHeight));
}

/**
 * 分割されたフォントランごとに、ベースラインを揃えながら描画する関数
 */
void
emfplus_draw_string_runs(
			 struct gdiobj* gdi,
			 const std::wstring& textUtf16,
			 const std::wstring& primaryFontName,
			 const char* const* fallbackFontsUtf8,
			 Gdiplus::REAL fontSize,
			 Gdiplus::FontStyle fontStyle,
			 double space,
			 const Gdiplus::PointF& startPoint)
{
  if (!gdi || !gdi->graphics || !gdi->brush || textUtf16.empty()) return;

  // 1. 文字列をフォントごとの区間(Run)に分割
  std::vector<TextRun> runs = SplitTextIntoFontRuns(
						    gdi->dwriteReady ? gdi->sysFontCollection.Get() : nullptr,
						    textUtf16,
						    primaryFontName,
						    fallbackFontsUtf8,
						    fontStyle
						    );

  // 2. 計測・描画用の StringFormat を準備
  Gdiplus::StringFormat format(Gdiplus::StringFormat::GenericTypographic());
  INT flags = format.GetFormatFlags();
  flags |= Gdiplus::StringFormatFlagsMeasureTrailingSpaces;
  flags |= Gdiplus::StringFormatFlagsNoClip;
  flags &= ~Gdiplus::StringFormatFlagsLineLimit;
  format.SetFormatFlags(flags);

  // 3. 基準となる「第1候補フォント」のアセント(px)を算出
  Gdiplus::FontFamily primaryFamily(primaryFontName.c_str());
  Gdiplus::REAL baseAscent = 0.0f;
  if (primaryFamily.GetLastStatus() == Gdiplus::Ok) {
    baseAscent = GetFontAscentPx(primaryFamily, fontStyle, fontSize);
  } else {
    baseAscent = GetFontAscentPx(*Gdiplus::FontFamily::GenericSansSerif(), fontStyle, fontSize);
  }

  Gdiplus::REAL currentX = startPoint.X;

  // 4. 分割されたランごとにベースラインを合わせて順番に描画
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

    // このランのフォントのアセントを算出し、基準アセントとの差分で Y 座標を補正
    Gdiplus::REAL runAscent = GetFontAscentPx(*activeFamily, fontStyle, fontSize);
    Gdiplus::REAL adjustedY = startPoint.Y + (baseAscent - runAscent);

    Gdiplus::PointF pos(currentX, adjustedY);

    // 描画実行
    gdi->graphics->DrawString(run.text.c_str(), -1, activeFont, pos, &format, gdi->brush);

    // 次のランの開始 X 座標を進める
    Gdiplus::RectF boundRect;
    gdi->graphics->MeasureString(run.text.c_str(), -1, activeFont, pos, &format, &boundRect);
    currentX += boundRect.Width + space;
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
  emfplus_draw_string_runs(gdi, text, fontinfo->font, fontinfo->alternative, fontSize, (Gdiplus::FontStyle)fontinfo->style, space, pointF);
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

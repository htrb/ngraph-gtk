#include "common.h"

#if WINDOWS
#ifndef __EMFPLUS__
#define __EMFPLUS__ 1
#include <wchar.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

  struct font_info
  {
    double size, dir, space;
    int style;
    wchar_t *font;
  };

  struct gdiobj;

  struct gdiobj *emfplus_init (const wchar_t *filename, int width, int height, int iscale);
  void emfplus_finalize (struct gdiobj *gdi);
  void emfplus_flush (struct gdiobj *gdi);
  void emfplus_line (struct gdiobj *gdi, int x1, int y1, int x2, int y2);
  void emfplus_text (struct gdiobj *gdi, int *x, int *y, struct font_info *font, const wchar_t *text);
  void emfplus_line_attribte (struct gdiobj *gdi, int width, int cap, int join, int miter, int n, int *style);
  void emfplus_rectangle (struct gdiobj *gdi, int x1, int y1, int x2, int y2, int fill);
  void emfplus_polygon (struct gdiobj *gdi, int n, const int *xy, int fill);
  void emfplus_color (struct gdiobj *gdi, int r, int g, int b, int a);
  void emfplus_lines (struct gdiobj *gdi, int n, const int *xy);
  void emfplus_clip (struct gdiobj *gdi, int x1, int y1, int x2, int y2);
  void emfplus_arc (struct gdiobj *gdi, int x, int y, int w, int h, int start, int angle, int style);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
#endif

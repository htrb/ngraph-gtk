/*
 * $Id: ogra2cairofile.c,v 1.16 2009-11-16 09:13:05 hito Exp $
 */

#include "gtk_common.h"

#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <math.h>

#include <cairo/cairo.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-svg.h>
#ifdef CAIRO_HAS_WIN32_SURFACE
#include <cairo/cairo-win32.h>
#endif	/* CAIRO_HAS_WIN32_SURFACE */

#include "mathfn.h"
#include "object.h"
#include "ioutil.h"

#include "init.h"
#include "x11gui.h"
#include "ogra2cairo.h"
#include "ogra2cairofile.h"

#define NAME "gra2cairofile"
#define PARENT "gra2cairo"
#define OVERSION  "1.00.00"

#ifndef M_PI
#define M_PI 3.141592
#endif

char *surface_type[] = {
  "ps2",
  "ps3",
  "eps2",
  "eps3",
  "pdf",
  "svg1.1",
  "svg1.2",
  "png",
#ifdef CAIRO_HAS_WIN32_SURFACE
  "emf",
#endif	/* CAIRO_HAS_WIN32_SURFACE */
  NULL,
};


static int
gra2cairofile_init(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  int dpi;
  struct gra2cairo_local *local;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  dpi = 72;

  if (_putobj(obj, "dpi", inst, &dpi) < 0)
    goto Err;

  if (_putobj(obj, "dpix", inst, &dpi) < 0)
    goto Err;

  if (_putobj(obj, "dpiy", inst, &dpi) < 0)
    goto Err;

  _getobj(obj, "_local", inst, &local);

  local->pixel_dot_x =
  local->pixel_dot_y = dpi / (DPI_MAX * 1.0);

  return 0;

 Err:
  local = gra2cairo_free(obj, inst);
  g_free(local);

  return 1;
}

static int
gra2cairofile_done(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  return 0;
}

#ifdef CAIRO_HAS_WIN32_SURFACE
static cairo_surface_t *
open_emf(int dpi)
{
  HDC hdc;
  cairo_surface_t *surface;
  XFORM xform = {1, 0, 0, 1, 0, 0};
  int disp_dpi;

  hdc = CreateEnhMetaFile(NULL, NULL, NULL, NULL);
  if (hdc == NULL) {
    return NULL;
  }

  SetGraphicsMode(hdc, GM_ADVANCED);
  disp_dpi = GetDeviceCaps(hdc, LOGPIXELSX);
  xform.eM11 = xform.eM22 = 1.0 * disp_dpi / dpi;
  SetWorldTransform(hdc, &xform);

  surface = cairo_win32_printing_surface_create(hdc);
  StartPage(hdc);

  return surface;
}

static int
close_emf(cairo_surface_t *surface, const char *fname)
{
  HDC hdc;
  HENHMETAFILE emf;
  int r;

  hdc = cairo_win32_surface_get_dc(surface);
  cairo_surface_flush(surface);
  cairo_surface_copy_page(surface);
  cairo_surface_finish(surface);

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
  /* DeleteDC() is called in the cairo library */
  return r;
}
#endif	/* CAIRO_HAS_WIN32_SURFACE */

static cairo_t *
create_cairo(struct objlist *obj, N_VALUE *inst, char *fname, int iw, int ih, int *err)
{
  cairo_surface_t *ps;
  cairo_t *cairo;
  double w, h;
  int format, dpi, r;
  struct gra2cairo_local *local;

  _getobj(obj, "format", inst, &format);

#ifdef CAIRO_HAS_WIN32_SURFACE
  if (format != TYPE_EMF && fname == NULL) {
      return NULL;
  }
#else
  if (fname == NULL) {
    return NULL;
  }
#endif

  if (fname) {
    fname = g_filename_from_utf8(fname, -1, NULL, NULL, NULL);
    if (fname == NULL) {
      *err = CAIRO_STATUS_NO_MEMORY;
      return NULL;
    }
  }

  *err = 0;

  _getobj(obj, "dpi", inst, &dpi);
  _getobj(obj, "_local", inst, &local);

  w = iw * dpi / 25.4 / 100;
  h = ih * dpi / 25.4 / 100;

  switch (format) {
  case TYPE_PS2:
    ps = cairo_ps_surface_create(fname, w, h);
    cairo_ps_surface_restrict_to_level(ps, CAIRO_PS_LEVEL_2);
    cairo_ps_surface_set_eps(ps, FALSE);
    break;
  case TYPE_PS3:
    ps = cairo_ps_surface_create(fname, w, h);
    cairo_ps_surface_restrict_to_level(ps, CAIRO_PS_LEVEL_3);
    cairo_ps_surface_set_eps(ps, FALSE);
    break;
  case TYPE_EPS2:
    ps = cairo_ps_surface_create(fname, w, h);
    cairo_ps_surface_restrict_to_level(ps, CAIRO_PS_LEVEL_2);
    cairo_ps_surface_set_eps(ps, TRUE);
    break;
  case TYPE_EPS3:
    ps = cairo_ps_surface_create(fname, w, h);
    cairo_ps_surface_restrict_to_level(ps, CAIRO_PS_LEVEL_3);
    cairo_ps_surface_set_eps(ps, TRUE);
    break;
  case TYPE_PDF:
    ps = cairo_pdf_surface_create(fname, w, h);
    break;
  case TYPE_SVG1_1:
    ps = cairo_svg_surface_create(fname, w, h);
    cairo_svg_surface_restrict_to_version(ps, CAIRO_SVG_VERSION_1_1);
    break;
  case TYPE_SVG1_2:
    ps = cairo_svg_surface_create(fname, w, h);
    cairo_svg_surface_restrict_to_version(ps, CAIRO_SVG_VERSION_1_2);
    break;
  case TYPE_PNG:
    ps = cairo_image_surface_create(CAIRO_FORMAT_RGB24, w, h);
    break;
#ifdef CAIRO_HAS_WIN32_SURFACE
  case TYPE_EMF:
    ps = open_emf(dpi);
    if (ps == NULL) {
      g_free(fname);
      return NULL;
    }
    break;
#endif	/* CAIRO_HAS_WIN32_SURFACE */
  default:
    ps = cairo_ps_surface_create(fname, w, h);
  }

  g_free(fname);

  r = cairo_surface_status(ps);
  if (r != CAIRO_STATUS_SUCCESS) {
    *err = r;
    cairo_surface_destroy(ps);
    return NULL;
  }

  cairo = cairo_create(ps);
  /* cairo_create() references target, so you can immediately call cairo_surface_destroy() on it */
  cairo_surface_destroy(ps);

  r = cairo_status(cairo);
  if (r != CAIRO_STATUS_SUCCESS) {
    *err = r;
    cairo_destroy(cairo);
    return NULL;
  }

  switch (format) {
  case TYPE_PNG:
    cairo_set_source_rgb(cairo, 1, 1, 1);
    cairo_paint(cairo);
    cairo_new_path(cairo);
    break;
  }

  return cairo;
}

static int
init_cairo(struct objlist *obj, N_VALUE *inst, struct gra2cairo_local *local, int w, int h)
{
  char *fname;
  cairo_t *cairo;
  int t2p, r;

  _getobj(obj, "file", inst, &fname);

  cairo = create_cairo(obj, inst, fname, w, h, &r);

  if (cairo == NULL) {
    return r;
  }

  if (local->cairo)
    cairo_destroy(local->cairo);

  local->cairo = cairo;

  _getobj(obj, "text2path", inst, &t2p);

  local->text2path = t2p;

  return 0;
}

static int
gra2cairofile_output(struct objlist *obj, N_VALUE *inst, N_VALUE *rval,
                 int argc, char **argv)
{
  char code, *fname;
  const int *cpar;
  int format, r;
  struct gra2cairo_local *local;

  local = (struct gra2cairo_local *)argv[2];
  code = *(char *)(argv[3]);
  cpar = (int *)argv[4];

  switch (code) {
  case 'I':
    r = init_cairo(obj, inst, local, cpar[3], cpar[4]);
    if (r) {
      _getobj(obj, "file", inst, &fname);
      error2(obj, r + 100, fname);
      return 1;
    }
    break;
  case 'E':
    _getobj(obj, "format", inst, &format);
    if (local->cairo) {
      cairo_surface_t *surface;
      switch (format) {
      case TYPE_PNG:
	gra2cairo_draw_path(local);

	_getobj(obj, "file", inst, &fname);
	if (fname == NULL)
	  return 1;

	fname = g_filename_from_utf8(fname, -1, NULL, NULL, NULL);
	if (fname == NULL) {
	  error(obj, CAIRO_STATUS_NO_MEMORY + 100);
	  return 1;
	}
	surface = cairo_get_target(local->cairo);
	r = cairo_surface_write_to_png(surface, fname);

	g_free(fname);

	if (r) {
	  _getobj(obj, "file", inst, &fname);
	  error2(obj, r + 100, fname);
	  return 1;
	}
	break;
#ifdef CAIRO_HAS_WIN32_SURFACE
      case TYPE_EMF:
	gra2cairo_draw_path(local);
	surface = cairo_get_target(local->cairo);
        _getobj(obj, "file", inst, &fname);
	r = close_emf(surface, fname);
	if (r) {
	  error2(obj, CAIRO_STATUS_WRITE_ERROR + 100, fname);
	  return 1;
	}
	break;
#endif	/* CAIRO_HAS_WIN32_SURFACE */
      }
    }
    break;
  }

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv))
    return 1;

  return 0;
}

static struct objtable gra2cairofile[] = {
  {"init", NVFUNC, NEXEC, gra2cairofile_init, NULL, 0},
  {"done", NVFUNC, NEXEC, gra2cairofile_done, NULL, 0},
  {"next", NPOINTER, 0, NULL, NULL, 0},
  {"file", NSTR, NREAD | NWRITE, NULL, NULL,0},
  {"text2path", NBOOL, NREAD | NWRITE, NULL, NULL,0},
  {"format", NENUM, NREAD | NWRITE, NULL, surface_type, 0},
  {"_output", NVFUNC, 0, gra2cairofile_output, NULL, 0},
  {"_strwidth", NIFUNC, 0, gra2cairo_strwidth, NULL, 0},
  {"_charascent", NIFUNC, 0, gra2cairo_charheight, NULL, 0},
  {"_chardescent", NIFUNC, 0, gra2cairo_charheight, NULL, 0},
};

#define TBLNUM (sizeof(gra2cairofile) / sizeof(*gra2cairofile))

void *
addgra2cairofile()
/* addgra2cairofile() returns NULL on error */
{
  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, gra2cairofile, Gra2CairoErrMsgNum, Gra2CairoErrMsgs, NULL, NULL);
}

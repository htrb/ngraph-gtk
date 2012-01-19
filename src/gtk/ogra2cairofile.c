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
#include "ngraph.h"
#include "object.h"
#include "ioutil.h"

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
static HDC
create_reference_dc(void)
{
#if 0
  PRINTDLG print_dlg;
  DEVNAMES *dev_names;
  DEVMODE *dev_mode;
  char *driver_name, *device_name;
  memset(&print_dlg, 0, sizeof(print_dlg));

  print_dlg.lStructSize = sizeof(print_dlg);
  print_dlg.Flags = PD_RETURNDEFAULT;

  if (PrintDlg(&print_dlg) == 0) {
    return CreateDC("DISPLAY", NULL, NULL, NULL);
  }

  dev_names   = (DEVNAMES *) (GlobalLock(print_dlg.hDevNames));
  dev_mode    = (DEVMODE *) (GlobalLock(print_dlg.hDevMode));
  driver_name = (char *) (dev_names) + dev_names->wDriverOffset;
  device_name = (char *) (dev_names) + dev_names->wDeviceOffset;

  return CreateDC(driver_name, device_name, NULL, dev_mode);
#else
  char name[1024];
  DWORD len;
  HDC hdc;

  len = sizeof(name);
  if (GetDefaultPrinter(name, &len)) {
    hdc = CreateDC("WINSPOOL", name, NULL, NULL);
  } else {
    hdc = CreateDC("DISPLAY", NULL, NULL, NULL);
  }

  return hdc;
#endif
}
#endif

static cairo_t *
create_cairo(struct objlist *obj, N_VALUE *inst, char *fname, int iw, int ih, int *err)
{
  cairo_surface_t *ps;
  cairo_t *cairo;
  double w, h;
  int format, dpi, r;
  struct gra2cairo_local *local;
#ifdef CAIRO_HAS_WIN32_SURFACE
  HDC hdc, hdc_ref;
#endif	/* CAIRO_HAS_WIN32_SURFACE */

#ifdef WINDOWS
  fname = g_locale_from_utf8(fname, -1, NULL, NULL, NULL);
#else  /* WINDOWS */
  fname = g_filename_from_utf8(fname, -1, NULL, NULL, NULL);
#endif	/* WINDOWS */

  if (fname == NULL) {
    *err = CAIRO_STATUS_NO_MEMORY;
    return NULL;
  }

  *err = 0;

  _getobj(obj, "format", inst, &format);
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
    hdc_ref = create_reference_dc();
    hdc = CreateEnhMetaFile(hdc_ref, fname, NULL, NULL);
    DeleteDC(hdc_ref);
    if (hdc == NULL) {
      g_free(fname);
      return NULL;
    }
    ps = cairo_win32_printing_surface_create(hdc);
    StartPage(hdc);
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
  if (fname == NULL)
    return CAIRO_STATUS_NULL_POINTER;

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
  int *cpar, format, r;
  struct gra2cairo_local *local;
  cairo_surface_t *surface;
#ifdef CAIRO_HAS_WIN32_SURFACE
  HDC hdc;
  HENHMETAFILE emf;
#endif	/* CAIRO_HAS_WIN32_SURFACE */

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
      switch (format) {
      case TYPE_PNG:
	gra2cairo_draw_path(local);

	_getobj(obj, "file", inst, &fname);
	if (fname == NULL)
	  return 1;

#if WINDOWS
	fname = g_locale_from_utf8(fname, -1, NULL, NULL, NULL);
#else  /* WINDOWS */
	fname = g_filename_from_utf8(fname, -1, NULL, NULL, NULL);
#endif	/* WINDOWS */
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
	hdc = cairo_win32_surface_get_dc(surface);
	cairo_surface_flush(surface);
	cairo_surface_copy_page(surface);
	cairo_surface_finish(surface);
	EndPage(hdc);
	emf = CloseEnhMetaFile(hdc);
	DeleteEnhMetaFile(emf);
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

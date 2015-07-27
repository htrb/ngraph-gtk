#ifndef _O_GRA2CAIROFILE_HEADER
#define _O_GRA2CAIROFILE_HEADER

#include <cairo/cairo.h>
#include <cairo/cairo-ps.h>
#include <cairo/cairo-pdf.h>
#include <cairo/cairo-svg.h>

enum surface_type_id {
  TYPE_PS2,
  TYPE_PS3,
  TYPE_EPS2,
  TYPE_EPS3,
  TYPE_PDF,
  TYPE_SVG1_1,
  TYPE_SVG1_2,
  TYPE_PNG,
#ifdef CAIRO_HAS_WIN32_SURFACE
  TYPE_EMF,
  TYPE_CLIPBOARD,
#endif	/* CAIRO_HAS_WIN32_SURFACE */
};

#endif


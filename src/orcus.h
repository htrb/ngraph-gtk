#include "common.h"

#if HAVE_LIBORCUS

#ifndef __ORCUS__
#define __ORCUS__ 1

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include "spreadsheet.h"
#include "math/math_equation.h"

  struct n_orcus;

  void n_orcus_close (struct spreadsheet *sheet);
  struct n_orcus *n_orcus_open (const char *filename, enum spreadsheet_type type);
  int n_orcus_select_sheet (struct spreadsheet *sheet, int index);
  int n_orcus_get_dimension (struct spreadsheet *sheet, int *column, int *row);
  char *n_orcus_get_sheet_name (struct spreadsheet *sheet, int index);
  char *n_orcus_get_text (struct spreadsheet *sheet, int col, int row);
  void n_orcus_get_double (struct spreadsheet *sheet, int col, int row, MathValue *data);
  int n_orcus_sheet_count (const struct spreadsheet *sheet);

#ifdef __cplusplus
}
#endif /* __cplusplus */
#endif
#endif

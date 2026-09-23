#ifndef __NFREEXL__
#define __NFREEXL__ 1

#include "common.h"
#include "spreadsheet.h"
#include "math/math_equation.h"

const void *n_freexl_open (const char *file, enum spreadsheet_type type);
void n_freexl_close (const void *handle);
int n_freexl_get_dimension (const void *handle, int *col, int *row);
char *n_freexl_get_sheet_name (const void *handle, int i);
int n_freexl_sheet_count (const void *handle);

int n_freexl_select_sheet (struct spreadsheet *sheet, int index);
char *n_freexl_get_text (struct spreadsheet *sheet, int col, int row);
void n_freexl_get_double (struct spreadsheet *sheet, int col, int row, MathValue *data);

#endif

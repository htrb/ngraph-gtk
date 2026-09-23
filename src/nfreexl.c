#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freexl.h>
#include "odata.h"
#include "ntime.h"
#include "spreadsheet.h"
#include "math/math_equation.h"

const void *
n_freexl_open (const char *file, enum spreadsheet_type type)
{
  const void *handle;
  int ret;

  switch (type) {
  case SPREADSHEET_TYPE_XLSX:
    ret = freexl_open_xlsx (file, &handle);
    break;
  case SPREADSHEET_TYPE_XLS:
    ret = freexl_open (file, &handle);
    break;
  case SPREADSHEET_TYPE_ODS:
    ret = freexl_open_ods (file, &handle);
    break;
  default:
    ret = FREEXL_FILE_NOT_FOUND;
    break;
  }

  if (ret != FREEXL_OK) {
    freexl_close (handle);
    return NULL;
  }

  return handle;
}

void
n_freexl_close (const void *handle)
{
  if (handle == NULL) {
    return;
  }
  freexl_close (handle);
}

static int
date_to_serial (const char *date)
{
  int year, month, day, n;
  if (date == NULL) {
    return 0;
  }
  n = sscanf (date, "%d-%d-%d", &year, &month, &day);
  if (n != 3) {
    return 0;
  }
  return date_to_mjd (year, month, day);
}

static double
time_to_serial (const char *time)
{
  int h, m, s, n;
  if (time == NULL) {
    return 0;
  }
  n = sscanf (time, "%d-%d-%d", &h, &m, &s);
  if (n != 3) {
    return 0;
  }
  return h / 24.0 + m / 1440.0 + s / 86400.0;
}

static double
datetime_to_serial (const char *datetime)
{
  const char *time;
  double mjd, t = 0;

  if (datetime == NULL) {
    return 0;
  }
  time = strchr (datetime, ' ');
  if (time) {
    t = time_to_serial (time);
  }
  mjd = date_to_serial (datetime);
  return mjd + t;
}

void
n_freexl_get_double (struct spreadsheet *sheet, int col, int row, MathValue *data)
{
  int ret;
  FreeXL_CellValue cell;
  const void *handle;
  const char *str;

  data->val = 0;
  data->type = MATH_VALUE_NAN;

  if (sheet == NULL) {
    return;
  }
  handle = sheet->freexl;
  if (handle == NULL) {
    return;
  }

  ret = freexl_get_cell_value (handle, row, col, &cell);
  if (ret != FREEXL_OK) {
    return;
  }

  switch (cell.type) {
  case FREEXL_CELL_INT:
    data->val = cell.value.int_value;
    data->type = MATH_VALUE_NORMAL;
    break;
  case FREEXL_CELL_DOUBLE:
    data->val = cell.value.double_value;
    data->type = MATH_VALUE_NORMAL;
    break;
  case FREEXL_CELL_DATE:
    str = cell.value.text_value;
    if (str) {
      data->val = date_to_serial (str);
      data->type = MATH_VALUE_NORMAL;
    }
    break;
  case FREEXL_CELL_DATETIME:
    str = cell.value.text_value;
    if (str) {
      data->val = datetime_to_serial (str);
      data->type = MATH_VALUE_NORMAL;
    }
    break;
  case FREEXL_CELL_TIME:
    str = cell.value.text_value;
    if (str) {
      data->val = time_to_serial (str);
      data->type = MATH_VALUE_NORMAL;
    }
    break;
  case FREEXL_CELL_NULL:
    break;
  case FREEXL_CELL_TEXT:
  case FREEXL_CELL_SST_TEXT:
    n_strtod (cell.value.text_value, data);
    break;
  default:
    break;
  }
}

char *
n_freexl_get_text (struct spreadsheet *sheet, int col, int row)
{
  int ret;
  FreeXL_CellValue cell;
  char *str;
  const void *handle;

  if (sheet == NULL) {
    return NULL;
  }
  handle = sheet->freexl;
  if (handle == NULL) {
    return NULL;
  }

  ret = freexl_get_cell_value (handle, row, col, &cell);
  if (ret != FREEXL_OK) {
    return NULL;
  }

  switch (cell.type) {
  case FREEXL_CELL_INT:
    str = g_strdup_printf ("%d", cell.value.int_value);
    break;
  case FREEXL_CELL_DOUBLE:
    str = g_strdup_printf ("%g", cell.value.double_value);
    break;
  case FREEXL_CELL_DATE:
    str = g_strdup_printf ("%d", date_to_serial (cell.value.text_value));
    break;
  case FREEXL_CELL_DATETIME:
    str = g_strdup_printf ("%g", datetime_to_serial (cell.value.text_value));
    break;
  case FREEXL_CELL_TIME:
    str = g_strdup_printf ("%g", time_to_serial (cell.value.text_value));
    break;
  case FREEXL_CELL_TEXT:
  case FREEXL_CELL_SST_TEXT:
    str = g_strdup (cell.value.text_value);
    break;
  default:
    str = NULL;
    break;
  }
  return str;
}

int
n_freexl_select_sheet (struct spreadsheet *sheet, int index)
{
  int ret;
  const void *handle;

  if (sheet == NULL) {
    return 1;
  }
 handle = sheet->freexl;
  if (handle == NULL) {
    return 1;
  }

  ret = freexl_select_active_worksheet (handle, index);
  if (ret != FREEXL_OK) {
    return 1;
  }

  return 0;
}

int
n_freexl_get_dimension (const void *handle, int *col, int *row)
{
  unsigned int rows;
  unsigned short columns;
  int ret;

  if (row) {
    *row = 0;
  }
  if (col) {
    *col = 0;
  }
  if (handle == NULL) {
    return 1;
  }
  ret = freexl_worksheet_dimensions(handle, &rows, &columns);
  if (ret != FREEXL_OK || columns < 1 || rows < 1) {
    return 1;
  }
  if (row) {
    *row = rows;
  }
  if (col) {
    *col = columns;
  }
  return 0;
}

int
n_freexl_sheet_count (const void *handle)
{
  unsigned int num;
  int ret;

  if (handle == NULL) {
    return 0;
  }

  ret = freexl_get_worksheets_count (handle, &num);
  if (ret != FREEXL_OK || num < 1) {
    return 0;
  }
  return num;
}

char *
n_freexl_get_sheet_name (const void *handle, int i)
{
  const char *name = NULL;
  int ret;

  if (handle == NULL) {
    return NULL;
  }

  ret = freexl_get_worksheet_name (handle, i, &name);
  if (ret != FREEXL_OK || name == NULL) {
    return NULL;
  }

  return g_strdup (name);
}

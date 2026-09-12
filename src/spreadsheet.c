#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <freexl.h>
#include "odata.h"
#include "spreadsheet.h"
#include "math/math_equation.h"

static struct spreadsheet *spreadsheet_init (const void *handle);

enum spreadsheet_type
spreadsheet_check (const char *file)
{
  const char *ext;

  if (file == NULL) {
    return SPREADSHEET_TYPE_NULL;
  }

  ext = strrchr(file, '.');
  if (ext == NULL) {
    return SPREADSHEET_TYPE_NULL;
  }

  if (g_ascii_strcasecmp (ext, ".xlsx") == 0) {
    return SPREADSHEET_TYPE_XLSX;
  }

  if (g_ascii_strcasecmp (ext, ".xls") == 0) {
    return SPREADSHEET_TYPE_XLS;
  }

  if (g_ascii_strcasecmp (ext, ".ods") == 0) {
    return SPREADSHEET_TYPE_ODS;
  }

  return FALSE;
}

struct spreadsheet *
spreadsheet_open (const char *file)
{
  const void *handle;
  const char *ext;
  int ret;
  enum spreadsheet_type type;

  ext = strrchr(file, '.');
  if (ext == NULL) {
    return NULL;
  }

  type = spreadsheet_check (file);
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

  if (ret == FREEXL_OK) {
    return spreadsheet_init (handle);
  }

  freexl_close (handle);
  return NULL;
}

void
spreadsheet_close (struct spreadsheet **sheet_ptr)
{
  struct spreadsheet *sheet;
  if (sheet_ptr == NULL) {
    return;
  }

  sheet = *sheet_ptr;
  if (sheet == NULL) {
    return;
  }

  *sheet_ptr = NULL;
  freexl_close (sheet->handle);
  g_free (sheet);
}

const char *
spreadsheet_get_name (struct spreadsheet *sheet)
{
  if (sheet == NULL) {
    return NULL;
  }
  return sheet->worksheet[sheet->selected].name;
}

void
spreadsheet_get_double (struct spreadsheet *sheet, int col, int row, MathValue *data)
{
  int maxcol, maxrow, ret;
  FreeXL_CellValue cell;

  if (data == NULL) {
    return;
  }
  data->val = 0;
  data->type = MATH_VALUE_NAN;
  if (sheet == NULL) {
    return;
  }

  maxcol = spreadsheet_max_column (sheet);
  maxrow = spreadsheet_max_row (sheet);
  if (row >= maxrow || col >= maxcol) {
    return;
  }

  ret = freexl_get_cell_value (sheet->handle, row, col, &cell);
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
  case FREEXL_CELL_DATETIME:
  case FREEXL_CELL_TIME:
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
spreadsheet_get_text (struct spreadsheet *sheet, int col, int row, enum spreadsheet_column_type *type)
{
  int maxcol, maxrow, ret;
  FreeXL_CellValue cell;
  char *str;
  enum spreadsheet_column_type t;

  if (sheet == NULL) {
    return NULL;
  }

  maxcol = spreadsheet_max_column (sheet);
  maxrow = spreadsheet_max_row (sheet);
  if (row >= maxrow || col >= maxcol) {
    return NULL;
  }

  ret = freexl_get_cell_value (sheet->handle, row, col, &cell);
  if (ret != FREEXL_OK) {
    return NULL;
  }

  switch (cell.type) {
  case FREEXL_CELL_INT:
    str = g_strdup_printf ("%d", cell.value.int_value);
    t = SPREADSHEET_COLUMN_TYPE_INT;
    break;
  case FREEXL_CELL_DOUBLE:
    str = g_strdup_printf ("%g", cell.value.double_value);
    t = SPREADSHEET_COLUMN_TYPE_FLOAT;
    break;
  case FREEXL_CELL_DATE:
  case FREEXL_CELL_DATETIME:
  case FREEXL_CELL_TIME:
  case FREEXL_CELL_TEXT:
  case FREEXL_CELL_SST_TEXT:
    t = SPREADSHEET_COLUMN_TYPE_TEXT;
    str = g_strdup (cell.value.text_value);
    break;
  default:
    t = SPREADSHEET_COLUMN_TYPE_UNKNOWN;
    str = NULL;
    break;
  }
  if (type) {
    *type = t;
  }
  return str;
}

int
spreadsheet_max_column (struct spreadsheet *sheet)
{
  if (sheet == NULL) {
    return 0;
  }
  return sheet->worksheet[sheet->selected].maxcol;
}

int
spreadsheet_max_row (struct spreadsheet *sheet)
{
  if (sheet == NULL) {
    return 0;
  }
  return sheet->worksheet[sheet->selected].maxrow;
}

int
spreadsheet_select_sheet (struct spreadsheet *sheet, int index)
{
  int ret;

  if (sheet == NULL) {
    return 1;
  }

  if (index >= sheet->num) {
    return 1;
  }

  ret = freexl_select_active_worksheet (sheet->handle, index);
  if (ret != FREEXL_OK) {
    return 1;
  }
  sheet->selected = index;

  return 0;
}

static struct spreadsheet *
spreadsheet_init (const void *handle)
{
  struct spreadsheet *sheet;
  const char *name;
  unsigned int rows;
  unsigned short columns;
  unsigned int i, num;
  int ret;

  ret = freexl_get_worksheets_count (handle, &num);
  if (ret != FREEXL_OK || num == 0) {
    freexl_close(handle);
    return NULL;
  }

  sheet = g_malloc (sizeof (*sheet));
  sheet->handle = handle;
  sheet->worksheet = g_malloc (sizeof (*sheet->worksheet) * num);
  sheet->num = num;
  for (i = 0; i < num; i++) {
    sheet->worksheet[i].maxcol = 0;
    sheet->worksheet[i].maxrow = 0;
    sheet->worksheet[i].name = NULL;
    ret = freexl_select_active_worksheet(handle, i);
    if (ret != FREEXL_OK) {
      continue;
    }
    freexl_get_worksheet_name (handle, i, &name);
    sheet->worksheet[i].name = name;
    sheet->selected = i;

    ret = freexl_worksheet_dimensions(handle, &rows, &columns);
    if (ret != FREEXL_OK || columns < 1 || rows < 1) {
      continue;
    }
    sheet->worksheet[i].maxcol = (columns > FILE_OBJ_MAXCOL) ? FILE_OBJ_MAXCOL : columns;
    sheet->worksheet[i].maxrow = rows;
  }
  return sheet;
}

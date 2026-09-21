#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_LIBORCUS
#include "orcus.h"
#else
#include <freexl.h>
#endif
#include "odata.h"
#include "spreadsheet.h"
#include "math/math_equation.h"

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

const char *
spreadsheet_get_name (struct spreadsheet *sheet)
{
  if (sheet == NULL) {
    return NULL;
  }
  return sheet->worksheet[sheet->selected].name;
}

int
spreadsheet_n_columns (struct spreadsheet *sheet)
{
  if (sheet == NULL) {
    return 0;
  }
  return sheet->worksheet[sheet->selected].n_columns;
}

int
spreadsheet_n_rows (struct spreadsheet *sheet)
{
  if (sheet == NULL) {
    return 0;
  }
  return sheet->worksheet[sheet->selected].n_rows;
}

#if HAVE_LIBORCUS
static struct spreadsheet *spreadsheet_init (struct n_orcus *norcus);

struct spreadsheet *
spreadsheet_open (const char *file)
{
  struct n_orcus *norcus = NULL;
  enum spreadsheet_type type;

  type = spreadsheet_check (file);
  switch (type) {
  case SPREADSHEET_TYPE_XLSX:
  case SPREADSHEET_TYPE_ODS:
    norcus = n_orcus_open (file, type);
    break;
  default:
    break;
  }

  if (norcus == NULL) {
    return NULL;
  }

  return spreadsheet_init (norcus);
}

void
spreadsheet_close (struct spreadsheet **sheet_ptr)
{
  struct spreadsheet *sheet;
  int i;
  if (sheet_ptr == NULL) {
    return;
  }

  sheet = *sheet_ptr;
  if (sheet == NULL) {
    return;
  }

  *sheet_ptr = NULL;
  n_orcus_close (sheet->handle);
  for (i = 0; i < sheet->num; i++) {
    g_free (sheet->worksheet[i].name);
  }
  g_free (sheet);
}

void
spreadsheet_get_double (struct spreadsheet *sheet, int col, int row, MathValue *data)
{
  int n_columns, n_rows;

  if (data == NULL) {
    return;
  }
  data->val = 0;
  data->type = MATH_VALUE_NAN;
  if (sheet == NULL) {
    return;
  }

  n_columns = spreadsheet_n_columns (sheet);
  n_rows = spreadsheet_n_rows (sheet);
  if (row >= n_rows || col >= n_columns) {
    return;
  }

  n_orcus_get_double (sheet->handle, col, row, data);
}

char *
spreadsheet_get_text (struct spreadsheet *sheet, int col, int row)
{
  int n_columns, n_rows;
  char *str;

  if (sheet == NULL) {
    return NULL;
  }

  n_columns = spreadsheet_n_columns (sheet);
  n_rows = spreadsheet_n_rows (sheet);
  if (row >= n_rows || col >= n_columns) {
    return NULL;
  }

  str = n_orcus_get_text (sheet->handle, col, row);
  return str;
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

  ret = n_orcus_select_sheet (sheet->handle, index);
  if (ret) {
    return 1;
  }
  sheet->selected = index;

  return 0;
}

static struct spreadsheet *
spreadsheet_init (struct n_orcus *handle)
{
  struct spreadsheet *sheet;
  char *name;
  int rows, columns, i, num, ret;

  sheet = g_malloc (sizeof (*sheet));
  sheet->handle = handle;
  num = n_orcus_sheet_count (handle);
  sheet->worksheet = g_malloc (sizeof (*sheet->worksheet) * num);
  sheet->num = num;
  for (i = 0; i < num; i++) {
    sheet->worksheet[i].n_columns = 0;
    sheet->worksheet[i].n_rows = 0;
    sheet->worksheet[i].name = NULL;
    ret = n_orcus_select_sheet (handle, i);
    if (ret) {
      continue;
    }
    name = n_orcus_get_sheet_name (handle);
    sheet->worksheet[i].name = name;
    sheet->selected = i;

    ret = n_orcus_get_dimension (handle, &columns, &rows);
    if (ret) {
      continue;
    }
    sheet->worksheet[i].n_columns = (columns > FILE_OBJ_MAXCOL) ? FILE_OBJ_MAXCOL : columns;
    sheet->worksheet[i].n_rows = rows;
  }
  return sheet;
}
#else
static struct spreadsheet *spreadsheet_init (const void *handle);

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
  int i;
  if (sheet_ptr == NULL) {
    return;
  }

  sheet = *sheet_ptr;
  if (sheet == NULL) {
    return;
  }

  *sheet_ptr = NULL;
  freexl_close (sheet->handle);
  for (i = 0; i < sheet->num; i++) {
    g_free (sheet->worksheet[i].name);
  }
  g_free (sheet);
}

void
spreadsheet_get_double (struct spreadsheet *sheet, int col, int row, MathValue *data)
{
  int n_columns, n_rows, ret;
  FreeXL_CellValue cell;

  if (data == NULL) {
    return;
  }
  data->val = 0;
  data->type = MATH_VALUE_NAN;
  if (sheet == NULL) {
    return;
  }

  n_columns = spreadsheet_n_columns (sheet);
  n_rows = spreadsheet_n_rows (sheet);
  if (row >= n_rows || col >= n_columns) {
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

static char *
str2utf8(const char *str)
{
  int valid;
  char *new_str;

  if (str == NULL) {
    return NULL;
  }

  valid = g_utf8_validate (str, -1, NULL);
  if (valid) {
    return g_strdup (str);
  }

  new_str = g_locale_to_utf8 (str, -1, NULL, NULL, NULL);
  if (new_str) {
    return new_str;
  }

  new_str = g_utf8_make_valid (str, -1);
  return new_str;
}

char *
spreadsheet_get_text (struct spreadsheet *sheet, int col, int row)
{
  int n_columns, n_rows, ret;
  FreeXL_CellValue cell;
  char *str;

  if (sheet == NULL) {
    return NULL;
  }

  n_columns = spreadsheet_n_columns (sheet);
  n_rows = spreadsheet_n_rows (sheet);
  if (row >= n_rows || col >= n_columns) {
    return NULL;
  }

  ret = freexl_get_cell_value (sheet->handle, row, col, &cell);
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
  case FREEXL_CELL_DATETIME:
  case FREEXL_CELL_TIME:
  case FREEXL_CELL_TEXT:
  case FREEXL_CELL_SST_TEXT:
    str = str2utf8 (cell.value.text_value);
    break;
  default:
    str = NULL;
    break;
  }
  return str;
}

int
spreadsheet_n_columns (struct spreadsheet *sheet)
{
  if (sheet == NULL) {
    return 0;
  }
  return sheet->worksheet[sheet->selected].n_columns;
}

int
spreadsheet_n_rows (struct spreadsheet *sheet)
{
  if (sheet == NULL) {
    return 0;
  }
  return sheet->worksheet[sheet->selected].n_rows;
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
    sheet->worksheet[i].n_columns = 0;
    sheet->worksheet[i].n_rows = 0;
    sheet->worksheet[i].name = NULL;
    ret = freexl_select_active_worksheet(handle, i);
    if (ret != FREEXL_OK) {
      continue;
    }
    freexl_get_worksheet_name (handle, i, &name);
    sheet->worksheet[i].name = g_strdup (name);
    sheet->selected = i;

    ret = freexl_worksheet_dimensions(handle, &rows, &columns);
    if (ret != FREEXL_OK || columns < 1 || rows < 1) {
      continue;
    }
    sheet->worksheet[i].n_columns = (columns > FILE_OBJ_MAXCOL) ? FILE_OBJ_MAXCOL : columns;
    sheet->worksheet[i].n_rows = rows;
  }
  return sheet;
}
#endif

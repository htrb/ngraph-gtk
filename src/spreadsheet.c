#include "common.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if HAVE_LIBORCUS
#include "orcus.h"
#endif
#include "nfreexl.h"
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

static struct spreadsheet *n_freexl_init (const void *handle);
#if HAVE_LIBORCUS
static struct spreadsheet *n_orcus_init (struct n_orcus *norcus);
#endif

struct spreadsheet *
spreadsheet_open (const char *file)
{
  enum spreadsheet_type type;
  const void *handle = NULL;

  type = spreadsheet_check (file);
  switch (type) {
  case SPREADSHEET_TYPE_XLSX:
  case SPREADSHEET_TYPE_XLS:
  case SPREADSHEET_TYPE_ODS:
    handle = n_freexl_open (file, type);
    break;
  default:
    break;
  }
  if (handle) {
    return n_freexl_init (handle);
  }

#if HAVE_LIBORCUS
  struct n_orcus *norcus = NULL;
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

  return n_orcus_init (norcus);
#endif
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
  if (sheet->freexl) {
    n_freexl_close (sheet->freexl);
  }
#if HAVE_LIBORCUS
  if (sheet->norcus) {
    n_orcus_close (sheet->norcus);
  }
#endif
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

  sheet->get_double (sheet, col, row, data);
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

  str = sheet->get_text (sheet, col, row);
  return str;
}

int
spreadsheet_select_sheet (struct spreadsheet *sheet, int index)
{
  int ret;

  if (sheet == NULL) {
    return 1;
  }

  if (index < 0 || index >= sheet->num) {
    return 1;
  }

  ret = sheet->select_sheet (sheet, index);
  if (ret) {
    return 1;
  }
  sheet->selected = index;

  return 0;
}

#if HAVE_LIBORCUS
static struct spreadsheet *
n_orcus_init (struct n_orcus *handle)
{
  struct spreadsheet *sheet;
  char *name;
  int rows, columns, i, num, ret;

  num = n_orcus_sheet_count (handle);
  if (num < 1) {
    return NULL;
  }

  sheet = g_malloc0 (sizeof (*sheet));
  sheet->norcus = handle;
  sheet->worksheet = g_malloc (sizeof (*sheet->worksheet) * num);
  sheet->num = num;
  for (i = 0; i < num; i++) {
    sheet->worksheet[i].n_columns = 0;
    sheet->worksheet[i].n_rows = 0;
    sheet->worksheet[i].name = NULL;
    ret = n_orcus_select_sheet (sheet, i);
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
  sheet->select_sheet = n_orcus_select_sheet;
  sheet->get_text = n_orcus_get_text;
  sheet->get_double = n_orcus_get_double;
  return sheet;
}
#endif

static struct spreadsheet *
n_freexl_init (const void *handle)
{
  struct spreadsheet *sheet;
  const char *name;
  int rows, columns;
  int i, num;
  int ret;

  num = n_freexl_sheet_count (handle);
  if (num == 0) {
    n_freexl_close(handle);
    return NULL;
  }

  sheet = g_malloc0 (sizeof (*sheet));
  sheet->freexl = handle;
  sheet->worksheet = g_malloc (sizeof (*sheet->worksheet) * num);
  sheet->num = num;
  for (i = 0; i < num; i++) {
    sheet->worksheet[i].n_columns = 0;
    sheet->worksheet[i].n_rows = 0;
    sheet->worksheet[i].name = NULL;
    ret = n_freexl_select_sheet(sheet, i);
    if (ret) {
      continue;
    }
    name = n_freexl_get_sheet_name (handle, i);
    sheet->worksheet[i].name = g_strdup (name);
    sheet->selected = i;

    ret = n_freexl_get_dimension (handle, &columns, &rows);
    if (ret || columns < 1 || rows < 1) {
      continue;
    }
    sheet->worksheet[i].n_columns = (columns > FILE_OBJ_MAXCOL) ? FILE_OBJ_MAXCOL : columns;
    sheet->worksheet[i].n_rows = rows;
  }
  sheet->select_sheet = n_freexl_select_sheet;
  sheet->get_text = n_freexl_get_text;
  sheet->get_double = n_freexl_get_double;
  return sheet;
}

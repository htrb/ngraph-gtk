#ifndef SPREADSHEET_HEADER
#define SPREADSHEET_HEADER

#include "common.h"
#include "math/math_equation.h"

struct spreadsheet {
#if HAVE_LIBORCUS
  struct n_orcus *handle;
#else
  const void *handle;
#endif
  int num, selected;
  struct sheet {
    int n_columns, n_rows;
    char *name;
  } *worksheet;
};

enum spreadsheet_type {
  SPREADSHEET_TYPE_NULL = 0,
  SPREADSHEET_TYPE_XLSX,
  SPREADSHEET_TYPE_XLS,
  SPREADSHEET_TYPE_ODS
};

struct spreadsheet *spreadsheet_open (const char *file);
void spreadsheet_close (struct spreadsheet **sheet_ptr);
int spreadsheet_select_sheet (struct spreadsheet *sheet, int index);
int spreadsheet_n_columns (struct spreadsheet *sheet);
int spreadsheet_n_rows (struct spreadsheet *sheet);
enum spreadsheet_type spreadsheet_check (const char *file);
char *spreadsheet_get_text (struct spreadsheet *sheet, int col, int row);
void spreadsheet_get_double (struct spreadsheet *sheet, int col, int row, MathValue *data);
const char *spreadsheet_get_name (struct spreadsheet *sheet);


#endif

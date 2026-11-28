#ifndef SPREADSHEET_HEADER
#define SPREADSHEET_HEADER

#include "common.h"
#include "math/math_equation.h"

struct spreadsheet {
#if HAVE_LIBORCUS
  struct n_orcus *norcus;
#endif
  const void *freexl;
  int num, selected;
  struct sheet {
    int n_columns, n_rows;
    char *name;
  } *worksheet;
  int (* select_sheet) (struct spreadsheet *, int);
  char * (* get_text) (const struct spreadsheet *, int, int);
  char * (* get_text_raw) (const struct spreadsheet *, int, int);
  void (* get_double) (const struct spreadsheet *, int, int, int, MathValue *);
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
int spreadsheet_n_columns (const struct spreadsheet *sheet);
int spreadsheet_n_rows (const struct spreadsheet *sheet);
enum spreadsheet_type spreadsheet_check (const char *file);
char *spreadsheet_get_text_raw (const struct spreadsheet *sheet, int col, int row);
char *spreadsheet_get_text (const struct spreadsheet *sheet, int col, int row);
void spreadsheet_get_double (const struct spreadsheet *sheet, int col, int row, int is_date, MathValue *data);
const char *spreadsheet_get_name (const struct spreadsheet *sheet);


#endif

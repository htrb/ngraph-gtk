#ifndef SPREADSHEET_HEADER
#define SPREADSHEET_HEADER

struct spreadsheet {
  const void *handle;
  int num, selected;
  struct sheet {
    int maxcol, maxrow;
    const char *name;
  } *worksheet;
};

enum spreadsheet_type {
  SPREADSHEET_TYPE_NULL = 0,
  SPREADSHEET_TYPE_XLSX,
  SPREADSHEET_TYPE_XLS,
  SPREADSHEET_TYPE_ODS
};

enum spreadsheet_column_type {
  SPREADSHEET_COLUMN_TYPE_UNKNOWN = 0,
  SPREADSHEET_COLUMN_TYPE_INT,
  SPREADSHEET_COLUMN_TYPE_FLOAT,
  SPREADSHEET_COLUMN_TYPE_TEXT,
};

struct spreadsheet *spreadsheet_open (const char *file);
void spreadsheet_close (struct spreadsheet **sheet_ptr);
int spreadsheet_select_sheet (struct spreadsheet *sheet, int index);
int spreadsheet_max_column (struct spreadsheet *sheet);
int spreadsheet_max_row (struct spreadsheet *sheet);
enum spreadsheet_type spreadsheet_check (const char *file);
char *spreadsheet_get_text (struct spreadsheet *sheet, int col, int row, enum spreadsheet_column_type *type);
void spreadsheet_get_double (struct spreadsheet *sheet, int col, int row, MathValue *data);
const char *spreadsheet_get_name (struct spreadsheet *sheet);


#endif

#include "common.h"

#include <ixion/model_context.hpp>
#include <ixion/types.hpp>
#include <orcus/spreadsheet/document.hpp>
#include <orcus/spreadsheet/factory.hpp>
#include <orcus/spreadsheet/sheet.hpp>
#include <orcus/spreadsheet/styles.hpp>
#include <orcus/orcus_ods.hpp>
#include <orcus/orcus_xlsx.hpp>

#include <ixion/address.hpp>
#include <ixion/model_context.hpp>

#include <iostream>
#include <cstdlib>
#include <filesystem>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
#include "odata.h"
#include "spreadsheet.h"
#include "math/math_equation.h"
#ifdef __cplusplus
}
#endif /* __cplusplus */

#include "orcus.h"

struct n_orcus {
  orcus::spreadsheet::document *doc;
  orcus::spreadsheet::import_factory *factory;
  int n_sheet, current_sheet;
};

void
n_orcus_close (struct n_orcus *norcus)
{
  if (norcus == NULL) {
    return;
  }
  if (norcus->factory) {
    delete norcus->factory;
  }
  if (norcus->doc) {
    delete norcus->doc;
  }
  g_free (norcus);
}

struct n_orcus *
n_orcus_open (const char *filename, enum spreadsheet_type type)
{
  struct n_orcus *norcus = NULL;
  if (filename == NULL) {
    return NULL;
  }
  norcus = (struct n_orcus *) g_malloc0(sizeof (*norcus));
  try {
    orcus::spreadsheet::range_size_t ss{1048576, 16384};
    norcus->doc = new orcus::spreadsheet::document{ss};
    norcus->factory = new orcus::spreadsheet::import_factory(*norcus->doc);
    switch (type) {
    case SPREADSHEET_TYPE_XLSX:
      {
	orcus::orcus_xlsx loader(norcus->factory);
	loader.read_file(filename);
      }
      break;
    case SPREADSHEET_TYPE_ODS:
      {
	orcus::orcus_ods loader(norcus->factory);
	loader.read_file(filename);
      }
      break;
    default:
      n_orcus_close (norcus);
      norcus = NULL;
      break;
    }
    if (norcus) {
      const ixion::model_context& model = norcus->doc->get_model_context();
      norcus->n_sheet = model.get_sheet_count();
    }
  } catch  (const std::exception& e) {
    n_orcus_close (norcus);
    norcus = NULL;
  }
  return norcus;
}

int
n_orcus_sheet_count (struct n_orcus *norcus)
{
  if (norcus == NULL) {
    return 0;
  }
  return norcus->n_sheet;
}

int
n_orcus_select_sheet (struct n_orcus *norcus, int sheet)
{
  if (norcus == NULL) {
    return 1;
  }
  if (sheet < 0 || sheet >= norcus->n_sheet) {
    return 1;
  }
  norcus->current_sheet = sheet;
  return 0;
}

int
n_orcus_get_dimension (struct n_orcus *norcus, int *column, int *row)
{
  if (norcus == NULL) {
    return 1;
  }
  if (row) {
    *row = 0;
  }
  if (column) {
    *column = 0;
  }
  try {
    const ixion::model_context& model = norcus->doc->get_model_context();
    ixion::abs_range_t size = model.get_data_range(norcus->current_sheet);
    if (row && size.last.row >= 0) {
      *row = size.last.row + 1;
    }
    if (column && size.last.column >= 0) {
      *column = size.last.column + 1;
    }
  } catch  (const std::exception& e) {
  }
  return 0;
}

char *
n_orcus_get_sheet_name (struct n_orcus *norcus)
{
  char *sheetname = NULL;
  if (norcus == NULL) {
    return 0;
  }
  try {
    const ixion::model_context& model = norcus->doc->get_model_context();
    std::string_view name = model.get_sheet_name(norcus->current_sheet);
    sheetname = g_strdup (name.data());
  } catch  (const std::exception& e) {
    sheetname = NULL;
  }
  return sheetname;
}

char *
n_orcus_get_text (struct n_orcus *norcus, int col, int row)
{
  char *text = NULL;
  if (norcus == NULL) {
    return 0;
  }
  try {
    const ixion::model_context& model = norcus->doc->get_model_context();
    ixion::abs_address_t pos(norcus->current_sheet, row, col);
    const std::string *s;
    double val;
    bool state;
    ixion::string_id_t str_id;
    ixion::cell_value_t type = model.get_cell_value_type(pos);
    switch (type) {
    case ixion::cell_value_t::string:
      str_id = model.get_string_identifier(pos);
      s = model.get_string(str_id);
      text = g_strdup (s->c_str());
      break;
    case ixion::cell_value_t::numeric:
      val = model.get_numeric_value (pos);
      text = g_strdup_printf ("%g", val);
      break;
    case ixion::cell_value_t::error:
      text = g_strdup ("Err");
      break;
    case ixion::cell_value_t::boolean:
      state = model.get_boolean_value (pos);
      text = g_strdup (state ? "1" : "0");
      break;
    default:
      text = NULL;
      break;
    }
  } catch  (const std::exception& e) {
    text = NULL;
  }
  return text;
}

void
n_orcus_get_double (struct n_orcus *norcus, int col, int row, MathValue *data)
{
  data->val = 0;
  data->type = MATH_VALUE_NAN;
  if (norcus == NULL) {
    return;
  }
  try {
    const ixion::model_context& model = norcus->doc->get_model_context();
    ixion::abs_address_t pos(norcus->current_sheet, row, col);
    const std::string *s;
    double val;
    bool state;
    ixion::string_id_t str_id;
    ixion::cell_value_t type = model.get_cell_value_type(pos);
    switch (type) {
    case ixion::cell_value_t::unknown:
      data->type = MATH_VALUE_UNDEF;
      break;
    case ixion::cell_value_t::string:
      str_id = model.get_string_identifier(pos);
      s = model.get_string(str_id);
      n_strtod (s->c_str(), data);
      break;
    case ixion::cell_value_t::numeric:
      val = model.get_numeric_value (pos);
      data->val = val;
      data->type = MATH_VALUE_NORMAL;
      break;
    case ixion::cell_value_t::error:
      data->val = 0;
      data->type = MATH_VALUE_ERROR;
      break;
    case ixion::cell_value_t::boolean:
      state = model.get_boolean_value (pos);
      val = state ? 1 : 0;
      data->val = val;
      data->type = MATH_VALUE_NORMAL;
      break;
    default:
      data->type = MATH_VALUE_UNDEF;
      break;
    }
  } catch  (const std::exception& e) {
  }
}

void
show_value (const ixion::model_context& model, const ixion::abs_address_t &pos)
{
  double val;
  val = model.get_numeric_value (pos);
  printf ("\t%g", val);
}

void
cell_format(const orcus::spreadsheet::document &doc, const ixion::model_context& model, const ixion::abs_address_t &pos)
{
  const orcus::spreadsheet::sheet *sheet = doc.get_sheet ((orcus::spreadsheet::sheet_t) pos.sheet);
  auto format_id = sheet->get_cell_format(pos.row, pos.column);
  const orcus::spreadsheet::styles &styles = doc.get_styles();
  const orcus::spreadsheet::number_format_t *format = styles.get_number_format (format_id);
  if (format) {
    const char *format_str;
    format_str = format->format_string->data();
    const char *ptr = strstr (format_str, "YY");
    if (ptr) {
      auto date = sheet->get_date_time(pos.row, pos.column);
      printf ("\t%s", date.to_string ().c_str ());
    } else {
      show_value (model, pos);
    }
  } else {
    show_value (model, pos);
  }
}

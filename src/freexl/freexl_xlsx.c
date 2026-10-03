/* 
/ freexl_xlsx.c
/
/ FreeXL implementation (XLSX support)
/
/ version  2.0, 2021 May 30
/
/ Author: Sandro Furieri a.furieri@lqt.it
/
/ ------------------------------------------------------------------------------
/ 
/ Version: MPL 1.1/GPL 2.0/LGPL 2.1
/ 
/ The contents of this file are subject to the Mozilla Public License Version
/ 1.1 (the "License"); you may not use this file except in compliance with
/ the License. You may obtain a copy of the License at
/ http://www.mozilla.org/MPL/
/ 
/ Software distributed under the License is distributed on an "AS IS" basis,
/ WITHOUT WARRANTY OF ANY KIND, either express or implied. See the License
/ for the specific language governing rights and limitations under the
/ License.
/
/ The Original Code is the FreeXL library
/
/ The Initial Developer of the Original Code is Alessandro Furieri
/ 
/ Portions created by the Initial Developer are Copyright (C) 2021
/ the Initial Developer. All Rights Reserved.
/ 
/ Contributor(s):
/ 
/ Alternatively, the contents of this file may be used under the terms of
/ either the GNU General Public License Version 2 or later (the "GPL"), or
/ the GNU Lesser General Public License Version 2.1 or later (the "LGPL"),
/ in which case the provisions of the GPL or the LGPL are applicable instead
/ of those above. If you wish to allow use of your version of this file only
/ under the terms of either the GPL or the LGPL, and not to allow others to
/ use your version of this file under the terms of the MPL, indicate your
/ decision by deleting the provisions above and replace them with the notice
/ and other provisions required by the GPL or the LGPL. If you do not delete
/ the provisions above, a recipient may use your version of this file under
/ the terms of any one of the MPL, the GPL or the LGPL.
/ 
*/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdint.h>

#if defined(__MINGW32__) || defined(_WIN32)
#define LIBICONV_STATIC
#include <iconv.h>
#define LIBCHARSET_STATIC
#ifdef _MSC_VER
/* <localcharset.h> isn't supported on OSGeo4W */
/* applying a tricky workaround to fix this issue */
extern const char *locale_charset (void);
#else /* sane Windows - not OSGeo4W */
#include <localcharset.h>
#endif /* end localcharset */
#else /* not WINDOWS */
#if defined(__APPLE__) || defined(__ANDROID__)
#include <iconv.h>
#include <localcharset.h>
#else /* neither Mac OsX nor Android */
#include <iconv.h>
#include <langinfo.h>
#endif
#endif

#if defined(_WIN32) && !defined(__MINGW32__)
#include "config-msvc.h"
#else
#include "config.h"
#endif

#ifndef OMIT_XMLDOC		/* only if XML support is enabled */

#include <expat.h>

#include "freexl.h"
#include "freexl_internals.h"

#include <minizip/unzip.h>

#ifdef _WIN32
#define strcasecmp	_stricmp
#endif /* not WIN32 */

static void
xmlCharData (void *data, const XML_Char * s, int len)
{
/* parsing XML char data (Workbook) */
    xlsx_workbook *workbook = (xlsx_workbook *) data;
    if ((workbook->CharDataLen + len) > workbook->CharDataMax)
      {
	  /* we must increase the CharData buffer size */
	  void *new_buf;
	  int new_size = workbook->CharDataMax;
	  while (new_size < workbook->CharDataLen + len)
	      new_size += workbook->CharDataStep;
	  new_buf = realloc (workbook->CharData, new_size);
	  if (new_buf)
	    {
		workbook->CharData = new_buf;
		workbook->CharDataMax = new_size;
	    }
      }
    memcpy (workbook->CharData + workbook->CharDataLen, s, len);
    workbook->CharDataLen += len;
}

static void
xmlCharDataSheet (void *data, const XML_Char * s, int len)
{
/* parsing XML char data (Worksheet) */
    xlsx_worksheet *worksheet = (xlsx_worksheet *) data;
    if ((worksheet->CharDataLen + len) > worksheet->CharDataMax)
      {
	  /* we must increase the CharData buffer size */
	  void *new_buf;
	  int new_size = worksheet->CharDataMax;
	  while (new_size < worksheet->CharDataLen + len)
	      new_size += worksheet->CharDataStep;
	  new_buf = realloc (worksheet->CharData, new_size);
	  if (new_buf)
	    {
		worksheet->CharData = new_buf;
		worksheet->CharDataMax = new_size;
	    }
      }
    memcpy (worksheet->CharData + worksheet->CharDataLen, s, len);
    worksheet->CharDataLen += len;
}

static xlsx_workbook *
alloc_workbook ()
{
/* allocating and initializing the Workbook struct */
    xlsx_workbook *wb = malloc (sizeof (xlsx_workbook));
    if (!wb)
	return NULL;
    wb->first = NULL;
    wb->last = NULL;
    wb->active_sheet = NULL;
    wb->n_strings = 0;
    wb->xml_strings = 0;
    wb->strings = NULL;
    wb->n_formats = 0;
    wb->next_format = 0;
    wb->formats = NULL;
    wb->n_styles = 0;
    wb->next_style = 0;
    wb->styles = NULL;
    wb->first_date = NULL;
    wb->last_date = NULL;
    wb->error = 0;
    wb->SharedStringsZipEntry = NULL;
    wb->WorkbookZipEntry = NULL;
    wb->StylesZipEntry = NULL;
    wb->CharDataStep = 65536;
    wb->CharDataMax = wb->CharDataStep;
    wb->CharData = malloc (wb->CharDataStep);
    wb->CharDataLen = 0;
    wb->SharedStringsOk = 0;
    wb->WorksheetsOk = 0;
    wb->StylesOk = 0;
    wb->FormatsOk = 0;
    wb->CellStylesOk = 0;
    return wb;
}

static void
destroy_row (xlsx_row * row)
{
/* memory cleanup - destroying a WorkSheet Row */
    xlsx_cell *col;
    xlsx_cell *col_n;
    if (row == NULL)
	return;

    col = row->first;
    while (col != NULL)
      {
	  col_n = col->next;
	  free (col);
	  col = col_n;
      }
    free (row);
}

static void
destroy_worksheet (xlsx_worksheet * ws)
{
/* memory cleanup - destroying a WorkSheet object */
    xlsx_row *row;
    xlsx_row *row_n;
    if (ws == NULL)
	return;

    row = ws->first;
    while (row != NULL)
      {
	  row_n = row->next;
	  destroy_row (row);
	  row = row_n;
      }
    if (ws->name != NULL)
	free (ws->name);
    if (ws->rows != NULL)
	free (ws->rows);
    if (ws->CharData != NULL)
	free (ws->CharData);
    free (ws);
}

static void
destroy_workbook (xlsx_workbook * wb)
{
/* memory cleanup - destroying a WorkBook object */
    xlsx_worksheet *ws;
    xlsx_worksheet *ws_n;
    xml_datetime *date;
    xml_datetime *date_n;
    if (wb == NULL)
	return;

    ws = wb->first;
    while (ws != NULL)
      {
	  ws_n = ws->next;
	  destroy_worksheet (ws);
	  ws = ws_n;
      }
    date = wb->first_date;
    while (date != NULL)
      {
	  date_n = date->next;
	  free (date);
	  date = date_n;
      }
    if (wb->strings != NULL)
      {
	  int i;
	  for (i = 0; i < wb->n_strings; i++)
	    {
		char *str = *(wb->strings + i);
		if (str != NULL)
		    free (str);
	    }
	  free (wb->strings);
      }
    if (wb->formats != NULL)
	free (wb->formats);
    if (wb->styles != NULL)
	free (wb->styles);
    if (wb->SharedStringsZipEntry != NULL)
	free (wb->SharedStringsZipEntry);
    if (wb->WorkbookZipEntry != NULL)
	free (wb->WorkbookZipEntry);
    if (wb->StylesZipEntry != NULL)
	free (wb->StylesZipEntry);
    if (wb->CharData != NULL)
	free (wb->CharData);
    free (wb);
}

static char *
setString (const char *entry)
{
/* permanently storing some relevant string */
    char *str = malloc (strlen (entry) + 1);
    strcpy (str, entry);
    return str;
}

static void
do_list_zipfile_dir (unzFile uf, xlsx_workbook * workbook)
{
/* parsing a Zipfile directory */
    unz_global_info64 gi;
    int ret;
    unsigned int i;
    ret = unzGetGlobalInfo64 (uf, &gi);
    if (ret != UNZ_OK)
      {
	  workbook->error = 1;
	  return;
      }

    for (i = 0; i < gi.number_entry; i++)
      {
	  /* looping on Zipfile entries */
	  char filename[256];
	  unz_file_info64 file_info;
	  ret =
	      unzGetCurrentFileInfo64 (uf, &file_info, filename, 256, NULL, 0,
				       NULL, 0);
	  if (ret != UNZ_OK)
	    {
		workbook->error = 1;
		return;
	    }

	  if (strcasecmp (filename, "xl/sharedStrings.xml") == 0)
	    {
		/* found SharedStrings */
		workbook->SharedStringsZipEntry = setString (filename);
	    }
	  if (strcasecmp (filename, "xl/workbook.xml") == 0)
	    {
		/* found Workbook */
		workbook->WorkbookZipEntry = setString (filename);
	    }
	  if (strcasecmp (filename, "xl/styles.xml") == 0)
	    {
		/* found Styles */
		workbook->StylesZipEntry = setString (filename);
	    }

	  if (i == gi.number_entry - 1)
	      break;
	  ret = unzGoToNextFile (uf);
	  if (ret != UNZ_OK)
	    {
		workbook->error = 1;
		return;
	    }
      }
}

static void
add_xlsx_row (xlsx_worksheet * worksheet, int row_no)
{
/* adding a row to a Worksheet */
    xlsx_row *row = malloc (sizeof (xlsx_row));
    row->row_no = row_no;
    row->max_cell = -1;
    row->first = NULL;
    row->last = NULL;
    row->next = NULL;

    if (worksheet->first == NULL)
	worksheet->first = row;
    if (worksheet->last != NULL)
	worksheet->last->next = row;
    worksheet->last = row;

    if (row_no > worksheet->max_row)
	worksheet->max_row = row_no;
}

static int
find_col_no (const char *sym)
{
/* attempting to get a column number */
    int i;
    int nro = 0;
    char x1 = 'A';
    char x2 = '\0';
    char dummy[8];
    char ref[128];

    strcpy (ref, sym);
    for (i = strlen (ref); i >= 0; i--)
      {
	  if (ref[i] >= '0' && ref[i] <= '9')
	      ref[i] = '\0';
      }
    if (strlen (ref) == 1 || strlen (ref) == 2)
	;
    else
	return -1;

    while (1)
      {
	  if (x2 == '\0')
	      sprintf (dummy, "%c", x1);
	  else
	      sprintf (dummy, "%c%c", x1, x2);
	  if (strcmp (dummy, ref) == 0)
	      return nro;
	  nro++;
	  if (x2 == '\0')
	    {
		if (x1 == 'Z')
		  {
		      x1 = 'A';
		      x2 = 'A';
		  }
		else
		    x1 += 1;
	    }
	  else
	    {
		if (x2 == 'Z' && x1 != 'Z')
		  {
		      x1 += 1;
		      x2 = 'A';
		  }
		else
		    x2 += 1;
	    }
      }
    return -1;
}

static void
add_xlsx_col (xlsx_worksheet * worksheet, int col_no, int type, int is_datetime)
{
/* adding a cell to a Worksheet row */
    xlsx_row *row;
    xlsx_cell *cell;

    row = worksheet->last;
    if (row == NULL)
      {
	  worksheet->error = 1;
	  return;
      }

    cell = malloc (sizeof (xlsx_cell));
    cell->col_no = col_no;
    cell->type = type;
    cell->is_datetime = is_datetime;
    cell->assigned = 0;
    cell->next = NULL;

    if (row->first == NULL)
	row->first = cell;
    if (row->last != NULL)
	row->last->next = cell;
    row->last = cell;

    if (col_no > row->max_cell)
	row->max_cell = col_no;
    if (col_no > worksheet->max_cell)
	worksheet->max_cell = col_no;
}

static int
xlsx_is_datetime (xlsx_workbook * workbook, int style)
{
/* checking for a DATETIME cell value */
    int is_datetime = 0;
    if (style >= 0 && style < workbook->n_styles)
      {
	  xlsx_style *stl = workbook->styles + style;
	  if (stl->formatRef != NULL)
	      is_datetime = stl->formatRef->is_datetime;
      }
    return is_datetime;
}

static void
sheet_start_tag (void *data, const char *el, const char **attr)
{
/* some generic XML tag starts here */
    const char **attrib = attr;
    int count = 0;
    const char *k;
    const char *v;
    xlsx_worksheet *worksheet = (xlsx_worksheet *) data;
    if (strcmp (el, "worksheet") == 0)
	worksheet->RowOk = 1;
    if (strcmp (el, "sheetData") == 0)
      {
	  if (worksheet->RowOk == 1)
	      worksheet->RowOk = 2;
	  else
	      worksheet->error = 1;
      }
    if (strcmp (el, "row") == 0)
      {
	  if (worksheet->RowOk == 2)
	    {
		int row_no = -1;
		while (*attrib != NULL)
		  {
		      if ((count % 2) == 0)
			  k = *attrib;
		      else
			{
			    v = *attrib;
			    if (strcmp (k, "r") == 0)
				row_no = atoi (v);
			}
		      attrib++;
		      count++;
		  }
		if (row_no > 0)
		  {
		      add_xlsx_row (worksheet, row_no);
		      worksheet->RowOk = 3;
		  }
		else
		    worksheet->error = 1;
	    }
	  else
	      worksheet->error = 1;
      }
    if (strcmp (el, "c") == 0)
      {
	  if (worksheet->RowOk == 3)
	    {
		const char *r = NULL;
		const char *t = NULL;
		int s = -1;
		int col_no = -1;
		int type = XLSX_NULL;
		attrib = attr;
		count = 0;
		while (*attrib != NULL)
		  {
		      if ((count % 2) == 0)
			  k = *attrib;
		      else
			{
			    v = *attrib;
			    if (strcmp (k, "r") == 0)
				r = v;
			    if (strcmp (k, "t") == 0)
				t = v;
			    if (strcmp (k, "s") == 0)
				s = atoi (v);
			}
		      attrib++;
		      count++;
		  }
		if (r != NULL)
		    col_no = find_col_no (r);
		if (col_no >= 0)
		  {
		      int is_datetime = XLSX_DATE_NONE;
		      if (s >= 0)
			  is_datetime = xlsx_is_datetime (worksheet->wbRef, s);
		      if (t != NULL)
			{
			    if (strcmp (t, "s") == 0)
				type = XLSX_STR_INDEX;
			    /*
			     * 'b' = BOOLEAN+
			     * 'd' = DATE ISO-8601
			     * 'e' = error
			     * 'inlineStr'
			     * 'n' = number
			     * 's' = shared string index
			     * 'str' = formula string
			     */
			    if (strcmp (t, "n") == 0)
				type = XLSX_INTEGER;
			}
		      add_xlsx_col (worksheet, col_no, type, is_datetime);
		      worksheet->ColOk = 1;
		  }
	    }
	  else
	      worksheet->error = 1;
      }
    if (strcmp (el, "v") == 0)
      {
	  if (worksheet->ColOk == 1)
	      worksheet->CellValueOk = 1;
      }
    *(worksheet->CharData) = '\0';
    worksheet->CharDataLen = 0;
}

static void
set_xlsx_cell_value (xlsx_worksheet * worksheet, const char *val)
{
/* assigning a value to the current cell */
    xlsx_row *row;
    xlsx_cell *cell;

    if (worksheet == NULL)
	return;
    row = worksheet->last;
    if (row == NULL)
	return;
    cell = row->last;
    if (cell == NULL)
	return;

    if (cell->type == XLSX_NULL && val != NULL)
      {
	  /* it could be an Integer or Double */
	  cell->type = XLSX_INTEGER;
      }

    if (cell->type == XLSX_STR_INDEX)
      {
	  cell->str_index = atoi (val);
	  cell->assigned = 1;
      }
    if (cell->type == XLSX_INTEGER)
      {
	  int i_val = atoi (val);
	  double d_val = atof (val);
	  if (i_val != d_val)
	    {
		cell->type = XLSX_DOUBLE;
		cell->dbl_value = d_val;
	    }
	  else
	      cell->int_value = i_val;
	  cell->assigned = 1;
      }
}

static void
sheet_end_tag (void *data, const char *el)
{
/* some generic XML tag ends here */
    xlsx_worksheet *worksheet = (xlsx_worksheet *) data;
    if (strcmp (el, "worksheet") == 0)
      {
	  if (worksheet->RowOk == 1)
	      worksheet->RowOk = 0;
	  else
	      worksheet->error = 1;
      }
    if (strcmp (el, "sheetData") == 0)
      {
	  if (worksheet->RowOk == 2)
	      worksheet->RowOk = 1;
	  else
	      worksheet->error = 1;
      }
    if (strcmp (el, "row") == 0)
      {
	  if (worksheet->RowOk == 3)
	      worksheet->RowOk = 2;
	  else
	      worksheet->error = 1;
      }
    if (strcmp (el, "c") == 0)
      {
	  if (worksheet->ColOk == 1)
	      worksheet->ColOk = 0;
	  else
	      worksheet->error = 1;
      }
    if (strcmp (el, "v") == 0)
      {
	  if (worksheet->CellValueOk == 1)
	    {
		char *in;
		*(worksheet->CharData + worksheet->CharDataLen) = '\0';
		in = worksheet->CharData;
		set_xlsx_cell_value (worksheet, in);
		worksheet->CellValueOk = 0;
	    }
	  else
	      worksheet->error = 1;
      }
}

static void
do_parse_worksheet (xlsx_worksheet * worksheet, unsigned char *buf,
		    uint64_t size_buf)
{
/* parsing SheetN.xml */
    XML_Parser parser;
    int done = 0;
    char *p_buf;
    uint64_t done_buf;
    int len;

    parser = XML_ParserCreate (NULL);
    if (!parser)
      {
	  worksheet->error = 1;
	  return;
      }

    XML_SetUserData (parser, worksheet);
    XML_SetElementHandler (parser, sheet_start_tag, sheet_end_tag);
    XML_SetCharacterDataHandler (parser, xmlCharDataSheet);
    p_buf = (char *) buf;
    if (size_buf < BUFSIZ)
	len = size_buf;
    else
	len = BUFSIZ;
    done_buf = 0;
    while (1)
      {
	  /* reading small chunks to avoid memory overflow */
	  if (!XML_Parse (parser, p_buf, len, done))
	      worksheet->error = 1;
	  /* next chunk */
	  done_buf += len;
	  p_buf += len;
	  if (done_buf >= size_buf)
	      break;
	  if (done_buf + BUFSIZ <= size_buf)
	      len = BUFSIZ;
	  else
	      len = size_buf - done_buf;
      }
    XML_ParserFree (parser);
}

static void
do_fetch_worksheet (unzFile uf, xlsx_worksheet * worksheet)
{
/* uncompressing SheetN.xml */
    int err;
    char filename[256];
    unz_file_info64 file_info;
    uint64_t size_buf;
    unsigned char *buf = NULL;
    int is_open = 0;
    uint64_t rd_cnt;
    uint64_t unrd_cnt;
    char *zip_entry = malloc (strlen ("xl/worksheets/sheet123456789.xml") + 1);
    sprintf (zip_entry, "xl/worksheets/sheet%d.xml", worksheet->id);

    err = unzLocateFile (uf, zip_entry, 0);
    if (err != UNZ_OK)
      {
	  worksheet->error = 1;
	  goto skip;
      }
    err =
	unzGetCurrentFileInfo64 (uf, &file_info, filename, 256, NULL, 0, NULL,
				 0);
    if (err != UNZ_OK)
      {
	  worksheet->error = 1;
	  goto skip;
      }
    size_buf = file_info.uncompressed_size;
    buf = malloc (size_buf);
    err = unzOpenCurrentFile (uf);
    if (err != UNZ_OK)
      {
	  worksheet->error = 1;
	  goto skip;
      }
    is_open = 1;
    rd_cnt = 0;
    while (rd_cnt < size_buf)
      {
	  /* reading big chunks so to avoid large file issues */
	  uint32_t max = 1000000000;	/* max chunk size */
	  uint32_t len;
	  unrd_cnt = size_buf - rd_cnt;
	  if (unrd_cnt < max)
	      len = unrd_cnt;
	  else
	      len = max;
	  err = unzReadCurrentFile (uf, buf + rd_cnt, len);
	  if (err < 0)
	    {
		worksheet->error = 1;
		goto skip;
	    }
	  rd_cnt += len;
      }

/* parsing SheetN.xml */
    do_parse_worksheet (worksheet, buf, size_buf);

  skip:
    if (zip_entry != NULL)
	free (zip_entry);
    if (buf != NULL)
	free (buf);
    if (is_open)
	unzCloseCurrentFile (uf);
}

static void
do_add_worksheet (xlsx_workbook * workbook, int id, char *name)
{
/* adding a Worksheet to the Workbook */
    xlsx_worksheet *ws = malloc (sizeof (xlsx_worksheet));
    ws->id = id;
    ws->name = name;
    ws->first = NULL;
    ws->last = NULL;
    ws->max_row = -1;
    ws->max_cell = -1;
    ws->rows = NULL;
    ws->error = 0;
    ws->CharDataStep = 65536;
    ws->CharDataMax = ws->CharDataStep;
    ws->CharData = malloc (ws->CharDataStep);
    ws->CharDataLen = 0;
    ws->RowOk = 0;
    ws->ColOk = 0;
    ws->wbRef = workbook;
    ws->next = NULL;
    if (workbook->first == NULL)
	workbook->first = ws;
    if (workbook->last != NULL)
	workbook->last->next = ws;
    workbook->last = ws;
}

static void
worksheets_start_tag (void *data, const char *el, const char **attr)
{
/* some generic XML tag starts here */
    const char **attrib = attr;
    int count = 0;
    const char *k;
    const char *v;
    xlsx_workbook *workbook = (xlsx_workbook *) data;
    if (strcmp (el, "workbook") == 0)
	workbook->WorksheetsOk = 1;
    if (strcmp (el, "sheets") == 0)
      {
	  if (workbook->WorksheetsOk == 1)
	      workbook->WorksheetsOk = 2;
	  else
	      workbook->error = 1;
      }
    if (strcmp (el, "sheet") == 0)
      {
	  int id = -1;
	  char *name = NULL;
	  if (workbook->WorksheetsOk != 2)
	    {
		workbook->error = 1;
		return;
	    }
	  while (*attrib != NULL)
	    {
		if ((count % 2) == 0)
		    k = *attrib;
		else
		  {
		      v = *attrib;
		      if (strcmp (k, "sheetId") == 0)
			  id = atoi (v);
		      if (strcmp (k, "name") == 0)
			  name = setString (v);
		  }
		attrib++;
		count++;
	    }
	  if (id > 0 && name != NULL)
	      do_add_worksheet (workbook, id, name);
	  else
	    {
		if (name != NULL)
		    free (name);
		workbook->error = 1;
	    }
      }
}

static void
worksheets_end_tag (void *data, const char *el)
{
/* some generic XML tag ends here */
    xlsx_workbook *workbook = (xlsx_workbook *) data;
    if (strcmp (el, "workbook") == 0)
      {
	  if (workbook->WorksheetsOk == 1)
	      workbook->WorksheetsOk = 0;
	  else
	      workbook->error = 1;
      }
    if (strcmp (el, "sheets") == 0)
      {
	  if (workbook->WorksheetsOk == 2)
	      workbook->WorksheetsOk = 1;
	  else
	      workbook->error = 1;
      }
}

static void
do_parse_xlsx_worksheets (xlsx_workbook * workbook, unsigned char *buf,
			  uint64_t size_buf)
{
/* parsing Workbook.xml */
    XML_Parser parser;
    int done = 0;

    parser = XML_ParserCreate (NULL);
    if (!parser)
      {
	  workbook->error = 1;
	  return;
      }

    XML_SetUserData (parser, workbook);
    XML_SetElementHandler (parser, worksheets_start_tag, worksheets_end_tag);
    XML_SetCharacterDataHandler (parser, xmlCharData);
    if (!XML_Parse (parser, (char *) buf, size_buf, done))
	workbook->error = 1;
    XML_ParserFree (parser);
}

static void
do_fetch_xlsx_worksheets (unzFile uf, xlsx_workbook * workbook)
{
/* uncompressing Workbook.xml */
    int err;
    char filename[256];
    unz_file_info64 file_info;
    uint64_t size_buf;
    unsigned char *buf = NULL;
    int is_open = 0;
    uint64_t rd_cnt;
    uint64_t unrd_cnt;

    err = unzLocateFile (uf, workbook->WorkbookZipEntry, 0);
    if (err != UNZ_OK)
      {
	  workbook->error = 1;
	  goto skip;
      }
    err =
	unzGetCurrentFileInfo64 (uf, &file_info, filename, 256, NULL, 0, NULL,
				 0);
    if (err != UNZ_OK)
      {
	  workbook->error = 1;
	  goto skip;
      }
    size_buf = file_info.uncompressed_size;
    buf = malloc (size_buf);
    err = unzOpenCurrentFile (uf);
    if (err != UNZ_OK)
      {
	  workbook->error = 1;
	  goto skip;
      }
    is_open = 1;
    rd_cnt = 0;
    while (rd_cnt < size_buf)
      {
	  /* reading big chunks so to avoid large file issues */
	  uint32_t max = 1000000000;	/* max chunk size */
	  uint32_t len;
	  unrd_cnt = size_buf - rd_cnt;
	  if (unrd_cnt < max)
	      len = unrd_cnt;
	  else
	      len = max;
	  err = unzReadCurrentFile (uf, buf + rd_cnt, len);
	  if (err < 0)
	    {
		workbook->error = 1;
		goto skip;
	    }
	  rd_cnt += len;
      }

/* parsing Workbook.xml */
    do_parse_xlsx_worksheets (workbook, buf, size_buf);

  skip:
    if (buf != NULL)
	free (buf);
    if (is_open)
	unzCloseCurrentFile (uf);
}

static void
shared_strings_start_tag (void *data, const char *el, const char **attr)
{
/* some generic XML tag starts here */
    const char **attrib = attr;
    int count = 0;
    const char *k;
    const char *v;
    xlsx_workbook *workbook = (xlsx_workbook *) data;
    if (strcmp (el, "sst") == 0)
      {
	  while (*attrib != NULL)
	    {
		if ((count % 2) == 0)
		    k = *attrib;
		else
		  {
		      v = *attrib;
		      if (strcmp (k, "uniqueCount") == 0)
			  workbook->n_strings = atoi (v);
		  }
		attrib++;
		count++;
	    }
	  if (workbook->n_strings > 0)
	    {
		/* allocating the SharedStrings array */
		int i;
		workbook->strings =
		    malloc (sizeof (char *) * workbook->n_strings);
		for (i = 0; i < workbook->n_strings; i++)
		    *(workbook->strings + i) = NULL;
	    }
	  workbook->SharedStringsOk = 1;
      }
    *(workbook->CharData) = '\0';
    workbook->CharDataLen = 0;
}

static void
shared_strings_end_tag (void *data, const char *el)
{
/* some generic XML tag ends here */
    xlsx_workbook *workbook = (xlsx_workbook *) data;
    if (strcmp (el, "sst") == 0)
      {
	  if (workbook->n_strings != workbook->xml_strings)
	      workbook->error = 1;
	  workbook->SharedStringsOk = 0;
      }
    if (strcmp (el, "si") == 0)
      {
	  if (workbook->SharedStringsOk == 0)
	    {
		workbook->error = 1;
		return;
	    }
	  if (workbook->xml_strings < workbook->n_strings)
	    {
		char *in;
		*(workbook->CharData + workbook->CharDataLen) = '\0';
		in = workbook->CharData;
		*(workbook->strings + workbook->xml_strings) =
		    malloc (strlen (in) + 1);
		strcpy (*(workbook->strings + workbook->xml_strings), in);
		workbook->xml_strings += 1;
	    }
	  else
	      workbook->error = 1;
      }
}

static void
do_parse_xlsx_shared_strings (xlsx_workbook * workbook, unsigned char *buf,
			      uint64_t size_buf)
{
/* parsing SharedStrings.xml */
    XML_Parser parser;
    int done = 0;

    parser = XML_ParserCreate (NULL);
    if (!parser)
      {
	  workbook->error = 1;
	  return;
      }

    XML_SetUserData (parser, workbook);
    XML_SetElementHandler (parser, shared_strings_start_tag,
			   shared_strings_end_tag);
    XML_SetCharacterDataHandler (parser, xmlCharData);
    if (!XML_Parse (parser, (char *) buf, size_buf, done))
	workbook->error = 1;
    XML_ParserFree (parser);
}

static void
do_fetch_xlsx_shared_strings (unzFile uf, xlsx_workbook * workbook)
{
/* uncompressing SharedStrings.xml */
    int err;
    char filename[256];
    unz_file_info64 file_info;
    uint64_t size_buf;
    unsigned char *buf = NULL;
    int is_open = 0;
    uint64_t rd_cnt;
    uint64_t unrd_cnt;

    err = unzLocateFile (uf, workbook->SharedStringsZipEntry, 0);
    if (err != UNZ_OK)
      {
	  workbook->error = 1;
	  goto skip;
      }
    err =
	unzGetCurrentFileInfo64 (uf, &file_info, filename, 256, NULL, 0, NULL,
				 0);
    if (err != UNZ_OK)
      {
	  workbook->error = 1;
	  goto skip;
      }
    size_buf = file_info.uncompressed_size;
    buf = malloc (size_buf);
    err = unzOpenCurrentFile (uf);
    if (err != UNZ_OK)
      {
	  workbook->error = 1;
	  goto skip;
      }
    is_open = 1;
    rd_cnt = 0;
    while (rd_cnt < size_buf)
      {
	  /* reading big chunks so to avoid large file issues */
	  uint32_t max = 1000000000;	/* max chunk size */
	  uint32_t len;
	  unrd_cnt = size_buf - rd_cnt;
	  if (unrd_cnt < max)
	      len = unrd_cnt;
	  else
	      len = max;
	  err = unzReadCurrentFile (uf, buf + rd_cnt, len);
	  if (err < 0)
	    {
		workbook->error = 1;
		goto skip;
	    }
	  rd_cnt += len;
      }

/* parsing SharedStrings.xml */
    do_parse_xlsx_shared_strings (workbook, buf, size_buf);

  skip:
    if (buf != NULL)
	free (buf);
    if (is_open)
	unzCloseCurrentFile (uf);
}

static int
parse_xlsx_format (const char *code)
{
/* parsing a XLSX format string */
    int is_date = 0;
    int is_time = 0;
    int i;
    int len = strlen (code);
    for (i = 0; i < len; i++)
      {
	  if (code[i] == 'd' || code[i] == 'm' || code[i] == 'y')
	      is_date = 1;
	  if (code[i] == 'h' || code[i] == 's')
	      is_time = 1;
      }
    if (is_date && is_time)
	return XLSX_DATE_AND_TIME;
    if (is_date)
	return XLSX_DATE_SIMPLE;
    if (is_time)
	return XLSX_TIME_SIMPLE;
    return XLSX_DATE_NONE;
}

static xlsx_format *
find_xlsx_format (xlsx_workbook * workbook, int id)
{
/* searching for a given Format */
    int i;
    for (i = 0; i < workbook->n_formats; i++)
      {
	  xlsx_format *fmt = workbook->formats + i;
	  if (fmt->fmtId == id)
	      return fmt;
      }
    return NULL;
}

static void
styles_start_tag (void *data, const char *el, const char **attr)
{
/* some generic XML tag starts here */
    const char **attrib = attr;
    int count = 0;
    const char *k;
    const char *v;
    xlsx_workbook *workbook = (xlsx_workbook *) data;
    if (strcmp (el, "styleSheet") == 0)
	workbook->StylesOk = 1;
    if (strcmp (el, "numFmts") == 0)
      {
	  if (workbook->StylesOk == 1 && workbook->FormatsOk == 0)
	    {
		while (*attrib != NULL)
		  {
		      if ((count % 2) == 0)
			  k = *attrib;
		      else
			{
			    v = *attrib;
			    if (strcmp (k, "count") == 0)
				workbook->n_formats = atoi (v);
			}
		      attrib++;
		      count++;
		  }
		if (workbook->n_formats > 0)
		  {
		      /* allocating the Formats array */
		      int i;
		      workbook->formats =
			  malloc (sizeof (xlsx_format) * workbook->n_formats);
		      for (i = 0; i < workbook->n_formats; i++)
			{
			    xlsx_format *fmt = workbook->formats + i;
			    fmt->fmtId = -1;
			    fmt->is_datetime = 0;
			}
		  }
		workbook->next_format = 0;
		workbook->FormatsOk = 1;
	    }
	  else
	      workbook->error = 1;
      }
    if (strcmp (el, "numFmt") == 0)
      {
	  if (workbook->StylesOk == 1 && workbook->FormatsOk == 1
	      && workbook->next_format >= 0
	      && workbook->next_format < workbook->n_formats)
	    {
		int id = -1;
		const char *code;
		while (*attrib != NULL)
		  {
		      if ((count % 2) == 0)
			  k = *attrib;
		      else
			{
			    v = *attrib;
			    if (strcmp (k, "numFmtId") == 0)
				id = atoi (v);
			    if (strcmp (k, "formatCode") == 0)
				code = v;
			}
		      attrib++;
		      count++;
		  }
		if (id >= 0 && code != NULL)
		  {
		      xlsx_format *fmt =
			  workbook->formats + workbook->next_format;
		      fmt->fmtId = id;
		      fmt->is_datetime = parse_xlsx_format (code);
		  }
		workbook->next_format += 1;
	    }
	    /*
	     * avoiding to raise an error because some XLSX created
	     * by MS Excel itself present such a combination
	     * 
	  else
	      workbook->error = 1;
	      */
      }
    if (strcmp (el, "cellXfs") == 0)
      {
	  if (workbook->StylesOk == 1 && workbook->CellStylesOk == 0)
	    {
		while (*attrib != NULL)
		  {
		      if ((count % 2) == 0)
			  k = *attrib;
		      else
			{
			    v = *attrib;
			    if (strcmp (k, "count") == 0)
				workbook->n_styles = atoi (v);
			}
		      attrib++;
		      count++;
		  }
		if (workbook->n_styles > 0)
		  {
		      /* allocating the Styles array */
		      int i;
		      workbook->styles =
			  malloc (sizeof (xlsx_style) * workbook->n_styles);
		      for (i = 0; i < workbook->n_styles; i++)
			{
			    xlsx_style *stl = workbook->styles + i;
			    stl->stlId = -1;
			    stl->formatRef = NULL;
			}
		  }
		workbook->next_style = 0;
		workbook->CellStylesOk = 1;
	    }
	  else
	      workbook->error = 1;
      }
    if (strcmp (el, "xf") == 0)
      {
	  if (workbook->StylesOk == 1 && workbook->CellStylesOk == 1)
	    {
		if (workbook->next_style >= 0
		    && workbook->next_style < workbook->n_styles)
		  {
		      int id = -1;
		      while (*attrib != NULL)
			{
			    if ((count % 2) == 0)
				k = *attrib;
			    else
			      {
				  v = *attrib;
				  if (strcmp (k, "numFmtId") == 0)
				      id = atoi (v);
			      }
			    attrib++;
			    count++;
			}
		      if (id >= 0)
			{
			    xlsx_style *stl =
				workbook->styles + workbook->next_style;
			    stl->stlId = id;
			    stl->formatRef = find_xlsx_format (workbook, id);
			}
		      workbook->next_style += 1;
		  }
		else
		    workbook->error = 1;
	    }
      }
}

static void
styles_end_tag (void *data, const char *el)
{
/* some generic XML tag ends here */
    xlsx_workbook *workbook = (xlsx_workbook *) data;
    if (strcmp (el, "styleSheet") == 0)
      {
	  if (workbook->StylesOk == 1)
	      workbook->StylesOk = 0;
	  else
	      workbook->error = 1;
      }
    if (strcmp (el, "numFmts") == 0)
      {
	  if (workbook->FormatsOk == 1)
	      workbook->FormatsOk = 0;
	  else
	      workbook->error = 1;
      }
    if (strcmp (el, "cellXfs") == 0)
      {
	  if (workbook->CellStylesOk == 1)
	      workbook->CellStylesOk = 0;
	  else
	      workbook->error = 1;
      }
}

static void
do_parse_xlsx_styles (xlsx_workbook * workbook, unsigned char *buf,
		      uint64_t size_buf)
{
/* parsing Styles.xml */
    XML_Parser parser;
    int done = 0;

    parser = XML_ParserCreate (NULL);
    if (!parser)
      {
	  workbook->error = 1;
	  return;
      }

    XML_SetUserData (parser, workbook);
    XML_SetElementHandler (parser, styles_start_tag, styles_end_tag);
    XML_SetCharacterDataHandler (parser, xmlCharData);
    if (!XML_Parse (parser, (char *) buf, size_buf, done))
	workbook->error = 1;
    XML_ParserFree (parser);
}

static void
do_fetch_xlsx_styles (unzFile uf, xlsx_workbook * workbook)
{
/* uncompressing Styles.xml */
    int err;
    char filename[256];
    unz_file_info64 file_info;
    uint64_t size_buf;
    unsigned char *buf = NULL;
    int is_open = 0;
    uint64_t rd_cnt;
    uint64_t unrd_cnt;

    err = unzLocateFile (uf, workbook->StylesZipEntry, 0);
    if (err != UNZ_OK)
      {
	  workbook->error = 1;
	  goto skip;
      }
    err =
	unzGetCurrentFileInfo64 (uf, &file_info, filename, 256, NULL, 0, NULL,
				 0);
    if (err != UNZ_OK)
      {
	  workbook->error = 1;
	  goto skip;
      }
    size_buf = file_info.uncompressed_size;
    buf = malloc (size_buf);
    err = unzOpenCurrentFile (uf);
    if (err != UNZ_OK)
      {
	  workbook->error = 1;
	  goto skip;
      }
    is_open = 1;
    rd_cnt = 0;
    while (rd_cnt < size_buf)
      {
	  /* reading big chunks so to avoid large file issues */
	  uint32_t max = 1000000000;	/* max chunk size */
	  uint32_t len;
	  unrd_cnt = size_buf - rd_cnt;
	  if (unrd_cnt < max)
	      len = unrd_cnt;
	  else
	      len = max;
	  err = unzReadCurrentFile (uf, buf + rd_cnt, len);
	  if (err < 0)
	    {
		workbook->error = 1;
		goto skip;
	    }
	  rd_cnt += len;
      }

/* parsing Syles.xml */
    do_parse_xlsx_styles (workbook, buf, size_buf);

  skip:
    if (buf != NULL)
	free (buf);
    if (is_open)
	unzCloseCurrentFile (uf);
}

FREEXL_DECLARE int
freexl_open_xlsx (const char *path, const void **xl_handle)
{
/* opening and initializing the Workbook - XLSX format expected */
    xlsx_workbook *workbook;
    xlsx_worksheet *worksheet;
    freexl_handle **handle = (freexl_handle **) xl_handle;
    unzFile uf = NULL;
    int retval = 0;

/* opening the XLSX Spreadsheet as a Zipfile */
    uf = unzOpen64 (path);
    if (uf == NULL)
	return FREEXL_FILE_NOT_FOUND;

    retval = FREEXL_OK;
    *handle = malloc (sizeof (freexl_handle));
    (*handle)->xls_handle = NULL;
    (*handle)->xlsx_handle = NULL;
    (*handle)->ods_handle = NULL;

/* allocating the Workbook struct */
    workbook = alloc_workbook ();
    if (!workbook)
	return FREEXL_INSUFFICIENT_MEMORY;
/* parsing the Zipfile directory */
    do_list_zipfile_dir (uf, workbook);
    if (workbook->error)
      {
	  destroy_workbook (workbook);
	  retval = FREEXL_INVALID_XLSX;
	  goto stop;
      }

    if (workbook->SharedStringsZipEntry != NULL)
      {
	  /* parsing SharedStrings.xml */
	  do_fetch_xlsx_shared_strings (uf, workbook);
	  if (workbook->error)
	    {
		destroy_workbook (workbook);
		retval = FREEXL_INVALID_XLSX;
		goto stop;
	    }
      }
    if (workbook->StylesZipEntry != NULL)
      {
	  /* parsing Styles.xml */
	  do_fetch_xlsx_styles (uf, workbook);
	  if (workbook->error)
	    {
		destroy_workbook (workbook);
		retval = FREEXL_INVALID_XLSX;
		goto stop;
	    }
      }
    if (workbook->WorkbookZipEntry != NULL)
      {
	  /* parsing Workbook.xml */
	  do_fetch_xlsx_worksheets (uf, workbook);
	  if (workbook->error)
	    {
		destroy_workbook (workbook);
		retval = FREEXL_INVALID_XLSX;
		goto stop;
	    }
      }
    worksheet = workbook->first;
    while (worksheet != NULL)
      {
	  /* parsing all Worksheets */
	  do_fetch_worksheet (uf, worksheet);
	  if (worksheet->error)
	    {
		destroy_workbook (workbook);
		retval = FREEXL_INVALID_XLSX;
		goto stop;
	    }
	  worksheet = worksheet->next;
      }
    if (workbook->error == 0)
      {
	  /* adjusting all Worksheets */
	  xlsx_worksheet *ws = workbook->first;
	  while (ws != NULL)
	    {
		int max_col_no = -1;
		xlsx_cell *cell;
		xlsx_row *row = ws->first;
		ws->max_row = -1;
		ws->max_cell = -1;
		while (row != NULL)
		  {
		      max_col_no = -1;
		      cell = row->first;
		      row->max_cell = -1;
		      while (cell != NULL)
			{
			    if (cell->assigned && cell->type != XLSX_NULL)
			      {
				  if (cell->col_no > max_col_no)
				      max_col_no = cell->col_no;
			      }
			    cell = cell->next;
			}
		      if (max_col_no >= 0)
			{
			    row->max_cell = max_col_no;
			    if (row->row_no > ws->max_row)
				ws->max_row = row->row_no;
			    if (row->max_cell > ws->max_cell)
				ws->max_cell = row->max_cell;
			}
		      row = row->next;
		  }
		if (ws->max_row > 0)
		  {
		      /* creating and populating the ROWS Array */
		      int i;
		      ws->rows =
			  malloc (sizeof (ods_row *) * (ws->max_row + 1));
		      for (i = 0; i < ws->max_row; i++)
			  *(ws->rows + i) = NULL;
		      row = ws->first;
		      while (row != NULL)
			{
			    max_col_no = -1;
			    cell = row->first;
			    while (cell != NULL)
			      {
				  if (cell->assigned && cell->type != ODS_VOID)
				    {
					if (cell->col_no > max_col_no)
					    max_col_no = cell->col_no;
				    }
				  cell = cell->next;
			      }
			    if (max_col_no >= 0)
			      {
				  if (row->row_no > 0)
				      *(ws->rows + row->row_no - 1) = row;
			      }
			    row = row->next;
			}
		  }
		ws = ws->next;
	    }
      }
    (*handle)->xlsx_handle = workbook;

/* closing the Zipfile and quitting */
  stop:
    unzClose (uf);
    return retval;
}

FREEXL_DECLARE int
freexl_close_xlsx (const void *xl_handle)
{
/* attempting to destroy the Workbook */
    freexl_handle *handle = (freexl_handle *) xl_handle;
    if (!handle)
	return FREEXL_NULL_HANDLE;
    if (handle->xlsx_handle == NULL)
	return FREEXL_INVALID_HANDLE;

/* destroying the workbook */
    destroy_workbook (handle->xlsx_handle);
/* destroying the handle */
    free (handle);

    return FREEXL_OK;
}

#endif /* end conditional XML support */

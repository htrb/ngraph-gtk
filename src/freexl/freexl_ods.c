/* 
/ freexl_ods.c
/
/ FreeXL implementation (ODS support)
/
/ version  2.0, 2021 June 4
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
/* parsing XML char data */
    ods_workbook *workbook = (ods_workbook *) data;
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

static ods_workbook *
alloc_workbook ()
{
/* allocating and initializing the Workbook struct */
    ods_workbook *wb = malloc (sizeof (ods_workbook));
    if (!wb)
	return NULL;
    wb->first = NULL;
    wb->last = NULL;
    wb->active_sheet = NULL;
    wb->first_date = NULL;
    wb->last_date = NULL;
    wb->error = 0;
    wb->ContentZipEntry = NULL;
    wb->CharDataStep = 65536;
    wb->CharDataMax = wb->CharDataStep;
    wb->CharData = malloc (wb->CharDataStep);
    wb->CharDataLen = 0;
    wb->ContentOk = 0;
    wb->NextWorksheetId = 0;
    return wb;
}

static void
destroy_row (ods_row * row)
{
/* memory cleanup - destroying a WorkSheet Row */
    ods_cell *col;
    ods_cell *col_n;
    if (row == NULL)
	return;

    col = row->first;
    while (col != NULL)
      {
	  col_n = col->next;
	  if (col->txt_value != NULL)
	      free (col->txt_value);
	  free (col);
	  col = col_n;
      }
    free (row);
}

static void
destroy_worksheet (ods_worksheet * ws)
{
/* memory cleanup - destroying a WorkSheet object */
    ods_row *row;
    ods_row *row_n;
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
    free (ws);
}

static void
destroy_workbook (ods_workbook * wb)
{
/* memory cleanup - destroying a WorkBook object */
    ods_worksheet *ws;
    ods_worksheet *ws_n;
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
    if (wb->ContentZipEntry != NULL)
	free (wb->ContentZipEntry);
    if (wb->CharData != NULL)
	free (wb->CharData);
    free (wb);
}

static void
do_add_cell (ods_worksheet * worksheet, int type, const char *value)
{
/* adding a Cell to the Worksheet Row */
    ods_row *row;
    ods_cell *cell;
    int len;
    int int_val;
    double dbl_val;

    row = worksheet->last;
    if (row == NULL)
	return;
    if (type == ODS_VOID)
      {
	  /* empty cell; just increasing the column number */
	  row->NextColNo += 1;
	  return;
      }

    cell = malloc (sizeof (ods_cell));
    cell->col_no = row->NextColNo;
    row->NextColNo += 1;
    cell->type = type;
    cell->assigned = 0;
    cell->txt_value = NULL;
    cell->next = NULL;
    if (value != NULL)
      {
	  switch (cell->type)
	    {
	    case ODS_DATE:
	    case ODS_TIME:
	    case ODS_STRING:
		len = strlen (value);
		cell->txt_value = malloc (len + 1);
		strcpy (cell->txt_value, value);
		cell->assigned = 1;
		break;
	    case ODS_FLOAT:
		int_val = atoi (value);
		dbl_val = atof (value);
		if (int_val == dbl_val)
		  {
		      cell->type = ODS_INTEGER;
		      cell->int_value = int_val;
		  }
		else
		    cell->dbl_value = atof (value);
		cell->assigned = 1;
		break;
	    case ODS_CURRENCY:
	    case ODS_PERCENTAGE:
		cell->dbl_value = atof (value);
		cell->assigned = 1;
		break;
	    case ODS_BOOLEAN:
		if (strcmp (value, "true") == 0)
		    cell->int_value = 1;
		else
		    cell->int_value = 0;
		cell->assigned = 0;
		break;
	    };
      }
    if (row->first == NULL)
	row->first = cell;
    if (row->last != NULL)
	row->last->next = cell;
    row->last = cell;

    if (cell->col_no > row->max_cell)
	row->max_cell = cell->col_no;
    if (cell->col_no > worksheet->max_cell)
	worksheet->max_cell = cell->col_no;
}

static void
do_add_row (ods_worksheet * worksheet)
{
/* adding a Row to the Worksheet */
    ods_row *row = malloc (sizeof (ods_row));
    row->row_no = worksheet->NextRowNo;
    worksheet->NextRowNo += 1;
    row->max_cell = -1;
    row->first = NULL;
    row->last = NULL;
    row->NextColNo = 0;
    row->next = NULL;
    if (worksheet->first == NULL)
	worksheet->first = row;
    if (worksheet->last != NULL)
	worksheet->last->next = row;
    worksheet->last = row;

    if (row->row_no > worksheet->max_row)
	worksheet->max_row = row->row_no;
}

static void
do_add_worksheet (ods_workbook * workbook, char *name)
{
/* adding a Worksheet to the Workbook */
    ods_worksheet *ws = malloc (sizeof (ods_worksheet));
    ws->id = workbook->NextWorksheetId;
    workbook->NextWorksheetId += 1;
    ws->name = name;
    ws->first = NULL;
    ws->last = NULL;
    ws->max_row = -1;
    ws->max_cell = -1;
    ws->rows = NULL;
    ws->RowOk = 0;
    ws->ColOk = 0;
    ws->CellValueOk = 0;
    ws->NextRowNo = 1;
    ws->next = NULL;
    if (workbook->first == NULL)
	workbook->first = ws;
    if (workbook->last != NULL)
	workbook->last->next = ws;
    workbook->last = ws;
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
do_list_zipfile_dir (unzFile uf, ods_workbook * workbook)
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

	  if (strcasecmp (filename, "content.xml") == 0)
	    {
		/* found Workbook */
		workbook->ContentZipEntry = setString (filename);
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
start_tag (void *data, const char *el, const char **attr)
{
/* some generic XML tag starts here */
    const char **attrib = attr;
    int count = 0;
    const char *k;
    const char *v;
    ods_workbook *workbook = (ods_workbook *) data;
    if (strcmp (el, "office:document-content") == 0)
	workbook->ContentOk = 1;
    if (strcmp (el, "office:body") == 0)
      {
	  if (workbook->ContentOk == 1)
	      workbook->ContentOk = 2;
	  else
	      workbook->error = 1;
      }
    if (strcmp (el, "office:spreadsheet") == 0)
      {
	  if (workbook->ContentOk == 2)
	      workbook->ContentOk = 3;
	  else
	      workbook->error = 1;
      }
    if (strcmp (el, "table:table") == 0)
      {
	  if (workbook->ContentOk == 3)
	    {
		char *name = NULL;
		workbook->ContentOk = 4;
		count = 0;
		while (*attrib != NULL)
		  {
		      if ((count % 2) == 0)
			  k = *attrib;
		      else
			{
			    v = *attrib;
			    if (strcmp (k, "table:name") == 0)
				name = setString (v);
			}
		      attrib++;
		      count++;
		  }
		if (name != NULL)
		    do_add_worksheet (workbook, name);
		else
		    workbook->error = 1;
	    }
	  else
	      workbook->error = 1;
      }
    if (strcmp (el, "table:table-row") == 0)
      {
	  ods_worksheet *worksheet = workbook->last;
	  if (worksheet == NULL)
	      workbook->error = 1;
	  else
	    {
		if (workbook->ContentOk == 4 && worksheet->RowOk == 0)
		  {
		      int i;
		      int repeated = 1;
		      count = 0;
		      while (*attrib != NULL)
			{
			    if ((count % 2) == 0)
				k = *attrib;
			    else
			      {
				  v = *attrib;
				  if (strcmp (k, "table:number-rows-repeated")
				      == 0)
				      repeated = atoi (v);
			      }
			    attrib++;
			    count++;
			}
		      if (repeated > 1024)
			{
			    /* exceeding repetitions; should be near the bottom of worksheet */
			    repeated = 1;
			}
		      for (i = 0; i < repeated; i++)
			  do_add_row (worksheet);
		      worksheet->RowOk = 1;
		  }
		else
		    workbook->error = 1;
	    }
      }
    if (strcmp (el, "table:table-cell") == 0
	|| strcmp (el, "table:covered-table-cell") == 0)
      {
	  ods_worksheet *worksheet = workbook->last;
	  if (worksheet == NULL)
	      workbook->error = 1;
	  else
	    {
		if (workbook->ContentOk == 4 && worksheet->RowOk == 1
		    && worksheet->ColOk == 0)
		  {
		      int i;
		      int repeated = 1;
		      int type = ODS_VOID;
		      char *xtype = NULL;
		      char *value = NULL;
		      count = 0;
		      while (*attrib != NULL)
			{
			    if ((count % 2) == 0)
				k = *attrib;
			    else
			      {
				  v = *attrib;
				  if (strcmp (k, "office:value-type") == 0)
				      xtype = setString (v);
				  if (strcmp (k, "office:value") == 0)
				      value = setString (v);
				  if (strcmp (k, "office:string-value") == 0)
				      value = setString (v);
				  if (strcmp (k, "office:boolean-value") == 0)
				      value = setString (v);
				  if (strcmp (k, "office:date-value") == 0)
				      value = setString (v);
				  if (strcmp (k, "office:time-value") == 0)
				      value = setString (v);
				  if (strcmp
				      (k, "table:number-columns-repeated") == 0)
				      repeated = atoi (v);
			      }
			    attrib++;
			    count++;
			}
		      if (xtype != NULL)
			{
			    if (strcmp (xtype, "string") == 0)
				type = ODS_STRING;
			    if (strcmp (xtype, "float") == 0)
				type = ODS_FLOAT;
			    if (strcmp (xtype, "currency") == 0)
				type = ODS_CURRENCY;
			    if (strcmp (xtype, "percentage") == 0)
				type = ODS_PERCENTAGE;
			    if (strcmp (xtype, "boolean") == 0)
				type = ODS_BOOLEAN;
			    if (strcmp (xtype, "date") == 0)
				type = ODS_DATE;
			    if (strcmp (xtype, "time") == 0)
				type = ODS_TIME;
			}
		      for (i = 0; i < repeated; i++)
			  do_add_cell (worksheet, type, value);
		      worksheet->ColOk = 1;
		      if (xtype != NULL)
			  free (xtype);
		      if (value != NULL)
			  free (value);
		  }
		else
		    workbook->error = 1;
	    }
      }
    if (strcmp (el, "text:p") == 0)
      {
	  ods_worksheet *worksheet = workbook->last;
	  if (worksheet == NULL)
	      workbook->error = 1;
	  else
	    {
		if (workbook->ContentOk == 4 && worksheet->RowOk == 1
		    && worksheet->ColOk == 1 && worksheet->CellValueOk == 0)
		    worksheet->CellValueOk = 1;
		else
		    workbook->error = 1;
	    }
      }
    *(workbook->CharData) = '\0';
    workbook->CharDataLen = 0;
}

static void
set_ods_cell_value (ods_worksheet * worksheet, const char *val)
{
/* assigning a value to the current cell */
    ods_row *row;
    ods_cell *cell;
    int len;

    if (worksheet == NULL)
	return;
    row = worksheet->last;
    if (row == NULL)
	return;
    cell = row->last;
    if (cell == NULL)
	return;
    if (cell->type != ODS_STRING)
	return;

    len = strlen (val);
    cell->txt_value = malloc (len + 1);
    strcpy (cell->txt_value, val);
    cell->assigned = 1;
}

static void
end_tag (void *data, const char *el)
{
/* some generic XML tag ends here */
    ods_workbook *workbook = (ods_workbook *) data;
    if (strcmp (el, "office:document-content") == 0)
      {
	  if (workbook->ContentOk == 1)
	      workbook->ContentOk = 0;
	  else
	      workbook->error = 1;
      }
    if (strcmp (el, "office:body") == 0)
      {
	  if (workbook->ContentOk == 2)
	      workbook->ContentOk = 1;
	  else
	      workbook->error = 1;
      }
    if (strcmp (el, "office:spreadsheet") == 0)
      {
	  if (workbook->ContentOk == 3)
	      workbook->ContentOk = 2;
	  else
	      workbook->error = 1;
      }
    if (strcmp (el, "table:table") == 0)
      {
	  if (workbook->ContentOk == 4)
	      workbook->ContentOk = 3;
	  else
	      workbook->error = 1;
      }
    if (strcmp (el, "table:table-row") == 0)
      {
	  ods_worksheet *worksheet = workbook->last;
	  if (worksheet == NULL)
	      workbook->error = 1;
	  else
	    {
		if (worksheet->RowOk == 1)
		    worksheet->RowOk = 0;
		else
		    workbook->error = 1;
	    }
      }
    if (strcmp (el, "table:table-cell") == 0
	|| strcmp (el, "table:covered-table-cell") == 0)
      {
	  ods_worksheet *worksheet = workbook->last;
	  if (worksheet == NULL)
	      workbook->error = 1;
	  else
	    {
		if (worksheet->ColOk == 1)
		    worksheet->ColOk = 0;
		else
		    workbook->error = 1;
	    }
      }
    if (strcmp (el, "text:p") == 0)
      {
	  ods_worksheet *worksheet = workbook->last;
	  if (worksheet == NULL)
	      workbook->error = 1;
	  else
	    {
		if (worksheet->CellValueOk == 1)
		  {
		      char *in;
		      *(workbook->CharData + workbook->CharDataLen) = '\0';
		      in = workbook->CharData;
		      set_ods_cell_value (worksheet, in);
		      worksheet->CellValueOk = 0;
		  }
		else
		    workbook->error = 1;
	    }
      }
}

static void
do_parse_ods_worksheets (ods_workbook * workbook, unsigned char *buf,
			 uint64_t size_buf)
{
/* parsing Content.xml */
    XML_Parser parser;
    int done = 0;

    parser = XML_ParserCreate (NULL);
    if (!parser)
      {
	  workbook->error = 1;
	  return;
      }

    XML_SetUserData (parser, workbook);
    XML_SetElementHandler (parser, start_tag, end_tag);
    XML_SetCharacterDataHandler (parser, xmlCharData);
    if (!XML_Parse (parser, (char *) buf, size_buf, done))
	workbook->error = 1;
    XML_ParserFree (parser);
}

static void
do_fetch_ods_worksheets (unzFile uf, ods_workbook * workbook)
{
/* uncompressing Content.xml */
    int err;
    char filename[256];
    unz_file_info64 file_info;
    uint64_t size_buf;
    unsigned char *buf = NULL;
    int is_open = 0;
    uint64_t rd_cnt;
    uint64_t unrd_cnt;

    err = unzLocateFile (uf, workbook->ContentZipEntry, 0);
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
    do_parse_ods_worksheets (workbook, buf, size_buf);
    if (workbook->error == 0)
      {
	  /* adjusting all Worksheets */
	  ods_worksheet *ws = workbook->first;
	  while (ws != NULL)
	    {
		int max_col_no = -1;
		ods_cell *cell;
		ods_row *row = ws->first;
		ws->max_row = -1;
		ws->max_cell = -1;
		while (row != NULL)
		  {
		      max_col_no = -1;
		      cell = row->first;
		      row->max_cell = -1;
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

  skip:
    if (buf != NULL)
	free (buf);
    if (is_open)
	unzCloseCurrentFile (uf);
}

FREEXL_DECLARE int
freexl_open_ods (const char *path, const void **xl_handle)
{
/* opening and initializing the Workbook - XLSX format expected */
    ods_workbook *workbook;
    freexl_handle **handle = (freexl_handle **) xl_handle;
    unzFile uf = NULL;
    int retval = 0;

/* opening the ODS Spreadsheet as a Zipfile */
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

    if (workbook->ContentZipEntry != NULL)
      {
	  /* parsing Content.xml */
	  do_fetch_ods_worksheets (uf, workbook);
	  if (workbook->error)
	    {
		destroy_workbook (workbook);
		retval = FREEXL_INVALID_XLSX;
		goto stop;
	    }
      }
    (*handle)->ods_handle = workbook;

/* closing the Zipfile and quitting */
  stop:
    unzClose (uf);
    return retval;
}

FREEXL_DECLARE int
freexl_close_ods (const void *xl_handle)
{
/* attempting to destroy the Workbook */
    freexl_handle *handle = (freexl_handle *) xl_handle;
    if (!handle)
	return FREEXL_NULL_HANDLE;
    if (handle->ods_handle == NULL)
	return FREEXL_INVALID_HANDLE;

/* destroying the workbook */
    destroy_workbook (handle->ods_handle);
/* destroying the handle */
    free (handle);

    return FREEXL_OK;
}

#endif /* end conditional XML support */

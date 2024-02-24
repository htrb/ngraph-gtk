/*
 * $Id: gtk_entry_completion.c,v 1.8 2010-03-04 08:30:16 hito Exp $
 */

#include "gtk_common.h"

#include <stdio.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include "ioutil.h"
#include "object.h"

static int HistSize;

static GtkEntryCompletion *
entry_completion_create(GtkTreeModel *model)
{
  GtkEntryCompletion *comp;

  comp = gtk_entry_completion_new();
  gtk_entry_completion_set_model(comp, model);
  gtk_entry_completion_set_inline_completion(comp, FALSE);
  gtk_entry_completion_set_popup_completion(comp, TRUE);
  gtk_entry_completion_set_popup_set_width(comp, TRUE);
  gtk_entry_completion_set_text_column(comp, 0);
  g_object_ref(comp);

  return comp;
}

void
entry_completion_set_entry(GtkTreeModel *model, GtkWidget *entry)
{
  GtkEntryCompletion *comp;

  if (model == NULL || entry == NULL) {
    return;
  }

  comp = entry_completion_create(model);
  gtk_entry_set_completion(GTK_ENTRY(entry), comp);
}

static gboolean
save_history_cb(GtkTreeModel *model, GtkTreePath *path, GtkTreeIter *iter, gpointer data)
{
  FILE *fp;
  int *a;
  char *v;

  fp = (FILE *) data;

  a = gtk_tree_path_get_indices(path);

  if (a == NULL || a[0] >= HistSize)
    return TRUE;

  gtk_tree_model_get(model, iter, 0, &v, -1);

  if (v) {
    fprintf(fp, "%s\n", v);
    g_free(v);
  }
  return FALSE;
}

static gboolean
add_completion(GtkListStore *list, FILE *fp)
{
  char *buf;
  GtkTreeIter iter;
  int r;


  r = fgetline(fp, &buf);

  if (r || buf == NULL)
    return TRUE;

  g_strchomp(buf);

  if (g_utf8_validate(buf, -1, NULL)) {
    gtk_list_store_append(list, &iter);
    gtk_list_store_set(list, &iter, 0, buf, -1);
  }

  g_free(buf);

  return FALSE;
}

int
entry_completion_save(GtkTreeModel *model, char *file, int size)
{
  FILE *fp;

  if (model == NULL)
    return 1;

  fp = nfopen(file, "w");
  if (fp == NULL)
    return 1;

  HistSize = size;
  gtk_tree_model_foreach(model, save_history_cb, fp);

  fclose(fp);
  return 0;
}

GtkTreeModel *
entry_completion_load(char *file, int size)
{
  FILE *fp;
  int i;
  GtkTreeModel *model;
  GtkListStore *list;

  list = gtk_list_store_new (1, G_TYPE_STRING);
  g_object_ref (list);

  model = GTK_TREE_MODEL (list);

  fp = nfopen(file, "r");
  if (fp == NULL) {
    return model;
  }

  for (i = 0; i < size; i++) {
    if (add_completion(GTK_LIST_STORE(model), fp))
      break;
  }

  fclose(fp);
  return model;
}

void
entry_completion_append(GtkTreeModel *model, const char *str)
{
  gboolean found;
  GtkTreeIter iter;
  char *v = NULL;

  if (model == NULL || str == NULL || strlen(str) == 0)
    return;

  if (strchr(str, '\n')) {
    return;
  }

  found = gtk_tree_model_get_iter_first(model, &iter);
  while (found) {
    gtk_tree_model_get(model, &iter, 0, &v, -1);
    if (v) {
      if (strcmp(str, v) == 0) {
	gtk_list_store_remove(GTK_LIST_STORE(model), &iter);
	g_free(v);
	break;
      }
      g_free(v);
    }
    found = gtk_tree_model_iter_next(model, &iter);
  }
  gtk_list_store_prepend(GTK_LIST_STORE(model), &iter);
  gtk_list_store_set(GTK_LIST_STORE(model), &iter, 0, str, -1);
}

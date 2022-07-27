/* -*- Mode: C; coding: utf-8 -*-
 * modified from gtksourcecompletionproviderwords.c
 * This file is part of GtkSourceView
 *
 * Copyright (C) 2009 - Jesse van den Kieboom
 * Copyright (C) 2013 - Sébastien Wilmet
 *
 * gtksourceview is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * gtksourceview is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include "gtk_common.h"
#include "sourcecompletionwords.h"

#include <string.h>

#if GTK_CHECK_VERSION(4, 0, 0)
/* WordsProposal Object */
#define WORDS_TYPE_PROPOSAL (words_proposal_get_type())
G_DECLARE_FINAL_TYPE (WordsProposal, words_proposal, WORDS, PROPOSAL, GObject)

struct _WordsProposal
{
  GObject parent_instance;
  char *text;
  char *info;
};

enum
{
  PROP_0,
  PROP_WORD,
  N_PROPS
};

static GParamSpec *WordsProposalProperties[N_PROPS];

G_DEFINE_TYPE_WITH_CODE (WordsProposal, words_proposal, G_TYPE_OBJECT,
                         G_IMPLEMENT_INTERFACE (GTK_SOURCE_TYPE_COMPLETION_PROPOSAL, NULL))

/*
static void
words_proposal_dispose (GObject *object)
{
  WordsProposal *self = WORDS_PROPOSAL (object);
  g_clear_pointer (&self->text, g_free);
  g_clear_pointer (&self->info, g_free);
  G_OBJECT_CLASS (words_proposal_parent_class)->dispose (object);
}
*/

static void
words_proposal_finalize (GObject *object)
{
  WordsProposal *self = WORDS_PROPOSAL (object);
  g_free (self->text);
  g_free (self->info);
  G_OBJECT_CLASS (words_proposal_parent_class)->finalize (object);
}

static void
words_proposal_get_property (GObject    *object,
                             guint       prop_id,
                             GValue     *value,
                             GParamSpec *pspec)
{
  WordsProposal *self = WORDS_PROPOSAL (object);

  switch (prop_id) {
  case PROP_WORD:
    g_value_set_string (value, self->text);
    break;
  default:
    G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id, pspec);
  }
}

static void
words_proposal_class_init (WordsProposalClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);
  object_class->finalize = words_proposal_finalize;
  object_class->get_property = words_proposal_get_property;

  WordsProposalProperties [PROP_WORD] = g_param_spec_string ("word",
                                                "Word",
                                                "The word for the proposal",
                                                NULL,
                                                (G_PARAM_READABLE | G_PARAM_STATIC_STRINGS));

  g_object_class_install_properties (object_class, N_PROPS, WordsProposalProperties);
}

static void
words_proposal_init (WordsProposal *self)
{
}

WordsProposal *
words_proposal_new (void)
{
  return g_object_new (WORDS_TYPE_PROPOSAL, NULL);
}

void
words_proposal_set (WordsProposal *item, const char *text, const char *info)
{
  item->text = g_strdup (text);
  item->info = g_strdup (info);
}

/* SourceCompletionWords Object */
#define PRIORITY 10

struct _SourceCompletionWordsPrivate
{
  gchar *name;
  populate_func populate_func;
};

static void source_completion_words_iface_init (GtkSourceCompletionProviderInterface *iface);

G_DEFINE_TYPE_WITH_CODE (SourceCompletionWords,
			 source_completion_words,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (SourceCompletionWords)
			 G_IMPLEMENT_INTERFACE (GTK_SOURCE_TYPE_COMPLETION_PROVIDER,
				 		source_completion_words_iface_init))

static gchar *
source_completion_words_get_name (GtkSourceCompletionProvider *self)
{
  return g_strdup (SOURCE_COMPLETION_WORDS (self)->priv->name);
}

static char *
completion_context_get_word (GtkSourceCompletionContext *context)
{
  char *word, *tmp;
  tmp = gtk_source_completion_context_get_word (context);
  word = g_ascii_strdown (tmp, -1);
  g_free (tmp);
  return word;
}

static void
source_completion_words_populate_async (GtkSourceCompletionProvider *provider,
                                        GtkSourceCompletionContext  *context,
                                        GCancellable                *cancellable,
                                        GAsyncReadyCallback          callback,
                                        gpointer                     user_data)
{
  SourceCompletionWords *words = SOURCE_COMPLETION_WORDS (provider);
  GtkTextIter iter;
  gchar *word;
  GListStore *ret = NULL;
  GTask *task;

  word = completion_context_get_word (context);
  gtk_source_completion_context_get_bounds (context, NULL, &iter);
  task = g_task_new (provider, cancellable, callback, user_data);
  g_task_set_source_tag (task, source_completion_words_populate_async);
  g_task_set_priority (task, PRIORITY);
  ret = words->priv->populate_func(word, strlen(word), &iter);
  g_task_return_pointer (task, ret, g_object_unref);
  g_clear_pointer (&word, g_free);
}

static GListModel *
source_completion_words_populate_finish (GtkSourceCompletionProvider  *provider,
                                         GAsyncResult                 *result,
                                         GError                      **error)
{
  return g_task_propagate_pointer (G_TASK (result), error);
}

static void
source_completion_words_activate (GtkSourceCompletionProvider *provider,
                                  GtkSourceCompletionContext  *context,
                                  GtkSourceCompletionProposal *proposal)
{
  GtkTextBuffer *buffer;
  GtkTextIter begin, end;

  if (! gtk_source_completion_context_get_bounds (context, &begin, &end)) {
    return;
  }
  buffer = gtk_text_iter_get_buffer (&begin);

  gtk_text_buffer_begin_user_action (buffer);
  gtk_text_buffer_delete (buffer, &begin, &end);
  gtk_text_buffer_insert (buffer, &begin, WORDS_PROPOSAL (proposal)->text, -1);
  gtk_text_buffer_end_user_action (buffer);
}

static void
source_completion_words_display (GtkSourceCompletionProvider *provider,
                                 GtkSourceCompletionContext  *context,
                                 GtkSourceCompletionProposal *proposal,
                                 GtkSourceCompletionCell     *cell)
{
  WordsProposal *p = (WordsProposal *)proposal;
  GtkSourceCompletionColumn column;

  g_assert (GTK_SOURCE_IS_COMPLETION_CONTEXT (context));
  g_assert (WORDS_IS_PROPOSAL (p));
  g_assert (GTK_SOURCE_IS_COMPLETION_CELL (cell));

  column = gtk_source_completion_cell_get_column (cell);

  switch (column) {
  case GTK_SOURCE_COMPLETION_COLUMN_TYPED_TEXT:
    gtk_source_completion_cell_set_text (cell, p->text);
    break;
  case GTK_SOURCE_COMPLETION_COLUMN_COMMENT:
  case GTK_SOURCE_COMPLETION_COLUMN_DETAILS:
    gtk_source_completion_cell_set_markup (cell, p->info);
    break;
  case GTK_SOURCE_COMPLETION_COLUMN_ICON:
    gtk_source_completion_cell_set_icon_name (cell, NULL);
    break;
  case GTK_SOURCE_COMPLETION_COLUMN_BEFORE:
  case GTK_SOURCE_COMPLETION_COLUMN_AFTER:
    gtk_source_completion_cell_set_text (cell, NULL);
    break;
  }
}

static void
source_completion_words_dispose (GObject *object)
{
  SourceCompletionWords *provider = SOURCE_COMPLETION_WORDS (object);

  g_free (provider->priv->name);
  provider->priv->name = NULL;

  G_OBJECT_CLASS (source_completion_words_parent_class)->dispose (object);
}

static void
source_completion_words_class_init (SourceCompletionWordsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = source_completion_words_dispose;
}

static gint
source_completion_words_get_priority (GtkSourceCompletionProvider *provider, GtkSourceCompletionContext* context)
{
  return PRIORITY;
}

static void
source_completion_words_refilter (GtkSourceCompletionProvider *provider,
                                  GtkSourceCompletionContext  *context,
                                  GListModel                  *model)
{
  GtkFilterListModel *filter_model = NULL;
  GtkExpression *expression = NULL;
  GtkStringFilter *filter = NULL;
  GListModel *replaced_model = NULL;
  char *word;

  g_assert (GTK_SOURCE_IS_COMPLETION_CONTEXT (context));
  g_assert (G_IS_LIST_MODEL (model));

  word = gtk_source_completion_context_get_word (context);
  if (GTK_IS_FILTER_LIST_MODEL (model)) {
    model = gtk_filter_list_model_get_model (GTK_FILTER_LIST_MODEL (model));
  }
  expression = gtk_property_expression_new (WORDS_TYPE_PROPOSAL, NULL, "word");
  filter = gtk_string_filter_new (g_steal_pointer (&expression));
  gtk_string_filter_set_search (GTK_STRING_FILTER (filter), word);
  filter_model = gtk_filter_list_model_new (g_object_ref (model), GTK_FILTER (g_steal_pointer (&filter)));
  gtk_filter_list_model_set_incremental (filter_model, TRUE);
  gtk_source_completion_context_set_proposals_for_provider (context, provider, G_LIST_MODEL (filter_model));

  g_clear_object (&replaced_model);
  g_clear_object (&filter_model);
  g_clear_pointer (&word, g_free);
}

static void
source_completion_words_iface_init (GtkSourceCompletionProviderInterface *iface)
{
  iface->get_title = source_completion_words_get_name;
  iface->populate_async = source_completion_words_populate_async;
  iface->populate_finish = source_completion_words_populate_finish;
  iface->get_priority = source_completion_words_get_priority;
  iface->display = source_completion_words_display;
  iface->activate = source_completion_words_activate;
  iface->refilter = source_completion_words_refilter;
}

static void
source_completion_words_init (SourceCompletionWords *self)
{
  self->priv = source_completion_words_get_instance_private(self);
}

/**
 * gtk_source_completion_words_new:
 * @name: (nullable): The name for the provider, or %NULL.
 * @populate_func:  : The function to populate completion.
 *
 * Returns: a new #GtkSourceCompletionWords provider
 */
SourceCompletionWords *
source_completion_words_new (const gchar *name, populate_func populate_func)
{
  SourceCompletionWords *self;

  self = g_object_new(SOURCE_TYPE_COMPLETION_WORDS, NULL);
  self->priv->name = g_strdup(name);
  self->priv->populate_func = populate_func;
  return self;
}
#else
#define MINIMUM_WORD_SIZE 2
#define INTERACTIVE_DELAY 50
#define PRIORITY 10
#define ACTIVATION (GTK_SOURCE_COMPLETION_ACTIVATION_INTERACTIVE | GTK_SOURCE_COMPLETION_ACTIVATION_USER_REQUESTED)
#define PROPOSALS_BATCH_SIZE 300


struct _SourceCompletionWordsPrivate
{
  gchar *name;
  populate_func populate_func;
};

static void source_completion_words_iface_init (GtkSourceCompletionProviderIface *iface);

G_DEFINE_TYPE_WITH_CODE (SourceCompletionWords,
			 source_completion_words,
			 G_TYPE_OBJECT,
			 G_ADD_PRIVATE (SourceCompletionWords)
			 G_IMPLEMENT_INTERFACE (GTK_SOURCE_TYPE_COMPLETION_PROVIDER,
				 		source_completion_words_iface_init))

static gchar *
source_completion_words_get_name (GtkSourceCompletionProvider *self)
{
  return g_strdup (SOURCE_COMPLETION_WORDS (self)->priv->name);
}

static gboolean
valid_start_char (gunichar ch)
{
  return !g_unichar_isdigit (ch);
}

static gboolean
valid_word_char (gunichar ch)
{
  return g_unichar_isprint (ch) && (ch == '_' || ch == '%' || g_unichar_isalnum (ch));
}

static gchar *
get_end_word(gchar *text)
{
  gchar *cur_char = text + strlen (text);
  gboolean word_found = FALSE;
  gunichar ch;

  while (TRUE) {
    gchar *prev_char = g_utf8_find_prev_char(text, cur_char);

    if (prev_char == NULL) {
      break;
    }

    ch = g_utf8_get_char (prev_char);

    if (!valid_word_char (ch)) {
      break;
    }

    word_found = TRUE;
    cur_char = prev_char;
  }

  if (! word_found) {
    return NULL;
  }

  ch = g_utf8_get_char (cur_char);
  if (! valid_start_char(ch)) {
    return NULL;
  }

  return g_ascii_strdown(cur_char, -1);
}

static gchar *
get_word_at_iter (GtkTextIter *iter)
{
  GtkTextBuffer *buffer;
  GtkTextIter start_line;
  gchar *line_text;
  gchar *word;

  buffer = gtk_text_iter_get_buffer (iter);
  start_line = *iter;
  gtk_text_iter_set_line_offset (&start_line, 0);

  line_text = gtk_text_buffer_get_text (buffer, &start_line, iter, FALSE);

  word = get_end_word (line_text);

  g_free (line_text);
  return word;
}

static gboolean
text_char_predicate(gunichar ch, gpointer user_data)
{
  return ch == '#';
}

static int
in_comment(GtkTextIter *iter)
{
  GtkTextIter cur;
  cur = *iter;
  gtk_text_iter_set_line_offset(iter, 0);
  return gtk_text_iter_forward_find_char(iter, text_char_predicate, NULL, &cur);
}

static void
source_completion_words_populate (GtkSourceCompletionProvider *provider,
                                      GtkSourceCompletionContext  *context)
{
  SourceCompletionWords *words = SOURCE_COMPLETION_WORDS (provider);
  GtkSourceCompletionActivation activation;
  GtkTextIter iter;
  gchar *word;
  GList *ret = NULL;

  if (!gtk_source_completion_context_get_iter (context, &iter))
  {
    gtk_source_completion_context_add_proposals (context, provider, NULL, TRUE);
    return;
  }

  if (in_comment(&iter)) {
    gtk_source_completion_context_add_proposals (context, provider, NULL, TRUE);
    return;
  }

  word = get_word_at_iter (&iter);
  activation = gtk_source_completion_context_get_activation (context);

  if (word == NULL ||
      (activation == GTK_SOURCE_COMPLETION_ACTIVATION_INTERACTIVE &&
       g_utf8_strlen (word, -1) < MINIMUM_WORD_SIZE))
  {
    g_free(word);
    gtk_source_completion_context_add_proposals(context, provider, NULL, TRUE);
    return;
  }

  ret = words->priv->populate_func(word, strlen(word), &iter);
  g_free(word);
  ret = g_list_reverse(ret);
  gtk_source_completion_context_add_proposals(context,
                                              GTK_SOURCE_COMPLETION_PROVIDER(words),
                                              ret,
                                              TRUE);
  g_list_free (ret);
}

static void
source_completion_words_dispose (GObject *object)
{
  SourceCompletionWords *provider = SOURCE_COMPLETION_WORDS (object);
  SourceCompletionWordsPrivate *priv = source_completion_words_get_instance_private (provider);

  g_clear_pointer (&priv->name, g_free);
  G_OBJECT_CLASS (source_completion_words_parent_class)->dispose (object);
}

static void
source_completion_words_class_init (SourceCompletionWordsClass *klass)
{
  GObjectClass *object_class = G_OBJECT_CLASS (klass);

  object_class->dispose = source_completion_words_dispose;
}

static gboolean
source_completion_words_get_start_iter (GtkSourceCompletionProvider *provider,
                                        GtkSourceCompletionContext  *context,
                                        GtkSourceCompletionProposal *proposal,
                                        GtkTextIter                 *iter)
{
  gchar *word;
  glong nb_chars;

  if (!gtk_source_completion_context_get_iter (context, iter))
  {
    return FALSE;
  }

  word = get_word_at_iter (iter);
  g_return_val_if_fail (word != NULL, FALSE);

  nb_chars = g_utf8_strlen (word, -1);
  gtk_text_iter_backward_chars (iter, nb_chars);

  g_free (word);
  return TRUE;
}

static gint
source_completion_words_get_interactive_delay (GtkSourceCompletionProvider *provider)
{
  return INTERACTIVE_DELAY;
}

static gint
source_completion_words_get_priority (GtkSourceCompletionProvider *provider)
{
  return PRIORITY;
}

static GtkSourceCompletionActivation
source_completion_words_get_activation (GtkSourceCompletionProvider *provider)
{
  return ACTIVATION;
}

static void
source_completion_words_iface_init (GtkSourceCompletionProviderIface *iface)
{
  iface->get_name = source_completion_words_get_name;
  iface->populate = source_completion_words_populate;
  iface->get_start_iter = source_completion_words_get_start_iter;
  iface->get_interactive_delay = source_completion_words_get_interactive_delay;
  iface->get_priority = source_completion_words_get_priority;
  iface->get_activation = source_completion_words_get_activation;
}

static void
source_completion_words_init (SourceCompletionWords *self)
{
  self->priv = source_completion_words_get_instance_private(self);
}

/**
 * gtk_source_completion_words_new:
 * @name: (nullable): The name for the provider, or %NULL.
 * @populate_func:  : The function to populate completion.
 *
 * Returns: a new #GtkSourceCompletionWords provider
 */
SourceCompletionWords *
source_completion_words_new (const gchar *name, populate_func populate_func)
{
  SourceCompletionWords *self;

  self = g_object_new(SOURCE_TYPE_COMPLETION_WORDS, NULL);
  self->priv->name = g_strdup(name);
  self->priv->populate_func = populate_func;
  return self;
}
#endif

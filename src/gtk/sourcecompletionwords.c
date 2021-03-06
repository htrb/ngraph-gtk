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

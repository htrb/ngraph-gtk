/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8; coding: utf-8 -*-
 * modified from gtksourcecompletionproviderwords.h
 * This file is part of GtkSourceView
 *
 * Copyright (C) 2009 - Jesse van den Kieboom
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

#ifndef SOURCE_COMPLETION_WORDS_H
#define SOURCE_COMPLETION_WORDS_H

#define GTK_SOURCE_H_INSIDE

#include <gtksourceview/gtksourcecompletionprovider.h>
#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SOURCE_TYPE_COMPLETION_WORDS		(source_completion_words_get_type ())
#define SOURCE_COMPLETION_WORDS(obj)		(G_TYPE_CHECK_INSTANCE_CAST ((obj), SOURCE_TYPE_COMPLETION_WORDS, SourceCompletionWords))
#define SOURCE_COMPLETION_WORDS_CLASS(klass)	(G_TYPE_CHECK_CLASS_CAST ((klass), SOURCE_TYPE_COMPLETION_WORDS, SourceCompletionWordsClass))
#define SOURCE_IS_COMPLETION_WORDS(obj)		(G_TYPE_CHECK_INSTANCE_TYPE ((obj), SOURCE_TYPE_COMPLETION_WORDS))
#define SOURCE_IS_COMPLETION_WORDS_CLASS(klass)	(G_TYPE_CHECK_CLASS_TYPE ((klass), SOURCE_TYPE_COMPLETION_WORDS))
#define SOURCE_COMPLETION_WORDS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), SOURCE_TYPE_COMPLETION_WORDS, SourceCompletionWordsClass))

typedef struct _SourceCompletionWords		SourceCompletionWords;
typedef struct _SourceCompletionWordsClass		SourceCompletionWordsClass;
typedef struct _SourceCompletionWordsPrivate		SourceCompletionWordsPrivate;

struct _SourceCompletionWords {
	GObject parent;
	SourceCompletionWordsPrivate *priv;
};

struct _SourceCompletionWordsClass {
	GObjectClass parent_class;
};

GType source_completion_words_get_type (void) G_GNUC_CONST;

typedef GListStore * (* populate_func)(const char *, int, GtkTextIter *);
SourceCompletionWords *source_completion_words_new(const gchar *name, populate_func func);

struct _WordsProposal;
typedef struct _WordsProposal WordsProposal;
WordsProposal *words_proposal_new (void);
void words_proposal_set_text (WordsProposal *item, const char *text);
void words_proposal_set_info (WordsProposal *item, const char *info);
G_END_DECLS

#endif /* SOURCE_COMPLETION_WORDS_H */

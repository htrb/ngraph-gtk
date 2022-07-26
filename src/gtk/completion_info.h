#ifndef COMPLETION_INFO_HEADER
#define COMPLETION_INFO_HEADER
#include "gtk_common.h"
#include "sourcecompletionwords.h"

/* must be implemented */
struct completion_info
{
  char *lower_text, *text, *text_wo_paren, *info;
#if GTK_CHECK_VERSION(4, 0, 0)
  WordsProposal *proposal;      /* unused member */
#else
  GtkSourceCompletionItem *proposal;
#endif
};

extern struct completion_info completion_info_const[];
extern struct completion_info completion_info_func[];

#if GTK_CHECK_VERSION(4, 0, 0)
GListStore *completion_info_func_populate(const char *word, int len, GtkTextIter *iter);
GListStore *completion_info_const_populate(const char *word, int len, GtkTextIter *iter);
#else
GList *completion_info_func_populate(const char *word, int len, GtkTextIter *iter);
GList *completion_info_const_populate(const char *word, int len, GtkTextIter *iter);
#endif

#endif

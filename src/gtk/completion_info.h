#ifndef COMPLETION_INFO_HEADER
#define COMPLETION_INFO_HEADER
#include "gtk_common.h"
#include "sourcecompletionwords.h"

struct completion_info
{
  char *lower_text, *text, *text_wo_paren, *info;
  WordsProposal *proposal;
};

extern struct completion_info completion_info_const[];
extern struct completion_info completion_info_func[];

GListStore *completion_info_func_populate(const char *word, int len, const GtkTextIter *iter);
GListStore *completion_info_const_populate(const char *word, int len, const GtkTextIter *iter);

#endif

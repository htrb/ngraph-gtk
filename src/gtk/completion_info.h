#ifndef COMPLETION_INFO_HEADER
#define COMPLETION_INFO_HEADER
#include "gtk_common.h"

struct completion_info 
{
  char *lower_text, *text, *info;
  GtkSourceCompletionItem *proposal;
};

extern struct completion_info completion_info_const[];
extern struct completion_info completion_info_func[];

GList *completion_info_func_populate(const char *word, int len);
GList *completion_info_const_populate(const char *word, int len);

#endif

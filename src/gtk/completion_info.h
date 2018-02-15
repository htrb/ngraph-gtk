#ifndef COMPLETION_INFO_HEADER
#define COMPLETION_INFO_HEADER
#include "gtk_common.h"

struct completion_info 
{
  char *text, *info;
  GtkSourceCompletionItem *proposal;
};

extern struct completion_info completion_info_const[];
extern struct completion_info completion_info_func[];

int completion_info_func_num(void);
int completion_info_const_num(void);
GList *completion_info_func_populate(const char *word, int len);
GList *completion_info_const_populate(const char *word, int len);

#endif

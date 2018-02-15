#include "completion_info.h"

GList *
completion_info_populate(struct completion_info *info, int num, const char *word, int len)
{
  GList *ret = NULL;
  int i;

  for (i = 0; i < num; i++) {
    if (g_ascii_strncasecmp(info[i].text, word, len)) {
      continue;
    }
    if (info[i].proposal == NULL) {
      GtkSourceCompletionItem *proposal;
      proposal = gtk_source_completion_item_new2();
      gtk_source_completion_item_set_label(proposal, info[i].text);
      gtk_source_completion_item_set_text(proposal, info[i].text);
      gtk_source_completion_item_set_info(proposal, info[i].info);
      info[i].proposal = proposal;
    }
    ret = g_list_prepend (ret, info[i].proposal);
  }
  return ret;
}

GList *
completion_info_func_populate(const char *word, int len)
{
  return completion_info_populate(completion_info_func, completion_info_func_num(), word, len);
}

GList *
completion_info_const_populate(const char *word, int len)
{
  return completion_info_populate(completion_info_const, completion_info_const_num(), word, len);
}

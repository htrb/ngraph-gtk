#include "completion_info.h"

#if GTK_CHECK_VERSION(4, 0, 0)
/* must be implemented */
GList *
completion_info_func_populate(const char *word, int len, GtkTextIter *iter)
{
  return NULL;
}

GList *
completion_info_const_populate(const char *word, int len, GtkTextIter *iter)
{
  return NULL;
}
#else
static int
check_paren(GtkTextIter *iter)
{
  GtkTextIter cur;
  gunichar ch, paren;

  paren = g_utf8_get_char("(");
  cur = *iter;
  while (1) {
    ch = gtk_text_iter_get_char(&cur);
    if (! g_unichar_isspace(ch)) {
      break;
    }
    gtk_text_iter_forward_char(&cur);
  }
  return (ch == paren);
}

static GList *
completion_info_populate(struct completion_info *info, const char *word, int len, GtkTextIter *iter)
{
  GList *ret = NULL;
  int i;
  const char *text;

  for (i = 0; info[i].lower_text; i++) {
    int r;
    r = strncmp(info[i].lower_text, word, len);
    if (r < 0) {
      continue;
    } else if (r > 0) {
      break;
    }

    if (info[i].proposal == NULL) {
      GtkSourceCompletionItem *proposal;
      proposal = gtk_source_completion_item_new();
      gtk_source_completion_item_set_label(proposal, info[i].text);
      gtk_source_completion_item_set_text(proposal, info[i].text);
      gtk_source_completion_item_set_info(proposal, _(info[i].info));
      info[i].proposal = proposal;
    }
    if (info[i].text_wo_paren && check_paren(iter)) {
      text = info[i].text_wo_paren;
    } else {
      text = info[i].text;
    }
    gtk_source_completion_item_set_text(info[i].proposal, text);
    ret = g_list_prepend (ret, info[i].proposal);
  }
  return ret;
}

GList *
completion_info_func_populate(const char *word, int len, GtkTextIter *iter)
{
  return completion_info_populate(completion_info_func, word, len, iter);
}

GList *
completion_info_const_populate(const char *word, int len, GtkTextIter *iter)
{
  return completion_info_populate(completion_info_const, word, len, iter);
}
#endif

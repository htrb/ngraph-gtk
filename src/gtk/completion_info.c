#include "completion_info.h"

static int
check_paren(const GtkTextIter *iter)
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

static GListStore *
completion_info_populate(struct completion_info *info, const char *word, int len, const GtkTextIter *iter)
{
  GListStore *ret = NULL;
  int i;
  const char *text;

  ret = g_list_store_new (GTK_SOURCE_TYPE_COMPLETION_PROPOSAL);

  for (i = 0; info[i].lower_text; i++) {
    int r;
    r = strncmp(info[i].lower_text, word, len);
    if (r < 0) {
      continue;
    } else if (r > 0) {
      break;
    }

    if (info[i].proposal == NULL) {
      WordsProposal *proposal;
      proposal = words_proposal_new();
      words_proposal_set_info(proposal, _(info[i].info));
      info[i].proposal = proposal;
    }
    if (info[i].text_wo_paren && check_paren(iter)) {
      text = info[i].text_wo_paren;
    } else {
      text = info[i].text;
    }
    words_proposal_set_text(info[i].proposal, text);
    g_list_store_append (ret, info[i].proposal);
  }
  return ret;
}

GListStore *
completion_info_func_populate(const char *word, int len, const GtkTextIter *iter)
{
  return completion_info_populate(completion_info_func, word, len, iter);
}

GListStore *
completion_info_const_populate(const char *word, int len, const GtkTextIter *iter)
{
  return completion_info_populate(completion_info_const, word, len, iter);
}

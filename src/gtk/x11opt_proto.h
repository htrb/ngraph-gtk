#ifndef HAVE_UPDATE_FUNC
static void
CREATE_NAME(Pref, DialogUpdate_response)(struct response_callback *cb)
{
  struct CREATE_NAME(Pref, Dialog) *d;
  d = (struct CREATE_NAME(Pref, Dialog) *) cb->dialog;
  CREATE_NAME(Pref, DialogSetupItem)(d);
}

static void
CREATE_NAME(Pref, DialogUpdate)(GtkWidget *w, gpointer client_data)
{
  struct CREATE_NAME(Pref, Dialog) *d;
  int a, j;
  struct LIST_TYPE *fcur;

  d = (struct CREATE_NAME(Pref, Dialog) *) client_data;

  a = list_store_get_selected_index(d->list);

  j = 0;
  fcur = LIST_ROOT;
  while (fcur) {
    if (j == a)
      break;
    fcur = fcur->next;
    j++;
  }
#ifdef SET_DIALOG
  if (fcur) {
    CREATE_NAME(Set, Dialog)(&SET_DIALOG, fcur);
    response_callback_add(&SET_DIALOG, CREATE_NAME(Pref, DialogUpdate_response), NULL, NULL);
    DialogExecute(d->widget, &SET_DIALOG);
  }
#endif
}
#endif

#ifdef LIST_INIT
struct CREATE_NAME(Pref, DialogAdd_data) {
  struct LIST_TYPE *fprev, *fnew;
};

static void
CREATE_NAME(Pref, DialogAdd_response)(struct response_callback *cb)
{
  struct CREATE_NAME(Pref, DialogAdd_data) *data;
  struct LIST_TYPE *fprev, *fnew;
  struct CREATE_NAME(Pref, Dialog) *d;

  data = (struct CREATE_NAME(Pref, DialogAdd_data) *) cb->data;
  fnew = data->fnew;
  fprev = data->fprev;
  g_free(data);

  if (cb->return_value != IDOK) {
    if (fprev == NULL) {
      LIST_ROOT = NULL;
    } else {
      fprev->next = NULL;
    }
    g_free(fnew);
  }
  d = (struct CREATE_NAME(Pref, Dialog) *) cb->dialog;
  CREATE_NAME(Pref, DialogSetupItem)(d);
}

static void
CREATE_NAME(Pref, DialogAdd)(GtkWidget *w, gpointer client_data)
{
  struct LIST_TYPE *fcur, *fprev, *fnew;
  struct CREATE_NAME(Pref, Dialog) *d;
  struct CREATE_NAME(Pref, DialogAdd_data) *data;

  d = (struct CREATE_NAME(Pref, Dialog) *) client_data;
  fprev = NULL;
  fcur = LIST_ROOT;
  while (fcur) {
    fprev = fcur;
    fcur = fcur->next;
  }

  fnew = (struct LIST_TYPE *) g_malloc(sizeof(struct LIST_TYPE));
  if (fnew == NULL) {
    return;
  }

  LIST_INIT(fnew);
  if (fprev == NULL) {
    LIST_ROOT = fnew;
  } else {
    fprev->next = fnew;
  }

  CREATE_NAME(Set, Dialog)(&SET_DIALOG, fnew);
  data = g_malloc0(sizeof(*data));
  data->fnew = fnew;
  data->fprev = fprev;
  response_callback_add(&SET_DIALOG, CREATE_NAME(Pref, DialogAdd_response), NULL, data);
  DialogExecute(d->widget, &SET_DIALOG);
}
#endif

#ifdef LIST_FREE
static void
CREATE_NAME(Pref, DialogRemove)(GtkWidget *w, gpointer client_data)
{
  int a, j;
  struct LIST_TYPE *fcur, *fprev, *fdel;
  struct CREATE_NAME(Pref, Dialog) *d;

  d = (struct CREATE_NAME(Pref, Dialog) *) client_data;

  a = list_store_get_selected_index(d->list);

  j = 0;
  fprev = NULL;
  fcur = LIST_ROOT;
  while (fcur) {
    if (j == a) {
      fdel = fcur;
      if (fprev == NULL)
	LIST_ROOT = fcur->next;
      else
	fprev->next = fcur->next;
      fcur = fcur->next;
      LIST_FREE(fdel);
      CREATE_NAME(Pref, DialogSetupItem)(d);
      break;
    } else {
      fprev = fcur;
      fcur = fcur->next;
    }
    j++;
  }
}
#endif


static int
CREATE_NAME(Pref, DialogMoveSub)(struct CREATE_NAME(Pref, Dialog) *d, int a)
{
  int j, n;
  struct LIST_TYPE *fcur, *fprev, *next;

  n = list_store_get_num(d->list);
  if (a < 0 || a >= n - 1)
    return -1;

  j = 0;
  fprev = NULL;
  fcur = LIST_ROOT;
  while (fcur) {
    if (j == a) {
      next = fcur->next;

      if (next == NULL) {
	break;
      } else if (fprev == NULL) {
	LIST_ROOT = next;
      } else {
	fprev->next = next;
      }

      fcur->next = next->next;
      next->next = fcur;
      break;
    }
    fprev = fcur;
    fcur = fcur->next;
    j++;
  }
  CREATE_NAME(Pref, DialogSetupItem)(d);
  return a;
}

static void
CREATE_NAME(Pref, DialogUp)(GtkWidget *w, gpointer client_data)
{
  struct CREATE_NAME(Pref, Dialog) *d;
  int a;

  d = (struct CREATE_NAME(Pref, Dialog) *) client_data;

  a = list_store_get_selected_index(d->list);
  a = CREATE_NAME(Pref, DialogMoveSub)(d, a - 1);
  if (a >= 0) {
    list_store_select_nth(d->list, a);
  }
}

static void
CREATE_NAME(Pref, DialogDown)(GtkWidget *w, gpointer client_data)
{
  struct CREATE_NAME(Pref, Dialog) *d;
  int a;

  d = (struct CREATE_NAME(Pref, Dialog) *) client_data;

  a = list_store_get_selected_index(d->list);
  a = CREATE_NAME(Pref, DialogMoveSub)(d, a);
  if (a >= 0) {
    list_store_select_nth(d->list, a + 1);
  }
}

static gboolean
CREATE_NAME(Pref, ListDefailtCb)(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
  struct CREATE_NAME(Pref, Dialog) *d;
  int i;

  d = (struct CREATE_NAME(Pref, Dialog) *) user_data;

  i = list_store_get_selected_index(d->list);
  if (i < 0)
    return FALSE;

  switch (keyval) {
  case GDK_KEY_Up:
    if (state & GDK_SHIFT_MASK) {
      CREATE_NAME(Pref, DialogUp)(NULL, d);
      return TRUE;
    }
    break;
  case GDK_KEY_Down:
    if (state & GDK_SHIFT_MASK) {
      CREATE_NAME(Pref, DialogDown)(NULL, d);
      return TRUE;
    }
    break;
  case GDK_KEY_Delete:
    CREATE_NAME(Pref, DialogRemove)(NULL, d);
    return TRUE;
    break;
  }

  return FALSE;
}

static void
CREATE_NAME(Pref, ListActivatedCb)(GtkTreeView *view, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
  struct CREATE_NAME(Pref, Dialog) *d;
  int i;

  d = (struct CREATE_NAME(Pref, Dialog) *) user_data;

  i = list_store_get_selected_index(d->list);
  if (i < 0)
    return;

  CREATE_NAME(Pref, DialogUpdate)(NULL, d);
}

static gboolean
CREATE_NAME(Pref, ListSelCb)(GtkTreeSelection *sel, gpointer user_data)
{
  int a, n;
  struct CREATE_NAME(Pref, Dialog) *d;

  d = (struct CREATE_NAME(Pref, Dialog) *) user_data;

  a = list_store_get_selected_index(d->list);
  n = list_store_get_num(d->list);

  gtk_widget_set_sensitive(d->update_b, a >= 0);
  gtk_widget_set_sensitive(d->del_b, a >= 0);
  gtk_widget_set_sensitive(d->up_b, a > 0);
  gtk_widget_set_sensitive(d->down_b, a >= 0 && a < n - 1);

  return FALSE;
}

static void
CREATE_NAME(Pref, DialogCreateWidgets)(struct CREATE_NAME(Pref, Dialog) *d, GtkWidget *win_box, int n, n_list_store * list)
{
  GtkWidget *w, *hbox, *vbox, *swin;
  GtkTreeSelection *sel;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  swin = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(swin, TRUE);
  gtk_widget_set_hexpand(swin, TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  w = list_store_create(n, list);
  d->list = w;
  g_signal_connect(d->list, "row-activated", G_CALLBACK(CREATE_NAME(Pref, ListActivatedCb)), d);
  add_event_key(d->list, G_CALLBACK(CREATE_NAME(Pref, ListDefailtCb)), NULL, d);

  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);

  sel = gtk_tree_view_get_selection(GTK_TREE_VIEW(w));
  g_signal_connect(sel, "changed", G_CALLBACK(CREATE_NAME(Pref, ListSelCb)), d);

  w = gtk_frame_new(NULL);
  gtk_frame_set_child(GTK_FRAME(w), swin);

  if (win_box) {
    gtk_box_append(GTK_BOX(win_box), w);
    gtk_box_append(GTK_BOX(hbox), win_box);
  } else {
    gtk_box_append(GTK_BOX(hbox), w);
  }
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  w= gtk_button_new_with_mnemonic(_("_Add"));
  set_button_icon(w, "list-add");
  g_signal_connect(w, "clicked", G_CALLBACK(CREATE_NAME(Pref, DialogAdd)), d);
  gtk_box_append(GTK_BOX(vbox), w);

  w = gtk_button_new_with_mnemonic(_("_Preferences"));
  set_button_icon(w, "preferences-system");
  g_signal_connect(w, "clicked", G_CALLBACK(CREATE_NAME(Pref, DialogUpdate)), d);
  gtk_box_append(GTK_BOX(vbox), w);
  gtk_widget_set_sensitive(w, FALSE);
  d->update_b = w;

  w = gtk_button_new_with_mnemonic(_("_Remove"));
  set_button_icon(w, "list-remove");
  g_signal_connect(w, "clicked", G_CALLBACK(CREATE_NAME(Pref, DialogRemove)), d);
  gtk_box_append(GTK_BOX(vbox), w);
  gtk_widget_set_sensitive(w, FALSE);
  d->del_b = w;

  w = gtk_button_new_with_mnemonic(_("_Down"));
  set_button_icon(w, "go-down");
  g_signal_connect(w, "clicked", G_CALLBACK(CREATE_NAME(Pref, DialogDown)), d);
  gtk_box_append(GTK_BOX(vbox), w);
  gtk_widget_set_sensitive(w, FALSE);
  d->down_b = w;

  w = gtk_button_new_with_mnemonic(_("_Up"));
  set_button_icon(w, "go-up");
  g_signal_connect(w, "clicked", G_CALLBACK(CREATE_NAME(Pref, DialogUp)), d);
  gtk_box_append(GTK_BOX(vbox), w);
  gtk_widget_set_sensitive(w, FALSE);
  d->up_b = w;

  gtk_box_append(GTK_BOX(hbox), vbox);
  gtk_box_append(GTK_BOX(d->vbox), hbox);

  d->show_cancel = FALSE;
  d->ok_button = _("_Close");
}

#undef LIST_TYPE
#undef LIST_ROOT
#undef LIST_FREE
#undef LIST_INIT
#undef SET_DIALOG
#undef CREATE_NAME
#undef HAVE_UPDATE_FUNC

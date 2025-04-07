#ifndef HAVE_UPDATE_FUNC
static void
CREATE_NAME(Pref, DialogUpdate_response)(struct response_callback *cb)
{
  struct CREATE_NAME(Pref, Dialog) *d;
  d = (struct CREATE_NAME(Pref, Dialog) *) cb->data;
  CREATE_NAME(Pref, DialogSetupItem)(d);
}

static void
CREATE_NAME(Pref, DialogUpdate)(GtkWidget *w, gpointer client_data)
{
  struct CREATE_NAME(Pref, Dialog) *d;
  int a, j;
  struct LIST_TYPE *fcur;

  d = (struct CREATE_NAME(Pref, Dialog) *) client_data;

  a = columnview_get_active(d->list);

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
    response_callback_add(&SET_DIALOG, CREATE_NAME(Pref, DialogUpdate_response), NULL, d);
    DialogExecute(d->widget, &SET_DIALOG);
  }
#endif
}
#endif

#ifdef LIST_INIT
struct CREATE_NAME(Pref, DialogAdd_data) {
  struct LIST_TYPE *fprev, *fnew;
  struct CREATE_NAME(Pref, Dialog) *d;
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
  d = data->d;
  g_free(data);

  if (cb->return_value != IDOK) {
    if (fprev == NULL) {
      LIST_ROOT = NULL;
    } else {
      fprev->next = NULL;
    }
    g_free(fnew);
  }
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
  data->d = d;
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

  a = columnview_get_active(d->list);

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

  n = columnview_get_n_items(d->list);
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

  a = columnview_get_active(d->list);
  a = CREATE_NAME(Pref, DialogMoveSub)(d, a - 1);
  if (a >= 0) {
    gtk_column_view_scroll_to (GTK_COLUMN_VIEW (d->list), a, NULL, GTK_LIST_SCROLL_SELECT | GTK_LIST_SCROLL_FOCUS, NULL);
  }
}

static void
CREATE_NAME(Pref, DialogDown)(GtkWidget *w, gpointer client_data)
{
  struct CREATE_NAME(Pref, Dialog) *d;
  int a;

  d = (struct CREATE_NAME(Pref, Dialog) *) client_data;

  a = columnview_get_active(d->list);
  a = CREATE_NAME(Pref, DialogMoveSub)(d, a);
  if (a >= 0) {
    gtk_column_view_scroll_to (GTK_COLUMN_VIEW (d->list), a + 1, NULL, GTK_LIST_SCROLL_SELECT | GTK_LIST_SCROLL_FOCUS, NULL);
  }
}

static gboolean
CREATE_NAME(Pref, ListDefaultCb)(GtkEventControllerKey *controller, guint keyval, guint keycode, GdkModifierType state, gpointer user_data)
{
  struct CREATE_NAME(Pref, Dialog) *d;
  int i;

  d = (struct CREATE_NAME(Pref, Dialog) *) user_data;

  i = columnview_get_active(d->list);
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
CREATE_NAME(Pref, ListActivatedCb)(GtkSelectionModel *model, guint pos, gpointer user_data)
{
  struct CREATE_NAME(Pref, Dialog) *d;

  d = (struct CREATE_NAME(Pref, Dialog) *) user_data;


  CREATE_NAME(Pref, DialogUpdate)(NULL, d);
}

static gboolean
CREATE_NAME(Pref, ListSelCb)(GtkSelectionModel *sel, guint position, guint n_items, gpointer user_data)
{
  int a, n;
  struct CREATE_NAME(Pref, Dialog) *d;

  d = (struct CREATE_NAME(Pref, Dialog) *) user_data;

  a = selection_model_get_selected(sel);
  n = g_list_model_get_n_items (G_LIST_MODEL (sel));

  gtk_widget_set_sensitive(d->update_b, a >= 0);
  gtk_widget_set_sensitive(d->del_b, a >= 0);
  gtk_widget_set_sensitive(d->up_b, a > 0);
  gtk_widget_set_sensitive(d->down_b, a >= 0 && a < n - 1);

  return FALSE;
}

static void
CREATE_NAME (Bind, Item) (GtkListItemFactory *factory, GtkListItem *list_item, gpointer user_data)
{
  GtkWidget *label;
  NText *item;
  int i;

  label = gtk_list_item_get_child (list_item);
  item = gtk_list_item_get_item (list_item);
  i = GPOINTER_TO_INT (user_data);
  gtk_label_set_text(GTK_LABEL(label), item->text[i]);
}

static void
CREATE_NAME(Pref, DialogCreateWidgets)(struct CREATE_NAME(Pref, Dialog) *d, GtkWidget *win_box, int n, char ** list)
{
  GtkWidget *w, *hbox, *vbox, *swin;
  GtkSelectionModel *sel;
  int i;
  GtkEventController *ev;

  hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 4);

  swin = gtk_scrolled_window_new();
  gtk_widget_set_vexpand(swin, TRUE);
  gtk_widget_set_hexpand(swin, TRUE);
  gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(swin), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
  w = columnview_create(N_TYPE_TEXT, N_SELECTION_TYPE_SINGLE);
  for (i = 0; i < n; i++) {
    columnview_create_column(w, _(list[i]), G_CALLBACK(setup_listitem_cb), G_CALLBACK(CREATE_NAME (Bind, Item)), NULL, GINT_TO_POINTER (i), i == n - 1);
  }
  d->list = w;

  g_signal_connect(d->list, "activate", G_CALLBACK(CREATE_NAME(Pref, ListActivatedCb)), d);
  ev = add_event_key(d->list, G_CALLBACK(CREATE_NAME(Pref, ListDefaultCb)), NULL, d);
  gtk_event_controller_set_propagation_phase(ev, GTK_PHASE_CAPTURE);
  gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(swin), w);

  sel = gtk_column_view_get_model(GTK_COLUMN_VIEW(w));
  g_signal_connect(sel, "selection-changed", G_CALLBACK(CREATE_NAME(Pref, ListSelCb)), d);

  w = gtk_frame_new(NULL);
  gtk_frame_set_child(GTK_FRAME(w), swin);

  if (win_box) {
    gtk_box_append(GTK_BOX(win_box), w);
    gtk_box_append(GTK_BOX(hbox), win_box);
  } else {
    gtk_box_append(GTK_BOX(hbox), w);
  }
  vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 4);

  w = gtk_button_new_with_mnemonic(_("_Add"));
  g_signal_connect(w, "clicked", G_CALLBACK(CREATE_NAME(Pref, DialogAdd)), d);
  gtk_box_append(GTK_BOX(vbox), w);

  w = gtk_button_new_with_mnemonic(_("_Preferences"));
  g_signal_connect(w, "clicked", G_CALLBACK(CREATE_NAME(Pref, DialogUpdate)), d);
  gtk_box_append(GTK_BOX(vbox), w);
  gtk_widget_set_sensitive(w, FALSE);
  d->update_b = w;

  w = gtk_button_new_with_mnemonic(_("_Remove"));
  g_signal_connect(w, "clicked", G_CALLBACK(CREATE_NAME(Pref, DialogRemove)), d);
  gtk_box_append(GTK_BOX(vbox), w);
  gtk_widget_set_sensitive(w, FALSE);
  d->del_b = w;

  w = gtk_button_new_with_mnemonic(_("_Down"));
  g_signal_connect(w, "clicked", G_CALLBACK(CREATE_NAME(Pref, DialogDown)), d);
  gtk_box_append(GTK_BOX(vbox), w);
  gtk_widget_set_sensitive(w, FALSE);
  d->down_b = w;

  w = gtk_button_new_with_mnemonic(_("_Up"));
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

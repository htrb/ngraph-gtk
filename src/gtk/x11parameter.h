GtkWidget *create_parameter_list(struct SubWin *d);
void CmParameterAdd(void *w, gpointer client_data);
void CmParameterDelete(void *w, gpointer client_data);
void CmParameterUpdate(void *w, gpointer client_data);
void ParameterWinUpdate(struct obj_list_data *d, int clear, int draw);

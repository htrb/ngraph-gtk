#ifndef NGRAPH_PLUGIN_SHELL_HEADER
#define NGRAPH_PLUGIN_SHELL_HEADER

struct plugin_shell;
struct objlist;

typedef int (* plugin_shell_init) (struct plugin_shell *shell);
typedef int (* plugin_shell_done) (struct plugin_shell *shell);
typedef int (* plugin_shell_shell) (struct plugin_shell *shell, int argc, char *argv[]);

struct objlist *ngraph_plugin_shell_getobject(const char *name);
void *ngraph_plugin_shell_get_user_data(struct plugin_shell *shlocal);
void ngraph_plugin_shell_set_user_data(struct plugin_shell *shlocal, void *user_data);
int ngraph_plugin_shell_putobj(struct objlist *obj, const char *vname, int id, void *val);
int ngraph_plugin_shell_getobj(struct objlist *obj, const char *vname, int id, int argc, char **argv, void *val);
int ngraph_plugin_shell_exeobj(struct objlist *obj, const char *vname, int id, int argc, char **argv);
int ngraph_plugin_shell_getid_by_name(struct objlist *obj, const char *name);
int ngraph_plugin_shell_getid_by_oid(struct objlist *obj, int oid);
int ngraph_plugin_shell_move_top(struct objlist *obj, int id);
int ngraph_plugin_shell_move_last(struct objlist *obj, int id);
int ngraph_plugin_shell_move_up(struct objlist *obj, int id);
int ngraph_plugin_shell_move_down(struct objlist *obj, int id);
int ngraph_plugin_shell_exchange(struct objlist *obj, int id1, int id2);
int ngraph_plugin_shell_copy(struct objlist *obj, int id_dest, int id_src);
int ngraph_plugin_shell_new(struct objlist *obj);
int ngraph_plugin_shell_del(struct objlist *obj, int id);
int ngraph_plugin_shell_exist(struct objlist *obj, int id);
const char *ngraph_plugin_shell_get_obj_name(const char *name);
char *ngraph_plugin_shell_get_obj_fields(const char *name);
const char *ngraph_plugin_shell_get_obj_parent(const char *name);
const char *ngraph_plugin_shell_get_obj_version(const char *name);
int ngraph_plugin_shell_get_obj_id(const char *name);
int ngraph_plugin_shell_get_obj_size(const char *name);
int ngraph_plugin_shell_get_obj_current(const char *name);
int ngraph_plugin_shell_get_obj_last(const char *name);
int ngraph_plugin_shell_get_obj_inst_num(const char *name);
char *ngraph_plugin_shell_derive(const char *name);
char *ngraph_plugin_shell_derive_inst(const char *name);

#endif

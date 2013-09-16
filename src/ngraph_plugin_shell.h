#ifndef NGRAPH_PLUGIN_SHELL_HEADER
#define NGRAPH_PLUGIN_SHELL_HEADER

#define NVOID 0
#define NBOOL 1
#if USE_NCHAR
#define NCHAR 2
#endif
#define NINT 3
#define NDOUBLE 4
#define NSTR 5
#define NPOINTER 6
#define NIARRAY 7
#define NDARRAY 8
#define NSARRAY 9
#define NENUM 10
#define NOBJ  11
#if USE_LABEL
#define NLABEL 12
#endif
#define NVFUNC 20
#define NBFUNC 21
#if USE_NCHAR
#define NCFUNC 22
#endif
#define NIFUNC 23
#define NDFUNC 24
#define NSFUNC 25
#define NIAFUNC 26
#define NDAFUNC 27
#define NSAFUNC 28

#define NREAD 1
#define NWRITE 2
#define NEXEC 4

struct plugin_shell;
struct objlist;
union _ngraph_value;

struct ngraph_obj {
  struct objlist *obj;
  int id;
};

struct ngraph_array {
  int num;
  union array {
    int i;
    double d;
    char *str;
    struct ngraph_array *ary;
  } ary[1];
};

typedef struct ngraph_array ngraph_arg;

typedef union _ngraph_value {
  int i;
  double d;
  char *str;
  struct ngraph_obj obj;
  ngraph_arg *ary;
} ngraph_value;

typedef union _ngraph_returned_value {
  int i;
  double d;
  const char *str;
  struct {
    int num;
    union {
      const int *ia;
      const double *da;
      const char * const *sa;
    } data;
  } ary;
} ngraph_returned_value;

typedef int (* plugin_shell_shell) (struct plugin_shell *shell, int argc, char *argv[]);

struct objlist *ngraph_plugin_shell_get_object(const char *name);
void *ngraph_plugin_shell_get_user_data(struct plugin_shell *shlocal);
void ngraph_plugin_shell_set_user_data(struct plugin_shell *shlocal, void *user_data);
int ngraph_plugin_shell_putobj(struct objlist *obj, const char *vname, int id, ngraph_value *val);
int ngraph_plugin_shell_getobj(struct objlist *obj, const char *vname, int id, ngraph_arg *arg, ngraph_returned_value *val);
int ngraph_plugin_shell_exeobj(struct objlist *obj, const char *vname, int id, ngraph_arg *arg);
int *ngraph_plugin_shell_get_id_by_str(struct objlist *obj, const char *name);
int ngraph_plugin_shell_get_id_by_oid(struct objlist *obj, int oid);
int ngraph_plugin_shell_move_top(struct objlist *obj, int id);
int ngraph_plugin_shell_move_last(struct objlist *obj, int id);
int ngraph_plugin_shell_move_up(struct objlist *obj, int id);
int ngraph_plugin_shell_move_down(struct objlist *obj, int id);
int ngraph_plugin_shell_exchange(struct objlist *obj, int id1, int id2);
int ngraph_plugin_shell_copy(struct objlist *obj, int id_dest, int id_src);
int ngraph_plugin_shell_new(struct objlist *obj);
int ngraph_plugin_shell_del(struct objlist *obj, int id);
int ngraph_plugin_shell_exist(struct objlist *obj, int id);
const char *ngraph_plugin_shell_get_obj_name(struct objlist *obj);
int ngraph_plugin_shell_get_obj_field_num(struct objlist *obj);
int ngraph_plugin_shell_get_obj_field_permission(struct objlist *obj, const char *field);
int ngraph_plugin_shell_get_obj_field_type(struct objlist *obj, const char *field);
const char *ngraph_plugin_shell_get_obj_field_args(struct objlist *obj, const char *field);
const char *ngraph_plugin_shell_get_obj_field(struct objlist *obj, int i);
struct objlist *ngraph_plugin_shell_get_obj_parent(struct objlist *obj);
const char *ngraph_plugin_shell_get_obj_version(struct objlist *obj);
int ngraph_plugin_shell_get_obj_id(struct objlist *obj);
int ngraph_plugin_shell_get_obj_size(struct objlist *obj);
int ngraph_plugin_shell_get_obj_current_id(struct objlist *obj);
int ngraph_plugin_shell_get_obj_last_id(struct objlist *obj);
struct objlist *ngraph_plugin_shell_get_obj_next(struct objlist *obj);
struct objlist *ngraph_plugin_shell_get_obj_child(struct objlist *obj);

#endif

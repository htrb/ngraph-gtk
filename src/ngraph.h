#ifndef NGRAPH_HEADER
#define NGRAPH_HEADER

enum ngraph_object_field_type {
  NVOID = 0,
  NBOOL = 1,
#if USE_NCHAR
  NCHAR = 2,
#endif
  NINT = 3,
  NDOUBLE = 4,
  NSTR = 5,
  NPOINTER = 6,
  NIARRAY = 7,
  NDARRAY = 8,
  NSARRAY = 9,
  NENUM = 10,
  NOBJ = 11,
#ifdef USE_LABEL
  NLABEL = 12,
#endif
  NVFUNC = 20,
  NBFUNC = 21,
#if USE_NCHAR
  NCFUNC = 22,
#endif
  NIFUNC = 23,
  NDFUNC = 24,
  NSFUNC = 25,
  NIAFUNC = 26,
  NDAFUNC = 27,
  NSAFUNC = 28,
};

#define NREAD 1
#define NWRITE 2
#define NEXEC 4

struct ngraph_plugin;
struct objlist;
union _ngraph_value;

struct ngraph_obj {
  struct objlist *obj;
  int id;
};

struct ngraph_array;
typedef struct ngraph_array ngraph_arg;

typedef union _ngraph_value {
  int i;
  double d;
  const char *str;
  struct ngraph_array *ary;
} ngraph_value;

struct ngraph_array {
  int num;
  union _ngraph_value ary[1];
};


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

typedef int (* ngraph_plugin_exec) (int argc, char *argv[]);
typedef int (* ngraph_plugin_open) (void);
typedef int (* ngraph_plugin_close) (void);

int ngraph_initialize(int *argc, char ***argv);
void ngraph_save_shell_history(void);
void ngraph_finalize(void);
char *ngraph_get_init_file(const char *init_file);
int ngraph_exec_loginshell(char *loginshell, struct objlist *obj, int id);

int ngraph_object_put(struct objlist *obj, const char *vname, int id, ngraph_value *val);
int ngraph_object_get(struct objlist *obj, const char *vname, int id, ngraph_arg *arg, ngraph_returned_value *val);
int ngraph_object_exe(struct objlist *obj, const char *vname, int id, ngraph_arg *arg);
int ngraph_object_get_id_by_oid(struct objlist *obj, int oid);
int ngraph_object_move_top(struct objlist *obj, int id);
int ngraph_object_move_last(struct objlist *obj, int id);
int ngraph_object_move_up(struct objlist *obj, int id);
int ngraph_object_move_down(struct objlist *obj, int id);
int ngraph_object_exchange(struct objlist *obj, int id1, int id2);
int ngraph_object_copy(struct objlist *obj, int id_dest, int id_src);
int ngraph_object_new(struct objlist *obj);
int ngraph_object_del(struct objlist *obj, int id);
int ngraph_object_exist(struct objlist *obj, int id);

const char *ngraph_get_object_name(struct objlist *obj);
const char *ngraph_get_object_alias(struct objlist *obj);
const char *ngraph_get_object_field_args(struct objlist *obj, const char *field);
const char *ngraph_get_object_field(struct objlist *obj, int i);
const char *ngraph_get_object_version(struct objlist *obj);
struct objlist *ngraph_get_object(const char *name);
struct objlist *ngraph_get_parent_object(struct objlist *obj);
struct objlist *ngraph_get_root_object(void);
struct objlist *ngraph_get_next_object(struct objlist *obj);
struct objlist *ngraph_get_child_object(struct objlist *obj);
struct objlist *ngraph_get_object_instances_by_str(const char *str, int *n, int **ids);
int ngraph_get_object_id(struct objlist *obj);
int ngraph_get_object_size(struct objlist *obj);
int ngraph_get_object_current_id(struct objlist *obj);
int ngraph_get_object_last_id(struct objlist *obj);
int ngraph_get_object_field_num(struct objlist *obj);
int ngraph_get_object_field_permission(struct objlist *obj, const char *field);
enum ngraph_object_field_type ngraph_get_object_field_type(struct objlist *obj, const char *field);

int ngraph_puts(const char *s);
int ngraph_err_puts(const char *s);
void ngraph_sleep(double t);
void *ngraph_malloc(size_t size);
void ngraph_free(void *ptr);
char *ngraph_strdup(const char *str);
int ngraph_set_exec_func(const char *name, ngraph_plugin_exec func);

#endif

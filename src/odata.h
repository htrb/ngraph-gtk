#ifndef OFILE_HEADER
#define OFILE_HEADER

#define FILE_OBJ_MAXCOL 999
#define FILE_OBJ_SMOOTH_MAX 50

enum DATA_OBJ_SOURCE {
  DATA_SOURCE_FILE,
  DATA_SOURCE_ARRAY,
  DATA_SOURCE_RANGE,
};

enum MOVING_AVERAGE_TYPE {
  MOVING_AVERAGE_SIMPLE,
  MOVING_AVERAGE_WEIGHTED,
  MOVING_AVERAGE_EXPONENTIAL,
};

enum FILE_OBJ_PLOT_TYPE {
  PLOT_TYPE_MARK,
  PLOT_TYPE_LINE,
  PLOT_TYPE_POLYGON,
  PLOT_TYPE_POLYGON_SOLID_FILL,
  PLOT_TYPE_CURVE,
  PLOT_TYPE_DIAGONAL,
  PLOT_TYPE_ARROW,
  PLOT_TYPE_RECTANGLE,
  PLOT_TYPE_RECTANGLE_FILL,
  PLOT_TYPE_RECTANGLE_SOLID_FILL,
  PLOT_TYPE_ERRORBAR_X,
  PLOT_TYPE_ERRORBAR_Y,
  PLOT_TYPE_STAIRCASE_X,
  PLOT_TYPE_STAIRCASE_Y,
  PLOT_TYPE_BAR_X,
  PLOT_TYPE_BAR_Y,
  PLOT_TYPE_BAR_FILL_X,
  PLOT_TYPE_BAR_FILL_Y,
  PLOT_TYPE_BAR_SOLID_FILL_X,
  PLOT_TYPE_BAR_SOLID_FILL_Y,
  PLOT_TYPE_FIT,
};

enum axis_instance_field_type{
  AXIS_X,
  AXIS_Y,
  AXIS_REFERENCE,
};

#include "math/math_equation.h"

struct array_prm
{
  struct objlist *obj;
  int data_num, col_num;
  int id[FILE_OBJ_MAXCOL];
  struct narray *ary[FILE_OBJ_MAXCOL];
};

#define CHECK_TERMINATE(ch) ((ch) == '\0' || (ch) == '\n')
#define CHECK_CHR(ifs, ch) (ch && strchr(ifs, ch))

MathEquation *ofile_create_math_equation(int *id, enum EOEQ_ASSIGN_TYPE type, int prm_digit, int use_fprm, int use_const, int usr_func, int use_fobj_func, int use_fit_func);
int get_axis_id(struct objlist *obj, N_VALUE *inst, struct objlist **aobj, int axis);
int ofile_calc_fit_equation(struct objlist *obj, int id, double x, double *y);
int open_array(char *objstr, struct array_prm *ary);
char *odata_get_functions(void);
char *odata_get_constants(void);
const char *parse_data_line(struct narray *array, const char *str, const char *ifs, int csv);
int n_strtod(const char *str, MathValue *val);
int load_file(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int store(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv, int *endstore, FILE **storefd);

#endif

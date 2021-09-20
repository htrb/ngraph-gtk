#ifndef OAXIS_HEADER
#define OAXIS_HEADER

#define AXIS_GROUPE_NUM_MAX 4
struct AxisGroupInfo
{
  int type, num;
  N_VALUE *inst[AXIS_GROUPE_NUM_MAX];
  int id[AXIS_GROUPE_NUM_MAX];
};

int axis_get_group(struct objlist *obj, N_VALUE *inst,  struct AxisGroupInfo *info);
int axis_save_config(struct objlist *obj, N_VALUE *inst);

#endif  /* OAXIS_HEADER */

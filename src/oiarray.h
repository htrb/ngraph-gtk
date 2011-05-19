#ifndef OIARRAY_HEADER
#define OIARRAY_HEADER

struct narray *oarray_get_array(struct objlist *obj, N_VALUE *inst, unsigned int size);
int oarray_seq(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int oarray_reverse_seq(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int oarray_reverse(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int oarray_slice(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);
int oarray_num(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv);

#endif /* OIARRAY_HEADER */

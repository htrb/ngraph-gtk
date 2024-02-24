/*
 * $Id: object.h,v 1.20 2010-01-04 05:11:28 hito Exp $
 *
 * This file is part of "Ngraph for X11".
 *
 * Copyright (C) 2002, Satoshi ISHIZAKA. isizaka@msa.biglobe.ne.jp
 *
 * "Ngraph for X11" is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * "Ngraph for X11" is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 *
 */

#ifndef N_OBJECT_HEADER
#define N_OBJECT_HEADER

#define TEXT_SIZE_MIN 500
#define OBJ_MAX 128

#include "nhash.h"
#include "ngraph.h"

struct objlist;
struct objtable;

enum OBJ_LIST_SPECIFIED_TYPE {
  OBJ_LIST_SPECIFIED_NOT_FOUND,
  OBJ_LIST_SPECIFIED_BY_ID,
  OBJ_LIST_SPECIFIED_BY_OID,
  OBJ_LIST_SPECIFIED_BY_NAME,
  OBJ_LIST_SPECIFIED_BY_OTHER,
};

union n_value {
  int i;
  double d;
  char *str;
  struct narray *array;
  void *ptr;
  union n_value *inst;
};

typedef union n_value N_VALUE;

typedef
  int (*Proc)(struct objlist *obj,N_VALUE *inst,N_VALUE *rval,int argc,char **argv);

typedef
  int (*DoneProc)(struct objlist *obj,void *local);

typedef int (*UNDO_DUP_FUNC)(struct objlist *obj, N_VALUE *src, N_VALUE *dest);
typedef int (*UNDO_FREE_FUNC)(struct objlist *obj, N_VALUE *inst);

struct undo_inst {
  int operation;
  int curinst, lastinst, lastoid, lastinst2;
  N_VALUE *inst;
  struct undo_inst *next;
};

struct objtable {
    char *name;
    enum ngraph_object_field_type type;
    int attrib;
    Proc proc;
    void *arglist;
    int offset;
};

struct objlist {
    int id;
    int curinst;
    int lastinst;
    int lastoid;
    char *name;
    char *alias;
    char *ver;
    int tblnum, fieldnum;
    struct objtable *table;
    NHASH table_hash;
    int size;
    int errnum;
    char **errtable;
    N_VALUE *root;
    N_VALUE *root2;
    int lastinst2;
    struct undo_inst *undo, *redo;
    struct objlist *parent;
    struct objlist *next, *child;
    int idp,oidp,nextp;
    void *local;
    DoneProc doneproc;
  UNDO_DUP_FUNC dup_func;
  UNDO_FREE_FUNC free_func;
};

struct narray {
    unsigned int size;
    unsigned int base;
    unsigned int num;
    void *data;
};

struct loopproc;
struct loopproc {
  struct loopproc *next;
  const char *objname;
  const char *evname;
  struct objlist *obj;
  int idn;
  N_VALUE *inst;
  void *local;
};

#define ERRUNKNOWN 0
#define ERRHEAP 1
#define ERRPARENT 2
#define ERRDUP 3
#define ERROBJNUM 4
#define ERRINSTNUM 5
#define ERROBJFOUND 6
#define ERRVALFOUND 7
#define ERRNONEXT 8
#define ERRNOID 9
#define ERRIDFOUND 10
#define ERROIDFOUND 11
#define ERRNMFOUND 12
#define ERRDESTRUCT 13
#define ERRPERMISSION 14
#define ERROBJCINST 15
#define ERRNOINST 16
#define ERRILOBJ 17
#define ERRILINST 18
#define ERRFIELD 19
#define ERROEXTARG 20
#define ERROSMLARG 21
#define ERROVALUE  22
#define ERROVERWRITE 23

extern int (*getstdin)(void);
extern int (*putstdout)(const char *s);
extern int (*putstderr)(const char *s);
extern int (*printfstdout)(const char *fmt,...);
extern int (*printfstderr)(const char *fmt,...);
extern int (*ninterrupt)(void);
extern int (*inputyn)(const char *mes);
extern void (*ndisplaydialog)(const char *str);
extern void (*ndisplaystatus)(const char *str);

struct savedstdio {
  int (*getstdin)(void);
  int (*putstdout)(const char *s);
  int (*putstderr)(const char *s);
  int (*printfstdout)(const char *fmt,...);
  int (*printfstderr)(const char *fmt,...);
  int (*ninterrupt)(void);
  int (*inputyn)(const char *mes);
  void (*ndisplaydialog)(const char *str);
  void (*ndisplaystatus)(const char *str);
};

extern struct savedstdio stdiosave;

int seputs(const char *s);
int seprintf(const char *fmt,...);

void error(struct objlist *obj,int code);
void error2(struct objlist *obj,int code, const char *mes);
void error22(struct objlist *obj,int code, const char *mes1, const char *mes2);
void error3(struct objlist *obj,int code,int num);

void ignorestdio(struct savedstdio *save);
void restorestdio(struct savedstdio *save);
void savestdio(struct savedstdio *save);
void loadstdio(struct savedstdio *save);

void arrayinit(struct narray *array,unsigned int base);
struct narray *arraynew(unsigned int base);
void *arraydata(struct narray *array);
unsigned int arraynum(struct narray *array);
void arraydel(struct narray *array);
void arraydel2(struct narray *array);
void arrayclear(struct narray *array);
void arrayclear2(struct narray *array);
void arrayfree(struct narray *array);
void arrayfree2(struct narray *array);
struct narray *arrayadd(struct narray *array,const void *val);
struct narray *arrayadd2(struct narray *array,const char *val);
struct narray *arrayins(struct narray *array,const void *val,unsigned int idx);
struct narray *arrayins2(struct narray *array,const char *val,unsigned int idx);
struct narray *arrayndel(struct narray *array,unsigned int idx);
struct narray *arrayndel2(struct narray *array,unsigned int idx);
struct narray *arrayput(struct narray *array,const void *val,unsigned int idx);
struct narray *arrayput2(struct narray *array,const char *val,unsigned int idx);
struct narray *array_reverse(struct narray *array);
struct narray *array_slice(struct narray *array, int start, int length);
struct narray *array_slice2(struct narray *array, int start, int length);
struct narray *arraydup(struct narray *array);
struct narray *arraydup2(struct narray *array);
void *arraynget(struct narray *array,unsigned int idx);
int arraynget_int(struct narray *array, unsigned int idx);
double arraynget_double(struct narray *array, unsigned int idx);
char *arraynget_str(struct narray *array, unsigned int idx);
void *arraylast(struct narray *array);
int arraylast_int(struct narray *array);
int arraypop_int(struct narray *array);
int array_find_int(struct narray *array, int number);
void arraysort_int(struct narray *array);
void arrayrsort_int(struct narray *array);
void arrayuniq_int(struct narray *array);
void arraysort_double(struct narray *array);
void arrayrsort_double(struct narray *array);
void arrayuniq_double(struct narray *array);
void arrayrsort_str(struct narray *array);
void arraysort_str(struct narray *array);
void arrayuniq_str(struct narray *array);
void arrayuniq_all_str(struct narray *array);
int arraycmp(struct narray *a, struct narray *b);
int arraycpy(struct narray *a, struct narray *b);

int getargc(char **arg);
char **arg_add(char ***arg,void *ptr);
void arg_del(char **arg);

#if USE_EVENT_LOOP
void registerevloop(const char *objname, const char *evname,
                    struct objlist *obj,int idn,N_VALUE *inst,void *local);
void unregisterevloop(struct objlist *obj,int idn,N_VALUE *inst);
void eventloop(void);
#endif

struct objlist *chkobjroot(void);
void *addobject(char *name,char *alias,char *parentname,
                char *ver,int tblnum,struct objtable *table,
                int errnum,char **errtable,void *local,DoneProc doneproc);
void hideinstance(struct objlist *obj);
void recoverinstance(struct objlist *obj);
struct objlist *chkobject(const char *name);
int chkobjectid(struct objlist *obj);
const char *chkobjectname(struct objlist *obj);
const char *chkobjectalias(struct objlist *obj);
struct objlist *chkobjparent(struct objlist *obj);
int chkobjchild(struct objlist *parent,struct objlist *child);
char *chkobjver(struct objlist *obj);
int chkobjsize(struct objlist *obj);
int chkobjlastinst(struct objlist *obj);
int chkobjcurinst(struct objlist *obj);
int chkobjoffset(struct objlist *obj, const char *name);
int chkobjoffset2(struct objlist *obj,int tblpos);
N_VALUE *chkobjinstoid(struct objlist *obj,int oid);
N_VALUE *chkobjinst(struct objlist *obj,int id);
int chkobjoid(struct objlist *obj,int oid);
int chkobjfieldnum(struct objlist *obj);
char *chkobjfieldname(struct objlist *obj,int num);
int chkobjfield(struct objlist *obj, const char *name);
int chkobjperm(struct objlist *obj, const char *name);
enum ngraph_object_field_type chkobjfieldtype(struct objlist *obj, const char *name);
const char *chkobjarglist(struct objlist *obj, const char *name);

struct objlist *getobject(const char *name);
char *getobjver(const char *name);
char *getobjectname(struct objlist *obj);
int getobjoffset(struct objlist *obj, const char *name);
int getobjtblpos(struct objlist *obj, const char *name,struct objlist **robj);
N_VALUE *getobjinstoid(struct objlist *obj,int oid);
N_VALUE *getobjinst(struct objlist *obj,int id);
int getobjfield(struct objlist *obj, const char *name);

int _putobj(struct objlist *obj, const char *vname,N_VALUE *inst,void *val);
int _getobj(struct objlist *obj, const char *vname,N_VALUE *inst,void *val);
int _exeparent(struct objlist *obj,const char *vname,N_VALUE *inst,N_VALUE *rval,
               int argc,char **argv);
int _exeobj(struct objlist *obj,const char *vname,N_VALUE *inst,int argc,char **argv);
int __exeobj(struct objlist *obj,int idn,N_VALUE *inst,int argc,char **argv);
int copyobj(struct objlist *obj, const char *vname,int did,int sid);
int newobj(struct objlist *obj);
int newobj_alias(struct objlist *obj, const char *name);
int delobj(struct objlist *obj,int delid);
int putobj(struct objlist *obj, const char *vname,int id,void *val);
int getobj(struct objlist *obj, const char *vname,int id,
           int argc,char **argv,void *val);
int exeobj(struct objlist *obj,const char *vname,int id,int argc,char **argv);
int moveobj(struct objlist *obj,int did,int sid);
int moveupobj(struct objlist *obj,int id);
int movetopobj(struct objlist *obj,int id);
int movedownobj(struct objlist *obj,int id);
int movelastobj(struct objlist *obj,int id);
int exchobj(struct objlist *obj,int id1,int id2);
void set_newobj_cb(void (* newobj_cb)(struct objlist *obj));
void set_delobj_cb(void (* delobj_cb)(struct objlist *obj));

int getobjiname(char *s, char **name, char **ptr);
int chkobjilist(char *s,struct objlist **obj,struct narray *iarray,
                int def,int *spc);
int getobjilist(char *s,struct objlist **obj,struct narray *iarray,
                int def,int *spc);
int chkobjilist2(char **s,struct objlist **obj,struct narray *iarray,
                 int def);
char *mkobjlist(struct objlist *obj, const char *objname,int id, const char *field,int oid);
struct objlist *getobjlist(char *list,int *id,char **field,int *oid);
char *chgobjlist(char *olist);
char *getvaluestr(struct objlist *obj, const char *field,void *val,int cr,int quote);
int isobject(char **s);

int schkobjfield(struct objlist *obj,int id, const char *field,char *arg,
                 char **valstr,int limittype,int cr,int quote);
int sgetobjfield(struct objlist *obj,int id, const char *field,char *arg,
                 char **valstr,int limittype,int cr,int quote);
int sgetfield(struct objlist *obj,int id,char *arg,char **valstr,
              int limittype,int cr,int quote);
struct narray *sgetobj(char *arg,int limittype,int cr,int quote);
int sputobjfield(struct objlist *obj,int id, const char *field,char *arg);
int sputfield(struct objlist *obj,int id,char *arg);
int sputobj(char *arg);
int sexefield(struct objlist *obj,int id,char *arg);
int sexeobj(char *arg);
int has_eventloop(void);
void obj_do_tighten(struct objlist *obj, N_VALUE *inst,  const char *field);
void obj_do_tighten_all(struct objlist *obj, N_VALUE *inst, const char *field);
int getobjilist2(char **s,struct objlist **obj,struct narray *iarray,int def);
void delchildobj(struct objlist *parent);
int vinterrupt(void);
int vinputyn(const char *mes);
int copy_obj_field(struct objlist *obj, int dist, int src, char **ignore_field);
int str_calc(const char *str, double *val, int *r, char **err_msg);

typedef int (*UNDO_FUNC)(struct objlist *obj);
int undo_save(struct objlist *obj);
int undo_undo(struct objlist *obj);
int undo_redo(struct objlist *obj);
int undo_clear(struct objlist *obj);
int undo_delete(struct objlist *obj);
void obj_set_undo_func(struct objlist *obj, UNDO_DUP_FUNC dup_func, UNDO_FREE_FUNC free_func);
int obj_get_field_pos(struct objlist *obj, const char *field);
int undo_check_undo(struct objlist *obj);
int undo_check_redo(struct objlist *obj);
double arg_to_double(char **argv, int index);

#endif

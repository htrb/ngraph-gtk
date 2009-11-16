/* 
 * $Id: object.h,v 1.18 2009/11/16 09:13:04 hito Exp $
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

#include "nhash.h"

#ifdef DEBUG
extern struct plist *memallocroot;

struct plist;
struct plist {
    void *val;
    struct plist *next;
};
#endif

struct objlist;
struct objtable;

typedef
  int (*Proc)(struct objlist *obj,char *inst,char *rval,int argc,char **argv);

typedef
  int (*DoneProc)(struct objlist *obj,void *local);

struct objtable {
    char *name;
    int type;
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
    void *root;
    void *root2;
    int lastinst2;
    struct objlist *parent;
    struct objlist *next, *child;
    int idp,oidp,nextp;
    void *local;
    DoneProc doneproc;
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
  char *objname;
  char *evname;
  struct objlist *obj;
  int idn;
  char *inst;
  void *local;
};

#define NVOID 0
#define NBOOL 1
#define NCHAR 2
#define NINT 3
#define NDOUBLE 4
#define NSTR 5
#define NPOINTER 6
#define NIARRAY 7
#define NDARRAY 8
#define NSARRAY 9
#define NENUM 10
#define NOBJ  11
#define NLABEL 12
#define NVFUNC 20
#define NBFUNC 21
#define NCFUNC 22
#define NIFUNC 23
#define NDFUNC 24
#define NSFUNC 25
#define NIAFUNC 26
#define NDAFUNC 27
#define NSAFUNC 28

#define NREAD 1
#define NWRITE 2
#define NEXEC 4

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
extern int (*putstdout)(char *s);
extern int (*putstderr)(char *s);
extern int (*printfstdout)(char *fmt,...);
extern int (*printfstderr)(char *fmt,...);
extern int (*ninterrupt)(void);
extern int (*inputyn)(char *mes);
extern void (*ndisplaydialog)(char *str);
extern void (*ndisplaystatus)(char *str);

struct savedstdio {
  int (*getstdin)(void);
  int (*putstdout)(char *s);
  int (*putstderr)(char *s);
  int (*printfstdout)(char *fmt,...);
  int (*printfstderr)(char *fmt,...);
  int (*ninterrupt)(void);
  int (*inputyn)(char *mes);
  void (*ndisplaydialog)(char *str);
  void (*ndisplaystatus)(char *str);
};

extern struct savedstdio stdiosave;

extern int seputs(char *s);
extern int seprintf(char *fmt,...);

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
void arrayfree(struct narray *array);
void arrayfree2(struct narray *array);
struct narray *arrayadd(struct narray *array,void *val);
struct narray *arrayadd2(struct narray *array,char **val);
struct narray *arrayins(struct narray *array,void *val,unsigned int idx);
struct narray *arrayins2(struct narray *array, char **val,unsigned int idx);
struct narray *arrayndel(struct narray *array,unsigned int idx);
struct narray *arrayndel2(struct narray *array,unsigned int idx);
struct narray *arrayput(struct narray *array,void *val,unsigned int idx);
struct narray *arrayput2(struct narray *array,char **val,unsigned int idx);
void *arraynget(struct narray *array,unsigned int idx);
void *arraylast(struct narray *array);
void arraysort_int(struct narray *array);
void arrayuniq_int(struct narray *array);

int getargc(char **arg);
char **arg_add(char ***arg,void *ptr);
void arg_del(char **arg);

void registerevloop(char *objname,char *evname,
                    struct objlist *obj,int idn,char *inst,void *local);
void unregisterevloop(struct objlist *obj,int idn,char *inst);
void eventloop(void);

struct objlist *chkobjroot(void);
void *addobject(char *name,char *alias,char *parentname,
                char *ver,int tblnum,struct objtable *table,
                int errnum,char **errtable,void *local,DoneProc doneproc);
void hideinstance(struct objlist *obj);
void recoverinstance(struct objlist *obj);
struct objlist *chkobject(char *name);
int chkobjectid(struct objlist *obj);
char *chkobjectname(struct objlist *obj);
char *chkobjectalias(struct objlist *obj);
struct objlist *chkobjparent(struct objlist *obj);
int chkobjchild(struct objlist *parent,struct objlist *child);
char *chkobjver(struct objlist *obj);
int chkobjsize(struct objlist *obj);
int chkobjlastinst(struct objlist *obj);
int chkobjcurinst(struct objlist *obj);
int chkobjoffset(struct objlist *obj,char *name);
int chkobjoffset2(struct objlist *obj,int tblpos);
char *chkobjinstoid(struct objlist *obj,int oid);
char *chkobjinst(struct objlist *obj,int id);
int chkobjoid(struct objlist *obj,int oid);
int chkobjfieldnum(struct objlist *obj);
char *chkobjfieldname(struct objlist *obj,int num);
int chkobjfield(struct objlist *obj,char *name);
int chkobjperm(struct objlist *obj,char *name);
int chkobjfieldtype(struct objlist *obj,char *name);
char *chkobjarglist(struct objlist *obj,char *name);

struct objlist *getobject(char *name);
char *getobjver(char *name);
char *getobjectname(struct objlist *obj);
int getobjoffset(struct objlist *obj,char *name);
int getobjtblpos(struct objlist *obj,char *name,struct objlist **robj);
char *getobjinstoid(struct objlist *obj,int oid);
char *getobjinst(struct objlist *obj,int id);
int getobjfield(struct objlist *obj,char *name);

int _putobj(struct objlist *obj,char *vname,char *inst,void *val);
int _getobj(struct objlist *obj,char *vname,char *inst,void *val);
int _exeparent(struct objlist *obj,char *vname,char *inst,char *rval,
               int argc,char **argv);
int _exeobj(struct objlist *obj,char *vname,char *inst,int argc,char **argv);
int __exeobj(struct objlist *obj,int idn,char *inst,int argc,char **argv);
int copyobj(struct objlist *obj,char *vname,int did,int sid);
int newobj(struct objlist *obj);
int delobj(struct objlist *obj,int delid);
int putobj(struct objlist *obj,char *vname,int id,void *val);
int getobj(struct objlist *obj,char *vname,int id,
           int argc,char **argv,void *val);
int exeobj(struct objlist *obj,char *vname,int id,int argc,char **argv);
int moveobj(struct objlist *obj,int did,int sid);
int moveupobj(struct objlist *obj,int id);
int movetopobj(struct objlist *obj,int id);
int movedownobj(struct objlist *obj,int id);
int movelastobj(struct objlist *obj,int id);
int exchobj(struct objlist *obj,int id1,int id2);

int chkobjilist(char *s,struct objlist **obj,struct narray *iarray,
                int def,int *spc);
int getobjilist(char *s,struct objlist **obj,struct narray *iarray,
                int def,int *spc);
int chkobjilist2(char **s,struct objlist **obj,struct narray *iarray,
                 int def);
char *mkobjlist(struct objlist *obj,char *objname,int id,char *field,int oid);
struct objlist *getobjlist(char *list,int *id,char **field,int *oid);
char *chgobjlist(char *olist);
char *getvaluestr(struct objlist *obj,char *field,void *val,int cr,int quote);
int isobject(char **s);

int schkobjfield(struct objlist *obj,int id,char *field,char *arg,
                 char **valstr,int limittype,int cr,int quote);
int sgetobjfield(struct objlist *obj,int id,char *field,char *arg,
                 char **valstr,int limittype,int cr,int quote);
int sgetfield(struct objlist *obj,int id,char *arg,char **valstr,
              int limittype,int cr,int quote);
struct narray *sgetobj(char *arg,int limittype,int cr,int quote);
int sputobjfield(struct objlist *obj,int id,char *field,char *arg);
int sputfield(struct objlist *obj,int id,char *arg);
int sputobj(char *arg);
int sexefield(struct objlist *obj,int id,char *arg);
int sexeobj(char *arg);
int has_eventloop(void);
void obj_do_tighten(struct objlist *obj, char *inst, char *field);
int getobjilist2(char **s,struct objlist **obj,struct narray *iarray,int def);
void delchildobj(struct objlist *parent);
int vinterrupt(void);
int vinputyn(char *mes);
int copy_obj_field(struct objlist *obj, int dist, int src, char **ignore_field);
int str_calc(const char *str, double *val, int *r, char **err_msg);

#endif

extern char *matherrorchar[];

enum MATH_CODE_ERROR_NO {
  MCNOERR = 0,
  MCSYNTAX,
  MCILLEGAL,
  MCNEST,
};

#define MATH_CODE_ERROR_NUM (MCNEST + 1)

#define MNOERR  0
#define MERR    1
#define MNAN    2
#define MUNDEF  3
#define MSERR   4
#define MSCONT  5
#define MSBREAK 6
#define MNONUM  7
#define MEOF    8

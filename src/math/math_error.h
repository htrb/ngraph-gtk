/* 
 * $Id: math_error.h,v 1.5 2009-11-21 11:39:10 hito Exp $
 * 
 */

#ifndef MATH_ERROR_HEADER
#define MATH_ERROR_HEADER

#define MATH_ERROR_TYPE_SYNTAX	0x00010000
#define MATH_ERROR_TYPE_PARSE	0x00020000
#define MATH_ERROR_TYPE_RUNTIME	0x00040000

enum MATH_ERROR {
  MATH_ERROR_NONE	  = 0,

  MATH_ERROR_EOEQ	  = (MATH_ERROR_TYPE_SYNTAX | 0x0001),
  MATH_ERROR_FDEF_NEST	  = (MATH_ERROR_TYPE_SYNTAX | 0x0002),
  MATH_ERROR_UNEXP_OPE	  = (MATH_ERROR_TYPE_SYNTAX | 0x0003),
  MATH_ERROR_ARG_NUM	  = (MATH_ERROR_TYPE_SYNTAX | 0x0004),
  MATH_ERROR_MISS_RP	  = (MATH_ERROR_TYPE_SYNTAX | 0x0005),
  MATH_ERROR_MISS_RB	  = (MATH_ERROR_TYPE_SYNTAX | 0x0006),
  MATH_ERROR_MISS_RC	  = (MATH_ERROR_TYPE_SYNTAX | 0x0007),
  MATH_ERROR_UNKNOWN_FUNC = (MATH_ERROR_TYPE_SYNTAX | 0x0008),
  MATH_ERROR_INVALID_FDEF = (MATH_ERROR_TYPE_SYNTAX | 0x0009),
  MATH_ERROR_INVALID_CDEF = (MATH_ERROR_TYPE_PARSE  | 0x000A),
  MATH_ERROR_UNEXP_TOKEN  = (MATH_ERROR_TYPE_SYNTAX | 0x000B),

  MATH_ERROR_INVALID_PRM  = (MATH_ERROR_TYPE_PARSE  | 0x0010),
  MATH_ERROR_PRM_IN_DEF   = (MATH_ERROR_TYPE_PARSE  | 0x0020),
  MATH_ERROR_CONST_EXIST  = (MATH_ERROR_TYPE_PARSE  | 0x0040),

  MATH_ERROR_CALCULATION  = (MATH_ERROR_TYPE_RUNTIME | 0x0010),

  MATH_ERROR_MEMORY	  = 0x0100,
  MATH_ERROR_UNKNOWN	  = 0x1000,

  MATH_ERROR_INVALID_FUNC = -2,
};

char *math_err_get_error_message(MathEquation *eq, const char *code, int err);

#endif

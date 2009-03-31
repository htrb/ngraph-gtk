/**
 *
 * $Id: wmfapi.h,v 1.3 2009/03/31 09:10:51 hito Exp $
 *
 * This is free software; you can redistribute it and/or modify it.
 *
 * Original author: Satoshi ISHIZAKA
 *                  isizaka@msa.biglobe.ne.jp
 **/

/**
 *
 * $Log: wmfapi.h,v $
 * Revision 1.3  2009/03/31 09:10:51  hito
 * *** empty log message ***
 *
 * Revision 1.2  2009-03-29 02:13:18  hito
 * *** empty log message ***
 *
 * Revision 1.1.1.1  2008-05-29 09:37:33  hito
 * inital version.
 *
 * Revision 1.1.1.1  2008-05-29 08:47:24  hito
 * initial version.
 *
 * Revision 1.1.1.1  2008-05-26 06:03:45  hito
 * initial version
 *
 * Revision 1.1  2003/08/15 12:57:56  isizaka
 * Initial revision
 *
 *
 **/

typedef unsigned char BYTE;
typedef unsigned short UINT;
typedef short int INT;
typedef short int LONG;
typedef unsigned short int WORD;
typedef int BOOL;
typedef unsigned int DWORD;
typedef unsigned int COLORREF;
typedef int HDC;
typedef char *LPCTSTR;

typedef struct tagSIZE {
    LONG cx;
    LONG cy;
} SIZE;

typedef struct tagPOINT {
   LONG x;
   LONG y;
} POINT;

typedef struct tagRECT {
   LONG left;
   LONG top;
   LONG right;
   LONG bottom;
} RECT;

typedef struct tagLOGPEN {
    UINT     lopnStyle;
    POINT    lopnWidth;
    COLORREF lopnColor;
} LOGPEN;

typedef struct tagLOGBRUSH {
    UINT     lbStyle;
    COLORREF lbColor;
    LONG     lbHatch;
} LOGBRUSH;

#define LF_FACESIZE 32

typedef struct tagLOGFONT {
    LONG  lfHeight;
    LONG  lfWidth;
    LONG  lfEscapement;
    LONG  lfOrientation;
    LONG  lfWeight;
    BYTE  lfItalic;
    BYTE  lfUnderline;
    BYTE  lfStrikeOut;
    BYTE  lfCharSet;
    BYTE  lfOutPrecision;
    BYTE  lfClipPrecision;
    BYTE  lfQuality;
    BYTE  lfPitchAndFamily;
    BYTE  lfFaceName[LF_FACESIZE];
} LOGFONT;

typedef POINT LOGRGN[4];

#define TYPEPEN   1
#define TYPEBRUSH 2
#define TYPEFONT  3

typedef struct _HGDIOBJ {
  unsigned int Type;
  unsigned int Index;
  HDC Dc;
} *HGDIOBJ;

typedef struct _HPEN {
  unsigned int Type;
  unsigned int Index;
  HDC Dc;
  LOGPEN Pen;
} *HPEN;

typedef struct _HBRUSH {
  unsigned int Type;
  unsigned int Index;
  HDC Dc;
  LOGBRUSH Brush;
} *HBRUSH;

typedef struct _HFONT {
  unsigned int Type;
  unsigned int Index;
  HDC Dc;
  LOGFONT Font;
} *HFONT;

typedef unsigned short int HANDLE;

typedef struct {
    DWORD   key;
    HANDLE  hmf;
    RECT    bbox;
    WORD    inch;
    DWORD   reserved;
    WORD    checksum;
} METAFILEHEADER; 

typedef struct tagMETAHEADER {
    WORD  mtType;
    WORD  mtHeaderSize;
    WORD  mtVersion;
    DWORD mtSize;
    WORD  mtNoObjects;
    DWORD mtMaxRecord;
    WORD  mtNoParameters;
} METAHEADER; 

#define RGB(r,g,b) ((COLORREF)(((BYTE)(r)|((WORD)(g)<<8))|(((DWORD)(BYTE)(b))<<16)))

#define PS_SOLID  0
#define PS_NULL  5
#define PS_USERSTYLE  7
#define PS_ENDCAP_SQUARE  0x00000100
#define PS_ENDCAP_ROUND  0x00000000
#define PS_ENDCAP_FLAT  0x00000200
#define PS_JOIN_BEVEL  0x00001000
#define PS_JOIN_MITER  0x00002000
#define PS_JOIN_ROUND  0x00000000
#define PS_GEOMETRIC  0x00010000
#define BS_NULL  1
#define BS_SOLID  0
#define ALTERNATE  1
#define WINDING  2
#define TA_BASELINE  24
#define TA_LEFT  0
#define TRANSPARENT  1
#define ANSI_CHARSET 0
#define SHIFTJIS_CHARSET 128
#define SYMBOL_CHARSET 2
#define OEM_CHARSET 255
#define HANGEUL_CHARSET 129
#define CHINESEBIG5_CHARSET 136
#define DEFAULT_CHARSET 1
#define MM_ANISOTROPIC  8

BOOL Polygon(HDC hdc,POINT *lpPoints,INT nCount);
BOOL Ellipse(HDC hdc,INT nLeftRect,INT nTopRect,INT nRightRect,INT nBottomRect);
BOOL Arc(HDC hdc,INT nLeftRect,INT nTopRect,INT nRightRect,INT nBottomRect,
         INT nXStartArc,INT nYStartArc,INT nXEndArc,INT nYEndArc);
BOOL Pie(HDC hdc,INT nLeftRect,INT nTopRect,INT nRightRect,INT nBottomRect,
         INT nXRadial1,INT nYRadial1,INT nXRadial2,INT nYRadial2);
BOOL Chord(HDC hdc,INT nLeftRect,INT nTopRect,INT nRightRect,INT nBottomRect,
         INT nXRadial1,INT nYRadial1,INT nXRadial2,INT nYRadial2);
void SetTextColor(HDC hdc,COLORREF crColor);
void SetPixel(HDC hdc,INT X,INT Y,COLORREF crColor);
void SetPolyFillMode(HDC hdc,INT iPolyFillMode);
void SetBkMode(HDC hdc,INT iBkMode);
void SetTextAlign(HDC hdc,UINT fMode);
void SetTextCharacterExtra(HDC hdc,INT nCharExtra);
BOOL TextOut(HDC hdc,INT X,INT Y,LPCTSTR lpszString,UINT cbCount);
BOOL Rectangle(HDC hdc,INT nLeftRect,INT nTopRect,INT nRightRect,INT nBottomRect);
HFONT CreateFontIndirect(LOGFONT *lplf);
HBRUSH CreateSolidBrush(COLORREF crColor);
HBRUSH CreateBrushIndirect(LOGBRUSH *lplb);
BOOL MoveTo(HDC hdc,INT nX,INT nY);
BOOL LineTo(HDC hdc,INT nXEnd,INT nYEnd);
HPEN CreatePen(INT fnPenStyle,INT nWidth,COLORREF crColor);
HGDIOBJ SelectObject(HDC hdc,void *hgdiobj);
BOOL DeleteObject(void *hObject);
BOOL SetWindowOrg(HDC hdc,INT X,INT Y);
BOOL SetWindowExt(HDC hdc,INT nXExtent,INT nYExtent);
void SetMapMode(HDC hdc,INT fnMapMode);
void CloseMetaFile(HDC hdc,METAHEADER *mh);
HDC CreateMetaFile(LPCTSTR lpszFile);



















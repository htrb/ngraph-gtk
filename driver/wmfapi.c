/**
 *
 * $Id: wmfapi.c,v 1.2 2008/06/10 13:41:41 hito Exp $
 *
 * This is free software; you can redistribute it and/or modify it.
 *
 * Original author: Satoshi ISHIZAKA
 *                  isizaka@msa.biglobe.ne.jp
 **/


#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include "wmfapi.h"

#define TRUE 1
#define FALSE 0

typedef struct tagREC {
    DWORD rdSize;
    WORD  rdFunction;
    WORD  rdParm[10];
} REC;

int RECN,MAXREC;

#define ALLOCSIZE 512

struct table {
    unsigned int size;
    unsigned int num;
    char *data;
} tbl;

void tblinit(void)
{
  tbl.num=0;
  tbl.size=0;
  tbl.data=NULL;
}

void tbldone(void)
{
  if (tbl.data!=NULL) free(tbl.data);
  tbl.data=NULL;
}

int tbladd(void)
{
  int i,size;
  char *data;

  for (i=0;i<tbl.num;i++) {
    if (tbl.data[i]==0) {
      tbl.data[i]=1;
      return i;
    }
  }
  if (tbl.num==tbl.size) {
    size=tbl.size+ALLOCSIZE;
    if ((data=realloc(tbl.data,size))==NULL) {
      fprintf(stderr,"error: memory allocation.\n");
    }
    tbl.data=data;
    tbl.size=size;
  }
  tbl.data[tbl.num]=1;
  tbl.num++;
  return tbl.num-1;
}

void tbldel(int po)
{
  if (po<tbl.num) tbl.data[po]=0;
}

BOOL Polygon(HDC hdc,POINT *lpPoints,INT nCount)
{
  REC r;
  int i;

  r.rdSize=4+nCount*2;
  r.rdFunction=0x0324;
  r.rdParm[0]=nCount;
  write(hdc,&r,8);
  for (i=0;i<nCount;i++) {
    write(hdc,&(lpPoints[i].x),2);
    write(hdc,&(lpPoints[i].y),2);
  }
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;

  return 0;
}

BOOL Ellipse(HDC hdc,INT nLeftRect,INT nTopRect,INT nRightRect,INT nBottomRect)
{
  REC r;

  r.rdSize=7;
  r.rdFunction=0x0418;
  r.rdParm[0]=nBottomRect;
  r.rdParm[1]=nRightRect;
  r.rdParm[2]=nTopRect;
  r.rdParm[3]=nLeftRect;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;

  return 0;
}

BOOL Arc(HDC hdc,INT nLeftRect,INT nTopRect,INT nRightRect,INT nBottomRect,
         INT nXStartArc,INT nYStartArc,INT nXEndArc,INT nYEndArc)
{
  REC r;

  r.rdSize=11;
  r.rdFunction=0x0817;
  r.rdParm[0]=nYEndArc;
  r.rdParm[1]=nXEndArc;
  r.rdParm[2]=nYStartArc;
  r.rdParm[3]=nXStartArc;
  r.rdParm[4]=nBottomRect;
  r.rdParm[5]=nRightRect;
  r.rdParm[6]=nTopRect;
  r.rdParm[7]=nLeftRect;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;

  return 0;
}

BOOL Pie(HDC hdc,INT nLeftRect,INT nTopRect,INT nRightRect,INT nBottomRect,
         INT nXRadial1,INT nYRadial1,INT nXRadial2,INT nYRadial2)
{
  REC r;

  r.rdSize=11;
  r.rdFunction=0x081a;
  r.rdParm[0]=nYRadial2;
  r.rdParm[1]=nXRadial2;
  r.rdParm[2]=nYRadial1;
  r.rdParm[3]=nXRadial1;
  r.rdParm[4]=nBottomRect;
  r.rdParm[5]=nRightRect;
  r.rdParm[6]=nTopRect;
  r.rdParm[7]=nLeftRect;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;

  return 0;
}

BOOL Chord(HDC hdc,INT nLeftRect,INT nTopRect,INT nRightRect,INT nBottomRect,
         INT nXRadial1,INT nYRadial1,INT nXRadial2,INT nYRadial2)
{
  REC r;

  r.rdSize=11;
  r.rdFunction=0x0830;
  r.rdParm[0]=nYRadial2;
  r.rdParm[1]=nXRadial2;
  r.rdParm[2]=nYRadial1;
  r.rdParm[3]=nXRadial1;
  r.rdParm[4]=nBottomRect;
  r.rdParm[5]=nRightRect;
  r.rdParm[6]=nTopRect;
  r.rdParm[7]=nLeftRect;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;

  return 0;
}

void SetPixel(HDC hdc,INT X,INT Y,COLORREF crColor)
{
  REC r;

  r.rdSize=7;
  r.rdFunction=0x041f;
  r.rdParm[0]=crColor & 0xffff;
  r.rdParm[1]=crColor >> 16;
  r.rdParm[2]=Y;
  r.rdParm[3]=X;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
}

void SetPolyFillMode(HDC hdc,INT iPolyFillMode)
{
  REC r;

  r.rdSize=4;
  r.rdFunction=0x0106;
  r.rdParm[0]=iPolyFillMode;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
}

void SetBkMode(HDC hdc,INT iBkMode)
{
  REC r;

  r.rdSize=4;
  r.rdFunction=0x0102;
  r.rdParm[0]=iBkMode;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
}

void SetTextAlign(HDC hdc,UINT fMode)
{
  REC r;

  r.rdSize=4;
  r.rdFunction=0x012e;
  r.rdParm[0]=fMode;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
}

void SetTextCharacterExtra(HDC hdc,INT nCharExtra)
{
  REC r;

  r.rdSize=4;
  r.rdFunction=0x0108;
  r.rdParm[0]=nCharExtra;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
}

BOOL TextOut(HDC hdc,INT X,INT Y,LPCTSTR lpszString,UINT cbCount)
{
  REC r;
  int size;
  size=(cbCount+1)>>1;
  r.rdSize=6+size;
  r.rdFunction=0x0521;
  r.rdParm[0]=cbCount;
  write(hdc,&r,8);
  write(hdc,lpszString,size*2);
  write(hdc,&Y,2);
  write(hdc,&X,2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
  return TRUE;
}

void SetTextColor(HDC hdc,COLORREF crColor)
{
  REC r;
  r.rdSize=5;
  r.rdFunction=0x0209;
  r.rdParm[0]=crColor & 0xffff;
  r.rdParm[1]=crColor >> 16;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
}

BOOL Rectangle(HDC hdc,INT nLeftRect,INT nTopRect,INT nRightRect,INT nBottomRect)
{
  REC r;

  r.rdSize=7;
  r.rdFunction=0x041b;
  r.rdParm[0]=nBottomRect;
  r.rdParm[1]=nRightRect;
  r.rdParm[2]=nTopRect;
  r.rdParm[3]=nLeftRect;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
  return TRUE;
}

BOOL MoveTo(HDC hdc,INT nX,INT nY)
{
  REC r;

  r.rdSize=5;
  r.rdFunction=0x0214;
  r.rdParm[0]=nY;
  r.rdParm[1]=nX;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
  return TRUE;
}

BOOL LineTo(HDC hdc,INT nXEnd,INT nYEnd)
{
  REC r;

  r.rdSize=5;
  r.rdFunction=0x0213;
  r.rdParm[0]=nYEnd;
  r.rdParm[1]=nXEnd;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
  return TRUE;
}

HFONT CreateFontIndirect(LOGFONT *lplf)
{
  HFONT font;

  if ((font=malloc(sizeof(struct _HFONT)))!=NULL) {
    font->Type=TYPEFONT;
    font->Index=-1;
    font->Dc=-1;
    font->Font=*lplf;
  }
  return font;
}

HBRUSH CreateSolidBrush(COLORREF crColor)
{
  LOGBRUSH lb;

  lb.lbStyle=BS_SOLID;
  lb.lbColor=crColor;
  lb.lbHatch=0;
  return CreateBrushIndirect(&lb);
}

HBRUSH CreateBrushIndirect(LOGBRUSH *lplb)
{
  HBRUSH brush;

  if ((brush=malloc(sizeof(struct _HBRUSH)))!=NULL) {
    brush->Type=TYPEBRUSH;
    brush->Index=-1;
    brush->Dc=-1;
    brush->Brush=*lplb;
  }
  return brush;
}

HPEN CreatePen(INT fnPenStyle,INT nWidth,COLORREF crColor)
{
  HPEN pen;

  if ((pen=malloc(sizeof(struct _HPEN)))!=NULL) {
    if (nWidth<1) nWidth=1;
    pen->Type=TYPEPEN;
    pen->Index=-1;
    pen->Dc=-1;
    pen->Pen.lopnStyle=fnPenStyle;
    pen->Pen.lopnWidth.x=nWidth;
    pen->Pen.lopnWidth.y=0;
    pen->Pen.lopnColor=crColor;
  }
  return pen;
}

HGDIOBJ SelectObject(HDC hdc,void *hgdiobj)
{
  HPEN pen;
  HBRUSH brush;
  HFONT font;
  REC r;

  if (hgdiobj==NULL) return NULL;
  switch (((HGDIOBJ)hgdiobj)->Type) {
    case TYPEPEN:
      pen=(HPEN)hgdiobj;
      if (pen->Index==-1) {
        pen->Index=tbladd();
        pen->Dc=hdc;
        r.rdSize=3+5;
        r.rdFunction=0x02fa;
        write(hdc,&r,6);
        write(hdc,&(pen->Pen.lopnStyle),sizeof(pen->Pen.lopnStyle));
        write(hdc,&(pen->Pen.lopnWidth),sizeof(pen->Pen.lopnWidth));
        write(hdc,&(pen->Pen.lopnColor),sizeof(pen->Pen.lopnColor));
        RECN+=r.rdSize;
        if (MAXREC<r.rdSize) MAXREC=r.rdSize;
      }
      r.rdSize=4;
      r.rdFunction=0x012d;
      r.rdParm[0]=pen->Index;
      write(hdc,&r,r.rdSize*2);
      RECN+=r.rdSize;
      if (MAXREC<r.rdSize) MAXREC=r.rdSize;
      break;
    case TYPEBRUSH:
      brush=(HBRUSH)hgdiobj;
      if (brush->Index==-1) {
        brush->Index=tbladd();
        brush->Dc=hdc;
        r.rdSize=3+4;
        r.rdFunction=0x02fc;
        write(hdc,&r,6);
        write(hdc,&(brush->Brush.lbStyle),sizeof(brush->Brush.lbStyle));
        write(hdc,&(brush->Brush.lbColor),sizeof(brush->Brush.lbColor));
        write(hdc,&(brush->Brush.lbHatch),sizeof(brush->Brush.lbHatch));
        RECN+=r.rdSize;
        if (MAXREC<r.rdSize) MAXREC=r.rdSize;
      }
      r.rdSize=4;
      r.rdFunction=0x012d;
      r.rdParm[0]=brush->Index;
      write(hdc,&r,r.rdSize*2);
      RECN+=r.rdSize;
      if (MAXREC<r.rdSize) MAXREC=r.rdSize;
      break;
    case TYPEFONT:
      font=(HFONT)hgdiobj;
      if (font->Index==-1) {
        font->Index=tbladd();
        font->Dc=hdc;
        r.rdSize=3+25;
        r.rdFunction=0x02fb;
        write(hdc,&r,6);
        write(hdc,&(font->Font.lfHeight),sizeof(font->Font.lfHeight));
        write(hdc,&(font->Font.lfWidth),sizeof(font->Font.lfWidth));
        write(hdc,&(font->Font.lfEscapement),sizeof(font->Font.lfEscapement));
        write(hdc,&(font->Font.lfOrientation),sizeof(font->Font.lfOrientation));
        write(hdc,&(font->Font.lfWeight),sizeof(font->Font.lfWeight));
        write(hdc,&(font->Font.lfItalic),sizeof(font->Font.lfItalic));
        write(hdc,&(font->Font.lfUnderline),sizeof(font->Font.lfUnderline));
        write(hdc,&(font->Font.lfStrikeOut),sizeof(font->Font.lfStrikeOut));
        write(hdc,&(font->Font.lfCharSet),sizeof(font->Font.lfCharSet));
        write(hdc,&(font->Font.lfOutPrecision),sizeof(font->Font.lfOutPrecision));
        write(hdc,&(font->Font.lfClipPrecision),sizeof(font->Font.lfClipPrecision));
        write(hdc,&(font->Font.lfQuality),sizeof(font->Font.lfQuality));
        write(hdc,&(font->Font.lfPitchAndFamily),sizeof(font->Font.lfPitchAndFamily));
        write(hdc,font->Font.lfFaceName,LF_FACESIZE);
        RECN+=r.rdSize;
        if (MAXREC<r.rdSize) MAXREC=r.rdSize;
      }
      r.rdSize=4;
      r.rdFunction=0x012d;
      r.rdParm[0]=font->Index;
      write(hdc,&r,r.rdSize*2);
      RECN+=r.rdSize;
      if (MAXREC<r.rdSize) MAXREC=r.rdSize;
      break;
    default:
      break;
  }
  return hgdiobj;
}

BOOL DeleteObject(void *hObject)
{
  REC r;
  HGDIOBJ obj;

  if (hObject!=NULL) {
    obj=(HGDIOBJ)hObject;
    if (obj->Index!=-1) {
      r.rdSize=4;
      r.rdFunction=0x01f0;
      r.rdParm[0]=obj->Index;
      write(obj->Dc,&r,r.rdSize*2);
      tbldel(obj->Index);
      RECN+=r.rdSize;
      if (MAXREC<r.rdSize) MAXREC=r.rdSize;
    }
    free(obj);
  }
  return TRUE;
}

BOOL SetWindowOrg(HDC hdc,INT X,INT Y)
{
  REC r;

  r.rdSize=5;
  r.rdFunction=0x020b;
  r.rdParm[0]=Y;
  r.rdParm[1]=X;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
  return TRUE;
}

BOOL SetWindowExt(HDC hdc,INT nXExtent,INT nYExtent)
{
  REC r;

  r.rdSize=5;
  r.rdFunction=0x020c;
  r.rdParm[0]=nYExtent;
  r.rdParm[1]=nXExtent;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
  return TRUE;
}

void SetMapMode(HDC hdc,INT fnMapMode)
{
  REC r;

  r.rdSize=4;
  r.rdFunction=0x0103;
  r.rdParm[0]=fnMapMode;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
}

void CloseMetaFile(HDC hdc,METAHEADER *mh)
{
  REC r;

  r.rdSize=3;
  r.rdFunction=0;
  write(hdc,&r,r.rdSize*2);
  RECN+=r.rdSize;
  if (MAXREC<r.rdSize) MAXREC=r.rdSize;
  mh->mtType=1;
  mh->mtHeaderSize=9;
  mh->mtVersion=0x300;
  mh->mtSize=RECN+mh->mtHeaderSize;
  mh->mtNoObjects=tbl.num;
  mh->mtMaxRecord=MAXREC;
  mh->mtNoParameters=0;
  close(hdc);
  tbldone();
}

HDC CreateMetaFile(LPCTSTR lpszFile)
{
  HDC dc;

  tblinit();
  RECN=0;
  MAXREC=0;
  dc=open(lpszFile,O_CREAT|O_TRUNC|O_WRONLY,S_IREAD|S_IWRITE);
  return dc;
}

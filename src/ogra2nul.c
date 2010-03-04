/* 
 * $Id: ogra2nul.c,v 1.7 2010/03/04 08:30:16 hito Exp $
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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "common.h"

#include "ngraph.h"
#include "object.h"
#include "ioutil.h"
#include "gra.h"
#include "nstring.h"
#include "jnstring.h"
#include "mathfn.h"
#include "nconfig.h"

#define NAME "gra2null"
#define PARENT "gra2"
#define OVERSION  "1.00.00"

#define ERRCONF 100

static char *g2nulerrorlist[]={
  "",
};

#define ERRNUM (sizeof(g2nulerrorlist) / sizeof(*g2nulerrorlist))

static int 
g2nulinit(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  if (_exeparent(obj,(char *)argv[1],inst,rval,argc,argv)) return 1;
  return 0;
}


static int 
g2nuldone(struct objlist *obj,char *inst,char *rval,int argc,char **argv)
{
  return 0;
}

static int HelveticaSet[256]={
278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
278,278,355,556,556,889,667,222,333,333,389,584,278,584,278,278,
556,556,556,556,556,556,556,556,556,556,278,278,584,584,584,556,
1015,667,667,722,722,667,611,778,722,278,500,667,556,833,722,778,
667,778,722,667,611,722,667,944,667,667,611,278,278,278,469,556,
222,556,556,500,556,556,278,556,556,222,222,500,222,833,556,556,
556,556,333,500,278,556,500,722,500,500,500,334,260,334,584,278,
278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
278,333,333,333,333,333,333,333,333,278,333,333,278,333,333,333,
278,333,556,556,556,556,260,556,333,737,370,556,584,333,737,333,
400,584,333,333,333,556,537,278,333,333,365,556,834,834,834,611,
667,667,667,667,667,667,1000,722,667,667,667,667,278,278,278,278,
722,722,778,778,778,778,778,584,778,722,722,722,722,667,667,611,
556,556,556,556,556,556,889,500,556,556,556,556,278,278,278,278,
556,556,556,556,556,556,556,584,611,556,556,556,556,500,556,500};

static int HelveticaBoldSet[256]={
278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
278,333,474,556,556,889,722,278,333,333,389,584,278,584,278,278,
556,556,556,556,556,556,556,556,556,556,333,333,584,584,584,611,
975,722,722,722,722,667,611,778,722,278,556,722,611,833,722,778,
667,778,722,667,611,722,667,944,667,667,611,333,278,333,584,556,
278,556,611,556,611,556,333,611,611,278,278,556,278,889,611,611,
611,611,389,556,333,611,556,778,556,556,500,389,280,389,584,278,
278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,278,
278,333,333,333,333,333,333,333,333,278,333,333,278,333,333,333,
278,333,556,556,556,556,280,556,333,737,370,556,584,333,737,333,
400,584,333,333,333,611,556,278,333,333,365,556,834,834,834,611,
722,722,722,722,722,722,1000,722,667,667,667,667,278,278,278,278,
722,722,778,778,778,778,778,584,778,722,722,722,722,667,667,611,
556,556,556,556,556,556,889,556,556,556,556,556,278,278,278,278,
611,611,611,611,611,611,611,584,611,611,611,611,611,556,611,556};

static int TimesRomanSet[256]={
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,333,408,500,500,833,778,333,333,333,500,564,250,564,250,278,
500,500,500,500,500,500,500,500,500,500,278,278,564,564,564,444,
921,722,667,667,722,611,556,722,722,333,389,722,611,889,722,722,
556,722,667,556,611,722,722,944,722,722,611,333,278,333,469,500,
333,444,500,444,500,444,333,500,500,278,278,500,278,778,500,500,
500,500,333,389,278,500,500,722,500,500,444,480,200,480,541,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
278,333,333,333,333,333,333,333,333,250,333,333,250,333,333,333,
250,333,500,500,500,500,200,500,333,760,276,500,564,333,760,333,
400,564,300,300,333,500,453,250,333,300,310,500,750,750,750,444,
722,722,722,722,722,722,889,667,611,611,611,611,333,333,333,333,
722,722,722,722,722,722,722,564,722,722,722,722,722,722,556,500,
444,444,444,444,444,444,667,444,444,444,444,444,278,278,278,278,
500,500,500,500,500,500,500,564,500,500,500,500,500,500,500,500};

static int TimesBoldSet[256]={
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,333,555,500,500,1000,833,333,333,333,500,570,250,570,250,278,
500,500,500,500,500,500,500,500,500,500,333,333,570,570,570,500,
930,722,667,722,722,667,611,778,778,389,500,778,667,944,722,778,
611,778,722,556,667,722,722,1000,722,722,667,333,278,333,581,500,
333,500,556,444,556,444,333,500,556,278,333,556,278,833,556,500,
556,556,444,389,333,556,500,722,500,500,444,394,220,394,520,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
278,333,333,333,333,333,333,333,333,250,333,333,250,333,333,333,
250,333,500,500,500,500,220,500,333,747,300,500,570,333,747,333,
400,570,300,300,333,556,540,250,333,300,330,500,750,750,750,500,
722,722,722,722,722,722,1000,722,667,667,667,667,389,389,389,389,
722,722,778,778,778,778,778,570,778,722,722,722,722,722,611,556,
500,500,500,500,500,500,722,444,444,444,444,444,278,278,278,278,
500,556,500,500,500,500,500,570,500,556,556,556,556,500,556,500};

static int TimesItalicSet[256]={
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,333,420,500,500,833,778,333,333,333,500,675,250,675,250,278,
500,500,500,500,500,500,500,500,500,500,333,333,675,675,675,500,
920,611,611,667,722,611,611,722,722,333,444,667,556,833,667,722,
611,722,611,500,556,722,611,833,611,556,556,389,278,389,422,500,
333,500,500,444,500,444,278,500,500,278,278,444,278,722,500,500,
500,500,389,389,278,500,444,667,444,444,389,400,275,400,541,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
278,333,333,333,333,333,333,333,333,250,333,333,250,333,333,333,
250,389,500,500,500,500,275,500,333,760,276,500,675,333,760,333,
400,675,300,300,333,500,523,250,333,300,310,500,750,750,750,500,
611,611,611,611,611,611,889,667,611,611,611,611,333,333,333,333,
722,667,722,722,722,722,722,675,722,722,722,722,722,556,611,500,
500,500,500,500,500,500,667,444,444,444,444,444,278,278,278,278,
500,500,500,500,500,500,500,675,500,500,500,500,500,444,500,444};

static int TimesBoldItalicSet[256]={
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,389,555,500,500,833,778,333,333,333,500,570,250,606,250,278,
500,500,500,500,500,500,500,500,500,500,333,333,570,570,570,500,
832,667,667,667,722,667,667,722,778,389,500,667,611,889,722,722,
611,722,667,556,611,722,667,889,667,611,611,333,278,333,570,500,
333,500,500,444,500,444,333,500,556,278,278,500,278,778,556,500,
500,500,389,389,278,556,444,667,500,444,389,348,220,348,570,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
278,333,333,333,333,333,333,333,333,250,333,333,250,333,333,333,
250,389,500,500,500,500,220,500,333,747,266,500,606,333,747,333,
400,570,300,300,333,576,500,250,333,300,300,500,750,750,750,500,
667,667,667,667,667,667,944,667,667,667,667,667,389,389,389,389,
722,722,722,722,722,722,722,570,722,722,722,722,722,611,611,500,
500,500,500,500,500,500,722,444,444,444,444,444,278,278,278,278,
500,556,500,500,500,500,500,570,500,556,556,556,556,444,500,444};

static int SymbolSet[256]={
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,333,713,500,549,833,778,439,333,333,500,549,250,549,250,278,
500,500,500,500,500,500,500,500,500,500,278,278,549,549,549,444,
549,722,667,722,612,611,763,603,722,333,631,722,686,889,722,722,
768,741,556,592,611,690,439,768,645,795,611,333,863,333,658,500,
500,631,549,549,494,439,521,411,603,329,603,549,549,576,521,549,
549,521,549,603,439,576,713,686,493,686,494,480,200,480,549,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,250,
250,620,247,549,167,713,500,753,753,753,753,1042,987,603,987,603,
400,549,411,549,549,713,494,460,549,549,549,549,1000,603,1000,658,
823,686,795,987,768,768,823,768,768,713,713,713,713,713,713,713,
768,713,790,790,890,823,549,250,713,603,603,1042,987,603,987,603,
494,329,790,790,786,713,384,384,384,384,384,384,494,494,494,494,
250,329,274,686,686,686,384,384,384,384,384,384,494,494,494,250};

static int 
g2nul_charwidth(struct objlist *obj,char *inst,char *rval,
                    int argc,char **argv)
{
  unsigned int ch;
  char *font;
  int size;

  ch=*(unsigned int *)(argv[3]);
  size=*(int *)(argv[4]);
  font=(char *)(argv[5]);
  if ((strcmp(font,"Times")==0) || (strcmp(font,"Tim")==0))
    *(int *)rval=nround(25.4/72000.0*size*TimesRomanSet[ch]);
  else if ((strcmp(font,"TimesBold")==0) || (strcmp(font,"TimB")==0))
    *(int *)rval=nround(25.4/72000.0*size*TimesBoldSet[ch]);
  else if ((strcmp(font,"TimesItalic")==0) || (strcmp(font,"TimI")==0))
    *(int *)rval=nround(25.4/72000.0*size*TimesItalicSet[ch]);
  else if ((strcmp(font,"TimesBoldItalic")==0) || (strcmp(font,"TimBI")==0))
    *(int *)rval=nround(25.4/72000.0*size*TimesBoldItalicSet[ch]);
  else if ((strcmp(font,"Helvetica")==0) || (strcmp(font,"Helv")==0)
        || (strcmp(font,"HelveticaOblique")==0) || (strcmp(font,"HelvO")==0)
        || (strcmp(font,"HelveticaItalic")==0) || (strcmp(font,"HelvI")==0))
    *(int *)rval=nround(25.4/72000.0*size*HelveticaSet[ch]);
  else if ((strcmp(font,"HelveticaBold")==0) || (strcmp(font,"HelvB")==0)
     || (strcmp(font,"HelveticaBoldOblique")==0) || (strcmp(font,"HelvBO")==0)
     || (strcmp(font,"HelveticaBoldItalic")==0) || (strcmp(font,"HelvBI")==0))
    *(int *)rval=nround(25.4/72000.0*size*HelveticaBoldSet[ch]);
  else if ((strcmp(font,"Symbol")==0) || (strcmp(font,"Sym")==0)
        || (strcmp(font,"SymbolBold")==0) || (strcmp(font,"SymB")==0)
        || (strcmp(font,"SymbolItalic")==0) || (strcmp(font,"SymI")==0)
        || (strcmp(font,"SymbolBoldItalic")==0) || (strcmp(font,"SymBI")==0))
    *(int *)rval=nround(25.4/72000.0*size*SymbolSet[ch]);
  else if ((strcmp(font,"Gothic")==0) || (strcmp(font,"Goth")==0)
        || (strcmp(font,"GothicBold")==0) || (strcmp(font,"GothB")==0)
        || (strcmp(font,"GothicItalic")==0) || (strcmp(font,"GothI")==0)
        || (strcmp(font,"GothicBoldItalic")==0) || (strcmp(font,"GothBI")==0)
        || (strcmp(font,"Mincho")==0) || (strcmp(font,"Min")==0)
        || (strcmp(font,"MinchoBold")==0) || (strcmp(font,"MinB")==0)
        || (strcmp(font,"MinchoItalic")==0) || (strcmp(font,"MinI")==0)
        || (strcmp(font,"MinchoBoldItalic")==0) || (strcmp(font,"MinBI")==0))
    *(int *)rval=nround(25.4/72000.0*size*1000);
  else *(int *)rval=nround(25.4/72000.0*size*600);
  return 0;
}

static int 
g2nul_charheight(struct objlist *obj,char *inst,char *rval,
                     int argc,char **argv)
{
  char *font;
  int size;

  size=*(int *)(argv[3]);
  font=(char *)(argv[4]);
  if ((strcmp(font,"Times")==0) || (strcmp(font,"Tim")==0))
    *(int *)rval=nround(25.4/72000.0*size*662);
  else if ((strcmp(font,"TimesBold")==0) || (strcmp(font,"TimB")==0))
    *(int *)rval=nround(25.4/72000.0*size*676);
  else if ((strcmp(font,"TimesItalic")==0) || (strcmp(font,"TimI")==0))
    *(int *)rval=nround(25.4/72000.0*size*653);
  else if ((strcmp(font,"TimesBoldItalic")==0) || (strcmp(font,"TimBI")==0))
    *(int *)rval=nround(25.4/72000.0*size*669);
  else if ((strcmp(font,"Helvetica")==0) || (strcmp(font,"Helv")==0)
        || (strcmp(font,"HelveticaOblique")==0) || (strcmp(font,"HelvO")==0)
        || (strcmp(font,"HelveticaItalic")==0) || (strcmp(font,"HelvI")==0))
    *(int *)rval=nround(25.4/72000.0*size*718);
  else if ((strcmp(font,"HelveticaBold")==0) || (strcmp(font,"HelvB")==0)
     || (strcmp(font,"HelveticaBoldOblique")==0) || (strcmp(font,"HelvBO")==0)
     || (strcmp(font,"HelveticaBoldItalic")==0) || (strcmp(font,"HelvBI")==0))
    *(int *)rval=nround(25.4/72000.0*size*718);
  else if ((strcmp(font,"Symbol")==0) || (strcmp(font,"Sym")==0)
        || (strcmp(font,"SymbolBold")==0) || (strcmp(font,"SymB")==0)
        || (strcmp(font,"SymbolItalic")==0) || (strcmp(font,"SymI")==0)
        || (strcmp(font,"SymbolBoldItalic")==0) || (strcmp(font,"SymBI")==0))
    *(int *)rval=nround(25.4/72000.0*size*673);
  else if ((strcmp(font,"CourierBold")==0) || (strcmp(font,"CourB")==0)
        || (strcmp(font,"CourierBoldOblique")==0) || (strcmp(font,"CourBO")==0)
        || (strcmp(font,"CourierBoldItalic")==0) || (strcmp(font,"CourBI")==0))
    *(int *)rval=nround(25.4/72000.0*size*562);
  else if ((strcmp(font,"Gothic")==0) || (strcmp(font,"Goth")==0)
        || (strcmp(font,"GothicBold")==0) || (strcmp(font,"GothB")==0)
        || (strcmp(font,"GothicItalic")==0) || (strcmp(font,"GothI")==0)
        || (strcmp(font,"GothicBoldItalic")==0) || (strcmp(font,"GothBI")==0))
    *(int *)rval=nround(25.4/72000.0*size*791);
  else if ((strcmp(font,"Mincho")==0) || (strcmp(font,"Min")==0)
        || (strcmp(font,"MinchoBold")==0) || (strcmp(font,"MinB")==0)
        || (strcmp(font,"MinchoItalic")==0) || (strcmp(font,"MinI")==0)
        || (strcmp(font,"MinchoBoldItalic")==0) || (strcmp(font,"MinBI")==0))
    *(int *)rval=nround(25.4/72000.0*size*807);
  else *(int *)rval=nround(25.4/72000.0*size*562);
  return 0;
}

static int 
g2nul_chardescent(struct objlist *obj,char *inst,char *rval,
                     int argc,char **argv)
{
  char *font;
  int size;

  size=*(int *)(argv[3]);
  font=(char *)(argv[4]);
  *(int *)rval=nround(25.4/72000.0*size*250);
  return 0;
}

static struct objtable gra2null[] = {
  {"init",NVFUNC,NEXEC,g2nulinit,NULL,0},
  {"done",NVFUNC,NEXEC,g2nuldone,NULL,0},
  {"next",NPOINTER,0,NULL,NULL,0},
  {"_charwidth",NIFUNC,0,g2nul_charwidth,NULL,0},
  {"_charascent",NIFUNC,0,g2nul_charheight,NULL,0},
  {"_chardescent",NIFUNC,0,g2nul_chardescent,NULL,0},
};

#define TBLNUM (sizeof(gra2null) / sizeof(*gra2null))

void *
addgra2null(void)
/* addgra2null() returns NULL on error */
{
  return addobject(NAME,NULL,PARENT,OVERSION,TBLNUM,gra2null,ERRNUM,g2nulerrorlist,NULL,NULL);
}

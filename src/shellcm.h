/* 
 * $Id: shellcm.h,v 1.8 2009/11/12 01:36:45 hito Exp $
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

#ifndef SHELLCM_HEADER
#define SHELLCM_HEADER
#include "shell.h"

int cmcd(struct nshell *nshell,int argc,char **argv);
int cmecho(struct nshell *nshell,int argc,char **argv);
int cmbasename(struct nshell *nshell,int argc,char **argv);
int cmdirname(struct nshell *nshell,int argc,char **argv);
int cmeval(struct nshell *nshell,int argc,char **argv);
int cmexit(struct nshell *nshell,int argc,char **argv);
int cmexport(struct nshell *nshell,int argc,char **argv);
int cmpwd(struct nshell *nshell,int argc,char **argv);
int cmset(struct nshell *nshell,int argc,char **argv);
int cmshift(struct nshell *nshell,int argc,char **argv);
int cmtype(struct nshell *nshell,int argc,char **argv);
int cmunset(struct nshell *nshell,int argc,char **argv);
int cmobject(struct nshell *nshell,int argc,char **argv);
int cmderive(struct nshell *nshell,int argc,char **argv);
int cmnew(struct nshell *nshell,int argc,char **argv);
int cmdel(struct nshell *nshell,int argc,char **argv);
int cmexist(struct nshell *nshell,int argc,char **argv);
int cmget(struct nshell *nshell,int argc,char **argv);
int cmput(struct nshell *nshell,int argc,char **argv);
int cmcpy(struct nshell *nshell,int argc,char **argv);
int cmmove(struct nshell *nshell,int argc,char **argv);
int cmmovetop(struct nshell *nshell,int argc,char **argv);
int cmcopy(struct nshell *nshell,int argc,char **argv);
int cmexch(struct nshell *nshell,int argc,char **argv);
int cmexe(struct nshell *nshell,int argc,char **argv);
int cmdexpr(struct nshell *nshell,int argc,char **argv);
int cmread(struct nshell *nshell,int argc,char **argv);
int cmseq(struct nshell *nshell, int argc, char **argv);

#endif

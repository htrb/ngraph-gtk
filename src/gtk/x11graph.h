/*
 * $Id: x11graph.h,v 1.3 2009-06-09 06:38:53 hito Exp $
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

#ifndef X11GRAPH_HEADER

void CmGraphQuit(void);
void CmGraphNewMenu(int menu_id);
void CmGraphLoad(void);
void CmGraphSave(void);
void CmGraphOverWrite(void);
void CmGraphShell(void);
void CmGraphDirectory(void);
void CmHelpHelp(void);
void CmHelpAbout(void);
void CmHelpDemo(void);
void CmGraphSwitch(void);
void CmGraphPage(int new_graph);
int chdir_to_ngp(const char *fname);

int set_paper_type(int w, int h);

enum LOAD_PATH_TYPE {
  LOAD_PATH_UNCHANGE,
  LOAD_PATH_FULL,
  LOAD_PATH_BASE,
};

extern char *LoadPathStr[];

#endif	/* X11GRAPH_HEADER */

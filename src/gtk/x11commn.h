/*
 * $Id: x11commn.h,v 1.11 2010-03-04 08:30:17 hito Exp $
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

void OpenGRA(void);
void CloseGRA(void);
void ChangeGRA(void);
void CheckPage(void);
void SetPageSettingsToGRA(void);
void GetPageSettingsFromGRA(void);
void AxisDel(int id);
void AxisMove(int id1, int id2);
void AdjustAxis(void);
void FitCopy(struct objlist *obj, int did, int sid);
void FitDel(struct objlist *obj, int id);
void ArrayDel(struct objlist *obj, int id);
void FitClear(void);
void LoadPrmFile(char *File);
void LoadNgpFile(char *File, int ignorepath, int expand,
		 char *exdir, int console, char *option);
int SaveDrawrable(char *name, int storedata, int storemerge);
int GraphSave(int overwrite);
void DeleteDrawable(void);
void FileAutoScale(void);
int CheckSave(void);
void AddDataFileList(const char *file);
void SetFileName(char *name);
int CheckIniFile(void);
int allocate_console(void);
void free_console(int allocnow);
char *FileCB(struct objlist *obj, int id);
char *PlotFileCB(struct objlist *obj, int id);
char *PlotFileArrayCB(struct objlist *obj, int id);
int SetFileHidden(void);
void ProgressDialogCreate(char *title);
void ProgressDialogSetTitle(char *title);
void ProgressDialogFinalize(void);
void ErrorMessage(void);

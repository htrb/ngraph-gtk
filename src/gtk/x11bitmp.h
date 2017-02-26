/*
 * $Id: x11bitmp.h,v 1.5 2009-01-07 02:39:34 hito Exp $
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

#include "gtk_common.h"
#include "ioutil.h"
#include "dir_defs.h"

#define NGRAPH_ALIGN_B_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_align_b.png"
#define NGRAPH_ALIGN_HC_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_align_hc.png"
#define NGRAPH_ALIGN_L_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_align_l.png"
#define NGRAPH_ALIGN_R_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_align_r.png"
#define NGRAPH_ALIGN_T_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_align_t.png"
#define NGRAPH_ALIGN_VC_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_align_vc.png"
#define NGRAPH_ARC_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_arc.png"
#define NGRAPH_AXISPOINT_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_axispoint.png"
#define NGRAPH_AXISWIN_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_axiswin.png"
#define NGRAPH_COORDWIN_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_coordwin.png"
#define NGRAPH_CROSS_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_cross.png"
#define NGRAPH_DATAPOINT_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_datapoint.png"
#define NGRAPH_DRAW_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_draw.png"
#define NGRAPH_EVAL_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_eval.png"
#define NGRAPH_FILEWIN_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_filewin.png"
#define NGRAPH_FRAME_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_frame.png"
#define NGRAPH_GAUSS_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_gauss.png"
#define NGRAPH_INFOWIN_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_infowin.png"
#define NGRAPH_LEGENDPOINT_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_legendpoint.png"
#define NGRAPH_LEGENDWIN_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_legendwin.png"
#define NGRAPH_LINE_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_line.png"
#define NGRAPH_MARK_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_mark.png"
#define NGRAPH_MATH_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_math.png"
#define NGRAPH_MERGEWIN_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_mergewin.png"
#define NGRAPH_POINT_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_point.png"
#define NGRAPH_RECT_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_rect.png"
#define NGRAPH_SCALE_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_scale.png"
#define NGRAPH_SECTION_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_section.png"
#define NGRAPH_SINGLE_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_single.png"
#define NGRAPH_TEXT_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_text.png"
#define NGRAPH_TRIMMING_ICON_FILE	RESOURCE_PATH "/pixmaps/ngraph_trimming.png"
#define NGRAPH_ZOOM_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_zoom.png"
#define NGRAPH_UNDO_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_undo.png"

#define NGRAPH_FILEWIN_ICON48_FILE	RESOURCE_PATH "/pixmaps/ngraph_filewin48.png"
#define NGRAPH_AXISWIN_ICON48_FILE	RESOURCE_PATH "/pixmaps/ngraph_axiswin48.png"
#define NGRAPH_LEGENDWIN_ICON48_FILE	RESOURCE_PATH "/pixmaps/ngraph_legendwin48.png"
#define NGRAPH_MERGEWIN_ICON48_FILE	RESOURCE_PATH "/pixmaps/ngraph_mergewin48.png"
#define NGRAPH_COORDWIN_ICON48_FILE	RESOURCE_PATH "/pixmaps/ngraph_coordwin48.png"
#define NGRAPH_INFOWIN_ICON48_FILE	RESOURCE_PATH "/pixmaps/ngraph_infowin48.png"

#define NGRAPH_ICON_FILE		RESOURCE_PATH "/pixmaps/ngraph_icon.png"
#define NGRAPH_ICON64_FILE		RESOURCE_PATH "/pixmaps/ngraph_icon64.png"

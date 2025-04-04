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

#define NGRAPH_ICON_PATH                RESOURCE_PATH "/icons/scalable/actions"
#define NGRAPH_APP_ICON_PATH            RESOURCE_PATH "/icons/scalable/apps"
#define NGRAPH_ALIGN_B_ICON	"ngraph_align_b"
#define NGRAPH_ALIGN_HC_ICON	"ngraph_align_hc"
#define NGRAPH_ALIGN_L_ICON	"ngraph_align_l"
#define NGRAPH_ALIGN_R_ICON	"ngraph_align_r"
#define NGRAPH_ALIGN_T_ICON	"ngraph_align_t"
#define NGRAPH_ALIGN_VC_ICON	"ngraph_align_vc"
#define NGRAPH_MATH_ICON	"ngraph_math"

#define NGRAPH_SVG_ICON_FILE		NGRAPH_APP_ICON_PATH "ngraph.svg"
#define NGRAPH_ICON_FILE		RESOURCE_PATH "/icons/48x48/apps/ngraph_icon.png"
#define NGRAPH_ICON64_FILE		RESOURCE_PATH "/icons/64x64/apps/ngraph_icon.png"
#define NGRAPH_ICON128_FILE		RESOURCE_PATH "/icons/128x128/apps/ngraph_icon.png"

#define NGRAPH_DRAW_ICON	"ngraph_draw-symbolic"
#define NGRAPH_POINT_ICON	"ngraph_point-symbolic"
#define NGRAPH_LEGENDPOINT_ICON	"ngraph_legendpoint-symbolic"
#define NGRAPH_AXISPOINT_ICON	"ngraph_axispoint-symbolic"
#define NGRAPH_DATAPOINT_ICON	"ngraph_datapoint-symbolic"
#define NGRAPH_TEXT_ICON	"ngraph_text-symbolic"
#define NGRAPH_ARC_ICON		"ngraph_arc-symbolic"
#define NGRAPH_GAUSS_ICON	"ngraph_gauss-symbolic"
#define NGRAPH_LINE_ICON	"ngraph_line-symbolic"
#define NGRAPH_MARK_ICON	"ngraph_mark-symbolic"
#define NGRAPH_RECT_ICON	"ngraph_rect-symbolic"
#define NGRAPH_CROSS_ICON	"ngraph_cross-symbolic"
#define NGRAPH_FRAME_ICON	"ngraph_frame-symbolic"
#define NGRAPH_SECTION_ICON	"ngraph_section-symbolic"
#define NGRAPH_SINGLE_ICON	"ngraph_single-symbolic"
#define NGRAPH_SCALE_ICON	"ngraph_scale-symbolic"
#define NGRAPH_TRIMMING_ICON	"ngraph_trimming-symbolic"
#define NGRAPH_EVAL_ICON	"ngraph_eval-symbolic"
#define NGRAPH_ZOOM_ICON	"ngraph_zoom-symbolic"
#define NGRAPH_AXISWIN_ICON	"ngraph_axiswin-symbolic"
#define NGRAPH_FILEWIN_ICON	"ngraph_filewin-symbolic"
#define NGRAPH_MERGEWIN_ICON	"ngraph_mergewin-symbolic"
#define NGRAPH_PARAMETER_ICON	"ngraph_parameter-symbolic"
#define NGRAPH_EXCHANGE_ICON	"ngraph_exchange-symbolic"
#define NGRAPH_LINK_ICON	"ngraph_link-symbolic"

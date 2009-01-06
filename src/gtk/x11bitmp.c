/* 
 * $Id: x11bitmp.c,v 1.5 2009/01/06 01:06:11 hito Exp $
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

#define ICON_WIDTH 48
#define ICON_HEIGHT 48

#include "gtk_common.h"

const gchar *Icon_xpm[] = {
  "48 48 11 1",
  ". c #000000",
  "# c #0000ff",
  "a c None",
  "b c #00ffff",
  "c c #808000",
  "d c #808080",
  "e c #c0c0c0",
  "f c #ff0000",
  "g c #ffff00",
  "h c #ffffff",
  "i c #008080",
  /* pixels */
  "aaaaaaaaaaaaaaaaaaaaaaaadddaaaaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaaaaaaaaaddhhhdaaaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaaaaaaaddhhhhhhdaaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaaaaaddhhhhhhhhhdaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaaaddhhhhhhhhhhhhdaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaddhhhhhhhhhhhhhhhdaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaddhhhhhhhhhhhhhhhhhhdaaaaaaaaaaaaaaa",
  "aaaaaaaaaaddhhhhhhhh##hhhhhffhhhhdaaaaaaaaaaaaaa",
  "aaaaaaaaddhhhhhhhhh#bb#hhhffhhhhhhdaaaaaaaaaaaaa",
  "aaaaaaddhhhhhhhhhhh#bb#hhffhhhhhhhhdaaaaaaaaaaaa",
  "aaaaddhhhhhhhhhhhhhh##hhffhhhhhhhhhhdaaaaaaaaaaa",
  "aaddhhhhhhhhhhhhhhhhhhhffhh##hhhhhhhhdaaaaaaaaaa",
  "adhhhhhhhh##hhhh##hhhhffhh#bb#hhhhhhhhdaaaaaaaaa",
  "addhhhhhh#bb#hh#bb#hhffhhh#bb#hhhhhhhhhdaaaaaaaa",
  ".eedhhhhh#bb#hh#bb#hffhhhhh##hhhhhhhhhhhdaaaaaaa",
  ".eeedhhhhh##hhhh##hffhhhhhhhhhhhhhhhhhhhhdaaaaaa",
  ".eeeedh.hhhhhhhhhhffhhhhhhhhhhhhhhhhhhhhhhdaaaaa",
  ".eeeeed..h.hhhhhhffhhhhhhhhhhhhhh.hhhh..hhhdaaaa",
  ".eeeeedh..hhhhhhffh##hhhhhhhhhhhhh.h..hhhhhhdaaa",
  ".eeeeed.h..hhhhffh#bb#hhhhhhhhhhhh..hhhhhhhhhdaa",
  ".eeeedhhhh..h.ffhh#bb#hhhhh.hhhh..hh.hhhhhhhhhda",
  ".eeedhhhhhh..ffhhhh##hhhhhhh.h..hhhhhhhhhhhhhhhd",
  ".eedhhhhhh.h..hhhhhhhhhhhhhh..hhhhhhhhhhhhhhhhhh",
  ".edhhhhhhhhff..h.hhhh.hhhh..ee.eeeeeeeeeeeeehhhh",
  ".dhhhhhhhhffhh..hhhhhh.h..hheeeeeeeeeeeeeeeehhhh",
  ".dhhhhhhhffhh.h..hhhhh..hhhheeeeeeeeeeeeeeeehhhh",
  "..dhhhhhffhhhhhh..hh..hh.hhheeeccccccccccccccchh",
  "a..dhhhffhhhhhhhh...hhhhhhhheeecgggggggggggggggh",
  "aa..dhhhhhhhhhhh....hhhhhhhheeecgggggggggggggggd",
  "aaa..dhhhhhhhh..hhh..hhhhhhheeecgiiiiiiiiiiiiiga",
  "aaaa..dhhhhh..hhhhhh..h.hhhheeecgiibiibiibiibiga",
  "aaaaa..dhhhhhhhhhhhhh..hhhhheeecgiibiibiibiibiga",
  "aaaaaa..dhhhhhhhhhhh.h..hhhheeecgiiiiiiiiiiiiiga",
  "aaaaaaa..dhhhhhhhhhhhhh..hhheeecggggggggggggggga",
  "aaaaaaaa..dhhhhhhhhhhhhhhhhheeecggggggggggggggga",
  "bbbbaaaaa..dbbbbhhhhhhhhhhhheeecggg##gg##gg##gga",
  "bbbbbaaaaa..bbbbhhhhhhhhhhhheedcgg.##g.##g.##gga",
  "bbbbbbaaaaa.bbbbhhhhhhhhhhhhdd.cgg..gg..gg..ggga",
  "bbbbbbbaaaaabbbbhhhhhhhhhhdd...cggggggggggggggga",
  "bbbbbbbbaaaabbbbhhhhhhhhdd.....cggg##gg##gg##gga",
  "bbbbbbbbbaaabbbbdhhhhhdd...a...cgg.##g.##g.##gga",
  "bbbbabbbbbaabbbb.dhhdd...aaa...cgg..gg..gg..ggga",
  "bbbbaabbbbbabbbb..dd...aaaaa...cggggggggggggggga",
  "bbbbaaabbbbbbbbba....aaaaaaaaaacggg##gg##gg##gga",
  "bbbbaaaabbbbbbbbaa.aaaaaaaaaaaacgg.##g.##g.##gga",
  "bbbbaaaaabbbbbbbaaaaaaaaaaaaaaacgg..gg..gg..ggga",
  "bbbbaaaaaabbbbbbaaaaaaaaaaaaaaaaggggggggggggggga",
  "bbbbaaaaaaabbbbbaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
};

const gchar *Icon_xpm_64[] = { 
  "64 64 11 1",
  ". c #000000",
  "# c #0000ff",
  "a c None",
  "b c #008080",
  "c c #00ffff",
  "d c #808000",
  "e c #808080",
  "f c #c0c0c0",
  "g c #ff0000",
  "h c #ffff00",
  "i c #ffffff",
  /* pixels */
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaeeeeaaaaaaaaaaaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaeeeiieeaaaaaaaaaaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaaaaaaaaaaaaaaaeeeiiiiieeaaaaaaaaaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaaaaaaaaaaaaaeeeiiiiiiiieeaaaaaaaaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaaaaaaaaaaaeeeiiiiiiiiiiieeaaaaaaaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaaaaaaaaaeeeiiiiiiiiiiiiiieeaaaaaaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaaaaaaaeeeiiiiiiiiiiiiiiiiieeaaaaaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaaaaaeeeiiiiiiiiiiiiiiiiiiiieeaaaaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaaaeeeiiiiiiiiiiiiiiiiiiiiiiieeaaaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaaaeeeiiiiiiiiiiiiiiiiiiiiiiiiiieeaaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaaaeeeiiiiiiiiiii##iiiiiiiiiiiiiiiieeaaaaaaaaaaaaaaaaaa",
  "aaaaaaaaaaeeeiiiiiiiiiiii#cc#iiiiiiiiiiiiiiiieeaaaaaaaaaaaaaaaaa",
  "aaaaaaaaeeeiiiiiiiiiiiiii#cc#iiiiiggiiiiiiiiiieeaaaaaaaaaaaaaaaa",
  "aaaaaaeeeiiiiiiiiiiiiiiiii##iiiiiggiiiiiiiiiiiieeaaaaaaaaaaaaaaa",
  "aaaaeeeiiiiiiiiiiiiiiiiiiiiiiiiiggiiiiiiiiiiiiiieeaaaaaaaaaaaaaa",
  "aaeeeiiiiiiiiiiiiiiiiiiiiiiiiiiggiii##iiiiiiiiiiieeaaaaaaaaaaaaa",
  "aeeiiiiiiiiiii##iiiiii##iiiiiiggiii#cc#iiiiiiiiiiieeaaaaaaaaaaaa",
  "aeeiiiiiiiiii#cc#iiii#cc#iiiiggiiii#cc#iiiiiiiiiiiieeaaaaaaaaaaa",
  ".eeeiiiiiiiii#cc#iiii#cc#iiiggiiiiii##iiiiiiiiiiiiiieeaaaaaaaaaa",
  ".efeeiiiiiiiii##iiiiii##iiiggiiiiiiiiiiiiiiiiiiiiiiiieeaaaaaaaaa",
  ".effeeiiiiiiiiiiiiiiiiiiiiggiiiiiiiiiiiiiiiiiiiiiiiiiieeaaaaaaaa",
  ".efffeeiiiiiiiiiiiiiiiiiiggiiiiiiiiiiiiiiiiiiiiiiiiiiiieeaaaaaaa",
  ".effffeei..iiiiiiiiiiiiiggiiiiiiiiiiiiiiiiiiiiii.iiiiiiieeaaaaaa",
  ".efffffeei..i.iiiiiiiiiggiiiiiiiiiiiiiiiiiiiiiiii...iiiiieeaaaaa",
  ".effffffeei..iiiiiiiiiggiiiiiiiiiiiiiiiiiiiiiiii...iiiiiiieeaaaa",
  ".effffffee.i..iiiiiiiggiii##iiiiiiiiiiiiii.iii...ii.iiiiiiieeaaa",
  ".efffffeeiiii..i.iiiggiii#cc#iiiiiiiiiiiiii....iiiiiiiiiiiiieeaa",
  ".effffeeiiiiii..iiiggiiii#cc#iiiiiiiiiiiii...iiiiiiiiiiiiiiiieea",
  ".efffeeiiiiii.i..iggiiiiii##iiiiiiii.iii...ii.iiiiiiiiiiiiiiiiee",
  ".effeeiiiiiiiiii..g.iiiiiiiiiiiiiiiif....fffffffffffffffffffiiie",
  ".efeeiiiiiiiiiiig..iiiiiiiiiiiiiiiii...fffffffffffffffffffffiiii",
  ".eeeiiiiiiiiiiig.i..iiiiiiiiii.iii...ff.ffffffffffffffffffffiiii",
  ".eeiiiiiiiiiiiggiii..i.iiiiiiii....iffffffffffffffffffffffffiiii",
  ".eiiiiiiiiiiiggiiiii..iiiiiiii...iiiffffffffffffffffffffffffiiii",
  ".eeiiiiiiiiiggiiiii.i..iiiii...ii.iiffffffffffffffffffffffffiiii",
  "..eeiiiiiiiggiiiiiiiii..ii...iiiiiiiffffffddddddddddddddddddddii",
  "a..eeiiiiiggiiiiiiiiiii....iiiiiiiiiffffffdddddddddddddddddddddi",
  "aa..eeiiiigiiiiiiiiiii....iiiiiiiiiiffffffddhhhhhhhhhhhhhhhhhhhh",
  "aaa..eeiiiiiiiii.iii...ii..i.iiiiiiiffffffddhhhhhhhhhhhhhhhhhhhh",
  "aaaa..eeiiiiiiiii....iiiii..iiiiiiiiffffffddhhbbbbbbbbbbbbbbbbhh",
  "aaaaa..eeiiiiiii...iiiiii.i..iiiiiiiffffffddhhbbcbbcbbcbbcbbcbhh",
  "aaaaaa..eeiiiii..ii.iiiiiiii..i.iiiiffffffddhhbbbbbbbbbbbbbbbbhh",
  "aaaaaaa..eeiiiiiiiiiiiiiiiiii..iiiiiffffffddhhbbcbbcbbcbbcbbcbhh",
  "aaaaaaaa..eeiiiiiiiiiiiiiiii.i..iiiiffffffddhhbbbbbbbbbbbbbbbbhh",
  "aaaaaaaaa..eeiiiiiiiiiiiiiiiiii.iiiiffffffddhhhhhhhhhhhhhhhhhhhh",
  "aaaaaaaaaa..eeiiiiiiiiiiiiiiiiiiiiiiffffffddhhhhhhhhhhhhhhhhhhhh",
  "aaaaaaaaaaa..eeiiiiiiiiiiiiiiiiiiiiiffffffddhhh##hhhh##hhhh##hhh",
  "aaaaaaaaaaaa..eeiiiiiiiiiiiiiiiiiiiiffffffddhh.###hh.###hh.###hh",
  "aaaaaaaaaaaaa..eeiiiiiiiiiiiiiiiiiiiffffeeddhh..##hh..##hh..##hh",
  "aaaaaaaaaaaaaa..eeiiiiiiiiiiiiiiiiiiffeee.ddhhh..hhhh..hhhh..hhh",
  "ccccaaaaaaaacccc.eeiiiiiiiiiiiiiiiiieee...ddhhhhhhhhhhhhhhhhhhhh",
  "cccccaaaaaaacccc..eeiiiiiiiiiiiiiieee.....ddhhhhhhhhhhhhhhhhhhhh",
  "ccccccaaaaaacccca..eeiiiiiiiiiiieee.......ddhhh##hhhh##hhhh##hhh",
  "cccccccaaaaaccccaa..eeiiiiiiiieee.........ddhh.###hh.###hh.###hh",
  "ccccccccaaaaccccaaa..eeiiiiieee....a......ddhh..##hh..##hh..##hh",
  "cccccccccaaaccccaaaa..eeiieee....aaa..c...ddhhh..hhhh..hhhh..hhh",
  "ccccacccccaaccccaaaaa..eeee....aaaaa..c...ddhhhhhhhhhhhhhhhhhhhh",
  "ccccaacccccaccccaaccca.cec.cccaacccc..ccccddhhhhhhhhhhhhhhhhhhhh",
  "ccccaaacccccccccacaaacacc..aaacacaaac.c...cdhhh##hhhh##hhhh##hhh",
  "ccccaaaaccccccccacaaacac.aaccccacaaac.c...cdhh.###hh.###hh.###hh",
  "ccccaaaaacccccccacaaacacaacaaacacaaacacaaacdhh..##hh..##hh..##hh",
  "ccccaaaaaaccccccaaccccacaacaaacaccccaacaaacdhhh..hhhh..hhhh..hhh",
  "ccccaaaaaaacccccacaaacacaaaccccacaaaaacaaacdhhhhhhhhhhhhhhhhhhhh",
  "ccccaaaaaaaaccccaacccaaaaaaaaaaacaaaaaaaaaaahhhhhhhhhhhhhhhhhhhh"
};


/* XPM */
const gchar * Arc_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c None",
  "                    ",
  "                    ",
  "        .....       ",
  "      .........     ",
  "     ...+++++...    ",
  "    ..+++++++++..   ",
  "   ..+++++++++++..  ",
  "   ..+++++++++++..  ",
  "  ..+++++++++++++.. ",
  "  ..+++++++++++++.. ",
  "  ..+++++++++++++.. ",
  "  ..+++++++++++++.. ",
  "  ..+++++++++++++.. ",
  "   ..+++++++++++..  ",
  "   ..+++++++++++..  ",
  "    ..+++++++++..   ",
  "     ...+++++...    ",
  "      .........     ",
  "        .....       ",
  "                    "};
/* XPM */
const gchar * Axispoint_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "                    ",
  "   .                ",
  "  ...               ",
  " .....              ",
  "   .                ",
  "   .                ",
  "  ... ....          ",
  "   .  .++....       ",
  "   .  .+++++.....   ",
  "  ... ..++++++++..  ",
  "   .   .++++++...   ",
  "   .   .+++++..     ",
  "  ...  ..+++++..    ",
  "   .    .++.+++..   ",
  "   .    .+...+++..  ",
  " .....  .+. ..+++.. ",
  "   .    ...  ..+++..",
  "   .     .    ..+.. ",
  "  ...          ...  ",
  "   .            .   "};
/* XPM */
const gchar * Axiswin_xpm[] = {
  "20 20 4 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "*	c #666666",
  "................... ",
  ".+++++++++++++++++.*",
  ".++++++.++++++++++.*",
  ".+++++...+++++++++.*",
  ".++++.....++++++++.*",
  ".++++++.++++++++++.*",
  ".++++++.++++++++++.*",
  ".+++++...+++++++++.*",
  ".++++++.++++++++++.*",
  ".++++++.++++++++++.*",
  ".+++++...+++++++++.*",
  ".++++++.+++++.++++.*",
  ".++++++.++.++..+++.*",
  ".+++............++.*",
  ".++++++.++.++..+++.*",
  ".++++++.+++++.++++.*",
  ".++++++.++++++++++.*",
  ".+++++++++++++++++.*",
  "...................*",
  " *******************"};
/* XPM */
const gchar * Clear_xpm[] = {
  "20 20 4 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "*	c #0099FF",
  "                    ",
  "          .....     ",
  "         .*.+++.    ",
  "        .*.*.+++.   ",
  "       .*.*.*.+++.  ",
  "      .*.*.*.*.+++. ",
  "     .*.*.*.*.*.+++.",
  "    .*.*.*.*.*.*....",
  "   .*.*.*.*.*.*.....",
  "  .*.*.*.*.*.*..... ",
  "  ..*.*.*.*.*.....  ",
  " .++.*.*.*.*.....   ",
  " .+++.*.*.*.....    ",
  " .++++.*.*.....     ",
  " .+++++.*.....      ",
  "  .+++++.....       ",
  "   .+++++++.        ",
  "    .+.+++.         ",
  "     .....          ",
  "                    "};
/* XPM */
const gchar * Coordwin_xpm[] = {
  "20 20 4 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "*	c #666666",
  "................... ",
  ".+++++++++++++++++.*",
  ".+++++++++++++++++.*",
  ".+++..++++++++++++.*",
  ".++....+++++++++++.*",
  ".++....+++++++++++.*",
  ".+++..++..++++++++.*",
  ".+++++++++..++++++.*",
  ".+++++++++++..++++.*",
  ".+++++++++++++..++.*",
  ".+++++++++++++++++.*",
  ".+++++++++++++++++.*",
  ".+++.+++.++.+++.++.*",
  ".++++.+.++++.+.+++.*",
  ".+++++.++++++.++++.*",
  ".++++.+.+++++.++++.*",
  ".+++.+++.++++.++++.*",
  ".+++++++++++++++++.*",
  "...................*",
  " *******************"};
/* XPM */
const gchar * Cross_xpm[] = {
  "20 20 2 1",
  " 	c None",
  ".	c #000000",
  "                    ",
  "                    ",
  "         .          ",
  "        ...         ",
  "       .....        ",
  "         .          ",
  "         .          ",
  "        ...         ",
  "         .          ",
  "         .          ",
  "        ...         ",
  "         .          ",
  "         .      .   ",
  "     .   .   .  ..  ",
  "  ................. ",
  "     .   .   .  ..  ",
  "     .   .      .   ",
  "         .          ",
  "                    ",
  "                    "};
/* XPM */
const gchar * Curve_xpm[] = {
  "20 20 2 1",
  " 	c None",
  ".	c #000000",
  "                    ",
  "                    ",
  "    .....           ",
  "   ......           ",
  "  ...               ",
  "  ..                ",
  "  ..                ",
  "  ..                ",
  "  ..       .....    ",
  "  ...     .......   ",
  "   .........   ...  ",
  "    .......     ..  ",
  "                ..  ",
  "                ..  ",
  "                ..  ",
  "               ...  ",
  "           ......   ",
  "           .....    ",
  "                    ",
  "                    "};
/* XPM */
const gchar * Datapoint_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "                    ",
  "                    ",
  "   ..               ",
  "  ....              ",
  "  ....              ",
  "   ..               ",
  "      ....          ",
  "      .++....       ",
  "      .+++++.....   ",
  "      ..++++++++..  ",
  "       .++++++...   ",
  "       .+++++..     ",
  "       ..+++++..    ",
  "        .++.+++..   ",
  "        .+...+++..  ",
  "        .+. ..+++.. ",
  "        ...  ..+++..",
  "         .    ..+.. ",
  "               ...  ",
  "                .   "};
/* XPM */
const gchar * Draw_xpm[] = {
  "20 20 4 1",
  " 	c None",
  ".	c #000000",
  "+	c #00CCCC",
  "*	c #CC9900",
  "                    ",
  "           .        ",
  "          .+.       ",
  "         .+++.      ",
  "        .+++.+.     ",
  "       .+++.+.+.    ",
  "      .+++.+.+...   ",
  "     .+++.+.+.....  ",
  "    .+++.+.+.....   ",
  "   .+++.+.+.....    ",
  "  .+++.+.+.....     ",
  " .*.+.+.+.....      ",
  " .**.+.+.....       ",
  " .***.+.....        ",
  " .****.....         ",
  " .*****...          ",
  " ..*****.           ",
  " .......            ",
  "                    ",
  "                    "};
/* XPM */
const gchar * Eval_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "                    ",
  " ........           ",
  " .++++++.           ",
  " ...++++.           ",
  " .++++++.           ",
  " ...++++.           ",
  " .++++++.           ",
  " ...++++.           ",
  " .++++++.  .......  ",
  " ....+++. ......... ",
  " .++++++. ..     .. ",
  " ...++++. ..     .. ",
  " .++++++.     ..... ",
  " ...++++.    .....  ",
  " .++++++.    ...    ",
  " ...++++.           ",
  " .++++++.    ...    ",
  " ........    ...    ",
  "                    ",
  "                    "};
/* XPM */
const gchar * Fileopen_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "                    ",
  "  ..                ",
  "  ......            ",
  "   .+++.            ",
  "   .+.......        ",
  "   .+.+++++.        ",
  "   .+.+.........    ",
  "   ...+.+++++++.    ",
  "     .+.+++++.......",
  "     .+.++++..+++++.",
  "     .+.+++...+.+.+.",
  "     ...++....+.+.+.",
  "       .+.....+++++.",
  "       .+.+.+.+.+.+.",
  "       .+.+.+.+.+.+.",
  "       ...+++++++++.",
  "         .+.+.+.+.+.",
  "         .+.+.+.+.+.",
  "         .+++++++++.",
  "         ..........."};
/* XPM */
const gchar * Filewin_xpm[] = {
  "20 20 4 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "*	c #666666",
  "................... ",
  ".+++++++++++++++++.*",
  ".++++.++++++++++++.*",
  ".++++.++++++++++++.*",
  ".++++.+++.+.+.+.++.*",
  ".++++.+++.+.+.+.++.*",
  ".+++++++++++++++++.*",
  ".+++..++++++++++++.*",
  ".+++++.+++++++++++.*",
  ".++++.+++.+.+.+.++.*",
  ".+++...++.+.+.+.++.*",
  ".+++++++++++++++++.*",
  ".+++..++++++++++++.*",
  ".+++++.+++++++++++.*",
  ".++++..++.+.+.+.++.*",
  ".+++++.++.+.+.+.++.*",
  ".+++..++++++++++++.*",
  ".+++++++++++++++++.*",
  "...................*",
  " *******************"};
/* XPM */
const gchar * Frame_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c None",
  "                    ",
  "                    ",
  "                    ",
  "   ...............  ",
  "   .+++.++.++.+++.  ",
  "   .++++++.++++++.  ",
  "   .+++++++++++++.  ",
  "   ..+++++++++++..  ",
  "   .+++++++++++++.  ",
  "   .+++++++++++++.  ",
  "   ...+++++++++...  ",
  "   .+++++++++++++.  ",
  "   .+++++++++++++.  ",
  "   ..+++++++++++..  ",
  "   .+++++++++++++.  ",
  "   .++++++.++++++.  ",
  "   .+++.++.++.+++.  ",
  "   ...............  ",
  "                    ",
  "                    "};
/* XPM */
const gchar * Gauss_xpm[] = {
  "20 20 2 1",
  " 	c None",
  ".	c #000000",
  "                    ",
  "                    ",
  "                    ",
  "                    ",
  "                    ",
  "         ...        ",
  "        .   .       ",
  "       ..   ..      ",
  "       ..   ..      ",
  "       ..   ..      ",
  "      ..     ..     ",
  "      ..     ..     ",
  "      ..     ..     ",
  "      ..     ..     ",
  "     ..      ...    ",
  "     ..       ..    ",
  "  .....       ..... ",
  "  ....         .... ",
  "                    ",
  "                    "};
/* XPM */
const gchar * Infowin_xpm[] = {
  "20 20 4 1",
  " 	c #FFFFFF",
  ".	c #0000CC",
  "+	c #FFFFFF",
  "*	c #000000",
  "********************",
  "*                  *",
  "*      ......      *",
  "*    ..........    *",
  "*   ....++++....   *",
  "*  .....++++.....  *",
  "*  ..............  *",
  "* .....+++++...... *",
  "* ......++++...... *",
  "* ......++++...... *",
  "* ......++++...... *",
  "* ......++++...... *",
  "* ......++++...... *",
  "*  .....++++.....  *",
  "*  ....++++++....  *",
  "*   ............   *",
  "*    ..........    *",
  "*      ......      *",
  "*                  *",
  "********************"};
/* XPM */
const gchar * Interrupt_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #CC0000",
  "+	c #FFFFFF",
  "                    ",
  "       ......       ",
  "     ..........     ",
  "    ............    ",
  "   ..............   ",
  "  ................  ",
  "  ................  ",
  " ..+++++++.++..++.. ",
  " .+....+..+..+.+.+. ",
  " ..++..+..+..+.+.+. ",
  " ....+.+..+..+.++.. ",
  " .+++..+...++..+... ",
  " .................. ",
  "  ................  ",
  "  ................  ",
  "   ..............   ",
  "    ............    ",
  "     ..........     ",
  "       ......       ",
  "                    "};
/* XPM */
const gchar * Legendpoint_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "  ..  ...   ..      ",
  " .  . .  . .  .     ",
  " .  . ...  .        ",
  " .... .  . .  .     ",
  " .  . ...   ..      ",
  "                    ",
  "      ....          ",
  "      .++....       ",
  "      .+++++.....   ",
  "      ..++++++++..  ",
  "       .++++++...   ",
  "       .+++++..     ",
  "       ..+++++..    ",
  "        .++.+++..   ",
  "        .+...+++..  ",
  "        .+. ..+++.. ",
  "        ...  ..+++..",
  "         .    ..+.. ",
  "               ...  ",
  "                .   "};
/* XPM */
const gchar * Legendwin_xpm[] = {
  "20 20 4 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "*	c #666666",
  "................... ",
  ".+++++++++++++++++.*",
  ".+++.+.+++++++++++.*",
  ".+++.+.+..++++++++.*",
  ".+++.+.+++.+++++++.*",
  ".++++++++++.++++++.*",
  ".+++++++++++.+++++.*",
  ".++++++++++++.+.++.*",
  ".++++.++++++++..++.*",
  ".+++...++++++...++.*",
  ".++++.++++++++++++.*",
  ".+++++.+++++++++++.*",
  ".+++++.+++++++++++.*",
  ".++++++.++++++++++.*",
  ".++++++.++++.+.+++.*",
  ".+++++++...+.+.+++.*",
  ".+++++++++++.+.+++.*",
  ".+++++++++++++++++.*",
  "...................*",
  " *******************"};
/* XPM */
const gchar * Line_xpm[] = {
  "20 20 2 1",
  " 	c None",
  ".	c #000000",
  "                    ",
  "                    ",
  "               .    ",
  "              ...   ",
  "             ...    ",
  "            ...     ",
  "           ...      ",
  "          ...       ",
  "         ...        ",
  "        ...         ",
  "       ...          ",
  "      ...           ",
  "     ...            ",
  "    ...             ",
  "   ...              ",
  "  ...............   ",
  "  ...............   ",
  "                    ",
  "                    ",
  "                    "};
/* XPM */
const gchar * Load_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "...........         ",
  ".+++++++++.         ",
  ".+..++++++.         ",
  ".+++++++++.         ",
  ".+++++++++.         ",
  "...........         ",
  "                    ",
  "                    ",
  " ...         .......",
  " ...        ..+++++.",
  " ... ..    ...+.+.+.",
  " ... ..   ....+.+.+.",
  " ....... .....+++++.",
  " .........+.+.+.+.+.",
  "  ...... .+.+.+.+.+.",
  "     ..  .+++++++++.",
  "     .   .+.+.+.+.+.",
  "         .+.+.+.+.+.",
  "         .+++++++++.",
  "         ..........."};
/* XPM */
const gchar * Mark_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "                    ",
  "                    ",
  "        .....       ",
  "      .........     ",
  "     ...++......    ",
  "    ..++++.......   ",
  "   ..+++++........  ",
  "   ..+++++........  ",
  "  ..++++++......... ",
  "  ..++++++......... ",
  "  ..++++++......... ",
  "  ..++++++......... ",
  "  ..++++++......... ",
  "   ..+++++........  ",
  "   ..+++++........  ",
  "    ..++++.......   ",
  "     ...++......    ",
  "      .........     ",
  "        .....       ",
  "                    "};
/* XPM */
const gchar * Math_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "                    ",
  "  ...............   ",
  "  .+++++++++++++..  ",
  "  .+...........+..  ",
  "  .+.+.+.+.+.+.+..  ",
  "  .+.+.+.+.+.+.+..  ",
  "  .+...........+..  ",
  "  .+++++++++++++..  ",
  "  .+..+..+..+..+..  ",
  "  .+..+..+..+..+..  ",
  "  .+++++++++++++..  ",
  "  .+..+..+..+..+..  ",
  "  .+..+..+..+..+..  ",
  "  .+++++++++++++..  ",
  "  .+..+..+..+..+..  ",
  "  .+..+..+..+..+..  ",
  "  .+++++++++++++..  ",
  "  ................  ",
  "   ...............  ",
  "                    "};
/* XPM */
const gchar * Mergewin_xpm[] = {
  "20 20 4 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "*	c #666666",
  "................... ",
  ".+++++++++++++++++.*",
  ".+...............+.*",
  ".+..++.++.++.++..+.*",
  ".+..+++++++++++..+.*",
  ".+...+++++++++...+.*",
  ".+..+++++++++++..+.*",
  ".+..++.++.++.++..+.*",
  ".+...............+.*",
  ".+++++++++++++++++.*",
  ".+...............+.*",
  ".+..++.++.++.++..+.*",
  ".+..+++++++++++..+.*",
  ".+...+++++++++...+.*",
  ".+..+++++++++++..+.*",
  ".+..++.++.++.++..+.*",
  ".+...............+.*",
  ".+++++++++++++++++.*",
  "...................*",
  " *******************"};
/* XPM */
const gchar * Point_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "                    ",
  "                    ",
  "  ...               ",
  "  .+....            ",
  "  ..+++....         ",
  "   .++++++....      ",
  "   .+++++++++..     ",
  "   ..++++++...      ",
  "    .++++++.        ",
  "    .++++++..       ",
  "    ..++++++..      ",
  "     .+...+++..     ",
  "     .+. ..+++..    ",
  "     ...  ..+++..   ",
  "      .    ..+++..  ",
  "            ..+++.. ",
  "             ..+..  ",
  "              ...   ",
  "               .    ",
  "                    "};
/* XPM */
const gchar * Polygon_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c None",
  "                    ",
  "                    ",
  "          .         ",
  "         ...        ",
  "        ..+..       ",
  "       ..+++..      ",
  "      ..+++++..     ",
  "     ..+++++++..    ",
  "    ..+++++++++..   ",
  "   ..+++++++++++..  ",
  "  ..+++++++++++++.. ",
  "   ..+++++++++++..  ",
  "    ..+++++++++..   ",
  "     ..+++++++..    ",
  "      ..+++++..     ",
  "       ..+++..      ",
  "        ..+..       ",
  "         ...        ",
  "          .         ",
  "                    "};
/* XPM */
const gchar * Preview_xpm[] = {
  "20 20 4 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "*	c #0000FF",
  "                    ",
  "  ................  ",
  "  .**************.  ",
  "  .********+*+*+*.  ",
  "  .**************.  ",
  "  .++++++++++++++.  ",
  "  .++++++++++++++.  ",
  "  .+++........+++.  ",
  "  .+++.++++++.+++.  ",
  "  .+++.++++++.+++.  ",
  "  .+++.++++++.+++.  ",
  "  .+++.++++++.+++.  ",
  "  .+++.++++++.+++.  ",
  "  .+++.++++++.+++.  ",
  "  .+++.++++++.+++.  ",
  "  .+++.++++++.+++.  ",
  "  .+++........+++.  ",
  "  .++++++++++++++.  ",
  "  ................  ",
  "                    "};
/* XPM */
const gchar * Print_xpm[] = {
  "20 20 6 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "*	c #CCCCCC",
  "#	c #999999",
  "R	c #FF9999",
  "                    ",
  "                    ",
  "        ............",
  "       .++++++++++. ",
  "      .++++++++++.  ",
  "     .++++++++++.   ",
  "     .++++++++++.   ",
  "    .++++++++++.    ",
  "    .++++++++++.....",
  "    .++++++++++...#.",
  "   ..++++++++++..##.",
  "  ..............###.",
  " ..............####.",
  " .************.####.",
  " .*RRR********.####.",
  " .************.###. ",
  " .*RRR********.##.  ",
  " .************.#.   ",
  " ...............    ",
  "                    "};
/* XPM */
const gchar * Rect_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "                    ",
  "                    ",
  "                    ",
  "    .............   ",
  "    .............   ",
  "    ..+++++++++..   ",
  "    ..+++++++++..   ",
  "    ..+++++++++..   ",
  "    ..+++++++++..   ",
  "    ..+++++++++..   ",
  "    ..+++++++++..   ",
  "    ..+++++++++..   ",
  "    ..+++++++++..   ",
  "    ..+++++++++..   ",
  "    ..+++++++++..   ",
  "    ..+++++++++..   ",
  "    .............   ",
  "    .............   ",
  "                    ",
  "                    "};
/* XPM */
const gchar * Save_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "...........         ",
  ".+++++++++.         ",
  ".+..++++++.         ",
  ".+++++++++.         ",
  ".+++++++++.         ",
  "...........         ",
  "                    ",
  "   .                ",
  "  ...        .......",
  " .....      ..+++++.",
  ".......    ...+.+.+.",
  "  ...     ....+.+.+.",
  "  ...    .....+++++.",
  "  ...    .+.+.+.+.+.",
  "  .....  .+.+.+.+.+.",
  "  .....  .+++++++++.",
  "   ....  .+.+.+.+.+.",
  "         .+.+.+.+.+.",
  "         .+++++++++.",
  "         ..........."};
/* XPM */
const gchar * Scale_xpm[] = {
  "20 20 2 1",
  " 	c None",
  ".	c #000000",
  "                    ",
  "    .               ",
  "   ...              ",
  "  .....  .......    ",
  "    .   .........   ",
  "    .   ..     ..   ",
  "    .   ..     ..   ",
  "    .       .....   ",
  "    .      .....    ",
  "    .      ...      ",
  "    .               ",
  "    .      ...      ",
  "    .      ...      ",
  "    .               ",
  "    .           .   ",
  "    .           ..  ",
  "  ................. ",
  "    .           ..  ",
  "    .           .   ",
  "                    "};
/* XPM */
const gchar * Scaleundo_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "=	c #0000CC",
  "                    ",
  ".  . .   . ...   .. ",
  ".  . ..  . .  . .  .",
  ".  . . . . .  . .  .",
  ".  . .  .. .  . .  .",
  " ..  .   . ...   .. ",
  "                    ",
  "       =            ",
  "      ==            ",
  "     ===            ",
  "    =============== ",
  "   ================ ",
  "  ================= ",
  " ================== ",
  "  ================= ",
  "   ================ ",
  "    =============== ",
  "     ===            ",
  "      ==            ",
  "       =            "};
/* XPM */
const gchar * Section_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "                    ",
  "                    ",
  "  ................  ",
  "  .++.++.++.++.++.  ",
  "  .++.++.++.++.++.  ",
  "  ................  ",
  "  .++.++.++.++.++.  ",
  "  .++.++.++.++.++.  ",
  "  ................  ",
  "  .++.++.++.++.++.  ",
  "  .++.++.++.++.++.  ",
  "  ................  ",
  "  .++.++.++.++.++.  ",
  "  .++.++.++.++.++.  ",
  "  ................  ",
  "  .++.++.++.++.++.  ",
  "  .++.++.++.++.++.  ",
  "  ................  ",
  "                    ",
  "                    "};
/* XPM */
const gchar * Single_xpm[] = {
  "20 20 2 1",
  " 	c None",
  ".	c #000000",
  "                    ",
  "                    ",
  "         .          ",
  "        ...         ",
  "       .....        ",
  "         .          ",
  "         .          ",
  "        ...         ",
  "         .          ",
  "         .          ",
  "        ...         ",
  "         .          ",
  "         .          ",
  "        ...         ",
  "         .          ",
  "         .          ",
  "        ...         ",
  "         .          ",
  "         .          ",
  "                    "};
/* XPM */
const gchar * Text_xpm[] = {
  "20 20 2 1",
  " 	c None",
  ".	c #000000",
  "                    ",
  "                    ",
  "                    ",
  "                    ",
  "   ...............  ",
  "   ...   ...   ...  ",
  "   ..    ...    ..  ",
  "   ..    ...    ..  ",
  "         ...        ",
  "         ...        ",
  "         ...        ",
  "         ...        ",
  "         ...        ",
  "         ...        ",
  "         ...        ",
  "         ...        ",
  "      .........     ",
  "                    ",
  "                    ",
  "                    "};
/* XPM */
const gchar * Trimming_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "                    ",
  "    .               ",
  "   ....  .          ",
  "  ..... .+.         ",
  "    .   .++.        ",
  "    .    .++.       ",
  "   ...   .+++.      ",
  "    .     .+++.     ",
  "   ..     .+++.     ",
  "  .++..    .++.     ",
  "   .+++..   .+.     ",
  "    .++++.  ..  ..  ",
  "   ...++++.......+. ",
  "    . .++++....++++.",
  "    .  .... ......+.",
  "   ...      .+.  ...",
  "    .      ..+.     ",
  "    .      .++..    ",
  "  .....     .++.    ",
  "    .        ...    "};
/* XPM */
const gchar * Zoom_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "                    ",
  "         ......     ",
  "       ..........   ",
  "      ...++++++...  ",
  "      ..++++++++..  ",
  "     ..++++..++++.. ",
  "     ..++++..++++.. ",
  "     ..++......++.. ",
  "     ..++......++.. ",
  "     ..++++..++++.. ",
  "     ..++++..++++.. ",
  "     ...++++++++..  ",
  "    .....+++++....  ",
  "   ..............   ",
  "  ....+........     ",
  "  ...+..            ",
  "  ..+..             ",
  "   ...              ",
  "                    ",
  "                    "};

/* XPM */
const gchar * Move_xpm[] = {
"20 20 2 1",
" 	c None",
".	c #000000",
"                    ",
"                    ",
"                    ",
"         ..         ",
"        ....        ",
"       ......       ",
"         ..         ",
"     .   ..   .     ",
"    ..   ..   ..    ",
"   ..............   ",
"   ..............   ",
"    ..   ..   ..    ",
"     .   ..   .     ",
"         ..         ",
"       ......       ",
"        ....        ",
"         ..         ",
"                    ",
"                    ",
"                    "};

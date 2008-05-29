/* 
 * $Id: x11bitmp.c,v 1.1.1.1 2008/05/29 09:37:33 hito Exp $
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

gchar *Icon_xpm[] = {
  "48 48 10 1",
  ". c #000000",
  "# c #0000ff",
  "a c #008080",
  "b c #00ffff",
  "c c #808000",
  "d c #808080",
  "e c #c0c0c0",
  "f c #ff0000",
  "g c #ffff00",
  "h c #ffffff",
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
  "aaa..dhhhhhhhh..hhh..hhhhhhheeecgaaaaaaaaaaaaaga",
  "aaaa..dhhhhh..hhhhhh..h.hhhheeecgaabaabaabaabaga",
  "aaaaa..dhhhhhhhhhhhhh..hhhhheeecgaabaabaabaabaga",
  "aaaaaa..dhhhhhhhhhhh.h..hhhheeecgaaaaaaaaaaaaaga",
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

gchar *Icon_xpm_64[] = { 
  "64 64 10 1",
  ". c #000000",
  "# c #0000ff",
  "a c #008080",
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
  "aaaa..eeiiiiiiiii....iiiii..iiiiiiiiffffffddhhaaaaaaaaaaaaaaaahh",
  "aaaaa..eeiiiiiii...iiiiii.i..iiiiiiiffffffddhhaacaacaacaacaacahh",
  "aaaaaa..eeiiiii..ii.iiiiiiii..i.iiiiffffffddhhaaaaaaaaaaaaaaaahh",
  "aaaaaaa..eeiiiiiiiiiiiiiiiiii..iiiiiffffffddhhaacaacaacaacaacahh",
  "aaaaaaaa..eeiiiiiiiiiiiiiiii.i..iiiiffffffddhhaaaaaaaaaaaaaaaahh",
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

#define BUTTON_WIDTH 20
#define BUTTON_HEIGHT 20


gchar Viewwin_bits[] = {
  0x00, 0x00, 0xf0, 0xfe, 0xff, 0xf7, 0xfe, 0xff, 0xf7, 0xc6, 0xff, 0xf7,
  0xc6, 0xff, 0xf7, 0xfe, 0xff, 0xf7, 0x02, 0x00, 0xf4, 0x02, 0x00, 0xf4,
  0x42, 0xc2, 0xf4, 0x42, 0x30, 0xf4, 0x42, 0x0c, 0xf4, 0x42, 0x83, 0xf4,
  0xc2, 0x08, 0xf4, 0x72, 0x00, 0xf4, 0x42, 0x00, 0xf4, 0xf2, 0xff, 0xf4,
  0x42, 0x00, 0xf4, 0x02, 0x00, 0xf4, 0xfe, 0xff, 0xf7, 0x00, 0x00, 0xf0};

gchar Filewin_bits[] = {
  0xfe, 0xff, 0xf3, 0x02, 0x00, 0xf6, 0x42, 0x00, 0xf6, 0x42, 0x00, 0xf6,
  0x42, 0x54, 0xf6, 0x42, 0x54, 0xf6, 0x02, 0x00, 0xf6, 0x62, 0x00, 0xf6,
  0x82, 0x00, 0xf6, 0x42, 0x54, 0xf6, 0xe2, 0x54, 0xf6, 0x02, 0x00, 0xf6,
  0x62, 0x00, 0xf6, 0x82, 0x00, 0xf6, 0xc2, 0x54, 0xf6, 0x82, 0x54, 0xf6,
  0x62, 0x00, 0xf6, 0x02, 0x00, 0xf6, 0xfe, 0xff, 0xf7, 0xfc, 0xff, 0xf7
};

gchar Axiswin_bits[] = {
  0xfe, 0xff, 0xf3, 0x02, 0x00, 0xf6, 0x82, 0x00, 0xf6, 0xc2, 0x01, 0xf6,
  0xe2, 0x03, 0xf6, 0x82, 0x00, 0xf6, 0x82, 0x00, 0xf6, 0xc2, 0x01, 0xf6,
  0x82, 0x00, 0xf6, 0x82, 0x00, 0xf6, 0xc2, 0x01, 0xf6, 0x82, 0x20, 0xf6,
  0x82, 0x64, 0xf6, 0xf2, 0xff, 0xf6, 0x82, 0x64, 0xf6, 0x82, 0x20, 0xf6,
  0x82, 0x00, 0xf6, 0x02, 0x00, 0xf6, 0xfe, 0xff, 0xf7, 0xfc, 0xff, 0xf7
};

gchar Legendwin_bits[] = {
  0xfe, 0xff, 0xf3, 0x02, 0x00, 0xf6, 0x52, 0x00, 0xf6, 0x52, 0x03, 0xf6,
  0x52, 0x04, 0xf6, 0x02, 0x08, 0xf6, 0x02, 0x10, 0xf6, 0x02, 0xa0, 0xf6,
  0x22, 0xc0, 0xf6, 0x72, 0xe0, 0xf6, 0x22, 0x00, 0xf6, 0x42, 0x00, 0xf6,
  0x42, 0x00, 0xf6, 0x82, 0x00, 0xf6, 0x82, 0x50, 0xf6, 0x02, 0x57, 0xf6,
  0x02, 0x50, 0xf6, 0x02, 0x00, 0xf6, 0xfe, 0xff, 0xf7, 0xfc, 0xff, 0xf7
};

gchar Mergewin_bits[] = {
  0xfe, 0xff, 0xf3, 0x02, 0x00, 0xf6, 0xfa, 0xff, 0xf6, 0x4a, 0x92, 0xf6,
  0x0a, 0x80, 0xf6, 0x1a, 0xc0, 0xf6, 0x0a, 0x80, 0xf6, 0x4a, 0x92, 0xf6,
  0xfa, 0xff, 0xf6, 0x02, 0x00, 0xf6, 0xfa, 0xff, 0xf6, 0x4a, 0x92, 0xf6,
  0x0a, 0x80, 0xf6, 0x1a, 0xc0, 0xf6, 0x0a, 0x80, 0xf6, 0x4a, 0x92, 0xf6,
  0xfa, 0xff, 0xf6, 0x02, 0x00, 0xf6, 0xfe, 0xff, 0xf7, 0xfc, 0xff, 0xf7
};

gchar Coordwin_bits[] = {
  0xfe, 0xff, 0xf3, 0x02, 0x00, 0xf6, 0x02, 0x00, 0xf6, 0x32, 0x00, 0xf6,
  0x7a, 0x00, 0xf6, 0x7a, 0x00, 0xf6, 0x32, 0x03, 0xf6, 0x02, 0x0c, 0xf6,
  0x02, 0x30, 0xf6, 0x02, 0xc0, 0xf6, 0x02, 0x00, 0xf6, 0x02, 0x00, 0xf6,
  0x12, 0x89, 0xf6, 0xa2, 0x50, 0xf6, 0x42, 0x20, 0xf6, 0xa2, 0x20, 0xf6,
  0x12, 0x21, 0xf6, 0x02, 0x00, 0xf6, 0xfe, 0xff, 0xf7, 0xfc, 0xff, 0xf7
};

gchar Infowin_bits[] = {
  0xff, 0xff, 0xff, 0x01, 0x00, 0xf8, 0x81, 0x1f, 0xf8, 0xe1, 0x7f, 0xf8,
  0xf1, 0xf0, 0xf8, 0xf9, 0xf0, 0xf9, 0xf9, 0xff, 0xf9, 0x7d, 0xf0, 0xfb,
  0xfd, 0xf0, 0xfb, 0xfd, 0xf0, 0xfb, 0xfd, 0xf0, 0xfb, 0xfd, 0xf0, 0xfb,
  0xfd, 0xf0, 0xfb, 0xf9, 0xf0, 0xf9, 0x79, 0xe0, 0xf9, 0xf1, 0xff, 0xf8,
  0xe1, 0x7f, 0xf8, 0x81, 0x1f, 0xf8, 0x01, 0x00, 0xf8, 0xff, 0xff, 0xff
};

gchar Fileopen_bits[] = {
  0x00, 0x00, 0xf0, 0x0c, 0x00, 0xf0, 0xfc, 0x00, 0xf0, 0x88, 0x00, 0xf0,
  0xe8, 0x0f, 0xf0, 0x28, 0x08, 0xf0, 0xa8, 0xff, 0xf0, 0xb8, 0x80, 0xf0,
  0xa0, 0xe0, 0xff, 0xa0, 0x30, 0xf8, 0xa0, 0xb8, 0xfa, 0xe0, 0xbc, 0xfa,
  0x80, 0x3e, 0xf8, 0x80, 0xaa, 0xfa, 0x80, 0xaa, 0xfa, 0x80, 0x03, 0xf8,
  0x00, 0xaa, 0xfa, 0x00, 0xaa, 0xfa, 0x00, 0x02, 0xf8, 0x00, 0xfe, 0xff
};

gchar Load_bits[] = {
  0xff, 0x07, 0xf0, 0x01, 0x04, 0xf0, 0x0d, 0x04, 0xf0, 0x01, 0x04, 0xf0,
  0x01, 0x04, 0xf0, 0xff, 0x07, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0,
  0x0e, 0xe0, 0xff, 0x0e, 0x30, 0xf8, 0x6e, 0xb8, 0xfa, 0x6e, 0xbc, 0xfa,
  0xfe, 0x3e, 0xf8, 0xfe, 0xab, 0xfa, 0xfc, 0xaa, 0xfa, 0x60, 0x02, 0xf8,
  0x20, 0xaa, 0xfa, 0x00, 0xaa, 0xfa, 0x00, 0x02, 0xf8, 0x00, 0xfe, 0xff
};

gchar Save_bits[] = {
  0xff, 0x07, 0xf0, 0x01, 0x04, 0xf0, 0x0d, 0x04, 0xf0, 0x01, 0x04, 0xf0,
  0x01, 0x04, 0xf0, 0xff, 0x07, 0xf0, 0x00, 0x00, 0xf0, 0x08, 0x00, 0xf0,
  0x1c, 0xe0, 0xff, 0x3e, 0x30, 0xf8, 0x7f, 0xb8, 0xfa, 0x1c, 0xbc, 0xfa,
  0x1c, 0x3e, 0xf8, 0x1c, 0xaa, 0xfa, 0x7c, 0xaa, 0xfa, 0x7c, 0x02, 0xf8,
  0x78, 0xaa, 0xfa, 0x00, 0xaa, 0xfa, 0x00, 0x02, 0xf8, 0x00, 0xfe, 0xff
};

gchar Scale_bits[] = {
  0x00, 0x00, 0xf0, 0x10, 0x00, 0xf0, 0x38, 0x00, 0xf0, 0x7c, 0xfe, 0xf0,
  0x10, 0xff, 0xf1, 0x10, 0x83, 0xf1, 0x10, 0x83, 0xf1, 0x10, 0xf0, 0xf1,
  0x10, 0xf8, 0xf0, 0x10, 0x38, 0xf0, 0x10, 0x00, 0xf0, 0x10, 0x38, 0xf0,
  0x10, 0x38, 0xf0, 0x10, 0x00, 0xf0, 0x10, 0x00, 0xf1, 0x10, 0x00, 0xf3,
  0xfc, 0xff, 0xf7, 0x10, 0x00, 0xf3, 0x10, 0x00, 0xf1, 0x00, 0x00, 0xf0
};

gchar Draw_bits[] = {
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00,
  0x00, 0x14, 0x00, 0x00, 0x00, 0x00, 0x22, 0x00, 0x00, 0x00,
  0x00, 0x51, 0x00, 0x00, 0x00, 0x80, 0xa8, 0x00, 0x00, 0x00,
  0x40, 0xd4, 0x01, 0x00, 0x00, 0x20, 0xea, 0x03, 0x00, 0x00,
  0x10, 0xf5, 0x01, 0x00, 0x00, 0x88, 0xfa, 0x00, 0x00, 0x00,
  0x44, 0xfd, 0x07, 0x00, 0x00, 0xaa, 0xbe, 0x0c, 0x00, 0x00,
  0x52, 0x9f, 0x10, 0x00, 0x00, 0xa2, 0x8f, 0x50, 0x9d, 0x24,
  0xc2, 0x87, 0xd0, 0xa0, 0x2a, 0x82, 0x83, 0x50, 0xbc, 0x2a,
  0x06, 0x81, 0x48, 0x22, 0x11, 0xfe, 0xc0, 0x47, 0x3c, 0x11,
  0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
};

gchar Clear_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x7c, 0xf0, 0x00, 0x8a, 0xf0, 0x00, 0x15, 0xf1,
  0x80, 0x2a, 0xf2, 0x40, 0x55, 0xf4, 0xa0, 0xaa, 0xf8, 0x50, 0x55, 0xff,
  0xa8, 0xaa, 0xff, 0x54, 0xd5, 0xf7, 0xac, 0xea, 0xf3, 0x52, 0xf5, 0xf1,
  0xa2, 0xfa, 0xf0, 0x42, 0x7d, 0xf0, 0x82, 0x3e, 0xf0, 0x04, 0x1f, 0xf0,
  0x08, 0x08, 0xf0, 0x50, 0x04, 0xf0, 0xe0, 0x03, 0xf0, 0x00, 0x00, 0xf0
};

gchar Print_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0xff, 0xff, 0x80, 0x00, 0xf4,
  0x40, 0x00, 0xf2, 0x20, 0x00, 0xf1, 0x20, 0x00, 0xf1, 0x10, 0x80, 0xf0,
  0x10, 0x80, 0xff, 0x10, 0x80, 0xfb, 0x18, 0x80, 0xfd, 0xfc, 0xff, 0xfe,
  0xfe, 0x7f, 0xff, 0x02, 0x80, 0xff, 0x3a, 0x80, 0xff, 0x02, 0x80, 0xf7,
  0x3a, 0x80, 0xf3, 0x02, 0x80, 0xf1, 0xfe, 0xff, 0xf0, 0x00, 0x00, 0xf0
};

gchar Preview_bits[] = {
  0x00, 0x00, 0xf0, 0xfc, 0xff, 0xf3, 0xfc, 0xff, 0xf3, 0xfc, 0x57, 0xf3,
  0xfc, 0xff, 0xf3, 0x04, 0x00, 0xf2, 0x04, 0x00, 0xf2, 0xc4, 0x3f, 0xf2,
  0x44, 0x20, 0xf2, 0x44, 0x20, 0xf2, 0x44, 0x20, 0xf2, 0x44, 0x20, 0xf2,
  0x44, 0x20, 0xf2, 0x44, 0x20, 0xf2, 0x44, 0x20, 0xf2, 0x44, 0x20, 0xf2,
  0xc4, 0x3f, 0xf2, 0x04, 0x00, 0xf2, 0xfc, 0xff, 0xf3, 0x00, 0x00, 0xf0
};

gchar Interrupt_bits[] = {
  0x00, 0x00, 0x00, 0x80, 0x1f, 0x00, 0xe0, 0x7f, 0x00, 0xf0, 0xff, 0x00,
  0xf8, 0xff, 0x01, 0xfc, 0xff, 0x03, 0xfc, 0xff, 0x03, 0x06, 0x64, 0x06,
  0x7a, 0x5b, 0x05, 0x66, 0x5b, 0x05, 0x5e, 0x5b, 0x06, 0x62, 0x67, 0x07,
  0xfe, 0xff, 0x07, 0xfc, 0xff, 0x03, 0xfc, 0xff, 0x03, 0xf8, 0xff, 0x01,
  0xf0, 0xff, 0x00, 0xe0, 0x7f, 0x00, 0x80, 0x1f, 0x00, 0x00, 0x00, 0x00
};

gchar Point_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x1c, 0x00, 0xf0, 0xf4, 0x00, 0xf0,
  0x8c, 0x07, 0xf0, 0x08, 0x3c, 0xf0, 0x08, 0x60, 0xf0, 0x18, 0x38, 0xf0,
  0x10, 0x08, 0xf0, 0x10, 0x18, 0xf0, 0x30, 0x30, 0xf0, 0xa0, 0x63, 0xf0,
  0xa0, 0xc6, 0xf0, 0xe0, 0x8c, 0xf1, 0x40, 0x18, 0xf3, 0x00, 0x30, 0xf6,
  0x00, 0x60, 0xf3, 0x00, 0xc0, 0xf1, 0x00, 0x80, 0xf0, 0x00, 0x00, 0xf0
};

gchar Legendpoint_bits[] = {
  0xcc, 0x31, 0xf0, 0x52, 0x4a, 0xf0, 0xd2, 0x09, 0xf0, 0x5e, 0x6a, 0xf0,
  0xd2, 0x31, 0xf0, 0x00, 0x00, 0xf0, 0xc0, 0x03, 0xf0, 0x40, 0x1e, 0xf0,
  0x40, 0xf0, 0xf1, 0xc0, 0x00, 0xf3, 0x80, 0xc0, 0xf1, 0x80, 0x60, 0xf0,
  0x80, 0xc1, 0xf0, 0x00, 0x89, 0xf1, 0x00, 0x1d, 0xf3, 0x00, 0x35, 0xf6,
  0x00, 0x67, 0xfc, 0x00, 0xc2, 0xf6, 0x00, 0x80, 0xf3, 0x00, 0x00, 0xf1
};

gchar Line_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x80, 0xf3, 0x00, 0xc0, 0xf1,
  0x00, 0xe0, 0xf0, 0x00, 0x70, 0xf0, 0x00, 0x38, 0xf0, 0x00, 0x1c, 0xf0,
  0x00, 0x0e, 0xf0, 0x00, 0x07, 0xf0, 0x80, 0x03, 0xf0, 0xc0, 0x01, 0xf0,
  0xe0, 0x00, 0xf0, 0x70, 0x00, 0xf0, 0x3c, 0x00, 0xf0, 0xfc, 0xff, 0xf1,
  0xfc, 0xff, 0xf1, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0
};

gchar Curve_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0xf0, 0x01, 0xf0, 0xf8, 0x01, 0xf0,
  0x1c, 0x00, 0xf0, 0x0c, 0x00, 0xf0, 0x0c, 0x00, 0xf0, 0x0c, 0x00, 0xf0,
  0x1c, 0xf8, 0xf0, 0x1c, 0xfc, 0xf1, 0xf8, 0x8f, 0xf3, 0xf0, 0x07, 0xf3,
  0x00, 0x00, 0xf3, 0x00, 0x00, 0xf3, 0x00, 0x00, 0xf3, 0x00, 0x80, 0xf3,
  0x00, 0xf8, 0xf1, 0x00, 0xf8, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0
};

gchar Polygon_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x04, 0xf0, 0x00, 0x0e, 0xf0,
  0x00, 0x1b, 0xf0, 0x80, 0x31, 0xf0, 0xc0, 0x60, 0xf0, 0x60, 0xc0, 0xf0,
  0x30, 0x80, 0xf1, 0x18, 0x00, 0xf3, 0x0c, 0x00, 0xf6, 0x18, 0x00, 0xf3,
  0x30, 0x80, 0xf1, 0x60, 0xc0, 0xf0, 0xc0, 0x60, 0xf0, 0x80, 0x31, 0xf0,
  0x00, 0x1b, 0xf0, 0x00, 0x0e, 0xf0, 0x00, 0x04, 0xf0, 0x00, 0x00, 0xf0
};

gchar Rect_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0xf0, 0xff, 0xf1,
  0xf0, 0xff, 0xf1, 0x30, 0x80, 0xf1, 0x30, 0x80, 0xf1, 0x30, 0x80, 0xf1,
  0x30, 0x80, 0xf1, 0x30, 0x80, 0xf1, 0x30, 0x80, 0xf1, 0x30, 0x80, 0xf1,
  0x30, 0x80, 0xf1, 0x30, 0x80, 0xf1, 0x30, 0x80, 0xf1, 0x30, 0x80, 0xf1,
  0xf0, 0xff, 0xf1, 0xf0, 0xff, 0xf1, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0
};

gchar Arc_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x1f, 0xf0, 0xc0, 0x7f, 0xf0,
  0xe0, 0xe0, 0xf0, 0x30, 0x80, 0xf1, 0x18, 0x00, 0xf3, 0x18, 0x00, 0xf3,
  0x0c, 0x00, 0xf6, 0x0c, 0x00, 0xf6, 0x0c, 0x00, 0xf6, 0x0c, 0x00, 0xf6,
  0x0c, 0x00, 0xf6, 0x18, 0x00, 0xf3, 0x18, 0x00, 0xf3, 0x30, 0x80, 0xf1,
  0xe0, 0xe0, 0xf0, 0xc0, 0x7f, 0xf0, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0xf0
};

gchar Mark_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x1f, 0xf0, 0xc0, 0x7f, 0xf0,
  0xe0, 0xfc, 0xf0, 0x30, 0xfc, 0xf1, 0x18, 0xfc, 0xf3, 0x18, 0xfc, 0xf3,
  0x0c, 0xfc, 0xf7, 0x0c, 0xfc, 0xf7, 0x0c, 0xfc, 0xf7, 0x0c, 0xfc, 0xf7,
  0x0c, 0xfc, 0xf7, 0x18, 0xfc, 0xf3, 0x18, 0xfc, 0xf3, 0x30, 0xfc, 0xf1,
  0xe0, 0xfc, 0xf0, 0xc0, 0x7f, 0xf0, 0x00, 0x1f, 0xf0, 0x00, 0x00, 0xf0
};

gchar Text_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0,
  0xf8, 0xff, 0xf3, 0x38, 0x8e, 0xf3, 0x18, 0x0e, 0xf3, 0x18, 0x0e, 0xf3,
  0x00, 0x0e, 0xf0, 0x00, 0x0e, 0xf0, 0x00, 0x0e, 0xf0, 0x00, 0x0e, 0xf0,
  0x00, 0x0e, 0xf0, 0x00, 0x0e, 0xf0, 0x00, 0x0e, 0xf0, 0x00, 0x0e, 0xf0,
  0xc0, 0x7f, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0
};

gchar Gauss_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0,
  0x00, 0x00, 0xf0, 0x00, 0x0e, 0xf0, 0x00, 0x11, 0xf0, 0x80, 0x31, 0xf0,
  0x80, 0x31, 0xf0, 0x80, 0x31, 0xf0, 0xc0, 0x60, 0xf0, 0xc0, 0x60, 0xf0,
  0xc0, 0x60, 0xf0, 0xc0, 0x60, 0xf0, 0x60, 0xe0, 0xf0, 0x60, 0xc0, 0xf0,
  0x7c, 0xc0, 0xf7, 0x3c, 0x80, 0xf7, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0
};

gchar Axispoint_bits[] = {
  0x00, 0x00, 0xf0, 0x08, 0x00, 0xf0, 0x1c, 0x00, 0xf0, 0x3e, 0x00, 0xf0,
  0x08, 0x00, 0xf0, 0x08, 0x00, 0xf0, 0xdc, 0x03, 0xf0, 0x48, 0x1e, 0xf0,
  0x48, 0xf0, 0xf1, 0xdc, 0x00, 0xf3, 0x88, 0xc0, 0xf1, 0x88, 0x60, 0xf0,
  0x9c, 0xc1, 0xf0, 0x08, 0x89, 0xf1, 0x08, 0x1d, 0xf3, 0x3e, 0x35, 0xf6,
  0x08, 0x67, 0xfc, 0x08, 0xc2, 0xf6, 0x1c, 0x80, 0xf3, 0x08, 0x00, 0xf1
};

gchar Frame_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0xf8, 0xff, 0xf3,
  0x88, 0x24, 0xf2, 0x08, 0x04, 0xf2, 0x08, 0x00, 0xf2, 0x18, 0x00, 0xf3,
  0x08, 0x00, 0xf2, 0x08, 0x00, 0xf2, 0x38, 0x80, 0xf3, 0x08, 0x00, 0xf2,
  0x08, 0x00, 0xf2, 0x18, 0x00, 0xf3, 0x08, 0x00, 0xf2, 0x08, 0x04, 0xf2,
  0x88, 0x24, 0xf2, 0xf8, 0xff, 0xf3, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0
};

gchar Section_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0xfc, 0xff, 0xf3, 0x24, 0x49, 0xf2,
  0x24, 0x49, 0xf2, 0xfc, 0xff, 0xf3, 0x24, 0x49, 0xf2, 0x24, 0x49, 0xf2,
  0xfc, 0xff, 0xf3, 0x24, 0x49, 0xf2, 0x24, 0x49, 0xf2, 0xfc, 0xff, 0xf3,
  0x24, 0x49, 0xf2, 0x24, 0x49, 0xf2, 0xfc, 0xff, 0xf3, 0x24, 0x49, 0xf2,
  0x24, 0x49, 0xf2, 0xfc, 0xff, 0xf3, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0
};

gchar Cross_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x02, 0xf0, 0x00, 0x07, 0xf0,
  0x80, 0x0f, 0xf0, 0x00, 0x02, 0xf0, 0x00, 0x02, 0xf0, 0x00, 0x07, 0xf0,
  0x00, 0x02, 0xf0, 0x00, 0x02, 0xf0, 0x00, 0x07, 0xf0, 0x00, 0x02, 0xf0,
  0x00, 0x02, 0xf1, 0x20, 0x22, 0xf3, 0xfc, 0xff, 0xf7, 0x20, 0x22, 0xf3,
  0x20, 0x02, 0xf1, 0x00, 0x02, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0
};

gchar Single_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x02, 0xf0, 0x00, 0x07, 0xf0,
  0x80, 0x0f, 0xf0, 0x00, 0x02, 0xf0, 0x00, 0x02, 0xf0, 0x00, 0x07, 0xf0,
  0x00, 0x02, 0xf0, 0x00, 0x02, 0xf0, 0x00, 0x07, 0xf0, 0x00, 0x02, 0xf0,
  0x00, 0x02, 0xf0, 0x00, 0x07, 0xf0, 0x00, 0x02, 0xf0, 0x00, 0x02, 0xf0,
  0x00, 0x07, 0xf0, 0x00, 0x02, 0xf0, 0x00, 0x02, 0xf0, 0x00, 0x00, 0xf0
};

gchar Trimming_bits[] = {
  0x00, 0x00, 0xf0, 0x10, 0x00, 0xf0, 0x78, 0x02, 0xf0, 0x7c, 0x05, 0xf0,
  0x10, 0x09, 0xf0, 0x10, 0x12, 0xf0, 0x38, 0x22, 0xf0, 0x10, 0x44, 0xf0,
  0x18, 0x44, 0xf0, 0x64, 0x48, 0xf0, 0x88, 0x51, 0xf0, 0x10, 0x32, 0xf3,
  0x38, 0xfc, 0xf5, 0x50, 0x78, 0xf8, 0x90, 0xf7, 0xfb, 0x38, 0x50, 0xfe,
  0x10, 0x58, 0xf0, 0x10, 0xc8, 0xf0, 0x7c, 0x90, 0xf0, 0x10, 0xe0, 0xf0
};

gchar Datapoint_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x18, 0x00, 0xf0, 0x3c, 0x00, 0xf0,
  0x3c, 0x00, 0xf0, 0x18, 0x00, 0xf0, 0xc0, 0x03, 0xf0, 0x40, 0x1e, 0xf0,
  0x40, 0xf0, 0xf1, 0xc0, 0x00, 0xf3, 0x80, 0xc0, 0xf1, 0x80, 0x60, 0xf0,
  0x80, 0xc1, 0xf0, 0x00, 0x89, 0xf1, 0x00, 0x1d, 0xf3, 0x00, 0x35, 0xf6,
  0x00, 0x67, 0xfc, 0x00, 0xc2, 0xf6, 0x00, 0x80, 0xf3, 0x00, 0x00, 0xf1
};

gchar Eval_bits[] = {
  0x00, 0x00, 0xf0, 0xfe, 0x01, 0xf0, 0x02, 0x01, 0xf0, 0x0e, 0x01, 0xf0,
  0x02, 0x01, 0xf0, 0x0e, 0x01, 0xf0, 0x02, 0x01, 0xf0, 0x0e, 0x01, 0xf0,
  0x02, 0xf9, 0xf3, 0x1e, 0xfd, 0xf7, 0x02, 0x0d, 0xf6, 0x0e, 0x0d, 0xf6,
  0x02, 0xc1, 0xf7, 0x0e, 0xe1, 0xf3, 0x02, 0xe1, 0xf0, 0x0e, 0x01, 0xf0,
  0x02, 0xe1, 0xf0, 0xfe, 0xe1, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0
};

gchar Zoom_bits[] = {
  0x00, 0x00, 0xf0, 0x00, 0x7e, 0xf0, 0x80, 0xff, 0xf1, 0xc0, 0x81, 0xf3,
  0xc0, 0x00, 0xf3, 0x60, 0x18, 0xf6, 0x60, 0x18, 0xf6, 0x60, 0x7e, 0xf6,
  0x60, 0x7e, 0xf6, 0x60, 0x18, 0xf6, 0x60, 0x18, 0xf6, 0xe0, 0x00, 0xf3,
  0xf0, 0xc1, 0xf3, 0xf8, 0xff, 0xf1, 0xbc, 0x7f, 0xf0, 0xdc, 0x00, 0xf0,
  0x6c, 0x00, 0xf0, 0x38, 0x00, 0xf0, 0x00, 0x00, 0xf0, 0x00, 0x00, 0xf0
};

gchar Math_bits[] = {
  0x00, 0x00, 0xf0, 0xfc, 0xff, 0xf1, 0x04, 0x00, 0xf3, 0xf4, 0x7f, 0xf3,
  0x54, 0x55, 0xf3, 0x54, 0x55, 0xf3, 0xf4, 0x7f, 0xf3, 0x04, 0x00, 0xf3,
  0xb4, 0x6d, 0xf3, 0xb4, 0x6d, 0xf3, 0x04, 0x00, 0xf3, 0xb4, 0x6d, 0xf3,
  0xb4, 0x6d, 0xf3, 0x04, 0x00, 0xf3, 0xb4, 0x6d, 0xf3, 0xb4, 0x6d, 0xf3,
  0x04, 0x00, 0xf3, 0xfc, 0xff, 0xf3, 0xf8, 0xff, 0xf3, 0x00, 0x00, 0xf0
};

gchar Scaleundo_bits[] = {
  0x00, 0x00, 0xf0, 0x29, 0x3a, 0xf6, 0x69, 0x4a, 0xf9, 0xa9, 0x4a, 0xf9,
  0x29, 0x4b, 0xf9, 0x26, 0x3a, 0xf6, 0x00, 0x00, 0xf0, 0x80, 0x00, 0xf0,
  0xc0, 0x00, 0xf0, 0xe0, 0x00, 0xf0, 0xf0, 0xff, 0xf7, 0xf8, 0xff, 0xf7,
  0xfc, 0xff, 0xf7, 0xfe, 0xff, 0xf7, 0xfc, 0xff, 0xf7, 0xf8, 0xff, 0xf7,
  0xf0, 0xff, 0xf7, 0xe0, 0x00, 0xf0, 0xc0, 0x00, 0xf0, 0x80, 0x00, 0xf0
};


/* XPM */
gchar * Arc_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
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
gchar * Axispoint_xpm[] = {
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
gchar * Axiswin_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "................... ",
  ".+++++++++++++++++..",
  ".++++++.++++++++++..",
  ".+++++...+++++++++..",
  ".++++.....++++++++..",
  ".++++++.++++++++++..",
  ".++++++.++++++++++..",
  ".+++++...+++++++++..",
  ".++++++.++++++++++..",
  ".++++++.++++++++++..",
  ".+++++...+++++++++..",
  ".++++++.+++++.++++..",
  ".++++++.++.++..+++..",
  ".+++............++..",
  ".++++++.++.++..+++..",
  ".++++++.+++++.++++..",
  ".++++++.++++++++++..",
  ".+++++++++++++++++..",
  "....................",
  " ..................."};
/* XPM */
gchar * Clear_xpm[] = {
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
gchar * Coordwin_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "................... ",
  ".+++++++++++++++++..",
  ".+++++++++++++++++..",
  ".+++..++++++++++++..",
  ".++....+++++++++++..",
  ".++....+++++++++++..",
  ".+++..++..++++++++..",
  ".+++++++++..++++++..",
  ".+++++++++++..++++..",
  ".+++++++++++++..++..",
  ".+++++++++++++++++..",
  ".+++++++++++++++++..",
  ".+++.+++.++.+++.++..",
  ".++++.+.++++.+.+++..",
  ".+++++.++++++.++++..",
  ".++++.+.+++++.++++..",
  ".+++.+++.++++.++++..",
  ".+++++++++++++++++..",
  "....................",
  " ..................."};
/* XPM */
gchar * Cross_xpm[] = {
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
gchar * Curve_xpm[] = {
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
  "  ...      .....    ",
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
gchar * Datapoint_xpm[] = {
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
gchar * Draw_xpm[] = {
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
gchar * Eval_xpm[] = {
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
gchar * Fileopen_xpm[] = {
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
gchar * Filewin_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "................... ",
  ".+++++++++++++++++..",
  ".++++.++++++++++++..",
  ".++++.++++++++++++..",
  ".++++.+++.+.+.+.++..",
  ".++++.+++.+.+.+.++..",
  ".+++++++++++++++++..",
  ".+++..++++++++++++..",
  ".+++++.+++++++++++..",
  ".++++.+++.+.+.+.++..",
  ".+++...++.+.+.+.++..",
  ".+++++++++++++++++..",
  ".+++..++++++++++++..",
  ".+++++.+++++++++++..",
  ".++++..++.+.+.+.++..",
  ".+++++.++.+.+.+.++..",
  ".+++..++++++++++++..",
  ".+++++++++++++++++..",
  "....................",
  " ..................."};
/* XPM */
gchar * Frame_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
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
gchar * Gauss_xpm[] = {
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
gchar * Infowin_xpm[] = {
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
gchar * Interrupt_xpm[] = {
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
gchar * Legendpoint_xpm[] = {
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
gchar * Legendwin_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "................... ",
  ".+++++++++++++++++..",
  ".+++.+.+++++++++++..",
  ".+++.+.+..++++++++..",
  ".+++.+.+++.+++++++..",
  ".++++++++++.++++++..",
  ".+++++++++++.+++++..",
  ".++++++++++++.+.++..",
  ".++++.++++++++..++..",
  ".+++...++++++...++..",
  ".++++.++++++++++++..",
  ".+++++.+++++++++++..",
  ".+++++.+++++++++++..",
  ".++++++.++++++++++..",
  ".++++++.++++.+.+++..",
  ".+++++++...+.+.+++..",
  ".+++++++++++.+.+++..",
  ".+++++++++++++++++..",
  "....................",
  " ..................."};
/* XPM */
gchar * Line_xpm[] = {
  "20 20 2 1",
  " 	c None",
  ".	c #000000",
  "                    ",
  "                    ",
  "               ...  ",
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
  "  ....              ",
  "  ...............   ",
  "  ...............   ",
  "                    ",
  "                    ",
  "                    "};
/* XPM */
gchar * Load_xpm[] = {
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
gchar * Mark_xpm[] = {
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
gchar * Math_xpm[] = {
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
gchar * Mergewin_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "................... ",
  ".+++++++++++++++++..",
  ".+...............+..",
  ".+..++.++.++.++..+..",
  ".+..+++++++++++..+..",
  ".+...+++++++++...+..",
  ".+..+++++++++++..+..",
  ".+..++.++.++.++..+..",
  ".+...............+..",
  ".+++++++++++++++++..",
  ".+...............+..",
  ".+..++.++.++.++..+..",
  ".+..+++++++++++..+..",
  ".+...+++++++++...+..",
  ".+..+++++++++++..+..",
  ".+..++.++.++.++..+..",
  ".+...............+..",
  ".+++++++++++++++++..",
  "....................",
  " ..................."};
/* XPM */
gchar * Point_xpm[] = {
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
gchar * Polygon_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
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
gchar * Preview_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "                    ",
  "  ................  ",
  "  ................  ",
  "  .........+.+.+..  ",
  "  ................  ",
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
gchar * Print_xpm[] = {
  "20 20 3 1",
  " 	c None",
  ".	c #000000",
  "+	c #FFFFFF",
  "                    ",
  "                    ",
  "        ............",
  "       .++++++++++. ",
  "      .++++++++++.  ",
  "     .++++++++++.   ",
  "     .++++++++++.   ",
  "    .++++++++++.    ",
  "    .++++++++++.....",
  "    .++++++++++...+.",
  "   ..++++++++++..+..",
  "  ..............+...",
  " ..............+....",
  " .+++++++++++++.....",
  " .+...+++++++++.....",
  " .+++++++++++++.... ",
  " .+...+++++++++...  ",
  " .+++++++++++++..   ",
  " ...............    ",
  "                    "};
/* XPM */
gchar * Rect_xpm[] = {
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
gchar * Save_xpm[] = {
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
gchar * Scale_xpm[] = {
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
gchar * Scaleundo_xpm[] = {
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
gchar * Section_xpm[] = {
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
gchar * Single_xpm[] = {
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
gchar * Text_xpm[] = {
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
gchar * Trimming_xpm[] = {
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
gchar * Zoom_xpm[] = {
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

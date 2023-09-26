#include "common.h"

#if WINDOWS
#include <stdlib.h>
#include <stdarg.h>
#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <unistd.h>
#include <math.h>
#include <cairo/cairo.h>

#include "gtk_common.h"

#include "object.h"
#include "nstring.h"
#include "ioutil.h"
#include "shell.h"
#include "strconv.h"
#include "gra.h"
#include "ogra2cairo.h"

#define NAME "gra2emf"
#define PARENT "gra2"
#define OVERSION  "1.00.00"

#define DEFAULT_FONT "Sans-serif"

#define USE_LINE_TO 1

#ifndef MPI
#define MPI 3.14159265358979323846
#endif

#define ERRFOPEN 100
#define ERREMF   101

#define USE_EnumFontFamiliesExW 0
/* FixMe: the program will be crashed when use EnumLogFontExW */

static char *gra2emf_errorlist[]={
  "I/O error: open file",
  "EMF error",
};

#define ERRNUM (sizeof(gra2emf_errorlist) / sizeof(*gra2emf_errorlist))

#define CHAR_SET_NUM 32

struct gra2emf_fontmap
{
  char *name;
  int char_set[CHAR_SET_NUM + 1];
  struct gra2emf_fontmap *next;
};

struct gra2emf_local {
  HDC hdc, hdc_dummy;
  int r, g, b, x, y, offsetx, offsety;
  char *fontalias, *fontname;
  int font_style, line_join, line_cap, line_width, line_style_num, symbol;
  int update_pen_attribute, update_brush_attribute;
  double fontdir, fontcos, fontsin, fontspace, fontsize;
  DWORD *line_style;
#if USE_LINE_TO
  int line;
#else
  struct narray line;
#endif
  NHASH fontmap;
  HPEN null_pen, the_pen;
  HBRUSH the_brush;
};

static void draw_lines(struct gra2emf_local *local);
static int close_emf(struct gra2emf_local *local, const char *fname);

static int
#if USE_EnumFontFamiliesExW
enum_font_cb(ENUMLOGFONTEXW *lpelfe, NEWTEXTMETRICEXW *lpntme, DWORD FontType, LPARAM lParam)
#else
enum_font_cb(ENUMLOGFONTEX *lpelfe, NEWTEXTMETRICEX *lpntme, DWORD FontType, LPARAM lParam)
#endif
{
  int char_set, i;
  struct gra2emf_fontmap *fontmap;

  if (lpelfe == NULL || lpntme == NULL || lParam == 0) {
    return 0;
  }

  if (FontType == 0) {
    return 0;			/* this check may necessary to avoid crash on Windows8 */
  }

  if (FontType & ~(TRUETYPE_FONTTYPE | RASTER_FONTTYPE | DEVICE_FONTTYPE)) {
    return 0;			/* this check may necessary to avoid crash on Windows8 */
  }

  if (lpntme->ntmTm.tmWeight != FW_NORMAL || lpntme->ntmTm.tmItalic) {
    return 1;
  }

  fontmap = (struct gra2emf_fontmap *) lParam;
  char_set = lpntme->ntmTm.tmCharSet;

  for (i = 0; i < CHAR_SET_NUM; i++) {
    if (fontmap->char_set[i] < 0) {
      fontmap->char_set[i] = char_set;
      fontmap->char_set[i + 1] = -1;
      break;
    }
  }

  return 1;
}

static int
get_char_set(gunichar ch)
{
  GUnicodeScript script;
  int char_set;

  script = g_unichar_get_script(ch);

  switch (script) {
  case G_UNICODE_SCRIPT_COMMON:
    char_set = ANSI_CHARSET;
    break;
  case G_UNICODE_SCRIPT_INHERITED:
    char_set = DEFAULT_CHARSET;
    break;
  case G_UNICODE_SCRIPT_ARABIC:
    char_set = ARABIC_CHARSET;
    break;
  case G_UNICODE_SCRIPT_ARMENIAN:
    char_set = EASTEUROPE_CHARSET;
    break;
  case G_UNICODE_SCRIPT_BENGALI:
  case G_UNICODE_SCRIPT_BOPOMOFO:
  case G_UNICODE_SCRIPT_CHEROKEE:
  case G_UNICODE_SCRIPT_COPTIC:
    char_set = DEFAULT_CHARSET;
    break;
  case G_UNICODE_SCRIPT_CYRILLIC:
    char_set = RUSSIAN_CHARSET;
    break;
  case G_UNICODE_SCRIPT_DESERET:
  case G_UNICODE_SCRIPT_DEVANAGARI:
  case G_UNICODE_SCRIPT_ETHIOPIC:
  case G_UNICODE_SCRIPT_GEORGIAN:
  case G_UNICODE_SCRIPT_GOTHIC:
    char_set = DEFAULT_CHARSET;
    break;
  case G_UNICODE_SCRIPT_GREEK:
    char_set = GREEK_CHARSET;
    break;
  case G_UNICODE_SCRIPT_GUJARATI:
  case G_UNICODE_SCRIPT_GURMUKHI:
    char_set = DEFAULT_CHARSET;
    break;
  case G_UNICODE_SCRIPT_HAN:
/*
    char_set = CHINESEBIG5_CHARSET;
    char_set = GB2312_CHARSET;
*/
    char_set = SHIFTJIS_CHARSET;
    break;
  case G_UNICODE_SCRIPT_HANGUL:
    char_set = HANGEUL_CHARSET;
    break;
  case G_UNICODE_SCRIPT_HEBREW:
    char_set = HEBREW_CHARSET;
    break;
  case G_UNICODE_SCRIPT_HIRAGANA:
    char_set = SHIFTJIS_CHARSET;
    break;
  case G_UNICODE_SCRIPT_KANNADA:
    char_set = DEFAULT_CHARSET;
    break;
  case G_UNICODE_SCRIPT_KATAKANA:
    char_set = SHIFTJIS_CHARSET;
    break;
  case G_UNICODE_SCRIPT_KHMER:
  case G_UNICODE_SCRIPT_LAO:
    char_set = DEFAULT_CHARSET;
    break;
  case G_UNICODE_SCRIPT_LATIN:
    char_set = ANSI_CHARSET;
    break;
  case G_UNICODE_SCRIPT_MALAYALAM:
  case G_UNICODE_SCRIPT_MONGOLIAN:
  case G_UNICODE_SCRIPT_MYANMAR:
  case G_UNICODE_SCRIPT_OGHAM:
  case G_UNICODE_SCRIPT_OLD_ITALIC:
  case G_UNICODE_SCRIPT_ORIYA:
  case G_UNICODE_SCRIPT_RUNIC:
  case G_UNICODE_SCRIPT_SINHALA:
  case G_UNICODE_SCRIPT_SYRIAC:
  case G_UNICODE_SCRIPT_TAMIL:
  case G_UNICODE_SCRIPT_TELUGU:
  case G_UNICODE_SCRIPT_THAANA:
    char_set = DEFAULT_CHARSET;
    break;
  case G_UNICODE_SCRIPT_THAI:
    char_set = THAI_CHARSET;
    break;
  case G_UNICODE_SCRIPT_TIBETAN:
  case G_UNICODE_SCRIPT_CANADIAN_ABORIGINAL:
  case G_UNICODE_SCRIPT_YI:
  case G_UNICODE_SCRIPT_TAGALOG:
  case G_UNICODE_SCRIPT_HANUNOO:
  case G_UNICODE_SCRIPT_BUHID:
  case G_UNICODE_SCRIPT_TAGBANWA:
  case G_UNICODE_SCRIPT_BRAILLE:
  case G_UNICODE_SCRIPT_CYPRIOT:
  case G_UNICODE_SCRIPT_LIMBU:
  case G_UNICODE_SCRIPT_OSMANYA:
  case G_UNICODE_SCRIPT_SHAVIAN:
  case G_UNICODE_SCRIPT_LINEAR_B:
  case G_UNICODE_SCRIPT_TAI_LE:
  case G_UNICODE_SCRIPT_UGARITIC:
  case G_UNICODE_SCRIPT_NEW_TAI_LUE:
  case G_UNICODE_SCRIPT_BUGINESE:
  case G_UNICODE_SCRIPT_GLAGOLITIC:
  case G_UNICODE_SCRIPT_TIFINAGH:
  case G_UNICODE_SCRIPT_SYLOTI_NAGRI:
  case G_UNICODE_SCRIPT_OLD_PERSIAN:
  case G_UNICODE_SCRIPT_KHAROSHTHI:
  case G_UNICODE_SCRIPT_UNKNOWN:
  case G_UNICODE_SCRIPT_BALINESE:
  case G_UNICODE_SCRIPT_CUNEIFORM:
  case G_UNICODE_SCRIPT_PHOENICIAN:
  case G_UNICODE_SCRIPT_PHAGS_PA:
  case G_UNICODE_SCRIPT_NKO:
  case G_UNICODE_SCRIPT_KAYAH_LI:
  case G_UNICODE_SCRIPT_LEPCHA:
  case G_UNICODE_SCRIPT_REJANG:
  case G_UNICODE_SCRIPT_SUNDANESE:
  case G_UNICODE_SCRIPT_SAURASHTRA:
  case G_UNICODE_SCRIPT_CHAM:
  case G_UNICODE_SCRIPT_OL_CHIKI:
  case G_UNICODE_SCRIPT_VAI:
  case G_UNICODE_SCRIPT_CARIAN:
  case G_UNICODE_SCRIPT_LYCIAN:
  case G_UNICODE_SCRIPT_LYDIAN:
  case G_UNICODE_SCRIPT_AVESTAN:
  case G_UNICODE_SCRIPT_BAMUM:
  case G_UNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS:
  case G_UNICODE_SCRIPT_IMPERIAL_ARAMAIC:
  case G_UNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI:
  case G_UNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN:
  case G_UNICODE_SCRIPT_JAVANESE:
  case G_UNICODE_SCRIPT_KAITHI:
  case G_UNICODE_SCRIPT_LISU:
  case G_UNICODE_SCRIPT_MEETEI_MAYEK:
  case G_UNICODE_SCRIPT_OLD_SOUTH_ARABIAN:
    char_set = DEFAULT_CHARSET;
    break;
  default:
    char_set = DEFAULT_CHARSET;
  }

  /*
    0:   'ANSI_CHARSET';
    1:   'DEFAULT_CHARSET';
    2:   'SYMBOL_CHARSET';
    77:  'MAC_CHARSET';
    128: 'SHIFTJIS_CHARSET';
    129: 'HANGEUL_CHARSET';
    130: 'JOHAB_CHARSET';
    134: 'GB2312_CHARSET';
    136: 'CHINESEBIG5_CHARSET';
    161: 'GREEK_CHARSET';
    162: 'TURKISH_CHARSET';
    163: 'VIETNAMESE_CHARSET';
    177: 'HEBREW_CHARSET';
    178: 'ARABIC_CHARSET';
    186: 'BALTIC_CHARSET';
    204: 'RUSSIAN_CHARSET';
    222: 'THAI_CHARSET';
    238: 'EASTEUROPE_CHARSET';
    255: 'OEM_CHARSET ';
  */
#if 0
  {
    char *script_name;
    char str[16], *sstr;
    int l;

    switch (script) {
    case G_UNICODE_SCRIPT_COMMON:
      script_name = "G_UNICODE_SCRIPT_COMMON";
      break;
    case G_UNICODE_SCRIPT_INHERITED:
      script_name = "G_UNICODE_SCRIPT_INHERITED";
      break;
    case G_UNICODE_SCRIPT_ARABIC:
      script_name = "G_UNICODE_SCRIPT_ARABIC";
      break;
    case G_UNICODE_SCRIPT_ARMENIAN:
      script_name = "G_UNICODE_SCRIPT_ARMENIAN";
      break;
    case G_UNICODE_SCRIPT_BENGALI:
      script_name = "G_UNICODE_SCRIPT_BENGALI";
      break;
    case G_UNICODE_SCRIPT_BOPOMOFO:
      script_name = "G_UNICODE_SCRIPT_BOPOMOFO";
      break;
    case G_UNICODE_SCRIPT_CHEROKEE:
      script_name = "G_UNICODE_SCRIPT_CHEROKEE";
      break;
    case G_UNICODE_SCRIPT_COPTIC:
      script_name = "G_UNICODE_SCRIPT_COPTIC";
      break;
    case G_UNICODE_SCRIPT_CYRILLIC:
      script_name = "G_UNICODE_SCRIPT_CYRILLIC";
      break;
    case G_UNICODE_SCRIPT_DESERET:
      script_name = "G_UNICODE_SCRIPT_DESERET";
      break;
    case G_UNICODE_SCRIPT_DEVANAGARI:
      script_name = "G_UNICODE_SCRIPT_DEVANAGARI";
      break;
    case G_UNICODE_SCRIPT_ETHIOPIC:
      script_name = "G_UNICODE_SCRIPT_ETHIOPIC";
      break;
    case G_UNICODE_SCRIPT_GEORGIAN:
      script_name = "G_UNICODE_SCRIPT_GEORGIAN";
      break;
    case G_UNICODE_SCRIPT_GOTHIC:
      script_name = "G_UNICODE_SCRIPT_GOTHIC";
      break;
    case G_UNICODE_SCRIPT_GREEK:
      script_name = "G_UNICODE_SCRIPT_GREEK";
      break;
    case G_UNICODE_SCRIPT_GUJARATI:
      script_name = "G_UNICODE_SCRIPT_GUJARATI";
      break;
    case G_UNICODE_SCRIPT_GURMUKHI:
      script_name = "G_UNICODE_SCRIPT_GURMUKHI";
      break;
    case G_UNICODE_SCRIPT_HAN:
      script_name = "G_UNICODE_SCRIPT_HAN";
      break;
    case G_UNICODE_SCRIPT_HANGUL:
      script_name = "G_UNICODE_SCRIPT_HANGUL";
      break;
    case G_UNICODE_SCRIPT_HEBREW:
      script_name = "G_UNICODE_SCRIPT_HEBREW";
      break;
    case G_UNICODE_SCRIPT_HIRAGANA:
      script_name = "G_UNICODE_SCRIPT_HIRAGANA";
      break;
    case G_UNICODE_SCRIPT_KANNADA:
      script_name = "G_UNICODE_SCRIPT_KANNADA";
      break;
    case G_UNICODE_SCRIPT_KATAKANA:
      script_name = "G_UNICODE_SCRIPT_KATAKANA";
      break;
    case G_UNICODE_SCRIPT_KHMER:
      script_name = "G_UNICODE_SCRIPT_KHMER";
      break;
    case G_UNICODE_SCRIPT_LAO:
      script_name = "G_UNICODE_SCRIPT_LAO";
      break;
    case G_UNICODE_SCRIPT_LATIN:
      script_name = "G_UNICODE_SCRIPT_LATIN";
      break;
    case G_UNICODE_SCRIPT_MALAYALAM:
      script_name = "G_UNICODE_SCRIPT_MALAYALAM";
      break;
    case G_UNICODE_SCRIPT_MONGOLIAN:
      script_name = "G_UNICODE_SCRIPT_MONGOLIAN";
      break;
    case G_UNICODE_SCRIPT_MYANMAR:
      script_name = "G_UNICODE_SCRIPT_MYANMAR";
      break;
    case G_UNICODE_SCRIPT_OGHAM:
      script_name = "G_UNICODE_SCRIPT_OGHAM";
      break;
    case G_UNICODE_SCRIPT_OLD_ITALIC:
      script_name = "G_UNICODE_SCRIPT_OLD_ITALIC";
      break;
    case G_UNICODE_SCRIPT_ORIYA:
      script_name = "G_UNICODE_SCRIPT_ORIYA";
      break;
    case G_UNICODE_SCRIPT_RUNIC:
      script_name = "G_UNICODE_SCRIPT_RUNIC";
      break;
    case G_UNICODE_SCRIPT_SINHALA:
      script_name = "G_UNICODE_SCRIPT_SINHALA";
      break;
    case G_UNICODE_SCRIPT_SYRIAC:
      script_name = "G_UNICODE_SCRIPT_SYRIAC";
      break;
    case G_UNICODE_SCRIPT_TAMIL:
      script_name = "G_UNICODE_SCRIPT_TAMIL";
      break;
    case G_UNICODE_SCRIPT_TELUGU:
      script_name = "G_UNICODE_SCRIPT_TELUGU";
      break;
    case G_UNICODE_SCRIPT_THAANA:
      script_name = "G_UNICODE_SCRIPT_THAANA";
      break;
    case G_UNICODE_SCRIPT_THAI:
      script_name = "G_UNICODE_SCRIPT_THAI";
      break;
    case G_UNICODE_SCRIPT_TIBETAN:
      script_name = "G_UNICODE_SCRIPT_TIBETAN";
      break;
    case G_UNICODE_SCRIPT_CANADIAN_ABORIGINAL:
      script_name = "G_UNICODE_SCRIPT_CANADIAN_ABORIGINAL";
      break;
    case G_UNICODE_SCRIPT_YI:
      script_name = "G_UNICODE_SCRIPT_YI";
      break;
    case G_UNICODE_SCRIPT_TAGALOG:
      script_name = "G_UNICODE_SCRIPT_TAGALOG";
      break;
    case G_UNICODE_SCRIPT_HANUNOO:
      script_name = "G_UNICODE_SCRIPT_HANUNOO";
      break;
    case G_UNICODE_SCRIPT_BUHID:
      script_name = "G_UNICODE_SCRIPT_BUHID";
      break;
    case G_UNICODE_SCRIPT_TAGBANWA:
      script_name = "G_UNICODE_SCRIPT_TAGBANWA";
      break;
    case G_UNICODE_SCRIPT_BRAILLE:
      script_name = "G_UNICODE_SCRIPT_BRAILLE";
      break;
    case G_UNICODE_SCRIPT_CYPRIOT:
      script_name = "G_UNICODE_SCRIPT_CYPRIOT";
      break;
    case G_UNICODE_SCRIPT_LIMBU:
      script_name = "G_UNICODE_SCRIPT_LIMBU";
      break;
    case G_UNICODE_SCRIPT_OSMANYA:
      script_name = "G_UNICODE_SCRIPT_OSMANYA";
      break;
    case G_UNICODE_SCRIPT_SHAVIAN:
      script_name = "G_UNICODE_SCRIPT_SHAVIAN";
      break;
    case G_UNICODE_SCRIPT_LINEAR_B:
      script_name = "G_UNICODE_SCRIPT_LINEAR_B";
      break;
    case G_UNICODE_SCRIPT_TAI_LE:
      script_name = "G_UNICODE_SCRIPT_TAI_LE";
      break;
    case G_UNICODE_SCRIPT_UGARITIC:
      script_name = "G_UNICODE_SCRIPT_UGARITIC";
      break;
    case G_UNICODE_SCRIPT_NEW_TAI_LUE:
      script_name = "G_UNICODE_SCRIPT_NEW_TAI_LUE";
      break;
    case G_UNICODE_SCRIPT_BUGINESE:
      script_name = "G_UNICODE_SCRIPT_BUGINESE";
      break;
    case G_UNICODE_SCRIPT_GLAGOLITIC:
      script_name = "G_UNICODE_SCRIPT_GLAGOLITIC";
      break;
    case G_UNICODE_SCRIPT_TIFINAGH:
      script_name = "G_UNICODE_SCRIPT_TIFINAGH";
      break;
    case G_UNICODE_SCRIPT_SYLOTI_NAGRI:
      script_name = "G_UNICODE_SCRIPT_SYLOTI_NAGRI";
      break;
    case G_UNICODE_SCRIPT_OLD_PERSIAN:
      script_name = "G_UNICODE_SCRIPT_OLD_PERSIAN";
      break;
    case G_UNICODE_SCRIPT_KHAROSHTHI:
      script_name = "G_UNICODE_SCRIPT_KHAROSHTHI";
      break;
    case G_UNICODE_SCRIPT_UNKNOWN:
      script_name = "G_UNICODE_SCRIPT_UNKNOWN";
      break;
    case G_UNICODE_SCRIPT_BALINESE:
      script_name = "G_UNICODE_SCRIPT_BALINESE";
      break;
    case G_UNICODE_SCRIPT_CUNEIFORM:
      script_name = "G_UNICODE_SCRIPT_CUNEIFORM";
      break;
    case G_UNICODE_SCRIPT_PHOENICIAN:
      script_name = "G_UNICODE_SCRIPT_PHOENICIAN";
      break;
    case G_UNICODE_SCRIPT_PHAGS_PA:
      script_name = "G_UNICODE_SCRIPT_PHAGS_PA";
      break;
    case G_UNICODE_SCRIPT_NKO:
      script_name = "G_UNICODE_SCRIPT_NKO";
      break;
    case G_UNICODE_SCRIPT_KAYAH_LI:
      script_name = "G_UNICODE_SCRIPT_KAYAH_LI";
      break;
    case G_UNICODE_SCRIPT_LEPCHA:
      script_name = "G_UNICODE_SCRIPT_LEPCHA";
      break;
    case G_UNICODE_SCRIPT_REJANG:
      script_name = "G_UNICODE_SCRIPT_REJANG";
      break;
    case G_UNICODE_SCRIPT_SUNDANESE:
      script_name = "G_UNICODE_SCRIPT_SUNDANESE";
      break;
    case G_UNICODE_SCRIPT_SAURASHTRA:
      script_name = "G_UNICODE_SCRIPT_SAURASHTRA";
      break;
    case G_UNICODE_SCRIPT_CHAM:
      script_name = "G_UNICODE_SCRIPT_CHAM";
      break;
    case G_UNICODE_SCRIPT_OL_CHIKI:
      script_name = "G_UNICODE_SCRIPT_OL_CHIKI";
      break;
    case G_UNICODE_SCRIPT_VAI:
      script_name = "G_UNICODE_SCRIPT_VAI";
      break;
    case G_UNICODE_SCRIPT_CARIAN:
      script_name = "G_UNICODE_SCRIPT_CARIAN";
      break;
    case G_UNICODE_SCRIPT_LYCIAN:
      script_name = "G_UNICODE_SCRIPT_LYCIAN";
      break;
    case G_UNICODE_SCRIPT_LYDIAN:
      script_name = "G_UNICODE_SCRIPT_LYDIAN";
      break;
    case G_UNICODE_SCRIPT_AVESTAN:
      script_name = "G_UNICODE_SCRIPT_AVESTAN";
      break;
    case G_UNICODE_SCRIPT_BAMUM:
      script_name = "G_UNICODE_SCRIPT_BAMUM";
      break;
    case G_UNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS:
      script_name = "G_UNICODE_SCRIPT_EGYPTIAN_HIEROGLYPHS";
      break;
    case G_UNICODE_SCRIPT_IMPERIAL_ARAMAIC:
      script_name = "G_UNICODE_SCRIPT_IMPERIAL_ARAMAIC";
      break;
    case G_UNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI:
      script_name = "G_UNICODE_SCRIPT_INSCRIPTIONAL_PAHLAVI";
      break;
    case G_UNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN:
      script_name = "G_UNICODE_SCRIPT_INSCRIPTIONAL_PARTHIAN";
      break;
    case G_UNICODE_SCRIPT_JAVANESE:
      script_name = "G_UNICODE_SCRIPT_JAVANESE";
      break;
    case G_UNICODE_SCRIPT_KAITHI:
      script_name = "G_UNICODE_SCRIPT_KAITHI";
      break;
    case G_UNICODE_SCRIPT_LISU:
      script_name = "G_UNICODE_SCRIPT_LISU";
      break;
    case G_UNICODE_SCRIPT_MEETEI_MAYEK:
      script_name = "G_UNICODE_SCRIPT_MEETEI_MAYEK";
      break;
    case G_UNICODE_SCRIPT_OLD_SOUTH_ARABIAN:
      script_name = "G_UNICODE_SCRIPT_OLD_SOUTH_ARABIAN";
      break;
    default:
      script_name = "UNKNOWN";
    }

    l = g_unichar_to_utf8(ch, str);
    str[l] = '\0';
    sstr = utf8_to_sjis(str);
    printf("%s (%d): %s\n", script_name, script, sstr);
    g_free(sstr);
  }
#endif

  return char_set;
}

static void
fontmap_append(struct gra2emf_local *local, const char *font_name, struct gra2emf_fontmap *fontmap)
{
  struct gra2emf_fontmap *cur;

  if (nhash_get_ptr(local->fontmap, font_name, (void *) &cur)) {
    nhash_set_ptr(local->fontmap, font_name, fontmap);
    return;
  }

  while (cur) {
    if (cur->next == NULL) {
      cur->next = fontmap;
      fontmap->next = NULL;
      break;
    }
    cur = cur->next;
  }
}

static void
check_fonts(struct gra2emf_local *local, HDC hdc, const char *alias, const char *font_name)
{
#if USE_EnumFontFamiliesExW
  LOGFONTW logfont;
#else
  LOGFONT logfont;
#endif
  glong len, size;
  gunichar2 *ustr;
  struct gra2emf_fontmap *fontmap;

  ustr = g_utf8_to_utf16(font_name, -1, NULL, &len, NULL);
  len *= 2;

  logfont.lfCharSet = DEFAULT_CHARSET;
  logfont.lfPitchAndFamily = 0;
  size = (len > LF_FACESIZE - 2) ? LF_FACESIZE - 2: len;
  memcpy(logfont.lfFaceName, ustr, size);
  memset(((char *) logfont.lfFaceName) + size, 0, 2);

  g_free(ustr);

  fontmap = g_malloc0(sizeof(*fontmap));
  if (fontmap == NULL) {
    return;
  }

  fontmap->char_set[0] = -1;
  fontmap->next = NULL;

  fontmap->name = g_strdup(font_name);
  if (fontmap->name == NULL) {
    g_free(fontmap);
    return;
  }

  fontmap_append(local, alias, fontmap);
   /* fontmap_append() should be called before EnumFontFamiliesExW() when compiled with -O2 option */

#if USE_EnumFontFamiliesExW
  EnumFontFamiliesExW(hdc, &logfont, (FONTENUMPROCW) enum_font_cb, (LPARAM) fontmap, 0);
#else
  EnumFontFamiliesEx(hdc, &logfont, (FONTENUMPROC) enum_font_cb, (LPARAM) fontmap, 0);
#endif
}

static int
gra2emf_init(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2emf_local *local = NULL;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  local = g_malloc0(sizeof(struct gra2emf_local));
  if (local == NULL){
    goto errexit;
  }

  local->fontmap = nhash_new();
  local->hdc_dummy = CreateDC("DISPLAY", NULL, NULL, NULL);
  if (local->hdc_dummy == NULL) {
    g_free(local);
    goto errexit;
  }

  if (_putobj(obj, "_local", inst, local)) {
    goto errexit;
  }

  local->null_pen = CreatePen(PS_NULL, 0, RGB(0, 0, 0));
#if USE_LINE_TO
  local->line = 0;
#else
  arrayinit(&local->line, sizeof(int));
#endif

  return 0;

 errexit:
  g_free(local);

  return 1;
}

static void
free_fontmap_sub(NHASH fontmap, const char *name)
{
  struct gra2emf_fontmap *cur, *next;

  if (nhash_get_ptr(fontmap, name, (void *) &cur)) {
    return;
  }

  while (cur) {
    if (cur->name) {
      g_free(cur->name);
    }
    next = cur->next;
    g_free(cur);
    cur = next;
  }

  nhash_del(fontmap, name);
}

static void
free_fontmap(NHASH fontmap)
{
  free_fontmap_sub(fontmap, "Serif");
  free_fontmap_sub(fontmap, "Sans-serif");
  free_fontmap_sub(fontmap, "Monospace");
}

static int
gra2emf_done(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2emf_local *local;

  if (_exeparent(obj, (char *)argv[1], inst, rval, argc, argv)) {
    return 1;
  }

  _getobj(obj, "_local", inst, &local);
  if (local == NULL) {
    return 0;
  }

  if (local->hdc) {
    close_emf(local, NULL);
  }

  if (local->null_pen) {
    DeleteObject(local->null_pen);
  }

  DeleteDC(local->hdc_dummy);

#if USE_LINE_TO
  draw_lines(local);
#else
  arraydel(&local->line);
#endif

  free_fontmap(local->fontmap);
  nhash_free(local->fontmap);

  return 0;
}

static void
add_fontmap(struct gra2emf_local *local, HDC hdc, const char *alias)
{
  struct fontmap *font_map;

  font_map = gra2cairo_get_fontmap(alias);
  if (font_map == NULL) {
    return;
  }

  if (font_map->fontname) {
    check_fonts(local, hdc, alias, font_map->fontname);
  }

  if (font_map->alternative) {
    gchar **alternative;
    int i;

    alternative = g_strsplit(font_map->alternative, ",", 0);
    for (i = 0; alternative[i]; i++) {
      check_fonts(local, hdc, alias, alternative[i]);
    }
    g_strfreev(alternative);
  }
}

static int
open_emf(struct gra2emf_local *local)
{
  HDC hdc;
  XFORM xform = {1, 0, 0, -1, 0, 0};

  hdc = CreateEnhMetaFile(NULL, NULL, NULL, NULL);
  if (hdc == NULL) {
    return 1;
  }

  add_fontmap(local, hdc, "Serif");
  add_fontmap(local, hdc, "Sans-serif");
  add_fontmap(local, hdc, "Monospace");

  StartPage(hdc);
  SaveDC(hdc);
  SetGraphicsMode(hdc, GM_ADVANCED);
  SetMapMode(hdc, MM_HIMETRIC);
  SetWorldTransform(hdc, &xform);
  SetBkMode(hdc, TRANSPARENT);
  SetArcDirection(hdc, AD_COUNTERCLOCKWISE);

  local->update_pen_attribute = TRUE;
  local->update_brush_attribute = TRUE;
  local->the_brush = NULL;
  local->the_pen = NULL;

  local->hdc = hdc;

  return 0;
}

static int
close_emf(struct gra2emf_local *local, const char *fname)
{
  HENHMETAFILE emf;
  int r;
  HPEN pen;
  HBRUSH brush;

  if (local->hdc == NULL) {
    return 1;
  }

  r = 1;

  if (local->the_pen) {
    pen = SelectObject(local->hdc, local->the_pen);
    DeleteObject(pen);
    local->the_pen = NULL;
  }

  if (local->the_brush) {
    brush = SelectObject(local->hdc, local->the_brush);
    DeleteObject(brush);
    local->the_brush = NULL;
  }

  RestoreDC(local->hdc, -1);
  EndPage(local->hdc);
  emf = CloseEnhMetaFile(local->hdc);
  if (emf == NULL) {
    return 1;
  }

  if (fname) {
    HENHMETAFILE emf2;
    emf2 = CopyEnhMetaFile(emf, fname);
    if (emf2) {
      DeleteEnhMetaFile(emf2);
      r = 0;
    }
  } else {
    if (OpenClipboard(NULL)) {
      EmptyClipboard();
      SetClipboardData(CF_ENHMETAFILE, emf);
      CloseClipboard();
      r = 0;
    }
  }
  DeleteEnhMetaFile(emf);
  DeleteDC(local->hdc);
  local->hdc = NULL;

  if (local->fontalias) {
    g_free(local->fontalias);
    local->fontalias = NULL;
  }

  if (local->fontname) {
    g_free(local->fontname);
    local->fontname = NULL;
  }


  free_fontmap(local->fontmap);

  return r;
}

static HFONT
select_font(struct gra2emf_local *local, const char *fontname, int charset)
{
  LOGFONTW id_font;
  gunichar2 *ustr;

  id_font.lfHeight = - local->fontsize;
  id_font.lfWidth = 0;
  id_font.lfEscapement = local->fontdir * 10;
  id_font.lfOrientation = local->fontdir * 10;
  id_font.lfUnderline = 0;
  id_font.lfStrikeOut = 0;
  id_font.lfWeight = (local->font_style & GRA_FONT_STYLE_BOLD) ? FW_BOLD : FW_NORMAL;
  id_font.lfItalic = (local->font_style & GRA_FONT_STYLE_ITALIC) ? TRUE : FALSE;

  if (local->fontalias == NULL) {
    id_font.lfPitchAndFamily = (VARIABLE_PITCH | FF_SWISS);
  } else if (g_ascii_strncasecmp(local->fontalias, "Sans-serif", 5) == 0) {
    id_font.lfPitchAndFamily = (VARIABLE_PITCH | FF_SWISS);
  } else if (g_ascii_strncasecmp(local->fontalias, "Serif", 5) == 0) {
    id_font.lfPitchAndFamily = (VARIABLE_PITCH | FF_ROMAN);
  } else if (g_ascii_strncasecmp(local->fontalias, "Monospace", 7) == 0) {
    id_font.lfPitchAndFamily = (FIXED_PITCH | FF_MODERN);
  } else {
    id_font.lfPitchAndFamily = (VARIABLE_PITCH | FF_SWISS);
  }

  id_font.lfCharSet = charset;
//  id_font.lfOutPrecision = OUT_DEFAULT_PRECIS;
  id_font.lfOutPrecision = OUT_TT_ONLY_PRECIS;
  id_font.lfClipPrecision = CLIP_STROKE_PRECIS;
  id_font.lfQuality = PROOF_QUALITY;

  ustr = g_utf8_to_utf16(fontname, -1, NULL, NULL, NULL);
  wcsncpy(id_font.lfFaceName, ustr, LF_FACESIZE - 1);
  g_free(ustr);

  id_font.lfFaceName[LF_FACESIZE - 1] = L'\0';

  return CreateFontIndirectW(&id_font);
}

void
draw_text_rect(struct gra2emf_local *local, int w, int h)
{
  POINT pos[4];
  HGDIOBJ brush, old_brush;

  pos[0].x = local->x - h * local->fontsin;
  pos[0].y = local->y - h * local->fontcos;

  pos[1].x = local->x;
  pos[1].y = local->y;

  pos[2].x = local->x + w * local->fontcos;
  pos[2].y = local->y - w * local->fontsin;

  pos[3].x = local->x + w * local->fontcos - h * local->fontsin;
  pos[3].y = local->y - w * local->fontsin - h * local->fontcos;

  brush = GetStockObject(NULL_BRUSH);
  old_brush = SelectObject(local->hdc, brush);
  BeginPath(local->hdc);
  Polygon(local->hdc, pos, 4);
  EndPath(local->hdc);
  FillPath(local->hdc);
  SelectObject(local->hdc, old_brush);
}

static void
draw_str_sub(struct gra2emf_local *local, const char *str, const char *fontname, int charset)
{
  gunichar2 *ustr;
  glong len;
  HDC hdc;
  char *utf8_str;
  HFONT font, old_font;
  SIZE str_size;
  UINT align;

  if (str == NULL || str[0] == '\0') {
    return;
  }

  hdc = local->hdc;

  align = TA_BASELINE;
  if (charset == HEBREW_CHARSET || charset == ARABIC_CHARSET) {
    align |= TA_RTLREADING;
  }
  SetTextCharacterExtra(hdc, local->fontspace);
  SetTextColor(hdc, RGB(local->r, local->g, local->b));
  font = select_font(local, fontname, charset);
  old_font = SelectObject(hdc, font);
  SetTextAlign(hdc, align);

  utf8_str = gra2cairo_get_utf8_str(str, local->symbol);
  ustr = g_utf8_to_utf16(utf8_str, -1, NULL, &len, NULL);
  g_free(utf8_str);

  ExtTextOutW(hdc,
	      local->x, local->y,
	      0,
	      NULL,
	      ustr, len,
	      NULL);

  GetTextExtentPoint32W(hdc, ustr, len, &str_size);

  SelectObject(hdc, old_font);
  DeleteObject(font);

  draw_text_rect(local, str_size.cx, str_size.cy);

  local->x += str_size.cx * local->fontcos;
  local->y -= str_size.cx * local->fontsin;

  g_free(ustr);
}

static const char *
check_font_indices(struct gra2emf_local *local, gunichar ch)
{
  WORD indices[2];
  HFONT font, old_font;
  gunichar str[2];
  gunichar2 *ustr;
  struct gra2emf_fontmap *cur;
  DWORD r;

  str[0] = ch;
  str[1] = 0;

  if (local->fontalias == NULL) {
    return NULL;
  }

  if (nhash_get_ptr(local->fontmap, local->fontalias, (void *) &cur)) {
    return NULL;
  }

  ustr = g_ucs4_to_utf16(str, 1, NULL, NULL, NULL);
  if (ustr == NULL) {
    return NULL;
  }

  while (cur) {
    font = select_font(local, cur->name, ANSI_CHARSET);
    old_font = SelectObject(local->hdc_dummy, font);
    r = GetGlyphIndicesW(local->hdc_dummy, ustr, 1, indices, GGI_MARK_NONEXISTING_GLYPHS);
    SelectObject(local->hdc_dummy, old_font);
    DeleteObject(font);

    if (r != GDI_ERROR && indices[0] != 0xffff) {
      g_free(ustr);
      return cur->name;
    }
    cur = cur->next;
  }

  g_free(ustr);
  return NULL;
}

static void
draw_str(struct gra2emf_local *local, const char *str)
{
  gunichar ch;
  const char *ptr;
  GString *sub_str;
  const char *font, *prev_font;
  int prev_charset;

  sub_str = g_string_new("");
  prev_font = NULL;
  prev_charset = DEFAULT_CHARSET;

  for (ptr = str; *ptr; ptr = g_utf8_next_char(ptr)) {
    int charset;
    ch = g_utf8_get_char(ptr);
    charset = get_char_set(ch);
    font = check_font_indices(local, ch);
    if (font == NULL) {
      font = "Arial";
    }

    if (prev_font == NULL) {
      prev_charset = charset;
      prev_font = font;
    } else if (prev_font != font) {
      draw_str_sub(local, sub_str->str, prev_font, prev_charset);
      g_string_truncate(sub_str, 0);
      prev_font = font;
      prev_charset = charset;
    }

    if (prev_charset < charset) {
      prev_charset = charset;
    }

    g_string_append_unichar(sub_str, ch);
  }
  draw_str_sub(local, sub_str->str, prev_font, prev_charset);
  g_string_free(sub_str, TRUE);
}

static int
gra2emf_flush(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  struct gra2emf_local *local;

  _getobj(obj, "_local", inst, &local);

  if (local->hdc == NULL) {
    return -1;
  }

  return 0;
}

static void
create_pen(struct gra2emf_local *local)
{
  HPEN old;
  DWORD line_attr;
  LOGBRUSH log_brush;
  HPEN pen;

  if (! local->update_pen_attribute) {
    return;
  }

  line_attr = PS_GEOMETRIC;
  switch (local->line_cap) {
  case 2:
    line_attr |= PS_ENDCAP_SQUARE;
    break;
  case 1:
    line_attr |= PS_ENDCAP_ROUND;
    break;
  default:
    line_attr |= PS_ENDCAP_FLAT;
  }

  switch (local->line_join) {
  case 2:
    line_attr |= PS_JOIN_BEVEL;
    break;
  case 1:
    line_attr |= PS_JOIN_ROUND;
    break;
  default:
    line_attr |= PS_JOIN_MITER;
  }

  if (local->line_style) {
    line_attr |= PS_USERSTYLE;
  } else {
    line_attr |= PS_SOLID;
  }
  log_brush.lbStyle = BS_SOLID;
  log_brush.lbColor = RGB(local->r, local->g, local->b);
  pen = ExtCreatePen(line_attr, local->line_width, &log_brush, local->line_style_num, local->line_style);
  old = SelectObject(local->hdc, pen);
  if (local->the_pen) {
    DeleteObject(old);
  } else {
    local->the_pen = old;
  }
  local->update_pen_attribute = FALSE;
}

static void
create_brush(struct gra2emf_local *local)
{
  HBRUSH brush, old_brush;

  if (! local->update_brush_attribute) {
    return;
  }

  brush = CreateSolidBrush(RGB(local->r, local->g, local->b));
  old_brush = SelectObject(local->hdc, brush);
  if (local->the_brush) {
    DeleteObject(old_brush);
  } else {
    local->the_brush = old_brush;
  }
  local->update_brush_attribute = FALSE;
}

static void
draw_arc(struct gra2emf_local *local, int x, int y, int w, int h, int start, int angle, int style)
{
  double a1, a2;
  HPEN old_pen;

  if (angle == 0) {
    return;
  }

  if (angle % 36000 == 0 && (style == 1 || style == 2)) {
    create_brush(local);
    old_pen = SelectObject(local->hdc, local->null_pen);
    Ellipse(local->hdc,
	    x - w, y - h,
	    x + w, y + h);
    SelectObject(local->hdc, old_pen);
    return;
  }

  a1 = start * (MPI / 18000.0);
  a2 = angle * (MPI / 18000.0) + a1;

  switch (style) {
  case 1:
    create_brush(local);
    old_pen = SelectObject(local->hdc, local->null_pen);
    Pie(local->hdc,
	x - w, y - h,
	x + w, y + h,
	x + w * cos(a1),
	y - h * sin(a1),
	x + w * cos(a2),
	y - h * sin(a2)
	);
    SelectObject(local->hdc, old_pen);
    break;
  case 2:
    create_brush(local);
    old_pen = SelectObject(local->hdc, local->null_pen);
    Chord(local->hdc,
	  x - w, y - h,
	  x + w, y + h,
	  x + w * cos(a1),
	  y - h * sin(a1),
	  x + w * cos(a2),
	  y - h * sin(a2));
    SelectObject(local->hdc, old_pen);
    break;
  case 3:
    create_pen(local);
    MoveToEx(local->hdc, x, y, NULL);
    BeginPath(local->hdc);
    ArcTo(local->hdc,
	  x - w, y - h,
	  x + w, y + h,
	  x + w * cos(a1),
	  y - h * sin(a1),
	  x + w * cos(a2),
	  y - h * sin(a2)
	  );
    CloseFigure(local->hdc);
    EndPath(local->hdc);
    StrokePath(local->hdc);
    break;
  case 4:
    create_pen(local);
    BeginPath(local->hdc);
    Arc(local->hdc,
	x - w, y - h,
	x + w, y + h,
	x + w * cos(a1),
	y - h * sin(a1),
	x + w * cos(a2),
	y - h * sin(a2)
	);
    CloseFigure(local->hdc);
    EndPath(local->hdc);
    StrokePath(local->hdc);
    break;
  default:
    create_pen(local);
    Arc(local->hdc,
	x - w, y - h,
	x + w, y + h,
	x + w * cos(a1),
	y - h * sin(a1),
	x + w * cos(a2),
	y - h * sin(a2)
	);
    break;
  }
}

static void
set_alternative_font(struct gra2emf_local *local)
{
  struct compatible_font_info *info;

  if (local->fontalias) {
    g_free(local->fontalias);
  }
  local->fontalias = NULL;

  if (local->fontname == NULL || local->fontname[0] == '\0') {
    return;
  }

  info = gra2cairo_get_compatible_font_info(local->fontname);
  if (info == NULL) {
    local->fontalias = g_strdup(local->fontname);
    local->symbol = FALSE;
    return;
  }

  local->fontalias = g_strdup(info->name);
  local->font_style = info->style;
  local->symbol = info->symbol;
}

static void
draw_rectangle(struct gra2emf_local *local, int x1, int y1, int x2, int y2, int fill)
{
  x1 += local->offsetx;
  y1 += local->offsety;
  x2 += local->offsetx;
  y2 += local->offsety;

#if USE_LINE_TO
  if (fill) {
    create_brush(local);
  } else {
    create_pen(local);
  }

  BeginPath(local->hdc);
  MoveToEx(local->hdc, x1, y1, NULL);
  LineTo(local->hdc, x1, y2);
  LineTo(local->hdc, x2, y2);
  LineTo(local->hdc, x2, y1);
  CloseFigure(local->hdc);
  EndPath(local->hdc);

  if (fill) {
    FillPath(local->hdc);
  } else {
    StrokePath(local->hdc);
  }
#else  /* USE_LINE_TO */
  if (fill) {
    HPEN old_pen;

    create_brush(local);
    old_pen = SelectObject(local->hdc, local->null_pen);
    Rectangle(local->hdc, x1, y1, x2, y2);
    SelectObject(local->hdc, old_pen);
  } else {
    HBRUSH old_brush;

    create_pen(local);
    old_brush = SelectObject(local->hdc, GetStockObject(NULL_BRUSH));
    Rectangle(local->hdc, x1, y1, x2, y2);
    SelectObject(local->hdc, old_brush);
  }
#endif	/* USE_LINE_TO */
}

static void
draw_polygon(struct gra2emf_local *local, int n, const int *points, int fill)
{
  int i;
#if USE_LINE_TO
  if (n < 2) {
    return;
  }

  switch (fill) {
  case 0:
    create_pen(local);
    break;
  case 1:
    create_brush(local);
    SetPolyFillMode(local->hdc, ALTERNATE);
    break;
  case 2:
    create_brush(local);
    SetPolyFillMode(local->hdc, WINDING);
    break;
  }

  BeginPath(local->hdc);
  MoveToEx(local->hdc, points[0] + local->offsetx, points[1] + local->offsety, NULL);
  for (i = 1; i < n; i++) {
    LineTo(local->hdc,
	   points[i * 2 + 0] + local->offsetx,
	   points[i * 2 + 1] + local->offsety);
  }
  CloseFigure(local->hdc);
  EndPath(local->hdc);

  switch (fill) {
  case 0:
    StrokePath(local->hdc);
    break;
  case 1:
  case 2:
    FillPath(local->hdc);
    break;
  }
#else  /* USE_LINE_TO */
  HPEN old_pen;
  HBRUSH old_brush;
  POINT *pos;

  if (n < 2) {
    return;
  }

  pos = g_malloc(sizeof(*pos) * n);
  if (pos == NULL) {
    return;
  }

  for (i = 0; i < n; i++) {
    pos[i].x = points[i * 2] + local->offsetx;
    pos[i].y = points[i * 2 + 1] + local->offsety;
  }

  switch (fill) {
  case 0:
    create_pen(local);
    old_brush = SelectObject(local->hdc, GetStockObject(NULL_BRUSH));
    Polygon(local->hdc, pos, n);
    SelectObject(local->hdc, old_brush);
    break;
  case 1:
    create_brush(local);
    SetPolyFillMode(local->hdc, ALTERNATE);
    old_pen = SelectObject(local->hdc, local->null_pen);
    Polygon(local->hdc, pos, n);
    SelectObject(local->hdc, old_pen);
    break;
  case 2:
    create_brush(local);
    SetPolyFillMode(local->hdc, WINDING);
    old_pen = SelectObject(local->hdc, local->null_pen);
    Polygon(local->hdc, pos, n);
    SelectObject(local->hdc, old_pen);
    break;
  }
#endif	/* USE_LINE_TO */
}

static void
draw_polyline(struct gra2emf_local *local, int n, const int *points)
{
  int i;
#if USE_LINE_TO
  if (n < 2) {
    return;
  }

  create_pen(local);
  BeginPath(local->hdc);
  MoveToEx(local->hdc, points[0] + local->offsetx, points[1] + local->offsety, NULL);
  for (i = 1; i < n; i++) {
    LineTo(local->hdc,
	   points[i * 2 + 0] + local->offsetx,
	   points[i * 2 + 1] + local->offsety);
  }
  EndPath(local->hdc);
  StrokePath(local->hdc);
#else  /* USE_LINE_TO */
  POINT *pos;

  if (n < 2) {
    return;
  }

  pos = g_malloc(sizeof(*pos) * n);
  if (pos == NULL) {
    return;
  }

  for (i = 0; i < n; i++) {
    pos[i].x = points[i * 2 + 0] + local->offsetx;
    pos[i].y = points[i * 2 + 1] + local->offsety;
  }

  create_pen(local);
  Polyline(local->hdc, pos, n);
#endif	/* USE_LINE_TO */
}

static void
draw_lines(struct gra2emf_local *local)
{
#if USE_LINE_TO
  if (local->line > 0) {
    EndPath(local->hdc);
    StrokePath(local->hdc);
  }
  local->line = 0;
#else
  POINT *pos;
  int i, n, *data;

  n = arraynum(&local->line) / 2;
  if (n < 2) {
    arrayclear(&local->line);
    return;
  }

  data = arraydata(&local->line);

  pos = g_malloc(sizeof(*pos) * n);
  if (pos == NULL) {
    return;
  }

  for (i = 0; i < n; i++) {
    pos[i].x = data[i * 2];
    pos[i].y = data[i * 2 + 1];
  }

  create_pen(local);
  Polyline(local->hdc, pos, n);

  arrayclear(&local->line);
#endif
}

static int
gra2emf_output(struct objlist *obj, N_VALUE *inst, N_VALUE *rval, int argc, char **argv)
{
  char code, *cstr, *tmp, *fname;
  int *cpar, r;
  double x, y, w, h, fontdir;
  struct gra2emf_local *local;
  POINT lpoint[2];

  local = (struct gra2emf_local *)argv[2];
  code = *(char *)(argv[3]);
  cpar = (int *)argv[4];
  cstr = argv[5];

  if (code != 'I' && local->hdc == NULL) {
    return 1;
  }

  if (code != 'T') {
    draw_lines(local);
  }

  switch (code) {
  case 'I':
    if (open_emf(local)) {
      error(obj, ERREMF);
      return 1;
    }
    break;
  case '%': case 'X': case 'Z':
    break;
  case 'E':
    r = 0;
    _getobj(obj, "file", inst, &fname);
    if (fname) {
      fname = g_win32_locale_filename_from_utf8(fname);
    }
    r = close_emf(local, fname);
    if (fname) {
      g_free(fname);
    }
    if (r) {
      error(obj, ERREMF);
      return 1;
    }
    break;
  case 'V':
    local->offsetx = cpar[1];
    local->offsety = cpar[2];
    if (cpar[5]) {
      BeginPath(local->hdc);
      Rectangle(local->hdc,cpar[1], cpar[2], cpar[3], cpar[4]);
      EndPath(local->hdc);
      SelectClipPath(local->hdc, RGN_COPY);
    } else {
      SelectClipRgn(local->hdc, NULL);
    }
    break;
  case 'A':
    if (local->line_style) {
      g_free(local->line_style);
    }
    local->line_style = NULL;
    local->line_style_num = 0;
    if (cpar[1]) {
      int i;
      local->line_style = g_malloc(sizeof(* (local->line_style)) * cpar[1]);
      if (local->line_style == NULL) {
	break;
      }
      for (i = 0; i < cpar[1]; i++) {
	local->line_style[i] = cpar[6 + i];
      }
      local->line_style_num = cpar[1];
    }
    local->line_width = cpar[2];
    local->line_cap = cpar[3];
    local->line_join = cpar[4];
    SetMiterLimit(local->hdc, cpar[5] / 100.0, NULL);
    local->update_pen_attribute = TRUE;
    break;
  case 'G':
    local->r = cpar[1];
    local->g = cpar[2];
    local->b = cpar[3];
    local->update_pen_attribute = TRUE;
    local->update_brush_attribute = TRUE;
    break;
  case 'M':
    local->x = cpar[1] + local->offsetx;
    local->y = cpar[2] + local->offsety;
    break;
  case 'N':
    local->x += cpar[1];
    local->y += cpar[2];
    break;
  case 'L':
    lpoint[0].x = cpar[1] + local->offsetx;
    lpoint[0].y = cpar[2] + local->offsety;
    lpoint[1].x = cpar[3] + local->offsetx;
    lpoint[1].y = cpar[4] + local->offsety;
    create_pen(local);
    Polyline(local->hdc, lpoint, 2);
    break;
  case 'T':
#if USE_LINE_TO
    if (local->line == 0) {
      create_pen(local);
      BeginPath(local->hdc);
      MoveToEx(local->hdc, local->x, local->y, NULL);
    }
    local->x = cpar[1] + local->offsetx;
    local->y = cpar[2] + local->offsety;
    LineTo(local->hdc, local->x, local->y);
    local->line++;
#else  /* USE_LINE_TO */
    /*
      it seems that the function LineTo() cannot handle dotted line
      correctly when the points exist very closely.
    */
    if (arraynum(&local->line) < 1) {
      arrayclear(&local->line);
      arrayadd(&local->line, &local->x);
      arrayadd(&local->line, &local->y);
    }
    local->x = cpar[1] + local->offsetx;
    local->y = cpar[2] + local->offsety;
    arrayadd(&local->line, &local->x);
    arrayadd(&local->line, &local->y);
#endif	/* USE_LINE_TO */
    break;
  case 'C':
    x = cpar[1] + local->offsetx;
    y = cpar[2] + local->offsety;
    w = cpar[3];
    h = cpar[4];
    draw_arc(local, x, y, w, h, cpar[5], cpar[6], cpar[7]);
    break;
  case 'B':
    draw_rectangle(local, cpar[1], cpar[2], cpar[3], cpar[4], cpar[5]);
    break;
  case 'P':
    SetPixel(local->hdc,
	     cpar[1] + local->offsetx, cpar[2] + local->offsety,
	     RGB(local->r, local->g, local->b));
    break;
  case 'R':
    draw_polyline(local, cpar[1], cpar + 2);
    break;
  case 'D':
    draw_polygon(local, cpar[1], cpar + 3, cpar[2]);
    break;
  case 'F':
    if (local->fontname) {
      g_free(local->fontname);
    }
    local->fontname = g_strdup(cstr);
    break;
  case 'H':
    local->fontspace = cpar[2] / 72.0 * 25.4;
    local->fontsize = cpar[1] / 72.0 * 25.4;
    fontdir = cpar[3] * (MPI / 18000.0);
    local->fontdir = (cpar[3] % 36000) / 100.0;
    if (local->fontdir < 0) {
      local->fontdir += 360;
    }
    local->fontsin = sin(fontdir);
    local->fontcos = cos(fontdir);
    local->font_style = (cpar[0] > 3) ? cpar[4] : 0;
    set_alternative_font(local);
    break;
  case 'S':
    draw_str(local, argv[5]);
    break;
  case 'K':
    tmp = sjis_to_utf8(cstr);
    if (tmp) {
      draw_str(local, tmp);
      g_free(tmp);
    }
    break;
  default:
    break;
  }
  return 0;
}

static struct objtable gra2emf[] = {
  {"init",    NVFUNC, NEXEC, gra2emf_init, NULL, 0},
  {"done",    NVFUNC, NEXEC, gra2emf_done, NULL, 0},
  {"next",    NPOINTER, 0, NULL, NULL, 0},
  {"file",    NSTR, NREAD | NWRITE, NULL, NULL,0},
  {"flush",   NVFUNC,NREAD|NEXEC, gra2emf_flush,"",0},
  {"_output", NVFUNC, 0, gra2emf_output, NULL, 0},
  {"_local",  NPOINTER, 0, NULL, NULL, 0},
};

#define TBLNUM (sizeof(gra2emf) / sizeof(*gra2emf))

void *
addgra2emf(void)
/* addgra2emfile() returns NULL on error */
{
  return addobject(NAME, NULL, PARENT, OVERSION, TBLNUM, gra2emf, ERRNUM, gra2emf_errorlist, NULL, NULL);
}

#endif	/* WINDOWS */

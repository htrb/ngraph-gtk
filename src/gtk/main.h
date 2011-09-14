#ifndef _MAIN_HEADER
#define _MAIN_HEADER

extern char *AppName, *AppClass, *Auther[], *Translator, *Documenter[];
#if ! GTK_CHECK_VERSION(3, 0, 0)
extern char *License;
#endif

int OpenApplication(void);
int nallocconsole(void);
void nfreeconsole(void);
void nforegroundconsole(void);
void hide_console(void);
void resotre_console(void);

#endif

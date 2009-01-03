#ifndef _MAIN_HEADER
#define _MAIN_HEADER

extern char *AppName, *AppClass, *License, *Auther[], *Translator, *Documenter[];

int OpenApplication(void);
int nallocconsole(void);
void nfreeconsole(void);
void nforegroundconsole(void);

#endif

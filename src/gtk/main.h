#ifndef _MAIN_HEADER
#define _MAIN_HEADER

extern char *AppName, *AppClass, *Auther[], *Translator, *Documenter[];
#if ! GTK_CHECK_VERSION(3, 0, 0)
extern char *License;
#endif

#define NGRAPH_GRAPH_MIME "application/x-ngraph"
#define NGRAPH_DATA_MIME  "text/plain"
#define NGRAPH_TEXT_MIME  "text/"

int OpenApplication(void);
int nallocconsole(void);
void nfreeconsole(void);
void nforegroundconsole(void);
void hide_console(void);
void resotre_console(void);

#endif

#ifndef _INIT_HEADER
#define _INIT_HEADER

extern char *AppName, *AppClass, *Auther[], *Translator, *Documenter[], *License;

#define NGRAPH_GRAPH_MIME "application/x-ngraph"
#define NGRAPH_DATA_MIME  "text/plain"
#define NGRAPH_TEXT_MIME  "text/"

int OpenApplication(void);
int nallocconsole(void);
void nfreeconsole(void);
void nforegroundconsole(void);
void hide_console(void);
void resotre_console(void);
const char *n_getlocale(void);

int n_initialize(int *argc, char ***argv);
void n_save_shell_history(void);
void n_finalize(void);

#endif

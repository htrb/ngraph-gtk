#ifndef OSYSTEM_HEADER
#define OSYSTEM_HEADER
typedef void (* DRAW_NOTIFY_FUNC)(int);
int system_set_exec_func(const char *name, ngraph_plugin_exec func);
void system_set_draw_notify_func(DRAW_NOTIFY_FUNC func);
void system_draw_notify(void);
#endif

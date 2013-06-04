#ifndef GTK_COMMON_HEADER
#define GTK_COMMON_HEADER

#include <gtk/gtk.h>
#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <glib.h>
#include <cairo/cairo.h>

#include "common.h"

#ifndef GTK_WIDGET_VISIBLE
#define GTK_WIDGET_VISIBLE(w) gtk_widget_get_visible(w)
#endif

#if GTK_CHECK_VERSION(2, 18, 0)
#define GTK_WIDGET_SET_CAN_FOCUS(w) gtk_widget_set_can_focus(w, TRUE)
#else
#define GTK_WIDGET_SET_CAN_FOCUS(w) GTK_WIDGET_SET_FLAGS(w, GTK_CAN_FOCUS)
#endif

#if GTK_CHECK_VERSION(3, 2, 0)
#define gtk_hbox_new(h, s)      gtk_box_new(GTK_ORIENTATION_HORIZONTAL, s)
#define gtk_vbox_new(h, s)      gtk_box_new(GTK_ORIENTATION_VERTICAL, s)
#define gtk_hbutton_box_new()   gtk_button_box_new(GTK_ORIENTATION_HORIZONTAL)
#define gtk_hseparator_new()    gtk_separator_new(GTK_ORIENTATION_HORIZONTAL)
#define gtk_hscrollbar_new(adj) gtk_scrollbar_new(GTK_ORIENTATION_HORIZONTAL, adj)
#define gtk_vscrollbar_new(adj) gtk_scrollbar_new(GTK_ORIENTATION_VERTICAL, adj)
#define gtk_hscale_new_with_range(min, max, step) gtk_scale_new_with_range(GTK_ORIENTATION_HORIZONTAL, min, max, step)
#endif

#define CAIRO_COORDINATE_OFFSET 1

#ifndef GDK_KEY_BackSpace
#define GDK_KEY_BackSpace GDK_BackSpace
#define GDK_KEY_Control_L GDK_Control_L
#define GDK_KEY_Control_R GDK_Control_R
#define GDK_KEY_Delete GDK_Delete
#define GDK_KEY_Down GDK_Down
#define GDK_KEY_End GDK_End
#define GDK_KEY_Escape GDK_Escape
#define GDK_KEY_F GDK_F
#define GDK_KEY_F1 GDK_F1
#define GDK_KEY_F3 GDK_F3
#define GDK_KEY_F4 GDK_F4
#define GDK_KEY_F5 GDK_F5
#define GDK_KEY_F6 GDK_F6
#define GDK_KEY_F7 GDK_F7
#define GDK_KEY_F8 GDK_F8
#define GDK_KEY_Home GDK_Home
#define GDK_KEY_Insert GDK_Insert
#define GDK_KEY_Left GDK_Left
#define GDK_KEY_Page_Down GDK_Page_Down
#define GDK_KEY_Page_Up GDK_Page_Up
#define GDK_KEY_Return GDK_Return
#define GDK_KEY_Right GDK_Right
#define GDK_KEY_Shift_L GDK_Shift_L
#define GDK_KEY_Shift_R GDK_Shift_R
#define GDK_KEY_Up GDK_Up
#define GDK_KEY_c GDK_c
#define GDK_KEY_d GDK_d
#define GDK_KEY_e GDK_e
#define GDK_KEY_f GDK_f
#define GDK_KEY_g GDK_g
#define GDK_KEY_o GDK_o
#define GDK_KEY_p GDK_p
#define GDK_KEY_plus GDK_plus
#define GDK_KEY_q GDK_q
#define GDK_KEY_r GDK_r
#define GDK_KEY_s GDK_s
#define GDK_KEY_space GDK_space
#define GDK_KEY_v GDK_v
#define GDK_KEY_w GDK_w
#define GDK_KEY_x GDK_x
#endif	/* GDK_KEY_BackSpace */

#endif


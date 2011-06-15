#include <audacious/i18n.h>
#include <libaudgui/libaudgui-gtk.h>
#include <gtk/gtk.h>
#include <string.h>
#include "config.h"

void fc_ip_about() {
    static GtkWidget *about_window = NULL;

    audgui_simple_message(&about_window, GTK_MESSAGE_INFO,
                          g_strdup_printf(_("Future Composer player plugin %s"), VERSION),_("http://xmms-fc.sourceforge.net\nCreated by Michael Schwendt.\n\nMuch room for improvement.\nHave a go if you like to.\n"));
}

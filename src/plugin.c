#include <glib.h>
#include <audacious/plugin.h>
#include "config.h"

static const char fc_ip_about[] =
   "Future Composer decoder plug-in\n\n"
   "http://xmms-fc.sourceforge.net\nCreated by Michael Schwendt.\n\n"
   "Much room for improvement.\nHave a go if you like to.\n";

const gchar* const fc_fmts[] = { "fc", "fc13", "fc14", NULL };

extern PluginPreferences fc_ip_preferences;

gboolean ip_init(void);
gint ip_is_valid_file_vfs(const gchar *filename, VFSFile *fd);
gboolean ip_play(const gchar *filename, VFSFile *fd);
Tuple *ip_probe_for_tuple(const gchar *filename, VFSFile *fd);

AUD_INPUT_PLUGIN
(
    .name = "Future Composer Decoder " VERSION,
    .init = ip_init,
    .about_text = fc_ip_about,
    .prefs = &fc_ip_preferences,
    .have_subtune = FALSE,
    .extensions = fc_fmts,
    .play = ip_play,
    .probe_for_tuple = ip_probe_for_tuple,
    .is_our_file_from_vfs = ip_is_valid_file_vfs
)

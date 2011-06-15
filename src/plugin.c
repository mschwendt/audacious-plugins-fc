#include <audacious/plugin.h>

const gchar* const fc_fmts[] = { "fc", "fc13", "fc14", NULL };

extern PluginPreferences fc_ip_preferences;

gboolean ip_init(void);
void ip_cleanup(void);
void fc_ip_about(void);
gint ip_is_valid_file_vfs(const gchar *filename, VFSFile *fd);
gboolean ip_play(InputPlayback *playback, const gchar *filename, VFSFile *fd,
                 gint start_time, gint stop_time, gboolean pause);
void ip_stop(InputPlayback *playback);
void ip_pause(InputPlayback *playback, gboolean p);
void ip_seek(InputPlayback *playback, gint secs);
void ip_mseek(InputPlayback *playback, gint msec);
Tuple *ip_probe_for_tuple(const gchar *filename, VFSFile *fd);

AUD_INPUT_PLUGIN
(
    .name = "Future Composer decoder",
    .init = ip_init,
    .about = fc_ip_about,
    .settings = &fc_ip_preferences,
    .have_subtune = FALSE,
    .extensions = fc_fmts,
    .play = ip_play,
    .stop = ip_stop,
    .pause = ip_pause,
    .mseek = ip_mseek,
    .cleanup = ip_cleanup,
    .probe_for_tuple = ip_probe_for_tuple,
    .is_our_file_from_vfs = ip_is_valid_file_vfs
)

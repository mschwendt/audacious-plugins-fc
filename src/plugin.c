#include <audacious/plugin.h>

const gchar* const fc_fmts[] = { "fc", "fc13", "fc14", NULL };

gboolean ip_init(void);
void ip_cleanup(void);
void fc_ip_about(void);
void fc_ip_configure(void);
gint ip_is_valid_file_vfs(const gchar *filename, VFSFile *fd);
gboolean ip_play(InputPlayback *playback, const gchar *filename, VFSFile *fd,
                 gint start_time, gint stop_time, gboolean pause);
void ip_stop(InputPlayback *playback);
void ip_pause(InputPlayback *playback, gboolean p);
void ip_seek(InputPlayback *playback, gint secs);
void ip_mseek(InputPlayback *playback, gint msec);
Tuple *ip_probe_for_tuple(const gchar *filename, VFSFile *fd);

InputPlugin iplugin =
{
    .description = "Future Composer decoder",
    .init = ip_init,
    .about = fc_ip_about,
    .configure = fc_ip_configure,
    .have_subtune = FALSE,
    .vfs_extensions = fc_fmts,
    .play = ip_play,
    .stop = ip_stop,
    .pause = ip_pause,
    .mseek = ip_mseek,
    .cleanup = ip_cleanup,
    .probe_for_tuple = ip_probe_for_tuple,
    .is_our_file_from_vfs = ip_is_valid_file_vfs
};

InputPlugin *fc_iplist[] = { &iplugin, NULL };

SIMPLE_INPUT_PLUGIN(libfc, fc_iplist);

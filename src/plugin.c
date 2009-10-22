#include <audacious/plugin.h>

gchar *fc_fmts[] = { "fc", "fc13", "fc14", NULL };

void ip_init(void);
void ip_cleanup(void);
void fc_ip_about(void);
void fc_ip_configure(void);
gint ip_is_valid_file_vfs(const gchar *fileName, VFSFile *fd);
void ip_play_file(InputPlayback *playback);
void ip_stop(InputPlayback *playback);
void ip_pause(InputPlayback *playback, gshort p);
void ip_seek(InputPlayback *playback, gint secs);
void ip_mseek(InputPlayback *playback, gulong msec);
Tuple* ip_get_song_tuple(const gchar *filename);

InputPlugin iplugin =
{
    .description = "Future Composer Player",
    .init = ip_init,
    .about = fc_ip_about,
    .configure = fc_ip_configure,
    .enabled = TRUE,
    .have_subtune = FALSE,
    .vfs_extensions = (gchar**)fc_fmts,
    .play_file = ip_play_file,
    .stop = ip_stop,
    .pause = ip_pause,
    .seek = ip_seek,
    .mseek = ip_mseek,
    .cleanup = ip_cleanup,
    .get_song_tuple = ip_get_song_tuple,
    .is_our_file_from_vfs = ip_is_valid_file_vfs
};

InputPlugin *fc_iplist[] = { &iplugin, NULL };

SIMPLE_INPUT_PLUGIN(libfc, fc_iplist);

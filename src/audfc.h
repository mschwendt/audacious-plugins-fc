#include <libaudcore/plugin.h>
#include <libaudcore/preferences.h>

#include "config.h"

class AudFC : public InputPlugin {
 public:
    static const char about[];
    static const char *const exts[];
    static const char *const defaults[];
    static const PreferencesWidget widgets[];
    static const PluginPreferences prefs;

    static constexpr PluginInfo info = {
        "Future Composer Decoder " VERSION,
        PACKAGE,
        about,
        &prefs
    };

    static constexpr auto iinfo = InputInfo()
        .with_exts (exts);

    constexpr AudFC() : InputPlugin(info,iinfo) {
    }

    bool init();
    bool is_our_file(const char *filename, VFSFile &file);
    Tuple read_tuple(const char *filename, VFSFile &file);
    bool play(const char *filename, VFSFile &file);
};

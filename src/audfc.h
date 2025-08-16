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
        "FC & Hippel Decoder",
        PACKAGE,
        about,
        &prefs
    };

    constexpr AudFC() : InputPlugin(info, InputInfo(FlagSubtunes)
        .with_exts (exts)) {}

    bool init();
    bool is_our_file(const char *filename, VFSFile &file);
    bool read_tag(const char *filename, VFSFile &file, Tuple &tuple, Index<char> * image);
    bool play(const char *filename, VFSFile &file);
};

#include <libaudcore/plugin.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include <cstring>

#include "audfc.h"
#include "configure.h"

FCpluginConfig fc_myConfig;

static char const configSection[] = "FutureComposer";

static const int FREQ_SAMPLE_48 = 48000;
static const int FREQ_SAMPLE_44 = 44100;
static const int FREQ_SAMPLE_22 = 22050;

void fc_ip_eval_config() {
    fc_myConfig.frequency = aud_get_int(configSection, "frequency");
    fc_myConfig.precision = aud_get_int(configSection, "precision");
    fc_myConfig.channels = aud_get_int(configSection, "channels");
    fc_myConfig.panning = aud_get_int(configSection, "panning");
    fc_myConfig.endshorts = aud_get_bool(configSection, "endshorts");
    fc_myConfig.maxsecs = aud_get_int(configSection, "maxsecs");
}

void fc_ip_load_config() {
    aud_config_set_defaults(configSection,AudFC::defaults);
    fc_ip_eval_config();
}

static void configure_apply() {
    fc_ip_eval_config();
}

static const PreferencesWidget frequency_widgets[] = {
    WidgetLabel("<b>Frequency</b> [Hz]:"),
    WidgetRadio("48000", WidgetInt(configSection,"frequency",&configure_apply), {48000}),
    WidgetRadio("44100", WidgetInt(configSection,"frequency",&configure_apply), {44100}),
    WidgetRadio("22050", WidgetInt(configSection,"frequency",&configure_apply), {22050}),
};

static const PreferencesWidget precision_widgets[] = {
    WidgetLabel("<b>Precision</b> [bits]:"),
    WidgetRadio("16", WidgetInt(configSection,"precision",&configure_apply), {16}),
    WidgetRadio("8", WidgetInt(configSection,"precision",&configure_apply), {8}),
};

static const PreferencesWidget channels_widgets[] = {
    WidgetLabel("<b>Channels</b>:"),
    WidgetRadio("Stereo", WidgetInt(configSection,"channels",&configure_apply), {2}),
    WidgetRadio("Mono", WidgetInt(configSection,"channels",&configure_apply), {1}),
};

static const PreferencesWidget widget_columns[] = {
    WidgetBox({{frequency_widgets}}),
    WidgetSeparator(),
    WidgetBox({{precision_widgets}}),
    WidgetSeparator(),
    WidgetBox({{channels_widgets}})
};

const PreferencesWidget AudFC::widgets[] = {
    WidgetBox({{widget_columns}, true}),

    WidgetLabel("<b>Stereo Panning</b>:"),
    WidgetSpin("",WidgetInt(configSection,"panning",&configure_apply), {0, 100, 5, "100=stereo, 50=middle, 0=mirrored stereo"}),

    WidgetLabel("<b>Short tracks</b>:"),
    WidgetCheck("End immediately if shorter than",WidgetBool(configSection,"endshorts",&configure_apply)),
    WidgetSpin("",WidgetInt(configSection,"maxsecs",&configure_apply), {5, 30, 1, "seconds"}),

    WidgetLabel ("<b>Note</b>: These settings will take effect when restarting playback.")};

const PluginPreferences AudFC::prefs = {{widgets}};

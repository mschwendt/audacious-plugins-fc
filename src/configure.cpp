#include <libaudcore/plugin.h>
#include <libaudcore/preferences.h>
#include <libaudcore/runtime.h>

#include <cstring>

#include "audfc.h"
#include "configure.h"

FCpluginConfig fc_myConfig;
FCpluginConfig config;

static char const configSection[] = "FutureComposer";

static const int FREQ_SAMPLE_48 = 48000;
static const int FREQ_SAMPLE_44 = 44100;
static const int FREQ_SAMPLE_22 = 22050;

void fc_ip_load_config() {
    aud_config_set_defaults(configSection,AudFC::defaults);

    fc_myConfig.frequency = aud_get_int(configSection, "frequency");
    fc_myConfig.precision = aud_get_int(configSection, "precision");
    fc_myConfig.channels = aud_get_int(configSection, "channels");

    fc_myConfig.freq48 = fc_myConfig.freq44 = fc_myConfig.freq22 = false;
    fc_myConfig.bits16 = fc_myConfig.bits8 = false;
    fc_myConfig.mono = fc_myConfig.stereo = false;

    if (fc_myConfig.frequency == FREQ_SAMPLE_48) {
        fc_myConfig.freq48 = true;
    }
    else if (fc_myConfig.frequency == FREQ_SAMPLE_22) {
        fc_myConfig.freq22 = true;
    }
    else {
        fc_myConfig.freq44 = true;
    }

    switch (fc_myConfig.channels) {
    case 2:
        fc_myConfig.stereo = true;
        break;
    case 1:
    default:
        fc_myConfig.mono = true;
        break;
    }

    switch (fc_myConfig.precision) {
    case 16:
        fc_myConfig.bits16 = true;
        break;
    case 8:
    default:
        fc_myConfig.bits8 = true;
        break;
    }
}

static void fc_ip_config_save() {
    aud_set_int(configSection, "frequency", fc_myConfig.frequency);
    aud_set_int(configSection, "precision", fc_myConfig.precision);
    aud_set_int(configSection, "channels", fc_myConfig.channels);
}

static void configure_apply() {
    memcpy(&fc_myConfig, &config, sizeof(FCpluginConfig));

    if (config.bits16) {
        fc_myConfig.precision = 16;
    }
    else {  /* if (config.bits8) { */
        fc_myConfig.precision = 8;
    }

    if (config.stereo) {
        fc_myConfig.channels = 2;
    }
    else {  /* if (config.mono) { */
        fc_myConfig.channels = 1;
    }

    if (config.freq48) {
        fc_myConfig.frequency = FREQ_SAMPLE_48;
    }
    else if (config.freq22) {
        fc_myConfig.frequency = FREQ_SAMPLE_22;
    }
    else {  /* if (config.freq44) { */
        fc_myConfig.frequency = FREQ_SAMPLE_44;
    }

    fc_ip_config_save();
}

static void configure_init(void) {
    memcpy(&config, &fc_myConfig, sizeof(FCpluginConfig));
}

static PreferencesWidget prefs_precision[] = {
    //{WIDGET_RADIO_BTN, "16", &config.bits16, nullptr, nullptr, false, .cfg_type = VALUE_BOOLEAN},
    WidgetRadio("16", WidgetBool(config.bits16), {1}, WIDGET_NOT_CHILD),
    //{WIDGET_RADIO_BTN, "8", &config.bits8, nullptr, nullptr, false, .cfg_type = VALUE_BOOLEAN},
    WidgetRadio("8", WidgetBool(config.bits8), {1}, WIDGET_NOT_CHILD),
};

static PreferencesWidget prefs_channels[] = {
    //{WIDGET_RADIO_BTN, "Stereo", &config.stereo, nullptr, nullptr, false, .cfg_type = VALUE_BOOLEAN},
    WidgetRadio("Stereo", WidgetBool(config.stereo), {1}, WIDGET_NOT_CHILD),
    //{WIDGET_RADIO_BTN, "Mono", &config.mono, nullptr, nullptr, false, .cfg_type = VALUE_BOOLEAN},
    WidgetRadio("Mono", WidgetBool(config.mono), {1}, WIDGET_NOT_CHILD),
};

static PreferencesWidget prefs_frequency[] = {
    //{WIDGET_RADIO_BTN, "48000", &config.freq48, nullptr, nullptr, false, .cfg_type = VALUE_BOOLEAN},
    WidgetRadio("48000", WidgetBool(config.freq48), {1}, WIDGET_NOT_CHILD),
    //{WIDGET_RADIO_BTN, "44100", &config.freq44, nullptr, nullptr, false, .cfg_type = VALUE_BOOLEAN},
    WidgetRadio("44100", WidgetBool(config.freq44), {1}, WIDGET_NOT_CHILD),
    //{WIDGET_RADIO_BTN, "22050", &config.freq22, nullptr, nullptr, false, .cfg_type = VALUE_BOOLEAN},
    WidgetRadio("22050", WidgetBool(config.freq22), {1}, WIDGET_NOT_CHILD),
};

static PreferencesWidget prefs_top_row[] = {
    //{WIDGET_BOX, "Frequency [Hz]:", nullptr, nullptr, nullptr, false, VALUE_nullptr, nullptr, nullptr,
    // {.box = {prefs_frequency, G_N_ELEMENTS(prefs_frequency), false, true}}},
    //{WIDGET_BOX, "Precision [bits]:", nullptr, nullptr, nullptr, false, VALUE_nullptr, nullptr, nullptr,
    // {.box = {prefs_precision, G_N_ELEMENTS(prefs_precision), false, true}}},
    //{WIDGET_BOX, "Channels:", nullptr, nullptr, nullptr, false, VALUE_nullptr, nullptr, nullptr,
    // {.box = {prefs_channels, G_N_ELEMENTS(prefs_channels), false, true}}},
};

static PreferencesWidget prefs[] = {
    //    {WIDGET_BOX, nullptr, nullptr, nullptr, nullptr, false, VALUE_nullptr, nullptr, nullptr,
    // {.box = {prefs_top_row, G_N_ELEMENTS(prefs_top_row), true, false}}},
};

PluginPreferences fc_ip_preferences = {
    //.widgets = prefs,
    //.n_widgets = G_N_ELEMENTS(prefs),
    //    .init = configure_init,
    //.apply = configure_apply,
};

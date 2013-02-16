#include <glib.h>
#include <audacious/misc.h>
#include <audacious/plugin.h>
#include <audacious/preferences.h>

#include "configure.h"

FCpluginConfig fc_myConfig;
FCpluginConfig config;

static gchar const configSection[] = "FutureComposer";

static const gint FREQ_SAMPLE_48 = 48000;
static const gint FREQ_SAMPLE_44 = 44100;
static const gint FREQ_SAMPLE_22 = 22050;

static const gchar* const plugin_defaults[] = {
    "frequency", "44100",
    "precision", "8",
    "channels", "1",
    NULL
};

void fc_ip_load_config() {
    aud_config_set_defaults(configSection,plugin_defaults);

    fc_myConfig.frequency = aud_get_int(configSection, "frequency");
    fc_myConfig.precision = aud_get_int(configSection, "precision");
    fc_myConfig.channels = aud_get_int(configSection, "channels");

    fc_myConfig.freq48 = fc_myConfig.freq44 = fc_myConfig.freq22 = FALSE;
    fc_myConfig.bits16 = fc_myConfig.bits8 = FALSE;
    fc_myConfig.mono = fc_myConfig.stereo = FALSE;

    if (fc_myConfig.frequency == FREQ_SAMPLE_48) {
        fc_myConfig.freq48 = TRUE;
    }
    else if (fc_myConfig.frequency == FREQ_SAMPLE_22) {
        fc_myConfig.freq22 = TRUE;
    }
    else {
        fc_myConfig.freq44 = TRUE;
    }

    switch (fc_myConfig.channels) {
    case 2:
        fc_myConfig.stereo = TRUE;
        break;
    case 1:
    default:
        fc_myConfig.mono = TRUE;
        break;
    }

    switch (fc_myConfig.precision) {
    case 16:
        fc_myConfig.bits16 = TRUE;
        break;
    case 8:
    default:
        fc_myConfig.bits8 = TRUE;
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
    {WIDGET_RADIO_BTN, "16", &config.bits16, NULL, NULL, FALSE, .cfg_type = VALUE_BOOLEAN},
    {WIDGET_RADIO_BTN, "8", &config.bits8, NULL, NULL, FALSE, .cfg_type = VALUE_BOOLEAN},
};

static PreferencesWidget prefs_channels[] = {
    {WIDGET_RADIO_BTN, "Stereo", &config.stereo, NULL, NULL, FALSE, .cfg_type = VALUE_BOOLEAN},
    {WIDGET_RADIO_BTN, "Mono", &config.mono, NULL, NULL, FALSE, .cfg_type = VALUE_BOOLEAN},
};

static PreferencesWidget prefs_frequency[] = {
    {WIDGET_RADIO_BTN, "48000", &config.freq48, NULL, NULL, FALSE, .cfg_type = VALUE_BOOLEAN},
    {WIDGET_RADIO_BTN, "44100", &config.freq44, NULL, NULL, FALSE, .cfg_type = VALUE_BOOLEAN},
    {WIDGET_RADIO_BTN, "22050", &config.freq22, NULL, NULL, FALSE, .cfg_type = VALUE_BOOLEAN},
};

static PreferencesWidget prefs_top_row[] = {
    {WIDGET_BOX, "Frequency [Hz]:", NULL, NULL, NULL, FALSE, VALUE_NULL, NULL, NULL,
     {.box = {prefs_frequency, G_N_ELEMENTS(prefs_frequency), FALSE, TRUE}}},
    {WIDGET_BOX, "Precision [bits]:", NULL, NULL, NULL, FALSE, VALUE_NULL, NULL, NULL,
     {.box = {prefs_precision, G_N_ELEMENTS(prefs_precision), FALSE, TRUE}}},
    {WIDGET_BOX, "Channels:", NULL, NULL, NULL, FALSE, VALUE_NULL, NULL, NULL,
     {.box = {prefs_channels, G_N_ELEMENTS(prefs_channels), FALSE, TRUE}}},
};

static PreferencesWidget prefs[] = {
    {WIDGET_BOX, NULL, NULL, NULL, NULL, FALSE, VALUE_NULL, NULL, NULL,
     {.box = {prefs_top_row, G_N_ELEMENTS(prefs_top_row), TRUE, FALSE}}},
};

PluginPreferences fc_ip_preferences = {
    .widgets = prefs,
    .n_widgets = G_N_ELEMENTS(prefs),
    .init = configure_init,
    .apply = configure_apply,
};

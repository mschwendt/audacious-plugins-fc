#ifndef CONFIGURE_H
#define CONFIGURE_H

#include <glib.h>

typedef struct 
{
    gint frequency;
    gint precision;
    gint channels;

    gboolean freq48, freq44, freq22;
    gboolean bits16, bits8;
    gboolean mono, stereo;
} FCpluginConfig;

extern void fc_ip_load_config(void);

extern FCpluginConfig fc_myConfig;

#endif  /* CONFIGURE_H */

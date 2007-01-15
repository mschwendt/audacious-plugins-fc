#ifndef CONFIGURE_H
#define CONFIGURE_H

#include <gtk/gtk.h>

typedef struct 
{
    gint frequency;
    gint precision;
    gint channels;
} FCpluginConfig;

extern void fc_ip_configure(void);
extern void fc_ip_load_config(void);

extern FCpluginConfig fc_myConfig;

#endif  /* CONFIGURE_H */

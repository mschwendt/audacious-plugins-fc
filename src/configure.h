#ifndef CONFIGURE_H
#define CONFIGURE_H

typedef struct 
{
    int frequency;
    int precision;
    int channels;
    int panning;
    bool ignoreshorts;
    bool endshorts;
    int maxsecs;
    bool filtering;
} FCpluginConfig;

extern void fc_ip_load_config(void);

extern FCpluginConfig fc_myConfig;

#endif  /* CONFIGURE_H */

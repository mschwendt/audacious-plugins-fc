#ifndef CONFIGURE_H
#define CONFIGURE_H

typedef struct 
{
    int frequency;
    int precision;
    int channels;

    bool freq48, freq44, freq22;
    bool bits16, bits8;
    bool mono, stereo;
} FCpluginConfig;

extern void fc_ip_load_config(void);

extern FCpluginConfig fc_myConfig;

#endif  /* CONFIGURE_H */

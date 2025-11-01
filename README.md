Audacious input plugin to handle TFMX and Hippel TFMX related music
file formats like Future Composer from the Commodore Amiga era of
computing: https://github.com/mschwendt/audacious-plugins-fc

File content detection for files typically ending with these
file name extensions:

     .tfmx, .tfx, .tfm, .mdat
     .fc, .fc13, .fc14, .fc3, .fc4, .smod
     .hip, .hipc, .hip7
     .mcmd

If you run this plugin in Audacious with files using their original names
from Amiga, such as a pair "mdat.theme" and "smpl.theme", consider renaming
the files to give them PC-style file name extensions. E.g. "theme.tfx" and
"theme.sam" is one of the options. It will avoid issues with subsongs within
the files. Else visit the advanced Audacious settings and enable the checkbox,

```
 [X] Probe contents of files with no recognized file name extension
```

but that can cause side-effects with other file types.

This plugin strictly requires ``libtfmxaudiodecoder``.

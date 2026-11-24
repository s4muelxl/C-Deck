#ifndef CDECK_AUDIO_H
#define CDECK_AUDIO_H

#include <windows.h>

BOOL cdeck_change_volume(float difference);
BOOL cdeck_set_volume(float level);
BOOL cdeck_get_volume(float *level);
BOOL cdeck_toggle_mute(void);
BOOL cdeck_get_mute_state(BOOL *muted);

#endif

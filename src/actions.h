#ifndef CDECK_ACTIONS_H
#define CDECK_ACTIONS_H
#include "config.h"
#include <wchar.h>

typedef enum ActionResult {
    ACTION_SUCCESS = 0,
    ACTION_INVALID_ARGUMENT,
    ACTION_AUDIO_UNAVAILABLE,
    ACTION_MEDIA_UNAVAILABLE,
    ACTION_LAUNCH_FAILED,
    ACTION_NOT_CONFIGURED
} ActionResult;

ActionResult action_volume_step(float delta);
ActionResult action_get_volume(float *level, int *muted);
ActionResult action_set_volume(float level);
ActionResult action_toggle_mute(void);
ActionResult action_toggle_mic_mute(void);
ActionResult action_get_mic_mute(int *muted);
ActionResult action_open_sound_devices(void);
ActionResult action_media_play_pause(void);
ActionResult action_media_next(void);
ActionResult action_media_previous(void);
/* Strict UTF-8 -> UTF-16 conversion used by the native ShellExecuteW launcher.
   Exposed so Windows CI can verify the exact target without launching it. */
ActionResult action_prepare_launch_target(const DeckApp *app, wchar_t *wide_path, int max_chars);
ActionResult action_launch_app(const DeckApp *app);

#endif

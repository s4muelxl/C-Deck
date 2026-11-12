#ifndef CDECK_ACTIONS_H
#define CDECK_ACTIONS_H
#include "config.h"
typedef enum ActionResult { ACTION_SUCCESS=0, ACTION_INVALID_ARGUMENT, ACTION_AUDIO_UNAVAILABLE, ACTION_MEDIA_UNAVAILABLE, ACTION_LAUNCH_FAILED, ACTION_NOT_CONFIGURED } ActionResult;
ActionResult action_volume_step(float delta);
ActionResult action_toggle_mute(void);
ActionResult action_media_play_pause(void);
ActionResult action_media_next(void);
ActionResult action_media_previous(void);
ActionResult action_launch_app(const DeckApp *app);
#endif

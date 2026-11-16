#ifndef CDECK_AUDIO_ENDPOINT_POLICY_H
#define CDECK_AUDIO_ENDPOINT_POLICY_H

#include <stdint.h>

typedef enum DeckCaptureEndpointRole {
    DECK_CAPTURE_ENDPOINT_COMMUNICATIONS = 0,
    DECK_CAPTURE_ENDPOINT_MULTIMEDIA = 1,
    DECK_CAPTURE_ENDPOINT_NONE = 2
} DeckCaptureEndpointRole;

/* The callback must return nonzero only after the endpoint is fully usable. */
typedef int (*DeckCaptureEndpointAttempt)(DeckCaptureEndpointRole role, void *context);

/* Prefer the communications capture endpoint, then try multimedia if acquisition
   or activation failed. Returns NONE when neither endpoint can be activated. */
DeckCaptureEndpointRole deck_capture_endpoint_resolve(DeckCaptureEndpointAttempt attempt,
                                                       void *context);

/* Accept a COM out-parameter only when HRESULT succeeded and the interface is
   non-null. A partial interface returned with a failing HRESULT is released
   and cleared when a release callback is supplied. */
typedef void (*DeckAudioComRelease)(void *object);
int deck_audio_com_output_accept(int32_t hresult, void *output,
                                 DeckAudioComRelease release);

#endif

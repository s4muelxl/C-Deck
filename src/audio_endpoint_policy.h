#ifndef CDECK_AUDIO_ENDPOINT_POLICY_H
#define CDECK_AUDIO_ENDPOINT_POLICY_H

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

#endif

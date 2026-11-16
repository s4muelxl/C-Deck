#include "audio_endpoint_policy.h"

DeckCaptureEndpointRole deck_capture_endpoint_resolve(DeckCaptureEndpointAttempt attempt,
                                                       void *context) {
    if (!attempt) return DECK_CAPTURE_ENDPOINT_NONE;
    if (attempt(DECK_CAPTURE_ENDPOINT_COMMUNICATIONS, context)) {
        return DECK_CAPTURE_ENDPOINT_COMMUNICATIONS;
    }
    if (attempt(DECK_CAPTURE_ENDPOINT_MULTIMEDIA, context)) {
        return DECK_CAPTURE_ENDPOINT_MULTIMEDIA;
    }
    return DECK_CAPTURE_ENDPOINT_NONE;
}

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

int deck_audio_com_output_accept(int32_t hresult, void *output,
                                 DeckAudioComRelease release) {
    if (hresult >= 0 && output) return 1;

    /* COM normally clears failed out-parameters, but callers must not leak a
       partially returned interface if an implementation violates that rule. */
    if (output && release) release(output);
    return 0;
}

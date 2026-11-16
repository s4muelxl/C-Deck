#include "audio_endpoint_policy.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

typedef struct EndpointProbe {
    int communications_usable;
    int multimedia_usable;
    unsigned int calls;
    DeckCaptureEndpointRole attempted[2];
} EndpointProbe;

static int try_endpoint(DeckCaptureEndpointRole role, void *context) {
    EndpointProbe *probe = (EndpointProbe *)context;
    assert(probe != NULL);
    assert(probe->calls < 2);
    probe->attempted[probe->calls++] = role;
    if (role == DECK_CAPTURE_ENDPOINT_COMMUNICATIONS) {
        return probe->communications_usable;
    }
    if (role == DECK_CAPTURE_ENDPOINT_MULTIMEDIA) {
        return probe->multimedia_usable;
    }
    return 0;
}

typedef struct FakeComOutput {
    unsigned int releases;
} FakeComOutput;

static void release_fake_com_output(void *object) {
    FakeComOutput *fake = (FakeComOutput *)object;
    assert(fake != NULL);
    ++fake->releases;
}

static void test_com_out_parameter_cleanup(void) {
    FakeComOutput fake = {0};
    void *output = &fake;

    assert(deck_audio_com_output_accept(0, &output, release_fake_com_output));
    assert(output == &fake);
    assert(fake.releases == 0);

    /* S_FALSE is a successful HRESULT and must preserve the interface. */
    assert(deck_audio_com_output_accept(1, &output, release_fake_com_output));
    assert(output == &fake);
    assert(fake.releases == 0);

    /* A failed HRESULT with a non-null out pointer must release and clear it. */
    assert(!deck_audio_com_output_accept((int32_t)0x80004005u, &output,
                                         release_fake_com_output));
    assert(output == NULL);
    assert(fake.releases == 1);

    /* Success with a null interface is not a usable COM result. */
    output = NULL;
    assert(!deck_audio_com_output_accept(0, &output, release_fake_com_output));
    assert(output == NULL);
    assert(fake.releases == 1);

    /* Without a release callback, ownership stays with the caller. */
    output = &fake;
    assert(!deck_audio_com_output_accept((int32_t)0x80004005u, &output, NULL));
    assert(output == &fake);
    assert(fake.releases == 1);

    assert(!deck_audio_com_output_accept(0, NULL, release_fake_com_output));
}

int main(void) {
    EndpointProbe probe;
    DeckCaptureEndpointRole selected;

    test_com_out_parameter_cleanup();

    memset(&probe, 0, sizeof(probe));
    probe.communications_usable = 1;
    probe.multimedia_usable = 1;
    selected = deck_capture_endpoint_resolve(try_endpoint, &probe);
    assert(selected == DECK_CAPTURE_ENDPOINT_COMMUNICATIONS);
    assert(probe.calls == 1);
    assert(probe.attempted[0] == DECK_CAPTURE_ENDPOINT_COMMUNICATIONS);

    /* A communications endpoint may exist but fail IAudioEndpointVolume
       activation. The multimedia role must still be attempted. */
    memset(&probe, 0, sizeof(probe));
    probe.communications_usable = 0;
    probe.multimedia_usable = 1;
    selected = deck_capture_endpoint_resolve(try_endpoint, &probe);
    assert(selected == DECK_CAPTURE_ENDPOINT_MULTIMEDIA);
    assert(probe.calls == 2);
    assert(probe.attempted[0] == DECK_CAPTURE_ENDPOINT_COMMUNICATIONS);
    assert(probe.attempted[1] == DECK_CAPTURE_ENDPOINT_MULTIMEDIA);

    memset(&probe, 0, sizeof(probe));
    selected = deck_capture_endpoint_resolve(try_endpoint, &probe);
    assert(selected == DECK_CAPTURE_ENDPOINT_NONE);
    assert(probe.calls == 2);

    assert(deck_capture_endpoint_resolve(NULL, NULL) == DECK_CAPTURE_ENDPOINT_NONE);

    puts("capture endpoint fallback policy: OK");
    return 0;
}

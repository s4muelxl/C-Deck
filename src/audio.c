#define COBJMACROS
#include <windows.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <objbase.h>
#include "audio.h"

static IAudioEndpointVolume *get_audio_volume(void) {
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *device = NULL;
    IAudioEndpointVolume *volume = NULL;

    if (FAILED(CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                                &IID_IMMDeviceEnumerator, (void **)&enumerator))) {
        return NULL;
    }

    if (FAILED(IMMDeviceEnumerator_GetDefaultAudioEndpoint(
            enumerator, eRender, eMultimedia, &device))) {
        IMMDeviceEnumerator_Release(enumerator);
        return NULL;
    }

    if (FAILED(IMMDevice_Activate(device, &IID_IAudioEndpointVolume,
                                  CLSCTX_ALL, NULL, (void **)&volume))) {
        volume = NULL;
    }

    IMMDevice_Release(device);
    IMMDeviceEnumerator_Release(enumerator);
    return volume;
}

BOOL cdeck_get_volume(float *level) {
    IAudioEndpointVolume *volume;
    HRESULT result;

    if (level == NULL) return FALSE;
    volume = get_audio_volume();
    if (volume == NULL) return FALSE;

    result = IAudioEndpointVolume_GetMasterVolumeLevelScalar(volume, level);
    IAudioEndpointVolume_Release(volume);
    return SUCCEEDED(result);
}

BOOL cdeck_set_volume(float level) {
    IAudioEndpointVolume *volume;
    HRESULT result;

    if (level < 0.0f) level = 0.0f;
    if (level > 1.0f) level = 1.0f;
    volume = get_audio_volume();
    if (volume == NULL) return FALSE;

    result = IAudioEndpointVolume_SetMasterVolumeLevelScalar(volume, level, NULL);
    IAudioEndpointVolume_Release(volume);
    return SUCCEEDED(result);
}

BOOL cdeck_change_volume(float difference) {
    float current = 0.0f;
    if (!cdeck_get_volume(&current)) return FALSE;
    return cdeck_set_volume(current + difference);
}

BOOL cdeck_get_mute_state(BOOL *muted) {
    IAudioEndpointVolume *volume;
    HRESULT result;

    if (muted == NULL) return FALSE;
    volume = get_audio_volume();
    if (volume == NULL) return FALSE;

    result = IAudioEndpointVolume_GetMute(volume, muted);
    IAudioEndpointVolume_Release(volume);
    return SUCCEEDED(result);
}

BOOL cdeck_toggle_mute(void) {
    BOOL muted = FALSE;
    IAudioEndpointVolume *volume;
    HRESULT result;

    volume = get_audio_volume();
    if (volume == NULL) return FALSE;

    result = IAudioEndpointVolume_GetMute(volume, &muted);
    if (SUCCEEDED(result)) {
        result = IAudioEndpointVolume_SetMute(volume, !muted, NULL);
    }

    IAudioEndpointVolume_Release(volume);
    return SUCCEEDED(result);
}

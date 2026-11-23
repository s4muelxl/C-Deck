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

BOOL cdeck_change_volume(float difference) {
    IAudioEndpointVolume *volume = get_audio_volume();
    float current = 0.0f;
    HRESULT result;

    if (volume == NULL) return FALSE;

    result = IAudioEndpointVolume_GetMasterVolumeLevelScalar(volume, &current);
    if (SUCCEEDED(result)) {
        current += difference;
        if (current < 0.0f) current = 0.0f;
        if (current > 1.0f) current = 1.0f;
        result = IAudioEndpointVolume_SetMasterVolumeLevelScalar(volume, current, NULL);
    }

    IAudioEndpointVolume_Release(volume);
    return SUCCEEDED(result);
}

BOOL cdeck_toggle_mute(void) {
    IAudioEndpointVolume *volume = get_audio_volume();
    BOOL muted = FALSE;
    HRESULT result;

    if (volume == NULL) return FALSE;

    result = IAudioEndpointVolume_GetMute(volume, &muted);
    if (SUCCEEDED(result)) {
        result = IAudioEndpointVolume_SetMute(volume, !muted, NULL);
    }

    IAudioEndpointVolume_Release(volume);
    return SUCCEEDED(result);
}

#define COBJMACROS
#include "actions.h"
#include "audio_endpoint_policy.h"
#include <windows.h>
#include <shellapi.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
#include <wchar.h>
#include <string.h>

static void release_device_enumerator(void *object) {
    if (object) IMMDeviceEnumerator_Release((IMMDeviceEnumerator *)object);
}

static void release_audio_device(void *object) {
    if (object) IMMDevice_Release((IMMDevice *)object);
}

static void release_endpoint_volume(void *object) {
    if (object) IAudioEndpointVolume_Release((IAudioEndpointVolume *)object);
}

static IAudioEndpointVolume *get_endpoint_volume(void) {
    IMMDeviceEnumerator *enumerator = NULL;
    IMMDevice *device = NULL;
    IAudioEndpointVolume *volume = NULL;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                          &IID_IMMDeviceEnumerator, (void **)&enumerator);
    if (!deck_audio_com_output_accept((int32_t)hr, (void **)&enumerator,
                                      release_device_enumerator)) return NULL;

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumerator, eRender, eMultimedia, &device);
    if (!deck_audio_com_output_accept((int32_t)hr, (void **)&device, release_audio_device)) {
        goto cleanup;
    }

    hr = IMMDevice_Activate(device, &IID_IAudioEndpointVolume, CLSCTX_ALL,
                            NULL, (void **)&volume);
    if (!deck_audio_com_output_accept((int32_t)hr, (void **)&volume,
                                      release_endpoint_volume)) {
        goto cleanup;
    }

cleanup:
    if (device) IMMDevice_Release(device);
    if (enumerator) IMMDeviceEnumerator_Release(enumerator);
    return volume;
}

typedef struct CaptureEndpointActivation {
    IMMDeviceEnumerator *enumerator;
    IAudioEndpointVolume *volume;
} CaptureEndpointActivation;

static int activate_capture_endpoint(DeckCaptureEndpointRole role, void *context) {
    CaptureEndpointActivation *activation = (CaptureEndpointActivation *)context;
    IMMDevice *device = NULL;
    IAudioEndpointVolume *volume = NULL;
    ERole windows_role;
    HRESULT hr;

    if (!activation || !activation->enumerator || activation->volume) return 0;
    if (role == DECK_CAPTURE_ENDPOINT_COMMUNICATIONS) {
        windows_role = eCommunications;
    } else if (role == DECK_CAPTURE_ENDPOINT_MULTIMEDIA) {
        windows_role = eMultimedia;
    } else {
        return 0;
    }

    hr = IMMDeviceEnumerator_GetDefaultAudioEndpoint(activation->enumerator,
                                                       eCapture, windows_role, &device);
    if (!deck_audio_com_output_accept((int32_t)hr, (void **)&device, release_audio_device)) {
        return 0;
    }

    /* A default endpoint is not usable until IAudioEndpointVolume activates.
       If activation fails, release any partial interface and let the policy
       try the multimedia capture role. */
    hr = IMMDevice_Activate(device, &IID_IAudioEndpointVolume, CLSCTX_ALL,
                            NULL, (void **)&volume);
    if (!deck_audio_com_output_accept((int32_t)hr, (void **)&volume,
                                      release_endpoint_volume)) {
        IMMDevice_Release(device);
        return 0;
    }

    IMMDevice_Release(device);
    activation->volume = volume;
    return 1;
}

static IAudioEndpointVolume *get_capture_endpoint_volume(void) {
    IMMDeviceEnumerator *enumerator = NULL;
    CaptureEndpointActivation activation;
    HRESULT hr;

    hr = CoCreateInstance(&CLSID_MMDeviceEnumerator, NULL, CLSCTX_ALL,
                          &IID_IMMDeviceEnumerator, (void **)&enumerator);
    if (!deck_audio_com_output_accept((int32_t)hr, (void **)&enumerator,
                                      release_device_enumerator)) return NULL;

    activation.enumerator = enumerator;
    activation.volume = NULL;
    (void)deck_capture_endpoint_resolve(activate_capture_endpoint, &activation);

    IMMDeviceEnumerator_Release(enumerator);
    return activation.volume;
}

ActionResult action_volume_step(float delta) {
    IAudioEndpointVolume *volume;
    float current = 0.0f;
    HRESULT hr;

    if (delta == 0.0f) return ACTION_INVALID_ARGUMENT;
    volume = get_endpoint_volume();
    if (!volume) return ACTION_AUDIO_UNAVAILABLE;

    hr = IAudioEndpointVolume_GetMasterVolumeLevelScalar(volume, &current);
    if (SUCCEEDED(hr)) {
        float next = current + delta;
        if (next < 0.0f) next = 0.0f;
        if (next > 1.0f) next = 1.0f;
        hr = IAudioEndpointVolume_SetMasterVolumeLevelScalar(volume, next, NULL);
    }
    IAudioEndpointVolume_Release(volume);
    return SUCCEEDED(hr) ? ACTION_SUCCESS : ACTION_AUDIO_UNAVAILABLE;
}

ActionResult action_get_volume(float *level, int *muted) {
    IAudioEndpointVolume *volume;
    float current = 0.0f;
    BOOL is_muted = FALSE;
    HRESULT hr;

    if (!level || !muted) return ACTION_INVALID_ARGUMENT;
    volume = get_endpoint_volume();
    if (!volume) return ACTION_AUDIO_UNAVAILABLE;

    hr = IAudioEndpointVolume_GetMasterVolumeLevelScalar(volume, &current);
    if (SUCCEEDED(hr)) {
        hr = IAudioEndpointVolume_GetMute(volume, &is_muted);
    }
    IAudioEndpointVolume_Release(volume);
    if (FAILED(hr)) return ACTION_AUDIO_UNAVAILABLE;

    *level = current;
    *muted = is_muted ? 1 : 0;
    return ACTION_SUCCESS;
}

ActionResult action_set_volume(float level) {
    IAudioEndpointVolume *volume;
    HRESULT hr;

    if (level < 0.0f || level > 1.0f) return ACTION_INVALID_ARGUMENT;
    volume = get_endpoint_volume();
    if (!volume) return ACTION_AUDIO_UNAVAILABLE;

    hr = IAudioEndpointVolume_SetMasterVolumeLevelScalar(volume, level, NULL);
    IAudioEndpointVolume_Release(volume);
    return SUCCEEDED(hr) ? ACTION_SUCCESS : ACTION_AUDIO_UNAVAILABLE;
}

ActionResult action_toggle_mute(void) {
    IAudioEndpointVolume *volume = get_endpoint_volume();
    BOOL muted = FALSE;
    HRESULT hr;

    if (!volume) return ACTION_AUDIO_UNAVAILABLE;
    hr = IAudioEndpointVolume_GetMute(volume, &muted);
    if (SUCCEEDED(hr)) {
        hr = IAudioEndpointVolume_SetMute(volume, !muted, NULL);
    }
    IAudioEndpointVolume_Release(volume);
    return SUCCEEDED(hr) ? ACTION_SUCCESS : ACTION_AUDIO_UNAVAILABLE;
}

ActionResult action_toggle_mic_mute(void) {
    IAudioEndpointVolume *volume = get_capture_endpoint_volume();
    BOOL muted = FALSE;
    HRESULT hr;

    if (!volume) return ACTION_AUDIO_UNAVAILABLE;
    hr = IAudioEndpointVolume_GetMute(volume, &muted);
    if (SUCCEEDED(hr)) {
        hr = IAudioEndpointVolume_SetMute(volume, !muted, NULL);
    }
    IAudioEndpointVolume_Release(volume);
    return SUCCEEDED(hr) ? ACTION_SUCCESS : ACTION_AUDIO_UNAVAILABLE;
}

ActionResult action_get_mic_mute(int *muted) {
    IAudioEndpointVolume *volume;
    BOOL is_muted = FALSE;
    HRESULT hr;

    if (!muted) return ACTION_INVALID_ARGUMENT;
    *muted = 0;
    volume = get_capture_endpoint_volume();
    if (!volume) return ACTION_AUDIO_UNAVAILABLE;

    hr = IAudioEndpointVolume_GetMute(volume, &is_muted);
    IAudioEndpointVolume_Release(volume);
    if (FAILED(hr)) return ACTION_AUDIO_UNAVAILABLE;

    *muted = is_muted ? 1 : 0;
    return ACTION_SUCCESS;
}

ActionResult action_open_sound_devices(void) {
    HINSTANCE result = ShellExecuteW(NULL, L"open", L"ms-settings:sound", NULL, NULL, SW_SHOWNORMAL);
    if ((INT_PTR)result <= 32) {
        result = ShellExecuteW(NULL, L"open", L"control.exe", L"mmsys.cpl sounds", NULL, SW_SHOWNORMAL);
    }
    return (INT_PTR)result > 32 ? ACTION_SUCCESS : ACTION_LAUNCH_FAILED;
}

ActionResult action_prepare_launch_target(const DeckApp *app, wchar_t *wide_path, int max_chars) {
    if (!app || !wide_path || max_chars <= 0) return ACTION_INVALID_ARGUMENT;
    wide_path[0] = L'\0';
    if (!app->configured || !app->path[0]) return ACTION_NOT_CONFIGURED;
    /* DeckApp is fixed-size; never scan beyond an unterminated field. */
    if (!memchr(app->path, '\0', sizeof(app->path))) return ACTION_LAUNCH_FAILED;
    if (!MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, app->path, -1,
                             wide_path, max_chars)) {
        wide_path[0] = L'\0';
        return ACTION_LAUNCH_FAILED;
    }
    return ACTION_SUCCESS;
}

static intptr_t shell_execute_launch_target(const wchar_t *verb,
                                               const wchar_t *target,
                                               void *context) {
    (void)context;
    return (intptr_t)ShellExecuteW(NULL, verb, target, NULL, NULL, SW_SHOWNORMAL);
}

ActionResult action_launch_app_with_executor(const DeckApp *app,
                                             ActionShellExecuteExecutor executor,
                                             void *context) {
    wchar_t wide_path[DECK_PATH_WIDE_MAX];
    ActionResult prepared;
    intptr_t result;

    if (!executor) return ACTION_INVALID_ARGUMENT;
    prepared = action_prepare_launch_target(app, wide_path, DECK_PATH_WIDE_MAX);
    if (prepared != ACTION_SUCCESS) return prepared;

    result = executor(L"open", wide_path, context);
    return result > 32 ? ACTION_SUCCESS : ACTION_LAUNCH_FAILED;
}

ActionResult action_launch_app(const DeckApp *app) {
    return action_launch_app_with_executor(app, shell_execute_launch_target, NULL);
}

static ActionResult send_media_key(WORD virtual_key) {
    INPUT inputs[2];
    ZeroMemory(inputs, sizeof(inputs));
    inputs[0].type = INPUT_KEYBOARD;
    inputs[0].ki.wVk = virtual_key;
    inputs[1].type = INPUT_KEYBOARD;
    inputs[1].ki.wVk = virtual_key;
    inputs[1].ki.dwFlags = KEYEVENTF_KEYUP;
    return SendInput(2, inputs, sizeof(INPUT)) == 2 ? ACTION_SUCCESS : ACTION_MEDIA_UNAVAILABLE;
}

ActionResult action_media_play_pause(void) { return send_media_key(VK_MEDIA_PLAY_PAUSE); }
ActionResult action_media_next(void) { return send_media_key(VK_MEDIA_NEXT_TRACK); }
ActionResult action_media_previous(void) { return send_media_key(VK_MEDIA_PREV_TRACK); }

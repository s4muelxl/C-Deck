#define COBJMACROS
#include "actions.h"
#include <windows.h>
#include <shellapi.h>
#include <initguid.h>
#include <mmdeviceapi.h>
#include <endpointvolume.h>
static IAudioEndpointVolume *get_endpoint_volume(void) {
 IMMDeviceEnumerator *enumerator=NULL; IMMDevice *device=NULL; IAudioEndpointVolume *volume=NULL; HRESULT hr;
 hr=CoCreateInstance(&CLSID_MMDeviceEnumerator,NULL,CLSCTX_ALL,&IID_IMMDeviceEnumerator,(void **)&enumerator); if (FAILED(hr)||!enumerator) return NULL;
 hr=IMMDeviceEnumerator_GetDefaultAudioEndpoint(enumerator,eRender,eMultimedia,&device);
 if (SUCCEEDED(hr)&&device) hr=IMMDevice_Activate(device,&IID_IAudioEndpointVolume,CLSCTX_ALL,NULL,(void **)&volume);
 if (device) {
  IMMDevice_Release(device);
 }
 IMMDeviceEnumerator_Release(enumerator);
 return SUCCEEDED(hr) ? volume : NULL;
}
ActionResult action_volume_step(float delta) {
 IAudioEndpointVolume *volume; float current=0.0f; HRESULT hr; if (delta==0.0f) return ACTION_INVALID_ARGUMENT;
 volume=get_endpoint_volume(); if (!volume) return ACTION_AUDIO_UNAVAILABLE; hr=IAudioEndpointVolume_GetMasterVolumeLevelScalar(volume,&current);
 if (SUCCEEDED(hr)) { float next=current+delta; if(next<0.0f)next=0.0f;if(next>1.0f)next=1.0f;hr=IAudioEndpointVolume_SetMasterVolumeLevelScalar(volume,next,NULL); }
 IAudioEndpointVolume_Release(volume); return SUCCEEDED(hr)?ACTION_SUCCESS:ACTION_AUDIO_UNAVAILABLE;
}
ActionResult action_toggle_mute(void) { IAudioEndpointVolume *volume=get_endpoint_volume(); BOOL muted=FALSE; HRESULT hr; if(!volume)return ACTION_AUDIO_UNAVAILABLE; hr=IAudioEndpointVolume_GetMute(volume,&muted); if(SUCCEEDED(hr))hr=IAudioEndpointVolume_SetMute(volume,!muted,NULL); IAudioEndpointVolume_Release(volume); return SUCCEEDED(hr)?ACTION_SUCCESS:ACTION_AUDIO_UNAVAILABLE; }
ActionResult action_launch_app(const DeckApp *app) { HINSTANCE result; if(!app||!app->configured||!app->path[0])return ACTION_NOT_CONFIGURED; result=ShellExecuteA(NULL,"open",app->path,NULL,NULL,SW_SHOWNORMAL); return (INT_PTR)result>32?ACTION_SUCCESS:ACTION_LAUNCH_FAILED; }

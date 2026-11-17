#include "action_library.h"

static const DeckLibraryAction ACTIONS[] = {
    { DECK_LIBRARY_FAVORITES, L"Volume +5%", "Volume +", "action:volume_up", "icon:14" },
    { DECK_LIBRARY_FAVORITES, L"Alternar mudo", "Mudo Áudio", "action:audio_mute", "icon:13" },
    { DECK_LIBRARY_FAVORITES, L"Play / Pause", "Play / Pause", "action:media_play_pause", "icon:7" },
    { DECK_LIBRARY_FAVORITES, L"Abrir YouTube", "YouTube", "https://www.youtube.com", "icon:12" },

    { DECK_LIBRARY_SYSTEM, L"Explorador de Arquivos", "Explorador", "explorer.exe", "icon:4" },
    { DECK_LIBRARY_SYSTEM, L"Configurações de Som", "Configurações de Som", "action:sound_devices", "icon:14" },
    { DECK_LIBRARY_SYSTEM, L"Configurações de Tela", "Configurações de Tela", "ms-settings:display", "icon:5" },

    { DECK_LIBRARY_AUDIO, L"Aumentar volume", "Volume +", "action:volume_up", "icon:14" },
    { DECK_LIBRARY_AUDIO, L"Diminuir volume", "Volume -", "action:volume_down", "icon:14" },
    { DECK_LIBRARY_AUDIO, L"Alternar mudo de áudio", "Mudo Áudio", "action:audio_mute", "icon:13" },
    { DECK_LIBRARY_AUDIO, L"Alternar mudo do microfone", "Mudo Mic", "action:mic_mute", "icon:13" },

    { DECK_LIBRARY_MEDIA, L"Play / Pause", "Play / Pause", "action:media_play_pause", "icon:7" },
    { DECK_LIBRARY_MEDIA, L"Faixa anterior", "Faixa Anterior", "action:media_previous", "icon:7" },
    { DECK_LIBRARY_MEDIA, L"Próxima faixa", "Próxima Faixa", "action:media_next", "icon:7" },

    { DECK_LIBRARY_APPS, L"Bloco de Notas", "Bloco de Notas", "notepad.exe", "icon:1" },
    { DECK_LIBRARY_APPS, L"Calculadora", "Calculadora", "calc.exe", "icon:2" },
    { DECK_LIBRARY_APPS, L"Paint", "Paint", "mspaint.exe", "icon:3" },

    { DECK_LIBRARY_NAVIGATION, L"Abrir YouTube", "YouTube", "https://www.youtube.com", "icon:12" },
    { DECK_LIBRARY_NAVIGATION, L"Abrir Twitch", "Twitch", "https://www.twitch.tv", "icon:11" },
    { DECK_LIBRARY_NAVIGATION, L"Abrir GitHub", "GitHub", "https://github.com", "icon:6" }
};

size_t action_library_count(DeckLibraryCategory category) {
    size_t count = 0;
    if (category < 0 || category >= DECK_LIBRARY_CATEGORY_COUNT) return 0;
    for (size_t i = 0; i < sizeof(ACTIONS) / sizeof(ACTIONS[0]); ++i) {
        if (ACTIONS[i].category == category) ++count;
    }
    return count;
}

const DeckLibraryAction *action_library_get(DeckLibraryCategory category, size_t index) {
    if (category < 0 || category >= DECK_LIBRARY_CATEGORY_COUNT) return NULL;
    for (size_t i = 0; i < sizeof(ACTIONS) / sizeof(ACTIONS[0]); ++i) {
        if (ACTIONS[i].category != category) continue;
        if (index == 0) return &ACTIONS[i];
        --index;
    }
    return NULL;
}

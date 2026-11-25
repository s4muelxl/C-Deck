# C-Deck — Código Explicado

**Linguagem:** C11  
**Interface gráfica:** Raylib 5.5 + GLFW 3.4  
**Áudio do sistema:** Windows Core Audio (COM)  
**Atalhos de programas:** `ShellExecuteW`

## 1. Estrutura dos módulos

| Arquivo | Responsabilidade |
|---|---|
| `src/main.c` | Ponto de entrada `WinMain`, inicialização/encerramento do COM e registro de contexto de sessão |
| `src/interface.h` | Contrato público entre `main.c` e a interface (`cdeck_run` e `cdeck_log`) |
| `src/interface.c` | Sondagem WGL, criação da janela Raylib, renderização dos controles, leitura de mouse e log UTF-8 |
| `src/audio.h` / `src/audio.c` | Consulta e alteração do volume escalar (0.0 a 1.0) e do estado de mudo |
| `src/apps.h` / `src/apps.c` | Solicitação de abertura de executáveis do Windows via Shell |

## 2. Fluxo de execução

1. O Windows inicia o processo em `WinMain` (`src/main.c`).
2. `CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)` prepara o ambiente COM da thread principal para que a Windows Core Audio API funcione corretamente. Se ocorrer falha fatal, uma janela `MessageBoxW` informa o código `HRESULT` e interrompe a execução.
3. `cdeck_run()` (`src/interface.c`) sonda os formatos de pixel WGL disponíveis (`log_wgl_capabilities()`), configura o callback de log da Raylib (`SetTraceLogCallback`) e chama `InitWindow(900, 640, "C-Deck")` sem exigir MSAA (`SetConfigFlags(0)`).
4. Se `IsWindowReady()` for falso, o erro capturado do GLFW/Raylib é exibido explicitamente ao usuário junto ao caminho do arquivo `%TEMP%\C-Deck-startup.log`.
5. Com a janela ativa, `cdeck_get_volume()` e `cdeck_get_mute_state()` leem o estado inicial do áudio do sistema.
6. No loop principal (`while (!WindowShouldClose())`), a cada quadro:
   - Eventos de clique e arraste sobre a barra de volume chamam `cdeck_set_volume()`.
   - Cliques nos seis cards chamam `run_action()`, que delega para `audio.c` ou `apps.c` e atualiza o texto de status no rodapé.
   - `BeginDrawing()` e `EndDrawing()` redesenham o painel a 60 FPS.
7. Ao fechar a janela (botão fechar ou tecla `ESC`), `CloseWindow()` libera os recursos gráficos e `WinMain` chama `CoUninitialize()` quando aplicável.

## 3. Módulo de áudio (`src/audio.c`)

A função auxiliar `get_audio_volume()` obtém a interface `IAudioEndpointVolume` seguindo três passos COM:

1. Instancia `CLSID_MMDeviceEnumerator` via `CoCreateInstance`.
2. Recupera o dispositivo padrão de saída (`eRender`, `eMultimedia`) com `GetDefaultAudioEndpoint`.
3. Ativa a interface `IAudioEndpointVolume` no dispositivo e libera os ponteiros intermediários (`IMMDevice` e `IMMDeviceEnumerator`).

As funções públicas garantem saturação do valor escalar entre `0.0f` e `1.0f` e liberam a referência `IAudioEndpointVolume` após cada operação.

## 4. Módulo de atalhos (`src/apps.c`)

`cdeck_open_application(const wchar_t *path)` chama `ShellExecuteW(NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL)` para abrir `notepad.exe`, `calc.exe` e `mspaint.exe`. Pela documentação da Win32 API, valores de retorno convertidos para `INT_PTR` superiores a `32` indicam sucesso.

## 5. Build e compatibilidade gráfica (`scripts/`)

O script `scripts/build-raylib.ps1` baixa e verifica por SHA-256 as versões fixadas do **GLFW 3.4** e **Raylib 5.5**, aplica o patch `patches/glfw-wgl-software-fallback.patch` e compila as duas bibliotecas dinâmicas com `OPENGL_VERSION=1.1`. Em seguida, `scripts/build.ps1` compila os quatro arquivos `.c` e o arquivo de recursos (`cdeck-res.o` com ícone e manifesto) usando GCC UCRT64 e copia `libraylib.dll` e `glfw3.dll` para `build/`.

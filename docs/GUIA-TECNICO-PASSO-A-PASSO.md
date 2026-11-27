# C-Deck 1.5.2 — Guia Técnico Passo a Passo

Este documento apresenta o funcionamento interno da versão **1.5.2** do C-Deck, detalhando a arquitetura em C11, a integração com Raylib/GLFW e APIs do Windows, o pipeline de compilação e o processo de validação.

## 1. Visão geral

O **C-Deck** é um painel utilitário desktop para Windows 10 e Windows 11 (x64), escrito em **C11**. A camada de apresentação usa **Raylib 5.5** sobre **GLFW 3.4**, enquanto as operações de sistema utilizam a **Windows Core Audio API** e **ShellExecuteW**.

A janela oferece um slider contínuo de volume e seis ações diretas:

| Controle | Ação | Implementação |
|---|---|---|
| `VOLUME -` | Reduz 5 pontos percentuais | `cdeck_change_volume(-0.05f)` |
| `MUTE` | Alterna o estado de mudo | `cdeck_toggle_mute()` |
| `VOLUME +` | Aumenta 5 pontos percentuais | `cdeck_change_volume(0.05f)` |
| `NOTEPAD` | Abre o Bloco de Notas | `cdeck_open_application(L"notepad.exe")` |
| `CALCULATOR` | Abre a Calculadora | `cdeck_open_application(L"calc.exe")` |
| `PAINT` | Abre o Microsoft Paint | `cdeck_open_application(L"mspaint.exe")` |

## 2. Organização do projeto

```text
C-Deck/
├── src/
│   ├── main.c          # Ponto de entrada WinMain e inicialização COM
│   ├── interface.h     # Interface pública da camada gráfica e de log
│   ├── interface.c     # Janela Raylib, eventos, desenho e diagnóstico WGL
│   ├── audio.h         # Assinaturas do controle de áudio
│   ├── audio.c         # Acesso ao IAudioEndpointVolume via Core Audio
│   ├── apps.h          # Assinatura de abertura de programas
│   └── apps.c          # Integração com ShellExecuteW
├── patches/
│   └── glfw-wgl-software-fallback.patch
├── scripts/
│   ├── build-raylib.ps1
│   ├── build.ps1
│   └── check-version.ps1
├── installer/
│   ├── C-Deck.iss
│   └── C-Deck.manifest
└── .github/workflows/
```

### Por que separar em `.c` e `.h`?

- Os cabeçalhos (`.h`) expõem apenas as assinaturas públicas necessárias aos demais módulos.
- Os arquivos de implementação (`.c`) encapsulam os detalhes da Win32 API, COM e Raylib.
- Essa divisão evita incluir `<windows.h>` e `<mmdeviceapi.h>` dentro do mesmo escopo de símbolos da `raylib.h`, prevenindo conflitos de identificadores como `Rectangle`, `CloseWindow`, `ShowCursor`, `DrawText` e `PlaySound`.

## 3. Fluxo completo de inicialização e execução

1. O executável inicia em `WinMain()` (`src/main.c`).
2. `cdeck_log()` registra o início da sessão e o valor de `GetSystemMetrics(SM_REMOTESESSION)` em `%TEMP%\C-Deck-startup.log` (UTF-8 com timestamps).
3. `CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)` inicializa COM:
   - `S_OK` ou `S_FALSE`: marca `com_initialized = TRUE` para liberar com `CoUninitialize()` ao sair.
   - `RPC_E_CHANGED_MODE`: indica que a thread já possui modelo COM ativo; o programa registra o aviso e prossegue sem chamar `CoUninitialize()`.
   - Outros códigos de falha (`FAILED`): registra o `HRESULT` no log, exibe `MessageBoxW` e encerra com código `1`.
4. `cdeck_run()` (`src/interface.c`) chama `log_wgl_capabilities()` para contabilizar formatos de pixel WGL acelerados e genéricos antes de iniciar o contexto gráfico.
5. O callback `cdeck_raylib_log()` é conectado com `SetTraceLogCallback()`, e `SetConfigFlags(0)` evita exigir MSAA (`FLAG_MSAA_4X_HINT`).
6. `InitWindow(900, 640, "C-Deck")` cria a janela via GLFW. Se `!IsWindowReady()`, uma caixa de diálogo informa o erro capturado e retorna código `2`.
7. Com a janela pronta, o loop principal lê o estado inicial de volume/mudo, processa cliques e arraste do slider, executa as ações solicitadas e atualiza a tela a 60 FPS até o fechamento da janela ou pressionamento de `ESC`.

## 4. Solução de compatibilidade OpenGL / WGL

Durante os testes em máquinas Windows 10 reais, identificou-se que o pacote padrão da Raylib força contexto **OpenGL 3.3 Core Profile** (`GLFW_OPENGL_CORE_PROFILE`). Isso causa duas falhas conhecidas em hardware mais antigo ou virtualizado:

1. **Erro GLFW 65543 (`WGL_ARB_create_context_profile is unavailable`)**: ocorre em GPUs integradas antigas cujo driver suporta até OpenGL 2.1 (como Intel HD Graphics de 1ª geração).
2. **Erro GLFW 65542 (`The driver does not appear to support OpenGL`)**: ocorre quando não há ICD OpenGL de hardware (ex.: adaptador básico do Windows, máquinas virtuais sem aceleração 3D ou RDP), restando apenas o driver `Microsoft GDI Generic` (OpenGL 1.1), que o GLFW padrão ignora por possuir a flag `PFD_GENERIC_FORMAT`.

A correção definitiva atua diretamente na origem do problema:

- **Raylib 5.5 com `OPENGL_VERSION=1.1`:** os primitives 2D da Raylib (`DrawRectangleRounded`, `DrawCircle`, `DrawText`) utilizam o pipeline OpenGL 1.1 compatível tanto com GPUs modernas (OpenGL 3.3/4.x) quanto com drivers OpenGL 2.1 e renderizadores OpenGL 1.1.
- **Patch no GLFW 3.4 (`choosePixelFormatWGL`):** a seleção de pixel format realiza primeiro a busca padrão por formatos acelerados por hardware (`allowSoftware = 0`). Somente se nenhum formato acelerado existir (`usableCount == 0`), uma segunda passagem (`allowSoftware = 1`) aceita o formato genérico do Windows.
- **Manifesto (`installer/C-Deck.manifest`):** declara execução `asInvoker` e `longPathAware` sem forçar GUID de versão do Windows 10 que bloqueia drivers ICD antigos WDDM 1.1.

## 5. Análise dos módulos em C

### 5.1 `src/interface.h` e `src/main.c`

- `src/interface.h` exporta apenas `cdeck_log()` e `cdeck_run()`.
- `src/main.c` coordena inicialização do ambiente COM, validação explícita de erros e encerramento limpo.

### 5.2 `src/interface.c`

- Isola macros colidentes da Win32 API antes de incluir `<windows.h>` e restaura os nomes antes de incluir `"raylib.h"`.
- Mantém seis estruturas `CDeckCard` declaradas em grade fixa (3 colunas × 2 linhas).
- Mantém estado de arraste contínuo (`dragging_slider`) com tolerância mínima (`0.003f`) para evitar chamadas COM redundantes a cada quadro parado.

### 5.3 `src/audio.c` e `src/apps.c`

- `audio.c` ativa `#define COBJMACROS` para chamar métodos COM diretamente em C11 (`IMMDeviceEnumerator_GetDefaultAudioEndpoint`, `IMMDevice_Activate`, `IAudioEndpointVolume_GetMasterVolumeLevelScalar`, `SetMasterVolumeLevelScalar`, `GetMute`, `SetMute`).
- `apps.c` isola `<shellapi.h>` em uma unidade dedicada, mantendo o contrato simples `BOOL cdeck_open_application(const wchar_t *path)`.

## 6. Compilação, empacotamento e CI/CD

### Flags principais do GCC (`scripts/build.ps1`)

| Flag | Finalidade |
|---|---|
| `-Wall -Wextra -Wpedantic` | Habilita avisos rigorosos de conformidade |
| `-std=c11` | Padroniza a compilação em C11 |
| `-O2` | Otimização para distribuição |
| `-D_WIN32_WINNT=0x0601` | Compatibilidade a partir do Windows 7 / 10 / 11 |
| `-mwindows` | Gera binário de interface gráfica sem janela de console |
| `-lraylib -lopengl32 -lgdi32 -lwinmm -luser32 -lshell32 -lole32 -luuid` | Bibliotecas gráficas e nativas do Windows |

### Pipeline GitHub Actions (`.github/workflows/`)

- **`ci.yml`**: executa testes de contrato (`web/tests/ux-contract.test.js`), valida consistência de versão (`scripts/check-version.ps1`), compila GLFW 3.4 + Raylib 5.5 + `C-Deck.exe`, verifica dependências transitivas via `ldd`, gera o pacote portátil (`.zip`) e compila o instalador Inno Setup (`C-Deck-Setup.exe`).
- **`release.yml`**: ao atualizar a branch `main`, valida os metadados da versão em `VERSION`, compila o projeto completo e publica a release oficial no GitHub anexando o instalador e as DLLs de runtime.

## 7. Checklist de validação manual no Windows

1. Instalar via `C-Deck-Setup.exe` em um diretório limpo (`%LOCALAPPDATA%\Programs\C-Deck`).
2. Abrir o C-Deck e confirmar a exibição da janela sem erros de OpenGL/GLFW.
3. Verificar `%TEMP%\C-Deck-startup.log` confirmando inicialização do COM, formatos WGL e backend Raylib.
4. Testar **VOLUME -**, **VOLUME +**, **MUTE** e o slider de volume.
5. Testar **NOTEPAD**, **CALCULATOR** e **PAINT**.
6. Fechar por `ESC` ou pelo botão fechar da janela e confirmar encerramento com código `0`.
7. Validar o desinstalador (`unins000.exe`).

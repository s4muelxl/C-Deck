# C-Deck 1.5.0 — Guia técnico passo a passo

> Material de estudo e handoff para a equipe. Este documento descreve a implementação que está na branch `main`, explica o caminho de execução, as APIs utilizadas, como compilar e testar e inclui o código-fonte C completo desta versão.

## 1. Visão geral

O C-Deck é um painel desktop compacto para Windows, escrito em **C11**, usando a **Win32 API** para a janela e os controles e a **Windows Core Audio API** para controlar o volume do dispositivo de saída padrão.

A interface possui seis ações:

| Controle | Resultado | Implementação |
|---|---|---|
| Volume + | Aumenta 5 pontos percentuais | Core Audio |
| Volume - | Diminui 5 pontos percentuais | Core Audio |
| Mudo | Alterna entre áudio ligado e silenciado | Core Audio |
| Bloco de Notas | Solicita abertura do Notepad | ShellExecuteW |
| Calculadora | Solicita abertura da Calculadora | ShellExecuteW |
| Paint | Solicita abertura do Paint | ShellExecuteW |

O aplicativo não usa banco de dados, servidor, internet, plugins ou runtime externo. As operações são executadas localmente pelo Windows.

## 2. Arquitetura e divisão de responsabilidades

O projeto foi dividido em módulos pequenos para que cada integrante consiga estudar uma parte sem precisar entender todo o código de uma vez.

```text
C-Deck/
├── src/
│   ├── main.c          # ponto de entrada e inicialização COM
│   ├── interface.c    # janela, controles e eventos
│   ├── interface.h    # contrato público da interface
│   ├── audio.c        # acesso ao Windows Core Audio
│   ├── audio.h        # contrato público do módulo de áudio
│   ├── apps.c         # abertura de programas do Windows
│   └── apps.h         # contrato público do módulo de atalhos
├── scripts/
│   ├── build.ps1      # compilação local no Windows
│   └── check-version.ps1
├── installer/
│   └── C-Deck.iss     # receita do instalador Inno Setup
├── docs/
├── web/
└── README.md
```

### Por que separar em .c e .h?

- O arquivo **.h (header)** declara o que o módulo oferece: nomes de funções, parâmetros e tipos de retorno.
- O arquivo **.c** contém a implementação, isto é, as instruções que realmente executam o trabalho.
- Outros módulos incluem o header e chamam as funções sem precisar conhecer os detalhes internos.
- Isso reduz dependências, facilita testes e permite alterar a implementação sem mudar quem consome a função, desde que o contrato seja mantido.

### Contratos entre os módulos

| Função | Declarada em | Implementada em | Finalidade |
|---|---|---|---|
| `cdeck_run()` | `interface.h` | `interface.c` | Cria a janela e executa o loop de mensagens |
| `cdeck_change_volume()` | `audio.h` | `audio.c` | Ajusta o volume por uma diferença escalar |
| `cdeck_toggle_mute()` | `audio.h` | `audio.c` | Inverte o estado de mudo |
| `cdeck_open_application()` | `apps.h` | `apps.c` | Solicita a abertura de um executável |

## 3. Fluxo completo de execução

1. O Windows inicia o executável e chama `WinMain()`, em `main.c`.
2. `CoInitializeEx()` inicializa o ambiente COM da thread principal. O módulo de áudio usa interfaces COM.
3. Se a inicialização falhar, o programa mostra uma mensagem de erro e termina.
4. `cdeck_run()`, em `interface.c`, registra a classe da janela e cria a janela principal.
5. Durante a criação, o Windows envia `WM_CREATE`; o procedimento da janela chama `create_controls()`.
6. `create_controls()` cria títulos, seis botões e um texto de status.
7. O loop `GetMessageW / TranslateMessage / DispatchMessageW` mantém a janela responsiva e encaminha eventos para `window_procedure()`.
8. Quando o usuário clica em um botão, o Windows envia `WM_COMMAND`. O identificador do botão é lido com `LOWORD(wparam)`.
9. O identificador seleciona a operação: módulo de áudio ou módulo de abertura de aplicativos.
10. O resultado é mostrado na etiqueta de status.
11. Ao fechar a janela, chega `WM_DESTROY`; `PostQuitMessage(0)` encerra o loop.
12. O controle volta para `WinMain()`, que chama `CoUninitialize()` e retorna o código de saída.

## 4. Explicação arquivo por arquivo

### 4.1 `src/main.c` — inicialização

- `WinMain` é o ponto de entrada de aplicações gráficas Win32; por isso não é usado `main()` tradicional.
- `HINSTANCE` identifica a instância do programa no Windows.
- `CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)` inicializa COM no modelo apartment-threaded para a thread principal.
- `FAILED(com_result)` verifica se a inicialização falhou. Em caso de falha, uma caixa de mensagem informa o problema.
- `cdeck_run(instance, show_command)` transfere o controle para o módulo da interface.
- `CoUninitialize()` encerra a inicialização COM feita por esta thread antes de sair.

**Ponto importante:** COM deve ser inicializado por thread antes de usar objetos COM nessa thread. A inicialização feita aqui permite que o módulo de áudio crie e use as interfaces Core Audio durante os eventos da janela.

### 4.2 `src/interface.h` — contrato da interface

O header possui uma *include guard* (`CDECK_INTERFACE_H`) para impedir inclusões repetidas. Ele inclui `windows.h`, pois usa o tipo `HINSTANCE`, e declara `cdeck_run()`.

O header não cria a janela. Ele apenas informa aos outros arquivos que a função existe e qual assinatura deve ser respeitada.

### 4.3 `src/interface.c` — janela, botões e eventos

**Identificadores dos botões**

As constantes `ID_VOLUME_UP`, `ID_VOLUME_DOWN`, `ID_MUTE`, `ID_NOTEPAD`, `ID_CALCULATOR` e `ID_PAINT` atribuem um número único a cada controle. Quando um clique chega em `WM_COMMAND`, esse número permite descobrir qual botão foi acionado.

**Layout**

- `BUTTON_WIDTH` e `BUTTON_HEIGHT` definem dimensões.
- `LEFT_COLUMN` e `RIGHT_COLUMN` definem as duas colunas.
- `FIRST_ROW` e `ROW_GAP` definem a posição vertical das três linhas.
- As posições são coordenadas dentro da área cliente da janela.

**Criação dos controles**

- `CreateWindowW(L"STATIC", ...)` cria textos estáticos.
- `CreateWindowW(L"BUTTON", ...)` cria botões nativos do Windows.
- `WS_CHILD` indica que o controle pertence à janela principal.
- `WS_VISIBLE` faz o controle aparecer.
- `WS_TABSTOP` permite alcançar o botão pelo teclado com Tab.
- `BS_PUSHBUTTON` define o estilo de botão de ação.
- `SendMessageW(..., WM_SETFONT, ...)` aplica a fonte padrão da interface.

**Tratamento de eventos**

`window_procedure()` é a função de callback que recebe mensagens da janela:

- `WM_CREATE`: cria os controles.
- `WM_COMMAND`: identifica o clique e chama o módulo responsável.
- `WM_DESTROY`: sinaliza o encerramento.
- `DefWindowProcW()`: encaminha mensagens não tratadas ao comportamento padrão do Windows.

**Loop de mensagens**

`GetMessageW()` aguarda mensagens; `TranslateMessage()` converte mensagens de teclado; `DispatchMessageW()` entrega a mensagem ao procedimento da janela. Esse loop é o mecanismo que mantém a aplicação gráfica funcionando.

### 4.4 `src/audio.h` e `src/audio.c` — controle de áudio

O módulo usa a **Windows Core Audio API**, especificamente a interface `IAudioEndpointVolume`.

**Aquisição do dispositivo**

1. `CoCreateInstance()` cria um enumerador de dispositivos (`IMMDeviceEnumerator`).
2. `GetDefaultAudioEndpoint(eRender, eMultimedia, ...)` obtém o dispositivo padrão de saída para uso multimídia.
3. `IMMDevice_Activate()` solicita a interface `IAudioEndpointVolume` para esse dispositivo.
4. As interfaces temporárias `IMMDevice` e `IMMDeviceEnumerator` são liberadas antes do retorno.
5. Se alguma etapa falhar, a função retorna `NULL` e o chamador informa falha à interface.

**Ajuste de volume**

- `GetMasterVolumeLevelScalar()` lê o volume como valor normalizado entre `0.0` e `1.0`.
- O parâmetro `difference` representa o incremento: `0.05f` para aumentar 5 pontos percentuais e `-0.05f` para diminuir.
- O resultado é limitado: abaixo de zero vira `0.0f`; acima de um vira `1.0f`.
- `SetMasterVolumeLevelScalar()` aplica o novo valor.
- `IAudioEndpointVolume_Release()` libera a interface adquirida.

**Mudo**

- `GetMute()` consulta o estado atual.
- `SetMute(!muted, NULL)` grava o estado oposto.
- A interface é liberada no final.

**Gerenciamento de recursos:** toda interface COM adquirida com sucesso precisa ter sua referência liberada. O código libera as interfaces auxiliares e a interface de volume em seus caminhos de execução.

### 4.5 `src/apps.h` e `src/apps.c` — atalhos para programas

A função `cdeck_open_application()` recebe o nome/caminho do programa em texto Unicode (`wchar_t* `) e chama `ShellExecuteW()`.

- `L"open"` pede ao Shell para abrir o alvo.
- `notepad.exe`, `calc.exe` e `mspaint.exe` são os alvos usados pela interface.
- `SW_SHOWNORMAL` solicita que a janela do programa seja exibida normalmente.
- O retorno de `ShellExecuteW()` é considerado sucesso quando convertido para inteiro e maior que 32; valores iguais ou inferiores indicam erro segundo o contrato dessa API.

O módulo não implementa um editor de texto, calculadora ou editor de imagem: ele solicita ao Windows que abra os aplicativos instalados.

## 5. O que acontece em cada clique

| Clique | Caminho no código |
|---|---|
| Volume + | `WM_COMMAND → ID_VOLUME_UP → cdeck_change_volume(0.05f)` |
| Volume - | `WM_COMMAND → ID_VOLUME_DOWN → cdeck_change_volume(-0.05f)` |
| Mudo | `WM_COMMAND → ID_MUTE → cdeck_toggle_mute()` |
| Bloco de Notas | `WM_COMMAND → ID_NOTEPAD → cdeck_open_application(L"notepad.exe")` |
| Calculadora | `WM_COMMAND → ID_CALCULATOR → cdeck_open_application(L"calc.exe")` |
| Paint | `WM_COMMAND → ID_PAINT → cdeck_open_application(L"mspaint.exe")` |

## 6. Compilação: como o código vira um .exe

O script `scripts/build.ps1` automatiza a compilação local:

1. Interrompe em caso de erro (`$ErrorActionPreference = "Stop"`).
2. Cria a pasta `build`.
3. Confere se `gcc` e `windres` estão disponíveis no PATH.
4. Executa `brand/build-icon.ps1` para gerar o ícone.
5. Cria o arquivo de recursos `build/cdeck.rc`, que referencia o ícone e o manifesto.
6. Executa `windres` para transformar o recurso em objeto que o linker consegue incorporar.
7. Executa o GCC com os quatro arquivos C e o objeto de recursos.
8. Copia o README para a pasta de saída.

**Comando:**

```powershell
.\scripts\build.ps1
```

**Flags principais do GCC:**

| Flag | Significado |
|---|---|
| `-Wall -Wextra -Wpedantic` | Ativa grupos de avisos do compilador |
| `-std=c11` | Usa o padrão C11 |
| `-O2` | Ativa otimizações |
| `-D_WIN32_WINNT=0x0601` | Define a versão mínima de API Windows usada na compilação |
| `-mwindows` | Gera aplicação gráfica sem console |
| `-luser32` | Liga funções de janela e interface do Windows |
| `-lshell32` | Liga ShellExecute e funções do Shell |
| `-lole32` | Liga infraestrutura COM/OLE |
| `-luuid` | Resolve identificadores GUID usados pelas APIs |

Saída esperada: `build/C-Deck.exe`.

## 7. Instalador Windows — Inno Setup

O arquivo `installer/C-Deck.iss` descreve como empacotar o executável:

- Define nome e versão do aplicativo.
- Instala no perfil do usuário em `%LOCALAPPDATA%\Programs\C-Deck`, sem exigir privilégios administrativos.
- Restringe o instalador a Windows x64 compatível.
- Copia `C-Deck.exe` e o README.
- Cria atalho no menu Iniciar e oferece atalho opcional na área de trabalho.
- Oferece abrir o C-Deck ao concluir a instalação.
- Gera `C-Deck-Setup.exe` na pasta `build/installer`.

O instalador não compila o programa: ele empacota um executável que já foi gerado pelo GCC.

## 8. Integração contínua e publicação

Os workflows em `.github/workflows/` automatizam verificações no GitHub Actions.

### CI (`ci.yml`)

1. Baixa o repositório.
2. Instala Node.js para executar o teste de contrato do site.
3. Executa `web/tests/ux-contract.test.js`.
4. Confere consistência da versão.
5. Prepara o ambiente MinGW-w64.
6. Gera ícone e recursos.
7. Compila os quatro módulos C.
8. Cria um ZIP portátil e o instalador.
9. Publica os arquivos como artifacts do workflow para inspeção/download.

### Release (`release.yml`)

1. Lê a versão de `VERSION` e valida o formato MAJOR.MINOR.PATCH.
2. Confere que o workflow está executando no commit atual de `main`.
3. Confere a consistência de versão.
4. Prepara MinGW-w64, compila o programa e constrói o instalador.
5. Usa a ação de release para criar/publicar a release e anexar `C-Deck-Setup.exe`.

A CI valida e empacota builds. A publicação da release é uma etapa separada; não se deve afirmar que um instalador foi publicado antes de confirmar o sucesso da etapa de publicação e a presença do asset na página da release.

## 9. Como testar manualmente

Após compilar ou instalar em Windows 10/11 x64:

1. Abra o C-Deck.
2. Clique em Volume + e confirme que o volume aumenta aproximadamente 5 pontos percentuais.
3. Clique em Volume - e confirme que diminui aproximadamente 5 pontos percentuais.
4. Clique em Mudo duas vezes e confirme que o áudio alterna entre silenciado e ativo.
5. Clique nos três atalhos e confirme que cada programa abre.
6. Confirme se o texto de status muda após cada operação.
7. Feche a janela e confirme que o processo encerra.

**Limites do teste:** o resultado de volume depende do dispositivo de saída padrão ativo no Windows. Os atalhos dependem de os aplicativos estarem disponíveis no sistema. O teste de compilação não substitui o teste manual em uma máquina Windows real.

## 10. Erros comuns e diagnóstico

| Sintoma | Verificação inicial |
|---|---|
| `gcc não encontrado` | Instalar MinGW-w64 e adicionar o diretório `bin` ao PATH |
| `windres não encontrado` | Instalar o pacote MinGW-w64 que inclui binutils |
| Falha ao gerar ícone | Verificar os arquivos em `brand/` e a execução do script de ícone |
| Volume não muda | Conferir dispositivo de saída padrão, permissões e se o dispositivo aceita controle de volume por software |
| Aplicativo não abre | Conferir se o executável alvo está disponível nessa instalação do Windows |
| Janela não inicia | Conferir build, arquitetura e dependências do ambiente |

## 11. Divisão sugerida para três integrantes

### Integrante A — Interface e fluxo Win32
Arquivos: `src/interface.c` e `src/interface.h`.

Deve explicar: classe da janela, criação dos controles, IDs, mensagens Win32, callback, loop de mensagens e atualização de status.

### Integrante B — Áudio e COM
Arquivos: `src/audio.c` e `src/audio.h`, com a inicialização COM de `src/main.c`.

Deve explicar: COM, enumerador de dispositivos, endpoint padrão, volume escalar, limites 0–1, mute e liberação de interfaces.

### Integrante C — Atalhos, build e entrega
Arquivos: `src/apps.c` e `src/apps.h`, `scripts/build.ps1`, `installer/C-Deck.iss` e workflows.

Deve explicar: ShellExecuteW, retorno da API, compilação GCC, recursos, instalador, CI e release.

**Todos devem ler também:** este guia, `README.md` e o fluxo da seção 3. A divisão serve para organizar a apresentação, não para limitar o conhecimento do restante do projeto.

## 12. Código-fonte completo desta versão

Os blocos abaixo reproduzem os arquivos C e headers da versão documentada. O repositório continua sendo a fonte oficial para obter a versão mais recente.

### 12.1 `src/main.c`

```c
#include <windows.h>
#include <objbase.h>
#include "interface.h"

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous,
                   LPSTR command_line, int show_command) {
    HRESULT com_result;
    int exit_code;

    (void)previous;
    (void)command_line;

    com_result = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    if (FAILED(com_result)) {
        MessageBoxW(NULL, L"Nao foi possivel iniciar os recursos do Windows.",
                    L"C-Deck", MB_OK | MB_ICONERROR);
        return 1;
    }

    exit_code = cdeck_run(instance, show_command);
    CoUninitialize();
    return exit_code;
}
```

### 12.2 `src/interface.h`

```c
#ifndef CDECK_INTERFACE_H
#define CDECK_INTERFACE_H

#include <windows.h>

int cdeck_run(HINSTANCE instance, int show_command);

#endif
```

### 12.3 `src/interface.c`

```c
#include <windows.h>
#include "interface.h"
#include "audio.h"
#include "apps.h"

#define ID_VOLUME_UP 101
#define ID_VOLUME_DOWN 102
#define ID_MUTE 103
#define ID_NOTEPAD 104
#define ID_CALCULATOR 105
#define ID_PAINT 106

#define BUTTON_WIDTH 155
#define BUTTON_HEIGHT 42
#define LEFT_COLUMN 18
#define RIGHT_COLUMN 187
#define FIRST_ROW 82
#define ROW_GAP 50

static HWND status_label;

static void set_status(const wchar_t *message) {
    SetWindowTextW(status_label, message);
}

static void add_button(HWND window, HINSTANCE instance,
                       const wchar_t *label, int id, int x, int y) {
    HWND button = CreateWindowW(
        L"BUTTON", label, WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,
        x, y, BUTTON_WIDTH, BUTTON_HEIGHT,
        window, (HMENU)(INT_PTR)id, instance, NULL
    );
    if (button != NULL) {
        SendMessageW(button, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    }
}

static void create_controls(HWND window, HINSTANCE instance) {
    HWND title = CreateWindowW(
        L"STATIC", L"C-DECK", WS_CHILD | WS_VISIBLE,
        18, 16, 320, 26, window, NULL, instance, NULL
    );
    HWND subtitle = CreateWindowW(
        L"STATIC", L"Atalhos rápidos do Windows", WS_CHILD | WS_VISIBLE,
        18, 43, 320, 20, window, NULL, instance, NULL
    );

    if (title != NULL) SendMessageW(title, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    if (subtitle != NULL) SendMessageW(subtitle, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);

    add_button(window, instance, L"Volume +", ID_VOLUME_UP, LEFT_COLUMN, FIRST_ROW);
    add_button(window, instance, L"Volume -", ID_VOLUME_DOWN, RIGHT_COLUMN, FIRST_ROW);
    add_button(window, instance, L"Mudo", ID_MUTE, LEFT_COLUMN, FIRST_ROW + ROW_GAP);
    add_button(window, instance, L"Bloco de Notas", ID_NOTEPAD, RIGHT_COLUMN, FIRST_ROW + ROW_GAP);
    add_button(window, instance, L"Calculadora", ID_CALCULATOR, LEFT_COLUMN, FIRST_ROW + 2 * ROW_GAP);
    add_button(window, instance, L"Paint", ID_PAINT, RIGHT_COLUMN, FIRST_ROW + 2 * ROW_GAP);

    status_label = CreateWindowW(
        L"STATIC", L"Pronto para usar.", WS_CHILD | WS_VISIBLE,
        18, 245, 320, 22, window, NULL, instance, NULL
    );
    if (status_label != NULL) {
        SendMessageW(status_label, WM_SETFONT, (WPARAM)GetStockObject(DEFAULT_GUI_FONT), TRUE);
    }
}

static LRESULT CALLBACK window_procedure(HWND window, UINT message,
                                          WPARAM wparam, LPARAM lparam) {
    (void)lparam;

    switch (message) {
        case WM_CREATE:
            create_controls(window, ((LPCREATESTRUCTW)lparam)->hInstance);
            return 0;

        case WM_COMMAND:
            switch (LOWORD(wparam)) {
                case ID_VOLUME_UP:
                    set_status(cdeck_change_volume(0.05f) ? L"Volume aumentado." : L"Nao foi possivel alterar o volume.");
                    return 0;
                case ID_VOLUME_DOWN:
                    set_status(cdeck_change_volume(-0.05f) ? L"Volume reduzido." : L"Nao foi possivel alterar o volume.");
                    return 0;
                case ID_MUTE:
                    set_status(cdeck_toggle_mute() ? L"Estado do audio alterado." : L"Nao foi possivel alterar o audio.");
                    return 0;
                case ID_NOTEPAD:
                    set_status(cdeck_open_application(L"notepad.exe") ? L"Bloco de Notas aberto." : L"Nao foi possivel abrir o Bloco de Notas.");
                    return 0;
                case ID_CALCULATOR:
                    set_status(cdeck_open_application(L"calc.exe") ? L"Calculadora aberta." : L"Nao foi possivel abrir a Calculadora.");
                    return 0;
                case ID_PAINT:
                    set_status(cdeck_open_application(L"mspaint.exe") ? L"Paint aberto." : L"Nao foi possivel abrir o Paint.");
                    return 0;
                default:
                    break;
            }
            break;

        case WM_DESTROY:
            PostQuitMessage(0);
            return 0;

        default:
            break;
    }

    return DefWindowProcW(window, message, wparam, lparam);
}

int cdeck_run(HINSTANCE instance, int show_command) {
    WNDCLASSW window_class = {0};
    HWND window;
    MSG message;

    window_class.lpfnWndProc = window_procedure;
    window_class.hInstance = instance;
    window_class.lpszClassName = L"CDeckMainWindow";
    window_class.hCursor = LoadCursorW(NULL, MAKEINTRESOURCEW(32512));
    window_class.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);

    if (!RegisterClassW(&window_class)) return 1;

    window = CreateWindowW(
        L"CDeckMainWindow", L"C-Deck",
        WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
        CW_USEDEFAULT, CW_USEDEFAULT, 380, 330,
        NULL, NULL, instance, NULL
    );
    if (window == NULL) return 1;

    ShowWindow(window, show_command);
    UpdateWindow(window);

    while (GetMessageW(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return (int)message.wParam;
}
```

### 12.4 `src/audio.h`

```c
#ifndef CDECK_AUDIO_H
#define CDECK_AUDIO_H

#include <windows.h>

BOOL cdeck_change_volume(float difference);
BOOL cdeck_toggle_mute(void);

#endif
```

### 12.5 `src/audio.c`

```c
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
```

### 12.6 `src/apps.h`

```c
#ifndef CDECK_APPS_H
#define CDECK_APPS_H

#include <windows.h>

BOOL cdeck_open_application(const wchar_t *path);

#endif
```

### 12.7 `src/apps.c`

```c
#include <windows.h>
#include <shellapi.h>
#include "apps.h"

BOOL cdeck_open_application(const wchar_t *path) {
    HINSTANCE result = ShellExecuteW(NULL, L"open", path, NULL, NULL, SW_SHOWNORMAL);
    return (INT_PTR)result > 32;
}
```

## 13. Glossário rápido

| Termo | Significado |
|---|---|
| API | Interface que permite a um programa utilizar funcionalidades de outro sistema |
| Win32 | Conjunto de APIs nativas do Windows |
| C11 | Padrão da linguagem C usado na compilação |
| COM | Modelo do Windows para criação e comunicação com objetos por interfaces |
| HRESULT | Tipo de retorno usado por muitas APIs COM para indicar sucesso ou falha |
| HWND | Identificador de uma janela ou controle |
| LPARAM / WPARAM | Tipos usados para transportar parâmetros de mensagens Win32 |
| Callback | Função chamada pelo sistema em resposta a um evento |
| Endpoint de áudio | Dispositivo lógico de entrada ou saída de áudio |
| Scalar | Valor normalizado, aqui entre 0.0 e 1.0 |
| Linker | Etapa que combina objetos compilados e bibliotecas para gerar o executável |
| Artifact | Arquivo produzido e armazenado por uma execução de CI |

## 14. Referência oficial e arquivos de trabalho

- Repositório: https://github.com/s4muelxl/C-Deck
- Código C: https://github.com/s4muelxl/C-Deck/tree/main/src
- Script de build: https://github.com/s4muelxl/C-Deck/blob/main/scripts/build.ps1
- Instalador: https://github.com/s4muelxl/C-Deck/blob/main/installer/C-Deck.iss
- CI: https://github.com/s4muelxl/C-Deck/blob/main/.github/workflows/ci.yml
- Release workflow: https://github.com/s4muelxl/C-Deck/blob/main/.github/workflows/release.yml

**Regra para a equipe:** antes de alterar o código, atualize sua cópia local com a branch `main`. Se o código mudar, atualize este guia para que os trechos reproduzidos continuem correspondendo à implementação real.

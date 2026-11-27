# C-Deck — Roteiro de Apresentação Acadêmica

## 1. Identificação do Projeto

- **Nome:** C-Deck (versão 1.6.0)
- **Plataforma alvo:** Windows 10 e Windows 11 (x64)
- **Linguagem e bibliotecas:** C11, Raylib 5.5, GLFW 3.4, Win32 API, COM / Windows Core Audio e Inno Setup
- **Repositório:** https://github.com/s4muelxl/C-Deck

## 2. Problema e Objetivo

Controladores físicos de atalhos (como consoles de mesa estilo Stream Deck) possuem custo elevado e muitas aplicações equivalentes em software dependem de runtimes pesados baseados em navegador embutido. O objetivo do **C-Deck** é demonstrar a construção de um painel utilitário compacto diretamente em **C11**, consumindo poucos recursos e interagindo com o sistema operacional por meio de APIs nativas do Windows.

## 3. Escopo Funcional

A aplicação disponibiliza em uma única janela (900 × 640 px):

1. **Controle deslizante de volume:** leitura inicial e ajuste contínuo do dispositivo padrão de reprodução do Windows.
2. **VOLUME -:** redução do volume principal em 5 pontos percentuais.
3. **MUTE:** alternância entre áudio ativo e silenciado.
4. **VOLUME +:** incremento do volume principal em 5 pontos percentuais.
5. **CALCULATOR:** abertura da Calculadora (`calc.exe`).
6. **CAPTURA:** aciona a sobreposição da Ferramenta de Captura (`Win + Shift + S`) por `SendInput`.
7. **MIC MUTE:** alterna o estado de silêncio do microfone padrão de comunicação via Core Audio (`eCapture`).

## 4. Arquitetura Modular

O código-fonte em `src/` separa interface gráfica, inicialização de sistema e chamadas operacionais em quatro módulos:

| Módulo | Arquivos | Responsabilidade |
|---|---|---|
| Entrada e COM | `src/main.c` | Inicializa o modelo COM (`COINIT_APARTMENTTHREADED`), trata códigos `HRESULT` e executa `cdeck_run()` |
| Interface gráfica | `src/interface.c` / `src/interface.h` | Inicializa janela Raylib, captura eventos de mouse, desenha barra/cards e registra o log de diagnóstico |
| Controle de áudio | `src/audio.c` / `src/audio.h` | Acessa `IMMDeviceEnumerator` e `IAudioEndpointVolume` via Windows Core Audio para saída (`eRender`) e microfone (`eCapture`) |
| Atalhos de sistema | `src/apps.c` / `src/apps.h` | Abre a Calculadora via `ShellExecuteW` e aciona a Ferramenta de Captura via `SendInput` |

## 5. Decisões Técnicas e Desafios Resolvidos

Durante o desenvolvimento e validação em máquinas Windows reais, identificou-se que a biblioteca Raylib distribuída por padrão exige **OpenGL 3.3 Core Profile** (`WGL_ARB_create_context_profile`) e que o GLFW padrão descarta formatos de pixel renderizados por software (`PFD_GENERIC_FORMAT`). Em computadores com GPUs integradas antigas (OpenGL 2.1), adaptadores básicos de vídeo ou máquinas virtuais, a janela falhava antes de abrir (erros GLFW `65543` e `65542`).

A solução definitiva implementada no projeto envolveu:

1. Compilar o **Raylib 5.5** com `OPENGL_VERSION=1.1` (`scripts/build-raylib.ps1`), utilizando o pipeline compatível com qualquer versão OpenGL disponível no Windows.
2. Aplicar o patch [`patches/glfw-wgl-software-fallback.patch`](patches/glfw-wgl-software-fallback.patch) no **GLFW 3.4** para priorizar drivers de hardware e recorrer ao renderizador `GDI Generic` apenas quando nenhum formato acelerado estiver presente.
3. Unificar o registro de inicialização em UTF-8 em `%TEMP%\C-Deck-startup.log`, sondando os formatos WGL antes da abertura da janela e exibindo mensagens explícitas via `MessageBoxW` caso ocorra alguma falha de dispositivo.

## 6. Divisão Sugerida para Apresentação em Grupo

### Integrante 1 — Arquitetura, Entrada e COM (`src/main.c` e `src/interface.h`)
- Explicar o ponto de entrada `WinMain` no subsistema gráfico do Windows (`-mwindows`).
- Justificar a chamada a `CoInitializeEx(NULL, COINIT_APARTMENTTHREADED)` antes do uso da Core Audio API e o tratamento dos retornos `S_OK`, `S_FALSE`, `RPC_E_CHANGED_MODE` e erro fatal.
- Apresentar a separação em arquivos `.c` e `.h` e o papel do registro de diagnóstico (`cdeck_log`).

### Integrante 2 — Interface Gráfica e Compatibilidade OpenGL (`src/interface.c` e `patches/`)
- Explicar o isolamento de macros entre `<windows.h>` e `raylib.h` (`#define Rectangle Win32Rectangle` etc.).
- Mostrar o loop principal (`while (!WindowShouldClose())`), `BeginDrawing()` / `EndDrawing()`, detecção de colisão do ponteiro (`CheckCollisionPointRec`) e o despacho de ações via `run_action()`.
- Explicar por que `SetConfigFlags(0)`, `OPENGL_VERSION=1.1` e o fallback WGL garantem abertura da janela tanto em GPUs antigas quanto em ambientes sem driver 3D.

### Integrante 3 — Áudio, Atalhos e Empacotamento (`src/audio.c`, `src/apps.c`, `scripts/` e `installer/`)
- Explicar a cadeia COM de áudio: `CoCreateInstance(CLSID_MMDeviceEnumerator)` → `GetDefaultAudioEndpoint(eRender, eMultimedia)` → `Activate(IID_IAudioEndpointVolume)` → liberação explícita com `Release()`.
- Explicar `ShellExecuteW` e a convenção de retorno `> 32`.
- Demonstrar o script de build (`scripts/build.ps1`), verificação de dependências dinâmicas (`libraylib.dll` e `glfw3.dll`) e o instalador gerado pelo Inno Setup (`installer/C-Deck.iss`).

## 7. Roteiro de Demonstração Prática

1. Executar o instalador `C-Deck-Setup.exe` ou abrir o aplicativo instalado pelo menu Iniciar.
2. Mostrar o log gerado em `%TEMP%\C-Deck-startup.log` comprovando a inicialização do COM, os formatos WGL detectados e o driver OpenGL ativo.
3. Arrastar a barra de volume e clicar em **VOLUME -** e **VOLUME +**, acompanhando a alteração no mixer do Windows.
4. Clicar em **MUTE** duas vezes para validar a alternância de silêncio.
5. Clicar em **CALCULATOR**, **CAPTURA** e **MIC MUTE**, validando a calculadora, a sobreposição de captura e o mute independente do microfone.
6. Pressionar `ESC` para encerrar o aplicativo e verificar o retorno limpo no log.

## 8. Conclusão

O C-Deck entrega uma aplicação desktop funcional, de arquitetura enxuta e totalmente reproduzível a partir do código-fonte em C11. A separação clara entre interface, controle de áudio e abertura de processos, aliada à compilação controlada de suas dependências gráficas e ao empacotamento automatizado, torna o projeto adequado tanto para uso prático quanto para estudo de programação de sistemas em Windows.

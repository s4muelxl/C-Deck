# C-Deck

Painel de controle compacto para Windows desenvolvido em linguagem C (C11), com interface gráfica em Raylib e integração direta com as APIs nativas do sistema operacional.

Versão 1.5.2.

## Objetivo

O projeto demonstra o desenvolvimento de uma aplicação desktop nativa em C para Windows 10 e Windows 11, unindo renderização 2D por GPU/software (Raylib e GLFW), manipulação direta de áudio do sistema via COM (`IAudioEndpointVolume`) e acionamento de utilitários do sistema operacional via `ShellExecuteW`, sem dependência de navegador embutido ou servidor local.

## Funcionalidades

A janela principal apresenta um controle deslizante contínuo de volume e seis botões fixos organizados em duas linhas e três colunas:

| Controle | Ação | API Windows / Módulo |
|---|---|---|
| `VOLUME -` | Reduz o volume principal em 5 pontos percentuais | Core Audio (`audio.c`) |
| `MUTE` | Alterna o estado de silêncio da saída padrão | Core Audio (`audio.c`) |
| `VOLUME +` | Aumenta o volume principal em 5 pontos percentuais | Core Audio (`audio.c`) |
| `NOTEPAD` | Abre o Bloco de Notas (`notepad.exe`) | `ShellExecuteW` (`apps.c`) |
| `CALCULATOR` | Abre a Calculadora (`calc.exe`) | `ShellExecuteW` (`apps.c`) |
| `PAINT` | Abre o Microsoft Paint (`mspaint.exe`) | `ShellExecuteW` (`apps.c`) |

O controle deslizante superior consulta o nível atual do dispositivo padrão de reprodução na inicialização e aplica alterações em tempo real conforme o arraste do mouse.

## Arquitetura do código

O projeto adota divisão modular em quatro unidades de compilação:

```text
C-Deck/
├── src/
│   ├── main.c          # Ponto de entrada WinMain e ciclo de vida COM
│   ├── interface.c/h   # Janela Raylib, renderização, eventos e log de diagnóstico
│   ├── audio.c/h       # Controle de volume e mudo via Windows Core Audio
│   └── apps.c/h        # Execução de aplicativos externos via ShellExecuteW
├── patches/
│   └── glfw-wgl-software-fallback.patch
├── scripts/
│   ├── build-raylib.ps1
│   ├── build.ps1
│   └── check-version.ps1
├── installer/
│   ├── C-Deck.iss
│   └── C-Deck.manifest
├── docs/
│   ├── CODE-EXPLICADO.md
│   └── GUIA-TECNICO-PASSO-A-PASSO.md
├── web/                # Página de apresentação e testes de contrato
├── APRESENTACAO.md     # Roteiro de apresentação acadêmica
└── README.md
```

## Compatibilidade gráfica e diagnóstico

As bibliotecas gráficas padrão distribuídas pré-compiladas exigem **OpenGL 3.3 Core Profile** (`WGL_ARB_create_context_profile`) e rejeitam formatos de pixel genéricos do Windows. Em máquinas com GPUs integradas mais antigas (OpenGL 2.1), adaptadores básicos de vídeo, máquinas virtuais sem aceleração 3D ou sessões remotas, essa exigência impede a criação da janela.

Para garantir execução ampla em Windows 10 e Windows 11:

- O script [`scripts/build-raylib.ps1`](scripts/build-raylib.ps1) compila o **Raylib 5.5** com `OPENGL_VERSION=1.1`, preservando a aceleração por hardware quando disponível (OpenGL 1.1 a 4.x) sem exigir Core Profile.
- O **GLFW 3.4** recebe o patch [`patches/glfw-wgl-software-fallback.patch`](patches/glfw-wgl-software-fallback.patch), mantendo prioridade para drivers ICD de hardware e habilitando fallback para o renderizador `GDI Generic` apenas quando nenhum formato acelerado é encontrado.
- O aplicativo não depende de MSAA (`SetConfigFlags(0)`) e registra cada etapa da inicialização, incluindo formatos WGL detectados e mensagens internas da Raylib/GLFW, em `%TEMP%\C-Deck-startup.log`.

## Compilação no Windows

Instale o MSYS2 (ambiente **UCRT64**) com GCC, CMake e Ninja:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-cmake mingw-w64-ucrt-x86_64-ninja
```

Adicione `C:\msys64\ucrt64\bin` ao `PATH` do sistema e execute na raiz do repositório:

```powershell
.\scripts\build.ps1
```

Serão gerados em `build/` o executável `C-Deck.exe`, as bibliotecas `libraylib.dll` e `glfw3.dll`, além do arquivo `README.md`.

### Geração do instalador

Com o **Inno Setup 6** instalado, execute após o build:

```powershell
& "$env:ProgramFiles (x86)\Inno Setup 6\ISCC.exe" installer\C-Deck.iss
```

O instalador será salvo em `build\installer\C-Deck-Setup.exe`.

## Verificação e testes

Os testes de contrato e consistência de versão podem ser executados localmente com Node.js e PowerShell:

```powershell
node --test web/tests/ux-contract.test.js
.\scripts\check-version.ps1
```

## Documentação complementar

- [APRESENTACAO.md](APRESENTACAO.md): roteiro estruturado para apresentação acadêmica e demonstração prática.
- [docs/CODE-EXPLICADO.md](docs/CODE-EXPLICADO.md): visão resumida dos módulos e responsabilidades.
- [docs/GUIA-TECNICO-PASSO-A-PASSO.md](docs/GUIA-TECNICO-PASSO-A-PASSO.md): documentação técnica completa do código-fonte e do fluxo de execução.

## Licença

Projeto distribuído sob a licença MIT. Consulte [LICENSE](LICENSE).

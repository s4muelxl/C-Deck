# C-Deck

**Painel compacto de atalhos e controle do Windows.**

Versão 1.5.0.

O C-Deck é um aplicativo desktop desenvolvido em **C11**, com interface gráfica em **Raylib** e integração nativa com o Windows.

## Funcionalidades

| Controle | Ação |
|---|---|
| Volume - | Diminui o volume em 5 pontos percentuais |
| Mute | Ativa ou desativa o áudio |
| Volume + | Aumenta o volume em 5 pontos percentuais |
| Notepad | Abre o Bloco de Notas |
| Calculator | Abre a Calculadora |
| Paint | Abre o Paint |

O slider também consulta e altera diretamente o volume do dispositivo de reprodução padrão do Windows.

## Tecnologias

- C11
- Raylib — janela, desenho e entrada do usuário
- Windows Core Audio — volume e mudo
- Win32 / COM — integração com o sistema
- ShellExecuteW — abertura de aplicativos
- MinGW-w64 GCC e MSYS2 UCRT64

## Estrutura

```text
C-Deck/
├── src/
│   ├── main.c          # Inicialização COM e entrada Windows
│   ├── interface.c/h   # Interface Raylib e eventos
│   ├── audio.c/h       # Volume e mudo via Windows Core Audio
│   └── apps.c/h        # Abertura dos aplicativos via ShellExecuteW
├── scripts/
│   └── build.ps1
├── installer/
├── docs/
└── README.md
```

## Compilar no Windows

Instale MSYS2 UCRT64 com GCC, windres e Raylib:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib
```

Adicione `C:\msys64\ucrt64\bin` ao PATH do Windows. Depois, na pasta do projeto, execute:

```powershell
.\scripts\build.ps1
```

O executável e a DLL de runtime do Raylib serão gerados/copied para `build/`.

## Arquitetura

A Raylib é responsável somente pela interface e pela captura de interações. As operações de áudio continuam no Windows Core Audio, e os atalhos de aplicativos continuam usando ShellExecuteW. Isso mantém a separação entre apresentação e funcionalidades do sistema.

## Licença

MIT. Consulte [LICENSE](LICENSE).

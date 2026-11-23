# C-DECK

**Painel de controles rápidos para Windows, desenvolvido em C11 e Win32 API.**

O C-Deck é uma aplicação acadêmica simples com seis botões: três controles de áudio e três atalhos para programas do Windows.

## Funcionalidades

1. **Volume +** — aumenta o volume principal em 5%.
2. **Volume -** — diminui o volume principal em 5%.
3. **Mudo** — ativa ou desativa o som.
4. **Bloco de Notas** — abre o Notepad.
5. **Calculadora** — abre a Calculadora do Windows.
6. **Paint** — abre o Microsoft Paint.

O programa usa a API Win32 para criar a janela e os botões, a API Core Audio para controlar o volume e `ShellExecuteW` para abrir os aplicativos.

## Código

Toda a lógica do aplicativo está em **um único arquivo: `src/main.c`**.

- `get_audio_volume()`: acessa o dispositivo de áudio padrão.
- `change_volume()`: altera o volume em passos de 5%.
- `toggle_mute()`: alterna entre som ligado e mudo.
- `open_application()`: abre um programa do Windows.
- `window_procedure()`: cria os botões e trata os cliques.
- `WinMain()`: inicia a janela e o loop principal.

## Divisão da apresentação

| Integrante | Conteúdo |
|---|---|
| 1 — Interface | `WinMain()`, janela, botões e eventos `WM_COMMAND` |
| 2 — Áudio | `get_audio_volume()`, `change_volume()` e `toggle_mute()` |
| 3 — Aplicativos | `open_application()` e os três atalhos |

## Compilar no Windows

Requisitos: Windows 10/11 e MinGW-w64 GCC com `windres` no PATH.

Na raiz do projeto, execute:

```powershell
.\scripts\build.ps1
```

O executável será gerado em `build/C-Deck.exe`.

## Estrutura principal

```text
C-Deck/
├── src/
│   └── main.c
├── scripts/
│   └── build.ps1
├── installer/
├── brand/
├── web/
└── README.md
```

## Licença

MIT. Consulte [LICENSE](LICENSE).

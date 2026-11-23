# C-DECK

**Painel de controles rápidos para Windows, desenvolvido em C11 e Win32 API. Versão 1.4.0.**

O C-Deck foi simplificado para uma versão acadêmica fácil de demonstrar e explicar. O aplicativo tem apenas seis botões: três controles de áudio e três atalhos para programas do Windows.

## Funcionalidades

1. **Volume +** — aumenta o volume principal em 5%.
2. **Volume -** — diminui o volume principal em 5%.
3. **Mudo** — ativa ou desativa o som.
4. **Bloco de Notas** — abre o Notepad.
5. **Calculadora** — abre a Calculadora do Windows.
6. **Paint** — abre o Microsoft Paint.

O programa usa a API Win32 para criar a janela e os botões, a API Core Audio para controlar o volume e `ShellExecuteW` para abrir os aplicativos.

## Como o código funciona

Todo o aplicativo acadêmico está em **um único arquivo: `src/main.c`**.

- `get_audio_volume()`: encontra o dispositivo de áudio padrão do Windows.
- `change_volume()`: altera o volume em passos de 5%.
- `toggle_mute()`: alterna entre som ligado e mudo.
- `open_application()`: abre um programa do Windows.
- `window_procedure()`: cria os seis botões e recebe os cliques.
- `WinMain()`: inicia o programa e mantém a janela aberta.

## Divisão para apresentação

| Integrante | O que explica |
|---|---|
| 1 — Interface | `WinMain()`, criação da janela, botões e `WM_COMMAND` |
| 2 — Áudio | `get_audio_volume()`, `change_volume()` e `toggle_mute()` |
| 3 — Aplicativos | `open_application()` e os três atalhos |

## Compilar no Windows

Requisitos: Windows 10/11 e MinGW-w64 GCC no PATH.

Na raiz do projeto, execute:

```powershell
.\scripts\build.ps1
```

O executável será gerado em `build/C-Deck.exe`.

## Executar

Abra `build/C-Deck.exe`. Não é necessário configurar perfis, macros ou arquivos de atalhos.

## Estrutura principal

```text
C-Deck/
├── src/
│   └── main.c
├── scripts/
│   └── build.ps1
├── installer/
│   └── C-Deck.iss
├── web/
└── README.md
```

Os módulos e testes avançados que existiam na versão anterior foram mantidos no repositório para referência e compatibilidade com os testes antigos, mas não são compilados nem utilizados pelo aplicativo acadêmico simplificado.

## Licença

MIT. Consulte [LICENSE](LICENSE).

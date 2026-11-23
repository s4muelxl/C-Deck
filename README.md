# C-Deck

**Painel de atalhos e controles de áudio para Windows.**

Versão 1.4.0.

O C-Deck é um aplicativo desktop desenvolvido em C11 com a API Win32. Ele reúne seis ações em uma única janela:

- **Volume +** — aumenta o volume em 5%.
- **Volume -** — diminui o volume em 5%.
- **Mudo** — ativa ou desativa o áudio.
- **Bloco de Notas** — abre o Notepad.
- **Calculadora** — abre a Calculadora do Windows.
- **Paint** — abre o Paint.

## Tecnologias

- C11
- Windows API (Win32)
- Windows Core Audio
- MinGW-w64 GCC
- Inno Setup

## Organização

```text
C-Deck/
├── src/
│   └── main.c
├── scripts/
│   ├── build.ps1
│   └── check-version.ps1
├── installer/
│   ├── C-Deck.iss
│   └── C-Deck.manifest
├── brand/
├── web/
└── README.md
```

Toda a lógica do aplicativo está em `src/main.c`. As funções principais são:

| Função | Responsabilidade |
|---|---|
| `get_audio_volume()` | Acessa o dispositivo de áudio padrão |
| `change_volume()` | Ajusta o volume em passos de 5% |
| `toggle_mute()` | Alterna o estado de mudo |
| `open_application()` | Abre um programa do Windows |
| `window_procedure()` | Cria a interface e trata os cliques |
| `WinMain()` | Inicializa a aplicação |

## Código explicado

Consulte [`docs/CODE-EXPLICADO.md`](docs/CODE-EXPLICADO.md) para ver o código-fonte completo de `src/main.c`, com explicação por função e roteiro de apresentação.

## Compilar

Requisitos: Windows 10/11 e MinGW-w64 com GCC e `windres` disponíveis no PATH.

Na pasta do projeto, execute:

```powershell
.\scripts\build.ps1
```

O executável será criado em `build/C-Deck.exe`.

## Instalador

O instalador é gerado pelo GitHub Actions usando Inno Setup. Ele instala o executável e o README, cria um atalho opcional na área de trabalho e permite abrir o aplicativo ao final.

## Divisão da apresentação

| Integrante | Conteúdo |
|---|---|
| 1 — Interface | Janela, botões e eventos |
| 2 — Áudio | Volume e mudo |
| 3 — Aplicativos | Abertura dos três programas |

## Licença

MIT. Consulte [LICENSE](LICENSE).

**Repositório:** https://github.com/s4muelxl/C-Deck  
**Página do projeto:** https://c-deck.vercel.app/

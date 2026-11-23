# C-Deck

**Painel compacto de atalhos para Windows.**

Versão 1.5.0.

O C-Deck é um aplicativo desktop feito em **C11 e Win32**. A janela tem seis botões, organizados em duas colunas e três linhas:

| Botão | Ação |
|---|---|
| Volume + | Aumenta o volume em 5% |
| Volume - | Diminui o volume em 5% |
| Mudo | Ativa ou desativa o áudio |
| Bloco de Notas | Abre o Notepad |
| Calculadora | Abre a Calculadora |
| Paint | Abre o Paint |

## Tecnologias

- C11
- Windows API (Win32)
- Windows Core Audio
- MinGW-w64 GCC
- Inno Setup

## Estrutura do projeto

```text
C-Deck/
├── src/
│   ├── main.c          # Inicialização do programa
│   ├── interface.c/h   # Janela, botões e cliques
│   ├── audio.c/h       # Volume e mudo
│   └── apps.c/h        # Abertura dos programas
├── scripts/
│   ├── build.ps1
│   └── check-version.ps1
├── installer/
├── docs/
├── web/
└── README.md
```

Cada parte tem uma responsabilidade direta. Não há banco de dados, plugins, rede ou configurações adicionais.

## Compilar no Windows

Requisitos: Windows 10/11 e MinGW-w64 com GCC e `windres` no PATH.

Na pasta do projeto, execute:

```powershell
.\scripts\build.ps1
```

O executável será criado em `build/C-Deck.exe`.

## Instalador

O GitHub Actions compila o aplicativo e cria `C-Deck-Setup.exe` com Inno Setup. O instalador inclui o executável e este README.

## Divisão para apresentação

1. **Interface:** `interface.c/h` — janela, botões e eventos.
2. **Áudio:** `audio.c/h` — volume e mudo.
3. **Atalhos:** `apps.c/h` — abertura dos três programas.

## Documentação

Consulte [Código explicado](docs/CODE-EXPLICADO.md) para entender a estrutura e as funções.

## Licença

MIT. Consulte [LICENSE](LICENSE).

**Repositório:** https://github.com/s4muelxl/C-Deck  
**Página do projeto:** https://c-deck.vercel.app/

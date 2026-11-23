# C-Deck — versão acadêmica simplificada

O C-Deck é um painel de seis botões para Windows, desenvolvido em C11 com a Win32 API. Versão 1.4.0.

## Funcionalidades

| Botão | Função |
|---|---|
| Volume + | Aumenta o volume em 5% |
| Volume - | Diminui o volume em 5% |
| Mute | Ativa ou desativa o mudo |
| Bloco de Notas | Abre o Notepad |
| Calculadora | Abre a Calculadora do Windows |
| Paint | Abre o Paint |

## Como funciona

O programa principal está em src/simple_deck.c.

1. WinMain inicia o programa e cria a janela.
2. window_proc identifica qual botão foi pressionado.
3. change_volume e toggle_mute controlam o áudio do Windows.
4. open_app abre um dos três aplicativos.

O áudio usa a API Core Audio do Windows. Os aplicativos são iniciados com ShellExecuteW.

## Estrutura

    C-Deck/
    ├── src/
    │   └── simple_deck.c
    ├── scripts/
    │   └── build.ps1
    ├── installer/
    │   └── C-Deck.iss
    ├── web/
    └── README.md

Os módulos da versão avançada permanecem no repositório, mas não fazem parte da compilação acadêmica simplificada.

## Compilar

Requisitos: Windows 10/11, MinGW-w64 GCC e PowerShell.

Na raiz do projeto, execute:

    .\scripts\build.ps1

O executável será criado em build/C-Deck.exe.

## Divisão da apresentação

- Integrante 1 — Interface: janela, botões e identificação dos comandos.
- Integrante 2 — Áudio: aumento, redução e mute usando Core Audio.
- Integrante 3 — Aplicativos: abertura do Notepad, Calculadora e Paint.

## Limitações

- Os três aplicativos são fixos no código.
- O volume é alterado em passos de 5%.
- O áudio controla o dispositivo de reprodução padrão do Windows.
- Não há perfis, macros, biblioteca de ações ou atalhos globais.

## Licença

MIT. Consulte LICENSE.

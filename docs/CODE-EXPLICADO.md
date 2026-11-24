# C-Deck — Código explicado

**Linguagem:** C11  
**Interface gráfica:** Raylib  
**Áudio:** Windows Core Audio  
**Atalhos:** ShellExecuteW

## Estrutura

| Arquivo | Responsabilidade |
|---|---|
| `main.c` | Inicializa COM e inicia a aplicação |
| `interface.c/h` | Desenha a interface Raylib e trata as interações |
| `audio.c/h` | Consulta e controla volume/mudo pelo Windows Core Audio |
| `apps.c/h` | Abre aplicativos com ShellExecuteW |

## Fluxo

1. O Windows inicia a aplicação por WinMain.
2. CoInitializeEx inicializa COM.
3. cdeck_run inicializa a janela Raylib.
4. Raylib captura mouse e desenha a interface a cada quadro.
5. Os cliques nos cards chamam as funções de áudio ou abertura de aplicativos.
6. O slider chama cdeck_set_volume para atualizar o volume real do Windows.
7. Ao fechar a janela, CloseWindow libera os recursos da Raylib e CoUninitialize encerra COM.

## Interface — interface.c

Raylib é responsável pela janela, renderização dos textos, cards, barra de volume e leitura de mouse. Os seis cards preservam as ações existentes: diminuir volume, alternar mudo, aumentar volume, abrir Bloco de Notas, Calculadora e Paint.

A interface não executa diretamente operações de sistema. Ela encaminha as ações para os módulos especializados.

## Áudio — audio.c

get_audio_volume localiza o dispositivo padrão de reprodução e obtém IAudioEndpointVolume.

- cdeck_get_volume consulta o nível atual na escala de 0.0 a 1.0.
- cdeck_set_volume aplica um nível absoluto, limitado entre 0.0 e 1.0.
- cdeck_change_volume altera o nível relativo em pontos percentuais.
- cdeck_toggle_mute alterna o estado de mudo.
- cdeck_get_mute_state consulta o estado de mudo.

## Aplicativos — apps.c

cdeck_open_application usa ShellExecuteW para solicitar ao Windows a abertura de notepad.exe, calc.exe ou mspaint.exe.

## Compilação

No MSYS2 UCRT64, instale GCC e Raylib:

```bash
pacman -S --needed mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-raylib
```

A compilação vincula Raylib, OpenGL, GDI, WinMM e as bibliotecas necessárias da API do Windows. O runtime libraylib.dll deve acompanhar o executável.

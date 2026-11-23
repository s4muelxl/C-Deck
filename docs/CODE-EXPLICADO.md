# C-Deck 1.5.0 — Código explicado

**Linguagem:** C11  
**Sistema:** Windows 10/11 x64  
**Interface:** Win32 API  
**Áudio:** Windows Core Audio

O C-Deck tem somente seis ações: aumentar volume, diminuir volume, silenciar, abrir Bloco de Notas, abrir Calculadora e abrir Paint. A interface é uma janela compacta com duas colunas e três linhas.

## 1. Estrutura

O código foi separado em quatro partes pequenas:

| Arquivo | Responsabilidade |
|---|---|
| `main.c` | Inicializa COM e inicia a janela |
| `interface.c/h` | Cria a janela, os seis botões e trata os cliques |
| `audio.c/h` | Controla volume e mudo pelo Windows Core Audio |
| `apps.c/h` | Abre os três aplicativos do Windows |

Não existe banco de dados, rede, sistema de plugins ou camada de configuração.

## 2. Fluxo do programa

1. O Windows chama `WinMain()` em `main.c`.
2. `CoInitializeEx()` inicializa COM, necessário para acessar o Core Audio.
3. `cdeck_run()` registra e cria a janela.
4. A função `window_procedure()` recebe os eventos do Windows.
5. Quando um botão é clicado, `WM_COMMAND` identifica seu ID e chama a função correspondente.
6. Ao fechar a janela, `WM_DESTROY` encerra o loop.

## 3. Áudio — `audio.c`

A função privada `get_audio_volume()` localiza o dispositivo de saída padrão e obtém a interface `IAudioEndpointVolume`.

- `cdeck_change_volume(0.05f)`: aumenta o volume em cinco pontos percentuais.
- `cdeck_change_volume(-0.05f)`: diminui o volume em cinco pontos percentuais.
- `cdeck_toggle_mute()`: inverte o estado de mudo.

O volume é limitado ao intervalo de 0.0 a 1.0. Os objetos COM são liberados após cada operação.

## 4. Atalhos — `apps.c`

`cdeck_open_application()` usa `ShellExecuteW()` para solicitar ao Windows a abertura de um programa:

- `notepad.exe`
- `calc.exe`
- `mspaint.exe`

A função retorna verdadeiro quando o Windows aceita a solicitação.

## 5. Interface — `interface.c`

Os seis botões são controles nativos Win32. As posições são definidas por constantes no início do arquivo: largura, altura, colunas e espaçamento entre linhas.

A janela usa um tamanho fixo e compacto. O texto na parte inferior informa o resultado da última ação.

## 6. Compilação

No Windows 10/11 com MinGW-w64 (GCC e windres) no PATH:

```powershell
.\scripts\build.ps1
```

O executável é gerado em `build/C-Deck.exe`. O GitHub Actions usa os mesmos quatro arquivos C para validar a compilação e gerar o instalador.

## 7. Divisão para apresentação

1. **Interface:** `interface.c/h` — janela, botões, IDs e eventos.
2. **Áudio:** `audio.c/h` — acesso ao dispositivo, volume e mudo.
3. **Atalhos:** `apps.c/h` — chamadas para abrir os aplicativos.

## 8. Resumo

O projeto demonstra conceitos fundamentais de C e programação para Windows: funções, arquivos de cabeçalho, separação de responsabilidades, eventos de interface, chamadas à API do sistema e gerenciamento básico de objetos COM.

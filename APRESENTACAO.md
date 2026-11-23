# Apresentação do C-Deck

## 1. Objetivo

O C-Deck é um painel de atalhos para Windows. Ele reúne controles básicos de áudio e atalhos para programas em uma janela simples, desenvolvida em linguagem C.

## 2. Funcionalidades

- Aumentar o volume do Windows em 5%.
- Diminuir o volume do Windows em 5%.
- Ativar ou desativar o mudo.
- Abrir o Bloco de Notas.
- Abrir a Calculadora.
- Abrir o Paint.

## 3. Tecnologias utilizadas

- **C11:** linguagem utilizada no aplicativo.
- **Win32 API:** criação da janela e dos botões.
- **Windows Core Audio:** controle do volume e do mudo.
- **ShellExecuteW:** abertura dos programas.
- **MinGW-w64 GCC:** compilação.
- **Inno Setup:** criação do instalador.
- **GitHub Actions:** compilação automatizada.

## 4. Organização do código

Toda a lógica do aplicativo está em `src/main.c`.

| Função | Responsabilidade |
|---|---|
| `get_audio_volume()` | Obtém o dispositivo de áudio padrão |
| `change_volume()` | Aumenta ou diminui o volume |
| `toggle_mute()` | Liga ou desliga o mudo |
| `open_application()` | Abre um aplicativo |
| `window_procedure()` | Cria os botões e recebe os cliques |
| `WinMain()` | Inicia a janela e o loop de mensagens |

## 5. Divisão entre os integrantes

1. **Interface:** janela, botões e tratamento dos eventos.
2. **Áudio:** acesso ao dispositivo padrão, volume e mudo.
3. **Atalhos:** abertura do Bloco de Notas, Calculadora e Paint.

## 6. Como demonstrar

1. Abrir o C-Deck.
2. Clicar em Volume + e Volume -.
3. Ativar e desativar o mudo.
4. Abrir cada um dos três programas.

## 7. Compilação

Com MinGW-w64 instalado no Windows, execute:

```powershell
.\scripts\build.ps1
```

O arquivo `build/C-Deck.exe` será gerado.

## 8. Conclusão

O C-Deck demonstra a criação de uma aplicação desktop em C, com interface Win32, controle de áudio do Windows e abertura de programas. O projeto mantém uma estrutura pequena e permite explicar suas funções diretamente no código.

**Repositório:** https://github.com/s4muelxl/C-Deck

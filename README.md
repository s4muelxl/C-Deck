# C-DECK

**Painel de atalhos para Windows, desenvolvido em C11 e Win32 API.**

O C-DECK reúne controles básicos de áudio e atalhos para aplicativos em uma pequena janela desktop. A página em `web/` é apenas a apresentação do projeto; as ações do computador são executadas pelo aplicativo Windows.

## Recursos

- Aumentar e diminuir o volume principal em passos de 5%, ou ajustar diretamente em um controle deslizante com percentual atualizado.
- Alternar o mute do dispositivo de reprodução padrão.
- Enviar comandos de play/pause, próxima faixa e faixa anterior aos controles de mídia do Windows.
- Abrir até três aplicativos definidos pelo usuário.
- Configurar nomes e caminhos dos atalhos pela própria interface.
- Salvar as alterações em `config/apps.cfg`, sem editar o arquivo manualmente.
- Atalhos globais padrão: `Ctrl + Alt + ↑` (aumentar volume), `Ctrl + Alt + ↓` (reduzir volume) e `Ctrl + Alt + M` (mutar).\n- Minimização para a bandeja do Windows: minimizar ou fechar a janela mantém o C-Deck ativo; use o menu do ícone para reabrir ou sair.\n- Interface Win32 nativa, com tema escuro e janela redimensionável.

## Baixar e executar

1. Abra a aba **Actions** do repositório.
2. Entre na execução mais recente do workflow **C-Deck CI** que terminou com sucesso.
3. Na seção **Artifacts**, baixe `C-Deck-Windows`.
4. Para instalação guiada, execute `C-Deck-Setup.exe`. O instalador cria um atalho no menu Iniciar e permite criar um atalho na área de trabalho.
5. Se preferir não instalar, extraia `C-Deck-Portable.zip` para uma pasta e execute `C-Deck.exe`.

O pacote portátil contém:
- `C-Deck.exe`
- `config/apps.cfg`
- `README.md`

Na instalação guiada, o programa fica em `%LOCALAPPDATA%\Programs\C-Deck`. A configuração fica em uma subpasta `config`, preservada durante atualizações do aplicativo. O executável portátil também precisa permanecer ao lado da pasta `config`; não mova apenas o EXE para outro local.

## Configurar os atalhos pelo aplicativo

1. Abra o C-DECK.
2. Clique em **Configurar atalhos**.
3. Informe o nome que aparecerá no botão e o caminho do executável ou comando.
4. Para remover um atalho, deixe os dois campos correspondentes vazios.
5. Clique em **Salvar**. Os botões da janela principal são atualizados imediatamente.

O programa grava as alterações em `config/apps.cfg`. O salvamento usa um arquivo temporário e substitui o arquivo de configuração somente depois de concluir a gravação.

Também é possível editar o arquivo manualmente:

```ini
[app1]
name=Bloco de Notas
path=C:\Windows\System32\notepad.exe

[app2]
name=Calculadora
path=C:\Windows\System32\calc.exe

[app3]
name=Paint
path=mspaint.exe
```

- `name`: texto mostrado no botão.
- `path`: caminho absoluto do executável ou comando disponível no PATH.

## Compilar no Windows

Requisitos: Windows 10/11 e MinGW-w64 GCC no PATH.

Execute na raiz do repositório:

```powershell
.\scripts\build.ps1
```

O executável será criado em `build/C-Deck.exe`.

## Estrutura

- `src/main.c`: inicialização e localização da configuração.
- `src/interface.c`: janela principal e editor de atalhos Win32.
- `src/actions.c`: ações de áudio e abertura de aplicativos.
- `src/config.c`: leitura e gravação da configuração.
- `config/apps.cfg`: atalhos.
- `tests/`: testes de leitura e gravação da configuração.
- `web/`: landing page estática.
- `.github/workflows/ci.yml`: compilação, testes e empacotamento automatizados.

## Limitações atuais

- O áudio é controlado no dispositivo de reprodução padrão do Windows.
- Os atalhos globais usam combinações padrão fixas nesta versão; a personalização das combinações ainda não está disponível.\n- O volume pode ser ajustado pelo controle deslizante ou pelos botões em passos de 5%.
- Os controles de mídia enviam comandos globais do teclado multimídia; o resultado depende do aplicativo de mídia ativo.
- São suportados três atalhos configuráveis.
- Ainda não há macros, perfis, plugins, controle individual por aplicativo ou integração com hardware Stream Deck.

## Licença

MIT. Consulte [LICENSE](LICENSE).

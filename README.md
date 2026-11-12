# C-DECK

**Painel de atalhos e controles estilo Stream Deck para Windows, desenvolvido em C11 e Win32 API. Versão 1.3.1.**

O C-DECK reúne controles de áudio, comandos multimídia e atalhos para aplicativos e sites em um teclado virtual desktop estilo Stream Deck. A página em `web/` é a apresentação oficial do projeto; as ações do computador são executadas nativamente pelo aplicativo Windows.

## Identidade visual

O projeto usa duas versões oficiais da marca: o símbolo C geométrico para ícones do Windows e a assinatura horizontal C-Deck para o site e materiais. Os arquivos vetoriais ficam em `web/assets/`; `brand/build-icon.ps1` gera o `.ico` usado como recurso embutido no executável e no instalador.

## Recursos

- Aumentar e diminuir o volume principal em passos de 5%, ou ajustar diretamente em um controle deslizante com percentual atualizado.
- Alternar o mute do dispositivo de reprodução padrão.
- Enviar comandos de play/pause, próxima faixa e faixa anterior aos controles de mídia do Windows.
- Abrir até três destinos definidos pelo usuário: aplicativos Windows ou sites no navegador padrão.
- Reconhecer URLs `https://` e `http://` nos atalhos; sites recebem um ícone de globo e abrem no navegador padrão.
- Configurar nomes e caminhos dos atalhos pela própria interface.
- Exibir automaticamente o ícone original de cada executável configurado.
- Escolher opcionalmente um arquivo `.ico` personalizado para cada aplicativo, com seletor de arquivos integrado.
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
3. Informe o nome que aparecerá no botão e selecione um executável pelo botão `...`, ou cole uma URL completa começando com `https://` ou `http://` para abrir um site no navegador padrão.
4. Opcionalmente, selecione um arquivo `.ico` personalizado. Se deixar esse campo vazio, o C-Deck extrai e exibe o ícone do próprio executável.
5. Para remover um atalho, deixe nome e executável vazios.
6. Clique em **Salvar**. Os botões da janela principal são atualizados imediatamente.

O programa grava as alterações em `config/apps.cfg`. Um atalho de site usa a própria URL no campo `path`, por exemplo `https://www.youtube.com`. O salvamento usa um arquivo temporário e substitui o arquivo de configuração somente depois de concluir a gravação.

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
- `icon`: caminho opcional para um arquivo `.ico`; se omitido, o ícone é extraído do executável.

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
- Ainda não há macros, perfis múltiplos, plugins, controle individual por aplicativo ou integração com hardware Stream Deck.

## Licença

MIT. Consulte [LICENSE](LICENSE).

# C-DECK

**Painel de atalhos e controles estilo Stream Deck para Windows, desenvolvido em C11 e Win32 API. Versão 1.3.1.**

O C-DECK reúne controles de áudio, comandos multimídia e atalhos para aplicativos e sites em um teclado virtual desktop estilo Stream Deck. A página em `web/` é a apresentação oficial do projeto; as ações do computador são executadas nativamente pelo aplicativo Windows.

## Identidade visual

O projeto usa duas versões oficiais da marca: o símbolo C geométrico para ícones do Windows e a assinatura horizontal C-Deck para o site e materiais. Os arquivos vetoriais ficam em `web/assets/`; `brand/build-icon.ps1` gera o `.ico` usado como recurso embutido no executável e no instalador.

## Recursos

- Aumentar e diminuir o volume principal em passos de 5%, ou ajustar diretamente em um controle deslizante com percentual atualizado.
- Alternar o mute do dispositivo de reprodução padrão.
- Enviar comandos de play/pause, próxima faixa e faixa anterior aos controles de mídia do Windows.
- Configurar e persistir as 15 teclas da grade 5×3; cada tecla pode executar uma ação interna, abrir um aplicativo Windows ou acessar um site.
- Reconhecer URLs `https://` e `http://` nos atalhos; sites recebem um ícone de globo e abrem no navegador padrão.
- Configurar nomes e caminhos dos atalhos pela própria interface.
- Biblioteca nativa de ações por categoria — Favoritos, Sistema, Áudio, Mídia, Aplicativos e Navegação — com modelos prontos que podem ser adicionados à tecla selecionada e salvos no perfil ativo.
- Multi Action com até oito macros persistidas em `config/macros.cfg`, seleção pelo menu da categoria e execução em thread separada com limites de passos, atraso e alvos permitidos.
- Exibir automaticamente o ícone original de cada executável configurado.
- Escolher opcionalmente um arquivo `.ico` personalizado para cada aplicativo, com seletor de arquivos integrado.
- Salvar as alterações em `config/apps.cfg`, sem editar o arquivo manualmente.
- Atalhos globais padrão: `Ctrl + Alt + ↑` (aumentar volume), `Ctrl + Alt + ↓` (reduzir volume) e `Ctrl + Alt + M` (mutar). O registro é verificado individualmente; se outra aplicação já reservou uma combinação, o C-Deck informa qual falhou e mantém ativos os demais atalhos disponíveis.
- Quatro perfis integrados — Default, Streaming / OBS, Produtividade e Jogos / Gaming — com layouts independentes; cada perfil mantém suas próprias 15 teclas.
- Preferências persistentes para perfil ativo, minimização para bandeja, atalhos globais e opacidade da janela.
- Minimização para a bandeja do Windows: minimizar ou fechar a janela mantém o C-Deck ativo; use o menu do ícone para reabrir ou sair.
- Interface Win32 nativa, com tema escuro e janela redimensionável.

## Baixar e executar

1. Abra a aba **Actions** do repositório.
2. Entre na execução mais recente do workflow **C-Deck CI** que terminou com sucesso.
3. Na seção **Artifacts**, baixe `C-Deck-Windows`.
4. Para instalação guiada, execute `C-Deck-Setup.exe`. O instalador cria um atalho no menu Iniciar e permite criar um atalho na área de trabalho.
5. Se preferir não instalar, extraia `C-Deck-Portable.zip` para uma pasta e execute `C-Deck.exe`.

O pacote portátil contém:
- `C-Deck.exe`
- `config/apps.cfg`
- `config/macros.cfg.example`
- `README.md`

Na instalação guiada, o programa fica em `%LOCALAPPDATA%\Programs\C-Deck`. A configuração fica em uma subpasta `config`, preservada durante atualizações do aplicativo. O executável portátil também precisa permanecer ao lado da pasta `config`; não mova apenas o EXE para outro local.

## Biblioteca de ações

1. Selecione uma tecla na grade 5×3 para definir o destino.
2. Abra uma categoria na barra lateral.
3. Escolha uma opção no menu **Adicionar**. O modelo preenche nome, identificador/caminho e ícone do slot selecionado.
4. A ação é salva imediatamente no perfil ativo. Se a gravação falhar, o conteúdo anterior do slot é restaurado.

A biblioteca inclui ações internas de áudio e mídia, utilitários do Windows, aplicativos comuns e links web. Os modelos são apenas atalhos configuráveis: selecionar um item na biblioteca não o executa. Para executar, use a tecla depois de adicioná-la à grade. A categoria **Multi Action** lista as macros válidas encontradas em `config/macros.cfg`; escolher uma atribui a macro à tecla selecionada. Se o arquivo não existir, a categoria informa como começar usando `config/macros.cfg.example`.

## Multi Action e macros

O C-Deck carrega até oito macros de `config/macros.cfg` (esquema independente v1). Se o arquivo ainda não existir, copie `config/macros.cfg.example` para `config/macros.cfg` e ajuste os nomes e etapas. Cada macro aceita até 12 etapas, atraso individual de até 5 segundos e atraso acumulado máximo de 10 segundos. O módulo de armazenamento oferece gravação por arquivo temporário, backup `.bak` e substituição atômica. Até existir um editor gráfico, a configuração é editada manualmente: faça uma cópia antes de alterar `macros.cfg` e reinicie o C-Deck para recarregar as macros. Arquivos inválidos ou com versão desconhecida são rejeitados e nunca são sobrescritos pelo aplicativo.

Formato de cada etapa:

- `action:<id>`: apenas `volume_up`, `volume_down`, `audio_mute`, `mic_mute`, `media_play_pause`, `media_previous`, `media_next` ou `sound_devices`.
- `open:<alvo>`: somente URL `http://`/`https://`, URI `ms-settings:` ou destino `.exe`/`.lnk`. URLs podem conter host e caminho Unicode (UTF-8), mas não espaços literais; use percent-encoding para espaços em URLs. Caminhos de executáveis/atalhos podem conter espaços e caracteres Unicode, desde que terminem exatamente em `.exe` ou `.lnk`. O C-Deck passa o destino como `lpFile` de `ShellExecuteW`, sem parâmetros de linha de comando; argumentos, aspas e comandos arbitrários são rejeitados.
- `delay:<ms>`: inteiro entre 1 e 5000, respeitando o limite acumulado.

Exemplo:

```ini
[meta]
version=1

[macro1]
name=Preparar transmissão
step_count=3
step1=action:volume_down
step2=open:obs64.exe
step3=delay:500
```

A categoria Multi Action mostra apenas macros carregadas e validadas. A execução ocorre em thread separada, interrompe no primeiro passo com falha e pode ser cancelada quando o aplicativo encerra. Ainda não existe editor gráfico de macros; a configuração é feita no arquivo. Não são executados comandos arbitrários.

## Configurar os atalhos pelo aplicativo

1. Abra o C-DECK.
2. Clique em **Configurar atalhos**.
3. Informe o nome que aparecerá no botão e selecione um executável pelo botão `...`, ou cole uma URL completa começando com `https://` ou `http://` para abrir um site no navegador padrão.
4. Opcionalmente, selecione um arquivo `.ico` personalizado. Se deixar esse campo vazio, o C-Deck extrai e exibe o ícone do próprio executável.
5. Para remover um atalho, deixe nome e executável vazios.
6. Clique em **Salvar**. Os botões da janela principal são atualizados imediatamente.

O programa grava os 15 slots em `config/apps.cfg` com cabeçalho de esquema (`[meta]`, `version=3`). A leitura mantém compatibilidade com configurações legadas v1 (três ou nove destinos) e v2 (nove destinos). Na migração, os nove destinos antigos são preservados na mesma ordem e passam para as teclas 7–15; as seis primeiras teclas recebem os controles internos padrão. Ao salvar, a versão anterior é preservada em `config/apps.cfg.bak` antes da substituição atômica. Caminhos de configuração, executáveis, URLs e ícones personalizados são tratados como UTF-8 e convertidos para UTF-16 nas chamadas Win32. No esquema v3, os 15 slots são gravados explicitamente, inclusive quando vazios. Um atalho de site usa a própria URL no campo `path`, por exemplo `https://www.youtube.com`. O salvamento usa um arquivo temporário e substitui o arquivo de configuração somente depois de concluir a gravação.

Os perfis integrados usam arquivos independentes ao lado de `apps.cfg`: `apps-streaming.cfg`, `apps-productivity.cfg` e `apps-gaming.cfg`. Eles são criados na primeira ativação e também usam o esquema v3 e backup `.bak`. O perfil padrão continua em `apps.cfg`, preservando a compatibilidade com instalações anteriores. As preferências ficam em `apps-preferences.cfg` (esquema independente v1), com backup antes de cada substituição. Se um arquivo existente estiver inválido ou usar uma versão futura, o C-Deck mantém o arquivo original e carrega os padrões seguros em vez de sobrescrevê-lo.

Também é possível editar o arquivo manualmente:

```ini
[meta]
version=3

[app1]
name=Volume +
path=action:volume_up

[app2]
name=Mudo Som
path=action:audio_mute

[app10]
name=Mudo Mic
path=action:mic_mute

[app11]
name=Trocar Áudio
path=action:sound_devices
```

- `name`: texto mostrado no botão.
- `path`: caminho do executável, URL `http://`/`https://`, identificador interno `action:` ou referência `action:macro:<id>` para uma macro carregada. Ações internas disponíveis: `volume_up`, `volume_down`, `audio_mute`, `media_play_pause`, `media_previous`, `media_next`, `mic_mute` e `sound_devices`.
- `icon`: caminho opcional para um arquivo `.ico`; se omitido, o ícone é extraído do executável.

## Compilar no Windows

Requisitos: Windows 10/11 e MinGW-w64 GCC no PATH.

Execute na raiz do repositório:

```powershell
.\scripts\build.ps1
```

O executável será criado em `build/C-Deck.exe`. O build também trata casts incompatíveis de ponteiros de função como erro (`-Werror=cast-function-type`), evitando regressões na resolução dinâmica de APIs do Windows.

## Estrutura

- `src/main.c`: inicialização e localização da configuração.
- `src/interface.c`: janela principal e editor de atalhos Win32.
- `src/actions.c`: ações de áudio e abertura de aplicativos.
- `src/config.c`: leitura, migração e gravação das 15 ações.
- `src/profiles.c` / `src/profiles.h`: perfis integrados e preferências persistentes.
- `src/action_library.c` / `src/action_library.h`: catálogo imutável de modelos de ações por categoria.
- `src/macros.c` / `src/macros.h`: validação, persistência versionada e execução limitada de macros.
- `src/hotkeys.c` / `src/hotkeys.h`: reconciliação do registro de atalhos globais, rastreamento de combinações ativas e diagnóstico de conflitos.
- `src/gdi_scope.c` / `src/gdi_scope.h`: restauração de objetos GDI selecionados antes da liberação de brushes/pens temporários.
- `config/macros.cfg.example`: modelo versionado; `config/macros.cfg` é o arquivo editável do usuário.
- `config/apps.cfg`: ações do perfil padrão; arquivos dos demais perfis e preferências são gerados no primeiro uso.
- `tests/`: testes de configuração, migração, perfis, preferências, biblioteca, macros, registro de hotkeys e ciclo de vida GDI.
- `web/`: landing page estática.
- `.github/workflows/ci.yml`: compilação, testes e empacotamento automatizados.

## Limitações atuais

- O áudio é controlado no dispositivo de reprodução padrão do Windows.
- Os atalhos globais usam combinações padrão fixas nesta versão; a personalização das combinações ainda não está disponível. O registro de cada combinação é conferido separadamente. Conflitos (por exemplo, `ERROR_HOTKEY_ALREADY_REGISTERED`) aparecem na barra de status, sem impedir o registro das outras combinações. A preferência indica que o C-Deck deve tentar ativá-los; ela não garante que todos estejam disponíveis no Windows.
- O volume pode ser ajustado pelo controle deslizante ou pelos botões em passos de 5%.
- Os controles de mídia enviam comandos globais do teclado multimídia; o resultado depende do aplicativo de mídia ativo.
- Se a consulta do estado do microfone falhar, os ícones usam uma cor neutra em vez de indicar incorretamente microfone aberto ou silenciado.
- As 15 teclas da grade 5×3 são configuráveis e persistidas em `config/apps.cfg`.
- Os quatro perfis integrados são persistentes, mas ainda não há criação/renomeação de perfis personalizados, editor gráfico de macros, plugins, controle individual por aplicativo ou integração com hardware Stream Deck.

## Licença

MIT. Consulte [LICENSE](LICENSE).
